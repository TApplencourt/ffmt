#!/usr/bin/env python3
"""GPU resources of the test kernels, from a build of a build-only GPU id.

    test/run_id.sh 36 build
    python3 test/resource_usage.py measure 36 resources/head/id-36.json
    python3 test/resource_usage.py measure 36 resources/base/id-36.json --tree ../base
    python3 test/resource_usage.py compare resources/base resources/head

measure reads what the last `test/run_id.sh <id> build` of a checkout left:
which test files built (results/id-<id>/tests.log), and what the compiler
said about each kernel (asked by the FFMT_TEST_FLAGS of the id's setup), or
what the test executable embeds. Each RUN(...) is its own kernel, so a
kernel's numbers are those of the ffmt code its test inlines.

    FFMT_TEST_FLAGS                         where                       numbers
    -Xcuda-ptxas -v (CUDA, OpenMP NVPTX)    <name>.build.log            registers, stack
                                                                        frame, spill, smem
                                            (a compile-only OpenMP build has no registers:
                                            its region's code is a function of its own)
    -Rpass-analysis=kernel-resource-usage   <name>.build.log            VGPRs + AGPRs,
    (HIP)                                                               scratch, spills, LDS
    none needed (icpx AOT)                  the .ze_info of the AOT     private memory,
                                            binary in the test's        spill, SLM
                                            executable, build/id-<id>/bin
    IGC_ShaderDumpEnable (icpx AOT)         IGC's dump, build/id-<id>/  registers: GRFs
                                            igc, matched by .ze_info    the code uses

compare prints one markdown table of every id measured in both directories,
each metric's average over the kernels, base → head, then (collapsed) each
changed file's min / avg / max. It
exits 1 when a test grew (any number of a kernel measured on both sides), or
a file no longer builds while test/configs.json does not expect it to
(failed_to_compile). What has no reference, a new id, file or test, is
listed at the top, not failed.
"""

import argparse
import glob
import json
import os
import re
import struct
import sys

# In OpenCL's words: private memory is per work-item (NVIDIA's stack frame,
# AMD's scratch), local memory per work-group (NVIDIA's shared memory, AMD's
# LDS, Intel's SLM).
METRICS = ("regs", "private", "spill", "local")
STATS = ("min", "avg", "max")  # over the kernels of a test file


def new_kernel():
    return dict.fromkeys(METRICS, 0)


def parse_ptxas(text):
    """ptxas -v. An OpenMP target region's code is in K_omp_outlined...,
    which kernel K calls: in a compile-only build it stays a function of its
    own, whose stack and spills count for K, and whose registers ptxas does
    not report (they are known at link time), so K's are unknown (None)."""
    kernels, frames, name, function = {}, {}, None, None
    for line in text.splitlines():
        if m := re.search(r"Compiling entry function '([^']+)'", line):
            name = m.group(1)
            kernels[name] = new_kernel()
        elif m := re.search(r"Function properties for (\S+)", line):
            function = m.group(1)
        elif function and (
            m := re.search(r"(\d+) bytes stack frame, (\d+) bytes spill stores", line)
        ):
            frames[function] = (int(m.group(1)), int(m.group(2)))
            function = None
        elif name and (m := re.search(r"Used (\d+) registers", line)):
            kernels[name]["regs"] = int(m.group(1))
            if smem := re.search(r"(\d+) bytes smem", line):
                kernels[name]["local"] = int(smem.group(1))
            name = None
    for kernel, k in kernels.items():
        k["private"], k["spill"] = frames.get(kernel, (0, 0))
        for function, (stack, spill) in frames.items():
            if function.startswith(kernel + "_omp_outlined"):
                k["private"] += stack
                k["spill"] += spill
                k["regs"] = None
    return kernels


def parse_amd_remarks(text):
    """kernel-resource-usage remarks: `file:line:col: remark: Field: N` from a
    compile, `file:line:col: Field: N` from clang-linker-wrapper (the new
    offload driver makes the device code at link time)."""
    kernels, name = {}, None
    fields = {
        "VGPRs": "regs",
        "AGPRs": "regs",
        "ScratchSize [bytes/lane]": "private",
        "VGPRs Spill": "spill",
        "SGPRs Spill": "spill",
        "LDS Size [bytes/block]": "local",
    }
    for line in text.splitlines():
        if m := re.search(r":\d+:\d+:(?: remark:)? Function Name: (\S+)", line):
            name = m.group(1)
            kernels[name] = new_kernel()
        elif (
            name
            and (
                m := re.search(
                    r":\d+:\d+:(?: remark:)?\s+([^:\[]+(?: \[[^\]]+\])?): (\d+)", line
                )
            )
            and (key := fields.get(m.group(1).strip()))
        ):
            kernels[name][key] += int(m.group(2))
    return kernels


def embedded_sections(path, want):
    """The `want` section of each ELF image embedded in a file, such as the
    AOT GPU binary an icpx test executable carries (not the file's own)."""
    with open(path, "rb") as f:
        data = f.read()
    found, start = [], 1
    while (off := data.find(b"\x7fELF", start)) != -1:
        start = off + 1
        try:
            (shoff,) = struct.unpack_from("<Q", data, off + 0x28)
            shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, off + 0x3A)
            if (
                data[off + 4] != 2
                or shentsize != 64
                or not 0 < shnum < 65536
                or shstrndx >= shnum
            ):
                continue  # not an ELF64 image, or bytes that only look like one

            def header(i, off=off, shoff=shoff):
                return struct.unpack_from("<IIQQQQIIQQ", data, off + shoff + i * 64)

            strings = off + header(shstrndx)[4]
            for i in range(shnum):
                name, _, _, _, offset, size, *_ = header(i)
                if (
                    data[strings + name : data.index(b"\0", strings + name)]
                    == want.encode()
                ):
                    found.append(data[off + offset : off + offset + size].decode())
        except (struct.error, ValueError, UnicodeDecodeError):
            continue
    return found


def parse_zeinfo(text):
    """grf_count of execution_env; per_thread_memory_buffers by usage. Only
    the entries of kernels:, not of the other top-level lists (functions:)."""
    kernels, name, usage, section = {}, None, None, None
    for line in text.splitlines():
        if m := re.match(r"(\w+):", line):
            section, name = m.group(1), None
        elif section == "kernels" and (m := re.match(r"  - name:\s+(\S+)", line)):
            name = m.group(1)
            kernels[name] = new_kernel()
        elif name and (m := re.search(r"grf_count:\s+(\d+)", line)):
            kernels[name]["regs"] = int(m.group(1))
        elif name and (m := re.search(r"slm_size:\s+(\d+)", line)):
            kernels[name]["local"] = int(m.group(1))
        elif name and (m := re.search(r"usage:\s+(\S+)", line)):
            usage = m.group(1)
        elif name and usage and (m := re.match(r"\s+size:\s+(\d+)", line)):
            key = "spill" if usage == "spill_fill_space" else "private"
            kernels[name][key] += int(m.group(1))
            usage = None
    return kernels


def kernel_key(name):
    """A kernel's name without what depends on where its file was built: an
    OpenMP region is __omp_offloading_<device id>_<file id>_<function>_l<line>,
    and the file id changes with the checkout's path."""
    return re.sub(r"__omp_offloading_[0-9a-f]+_[0-9a-f]+_", "__omp_offloading_", name)


def summary(kernels):
    """Every kernel's numbers, and per metric their min, average (to 0.1) and
    max, for the table."""
    out = {"kernels": {kernel_key(k): v for k, v in kernels.items()}}
    for metric in METRICS:
        values = [k[metric] for k in kernels.values() if k[metric] is not None]
        out[metric] = values and {
            "min": min(values),
            "avg": round(sum(values) / len(values), 1),
            "max": max(values),
        }
    return out


def short(kernel):
    """A kernel's name as its test's place in the file: the line of an OpenMP
    region, else the RUN lambda's number in the Itanium mangling."""
    if m := re.search(r"_l(\d+)$", kernel):
        return f"line {m.group(1)}"
    # The RUN lambda closes the name: ...UlvE<n>_EvT_ (CUDA, HIP), or
    # ...E<n>_ then only E (SYCL); no <n> is the first lambda, 0 the second.
    if m := re.search(r"UlvE(\d*)_EvT_$", kernel) or re.search(r"E(\d*)_E*$", kernel):
        # SYCL also makes a RoundedRangeKernel of each, launched instead when
        # the range is not a multiple of the preferred work-group size.
        rounded = " (rounded range)" if "RoundedRangeKernel" in kernel else ""
        return f"lambda #{int(m.group(1) or -1) + 2}{rounded}"
    return kernel


def igc_dumps(directory):
    """IGC's dumps (IGC_ShaderDumpEnable in the setup), by the text of each
    module's .zeinfo: the one embedded in the test executable it built."""
    return {
        open(path).read(): path[: -len(".zeinfo")]
        for path in glob.glob(os.path.join(directory, "*.zeinfo"))
    }


def igc_registers(stem):
    """Per kernel of a dumped module, the GRFs its instructions use: the
    registers it really uses (.ze_info only has the GRF mode, 128 or 256)."""
    regs = {}
    for asm in glob.glob(stem + "_simd*_entry_*.asm"):
        with open(asm, errors="replace") as f:
            text = f.read()
        if name := re.search(r"^//\.kernel (\S+)", text, re.MULTILINE):
            code = "\n".join(
                line for line in text.splitlines() if not line.startswith("//")
            )
            regs[name.group(1)] = len(set(re.findall(r"\br(\d+)", code)))
    return regs


def file_kernels(text, binary, dumps):
    """The kernels of one test file, from its build log or its executable,
    and how their numbers were reported. An Intel kernel's registers are
    None without IGC's dump, as .ze_info does not have them."""
    if kernels := parse_ptxas(text):
        return kernels, "nvidia"
    if kernels := parse_amd_remarks(text):
        return kernels, "amd"
    if os.path.exists(binary) and (zeinfo := embedded_sections(binary, ".ze_info")):
        kernels = {}
        for z in zeinfo:
            regs = igc_registers(dumps[z]) if z in dumps else {}
            for name, k in parse_zeinfo(z).items():
                kernels[name] = {**k, "regs": regs.get(name)}
        return kernels, "intel"
    return {}, None


def measure(id_, out, tree):
    logs = os.path.join(tree, f"results/id-{id_}")
    result = {"id": id_, "backend": None, "files": {}}
    try:
        with open(os.path.join(logs, "tests.log")) as f:
            status = [line.split() for line in f]
    except FileNotFoundError:
        status = None
        result["missing"] = f"no build (test/run_id.sh {id_} build did not run)"
    dumps = igc_dumps(os.path.join(tree, f"build/id-{id_}/igc"))
    for code, step, name in (
        s for s in status or [] if len(s) == 3 and s[1] == "build"
    ):
        if code != "0":
            result["files"][name] = None
            continue
        with open(os.path.join(logs, f"{name}.build.log"), errors="replace") as f:
            text = f.read()
        kernels, kind = file_kernels(
            text, os.path.join(tree, f"build/id-{id_}/bin/{name}"), dumps
        )
        result["backend"] = result["backend"] or kind
        result["files"][name] = summary(kernels)
    if status is not None and result["backend"] is None:
        result["missing"] = (
            "no kernel numbers in its build (FFMT_TEST_FLAGS of its setup)"
        )
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    with open(out, "w") as f:
        json.dump(result, f, indent=1, sort_keys=True)


def load(directory):
    runs = {}
    for path in glob.glob(os.path.join(directory, "id-*.json")):
        with open(path) as f:
            run = json.load(f)
        runs[run["id"]] = run
    return runs


def show(stats):
    return " / ".join(f"{stats[k]:g}" for k in STATS) if stats else "n/a"


def expected_not_to_build(configs, id_, name):
    """Whether test/configs.json lists a test of this file as failed_to_compile."""
    row = next((r for r in configs if r["id"] == id_), {})
    return any(t.startswith(f"{name}:") for t in row.get("failed_to_compile", []))


def change(old, new):
    """A number of the per-id table: base → head (percentage), or one value."""
    if old == new:
        return f"{old:g}"
    pct = "new" if old == 0 else f"{(new - old) / old:+.0%}"
    return f"{old:g} → {new:g} ({pct})"


def details(summary, lines):
    return (
        f"<details><summary>{summary}</summary>\n\n"
        + "\n".join(lines)
        + "\n\n</details>\n"
    )


def compare(base_dir, head_dir, configs_path="test/configs.json"):
    """One row per id: each metric's average over the kernels measured on both
    sides, base → head; then, collapsed, each changed file's min / avg / max.
    What fails is a test, i.e. a kernel, measured on both sides whose number
    grew, or a file that no longer builds while test/configs.json does not
    expect it. What has no reference (a new id, file or test) is listed, not
    failed."""
    base, head = load(base_dir), load(head_dir)
    with open(configs_path) as f:
        configs = json.load(f)
    units = {
        "nvidia": "regs, stack frame, spill stores, shared memory",
        "nvidia-omp": "regs, stack frame, spill stores, shared memory",
        "amd": "VGPRs + AGPRs, scratch per lane, VGPR + SGPR spills (count), LDS",
        "intel": "GRFs used, private memory, spill, SLM",
    }
    header = "| registers | private B | spill B | local B |"
    ids, rows, grown, missing, skipped, unchanged = [], [], [], [], [], 0
    for id_ in sorted(head):
        h, b = head[id_], base.get(id_)
        label = f"{id_} ({h.get('backend') or '?'})"
        if "missing" in h:
            ids.append(f"| {label} | | {h['missing']} | | | | |")
            continue
        if b is None or "missing" in b:
            missing.append(f"id {id_}: no reference at all (new id?)")
            continue
        totals = {m: [0, 0] for m in METRICS}  # base, head sums over the kernels
        counted = dict.fromkeys(METRICS, 0)
        kernels, id_grew = 0, 0
        for name in sorted(set(h["files"]) | set(b["files"])):
            hf, bf = h["files"].get(name), b["files"].get(name)
            if hf is None:
                if expected_not_to_build(configs, id_, name):
                    skipped.append(f"id {id_} {name}")
                elif bf is not None:
                    grown.append(f"id {id_} {name}: no longer builds")
                    id_grew += 1
                    rows.append(f"| {label} | {name} | **no longer builds** | | | |")
                continue
            if bf is None:
                missing.append(
                    f"id {id_} {name}: the whole file (new, or it did not build there)"
                )
                continue
            for kernel in sorted(set(hf["kernels"]) - set(bf["kernels"])):
                missing.append(f"id {id_} {name}, {short(kernel)}")
            file_grew = False
            for kernel in sorted(set(hf["kernels"]) & set(bf["kernels"])):
                old, new = bf["kernels"][kernel], hf["kernels"][kernel]
                kernels += 1
                for m in METRICS:
                    if None not in (old[m], new[m]):
                        totals[m][0] += old[m]
                        totals[m][1] += new[m]
                        counted[m] += 1
                up = [
                    f"{m} {old[m]} → {new[m]}"
                    for m in METRICS
                    if None not in (old[m], new[m]) and new[m] > old[m]
                ]
                if up:
                    file_grew = True
                    id_grew += 1
                    grown.append(f"id {id_} {name}, {short(kernel)}: " + ", ".join(up))
            if not file_grew and all(hf[m] == bf[m] for m in METRICS):
                unchanged += 1
                continue
            cells = [
                show(hf[m]) if hf[m] == bf[m] else f"{show(bf[m])} → {show(hf[m])}"
                for m in METRICS
            ]
            if file_grew:
                cells = [f"**{c}**" if "→" in c else c for c in cells]
            shown = f"**{name}**" if file_grew else name
            rows.append(f"| {label} | {shown} | " + " | ".join(cells) + " |")
        cells = [
            change(*(round(t / counted[m], 1) for t in totals[m]))
            if counted[m]
            else "n/a"
            for m in METRICS
        ]
        grew = f"**{id_grew}**" if id_grew else "0"
        ids.append(f"| {label} | {kernels} | " + " | ".join(cells) + f" | {grew} |")
    print("## GPU resources of the test kernels, base → head\n")
    if missing:
        print(f"### ⚠️ Missing in the reference (new test added?): {len(missing)}\n")
        print(details("Not compared, not failed", [f"- {m}" for m in missing]))
    print(
        "Per id, each metric's average over the kernels measured on both sides "
        "(registers per work-item; private memory per work-item, local memory per "
        "work-group, in bytes). They are: "
        + "; ".join(f"{k}: {v}" for k, v in units.items())
        + ".\n"
    )
    print(f"| id (backend) | kernels {header} tests grew |")
    print("|---|---|---|---|---|---|---|")
    print("\n".join(ids) + "\n")
    if rows:
        print(
            details(
                f"Per test file: min / avg / max over its kernels ({len(rows)} changed, "
                f"{unchanged} unchanged; bold: a test of the file grew)",
                [f"| id (backend) | file {header}", "|---|---|---|---|---|---|", *rows],
            )
        )
    else:
        print(f"{unchanged} file(s) unchanged.\n")
    if skipped:
        print(
            details(
                f"{len(skipped)} file(s) not built, as test/configs.json expects "
                "(failed_to_compile)",
                [", ".join(skipped)],
            )
        )
    if grown:
        print(details(f"<b>{len(grown)} test(s) grew</b>", [f"- {g}" for g in grown]))
    else:
        print("No test grew.")
    return 1 if grown else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    m = sub.add_parser("measure")
    m.add_argument("id", type=int)
    m.add_argument("out")
    m.add_argument("--tree", default=".")
    c = sub.add_parser("compare")
    c.add_argument("base")
    c.add_argument("head")
    args = parser.parse_args()
    if args.command == "measure":
        measure(args.id, args.out, args.tree)
        return 0
    return compare(args.base, args.head)


if __name__ == "__main__":
    sys.exit(main())
