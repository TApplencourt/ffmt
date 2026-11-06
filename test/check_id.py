#!/usr/bin/env python3
"""Compare one id's results with its expected failures in test/configs.json.

    python3 test/check_id.py 4          after test/run_id.sh 4
    python3 test/check_id.py 39 build   after test/run_id.sh 39 build

Reads results/id-<id>/ and prints every failure that is not expected and
every expected failure that did not happen, then exits 1 if there is any.
test/run_id.sh exits non-zero whenever a test fails, expected or not, so CI
ignores its status and lets this script decide.

Names: a test is <file>:<number> (floats:104, null_string:3); a binary that
does not finish (a sanitizer report, a fault: any exit status but 0 and 88,
see run_failures) is <file>:crash, or main:crash for the main binary; a file
that does not compile is isolated:<name> or negative:<name>.
"""
import json
import os
import re
import sys


def stem(path):
    return os.path.basename(path).replace("test_", "", 1).replace(".cpp", "")


def tap_failures(path):
    text = open(path, errors="replace").read()
    return ["%s:%s" % (stem(m.group(2)), m.group(1))
            for m in re.finditer(r"^not ok (\d+) - (\S+)", text, re.M)]


# Exit status of a test binary that finished and saw a failing test
# (TEST_FAILED in test/capture.hpp).
TEST_FAILED = 88


def run_failures(log, status, name):
    """The failing tests of one binary's run, plus <name>:crash unless it
    exited 0 with none or TEST_FAILED with some: any other status (1 from a
    sanitizer, 128+n from a signal, ...) means it did not finish."""
    fails = tap_failures(log) if os.path.exists(log) else []
    if status != (TEST_FAILED if fails else 0):
        fails.append(name + ":crash")
    return fails


def check(id_, goal):
    row = {r["id"]: r for r in json.load(open("test/configs.json"))}[id_]
    logs = "results/id-%d" % id_
    ran = goal == "test"
    problems = []
    compile_fails, runtime_fails = set(), set()

    if ran:
        run_status = os.path.join(logs, "run.status")
        if os.path.exists(run_status):
            runtime_fails.update(run_failures(os.path.join(logs, "run.log"),
                                              int(open(run_status).read()), "main"))
        else:
            problems.append("the main binary did not build or did not run")

    isolated_log = os.path.join(logs, "isolated.log")
    if not os.path.exists(isolated_log):
        problems.append("no isolated.log: the isolated step did not run")
    else:
        for line in open(isolated_log):
            status, step, name = line.split()
            if step == "build" and status != "0":
                compile_fails.add("isolated:" + stem(name))
            elif step == "run":
                runtime_fails.update(run_failures(os.path.join(logs, "isolated-%s.log" % name),
                                                  int(status), stem(name)))

    negative_log = os.path.join(logs, "negative.log")
    if not os.path.exists(negative_log):
        problems.append("no negative.log: the negative step did not run")
    else:
        for line in open(negative_log):
            status, kind, name = line.split()
            compiled = status == "0"
            if name == "accept_valid":
                if not compiled:
                    problems.append("the negative control does not compile (%s pass), "
                                    "so the negative results mean nothing" % kind)
            elif kind == "syntax" and not compiled:
                problems.append("negative:%s is broken: it fails to compile even with "
                                "the format call made a no-op" % name)
            elif kind == "fmt" and compiled:
                problems.append("negative:%s: {fmt} accepts it, so it is not a real "
                                "restriction" % name)
            elif kind == "ffmt" and not compiled:
                compile_fails.add("negative:" + name)

    expected = row["expected_fail"]
    for what, actual, exp in (("compile", compile_fails, set(expected["compile"])),
                              ("runtime", runtime_fails, set(expected["runtime"]))):
        if what == "runtime" and not ran:
            continue
        for name in sorted(actual - exp):
            problems.append("unexpected %s failure: %s" % (what, name))
        for name in sorted(exp - actual):
            problems.append("expected %s failure did not happen: %s" % (what, name))

    if problems:
        result = "%d problem(s)" % len(problems)
    else:
        result = ("as expected (%d compile, %d runtime expected failures)"
                  % (len(compile_fails), len(runtime_fails) if ran else 0))
    print("id %d (%s): %s" % (id_, row["notes"], result))
    for p in problems:
        print("  " + p)

    # On GitHub Actions, also list the id on the run's summary page, so a
    # failing job can be read without opening its log.
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write("### %s id %d: %s\n%s\n%s\n" % (
                "❌" if problems else "✅", id_, row["notes"], result,
                "".join("- %s\n" % p for p in problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    sys.exit(check(int(sys.argv[1]), sys.argv[2] if len(sys.argv) > 2 else "test"))
