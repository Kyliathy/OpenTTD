"""Run paid assembly lifecycle and exact continuation as one bounded burst.

@file launch_assembly_checks-1.py Native assembly lifecycle acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2B/8B.
The resulting report names the explicit interrupted save consumed by bursts2/3.
"""
from launch_assembly_checks import main

if __name__ == "__main__":
    main(part=1)
