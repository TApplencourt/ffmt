import json
from pathlib import Path
import tempfile
import unittest

import aggregate_jlse


class AggregateJlseTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.configs = {row["id"]: row for row in
                        json.loads(Path("test/configs.json").read_text())}
        self.metadata = {"pipeline": {"id": 123, "web_url": "https://gitlab.example/pipelines/123"}}
        for number, batch in enumerate(aggregate_jlse.BATCHES, 1):
            self.metadata[batch] = {
                "id": number,
                "status": "success",
                "web_url": "https://gitlab.example/jobs/%d" % number,
                "cobalt_id": str(900000 + number),
            }
            self.write_batch(batch, aggregate_jlse.BATCHES[batch])

    def tearDown(self):
        self.temporary.cleanup()

    def write_summary(self, batch, id_, status, details=None):
        glyphs = {"PASS": "✅", "XFAIL": "⚠️", "FAIL": "❌"}
        if details is None:
            details = ["%s (1 compile, 2 runtime, 3 structural expected failures)" % status]
        path = self.root / batch / "results" / ("id-%d" % id_) / "summary.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("| %s %d | fixture | %s |\n" % (glyphs["FAIL" if status == "XPASS" else status],
                                                        id_, "<br>".join(details)))

    def write_batch(self, batch, ids, check_status=0):
        results = self.root / batch / "results"
        results.mkdir(parents=True, exist_ok=True)
        status_lines = ["id\trun_id_status\tcheck_id_status"]
        for id_ in ids:
            self.write_summary(batch, id_, "PASS")
            status_lines.append("%d\t0\t%d" % (id_, check_status))
        (results / "batch-status.tsv").write_text("\n".join(status_lines) + "\n")

    def render(self):
        return aggregate_jlse.report(self.metadata, self.root, self.configs)

    def test_complete_batches(self):
        self.write_summary("chiatta", 2, "XFAIL", [
            "XFAIL (55 compile, 2 runtime, 3 structural expected failures)",
            "expected structural failure: main:crash (exit 139)",
        ])
        output, failures = self.render()
        self.assertEqual(failures, 0, output)
        self.assertIn("**Totals:** 56 IDs — 55 PASS, 1 XFAIL, 0 XPASS, 0 FAIL, 0 incomplete.",
                      output)
        self.assertIn("**Unexpected or incomplete: 0.**", output)
        self.assertIn(self.configs[2]["notes"], output)
        self.assertIn("expected structural failure: main:crash (exit 139)", output)
        self.assertIn("[1](https://gitlab.example/jobs/1)", output)
        self.assertIn("900001", output)
        self.assertEqual(output.count("| chiatta |"), 4)

    def test_batch_failures_remain_visible(self):
        self.metadata["a100"]["status"] = "failed"
        self.write_summary("a100", 4, "XPASS", [
            "FAIL (1 problem(s)); observed 0 compile, 0 runtime, 0 structural expected failures",
            "unexpected compile pass (XPASS): isolated:alternate",
        ])
        self.write_summary("a100", 5, "FAIL", [
            "FAIL (1 problem(s)); observed 0 compile, 1 runtime, 0 structural expected failures",
            "unexpected runtime failure: floats:104",
        ])
        self.write_batch("a100", [6, 9])
        results = self.root / "a100/results/batch-status.tsv"
        results.write_text("id\trun_id_status\tcheck_id_status\n"
                           "4\t0\t1\n5\t0\t1\n6\t0\t0\n9\t0\t0\n")
        output, failures = self.render()
        self.assertEqual(failures, 2, output)
        self.assertIn("54 PASS, 0 XFAIL, 1 XPASS, 1 FAIL, 0 incomplete", output)
        self.assertIn("unexpected compile pass (XPASS): isolated:alternate", output)
        self.assertIn("unexpected runtime failure: floats:104", output)
        self.assertIn("| a100 | 9 |", output)

    def test_missing_artifacts_are_incomplete(self):
        self.write_batch("a100-extended", [18])  # ID 23 has no batch row or summary.
        for path in (self.root / "mi300x").glob("*"):
            if path.is_dir():
                import shutil
                shutil.rmtree(path)
            else:
                path.unlink()
        output, failures = self.render()
        self.assertEqual(failures, 3, output)
        self.assertIn("53 PASS, 0 XFAIL, 0 XPASS, 0 FAIL, 3 incomplete", output)
        self.assertIn("missing ID row in results/batch-status.tsv", output)
        self.assertIn("batch-status.tsv", output)
        self.assertIn("| a100-extended | 23 |", output)
        self.assertIn("| mi300x | 7 |", output)

    def test_unexplained_batch_failure_is_not_hidden(self):
        self.metadata["mi300x"]["status"] = "failed"
        output, failures = self.render()
        self.assertEqual(failures, 2, output)
        self.assertIn("54 PASS, 0 XFAIL, 0 XPASS, 0 FAIL, 2 incomplete", output)
        self.assertIn("GitLab job failed without a per-ID checker failure", output)


if __name__ == "__main__":
    unittest.main()
