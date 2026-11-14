#!/usr/bin/env python3
"""Compare one id's results with its expected failures in test/configs.json.

    python3 test/check_id.py 4          after test/run_id.sh 4
    python3 test/check_id.py 39 build   after test/run_id.sh 39 build
    python3 test/check_id.py 72 extra-test  after test/run_id.sh 72 extra-test

Reads results/id-<id>/ and prints every failure that is not expected and
every expected failure that did not happen, then exits 1 if there is any.
test/run_id.sh exits non-zero whenever a test fails, expected or not, so CI
ignores its status and lets this script decide.

Names: a test is <file>:<number> (floats:104, null_string:3); an isolated
binary that does not finish is <file>:crash; a file that does not compile is
isolated:<name> or negative:<name>. Backend and harness failures that do not
fit those categories use exact expected_fail.structural signatures, such as
main:crash, main:build, negative-control:accept_valid:ffmt, and
extra:<name>:build. Missing logs and an interrupted run_id.sh are always
failures, never expected structural failures.
"""
import glob
import json
import os
import re
import sys

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
id_ = int(sys.argv[1])
mode = sys.argv[2] if len(sys.argv) > 2 else "test"
if mode not in ("test", "build", "extra-test"):
    sys.exit("usage: python3 test/check_id.py <id> [test|build|extra-test]")
ran = mode in ("test", "extra-test")
row = {r["id"]: r for r in json.load(open("test/configs.json"))}[id_]
logs = "results/id-%d/" % id_
problems, compile_fails, runtime_fails = [], set(), set()
structural_fails = {}


def structural(signature, detail):
    structural_fails[signature] = detail


def stem(name):
    return os.path.basename(name).replace("test_", "", 1).replace(".cpp", "")


def run_failures(log, status, name):
    """Return a binary's test failures and record abnormal completion."""
    if not os.path.exists(logs + log):
        problems.append("no %s: cannot evaluate %s (exit %d)" % (log, name, status))
        return []
    fails = ["%s:%s" % (stem(f), n) for n, f in
             re.findall(r"^not ok (\d+) - (\S+)", open(logs + log, errors="replace").read(), re.M)]
    if status == (88 if fails else 0):
        return fails
    if name == "main":
        structural("main:crash", "the main binary did not complete normally (exit %d)" % status)
    else:
        fails.append(name + ":crash")
    return fails


def lines(log):
    """The fields of each line of a status log, or a problem if it is missing."""
    if os.path.exists(logs + log):
        return [line.split() for line in open(logs + log)]
    problems.append("no %s: that step did not run" % log)
    return []


if mode == "build":
    if os.path.exists(logs + "build.status"):
        status = int(open(logs + "build.status").read())
        if status != 0:
            problems.append("the build failed (exit %d, see build.log)" % status)
    else:
        problems.append("no build.status: the build did not run to completion")
if mode == "test" and os.path.exists(logs + "run.status") and not os.path.exists(logs + "run_id.status"):
    problems.append("no run_id.status: the full test invocation did not complete")
if mode == "extra-test" and not os.path.exists(logs + "extra-test.status"):
    problems.append("no extra-test.status: the example invocation did not complete")
if ran:
    if os.path.exists(logs + "run.status"):
        runtime_fails.update(run_failures("run.log", int(open(logs + "run.status").read()), "main"))
    elif os.path.exists(logs + "run.log"):
        problems.append("run.log exists but run.status is missing: the run did not complete")
    elif not os.path.exists(logs + "build.log") or os.path.getsize(logs + "build.log") == 0:
        problems.append("no nonempty build.log or run.status: the main build did not run to completion")
    elif not os.path.exists(logs + "run_id.status"):
        problems.append("no run_id.status: cannot distinguish a main build failure from an interrupted run")
    elif int(open(logs + "run_id.status").read()) == 0:
        problems.append("run_id.sh succeeded but run.status is missing")
    elif not os.path.exists(logs + "main-build.status"):
        problems.append("no main-build.status: cannot establish that the main binary failed to build")
    elif int(open(logs + "main-build.status").read()) == 0:
        problems.append("the main binary built but run.status is missing: the run did not complete")
    else:
        structural("main:build", "the main binary did not build in a completed run_id.sh invocation")

for status, step, name in lines("isolated.log"):
    if step == "build" and status != "0":
        compile_fails.add("isolated:" + stem(name))
    elif step == "run":
        runtime_fails.update(run_failures("isolated-%s.log" % name, int(status), stem(name)))

for status, kind, name in lines("negative.log"):
    compiled = status == "0"
    if name == "accept_valid":
        if not compiled:
            structural("negative-control:accept_valid:%s" % kind,
                       "the negative control does not compile (%s pass), "
                       "so the negative results mean nothing" % kind)
    elif kind == "syntax" and not compiled:
        structural("negative-control:%s:syntax" % name,
                   "negative:%s is broken: it fails to compile even with "
                   "the format call made a no-op" % name)
    elif kind == "fmt" and compiled:
        structural("negative-control:%s:fmt" % name,
                   "negative:%s: {fmt} accepts it, so it is not a real restriction" % name)
    elif kind == "ffmt" and not compiled:
        compile_fails.add("negative:" + name)

# FFMT_EXTRA_FILES (e.g. the README examples) use separate structural
# signatures rather than individual compile/runtime expectations.
extra_builds, extra_runs = {}, set()
for status, step, name in lines("extra.log"):
    if status != "0":
        structural("extra:%s:%s" % (name, step),
                   "extra file %s does not %s (exit %s, see extra-%s.log)"
                   % (name, step, status, name))
    if step == "build":
        extra_builds[name] = status
    elif step == "run":
        extra_runs.add(name)
if ran:
    for name, status in extra_builds.items():
        if status == "0" and name not in extra_runs:
            if mode != "test" or "main:build" not in structural_fails:
                structural("extra:%s:not-run" % name, "extra file %s built but did not run" % name)
    for name in extra_runs - extra_builds.keys():
        problems.append("extra file %s ran without a current build" % name)

expected = row["expected_fail"]
expected_compile = set(expected["compile"])
if "negative-control:accept_valid:ffmt" in structural_fails:
    compile_fails = {n for n in compile_fails if not n.startswith("negative:")}
    expected_compile = {n for n in expected_compile if not n.startswith("negative:")}
expected_runtime = set(expected["runtime"] if ran else [])
for what, actual, exp in (("compile", compile_fails, expected_compile),
                          ("runtime", runtime_fails, expected_runtime)):
    problems += ["unexpected %s failure: %s" % (what, n) for n in sorted(actual - set(exp))]
    if what == "runtime" and structural_fails.keys() & {"main:crash", "main:build"}:
        main_tests = {stem(path) for path in glob.glob("test/test_*.cpp")}
        exp = {n for n in exp if n.split(":", 1)[0] not in main_tests}
    problems += ["unexpected %s pass (XPASS): %s" % (what, n) for n in sorted(set(exp) - actual)]

expected_structural = expected.get("structural", [])
if not isinstance(expected_structural, list):
    problems.append("expected_fail.structural must be a list of exact signatures")
    expected_structural = []
expected_structural = set(expected_structural)
matched_structural = structural_fails.keys() & expected_structural
problems += ["unexpected structural failure: %s (%s)" % (n, structural_fails[n])
             for n in sorted(structural_fails.keys() - expected_structural)]
problems += ["unexpected structural pass (XPASS): %s" % n
             for n in sorted(expected_structural - structural_fails.keys())]

counts = "%d compile, %d runtime, %d structural expected failures" % (
    len(compile_fails), len(runtime_fails), len(matched_structural))
if problems:
    outcome, glyph = "FAIL", "❌"
    result = "FAIL (%d problem(s)); observed %s" % (len(problems), counts)
elif structural_fails:
    outcome, glyph = "XFAIL", "⚠️"
    result = "XFAIL (%s)" % counts
else:
    outcome, glyph = "PASS", "✅"
    result = "PASS (%s)" % counts
expected_details = ["expected structural failure: %s (%s)" % (n, structural_fails[n])
                    for n in sorted(matched_structural)]
print("id %d (%s): %s" % (id_, row["notes"], result))
for p in problems + expected_details:
    print("  " + p)

# The id's row of the table on the CI run's summary page (the workflow's
# summary job), so a failing id can be read without opening its log.
with open(logs + "summary.md", "w") as f:
    f.write("| %s %d | %s | %s |\n" % (glyph, id_, row["notes"],
                                     "<br>".join([result] + problems + expected_details).replace("|", "\\|")))
sys.exit(1 if problems else 0)
