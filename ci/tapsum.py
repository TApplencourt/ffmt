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


def parse(lines):
    tests, binaries = [], []
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
    return tests, binaries


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
    ap.add_argument("--per-test", action="store_true",
                    help="include the full per-assertion list, not just counts")
    args = ap.parse_args()

    tests, binaries = parse(sys.stdin)

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
        "binaries": binaries,
    }
    for kv in args.extra:
        key, _, value = kv.partition("=")
        row[key] = value
    if args.per_test:
        row["tests"] = tests

    json.dump(row, sys.stdout, indent=2, sort_keys=True)
    sys.stdout.write("\n")

    # Nonzero only on a real failure: a run that skipped everything still
    # produced a usable row, and the caller decides what to do about it.
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
