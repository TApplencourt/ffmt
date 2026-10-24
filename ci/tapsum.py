#!/usr/bin/env python3
"""Summarise ffmt's TAP output as one JSON row per run.

ffmt's test binaries write TAP to stderr (test/capture.hpp): one
`ok N - name`, `not ok N - name`, or `skipped N - name # reason` per
assertion, plus a `# name: P/T passed` line per test file. The Makefile's
run macro adds `binary: PASS (Nms)` per binary.

A skip is not a failure: it names a known compiler bug from the catalog at
the top of test/capture.hpp ("spirv-o0", "ptx-clang-o0", "o2-ptx-ice"), so
the pass rate is reported against the tests that actually ran, with the
skips counted separately and broken down by reason.

Usage:
    make test 2>&1 | tapsum.py --backend icpx-sycl --opt O2 > result.json
    tapsum.py --backend ... --opt ... < saved.log

The metadata flags are free-form; whatever is passed lands in the JSON so
the table generator can group on it.
"""
import argparse
import json
import platform
import re
import subprocess
import sys
from collections import Counter

# `ok 3 - test_floats.cpp`, `not ok 3 - ...`, `skipped 3 - ... # spirv-o0`
RESULT = re.compile(r"^(ok|not ok|skipped) (\d+) - (\S+)(?: # (.*))?$")
# The Makefile's run macro: `test_floats_O2: PASS (123ms)`
BINARY = re.compile(r"^(\S+): (PASS|FAIL) \((\d+)ms\)$")
# `make: *** [Makefile:213: build/test_main_omp] Error 1` -- a binary that
# never got built, so its tests could not run and are absent from the TAP.
MAKE_FAIL = re.compile(r"^make(?:\[\d+\])?: \*\*\* \[[^:]+: (\S+)\] Error")
# Every target make tried to link, so the ones that failed can be given as
# a fraction rather than a bare count.
LINK_LINE = re.compile(r" -o (build/\S+)\s*$")
# The first compiler error, kept as the note for why a build failed.
COMPILE_ERROR = re.compile(r"(error: .*)$")


def parse(lines):
    tests, binaries, unbuilt, attempted = [], [], [], []
    error = None
    for line in lines:
        line = line.rstrip("\n")
        m = RESULT.match(line)
        if m:
            status, num, name, reason = m.groups()
            tests.append({"status": status, "n": int(num),
                          "file": name, "reason": reason})
            continue
        m = BINARY.match(line)
        if m:
            name, verdict, ms = m.groups()
            binaries.append({"binary": name, "verdict": verdict,
                             "wall_ms": int(ms)})
            continue
        m = MAKE_FAIL.match(line)
        if m:
            unbuilt.append(m.group(1))
            continue
        m = LINK_LINE.search(line)
        if m and m.group(1) not in attempted:
            attempted.append(m.group(1))
        if error is None:
            m = COMPILE_ERROR.search(line)
            if m:
                error = m.group(1)
    return tests, binaries, unbuilt, attempted, error


def version_of(compiler):
    """First line of `<compiler> --version`, or None if it cannot be run."""
    if not compiler:
        return None
    # stdout/stderr rather than capture_output: CI images in the wild still
    # ship Python 3.6, where capture_output does not exist.
    try:
        out = subprocess.run([compiler, "--version"],
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             universal_newlines=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        return None
    first = out.stdout.splitlines()
    return first[0].strip() if first else None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--backend", required=True,
                    help="backend label, e.g. icpx-sycl, acpp, clang-omp-nvptx")
    ap.add_argument("--opt", default="", help="optimisation level, e.g. O0")
    ap.add_argument("--compiler", default="",
                    help="compiler binary; its --version is recorded")
    ap.add_argument("--device", default="", help="device label, e.g. pvc, a100")
    ap.add_argument("--commit", default="", help="git sha under test")
    ap.add_argument("--extra", action="append", default=[], metavar="K=V",
                    help="any other field to record; repeatable")
    ap.add_argument("--expected-builds", type=int, default=0,
                    help="how many targets this backend has, from `make -n`. "
                         "The log only names targets make reached, so a "
                         "compiler that crashes on the first one would "
                         "otherwise shrink the denominator to 1.")
    ap.add_argument("--per-test", action="store_true",
                    help="include the full per-assertion list, not just counts")
    args = ap.parse_args()

    tests, binaries, unbuilt, attempted, error = parse(sys.stdin)

    counts = Counter(t["status"] for t in tests)
    ok, failed, skipped = counts["ok"], counts["not ok"], counts["skipped"]
    ran = ok + failed

    row = {
        "backend": args.backend,
        "opt": args.opt,
        "device": args.device,
        "commit": args.commit,
        "compiler": args.compiler,
        "compiler_version": version_of(args.compiler),
        "host": platform.node(),
        "ok": ok,
        "failed": failed,
        "skipped": skipped,
        "total": len(tests),
        # Against what ran, so a backend is not penalised for skipping a
        # test it documents as hitting a known compiler bug. The skips are
        # right there in skip_reasons for anyone who wants the other rate.
        "pass_rate": (ok / ran) if ran else None,
        "skip_reasons": dict(Counter(t["reason"] for t in tests
                                     if t["status"] == "skipped" and t["reason"])),
        "failures": sorted({t["file"] for t in tests if t["status"] == "not ok"}),
        # Targets make could not build. Their tests are absent from the TAP
        # above rather than counted as failures, so a row with unbuilt
        # targets is reporting on less than the whole suite -- the table
        # says so rather than showing a pass rate over what survived.
        "unbuilt": unbuilt,
        "attempted_builds": max(args.expected_builds, len(attempted)),
        # Of the targets make tried to link, how many it could not. A route
        # whose compiler crashes reports 1.0 here and no tests at all; a
        # route where one test file fails to compile reports the fraction.
        "build_fail_rate": ((len(unbuilt) / max(args.expected_builds, len(attempted)))
                            if (args.expected_builds or attempted) else None),
        "build_error": error,
        "binaries": binaries,
    }
    for kv in args.extra:
        key, _, value = kv.partition("=")
        row[key] = value
    if args.per_test:
        row["tests"] = tests

    json.dump(row, sys.stdout, indent=2, sort_keys=True)
    sys.stdout.write("\n")

    # Nonzero on a failed test or a target that would not build; a run that
    # only skipped tests still produced a usable row.
    return 1 if (failed or unbuilt) else 0


if __name__ == "__main__":
    sys.exit(main())
