#!/usr/bin/env python3
"""GPU register / spill / stack usage of every CUDA test, without a GPU.

Each RUN(...) in a test file is its own kernel (_ffmt_run_kernel<lambda>,
see test/test_body_cuda.inc), so ptxas reports per test what the inlined
ffmt code costs. Register allocation happens in ptxas at compile time, so
this only needs clang, the CUDA headers/libdevice and ptxas.

  register_report.py measure ROOT OUT.json
      Compile ROOT/test/test_<name>.cpp device-only to PTX with clang
      (-x cuda --cuda-device-only -O2), run `ptxas -v`, write per-kernel
      registers, spill bytes and stack frame. Only needs the source tree,
      so CI measures the base rev with this branch's script.

  register_report.py compare BASE.json HEAD.json
      Print a markdown report (per-file maxima, then changed kernels) and
      exit 1 if a file's spill bytes grow at all, or its max registers grow
      by more than REG_SLACK_PCT percent.

Environment: CXX (clang++), CUDA_PATH (/usr/local/cuda), CUDA_ARCH (sm_80),
FMT_INCLUDE ({fmt} include dir, host side of the tests only), TESTS
(space-separated names; default: the Makefile's TEST_NAMES_CUDA),
REG_SLACK_PCT (5).
"""
import concurrent.futures
import json
import os
import re
import subprocess
import sys

DEFAULT_TESTS = "integers floats strings layout misc formatter buffer_path"


def compile_and_measure(root, name, env):
    src = os.path.join(root, "test", f"test_{name}.cpp")
    ptx = os.path.join(env["OUT_DIR"], f"test_{name}.ptx")
    cmd = [env["CXX"], "-std=c++20", "-O2", "-x", "cuda",
           "--cuda-gpu-arch=" + env["CUDA_ARCH"], "--cuda-path=" + env["CUDA_PATH"],
           "--cuda-device-only", "-Wno-unknown-cuda-version",
           "-I", os.path.join(root, "include"), "-isystem", env["FMT_INCLUDE"],
           "-S", src, "-o", ptx]
    subprocess.run(cmd, check=True)
    ptxas = os.path.join(env["CUDA_PATH"], "bin", "ptxas")
    out = subprocess.run([ptxas, "-arch=" + env["CUDA_ARCH"], "-v", ptx, "-o", os.devnull],
                         check=True, capture_output=True, text=True)
    return parse_ptxas(out.stderr + out.stdout)


def parse_ptxas(text):
    """{mangled kernel: {regs, spill_stores, spill_loads, stack}} for entries."""
    kernels, entry, props_for = {}, None, None
    for line in text.splitlines():
        m = re.search(r"Compiling entry function '([^']+)'", line)
        if m:
            entry = m.group(1)
            kernels[entry] = {"regs": 0, "spill_stores": 0, "spill_loads": 0, "stack": 0}
            continue
        m = re.search(r"Function properties for (\S+)", line)
        if m:
            props_for = m.group(1)
            continue
        m = re.search(r"(\d+) bytes stack frame, (\d+) bytes spill stores, (\d+) bytes spill loads",
                      line)
        if m and props_for in kernels:
            k = kernels[props_for]
            k["stack"], k["spill_stores"], k["spill_loads"] = map(int, m.groups())
            continue
        m = re.search(r"Used (\d+) registers", line)
        if m and entry:
            kernels[entry]["regs"] = int(m.group(1))
    return kernels


def short_name(mangled):
    """'lambda #N' for _ffmt_run_kernel<lambda>, N = the RUN's lambda number in
    its scope, read from the Itanium mangling (UlvE_ = #1, UlvE<n>_ = #n+2);
    no c++filt, whose lambda spelling differs between GNU and LLVM."""
    nums = re.findall(r"UlvE(\d*)_", mangled)
    if not nums:
        return mangled
    return "lambda #%d" % (int(nums[-1]) + 2 if nums[-1] else 1)


def measure(root, out_json):
    env = {
        "CXX": os.environ.get("CXX", "clang++"),
        "CUDA_PATH": os.environ.get("CUDA_PATH", "/usr/local/cuda"),
        "CUDA_ARCH": os.environ.get("CUDA_ARCH", "sm_80"),
        "FMT_INCLUDE": os.environ.get("FMT_INCLUDE",
                                      os.path.join(root, "third_party", "fmt", "include")),
        "OUT_DIR": os.path.join(os.path.abspath(os.path.dirname(out_json) or "."),
                                os.path.basename(out_json) + ".ptx.d"),
    }
    os.makedirs(env["OUT_DIR"], exist_ok=True)
    tests = [t for t in os.environ.get("TESTS", DEFAULT_TESTS).split()
             if os.path.exists(os.path.join(root, "test", f"test_{t}.cpp"))]
    result = {}
    with concurrent.futures.ThreadPoolExecutor() as pool:
        futures = {t: pool.submit(compile_and_measure, root, t, env) for t in tests}
        for t in tests:
            result[t] = {short_name(m): v for m, v in futures[t].result().items()}
    with open(out_json, "w") as f:
        json.dump(result, f, indent=1, sort_keys=True)


def summary(kernels):
    return {"kernels": len(kernels),
            "max_regs": max((k["regs"] for k in kernels.values()), default=0),
            "spill": sum(k["spill_stores"] + k["spill_loads"] for k in kernels.values()),
            "max_stack": max((k["stack"] for k in kernels.values()), default=0)}


def compare(base_json, head_json):
    base, head = json.load(open(base_json)), json.load(open(head_json))
    slack = float(os.environ.get("REG_SLACK_PCT", "5"))
    fails = []
    print("### GPU resource usage per test file (CUDA, `ptxas -v`)\n")
    print("| test | kernels | max registers | spill bytes | max stack frame |")
    print("|---|---|---|---|---|")
    fmt = lambda b, h: f"{h}" if b is None or b == h else f"{b} → **{h}**"
    for t in sorted(set(base) | set(head)):
        if t not in head:
            print(f"| {t} | removed | | | |")
            continue
        h = summary(head[t])
        b = summary(base[t]) if t in base else {k: None for k in h}
        print(f"| {t} | {fmt(b['kernels'], h['kernels'])} | {fmt(b['max_regs'], h['max_regs'])} "
              f"| {fmt(b['spill'], h['spill'])} | {fmt(b['max_stack'], h['max_stack'])} |")
        if t not in base:
            continue
        if h["spill"] > b["spill"]:
            fails.append(f"{t}: spill bytes {b['spill']} → {h['spill']}")
        if h["max_regs"] > b["max_regs"] * (1 + slack / 100):
            fails.append(f"{t}: max registers {b['max_regs']} → {h['max_regs']} "
                         f"(> {slack:g}% slack)")

    changed = [(t, n, base[t][n], k) for t in sorted(head) if t in base
               for n, k in sorted(head[t].items()) if n in base[t] and base[t][n] != k]
    if changed:
        print("\n<details><summary>Changed kernels "
              f"({len(changed)})</summary>\n")
        print("| test | kernel | registers | spill bytes | stack frame |")
        print("|---|---|---|---|---|")
        for t, n, b, h in changed:
            sp = lambda k: k["spill_stores"] + k["spill_loads"]
            print(f"| {t} | {n} | {fmt(b['regs'], h['regs'])} | {fmt(sp(b), sp(h))} "
                  f"| {fmt(b['stack'], h['stack'])} |")
        print("\n</details>")
    if fails:
        print("\n**FAIL**\n")
        for f in fails:
            print(f"- {f}")
        return 1
    print("\nPASS")
    return 0


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "measure":
        measure(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 4 and sys.argv[1] == "compare":
        sys.exit(compare(sys.argv[2], sys.argv[3]))
    else:
        sys.exit(__doc__)
