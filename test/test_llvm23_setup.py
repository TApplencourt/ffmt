import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
LLVM23_IDS = (4, 5, 6, 9, 18, 19, 20, 23, 54, 55, 56, 57, 58, 59, 78, 79)


class Llvm23SetupTest(unittest.TestCase):
    def test_only_requested_jobs_load_llvm23(self):
        loaded = set()
        for path in ROOT.glob("test/setup/setup_*.sh"):
            test_id = int(path.stem.removeprefix("setup_"))
            if "module load llvm/release-23.1.6" in path.read_text():
                loaded.add(test_id)
        self.assertEqual(loaded, set(LLVM23_IDS))

    def test_llvm23_jobs_use_architecture_selecting_module(self):
        rows = {row["id"]: row for row in json.loads((ROOT / "test/configs.json").read_text())}
        for test_id in LLVM23_IDS:
            with self.subTest(id=test_id):
                setup = (ROOT / f"test/setup/setup_{test_id}.sh").read_text()
                self.assertIn(
                    'module use "${FFMT_RUNTIME_ROOT:?FFMT_RUNTIME_ROOT must be set}/modulefiles"',
                    setup)
                self.assertIn("export CXX=clang++", setup)
                self.assertNotIn("export CXX=/soft/compilers/llvm/", setup)
                self.assertNotRegex(setup, r"module load llvm/release-(22\.1\.8|23\.1\.3)\n")
                self.assertIn("llvm release-23.1.6", rows[test_id]["notes"])

    def test_basic_and_extended_jobs_inherit_runtime_root(self):
        ci = (ROOT / ".gitlab-ci.yml").read_text()
        basic = ci.split(".jlse-batch:", 1)[1].split("\nchiatta:", 1)[0]
        self.assertIn(
            "FFMT_RUNTIME_ROOT: /vast/projects/intel_anl_shared/ffmt-ci/releases/20261110-r1",
            basic)


if __name__ == "__main__":
    unittest.main()
