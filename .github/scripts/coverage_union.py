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
Prints the percentage of covered regions.
"""
import json
import os
import subprocess
import sys
import tempfile

cov, profdata, source, *binaries = sys.argv[1:]
covered = {}  # code region of SOURCE -> covered by any binary
with tempfile.TemporaryDirectory() as tmp:
    raw, prof = os.path.join(tmp, "raw"), os.path.join(tmp, "prof")
    for binary in binaries:
        # 88 (TEST_FAILED, test/capture.hpp): the binary ran to the end with a
        # failing test, expected on the specifiers path; it still covered what
        # it ran, and test/check_id.py judges the results. Anything else is a
        # crash.
        status = subprocess.run([binary], env=dict(os.environ, LLVM_PROFILE_FILE=raw),
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
        if status not in (0, 88):
            sys.exit("%s exited %d: it did not finish" % (binary, status))
        subprocess.run([profdata, "merge", "-o", prof, raw], check=True)
        out = subprocess.run([cov, "export", binary, "-instr-profile=" + prof, "-sources", source],
                             check=True, capture_output=True, text=True).stdout
        for fn in json.loads(out)["data"][0]["functions"]:
            for ls, cs, le, ce, count, fid, _, kind, *_ in fn["regions"]:
                if kind == 0 and os.path.samefile(fn["filenames"][fid], source):
                    key = (ls, cs, le, ce)
                    covered[key] = covered.get(key, False) or count > 0
print(100.0 * sum(covered.values()) / len(covered))
