#!/usr/bin/env python3
"""
Coverage ratchet helper.

Policy:
  - Increase COVERAGE_MIN only when current measured line coverage exceeds
    the current minimum by a configurable headroom.
  - Avoid bumping into red CI by requiring headroom.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path


def sh(cmd: list[str]) -> str:
    return subprocess.check_output(cmd, text=True)


def read_makefile_coverage_min(makefile_path: Path) -> int:
    txt = makefile_path.read_text(encoding="utf-8")
    m = re.search(r"^COVERAGE_MIN\s*\?=\s*(\d+)\s*$", txt, flags=re.MULTILINE)
    if not m:
        raise RuntimeError("Could not find COVERAGE_MIN ?= N in Makefile")
    return int(m.group(1))


def parse_gcovr_line_rate(output: str) -> float:
    # gcovr summary includes: "lines: 25.7% (356 out of 1385)"
    m = re.search(r"^lines:\s*([0-9]+(?:\.[0-9]+)?)%\s*\(", output, flags=re.MULTILINE)
    if not m:
        raise RuntimeError("Could not parse gcovr line coverage from output")
    return float(m.group(1))


def bump_makefile(makefile_path: Path, new_min: int) -> None:
    txt = makefile_path.read_text(encoding="utf-8")
    txt2, n = re.subn(
        r"^COVERAGE_MIN\s*\?=\s*\d+\s*$",
        f"COVERAGE_MIN ?= {new_min}",
        txt,
        flags=re.MULTILINE,
    )
    if n != 1:
        raise RuntimeError(f"Expected to replace 1 COVERAGE_MIN line, replaced {n}")
    makefile_path.write_text(txt2, encoding="utf-8")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    makefile = repo / "Makefile"

    headroom = float(os.environ.get("RATCHET_HEADROOM", "2.0"))
    step = int(os.environ.get("RATCHET_STEP", "1"))

    current_min = read_makefile_coverage_min(makefile)
    # Run coverage with current threshold but do not fail-under; we want the number.
    out = sh(["make", "coverage", f"COVERAGE_MIN={current_min}"])
    measured = parse_gcovr_line_rate(out)

    target = current_min + step
    if measured + 1e-9 < (target + headroom):
        print(f"NO_BUMP: measured={measured:.1f} current_min={current_min} target={target} headroom={headroom}")
        return 0

    bump_makefile(makefile, target)
    print(f"BUMP: measured={measured:.1f} current_min={current_min} new_min={target} headroom={headroom}")
    return 0


if __name__ == "__main__":
    sys.exit(main())


