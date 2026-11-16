"""Unit tests of test/check_id.py, on results written by hand."""

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
EMPTY = {"failed_to_compile": [], "failed_to_run": [], "compiled_but_not_run": []}


def tap(count, not_ok=(), stop_in=None):
    """A binary's TAP log: `count` tests, the run dying in test `stop_in`."""
    lines = [f"1..{count}"]
    for n in range(1, count + 1):
        lines.append(f"# start {n}")
        if n == stop_in:
            break
        lines.append(f"{'not ok' if n in not_ok else 'ok'} {n} - x")
    return "\n".join(lines) + "\n"


class CheckIdTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "test").mkdir()
        shutil.copy(REPO / "test/check_id.py", self.root / "test")
        self.logs = self.root / "results/id-1"
        self.logs.mkdir(parents=True)
        self.configure()
        (self.root / "test/tests.json").write_text(json.dumps({"a": 3, "b": 2}))
        # A complete run where everything passes.
        self.write("config.txt", "commit=abc\ngoal=test\n")
        self.write("tests.log", "0 build a\n0 build b\n0 run a\n0 run b\n")
        self.write("a.run.log", tap(3))
        self.write("b.run.log", tap(2))
        self.write("negative.log", "0 syntax accept_valid\n0 fmt accept_valid\n0 ffmt accept_valid\n"
                                   "0 syntax reject_x\n1 fmt reject_x\n1 ffmt reject_x\n")
        self.write("extra.log", "0 build readme\n0 run readme\n")

    def configure(self, **states):
        row = {"id": 1, "notes": "synthetic", "tested_commit": "N/A", **EMPTY, **states}
        (self.root / "test/configs.json").write_text(json.dumps([row]))

    def write(self, name, contents):
        (self.logs / name).write_text(contents)

    def check(self, *args):
        return subprocess.run([sys.executable, "test/check_id.py", "1", *args], cwd=self.root,
                              capture_output=True, text=True, check=False)

    def row(self):
        return json.loads((self.root / "test/configs.json").read_text())[0]

    def test_all_pass(self):
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("✅ 1", (self.logs / "summary.md").read_text())

    def test_unexpected_failure_and_unexpected_pass(self):
        self.write("tests.log", "0 build a\n0 build b\n88 run a\n0 run b\n")
        self.write("a.run.log", tap(3, not_ok=[2]))
        result = self.check()
        self.assertEqual(result.returncode, 1)
        self.assertIn("a:2: expected passed, got failed_to_run", result.stdout)
        self.configure(failed_to_run=["a:2"])
        self.assertEqual(self.check().returncode, 0)
        self.configure(failed_to_run=["a:2", "b:1"])
        self.assertIn("b:1: expected failed_to_run, got passed", self.check().stdout)

    def test_file_that_does_not_build(self):
        self.write("tests.log", "1 build a\n0 build b\n0 run b\n")
        self.assertIn("a:1-3: expected passed, got failed_to_compile", self.check().stdout)
        self.configure(failed_to_compile=["a:1", "a:2", "a:3"])
        self.assertEqual(self.check().returncode, 0)

    def test_crash_names_its_test_and_those_not_run(self):
        self.write("tests.log", "0 build a\n0 build b\n139 run a\n0 run b\n")
        self.write("a.run.log", tap(3, stop_in=2))
        result = self.check()
        self.assertIn("a:2: expected passed, got failed_to_run", result.stdout)
        self.assertIn("a:3: expected passed, got compiled_but_not_run", result.stdout)
        self.configure(failed_to_run=["a:2"], compiled_but_not_run=["a:3"])
        self.assertEqual(self.check().returncode, 0)

    def test_what_no_state_can_express_always_fails(self):
        for name, contents, message in [
                ("tests.log", "0 build a\n0 build b\n0 run b\n", "a did not finish"),
                ("tests.log", "0 build a\n0 build b\n1 run a\n0 run b\n", "a: exit status 1 after test 3"),
                ("a.run.log", "", "a: exit status 0 before its first test"),
                ("a.run.log", tap(4), "a holds 4 tests, test/tests.json says 3"),
                ("negative.log", "0 syntax reject_x\n1 fmt reject_x\n0 ffmt reject_x\n",
                 "negative:reject_x: ffmt accepts what {fmt} rejects"),
                ("config.txt", "", "no goal"),
        ]:
            with self.subTest(message):
                saved = (self.logs / name).read_text()
                self.write(name, contents)
                result = self.check()
                self.assertEqual(result.returncode, 1)
                self.assertIn(message, result.stdout)
                self.write(name, saved)
        (self.logs / "tests.log").unlink()
        self.assertIn("tests.log: that step did not run", self.check().stdout)

    def test_extra_and_negative_control_have_states(self):
        self.write("extra.log", "0 build readme\n1 run readme\n1 build other\n")
        self.write("negative.log", "0 syntax accept_valid\n0 fmt accept_valid\n1 ffmt accept_valid\n"
                                   "0 syntax reject_x\n1 fmt reject_x\n0 ffmt reject_x\n")
        result = self.check()
        self.assertIn("extra:readme: expected passed, got failed_to_run", result.stdout)
        self.assertIn("extra:other: expected passed, got failed_to_compile", result.stdout)
        self.assertIn("negative:accept_valid: expected passed, got failed_to_compile", result.stdout)
        self.assertNotIn("reject_x", result.stdout)

    def test_build_only_run_checks_only_what_it_built(self):
        self.write("config.txt", "commit=abc\ngoal=build\n")
        self.write("tests.log", "1 build a\n0 build b\n")
        self.configure(failed_to_compile=["a:1", "a:2", "a:3"], failed_to_run=["b:1"])
        self.assertEqual(self.check().returncode, 0)

    def test_accept_rewrites_the_row_and_the_counts(self):
        self.write("tests.log", "0 build a\n1 build b\n139 run a\n")
        self.write("a.run.log", tap(4, not_ok=[1], stop_in=3))
        result = self.check("--accept-and-update-config")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.row(), {
            "id": 1, "notes": "synthetic", "tested_commit": "abc",
            "failed_to_compile": ["b:1", "b:2"], "failed_to_run": ["a:1", "a:3"],
            "compiled_but_not_run": ["a:4"]})
        self.assertEqual(json.loads((self.root / "test/tests.json").read_text()), {"a": 4, "b": 2})
        self.assertEqual(self.check().returncode, 0)

    def test_table_shows_every_id_checked_or_not(self):
        self.configure()
        rows = [{"id": i, "notes": f"config {i}", "tested_commit": "N/A", **EMPTY} for i in (1, 2)]
        (self.root / "test/configs.json").write_text(json.dumps(rows))
        self.assertEqual(self.check().returncode, 0)
        result = self.check("--table")
        self.assertEqual(result.returncode, 0)
        self.assertIn("| ✅ 1 | config 1 | as expected", result.stdout)
        result = self.check("2", "--table")
        self.assertEqual(result.returncode, 1)
        self.assertIn("| ⚠️ 2 | config 2 | no result", result.stdout)

    def test_accept_refuses_uncommitted_results_and_problems(self):
        self.write("a.run.log", tap(3, not_ok=[1]))
        self.write("tests.log", "0 build a\n0 build b\n88 run a\n0 run b\n")
        self.write("config.txt", "commit=abc-dirty\ngoal=test\n")
        self.assertIn("uncommitted changes", self.check("--accept-and-update-config").stdout)
        self.write("config.txt", "commit=abc\ngoal=test\n")
        self.write("tests.log", "0 build a\n0 build b\n88 run a\n")
        self.assertIn("not accepted", self.check("--accept-and-update-config").stdout)
        self.assertEqual(self.row()["failed_to_run"], [])


if __name__ == "__main__":
    unittest.main()
