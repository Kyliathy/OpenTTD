"""Run copied assembly previews against an explicit valid checkpoint as one bounded burst.

@file launch_assembly_checks-3.py Native assembly independent-preview refusal acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2B/8B.
Requires --source naming the interrupted save published by burst1.
"""
from launch_assembly_checks import main

if __name__ == "__main__":
    main(part=3)
