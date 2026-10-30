#!/usr/bin/env python3
"""Collect tapsum.py rows into a Markdown pass-rate table.

    ci/mktable.py results/*.json > table.md

One row per (backend, opt). The pass rate is against the tests that ran:
a skip names a known compiler bug from the catalog in test/capture.hpp, so
a backend is not marked down for documenting one. Skips are their own
column and the reasons are listed below the table.

A row that is missing entirely -- a job that never produced JSON because
the build failed -- is not the same as a row with no passes. Pass the
expected cells with --expect to make an absent row show as "no result"
rather than silently vanishing.
"""
import argparse
import json
import sys


def load(paths):
    rows = []
    for path in paths:
        try:
            with open(path) as f:
                rows.append(json.load(f))
        except (OSError, ValueError) as exc:
            print(f"{path}: {exc}", file=sys.stderr)
    return rows


def cell_key(row):
    return (row.get("backend", "?"), row.get("opt", ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("results", nargs="+", help="tapsum.py JSON files")
    ap.add_argument("--expect", action="append", default=[], metavar="BACKEND:OPT",
                    help="a cell that should exist; reported as 'no result' "
                         "if absent. Repeatable.")
    ap.add_argument("--title", default="ffmt pass rate by backend")
    args = ap.parse_args()

    rows = load(args.results)
    by_cell = {cell_key(r): r for r in rows}

    for spec in args.expect:
        backend, _, opt = spec.partition(":")
        by_cell.setdefault((backend, opt), None)

    print(f"# {args.title}\n")

    commits = {r.get("commit") for r in rows if r.get("commit")}
    if len(commits) == 1:
        print(f"Commit `{commits.pop()[:12]}`.\n")
    elif len(commits) > 1:
        # Rows from different commits are not comparable; say so rather
        # than printing one table as if they were.
        print("**Rows come from different commits:** "
              + ", ".join(f"`{c[:12]}`" for c in sorted(commits)) + "\n")

    print("| backend | opt | device | built | pass | fail | skip | pass rate |")
    print("|---|---|---|---|---|---|---|---|")
    for (backend, opt) in sorted(by_cell):
        row = by_cell[(backend, opt)]
        if row is None:
            print(f"| {backend} | {opt} | — | — | — | — | — | **no result** |")
            continue

        # Built: how many targets linked, out of those make tried. A route
        # whose compiler crashes shows 0/N here and no tests at all, which
        # is a different thing from a route that built and then failed.
        attempted = row.get("attempted_builds") or 0
        unbuilt = len(row.get("unbuilt") or [])
        built_s = f"{attempted - unbuilt}/{attempted}" if attempted else "—"
        if unbuilt:
            built_s = f"**{built_s}**"

        rate = row.get("pass_rate")
        rate_s = "—" if rate is None else f"{rate * 100:.0f}%"
        if row.get("failed"):
            rate_s = f"**{rate_s}**"
        elif unbuilt and not row.get("total"):
            rate_s = "**build failed**"
        print(f"| {backend} | {opt} | {row.get('device') or '—'} | {built_s} "
              f"| {row.get('ok', 0)} | {row.get('failed', 0)} "
              f"| {row.get('skipped', 0)} | {rate_s} |")

    # Skips, with their reasons. These are the known-compiler-bug tags, so
    # the list doubles as "what is broken where", which is the part worth
    # reading when the rates are all 100%.
    skips = [(cell, r) for cell, r in sorted(by_cell.items())
             if r and r.get("skip_reasons")]
    if skips:
        print("\n## Skipped, by known compiler bug\n")
        print("| backend | opt | reason | count |")
        print("|---|---|---|---|")
        for (backend, opt), row in skips:
            for reason, count in sorted(row["skip_reasons"].items()):
                # A reason may contain '|', which would split the cell.
                print(f"| {backend} | {opt} | `{reason.replace('|', chr(92) + '|')}` | {count} |")

    builds = [(cell, r) for cell, r in sorted(by_cell.items())
              if r and r.get("unbuilt")]
    if builds:
        print("\n## Targets that would not build\n")
        for (backend, opt), row in builds:
            print(f"- **{backend} {opt}**: " + ", ".join(row["unbuilt"]))
            if row.get("build_error"):
                print(f"  - `{row['build_error']}`")

    fails = [(cell, r) for cell, r in sorted(by_cell.items())
             if r and r.get("failures")]
    if fails:
        print("\n## Failing test files\n")
        for (backend, opt), row in fails:
            print(f"- **{backend} {opt}**: " + ", ".join(row["failures"]))

    versions = sorted({(r.get("backend", "?"), r.get("compiler_version"))
                       for r in rows if r.get("compiler_version")})
    if versions:
        print("\n## Compilers\n")
        for backend, version in versions:
            print(f"- {backend}: {version}")


if __name__ == "__main__":
    main()
