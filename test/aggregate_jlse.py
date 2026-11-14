#!/usr/bin/env python3
"""Build a pipeline-wide JLSE table from existing per-job artifacts."""

import argparse
import io
import json
import os
from pathlib import Path
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
import zipfile


BATCHES = {
    "chiatta": [2, 3, 10, 11],
    "a100": [4, 5, 6, 9],
    "mi300x": [7, 8],
    "chiatta-dpcpp": [14, 15, 16, 17, 64, 65, 82, 83],
    "a100-extended": [18, 23],
    "a100-omp-fast-math": [19, 20],
    "mi300x-omp-fast-math": [21, 22],
    "mi300x-hip": [48, 49, 80, 81],
    "h100-extended": [50, 51, 54, 55, 56, 57],
    "a40-extended": [78, 79],
    "adaptivecpp-a40": [70, 71],
    "adaptivecpp-mi300x": [72, 73],
    "chipstar": [74, 75],
    "gh200-extended": [52, 53, 58, 59, 62, 63, 68, 69],
    "rome-extended": [60, 61, 66, 67, 76, 77],
}


def markdown(value):
    return str(value).replace("|", "\\|").replace("\n", "<br>")


def summary_parts(path):
    """Return the checker's outcome and its existing count/diagnostic cells."""
    lines = path.read_text(errors="replace").splitlines()
    if len(lines) != 1:
        raise ValueError("summary.md does not contain exactly one table row")
    cells = re.split(r"(?<!\\)\|", lines[0])
    if len(cells) != 5 or cells[0].strip() or cells[-1].strip():
        raise ValueError("summary.md is not the checker's three-column table row")
    details = cells[3].strip().split("<br>")
    outcome = details[0].split(" ", 1)[0]
    if outcome not in {"PASS", "XFAIL", "FAIL"}:
        raise ValueError("summary.md has an unknown checker outcome")
    if outcome == "FAIL":
        problems = [detail for detail in details[1:]
                    if not detail.startswith("expected structural failure: ")]
        if problems and all(re.match(r"unexpected .+ pass \(XPASS\): ", problem)
                            for problem in problems):
            outcome = "XPASS"
    return outcome, "<br>".join(details)


def batch_statuses(path):
    rows = {}
    lines = path.read_text(errors="replace").splitlines()
    if not lines or lines[0].split("\t") != ["id", "run_id_status", "check_id_status"]:
        raise ValueError("batch-status.tsv has an invalid header")
    for line in lines[1:]:
        fields = line.split("\t")
        if len(fields) != 3:
            raise ValueError("batch-status.tsv has an invalid row")
        id_text, run_status, check_status = fields
        id_ = int(id_text)
        if id_ in rows:
            raise ValueError("batch-status.tsv repeats ID %d" % id_)
        rows[id_] = int(run_status), int(check_status)
    return rows


def report(metadata, artifact_root, configs):
    rows = []
    job_notes = []
    for batch, ids in BATCHES.items():
        job = metadata.get(batch, {})
        job_id = job.get("id")
        job_url = job.get("web_url")
        if job_id is not None and job_url:
            job_cell = "[%s](%s)" % (job_id, job_url)
        elif job_id is not None:
            job_cell = str(job_id)
        else:
            job_cell = "missing"
        results = artifact_root / batch / "results"
        cobalt = str(job.get("cobalt_id") or "not recorded")
        try:
            artifact_cobalt = (results / "cobalt-job-id").read_text().strip()
            if re.fullmatch(r"[0-9]+", artifact_cobalt):
                cobalt = artifact_cobalt
        except OSError:
            pass
        statuses = None
        batch_error = None
        try:
            statuses = batch_statuses(results / "batch-status.tsv")
        except (OSError, ValueError) as error:
            batch_error = str(error)
        job_status = job.get("status", "missing")
        if job.get("error"):
            job_notes.append("%s: %s" % (batch, job["error"]))
        batch_start = len(rows)
        for id_ in ids:
            notes = configs[id_]["notes"]
            status = "incomplete"
            diagnostics = []
            check_status = None
            if batch_error:
                diagnostics.append(batch_error)
            elif id_ not in statuses:
                diagnostics.append("missing ID row in results/batch-status.tsv")
            else:
                _, check_status = statuses[id_]
                try:
                    summary_status, diagnostics_text = summary_parts(
                        results / ("id-%d" % id_) / "summary.md")
                    diagnostics.append(diagnostics_text)
                    valid = ((check_status == 0 and summary_status in {"PASS", "XFAIL"}) or
                             (check_status != 0 and summary_status in {"FAIL", "XPASS"}))
                    if valid:
                        status = summary_status
                    else:
                        diagnostics.append(
                            "checker status %d is inconsistent with summary outcome %s" %
                            (check_status, summary_status))
                except (OSError, ValueError) as error:
                    diagnostics.append(str(error))
            if job_status not in {"success", "failed"}:
                status = "incomplete"
                diagnostics.append("GitLab job status is %s" % job_status)
            rows.append({"batch": batch, "id": id_, "notes": notes,
                         "status": status, "details": "<br>".join(diagnostics) or "missing diagnostics",
                         "job": job_cell, "cobalt": cobalt})
        if (job_status == "failed" and
                all(row["status"] in {"PASS", "XFAIL"} for row in rows[batch_start:])):
            for row in rows[batch_start:]:
                row["status"] = "incomplete"
                row["details"] += "<br>GitLab job failed without a per-ID checker failure"
    counts = {status: 0 for status in ("PASS", "XFAIL", "XPASS", "FAIL", "incomplete")}
    for row in rows:
        counts[row["status"]] += 1
    unexpected = sum(counts[status] for status in ("XPASS", "FAIL", "incomplete"))
    pipeline = metadata.get("pipeline", {})
    pipeline_id = pipeline.get("id")
    pipeline_url = pipeline.get("web_url")
    heading = "# JLSE pipeline results"
    if pipeline_id is not None and pipeline_url:
        heading += " — [%s](%s)" % (pipeline_id, pipeline_url)
    elif pipeline_id is not None:
        heading += " — %s" % pipeline_id
    output = [heading, "",
              "Generated from existing `results/id-ID/summary.md` and "
              "`results/batch-status.tsv` artifacts; no tests were rerun.", "",
              "**Totals:** %d IDs — %d PASS, %d XFAIL, %d XPASS, %d FAIL, %d incomplete. "
              "**Unexpected or incomplete: %d.**" %
              (len(rows), counts["PASS"], counts["XFAIL"], counts["XPASS"],
               counts["FAIL"], counts["incomplete"], unexpected), "",
              "| Batch | ID | Configuration | Status | Expected failures / diagnostics | GitLab job | Cobalt submission |",
              "| --- | ---: | --- | --- | --- | --- | ---: |"]
    for row in rows:
        output.append("| %s | %02d | %s | %s | %s | %s | %s |" %
                      (row["batch"], row["id"], markdown(row["notes"]), row["status"],
                       row["details"], row["job"], row["cobalt"]))
    if job_notes:
        output += ["", "## Artifact collection diagnostics", ""]
        output += ["- %s" % markdown(note) for note in job_notes]
    output.append("")
    return "\n".join(output), unexpected + len(job_notes)


class GitLab:
    def __init__(self):
        self.base = os.environ.get("CI_API_V4_URL", "https://gitlab.jlse.anl.gov/api/v4")
        self.project = urllib.parse.quote(os.environ["CI_PROJECT_ID"], safe="")
        self.pipeline = os.environ["CI_PIPELINE_ID"]
        self.token = os.environ.get("CI_JOB_TOKEN") or os.environ.get("GITLAB_TOKEN")
        self.header = "JOB-TOKEN" if os.environ.get("CI_JOB_TOKEN") else "PRIVATE-TOKEN"

    def get(self, path):
        request = urllib.request.Request(self.base + path)
        if self.token:
            request.add_header(self.header, self.token)
        with urllib.request.urlopen(request, timeout=60) as response:
            return response.read()

    def json(self, path):
        return json.loads(self.get(path))


def collect(root):
    gitlab = GitLab()
    root.mkdir(exist_ok=True)
    metadata = {"pipeline": {
        "id": int(gitlab.pipeline),
        "web_url": os.environ.get("CI_PIPELINE_URL"),
        "sha": os.environ.get("CI_COMMIT_SHA"),
    }}
    try:
        jobs = gitlab.json("/projects/%s/pipelines/%s/jobs?per_page=100" %
                           (gitlab.project, gitlab.pipeline))
    except (OSError, ValueError, urllib.error.URLError) as error:
        jobs = []
        for batch in BATCHES:
            metadata[batch] = {"status": "unknown", "error": "pipeline jobs API: %s" % error}
    by_name = {job.get("name"): job for job in jobs}
    for batch in BATCHES:
        if batch in metadata:
            continue
        job = by_name.get(batch)
        if job is None:
            metadata[batch] = {"status": "missing", "error": "GitLab job is missing"}
            continue
        record = {key: job.get(key) for key in ("id", "status", "web_url")}
        metadata[batch] = record
        try:
            trace = gitlab.get("/projects/%s/jobs/%s/trace" % (gitlab.project, job["id"]))
            match = re.findall(rb"Submitted batch job ([0-9]+)", trace)
            if match:
                record["cobalt_id"] = match[-1].decode()
        except (OSError, ValueError, urllib.error.URLError):
            pass
        try:
            archive = gitlab.get("/projects/%s/jobs/%s/artifacts" % (gitlab.project, job["id"]))
            destination = root / batch
            destination.mkdir(exist_ok=True)
            with zipfile.ZipFile(io.BytesIO(archive)) as zipped:
                zipped.extractall(destination)
        except (OSError, ValueError, urllib.error.URLError, zipfile.BadZipFile) as error:
            record["error"] = str(error)
    (root / "jobs.json").write_text(json.dumps(metadata, indent=2) + "\n")
    return root, metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fetch", action="store_true", help="collect current-pipeline artifacts")
    parser.add_argument("--artifact-root", type=Path, default=Path(".jlse-artifacts"))
    parser.add_argument("--jobs-json", type=Path)
    parser.add_argument("--configs", type=Path, default=Path("test/configs.json"))
    parser.add_argument("--output", type=Path, default=Path("results/jlse-summary.md"))
    args = parser.parse_args()
    if args.fetch:
        root, metadata = collect(args.artifact_root)
    else:
        root = args.artifact_root
        jobs_path = args.jobs_json or root / "jobs.json"
        metadata = json.loads(jobs_path.read_text())
    configs = {row["id"]: row for row in json.loads(args.configs.read_text())}
    output, failures = report(metadata, root, configs)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
