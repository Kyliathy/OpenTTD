"""Verify ordinary full-application reload of genuine collected chemicals.

@file surface_entry_chemical_checks-12.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.3.
Actual native Industry/CAPA/vehicle/financial owners survive unchanged. Known
ordinary/focused executable revisions are individually qualified, never waived.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_chemical_ledger import TABLE, products, salt_ledger, chemical_ledger

ORDINARY = runpy.run_path(str(Path(__file__).with_name("surface_supply_delivery_checks-2.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def execute(configuration, receipt):
    """Restore the actual selected collected-CHEM checkpoint in its ordinary game.

    @param configuration Independently built complete game application.
    @param receipt Exact genuine original collected21CHEM receipt.
    @return Exact ordinary reload process and preserved original material/capital owners.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration and selected["phase"] == "collect"
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    assert chemical_ledger(source)["useful_custody"]
    output, process = ORDINARY["ordinary"](configuration, source, "surface-entry-chemical-custody")
    first, final = [products(path) for path in (source, output)]
    ingredients = [salt_ledger(path) for path in (source, output)]
    chemicals = [chemical_ledger(path) for path in (source, output)]
    assert first == final and ingredients[0] == ingredients[1] and chemicals[0] == chemicals[1]
    before, after = [chunks(path)[1] for path in (source, output)]
    assert before.keys() == after.keys()
    changed = {key for key in before if before[key] != after[key]}
    assert changed <= {b"GLOG", b"VIEW", b"DATE"}, changed
    if b"GLOG" in changed:
        REVISION["REVISION"]["verify_revision_log"](source, output)
    dates = [TABLE(path, b"DATE")[0] for path in (source, output)]
    # The original load-time routing worker owns its optional LinkGraph pause.
    # All actual clocks, random state and world identity remain unchanged.
    for name, value in dates[0].items():
        if name == "pause_mode":
            assert (dates[1][name] & ~64) == ((value & ~64) | 1)
        else:
            assert dates[1][name] == value, name
    return dict(receipt=identity(receipt), source=process["source"], output=process["output"], process=process,
                initial=first, final=final, salt=ingredients[1], chemicals=chemicals[1], earned_ticks=0,
                changed_native_families=sorted(key.decode() for key in changed), sources=[identity(Path(__file__)),
                identity(Path(__file__).with_name("surface_chemical_ledger.py"))])


def main():
    """Publish one stopped ordinary reload without any gameplay hook or stock change."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="restore",
                                     operation=execute(args.configuration, receipt)), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
