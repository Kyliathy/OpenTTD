"""Separate focused placement checks from facility source/palette regeneration.

@file multilayer_facility_art-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 23/26/37/38
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md focused-test protocol
Large receivers reuse cached fixed-cell reductions, keeping each burst short.
"""

import argparse

import multilayer_facility_art as fixture


def main():
    """Check one selected family's physical contacts, entries and fixed pixels.

    @return None; fail on any geometry drift or invalid near/far door mask.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", required=True)
    args = parser.parse_args()
    count = fixture.verify_family(args.family)
    print(f"Facility placement fixtures: {args.family}, {count} poses; fixed contacts/housing and true entry masks pass")


if __name__ == "__main__":
    main()
