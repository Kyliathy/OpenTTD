"""Restore genuine additional components stored in the original paid launch center.

@file surface_entry_component_delivery_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CG.4.
Only the independently continued original branch supplies later fuel/capital work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_replay import load, replay, validate

DRIVER = runpy.run_path(str(Path(__file__).with_name("surface_entry_component_delivery_checks-1.py")))["execute"]
STEM = "surface-entry-component-delivery"


def execute(configuration, phase, receipt):
    """@param configuration Actual focused native build. @param phase Explicit stored-component service.
    @param receipt Original native receipt or ordinary restore export. @return Native continuation from unchanged source bytes with actual restore observations attached where required.
    """
    value = load(receipt); selected = receipt
    if "final" not in value["operation"]:
        # Ordinary restoration exports native save/process identities rather
        # than a gameplay snapshot. Annotate only this explicit restore role
        # from its real saved owners; the original export and game stay exact.
        assert phase == "component-delivery-service" and value["phase"] == "restore" and value["configuration"] == configuration
        source = ROOT / value["operation"]["output"]["path"]; assert identity(source) == value["operation"]["output"]
        snapshot = observe(source); assert snapshot["components_at_center"]
        selected = ROOT / f"logs/{STEM}-ordinary-source-{configuration}.json"
        annotated = json.loads(json.dumps(value)); annotated["_documentation"] = __doc__
        annotated["ordinary_restore"] = identity(receipt); annotated["operation"]["final"] = json.loads(json.dumps(snapshot))
        if selected.exists():
            assert load(selected) == annotated
        else:
            selected.write_text(json.dumps(annotated, indent=2) + "\n", encoding="utf-8")
    result = DRIVER(configuration, phase, selected); result["sources"].append(identity(Path(__file__)))
    return result


def resume(configuration, predecessor):
    """@param configuration Actual partially completed replay build. @param predecessor Exact original stored-material receipt.
    @return Validated ordinary/original replay using already completed native results and only the missing restored continuation.
    """
    restore_path = ROOT / f"logs/{STEM}-restored-{configuration}.json"
    original_path = ROOT / f"logs/{STEM}-continued-{configuration}-original.json"
    ordinary_path = ROOT / f"logs/{STEM}-continued-{configuration}-ordinary.json"
    restore, original = load(restore_path), load(original_path)
    assert restore["operation"]["source"] == load(predecessor)["operation"]["output"]
    assert original["operation"]["receipt"] == identity(predecessor) and original["operation"]["source"] == restore["operation"]["source"]
    if not ordinary_path.exists():
        operation = execute(configuration, "component-delivery-service", restore_path)
        ordinary_path.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, phase="component-delivery-service", operation=operation), indent=2) + "\n", encoding="utf-8")
    ordinary = load(ordinary_path); assert ordinary["operation"]["source"] == restore["operation"]["output"]
    # Resume retains the first actual original output. The restored branch
    # independently earns its own work and must match every saved gameplay
    # family; no copied result supplies material or progress to the original.
    first, last = [chunks(ROOT / value["operation"]["output"]["path"])[1] for value in (original, ordinary)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, configuration=configuration, source_receipt=identity(predecessor), restore=identity(restore_path),
                  continuations=[identity(original_path), identity(ordinary_path)], output=original["operation"]["output"], company=original["operation"]["final"]["company"],
                  additional_continuation_ticks=3000, source=identity(Path(replay.__code__.co_filename)), recovery_source=identity(Path(__file__)))
    validate(result, dict(receipt=identity(predecessor)), observe)
    return result


def main():
    """Ordinarily restore one original world and compare matching actual3000tick continuations."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--output", type=Path, required=True); parser.add_argument("--resume", action="store_true"); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    result = resume(args.configuration, ROOT / args.receipt) if args.resume else replay(args.configuration, ROOT / args.receipt, STEM, "component-delivery-service", execute, observe)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
