#!/usr/bin/env python3
"""Reserve a release's exact commit before builds, or verify an existing tag."""
import argparse
import json
import os
import re
from urllib.error import HTTPError
from urllib.parse import quote
from urllib.request import Request, urlopen


class APIError(RuntimeError):
    def __init__(self, status, message):
        self.status = status
        super().__init__(f'GitHub HTTP {status}: {message}')


def github_api(method, path, data=None):
    token = os.environ['GH_TOKEN']
    request = Request('https://api.github.com/' + path, method=method,
                      data=json.dumps(data).encode() if data is not None else None,
                      headers={'Authorization': f'Bearer {token}',
                               'Accept': 'application/vnd.github+json',
                               'Content-Type': 'application/json',
                               'User-Agent': 'meshit-release',
                               'X-GitHub-Api-Version': '2022-11-28'})
    try:
        with urlopen(request, timeout=30) as response:
            return json.load(response)
    except HTTPError as error:
        try:
            message = json.load(error).get('message', error.reason)
        except (ValueError, AttributeError):
            message = error.reason
        raise APIError(error.code, message) from None


def ensure_tag(repo, tag, commit, create=False, api=github_api):
    if not re.fullmatch(r'[\w.-]+/[\w.-]+', repo):
        raise ValueError('Invalid repository')
    if not re.fullmatch(r'v\d+\.\d+\.\d+\.[0-9a-f]{7}-beta', tag):
        raise ValueError('Invalid generated release tag')
    if not re.fullmatch(r'[0-9a-f]{40}', commit) or tag.split('.')[-1] != commit[:7] + '-beta':
        raise ValueError('Release tag does not identify the selected commit')
    path = f'repos/{repo}/git/ref/tags/{quote(tag, safe="")}'
    try:
        reference = api('GET', path)
    except APIError as error:
        if error.status != 404:
            raise  # A permissions/server error must never be treated as a missing tag.
        if not create:
            raise RuntimeError('Release tag was not reserved before the build. Start a new release run.') from None
        try:
            api('POST', f'repos/{repo}/git/refs', {'ref': 'refs/tags/' + tag, 'sha': commit})
        except APIError as error:
            if error.status not in (409, 422):
                raise
            # Another run may have reserved the same tag. Verify it rather than overwriting it.
        reference = api('GET', path)
    obj = reference['object']
    if obj.get('type') != 'commit' or obj.get('sha') != commit:
        raise RuntimeError('Release tag points to different code; refusing to move it. Choose a new version.')
    return commit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--create', action='store_true', help='Reserve the tag before any build starts')
    args = parser.parse_args()
    try:
        commit = ensure_tag(os.environ['GH_REPO'], os.environ['RELEASE_TAG'],
                            os.environ['RELEASE_COMMIT'], create=args.create)
        print(f'Release tag verified at {commit}')
    except (KeyError, ValueError, RuntimeError, OSError) as error:
        parser.exit(1, f'{error}\n')
