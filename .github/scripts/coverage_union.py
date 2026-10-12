#!/usr/bin/env python3
"""Region coverage of one source file, as the union over several binaries.

The header is compiled twice (FFMT_BUFFER_PATH=0 and =1), so the same
function exists in both coverage binaries, often with a different body.
`llvm-cov export A -object B` keeps only one record per function, and
which one depends on object order: a function the specifiers binary never
calls (zero counts) hides the buffer binary's real counts, and functions
whose body differs report "mismatched data". The total then depends on
the order the binaries are listed in.

Instead, run and export each binary on its own with its own profile, and
count a code region (file position) as covered if any binary covers it.

usage: coverage_union.py LLVM_COV LLVM_PROFDATA SOURCE BINARY...
Prints {"covered": N, "count": M, "percent": P} as JSON.
"""
import json
import os
import subprocess
import sys
import tempfile


def regions(cov, profdata, source, binary, tmp):
    name = os.path.basename(binary)
    raw = os.path.join(tmp, name + ".profraw")
    prof = os.path.join(tmp, name + ".profdata")
    env = dict(os.environ, LLVM_PROFILE_FILE=raw)
    subprocess.run([binary], env=env, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, check=True)
    subprocess.run([profdata, "merge", "-o", prof, raw], check=True)
    out = subprocess.run([cov, "export", binary, "-instr-profile=" + prof,
                          "-sources", source],
                         check=True, capture_output=True, text=True).stdout
    found = {}
    for fn in json.loads(out)["data"][0]["functions"]:
        for ls, cs, le, ce, count, fid, _, kind, *_ in fn["regions"]:
            if kind != 0 or not os.path.samefile(fn["filenames"][fid], source):
                continue  # code regions of SOURCE only
            key = (ls, cs, le, ce)
            found[key] = found.get(key, 0) or count  # any instantiation
    return found


def main():
    cov, profdata, source, *binaries = sys.argv[1:]
    union = {}
    with tempfile.TemporaryDirectory() as tmp:
        for b in binaries:
            for key, count in regions(cov, profdata, source, b, tmp).items():
                union[key] = union.get(key, 0) or count
    covered = sum(1 for c in union.values() if c)
    print(json.dumps({"covered": covered, "count": len(union),
                      "percent": 100.0 * covered / len(union)}))


if __name__ == "__main__":
    main()
