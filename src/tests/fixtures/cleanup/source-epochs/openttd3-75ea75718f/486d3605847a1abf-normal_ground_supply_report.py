"""Seal real native Surface supply and paid tube readiness on both normal originals.

@file normal_ground_supply_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Whole original paid floors/jobs/spoil remain accounted;useful operation is still6DCDD.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from construction_provider_return import identity
from normal_ground_supply import LAYOUT, LAYOUT_SHA, fields, observations, invoices_from
from normal_ground_start_report import native_world
from save_tables import saved_table

TRAIN = "train[0].common[0]."


def accept(report_paths, output):
    """Require actual depot travel,paid native participants/utility,real orders and every original spoil owner."""
    handoff = ROOT / LAYOUT
    assert identity(handoff)["sha256"] == LAYOUT_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    interval_source = ROOT / "src/spoil_receiver.h"
    interval = int(re.search(r"SPOIL_RECEIVER_INTERVAL_TICKS\s*=\s*(\d+)", interval_source.read_text(encoding="utf-8"))[1])
    records = {}
    owners = ("GEOS", "GDEP", "GSPN", "GVNT", "EXJS", "XJOB", "XJRT", "XJTL", "XPAS", "PATS", "NGRF")
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        assert report["classification"] == "normal_start_useful_supply" and report["bootstrap_claim"] is False
        assert report["layout_handoff"] == identity(handoff) and report["actual_input"] == flow["layout_input"]
        assert report["original_input"] == flow["actual"]["original_input"]
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        boundary = verify_reference(report["binding_boundary"]).read_bytes()
        assert "All tests passed (76 assertions in 1 test case)" in boundary.decode("utf-16" if boundary.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0 and int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        ready, repeat = (verify_reference(report["saves"][key]) for key in ("ready", "repeat"))
        before, after = native_world(source), native_world(ready)
        assert after == native_world(repeat)
        assert all(after[key] == before[key] for key in owners)
        measured, quote, machinery = report["ready"], report["quote"], report["machinery"]
        for key, marker in (("ready", "READY"), ("quote", "QUOTE"), ("machinery", "MACHINERY")):
            assert report[key] == fields(text, "NORMAL_GROUND_SUPPLY_" + marker)
        for key in ("parked", "stored", "ledger"):
            assert report[key] == observations(text, "NORMAL_GROUND_SUPPLY_" + key.upper())
        assert report["invoices"] == invoices_from(text)
        assert measured["opening_cash"] == flow["layout"]["cash"] and measured["first_tick"] == flow["layout"]["tick"]
        ticks = measured["tick"] - measured["first_tick"]
        assert 0 < ticks <= 8192 and measured["complete"] == 1 and measured["loan"] == 300000
        assert measured["carrier"] == machinery["carrier"] and measured["capsule"] == machinery["capsule"]
        assert len(report["ledger"]) == 13 and sorted(row["category"] for row in report["ledger"]) == list(range(13))
        assert measured["cash"] == measured["opening_cash"] - sum(row["cost"] for row in report["ledger"]) > 0
        assert report["capital_paid"] == sum(row["cost"] for row in report["invoices"]) == quote["tracks"] + quote["capsule"] + quote["wagon"] + quote["lease"]
        assert measured["cash"] == quote["cash"] - report["capital_paid"]
        assert report["parking_cash_delta"] == quote["cash"] - measured["opening_cash"]
        cash, capital_started = quote["cash"], False
        for invoice in report["invoices"]:
            capital_started |= invoice["action"] == "modal_surface_connection"
            if capital_started:
                cash -= invoice["cost"]
                assert cash == invoice["cash"]
            else:
                assert invoice["cost"] == 0
        assert cash == measured["cash"]
        assert sum(row["action"] == "modal_external_lease" for row in report["invoices"]) == 1
        assert sum(row["action"] == "modal_vehicle" for row in report["invoices"]) == 2
        assert {row["vehicle"]: row["tile"] for row in report["parked"]} == {2: 5149, 33: 7391}
        assert all(row["stored"] == 1 and row["visited"] > 1 for row in report["parked"])
        old_fleet, new_fleet = saved_table(source, b"VEHS"), saved_table(ready, b"VEHS")
        old_trains = {key: row for key, row in old_fleet.items() if TRAIN + "engine_type" in row}
        new_trains = {key: row for key, row in new_fleet.items() if TRAIN + "engine_type" in row}
        assert set(new_trains) - set(old_trains) == {machinery["wagon"], machinery["capsule"]}
        assert set(old_trains) <= set(new_trains)
        for vehicle, old in old_trains.items():
            assert all(new_trains[vehicle][TRAIN + field] == old[TRAIN + field] for field in ("owner", "engine_type", "cargo_type", "cargo_cap"))
        spoil = {key: row for key, row in old_trains.items() if row[TRAIN + "engine_type"] in (258, 259)}
        assert len(spoil) == 20
        assert all(not new_trains[key][TRAIN + "cargo.packets"] and new_trains[key][TRAIN + "cargo.action_counts"] == [0] * 4 for key in spoil)
        old_chain, cursor = [], measured["carrier"]
        while old_fleet[cursor][TRAIN + "next"]:
            cursor = old_fleet[cursor][TRAIN + "next"] - 1
            old_chain.append(cursor)
        assert old_chain == [row["vehicle"] for row in report["stored"]] and len(old_chain) == 6
        assert all(row["tile"] == 7391 and row["count"] == 0 and row["front"] != measured["carrier"] for row in report["stored"])
        assert all(new_trains[key][TRAIN + "tile"] == 7391 for key in old_chain)
        carrier, wagon, capsule = (new_trains[machinery[key]] for key in ("carrier", "wagon", "capsule"))
        assert carrier[TRAIN + "engine_type"] == 260 and wagon[TRAIN + "engine_type"] == 261
        assert carrier[TRAIN + "next"] == machinery["wagon"] + 1 and wagon[TRAIN + "next"] == capsule[TRAIN + "next"] == 0
        assert wagon[TRAIN + "cargo_type"] == capsule[TRAIN + "cargo_type"] == machinery["cargo"] == 1
        assert wagon[TRAIN + "cargo_cap"] == machinery["capacity"] == 64 and capsule[TRAIN + "cargo_cap"] == 60
        assert carrier[TRAIN + "tile"] == wagon[TRAIN + "tile"] == 7391 and capsule[TRAIN + "tile"] == flow["layout"]["tube_home"]
        assert carrier[TRAIN + "vehstatus"] & 2 and capsule[TRAIN + "vehstatus"] & 2
        assert not wagon[TRAIN + "cargo.packets"] and not capsule[TRAIN + "cargo.packets"]
        orders, depots = saved_table(ready, b"ORDL"), saved_table(ready, b"DEPT")
        freight_orders, tube_orders = orders[carrier[TRAIN + "orders"] - 1], orders[capsule[TRAIN + "orders"] - 1]
        assert freight_orders["orders.count"] == machinery["orders"] == 6 and tube_orders["orders.count"] == machinery["tube_orders"] == 4
        assert [(freight_orders[f"orders[{i}].type"], freight_orders[f"orders[{i}].flags"]) for i in range(6)] == [(66, 2), (1, 52), (1, 66), (66, 2), (1, 52), (1, 64)]
        assert [freight_orders[f"orders[{i}].dest"] for i in (1, 2, 4, 5)] == [0, 11, 12, 1]
        assert [depots[freight_orders[f"orders[{i}].dest"]]["xy"] for i in (0, 3)] == [5149, 25447]
        assert [(tube_orders[f"orders[{i}].type"], tube_orders[f"orders[{i}].flags"]) for i in range(4)] == [(1, 4), (1, 66), (66, 2), (66, 2)]
        assert [tube_orders[f"orders[{i}].dest"] for i in (0, 1)] == [11, 12]
        assert [depots[tube_orders[f"orders[{i}].dest"]]["xy"] for i in (2, 3)] == [25319, 24928]
        old_receivers, receivers = saved_table(source, b"SPRR"), saved_table(ready, b"SPRR")
        assert old_receivers.keys() == receivers.keys()
        for key, old in old_receivers.items():
            expected = dict(old, interval_ticks=(old["interval_ticks"] + ticks) % interval)
            assert receivers[key] == expected
        old_industries, industries = saved_table(source, b"INDY"), saved_table(ready, b"INDY")
        for receiver in receivers.values():
            index = receiver["industry"]
            assert industries[index]["accepted[0].waiting"] == old_industries[index]["accepted[0].waiting"] < 8
        assert industries[0]["produced[0].cargo"] == machinery["cargo"] and industries[0]["produced[0].rate"] > 0
        assert industries[1]["accepted[0].cargo"] == machinery["cargo"]
        assert saved_table(ready, b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
        company = saved_table(ready, b"PLYR")[0]
        assert company["money"] == measured["cash"] and company["current_loan"] == 300000
        connections, bindings, utility = saved_table(ready, b"TUPD"), saved_table(ready, b"TUVD"), saved_table(ready, b"UPLD")
        sources = [row for row in connections.values() if row["kind"] == 0]
        assert len(sources) == len(bindings) == 1
        power, binding = sources[0], next(iter(bindings.values()))
        assert power["source"] == machinery["source"] == binding["source"]
        assert power["owner"] == 0 and power["portal"] == 25055 and power["interchange"] == 24928 and power["active"] == 1
        assert power["invoice"] == machinery["lease"] == quote["lease"] == 20 and power["remainder"] == 0
        wire = sorted((row for row in connections.values() if row["kind"] == 1), key=lambda row: row["position"])
        assert [row["tile"] for row in wire] == [25183, 25184, 25056, 24928]
        assert binding["vehicle"] == machinery["capsule"] and binding["active"] == 1
        assert all(row["lifetime_generated"] == row["lifetime_consumed"] == row["transferred"] == row["ticks"] == 0 for row in utility.values())
        for owner in (b"TBMP", b"TBMS", b"TBXA", b"TBXS", b"TBXW"):
            assert saved_table(ready, owner) == {}
        records[cfg] = {"report": identity(path), "actual": report, "ready_input": report["saves"]["ready"], "ready": measured, "machinery": machinery,
                        "retained_spoil_wagons": sorted(spoil), "receivers": receivers, "power": power, "binding": binding}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCDC;actual normal-world paid Surface supply/native carriage/power readiness;useful operation remains6DCDD",
              "controller": identity(__file__), "layout_handoff": identity(handoff), "receiver_interval_source": identity(interval_source), "records": records,
              "classification": "normal_start_useful_supply", "bootstrap_claim": False, "native_cases": 2,
              "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "unchanged_owners": list(owners), "ignored_chunks": [],
              "remaining": "6DCDD actual useful operation;6DD native recovery/files;6DE genuine S1/S2 acceptance."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish both exact native preparation lineages after every selected owner passes."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
