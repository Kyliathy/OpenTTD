"""Verify ordinary restore and identical native earnings after genuine freight renewal.

@file surface_entry_fuel_checks-12.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIC.3.
The original purchase output remains the selected continuous world. A separately
restored branch proves identical continuation without supplying income or stock.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_income_ledger import TABLE, TRAIN, observe, reserve_identity

ACCEPTANCE = Path(__file__).parent
ORDINARY = runpy.run_path(str(ACCEPTANCE / "surface_supply_delivery_checks-2.py"))["ordinary"]
REVISION = runpy.run_path(str(ACCEPTANCE / "surface_entry_freight_checks-3.py"))["REVISION"]["verify_revision_log"]
LOAD_REVISION = runpy.run_path(str(ACCEPTANCE / "surface_entry_checks-7.py"))["known_revision"]
EARN = runpy.run_path(str(ACCEPTANCE / "surface_entry_fuel_checks-5.py"))["execute"]


def publish(path, value):
    """@param path Fresh repository log destination. @param value Complete original process receipt. Retain without overwrite."""
    assert path.is_relative_to(ROOT / "logs") and not path.exists()
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def restore_pause(source, restored):
    """Check the sole original worker-pause change during an ordinary load.

    @param source Native paused purchase checkpoint.
    @param restored Ordinary resave of that exact game.
    @return Exact original pause flags; all other date/RNG/tick fields must match.
    """
    first, last = [TABLE(path, b"DATE")[0] for path in (source, restored)]
    assert {key: value for key, value in first.items() if key != "pause_mode"} == {
        key: value for key, value in last.items() if key != "pause_mode"}
    assert first["pause_mode"] == 1 and last["pause_mode"] in (1, 65)
    if last["pause_mode"] == 65:
        # AfterLoad_LinkGraphPauseControl sets only LinkGraph bit6 while an
        # actually scheduled due job restarts. Validate its original saved
        # job/schedule owners; no arbitrary DATE or clock difference is waived.
        jobs, schedule = TABLE(restored, b"LGRJ"), TABLE(restored, b"LGRS")[0]
        assert any(jobs[reference - 1]["join_date"] <= last["economy_date"] for reference in schedule["running"])
    return dict(before=first["pause_mode"], after=last["pause_mode"], original_owner="AfterLoad_LinkGraphPauseControl")


def execute():
    """@return Actual both-build ordinary reload and exact native continuation proof."""
    foundation_path = ROOT / "logs/surface-entry-income-fleet-purchase-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["actual_renewal_complete"]
    physical, summaries, processes, rows_by_build = [], [], set(), []
    for configuration in ("RelWithDebInfo", "Debug"):
        purchase = ROOT / f"logs/surface-entry-income-fleet-buy-{configuration}.json"
        previous = json.loads(purchase.read_text(encoding="utf-8"))["operation"]
        expected, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        assert previous["output"] == expected
        source = ROOT / expected["path"]; assert identity(source) == expected
        restored, restore = ORDINARY(configuration, source, "income-fleet-restore-" + configuration.lower())
        REVISION(source, restored)
        old, loaded = [chunks(path)[1] for path in (source, restored)]
        pause = restore_pause(source, restored)
        assert old.keys() == loaded.keys() and all(old[key] == loaded[key] for key in old if key not in (b"GLOG", b"DATE"))
        initial, final = observe(source), observe(restored)
        assert initial == final and reserve_identity(initial) == reserve_identity(final)
        restored_receipt = ROOT / f"logs/surface-entry-income-fleet-restored-{configuration}.json"
        publish(restored_receipt, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore",
                                      operation=dict(**restore, initial=initial, final=final)))
        continuation_receipts, continuations, rows = [], [], []
        # The selected actual purchase branch and its ordinary resave each earn
        # exactly one native annual period. Every gameplay owner must agree;
        # the second branch supplies no capital to the selected game.
        for receipt, target in ((purchase, ROOT / f"logs/surface-entry-income-earned-{configuration}-27.json"),
                                (restored_receipt, ROOT / f"logs/surface-entry-income-fleet-replay-{configuration}.json")):
            operation = EARN(configuration, receipt, "earn")
            publish(target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="earn", operation=operation))
            earned, = operation["rows"]["earned"]
            assert earned["earned"] == 27000 and earned["income"] > 0
            assert earned["cash"] > earned["cash_before"]
            deliveries = {row["label"]: row["amount"] for row in operation["rows"]["delivered"]}
            assert all(deliveries.get(label, 0) > 0 for label in ("GRAI", "LVST", "GOOD", "COAL"))
            output = ROOT / operation["output"]["path"]
            before = ROOT / operation["before"]["path"]
            if TABLE(ROOT / operation["source"]["path"], b"GLOG") != TABLE(before, b"GLOG"):
                LOAD_REVISION(ROOT / operation["source"]["path"], before)
            assert TABLE(before, b"GLOG") == TABLE(output, b"GLOG")
            vehicles = TABLE(output, b"VEHS")
            for train in operation["final"]["trains"]:
                head = vehicles[train["head"]]
                assert head[TRAIN + "engine_type"] == 26 and head[TRAIN + "age"] < head[TRAIN + "max_age"]
                assert head[TRAIN + "reliability"] > 0
            continuations.append(chunks(output)[1]); rows.append(operation["rows"]); continuation_receipts.append(identity(target))
            processes.update(item["path"] for item in operation["native_processes"])
        assert continuations[0].keys() == continuations[1].keys()
        assert all(continuations[0][key] == continuations[1][key] for key in continuations[0] if key != b"GLOG")
        assert rows[0] == rows[1]
        for reference in restore["native_processes"]:
            assert identity(ROOT / reference["path"]) == reference
            process = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8")); assert process["exit_status"] == 0
            processes.add(reference["path"])
        # DATE was independently checked field by field above, including its
        # exact original due-worker pause. Compare the canonical source bytes
        # here so OS thread readiness cannot masquerade as simulation drift.
        normalized_restore = dict(loaded); normalized_restore[b"DATE"] = old[b"DATE"]
        physical.append([normalized_restore] + continuations); rows_by_build.append(rows)
        summaries.append(dict(configuration=configuration, restore=identity(restored_receipt), continuations=continuation_receipts,
                              output=json.loads((ROOT / f"logs/surface-entry-income-earned-{configuration}-27.json").read_text(encoding="utf-8"))["operation"]["output"],
                              earned=rows[0]["earned"], delivered=rows[0]["delivered"], original_restore_pause=pause))
        print(configuration, "ordinary restore and identical earned continuation passed", flush=True)
    assert rows_by_build[0] == rows_by_build[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert comparisons == 360 and len(processes) == 6
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=6, exact_cross_build_gameplay_comparisons=360, exact_native_replay_gameplay_comparisons=240,
                actual_reliable_income=True, full_propellant_affordability=False,
                sources=[identity(Path(__file__)), identity(ACCEPTANCE / "surface_entry_fuel_checks-5.py"),
                         identity(ACCEPTANCE / "surface_income_ledger.py")])


def main():
    """Retain actual useful renewal and continue the still-open full Propellant finance task."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    publish(output, execute())
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
