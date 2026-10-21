#!/usr/bin/env python3
"""Export docs/wiki to an existing GitHub wiki checkout (does not commit/push)."""
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REPO = 'https://github.com/pdxlocations/meshit'


def export(destination):
    source = ROOT / 'docs/wiki'
    destination.mkdir(parents=True, exist_ok=True)
    for page in source.glob('*.md'):
        def rewrite(match):
            target = match.group(1)
            if target.startswith(('https://', 'http://', '#')):
                return match.group(0)
            path, separator, fragment = target.partition('#')
            resolved = (page.parent / path).resolve()
            if resolved.parent == source and resolved.suffix == '.md':
                url = resolved.stem
            else:
                relative = resolved.relative_to(ROOT).as_posix()
                url = f'{REPO}/' + ('raw/refs/heads/main/' if resolved.suffix == '.png' else 'blob/main/') + relative
            return '](' + url + (separator + fragment if separator else '') + ')'
        text = re.sub(r'\]\(([^)]+)\)', rewrite, page.read_text())
        (destination / page.name).write_text(text)
        print(page.name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    export(parser.parse_args().destination.resolve())
