"""Verify the second half of native assembly continuation and physical cancellation.

@file orbital_module_checks-3-1.py Bounded Debug continuation after work/partial verification.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
"""
from pathlib import Path
import runpy


def main():
    """Join the preceding work/partial report with power/last replay and actual cancellation in a separate focused burst."""
    runpy.run_path(str(Path(__file__).with_name("orbital_module_checks-3.py")))["main"](complete=True)


if __name__ == "__main__":
    main()
