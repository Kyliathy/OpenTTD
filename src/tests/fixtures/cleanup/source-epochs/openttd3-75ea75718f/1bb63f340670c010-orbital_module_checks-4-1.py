"""Verify native assembly consumer, material and schema refusals in a separate bounded burst.

@file orbital_module_checks-4-1.py Second half of independent full-load and copied-preview rejection checks.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
"""
from pathlib import Path
import runpy


def main():
    """Verify the final five malformed-save cases while preserving the same immutable genuine partial checkpoint."""
    runpy.run_path(str(Path(__file__).with_name("orbital_module_checks-4.py")))["main"](second=True)


if __name__ == "__main__":
    main()
