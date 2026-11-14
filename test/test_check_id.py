#!/usr/bin/env python3
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[1]


class CheckerTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root / "test").mkdir()
        (self.root / "results/id-1").mkdir(parents=True)
        shutil.copy(REPO / "test/check_id.py", self.root / "test/check_id.py")
        for name in ("escape_percent", "floats", "formatter", "integers", "layout", "misc", "strings"):
            (self.root / ("test/test_%s.cpp" % name)).touch()
        self.configure()
        self.complete_run()

    def tearDown(self):
        self.temp.cleanup()

    def configure(self, compile=(), runtime=(), structural=()):
        row = {"id": 1, "notes": "synthetic", "expected_fail": {
            "compile": list(compile), "runtime": list(runtime), "structural": list(structural)}}
        (self.root / "test/configs.json").write_text(json.dumps([row]))

    def write(self, name, contents=""):
        path = self.root / "results/id-1" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)

    def complete_run(self):
        self.write("run_id.status", "0\n")
        self.write("main-build.status", "0\n")
        self.write("build.log", "build complete\n")
        self.write("run.status", "0\n")
        self.write("run.log")
        self.write("isolated.log")
        self.write("negative.log")
        self.write("extra.log")

    def check(self, mode="test"):
        return subprocess.run([sys.executable, "test/check_id.py", "1", mode], cwd=self.root,
                              text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def crash_main(self):
        self.write("run_id.status", "1\n")
        self.write("run.status", "134\n")

    def test_pass(self):
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("PASS", result.stdout)

    def test_exact_structural_xfail(self):
        self.configure(structural=["main:crash"])
        self.crash_main()
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("XFAIL", result.stdout)
        self.assertIn("expected structural failure: main:crash", result.stdout)
        self.assertIn("⚠️", (self.root / "results/id-1/summary.md").read_text())

    def test_unexpected_structural_failure(self):
        self.crash_main()
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("unexpected structural failure: main:crash", result.stdout)

    def test_structural_xpass(self):
        self.configure(structural=["main:crash"])
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("unexpected structural pass (XPASS): main:crash", result.stdout)

    def test_blanket_structural_configuration_is_rejected(self):
        rows = json.loads((self.root / "test/configs.json").read_text())
        rows[0]["expected_fail"]["structural"] = True
        (self.root / "test/configs.json").write_text(json.dumps(rows))
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("must be a list of exact signatures", result.stdout)

    def test_missing_run_log_is_failure(self):
        self.configure(structural=["main:crash"])
        self.crash_main()
        (self.root / "results/id-1/run.log").unlink()
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("no run.log", result.stdout)
        self.assertNotIn("Traceback", result.stdout)

    def test_interruption_after_main_status_is_failure(self):
        (self.root / "results/id-1/run_id.status").unlink()
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("no run_id.status", result.stdout)

    def test_interrupted_main_build_is_failure(self):
        self.configure(structural=["main:build"])
        (self.root / "results/id-1/run_id.status").unlink()
        (self.root / "results/id-1/run.status").unlink()
        (self.root / "results/id-1/run.log").unlink()
        self.write("build.log", "compiler failure\n")
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("no run_id.status", result.stdout)

    def test_main_build_xfail_does_not_waive_missing_status_log(self):
        self.configure(structural=["main:build"])
        self.write("run_id.status", "2\n")
        self.write("main-build.status", "2\n")
        (self.root / "results/id-1/run.status").unlink()
        (self.root / "results/id-1/run.log").unlink()
        (self.root / "results/id-1/isolated.log").unlink()
        self.write("build.log", "compiler failure\n")
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("no isolated.log", result.stdout)

    def test_completed_main_build_failure_is_xfail(self):
        self.configure(structural=["main:build"])
        self.write("run_id.status", "2\n")
        self.write("main-build.status", "2\n")
        (self.root / "results/id-1/run.status").unlink()
        (self.root / "results/id-1/run.log").unlink()
        self.write("build.log", "compiler failure\n")
        self.write("extra.log", "0 build readme\n")
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("XFAIL", result.stdout)
        self.assertNotIn("readme built but did not run", result.stdout)

    def test_main_build_xfail_requires_main_build_failure(self):
        self.configure(structural=["main:build"])
        self.write("run_id.status", "2\n")
        (self.root / "results/id-1/run.status").unlink()
        (self.root / "results/id-1/run.log").unlink()
        self.write("build.log", "unrelated make failure\n")
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("the main binary built but run.status is missing", result.stdout)

    def test_main_crash_does_not_hide_isolated_xpass(self):
        self.configure(runtime=["misc:42", "nvptx_o2_ice:crash"], structural=["main:crash"])
        self.crash_main()
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertNotIn("XPASS): misc:42", result.stdout)
        self.assertIn("unexpected runtime pass (XPASS): nvptx_o2_ice:crash", result.stdout)

    def test_main_crash_preserves_observed_main_expectations(self):
        self.configure(runtime=["misc:42"], structural=["main:crash"])
        self.crash_main()
        self.write("run.log", "not ok 42 - test_misc.cpp\n")
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("XFAIL", result.stdout)

    def test_new_failure_alongside_xfail_is_failure(self):
        self.configure(structural=["main:crash"])
        self.crash_main()
        self.write("isolated.log", "1 build test_float_sweep.cpp\n")
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("unexpected compile failure: isolated:float_sweep", result.stdout)
        self.assertIn("expected structural failure: main:crash", result.stdout)

    def test_failed_negative_control_invalidates_only_negative_results(self):
        self.configure(compile=["negative:reject_example", "isolated:float_sweep"],
                       structural=["negative-control:accept_valid:ffmt"])
        self.write("negative.log", "0 syntax accept_valid\n0 fmt accept_valid\n1 ffmt accept_valid\n"
                                   "0 syntax reject_example\n1 fmt reject_example\n1 ffmt reject_example\n")
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertNotIn("negative:reject_example", result.stdout)
        self.assertIn("unexpected compile pass (XPASS): isolated:float_sweep", result.stdout)

    def prepare_extra_test(self):
        self.configure(structural=["main:build"])
        self.write("run_id.status", "2\n")
        self.write("main-build.status", "2\n")
        (self.root / "results/id-1/run.status").unlink()
        (self.root / "results/id-1/run.log").unlink()
        self.write("build.log", "compiler failure\n")
        self.write("extra.log", "0 build readme\n")

    def test_interrupted_extra_test_is_failure(self):
        self.prepare_extra_test()
        result = self.check("extra-test")
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("no extra-test.status", result.stdout)

    def test_extra_test_does_not_waive_unrun_example_for_main_build(self):
        self.prepare_extra_test()
        self.write("extra-test.status", "0\n")
        result = self.check("extra-test")
        self.assertEqual(result.returncode, 1, result.stdout)
        self.assertIn("unexpected structural failure: extra:readme:not-run", result.stdout)


class BatchTest(unittest.TestCase):
    def test_run_id_records_completion_and_removes_stale_status(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "test/setup").mkdir(parents=True)
            (root / "results/id-1").mkdir(parents=True)
            (root / "bin").mkdir()
            shutil.copy(REPO / "test/run_id.sh", root / "test/run_id.sh")
            (root / "test/setup/setup_1.sh").write_text("export CXX=true\nexport FFMT_FLAGS=-O0\n")
            (root / "bin/make").write_text("#!/bin/sh\nexit 2\n")
            (root / "bin/make").chmod(0o755)
            (root / "results/id-1/run_id.status").write_text("stale\n")
            env = os.environ.copy()
            env["PATH"] = str(root / "bin") + os.pathsep + env["PATH"]
            result = subprocess.run(["test/run_id.sh", "1"], cwd=root, env=env,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.assertEqual(result.returncode, 2, result.stdout)
            self.assertEqual((root / "results/id-1/run_id.status").read_text(), "2\n")
            (root / "results/id-1/extra-test.status").write_text("stale\n")
            result = subprocess.run(["test/run_id.sh", "1", "extra-test"], cwd=root, env=env,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.assertEqual(result.returncode, 2, result.stdout)
            self.assertEqual((root / "results/id-1/extra-test.status").read_text(), "2\n")
            self.assertEqual((root / "results/id-1/run_id.status").read_text(), "2\n")

    def test_batch_continues_after_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "test").mkdir()
            shutil.copy(REPO / "test/run_ids.sh", root / "test/run_ids.sh")
            (root / "test/run_id.sh").write_text(
                "#!/bin/sh\nprintf '%s\\n' \"$1\" >> invoked\nexit \"$1\"\n")
            (root / "test/run_id.sh").chmod(0o755)
            (root / "test/check_id.py").write_text(
                "import pathlib, sys\n"
                "i = int(sys.argv[1])\n"
                "pathlib.Path('results/id-%d/summary.md' % i).write_text(str(i))\n"
                "sys.exit(1 if i == 2 else 0)\n")
            env = os.environ.copy()
            env["COBALT_JOBID"] = "1015999"
            result = subprocess.run(["test/run_ids.sh", "1", "2", "3"], cwd=root, env=env,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.assertEqual(result.returncode, 1, result.stdout)
            self.assertEqual((root / "invoked").read_text().splitlines(), ["1", "2", "3"])
            self.assertEqual((root / "results/batch-status.tsv").read_text().splitlines()[1:],
                             ["1\t1\t0", "2\t2\t1", "3\t3\t0"])
            self.assertEqual((root / "results/cobalt-job-id").read_text(), "1015999\n")
            for id_ in (1, 2, 3):
                self.assertTrue((root / "results" / ("id-%d" % id_) / "summary.md").exists())


if __name__ == "__main__":
    unittest.main()
