"""Run the second bounded shared approach refusal burst.

@file space_approach_checks-3.py Future priority and invalid staging loader/preview boundaries.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
"""
from pathlib import Path
import runpy


if __name__ == "__main__":
    runpy.run_path(str(Path(__file__).with_name("space_approach_checks-2.py")))["main"](second=True)
