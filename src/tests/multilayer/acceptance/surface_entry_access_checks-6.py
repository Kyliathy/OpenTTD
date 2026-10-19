"""Verify ordinary application reload of genuine paid-and-worked Underground access.

@file surface_entry_access_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.3.
The ordinary executable must preserve every native work, spoil, floor, packet
and financial owner. Qualified revision/view and native routing pause are explicit.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

DRIVE = runpy.run_path(str(Path(__file__).with_name("surface_entry_access_checks-5.py")))
ORDINARY = runpy.run_path(str(Path(__file__).with_name("surface_supply_delivery_checks-2.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def execute(configuration, source):
    """Reload the actual finished-work file in the complete ordinary application.

    @param configuration Independently built ordinary application.
    @param source Actual completed cut/exchange/disposal checkpoint.
    @return Original process membership and exact conserved native owners.
    """
    first = DRIVE["observe"](source)
    assert first["ready"]
    output, process = ORDINARY["ordinary"](configuration, source, "surface-entry-salt-access")
    final = DRIVE["observe"](output)
    products = [DRIVE["OWNERS"]["observe"](path) for path in (source, output)]
    assert first == final and products[0] == products[1]
    before, after = chunks(source)[1], chunks(output)[1]
    assert before.keys() == after.keys()
    changed = {key for key in before if before[key] != after[key]}
    assert changed <= {b"GLOG", b"VIEW", b"DATE"}, changed
    if b"GLOG" in changed:
        REVISION["REVISION"]["verify_revision_log"](source, output)
    dates = [DRIVE["OWNERS"]["saved_table"](path, b"DATE")[0] for path in (source, output)]
    # The player's normal pause is set, while the actual load-time routing
    # worker may retain its native LinkGraph pause until simulation joins it.
    # Every other clock/RNG/identity field remains exact at this stopped boundary.
    for name, value in dates[0].items():
        if name == "pause_mode":
            assert dates[1][name] & ~64 == value & ~64 | 1
        else:
            assert dates[1][name] == value, name
    return dict(process=process, initial=first, final=final, products=products[1], earned_ticks=0,
                changed_native_families=sorted(key.decode() for key in changed))


def main():
    """Retain one actual ordinary reload receipt without replaying any closed producer."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=execute(args.configuration, source), source=identity(Path(__file__))), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
