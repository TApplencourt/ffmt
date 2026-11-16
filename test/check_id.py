#!/usr/bin/env python3
"""Compare the results of ids with what test/configs.json expects of them.

    python3 test/check_id.py 4       after test/run_id.sh 4
    python3 test/check_id.py 2 3 10  several ids, e.g. from downloaded CI artifacts
    python3 test/check_id.py --all   every id that has a results/id-<id>/
    python3 test/check_id.py 4 --accept-and-update-config
    python3 test/check_id.py --summary
    python3 test/check_id.py --table 2 3 10
    python3 test/check_id.py --list jlse:chiatta

Reads results/id-<id>/ (written by test/run_id.sh, or unpacked from a CI
artifact) and gives every test one state:

    passed                it ran and printed what {fmt} prints
    failed_to_compile     its file did not build
    failed_to_run         it ran and printed something else, or crashed
    compiled_but_not_run  an earlier test of its file crashed the binary

A test is <file>:<number> (floats:104 is the 104th RUN of
test/test_floats.cpp). Two more things have a state, as they fail on some
ids: extra:<name>, a program of the id's FFMT_EXTRA_FILES (it passes when it
builds and exits 0), and negative:accept_valid, the control of the negative
tests (when it does not build through ffmt, nothing does, and the negative
tests are not judged).

test/configs.json is the one description of every id. Per id: every test
that is not expected to pass, under its state; `device`, what its tests ran
on when its results were last accepted, and `tested_commit`, at which
commit; `ci`, the CI job that runs it (github:<runner> or jlse:<job>; none
for an id run by hand), and `goal` if that job only builds it.
test/tests.json holds the number of tests of each file, which is how the
tests of a file that did not build are known.

The check fails if a test is in another state than expected, in either
direction, if the tests ran on another device, or if the logs do not explain
themselves (a missing log, an interrupted run, a crash outside a test, a
negative test that compiles, a {fmt} that is not the pinned one, ...): that
is never expected.

--accept-and-update-config rewrites the id's row (and test/tests.json) from
the results instead of comparing: review the diff before you commit it. It
takes only results of a commit whose tree is the one checked out, apart from
those two files.
--summary reads only test/configs.json and prints which ids share which
failures. --table prints one Markdown table of ids already checked, from the
row each check left in results/id-<id>/summary.md: for a CI summary page.
--list prints the ids of a CI job (jlse:chiatta) or of a whole CI (github:).
"""

import argparse
import collections
import glob
import json
import os
import re
import subprocess
import sys

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CONFIGS, TESTS = "test/configs.json", "test/tests.json"
STATES = ("failed_to_compile", "failed_to_run", "compiled_but_not_run")
FIELDS = ("id", "notes", "ci", "goal", "tested_commit", "device")  # of a row, before STATES
STATE_OF_STEP = {"build": "failed_to_compile", "run": "failed_to_run"}
TEST_FAILED = 88  # test/capture.hpp: ran to the end, some test failed


def key(test):
    """Sort key of a test: floats:9 before floats:10. extra:<name> and
    negative:accept_valid have a name where the others have a number."""
    name, n = test.split(":", 1)
    return name, int(n) if n.isdigit() else n


def tests_of(name, numbers):
    return [f"{name}:{n}" for n in numbers]


def ranges(tests):
    """floats:1 floats:2 floats:3 floats:7 -> ['floats:1-3,7'], one item per file."""
    by_file = collections.defaultdict(list)
    for name, n in sorted(map(key, tests)):
        by_file[name].append(n)
    out = [f"{name}:{n}" for name in ("extra", "negative") for n in by_file.pop(name, [])]
    for name, ns in by_file.items():
        spans, start = [], ns[0]
        for n, following in zip(ns, [*ns[1:], None]):
            if following != n + 1:
                spans.append(str(start) if start == n else f"{start}-{n}")
                start = following
        out.append(f"{name}:{','.join(spans)}")
    return out


def dump_configs(rows):
    """test/configs.json: plain JSON, each list with one line per test file."""

    def tests(names):
        by_file = collections.defaultdict(list)
        for t in sorted(names, key=key):
            by_file[key(t)[0]].append(json.dumps(t))
        lines = ",\n".join("      " + ", ".join(v) for v in by_file.values())
        return f"[\n{lines}\n    ]" if names else "[]"

    out = []
    for r in rows:
        fields = [(k, json.dumps(r[k], ensure_ascii=False)) for k in FIELDS if k in r]
        fields += [(s, tests(r[s])) for s in STATES]
        out.append("  {\n" + ",\n".join(f'    "{k}": {v}' for k, v in fields) + "\n  }")
    with open(CONFIGS, "w") as f:
        f.write("[\n" + ",\n".join(out) + "\n]\n")


class Results:
    """What results/id-<id>/ says: the state of every test that did not pass
    (states), the test count of every file that ran far enough to say it
    (counts), and what the logs cannot explain (problems)."""

    def __init__(self, id_, counts):
        self.logs = f"results/id-{id_}/"
        self.problems, self.states, self.counts = [], {}, {}
        self.devices = set()
        self.missing = not os.path.exists(self.logs + "config.txt")
        if self.missing:
            self.commit, self.ran = "", False
            self.problems.append(f"no {self.logs}config.txt: test/run_id.sh did not run")
            return
        config = dict(re.findall(r"^(goal|commit|fmt)=(.*)$", self.read("config.txt"), re.MULTILINE))
        self.commit = config.get("commit", "")
        self.ran = config.get("goal") == "test"
        if config.get("goal") not in ("test", "build"):
            self.problems.append(f"no goal in {self.logs}config.txt: test/run_id.sh did not run")
        fmt, _, pinned = config.get("fmt", "unknown pinned=unknown").partition(" pinned=")
        if fmt != pinned:
            self.problems.append(f"the reference is {{fmt}} {fmt}, not the pinned {pinned} "
                                 "(FMT_DIR of the setup)")

        status = collections.defaultdict(dict)
        for code, step, name in self.lines("tests.log"):
            status[name][step] = int(code)
        for name, st in sorted(status.items()):
            if st["build"] == 0:
                if self.ran:
                    self.run(name, st.get("run"))
            elif name in counts:
                self.set(name, range(1, counts[name] + 1), "failed_to_compile")
            else:
                self.problems.append(f"{name} does not build and {TESTS} does not know how many "
                                     "tests it holds: accept an id that builds it first")
        for name in sorted(set(counts) - set(status)) if status else ():
            self.problems.append(f"{name} is in {TESTS} but was not built: "
                                 "remove it there if the file is gone")
        self.negative()
        # FFMT_EXTRA_FILES (e.g. the README examples): see extra-<name>.log.
        for code, step, name in self.lines("extra.log"):
            if code != "0":
                self.states[f"extra:{name}"] = STATE_OF_STEP[step]

    def read(self, log):
        if os.path.exists(self.logs + log):
            with open(self.logs + log, errors="replace") as f:
                return f.read()
        self.problems.append(f"no {self.logs}{log}: that step did not run")
        return ""

    def lines(self, log):
        """The fields of each `<exit status> <step> <name>` line of a status log."""
        return [line.split() for line in self.read(log).splitlines()]

    def set(self, name, numbers, state):
        self.states.update((t, state) for t in tests_of(name, numbers))

    def run(self, name, status):
        """One binary's TAP log (tap_reporter, test/capture.hpp)."""
        log = self.read(f"{name}.run.log")
        self.devices.update(re.findall(r"^# device: (.*)$", log, re.MULTILINE))
        plan = re.search(r"^1\.\.(\d+)$", log, re.MULTILINE)
        started = [int(n) for n in re.findall(r"^# start (\d+)$", log, re.MULTILINE)]
        if status is None:
            self.problems.append(f"{name} did not finish: the run was interrupted")
            return
        if not plan or not started:
            self.problems.append(f"{name}: exit status {status} before its first test "
                                 f"(see {name}.run.log)")
            return
        count = self.counts[name] = int(plan.group(1))
        result = {int(n): r for r, n in re.findall(r"^(ok|not ok) (\d+) - ", log, re.MULTILINE)}
        failed = [n for n, r in result.items() if r == "not ok"]
        self.set(name, failed, "failed_to_run")
        finished = sorted(result) == list(range(1, count + 1))
        if finished and status == (TEST_FAILED if failed else 0):
            return
        last = started[-1]
        if last in result:
            # Not in a test: nothing a test's state can express.
            self.problems.append(f"{name}: exit status {status} after test {last} of {count} "
                                 f"(see {name}.run.log)")
            return
        # It died in test `last`: that one failed, the ones after it did not run.
        self.set(name, [last], "failed_to_run")
        self.set(name, range(last + 1, count + 1), "compiled_but_not_run")

    def negative(self):
        """Each test/negative/reject_*.cpp passes when ffmt rejects it. No id
        may expect otherwise, so anything else is a problem."""
        status = {(name, kind): code == "0" for code, kind, name in self.lines("negative.log")}
        for kind in ("syntax", "fmt"):
            if not status.pop(("accept_valid", kind), True):
                self.problems.append(f"the negative control does not compile ({kind} pass)")
        if not status.pop(("accept_valid", "ffmt"), True):
            # Nothing compiles through ffmt here: every rejection is void.
            self.states["negative:accept_valid"] = "failed_to_compile"
            return
        for (name, kind), compiled in sorted(status.items()):
            if kind == "syntax" and not compiled:
                self.problems.append(f"negative:{name} is broken: it fails to compile even "
                                     "with the format call made a no-op")
            elif kind == "fmt" and compiled:
                self.problems.append(f"negative:{name}: {{fmt}} accepts it, so it is not a "
                                     "real restriction")
            elif kind == "ffmt" and compiled:
                self.problems.append(f"negative:{name}: ffmt accepts what {{fmt}} rejects")


def not_this_tree(commit):
    """Why results of `commit` cannot be accepted into this checkout, or None.
    The rows describe the files next to them, so the tree tested must be
    this one, apart from the two files an accept writes: results of an
    older commit are fine as long as only those changed since."""
    def git(*args):
        return subprocess.run(["git", *args], capture_output=True, check=False).returncode

    if git("cat-file", "-e", f"{commit}^{{commit}}"):
        return (f"these results come from {commit}, which is not in this clone "
                "(a merge commit a CI made for its run?)")
    if git("diff", "--quiet", commit, "--", ".", f":!{CONFIGS}", f":!{TESTS}"):
        return (f"these results come from {commit}, and this checkout differs from it "
                f"(git diff {commit}); check it out, or run again here")
    return None


def check(row, counts, accept):
    """Check one id against its row, or with accept rewrite the row from the
    results. Returns (problems, the test counts the run reported)."""
    res = Results(row["id"], counts)
    problems = list(res.problems)
    if not accept:
        problems += [f"{name} holds {n} tests, {TESTS} says {counts.get(name)}"
                     for name, n in sorted(res.counts.items()) if counts.get(name) != n]

    # The device the tests ran on, as the binaries name it: the setup script
    # selects it, and this is what tells when it selected another one.
    device = " + ".join(sorted(res.devices))
    if res.ran and not accept and row.get("device", device) != device:
        problems.append(f"ran on {device or 'no device'}, expected {row['device']}")

    # A build-only run knows nothing about the run states.
    states = STATES if res.ran else STATES[:1]
    expected = {t: s for s in states for t in row[s]}
    actual = {t: s for t, s in res.states.items() if s in states}
    moved = collections.defaultdict(list)
    for t in () if res.problems else set(expected) | set(actual):
        if expected.get(t) != actual.get(t):
            moved[expected.get(t, "passed"), actual.get(t, "passed")].append(t)
    changes = [f"{r}: expected {exp}, got {got}"
               for (exp, got), tests in sorted(moved.items()) for r in ranges(tests)]

    if not accept:
        problems += changes
    elif res.commit.endswith("-dirty"):
        # tested_commit must name the tree that was tested.
        problems.append("not accepted: these results come from uncommitted changes "
                        f"({res.commit}); commit, run again, then accept")
    elif reason := not_this_tree(res.commit):
        problems.append(f"not accepted: {reason}")
    elif problems:
        problems.append("not accepted: the config cannot express the above")
    else:
        for s in states:
            row[s] = sorted((t for t, st in actual.items() if st == s), key=key)
        row["tested_commit"] = res.commit
        if res.ran:
            row["device"] = device
        expected = {t: s for s in states for t in row[s]}
        moved = {}

    matched = collections.Counter(state for t, state in actual.items()
                                  if expected.get(t) == state)
    differences = sum(map(len, moved.values()))
    result = ("FAIL — configuration conformance failed" if problems else
              "PASS — results match config")
    matched_states = ", ".join(f"{matched[s]} {s}" for s in states)
    detail = [f"matched expected states: {matched_states}",
              f"unexpected state differences: {differences}"]
    print(f"id {row['id']} ({row['notes']}): {result}")
    for line in detail + (problems or [f"accepted: {c}" for c in changes]):
        print("  " + line)

    # The id's row of --table, so a failing id can be read without its log.
    if os.path.isdir(res.logs):
        with open(res.logs + "summary.md", "w") as f:
            cell = "<br>".join([result, *detail, *problems]).replace("|", "\\|")
            f.write(f"| {'❌' if problems else '✅'} {row['id']} | {row['notes']} | {cell} |\n")
    return problems, res.counts


def resize(rows, name, old, new):
    """A file went from old to new tests: the ids that cannot build it fail
    to compile all of them, which needs no run to know."""
    was = set(tests_of(name, range(1, old + 1)))
    for r in rows:
        if was and was <= set(r["failed_to_compile"]):
            now = set(r["failed_to_compile"]) - was | set(tests_of(name, range(1, new + 1)))
            r["failed_to_compile"] = sorted(now, key=key)


def summary(rows):
    """Which ids share which failures, from test/configs.json alone."""
    print(f"{'id':>4}  {STATES[0]:>17}  {STATES[1]:>13}  {STATES[2]:>20}  notes")
    for r in rows:
        a, b, c = (len(r[s]) for s in STATES)
        print(f"{r['id']:>4}  {a:>17}  {b:>13}  {c:>20}  {r['notes']}")
    # Tests that are in the same state on the same set of ids go together.
    ids_of = collections.defaultdict(list)
    for r in rows:
        for s in STATES:
            for t in r[s]:
                ids_of[s, t].append(r["id"])
    groups = collections.defaultdict(list)
    for (s, t), ids in ids_of.items():
        groups[s, tuple(ids)].append(t)
    for (s, ids), tests in sorted(groups.items(), key=lambda g: (-len(g[0][1]), g[0])):
        print(f"\n{s} on {len(ids)} id(s): {' '.join(map(str, ids))}")
        for r in ranges(tests):
            print("  " + r)


def table(by_id, ids):
    """One Markdown table of ids already checked. Returns whether any of them
    has problems or was not checked at all."""
    records = []
    passed = failed = incomplete = 0
    for id_ in ids:
        path = f"results/id-{id_}/summary.md"
        if os.path.exists(path):
            with open(path) as f:
                row = f.read().strip()
            if row.startswith("| ✅"):
                passed += 1
            else:
                failed += 1
        else:
            row = f"| ⚠️ {id_} | {by_id[id_]['notes']} | no result: test/check_id.py did not run |"
            incomplete += 1
        records.append(row)
    print(f"**Configuration conformance:** {len(ids)} IDs — {passed} PASS, "
          f"{failed} FAIL, {incomplete} incomplete.\n")
    print("**Legend:** `PASS — results match config` requires complete artifacts and matching "
          "observed states. Matched expected failure-state counts are separate from unexpected "
          "state differences. Raw TAP `not ok` lines describe observed failures, including "
          "expected ones; checker success means they match `test/configs.json`. Negative "
          "compile rejections are passing behavior, not XFAILs. Incomplete artifacts fail.\n")
    print("| id | config | overall result and state comparison |\n|---|---|---|")
    print(*records, sep="\n")
    return bool(failed + incomplete)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("ids", type=int, nargs="*", help="ids to check, from results/id-<id>/")
    ap.add_argument("--all", action="store_true",
                    help="every id that has a results/id-<id>/, in place of a list of ids")
    ap.add_argument("--accept-and-update-config", action="store_true", dest="accept",
                    help=f"rewrite the ids' rows of {CONFIGS}, and {TESTS}, from the results")
    ap.add_argument("--summary", action="store_true",
                    help="print which ids share which failures and exit")
    ap.add_argument("--table", action="store_true",
                    help="print the Markdown table of ids already checked")
    ap.add_argument("--list", metavar="CI",
                    help="print the ids of a CI job (jlse:chiatta) or of a whole CI (github:)")
    args = ap.parse_args()
    with open(CONFIGS) as f:
        rows = json.load(f)
    if args.summary:
        return summary(rows)
    if args.list:
        # A whole CI ends with its colon, so that jlse:a100 is not jlse:a100-extended too.
        whole = args.list.endswith(":")
        print(*(r["id"] for r in rows if "ci" in r
                and (r["ci"].startswith(args.list) if whole else r["ci"] == args.list)))
        return 0
    if args.all:
        args.ids = sorted(int(d.split("-")[1]) for d in glob.glob("results/id-*"))
    if not args.ids:
        ap.error("no results/id-<id>/ to check" if args.all else "give ids, --all or --summary")
    by_id = {r["id"]: r for r in rows}
    if args.table:
        return int(table(by_id, args.ids))
    with open(TESTS) as f:
        counts = json.load(f)
    failed = False
    for id_ in args.ids:
        problems, seen = check(by_id[id_], counts, args.accept)
        failed |= bool(problems)
        if args.accept and not problems:
            for name, n in seen.items():
                resize(rows, name, counts.get(name, 0), n)
            counts.update(seen)
    if args.accept:
        dump_configs(rows)
        with open(TESTS, "w") as f:
            json.dump(counts, f, indent=2, sort_keys=True)
            f.write("\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
