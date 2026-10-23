"""Seal useful modal freight on actual normal-world paid irregular floor.

@file normal_ground_useful_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Income alone cannot close this gate:loaded curved tube travel and both empty returns are mandatory.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from construction_provider_return import identity
from normal_ground_useful import SUPPLY, SUPPLY_SHA, fields, observations, invoices_from
from normal_ground_boring_report import read_chain
from normal_ground_start_report import native_world
from normal_ground_supply_report import TRAIN
from save_tables import saved_table


def require_physical_useful(stats, traces, packets, controls, machinery):
    """Refuse direct Surface income or partial motion without every actual loaded modal leg and native empty return."""
    assert any(row["loaded_curve"] for row in stats), "No actual loaded curved tube travel"
    assert any(row["surface_pickup"] for row in stats), "No real Surface supply pickup"
    assert any(row["loaded_ug_return"] for row in stats), "No loaded conventional UG interchange pickup"
    assert any(row["loaded_surface_return"] for row in stats), "No actual loaded Surface return"
    final = stats[-1]
    assert final["complete"] == final["carrier_home"] == final["capsule_home"] == 1, "Whole native empty returns are missing"
    assert final["a_stock"] == final["b_stock"] == 0
    assert sum(row["delivered"] for row in stats) == machinery["capacity"] == 64
    assert sorted(row["action"] for row in controls) == [1, 2]
    assert max(row["peak_carrier"] for row in stats) == 64 and max(row["peak_capsule"] for row in stats) == 60
    assert final["energy"] > stats[0]["first_energy"] and final["invoice"] > stats[0]["first_invoice"]
    assert any(row["capsule_tile"] == 25312 and row["capsule_held"] > 0 for row in traces)
    assert any(a["capsule_held"] > 0 and b["capsule_held"] > 0 and a["capsule_tile"] != b["capsule_tile"] and b["energy"] > a["energy"]
               for a, b in zip(traces, traces[1:])), "Loaded capsule movement did not consume native energy"
    loaded_pickup = min(row["tick"] for row in traces if row["carrier_station"] == 12 and row["carrier_held"] > 0)
    assert all(row["cumulative_delivered"] == 0 for row in traces if row["tick"] < loaded_pickup), "Surface delivery preceded the actual modal pickup"
    assert any(row["front"] == machinery["capsule"] and row["feeder"] > 0 for row in packets)
    assert any(row["front"] == machinery["carrier"] and row["feeder"] > 0 for row in packets)
    grouped = {}
    for packet in packets:
        assert packet["source_kind"] == packet["source_id"] == packet["first"] == 0
        assert packet["amount"] > 0 and 0 <= packet["source"] < 16384
        assert packet["front"] in (machinery["carrier"], machinery["capsule"])
        key = packet["tick"], packet["front"]
        grouped[key] = grouped.get(key, 0) + packet["amount"]
    assert all(amount <= (60 if front == machinery["capsule"] else 64) for (_, front), amount in grouped.items())


def accept(report_paths, output):
    """Require exact native lineage,all real paid owners,actual useful modal delivery and complete endpoint saves."""
    handoff = ROOT / SUPPLY
    assert identity(handoff)["sha256"] == SUPPLY_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    interval_source = ROOT / "src/spoil_receiver.h"
    interval = int(re.search(r"SPOIL_RECEIVER_INTERVAL_TICKS\s*=\s*(\d+)", interval_source.read_text(encoding="utf-8"))[1])
    owners = ("GEOS", "GDEP", "GSPN", "GVNT", "EXJS", "XJOB", "XJRT", "XJTL", "XPAS", "PATS", "NGRF")
    records = {}
    for name in report_paths:
        chain = read_chain(ROOT / name)
        cfg = chain[-1]["actual"]["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        previous_ref, previous = flow["ready_input"], flow["ready"]
        stats, traces, packets, controls, totals = [], [], [], [], [0] * 13
        energy, invoice, delivered = 0, 20, 0
        for entry in chain:
            report = entry["actual"]
            assert report["configuration"] == cfg and report["classification"] == "normal_start_useful_operation"
            assert report["driver"] == "ground-acceptance-null"
            assert report["bootstrap_claim"] is False and report["children_reaped"] is True
            assert report["supply_handoff"] == identity(handoff) and report["actual_input"] == previous_ref
            assert report["original_input"] == flow["actual"]["original_input"]
            for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
                verify_reference(reference)
            boundary = verify_reference(report["motion_boundary"]).read_bytes()
            assert "All tests passed (564 assertions in 1 test case)" in boundary.decode("utf-16" if boundary.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
            text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
            count = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert count and int(count[1]) == report["native"]["assertions"] > 0 and int(count[2]) == report["native"]["cases"] == 1
            measured = report["useful"]
            assert measured == fields(text, "NORMAL_GROUND_USEFUL")
            for key in ("ledger", "traces", "packets", "controls"):
                assert report[key] == observations(text, "NORMAL_GROUND_USEFUL_" + ("TRACE" if key == "traces" else "PACKET" if key == "packets" else "CONTROL" if key == "controls" else "LEDGER"))
            assert report["invoices"] == invoices_from(text) and all(row["cost"] == 0 for row in report["invoices"])
            assert measured["opening_cash"] == previous["cash"] and measured["first_tick"] == previous["tick"]
            ticks = measured["tick"] - measured["first_tick"]
            assert 0 < ticks <= report["tick_budget"] + 3000 and measured["loan"] == 300000
            assert measured["carrier"] == flow["machinery"]["carrier"] and measured["capsule"] == flow["machinery"]["capsule"]
            assert measured["first_energy"] == energy and measured["first_invoice"] == invoice
            assert measured["energy"] >= energy and measured["invoice"] >= invoice
            assert len(report["ledger"]) == 13 and sorted(row["category"] for row in report["ledger"]) == list(range(13))
            assert measured["cash"] == measured["opening_cash"] - sum(row["cost"] for row in report["ledger"])
            for row in report["ledger"]:
                totals[row["category"]] += row["cost"]
            source = verify_reference(report["actual_input"])
            progress, repeat = (verify_reference(report["saves"][key]) for key in ("progress", "repeat"))
            before, after = native_world(source), native_world(progress)
            assert report["pure_chunks_equal"] == 126 and after == native_world(repeat)
            assert all(after[key] == before[key] for key in owners)
            assert saved_table(progress, b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
            old_receivers, receivers = saved_table(source, b"SPRR"), saved_table(progress, b"SPRR")
            assert old_receivers.keys() == receivers.keys()
            for key, old in old_receivers.items():
                assert receivers[key] == dict(old, interval_ticks=(old["interval_ticks"] + ticks) % interval)
            old_industries, industries = saved_table(source, b"INDY"), saved_table(progress, b"INDY")
            for receiver in receivers.values():
                index = receiver["industry"]
                assert industries[index]["accepted[0].waiting"] == old_industries[index]["accepted[0].waiting"] < 8
            old_fleet, new_fleet = saved_table(source, b"VEHS"), saved_table(progress, b"VEHS")
            old_trains = {key: row for key, row in old_fleet.items() if TRAIN + "engine_type" in row}
            new_trains = {key: row for key, row in new_fleet.items() if TRAIN + "engine_type" in row}
            assert old_trains.keys() == new_trains.keys()
            for vehicle, old in old_trains.items():
                assert all(new_trains[vehicle][TRAIN + field] == old[TRAIN + field] for field in ("owner", "engine_type", "cargo_type", "cargo_cap"))
            spoil = [row for row in new_trains.values() if row[TRAIN + "engine_type"] in (258, 259)]
            assert len(spoil) == 20 and all(not row[TRAIN + "cargo.packets"] and row[TRAIN + "cargo.action_counts"] == [0] * 4 for row in spoil)
            assert all(new_trains[vehicle][TRAIN + "vehstatus"] & 2 and new_trains[vehicle][TRAIN + "tile"] == tile for vehicle, tile in ((2, 5149), (33, 7391)))
            orders = saved_table(progress, b"ORDL")
            for vehicle, expected_count in ((measured["carrier"], 6), (measured["capsule"], 4)):
                orderlist = orders[new_trains[vehicle][TRAIN + "orders"] - 1]
                assert orderlist["orders.count"] == expected_count
                assert all(orderlist[f"orders[{i}].type"] & 64 for i in range(expected_count)), "Useful orders allow intermediate stops"
            power = next(row for row in saved_table(progress, b"TUPD").values() if row["kind"] == 0)
            producer = next(row for row in saved_table(progress, b"UPLD").values() if row["type"] == 0 and row["id"] == flow["machinery"]["source"])
            assert power["invoice"] == measured["invoice"] and power["active"] == producer["active"] == 1
            assert producer["lifetime_consumed"] == measured["energy"]
            company = saved_table(progress, b"PLYR")[0]
            assert company["money"] == measured["cash"] and company["current_loan"] == 300000
            stats.append(measured)
            traces.extend(dict(row, cumulative_delivered=delivered + row["delivered"]) for row in report["traces"])
            packets.extend(report["packets"]); controls.extend(report["controls"])
            delivered += measured["delivered"]
            previous_ref, previous, energy, invoice = report["saves"]["progress"], measured, measured["energy"], measured["invoice"]
        require_physical_useful(stats, traces, packets, controls, flow["machinery"])
        assert previous["cash"] == flow["ready"]["cash"] - sum(totals)
        records[cfg] = {"records": chain, "settled_input": previous_ref, "closing": previous, "delivered": delivered,
                        "ledger": [{"category": key, "cost": value} for key, value in enumerate(totals)],
                        "controls": controls, "actual_loaded_curve": True, "both_empty_returns": True}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCDD;actual useful normal-world curved tube/modal freight and paid original owners;native files/recovery/publication still follow",
              "controller": identity(__file__), "supply_handoff": identity(handoff), "receiver_interval_source": identity(interval_source), "records": records,
              "classification": "normal_start_useful_modal_freight", "bootstrap_claim": False,
              "native_cases": sum(len(row["records"]) for row in records.values()),
              "native_assertions": sum(entry["actual"]["native"]["assertions"] for row in records.values() for entry in row["records"]),
              "pure_pairs": sum(len(row["records"]) for row in records.values()), "complete_chunks_equal": 126,
              "unchanged_owners": list(owners), "ignored_chunks": [], "remaining": "6DD ordinary native readers/refusal/recovery;6DE genuine S1/S2 publication."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish useful final conversion only when both complete original native lineages pass every obligation."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
