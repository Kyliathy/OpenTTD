"""Run the second bounded activated-module refusal burst.

@file orbital_activation_checks-4.py Stock and unsupported work-consumer loader/preview boundaries.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4.
"""
from pathlib import Path
import runpy


if __name__ == "__main__":
    runpy.run_path(str(Path(__file__).with_name("orbital_activation_checks-3.py")))["main"](second=True)
