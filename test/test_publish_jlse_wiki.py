import io
import json
import os
import unittest
from unittest import mock
import urllib.error
import urllib.parse

import publish_jlse_wiki


class PublishJlseWikiTest(unittest.TestCase):
    def setUp(self):
        self.environment = mock.patch.dict(os.environ, {
            "CI_API_V4_URL": "https://gitlab.example/api/v4",
            "CI_PROJECT_ID": "230",
            "CI_PROJECT_URL": "https://gitlab.example/bertoni/ffmt",
            "FFMT_WIKI_TOKEN": "test-token",
        })
        self.environment.start()

    def tearDown(self):
        self.environment.stop()

    def response(self):
        response = mock.MagicMock()
        response.__enter__.return_value = response
        response.read.return_value = json.dumps(
            {"slug": publish_jlse_wiki.PAGE_SLUG}).encode()
        return response

    def request_fields(self, calls, index):
        request = calls[index][0]
        body = urllib.parse.parse_qs(request.data.decode()) if request.data else {}
        return request, body

    def test_creates_missing_page(self):
        calls = []
        def respond(request, timeout):
            self.assertEqual(timeout, 60)
            calls.append((request,))
            if request.get_method() == "GET":
                raise urllib.error.HTTPError(request.full_url, 404, "missing", {}, io.BytesIO())
            return self.response()
        with mock.patch.object(publish_jlse_wiki.urllib.request, "urlopen", side_effect=respond):
            publish_jlse_wiki.publish("report contents\n")
        self.assertEqual(len(calls), 2)
        request, body = self.request_fields(calls, 1)
        self.assertEqual(request.get_method(), "POST")
        self.assertTrue(request.full_url.endswith("/projects/230/wikis"))
        self.assertEqual(request.get_header("Private-token"), "test-token")
        self.assertEqual(body["title"], ["JLSE Results"])
        self.assertEqual(body["content"], ["report contents\n"])
        self.assertEqual(body["format"], ["markdown"])

    def test_updates_existing_page(self):
        calls = []
        def respond(request, timeout):
            self.assertEqual(timeout, 60)
            calls.append((request,))
            return self.response()
        with mock.patch.object(publish_jlse_wiki.urllib.request, "urlopen", side_effect=respond):
            publish_jlse_wiki.publish("new report\n")
        self.assertEqual(len(calls), 2)
        request, body = self.request_fields(calls, 1)
        self.assertEqual(request.get_method(), "PUT")
        self.assertTrue(request.full_url.endswith("/projects/230/wikis/JLSE-Results"))
        self.assertEqual(body["content"], ["new report\n"])

    def test_reports_non_missing_errors(self):
        error = urllib.error.HTTPError("https://gitlab.example", 403, "denied", {}, io.BytesIO())
        with mock.patch.object(publish_jlse_wiki.urllib.request, "urlopen", side_effect=error):
            with self.assertRaises(urllib.error.HTTPError):
                publish_jlse_wiki.publish("report")


if __name__ == "__main__":
    unittest.main()
