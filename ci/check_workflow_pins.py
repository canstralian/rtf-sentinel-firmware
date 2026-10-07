#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

PINNED = re.compile(r"^\s*-?\s*uses:\s*([^\s@]+)@([0-9a-f]{40})(?:\s+#.*)?$")
USES = re.compile(r"^\s*-?\s*uses:\s*(\S+)")


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("usage: check_workflow_pins.py WORKFLOW...", file=sys.stderr)
        return 2

    failures: list[str] = []
    for raw in argv[1:]:
        path = Path(raw)
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            match = USES.match(line)
            if not match:
                continue
            target = match.group(1)
            if target.startswith("./"):
                continue
            if not PINNED.match(line):
                failures.append(
                    f"{path}:{line_number}: action is not pinned to a 40-character commit SHA: {target}"
                )

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1

    print("Workflow action pins passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
