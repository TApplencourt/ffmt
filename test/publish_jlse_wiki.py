#!/usr/bin/env python3
"""Publish an existing JLSE report artifact to the project's GitLab wiki."""

import argparse
import json
import os
from pathlib import Path
import sys
import urllib.error
import urllib.parse
import urllib.request


PAGE_TITLE = "JLSE Results"
PAGE_SLUG = "JLSE-Results"


def request(method, path, token, content=None):
    base = os.environ["CI_API_V4_URL"].rstrip("/")
    project = urllib.parse.quote(os.environ["CI_PROJECT_ID"], safe="")
    data = None
    if content is not None:
        data = urllib.parse.urlencode({
            "title": PAGE_TITLE,
            "content": content,
            "format": "markdown",
        }).encode()
    http_request = urllib.request.Request(
        "%s/projects/%s/wikis%s" % (base, project, path), data=data, method=method)
    http_request.add_header("PRIVATE-TOKEN", token)
    if data is not None:
        http_request.add_header("Content-Type", "application/x-www-form-urlencoded")
    with urllib.request.urlopen(http_request, timeout=60) as response:
        body = response.read()
    return json.loads(body) if body else {}


def publish(content):
    token = os.environ.get("FFMT_WIKI_TOKEN")
    if not token:
        raise RuntimeError(
            "FFMT_WIKI_TOKEN must name a masked project access token with the api scope")
    slug_path = "/" + urllib.parse.quote(PAGE_SLUG, safe="")
    try:
        request("GET", slug_path, token)
    except urllib.error.HTTPError as error:
        status = error.code
        error.close()
        if status != 404:
            raise
        result = request("POST", "", token, content)
    else:
        result = request("PUT", slug_path, token, content)
    if result.get("slug") not in (None, PAGE_SLUG):
        raise RuntimeError("GitLab returned an unexpected wiki slug")
    project_url = os.environ.get("CI_PROJECT_URL", "GitLab project")
    print("Published JLSE results to %s/-/wikis/%s" % (project_url, PAGE_SLUG))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, nargs="?", default=Path("results/jlse-summary.md"))
    args = parser.parse_args()
    try:
        publish(args.report.read_text())
    except (OSError, KeyError, RuntimeError, ValueError, urllib.error.URLError) as error:
        parser.exit(1, "JLSE wiki publication failed: %s\n" % error)


if __name__ == "__main__":
    main()
