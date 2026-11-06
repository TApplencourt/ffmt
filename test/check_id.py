#!/usr/bin/env python3
"""Compare one id's results with its expected failures in test/configs.json.

    python3 test/check_id.py 4          after test/run_id.sh 4
    python3 test/check_id.py 39 build   after test/run_id.sh 39 build

Reads results/id-<id>/ and prints every failure that is not expected and
every expected failure that did not happen, then exits 1 if there is any.
test/run_id.sh exits non-zero whenever a test fails, expected or not, so CI
ignores its status and lets this script decide.

Names: a test is <file>:<number> (floats:104, null_string:3); a binary that
does not finish (a sanitizer report, a fault: any exit status but 0 and 88)
is <file>:crash, or main:crash for the main binary; a file that does not
compile is isolated:<name> or negative:<name>.
"""
import json
import os
import re
import sys

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
id_ = int(sys.argv[1])
ran = (sys.argv[2] if len(sys.argv) > 2 else "test") == "test"
row = {r["id"]: r for r in json.load(open("test/configs.json"))}[id_]
logs = "results/id-%d/" % id_
problems, compile_fails, runtime_fails = [], set(), set()


def stem(name):
    return os.path.basename(name).replace("test_", "", 1).replace(".cpp", "")


def run_failures(log, status, name):
    """The failing tests of one binary's run, plus <name>:crash unless it
    exited 0 with none or 88 (TEST_FAILED, test/capture.hpp) with some."""
    fails = ["%s:%s" % (stem(f), n) for n, f in
             re.findall(r"^not ok (\d+) - (\S+)", open(logs + log, errors="replace").read(), re.M)]
    return fails + [name + ":crash"] * (status != (88 if fails else 0))


def lines(log):
    """The fields of each line of a status log, or a problem if it is missing."""
    if os.path.exists(logs + log):
        return [line.split() for line in open(logs + log)]
    problems.append("no %s: that step did not run" % log)
    return []


if ran:
    if os.path.exists(logs + "run.status"):
        runtime_fails.update(run_failures("run.log", int(open(logs + "run.status").read()), "main"))
    else:
        problems.append("the main binary did not build or did not run")

for status, step, name in lines("isolated.log"):
    if step == "build" and status != "0":
        compile_fails.add("isolated:" + stem(name))
    elif step == "run":
        runtime_fails.update(run_failures("isolated-%s.log" % name, int(status), stem(name)))

for status, kind, name in lines("negative.log"):
    compiled = status == "0"
    if name == "accept_valid":
        if not compiled:
            problems.append("the negative control does not compile (%s pass), "
                            "so the negative results mean nothing" % kind)
    elif kind == "syntax" and not compiled:
        problems.append("negative:%s is broken: it fails to compile even with "
                        "the format call made a no-op" % name)
    elif kind == "fmt" and compiled:
        problems.append("negative:%s: {fmt} accepts it, so it is not a real restriction" % name)
    elif kind == "ffmt" and not compiled:
        compile_fails.add("negative:" + name)

# FFMT_EXTRA_FILES (e.g. the README examples) are not tests: no expected
# failure, so any that does not build or run is a problem.
for status, step, name in lines("extra.log"):
    if status != "0":
        problems.append("extra file %s does not %s (exit %s, see extra-%s.log)"
                        % (name, step, status, name))

expected = row["expected_fail"]
for what, actual, exp in (("compile", compile_fails, expected["compile"]),
                          ("runtime", runtime_fails, expected["runtime"] if ran else [])):
    problems += ["unexpected %s failure: %s" % (what, n) for n in sorted(actual - set(exp))]
    problems += ["expected %s failure did not happen: %s" % (what, n) for n in sorted(set(exp) - actual)]

result = ("%d problem(s)" % len(problems) if problems else
          "as expected (%d compile, %d runtime expected failures)"
          % (len(compile_fails), len(runtime_fails)))
print("id %d (%s): %s" % (id_, row["notes"], result))
for p in problems:
    print("  " + p)

# The id's row of the table on the CI run's summary page (the workflow's
# summary job), so a failing id can be read without opening its log.
with open(logs + "summary.md", "w") as f:
    f.write("| %s %d | %s | %s |\n" % ("❌" if problems else "✅", id_, row["notes"],
                                     "<br>".join([result] + problems).replace("|", "\\|")))
sys.exit(1 if problems else 0)
