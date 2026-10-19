"""Retain paid road-freight renewal and bind both ordinary exact continuations.

@file surface_entry_road_renewal_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEL.3.
Native output bytes and immutable retained build products establish this handoff.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import TABLE


def execute():
    """@return Both actual ordinary restorations, exact replay and retained focused products."""
    foundation_path = ROOT / "logs/surface-entry-road-renewal-custody-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_road_renewal_complete"]
    worlds, rows, summaries, processes = [], [], [], set()
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/surface-entry-road-renewal-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]
        assert restore["source"] == entry["output"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        # A real restored due-worker job may change only the qualified pause
        # bit. The prior focused owner check preserves all other DATE fields.
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]
        stages, comparable = [normalized], []
        processes.update(reference["path"] for reference in restore["native_processes"])
        for reference in value["continuations"]:
            receipt = ROOT / reference["path"]; assert identity(receipt) == reference
            continuation = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / continuation[field]["path"]) == continuation[field]
            stages.append(chunks(ROOT / continuation["output"]["path"])[1]); comparable.append(continuation["rows"])
            processes.update(reference["path"] for reference in continuation["native_processes"])
        assert comparable[0] == comparable[1]
        assert stages[1].keys() == stages[2].keys() and all(stages[1][key] == stages[2][key] for key in stages[1] if key != b"GLOG")
        assert {"IORE", "STEL"} <= {row["label"] for row in value["rows"]["delivered"] if row["amount"] > 0}
        # Each active native vehicle must still have a full safe one-year
        # interval; further long savings cannot silently run obsolete service.
        vehicles = TABLE(ROOT / value["output"]["path"], b"VEHS")
        lives = []
        for key, vehicle in vehicles.items():
            if vehicle["type"] not in (0, 1):
                continue
            prefix = "train[0].common[0]." if vehicle["type"] == 0 else "roadveh[0].common[0]."
            name = bytes.fromhex(vehicle[prefix + "name"]).decode("utf-8")
            active = name.startswith("Surface Income ") or name.startswith("Surface Launch passenger ") or name in ("Surface ore delivery", "Surface Steel delivery")
            if not active:
                continue
            remaining = vehicle[prefix + "max_age"] - vehicle[prefix + "age"]; assert remaining > 366
            lives.append(dict(vehicle=key, name=name, remaining=remaining))
        assert len(lives) == 8
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=value["company"],
                              road_services=value["road_services"], service_lives=lives, delivered=value["rows"]["delivered"], reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    assert len(processes) == 6
    retained = ROOT / "test-cache/surface-entry/road-renewal-products"; assert not retained.exists(); retained.mkdir(parents=True)
    products = [ROOT / "src/tests/surface_entry_road_renewal.cpp", ROOT / "src/tests/surface_entry_work.h", ROOT / "CMakeLists.txt", ROOT / "src/tests/CMakeLists.txt"]
    products += [ROOT / "src/tests/multilayer/acceptance" / name for name in ("surface_entry_road_renewal_checks-1.py", "surface_entry_road_renewal_checks-2.py", "surface_entry_road_renewal_checks-3.py", "surface_fuel_refill_ledger.py", "surface_propellant_ledger.py")]
    products += [ROOT / "build" / configuration / "openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")]
    copies = []
    # Preserve the exact tested products before a later group recompiles this
    # small target; original receipts remain independently reviewable.
    for index, source in enumerate(products):
        target = retained / f"{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]
        copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"; manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_road_renewal_complete=True, actual_launch_complete=False,
                foundation=identity(foundation_path), native_processes=6, cross_build_gameplay_comparisons=360,
                exact_native_replay_gameplay_comparisons=240, configurations=summaries, retained=identity(manifest), source=identity(Path(__file__)))


def main():
    """Publish one immutable original useful road-service handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
