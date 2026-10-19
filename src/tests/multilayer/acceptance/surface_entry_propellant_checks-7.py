"""Bind real crossing protection, external CHEM processing and physical PROP custody.

@file surface_entry_propellant_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.2.
The exact selected first-output chain alone supplies costs, movement and material.
Retained original producer products preserve earlier genuine waiting intervals.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]


def bind(specification):
    """@param specification Exact both-build native receipt membership and retained product aliases.
    @return Verified real traffic/material/finance lineage ending in20PROPdepot custody and resumed freight.
    """
    spec = json.loads(specification.read_text(encoding="utf-8"))
    foundation_path = ROOT / "logs/surface-entry-propellant-construction-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_empty_factory_ready"]
    aliases = {reference["sha256"]: ROOT / reference["path"] for reference in spec["retained"]}
    checked_ids, processes, physical, build_rows, summaries = {}, set(), [], [], []

    def checked(reference):
        """@param reference Exact immediate native artifact. @return Original or explicitly retained repository path."""
        path = ROOT / reference["path"]; key = reference["path"], reference["sha256"]
        if key in checked_ids:
            return checked_ids[key]
        if key not in checked_ids:
            if identity(path) != reference:
                path = aliases[reference["sha256"]]
                assert identity(path)["sha256"] == reference["sha256"], reference
            checked_ids[key] = path
        return path

    def walk(value):
        """@param value Immediate selected receipt/process tree. Verify exact original membership without older construction replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        previous_receipt, = [row["receipts"][-1] for row in foundation["configurations"] if row["configuration"] == configuration]
        snapshots, rows, phase_names, work, invoice, income = [], [], [], 0, 0, 0
        for name in entry["receipts"]:
            path = ROOT / name; value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            assert value["complete"] and value["configuration"] == configuration
            operation = value["operation"]
            assert operation["source"] == previous and operation["receipt"] == previous_receipt
            source, before, output = [checked(operation[key]) for key in ("source", "before", "output")]
            old, first, last = [chunks(item)[1] for item in (source, before, output)]
            assert old.keys() == first.keys() == last.keys()
            assert all(old[key] == first[key] for key in old if key != b"GLOG")
            if old[b"GLOG"] != first[b"GLOG"]:
                REVISION(source, before)
            assert first[b"GLOG"] == last[b"GLOG"]
            initial, final = operation["initial"], operation["final"]
            assert initial["reserves"] == final["reserves"] == dict(steel=36, goods=14)
            assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
            assert initial["plant"]["industry"] == final["plant"]["industry"] != 15
            if value["phase"] in ("chemical-order", "chemical-work", "process", "carrier", "collect-order", "collect-work"):
                # A physical depot arrival, rather than a quoted temporary
                # order, protects each loaded road trip from live railway traffic.
                assert initial["traffic_halted"] and final["traffic_halted"]
            for earned in operation["rows"]["earned"]:
                work += earned["earned"]; income += earned["income"]
            invoice += sum(row["cost"] for row in operation["rows"]["invoices"])
            for reference in operation["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            snapshots.extend((first, last)); rows.append(operation["rows"]); phase_names.append(value["phase"])
            previous = operation["output"]; previous_receipt = identity(path)
        assert phase_names[0] == "traffic-order" and phase_names[-1] == "resume"
        assert all(phase in phase_names for phase in ("traffic-work", "chemical-order", "chemical-work", "carrier", "collect-order", "collect-work"))
        assert phase_names.count("carrier") == phase_names.count("resume") == 1
        assert final["plant"]["chemicals"] == 1 and final["plant"]["consumed"] == final["plant"]["produced"] == final["plant"]["dispatched"] == 20
        assert final["plant"]["buffered"] == 0 and final["plant"]["batches"] == 10
        assert final["chemical_feed"]["held"] == 0 and final["chemical_feed"]["depot"]
        assert final["propellant_truck"]["held"] == 20 and final["propellant_truck"]["depot"] and final["propellant_truck"]["stopped"]
        assert not final["traffic_halted"] and len(operation["rows"]["resumed"]) == 3 and invoice == 6234
        physical.append(snapshots); build_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=[identity(ROOT / name) for name in entry["receipts"]], output=previous,
                              company=final["company"], actual_earned_ticks=work, actual_income=income, actual_carrier_invoice=invoice,
                              plant=final["plant"], collected=final["propellant_truck"], original_chemical=final["original_chemical"], mine=final["mine"]))
    assert len(summaries) == 2 and build_rows[0] == build_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), foundation=identity(foundation_path),
                configurations=summaries, native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                real_external_chemicals_delivered=21, real_chemicals_consumed=20, residual_chemicals=1, useful_propellant_collected=20,
                ordinary_reload_complete=False, actual_launch_complete=False, source=identity(Path(__file__)))


def main():
    """Retain genuine useful fuel custody before ordinary reload/continuation and launch acceptance."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    specification, output = [(ROOT / path).resolve() for path in (args.specification, args.output)]
    assert specification.is_relative_to(ROOT / "logs") and output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(specification), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
