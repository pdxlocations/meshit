import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from release_tag import APIError, ensure_tag


COMMIT = 'abcdef0' + '1' * 33
NEW_HEAD = '1234567' + '2' * 33
TAG = 'v0.1.5.abcdef0-beta'


class Repository:
    def __init__(self):
        self.head = COMMIT
        self.tag = None
        self.calls = []
        self.read_error = None

    def api(self, method, path, data=None):
        self.calls.append((method, path, data))
        if method == 'GET':
            if self.read_error:
                raise APIError(self.read_error, 'read failed')
            if self.tag is None:
                raise APIError(404, 'Not Found')
            return {'object': {'type': 'commit', 'sha': self.tag}}
        if data['sha'] != self.head:
            raise APIError(403, 'Resource not accessible by integration')
        self.tag = data['sha']
        return {'object': {'type': 'commit', 'sha': self.tag}}


class ReleaseTagTests(unittest.TestCase):
    def test_push_during_build_does_not_change_reserved_release(self):
        repo = Repository()
        ensure_tag('owner/repo', TAG, COMMIT, create=True, api=repo.api)
        repo.head = NEW_HEAD  # A push while the firmware and Android jobs run.
        self.assertEqual(ensure_tag('owner/repo', TAG, COMMIT, api=repo.api), COMMIT)
        self.assertEqual(ensure_tag('owner/repo', TAG, COMMIT, create=True, api=repo.api), COMMIT)
        self.assertEqual(sum(method == 'POST' for method, _, _ in repo.calls), 1)

    def test_missing_tag_cannot_be_created_at_publish_time(self):
        repo = Repository()
        repo.head = NEW_HEAD
        with self.assertRaisesRegex(RuntimeError, 'not reserved'):
            ensure_tag('owner/repo', TAG, COMMIT, api=repo.api)
        self.assertTrue(all(method == 'GET' for method, _, _ in repo.calls))

    def test_conflicting_tag_is_never_moved(self):
        repo = Repository()
        repo.tag = NEW_HEAD
        with self.assertRaisesRegex(RuntimeError, 'refusing to move'):
            ensure_tag('owner/repo', TAG, COMMIT, create=True, api=repo.api)
        self.assertEqual(repo.tag, NEW_HEAD)
        self.assertEqual(len(repo.calls), 1)

    def test_read_permission_errors_are_not_missing_tags(self):
        repo = Repository()
        repo.read_error = 403
        with self.assertRaises(APIError):
            ensure_tag('owner/repo', TAG, COMMIT, create=True, api=repo.api)
        self.assertEqual(len(repo.calls), 1)

    def test_mismatched_tag_identity_is_rejected_before_network_access(self):
        repo = Repository()
        with self.assertRaises(ValueError):
            ensure_tag('owner/repo', TAG, NEW_HEAD, create=True, api=repo.api)
        self.assertFalse(repo.calls)


if __name__ == '__main__':
    unittest.main()
