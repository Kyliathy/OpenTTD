"""Seal both actual paid ordinary freight construction predecessors.

@file normal_ground_freight_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Independent original save IDs retain the exact native6B exception;no other byte is excluded.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from construction_provider_return import identity
from normal_ground_start_report import native_world
from normal_ground_readiness import START, START_SHA
from save_tables import saved_table


def accept(report_paths, output):
    """Verify original lineage,invoices,pure saves and all cross-build bytes before6CB.1 closes."""
    handoff = ROOT / START
    assert identity(handoff)["sha256"] == START_SHA
    original = json.loads(handoff.read_text(encoding="utf-8"))
    records, worlds, dates = {}, {}, {}
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        assert report["classification"] == "normal_start_paid_freight"
        assert report["bootstrap_claim"] is False and report["children_reaped"] is True
        assert report["original_input"] == original["records"][cfg]["starting_input"]
        assert report["handoff"] == identity(handoff)
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], report["original_input"], *report["sources"]):
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        ready = verify_reference(report["saves"]["ready"])
        repeat = verify_reference(report["saves"]["repeat"])
        worlds[cfg] = native_world(ready)
        assert worlds[cfg] == native_world(repeat)
        dates[cfg] = saved_table(ready, b"DATE")[0]
        assert dates[cfg]["id"] == saved_table(verify_reference(report["original_input"]), b"DATE")[0]["id"]
        player = saved_table(ready, b"PLYR")[0]
        assert player["money"] == report["ready"]["cash"] == 267959
        assert player["current_loan"] == report["ready"]["loan"] == 300000
        assert sum(player["yearly_expenses"]) == report["construction_expense"] == 32041
        running_cash = 300000
        for invoice in report["invoices"]:
            running_cash -= invoice["cost"]
            assert invoice["cash"] == running_cash
        assert running_cash == report["ready"]["cash"]
        records[cfg] = {"report": identity(path), "actual": report}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    assert records["Debug"]["actual"]["proposal"] == records["RelWithDebInfo"]["actual"]["proposal"]
    assert records["Debug"]["actual"]["invoices"] == records["RelWithDebInfo"]["actual"]["invoices"]
    assert records["Debug"]["actual"]["ready"] == records["RelWithDebInfo"]["actual"]["ready"]
    assert [key for key in worlds["Debug"] if worlds["Debug"][key] != worlds["RelWithDebInfo"][key]] == ["DATE"]
    assert {key: value for key, value in dates["Debug"].items() if key != "id"} == {
        key: value for key, value in dates["RelWithDebInfo"].items() if key != "id"}
    debug_id, optimized_id = (bytes.fromhex(dates[cfg]["id"]) for cfg in ("Debug", "RelWithDebInfo"))
    assert debug_id != optimized_id and len(debug_id) == len(optimized_id) == 32
    assert worlds["Debug"]["DATE"].count(debug_id) == worlds["RelWithDebInfo"]["DATE"].count(optimized_id) == 1
    assert worlds["Debug"]["DATE"].replace(debug_id, optimized_id, 1) == worlds["RelWithDebInfo"]["DATE"]
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CB.1;actual ordinary paid freight from exact native normal-game originals",
              "controller": identity(__file__), "starting_handoff": identity(handoff), "records": records,
              "classification": "normal_start_paid_freight", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 125, "other_date_fields_equal": len(dates["Debug"]) - 1,
              "ignored_chunks": [], "native_newgame_ids": {cfg: dates[cfg]["id"] for cfg in dates},
              "remaining": "6CB.2 enough real freight capital;6CC paid site/yard;6CD participants/access;6D original S1/S2 acceptance."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish one immutable two-build freight construction handoff."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
