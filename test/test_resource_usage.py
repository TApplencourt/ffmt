"""Unit tests of test/resource_usage.py: its parsers, on compiler output
copied from real builds, and compare, on measurements written by hand."""

import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import resource_usage

PTXAS = """\
ptxas info    : Compiling entry function '_Z1kv' for 'sm_80'
ptxas info    : Function properties for _Z1kv
    792 bytes stack frame, 8 bytes spill stores, 8 bytes spill loads
ptxas info    : Used 31 registers, 1024 bytes smem, 353 bytes cmem[0]
ptxas info    : Function properties for helper
    16 bytes stack frame, 0 bytes spill stores, 0 bytes spill loads
"""

AMD = """\
x.hpp:1:1: remark: Function Name: k1 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     TotalSGPRs: 48 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     VGPRs: 56 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     AGPRs: 32 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     ScratchSize [bytes/lane]: 2608 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     Occupancy [waves/SIMD]: 5 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     SGPRs Spill: 2 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     VGPRs Spill: 0 [-Rpass-analysis=kernel-resource-usage]
x.hpp:1:1: remark:     LDS Size [bytes/block]: 64 [-Rpass-analysis=kernel-resource-usage]
"""

AMD_LINK = """\
test/capture.hpp:133:0: Function Name: k2
test/capture.hpp:133:0:     TotalSGPRs: 44
test/capture.hpp:133:0:     VGPRs: 36
test/capture.hpp:133:0:     AGPRs: 0
test/capture.hpp:133:0:     ScratchSize [bytes/lane]: 788
test/capture.hpp:133:0:     VGPRs Spill: 0
test/capture.hpp:133:0:     LDS Size [bytes/block]: 0
"""

ZEINFO = """\
version: '1.73'
kernels:
  - name:            kA
    execution_env:
      grf_count:       128
      simd_size:       32
      slm_size:        256
    payload_arguments:
      - arg_type:        global_id_offset
        offset:          0
        size:            12
    per_thread_memory_buffers:
      - type:            scratch
        usage:           private_space
        size:            1024
      - type:            scratch
        usage:           spill_fill_space
        size:            64
  - name:            kB
    execution_env:
      grf_count:       256
functions:
  - name:            some_function
    execution_env:
      grf_count:       128
"""


class Parsers(unittest.TestCase):
    def test_ptxas_counts_entry_functions_only(self):
        self.assertEqual(
            resource_usage.parse_ptxas(PTXAS),
            {"_Z1kv": {"regs": 31, "stack": 792, "spill": 8, "slm": 1024}},
        )

    def test_amd_adds_vgprs_and_agprs_and_both_spills(self):
        self.assertEqual(
            resource_usage.parse_amd_remarks(AMD),
            {"k1": {"regs": 88, "stack": 2608, "spill": 2, "slm": 64}},
        )

    def test_amd_remarks_from_the_linker_wrapper(self):
        self.assertEqual(
            resource_usage.parse_amd_remarks(AMD_LINK),
            {"k2": {"regs": 36, "stack": 788, "spill": 0, "slm": 0}},
        )

    def test_zeinfo_skips_payload_sizes(self):
        self.assertEqual(
            resource_usage.parse_zeinfo(ZEINFO),
            {
                "kA": {"regs": 128, "stack": 1024, "spill": 64, "slm": 256},
                "kB": {"regs": 256, "stack": 0, "spill": 0, "slm": 0},
            },
        )


def write(directory, id_, files):
    path = Path(directory) / f"id-{id_}.json"
    path.write_text(json.dumps({"id": id_, "backend": "nvidia", "files": files}))


def file_of(*kernels):
    """A test file's measurement, from its kernels' (regs, stack, spill, slm)."""
    return resource_usage.summary(
        {f"k{i}": dict(zip(resource_usage.METRICS, k)) for i, k in enumerate(kernels)}
    )


class Summary(unittest.TestCase):
    def test_min_average_max(self):
        regs = file_of((28, 0, 0, 0), (31, 0, 0, 0), (40, 0, 0, 0))["regs"]
        self.assertEqual(regs, {"min": 28, "avg": 33.0, "max": 40})

    def test_short_names(self):
        self.assertEqual(
            resource_usage.short("__omp_offloading_28_7a3d403e_main_l55"), "line 55"
        )
        self.assertEqual(
            resource_usage.short("_Z16_ffmt_run_kernelIZZ4mainENKUlvE0_clEvEUlvE_EvT_"),
            "lambda #1",
        )
        self.assertEqual(
            resource_usage.short(
                "_Z16_ffmt_run_kernelIZZ4mainENKUlvE0_clEvEUlvE29_EvT_"
            ),
            "lambda #31",
        )


class Measure(unittest.TestCase):
    """measure reads what test/run_id.sh <id> build left in results/id-<id>/."""

    def run_measure(self, tests_log, logs):
        with tempfile.TemporaryDirectory() as tree:
            results = Path(tree) / "results/id-36"
            results.mkdir(parents=True)
            if tests_log is not None:
                (results / "tests.log").write_text(tests_log)
            for name, text in logs.items():
                (results / f"{name}.build.log").write_text(text)
            out = Path(tree) / "id-36.json"
            resource_usage.measure(36, str(out), tree)
            return json.loads(out.read_text())

    def test_reads_the_build_logs(self):
        run = self.run_measure(
            "0 build floats\n1 build fill\n", {"floats": PTXAS, "fill": "error"}
        )
        self.assertEqual(run["backend"], "nvidia")
        self.assertEqual(
            run["files"]["floats"]["regs"], {"min": 31, "avg": 31.0, "max": 31}
        )
        self.assertIsNone(run["files"]["fill"])
        self.assertNotIn("missing", run)

    def test_no_build(self):
        self.assertIn("did not run", self.run_measure(None, {})["missing"])

    def test_a_build_without_the_flags(self):
        run = self.run_measure(
            "0 build floats\n", {"floats": "no compiler report here"}
        )
        self.assertIn("FFMT_TEST_FLAGS", run["missing"])


class Compare(unittest.TestCase):
    def run_compare(self, base, head, failed_to_compile=()):
        with tempfile.TemporaryDirectory() as b, tempfile.TemporaryDirectory() as h:
            if base is not None:
                write(b, 36, base)
            write(h, 36, head)
            configs = Path(h) / "configs.json"
            configs.write_text(
                json.dumps([{"id": 36, "failed_to_compile": list(failed_to_compile)}])
            )
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                code = resource_usage.compare(b, h, configs)
            return code, out.getvalue()

    def test_same_numbers_pass_and_list_nothing(self):
        files = {"floats": file_of((31, 792, 0, 0), (40, 792, 0, 0))}
        code, out = self.run_compare(files, files)
        self.assertEqual(code, 0)
        self.assertIn("No test grew", out)
        self.assertNotIn("Missing", out)

    def test_one_test_that_grows_fails_even_if_the_file_max_does_not(self):
        base = {"floats": file_of((31, 792, 0, 0), (40, 792, 0, 0))}
        head = {"floats": file_of((35, 792, 0, 0), (40, 792, 0, 0))}
        code, out = self.run_compare(base, head)
        self.assertEqual(code, 1)
        self.assertIn("id 36 floats, k0: regs 31 → 35", out)
        self.assertIn("**31 / 35.5 / 40 → 35 / 37.5 / 40**", out)

    def test_one_test_up_another_down_still_fails(self):
        base = {"floats": file_of((31, 0, 0, 0), (40, 0, 0, 0))}
        head = {"floats": file_of((32, 0, 0, 0), (30, 0, 0, 0))}
        code, out = self.run_compare(base, head)
        self.assertEqual(code, 1)
        self.assertIn("k0: regs 31 → 32", out)

    def test_a_drop_passes(self):
        code, out = self.run_compare(
            {"floats": file_of((31, 792, 8, 0))}, {"floats": file_of((31, 512, 0, 0))}
        )
        self.assertEqual(code, 0)
        self.assertIn("792 / 792 / 792 → 512 / 512 / 512", out)

    def test_a_new_test_passes_and_is_listed(self):
        base = {"floats": file_of((31, 0, 0, 0))}
        head = {"floats": file_of((31, 0, 0, 0), (64, 0, 0, 0))}
        code, out = self.run_compare(base, head)
        self.assertEqual(code, 0)
        self.assertIn("Missing in the reference (new test added?): 1", out)
        self.assertIn("- id 36 floats, k1", out)

    def test_no_reference_at_all_passes(self):
        code, out = self.run_compare(None, {"floats": file_of((31, 0, 0, 0))})
        self.assertEqual(code, 0)
        self.assertIn("id 36: no reference at all (new id?)", out)

    def test_slm_growth_fails(self):
        code, out = self.run_compare(
            {"floats": file_of((31, 0, 0, 0))}, {"floats": file_of((31, 0, 0, 512))}
        )
        self.assertEqual(code, 1)
        self.assertIn("k0: slm 0 → 512", out)

    def test_a_file_that_stops_building_fails(self):
        code, out = self.run_compare(
            {"floats": file_of((31, 792, 0, 0))}, {"floats": None}
        )
        self.assertEqual(code, 1)
        self.assertIn("floats: no longer builds", out)

    def test_a_file_expected_not_to_build_is_skipped(self):
        code, out = self.run_compare(
            {"fill": file_of((31, 0, 0, 0))},
            {"fill": None},
            failed_to_compile=["fill:1"],
        )
        self.assertEqual(code, 0)
        self.assertIn("not built, as test/configs.json expects", out)


if __name__ == "__main__":
    unittest.main()
