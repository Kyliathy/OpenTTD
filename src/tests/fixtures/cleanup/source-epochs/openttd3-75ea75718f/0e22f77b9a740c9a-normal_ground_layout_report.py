"""Seal actual paid modal layouts without claiming useful freight operation.

@file normal_ground_layout_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Both original native lineages retain complete paid geology/jobs/spoil and pure files.
"""

import argparse
import json
import re
import struct
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from construction_provider_return import identity
from normal_ground_layout import IRREGULAR_WORK, IRREGULAR_WORK_SHA, invoices_from, fields, observations
from normal_ground_start_report import native_world
from normal_ground_deep_report import geological_payload
from save_tables import saved_table


def accept(report_paths, output):
    """Require native release,real adjacent station joins,paid exact layout and unchanged earned floor/history."""
    handoff = ROOT / IRREGULAR_WORK
    assert identity(handoff)["sha256"] == IRREGULAR_WORK_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    owners = ("GEOS", "GDEP", "GSPN", "GVNT", "EXJS", "XJOB", "XJRT", "XJTL", "XPAS", "SPRR", "INDY", "PATS", "NGRF", "CAPA")
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        assert report["classification"] == "normal_start_modal_layout" and report["bootstrap_claim"] is False
        assert report["irregular_work_handoff"] == identity(handoff) and report["actual_input"] == flow["settled_input"]
        assert report["original_input"] == flow["records"][0]["actual"]["original_input"]
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        for reference in report["focused_boundaries"]:
            raw = verify_reference(reference).read_bytes()
            boundary = raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
            assert "All tests passed" in boundary
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0 and int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        layout, repeat = (verify_reference(report["saves"][key]) for key in ("layout", "repeat"))
        before, after = native_world(source), native_world(layout)
        assert after == native_world(repeat)
        assert all(after[key] == before[key] for key in owners)
        # Native station construction calls Random for real station/tile art.
        # Retain those actual draws and compare every other DATE field/byte.
        old_date, new_date = saved_table(source, b"DATE")[0], saved_table(layout, b"DATE")[0]
        random_fields = ("random_state[0]", "random_state[1]")
        assert old_date.keys() == new_date.keys()
        assert all(new_date[key] == value for key, value in old_date.items() if key not in random_fields)
        old_random = [old_date[key] for key in random_fields]
        new_random = [new_date[key] for key in random_fields]
        old_bytes, new_bytes = struct.pack(">II", *old_random), struct.pack(">II", *new_random)
        assert before["DATE"].count(old_bytes) == after["DATE"].count(new_bytes) == 1
        assert before["DATE"].replace(old_bytes, new_bytes, 1) == after["DATE"]
        measured = report["layout"]
        assert measured == fields(text, "NORMAL_GROUND_LAYOUT")
        assert report["cells"] == observations(text, "NORMAL_GROUND_LAYOUT_CELL")
        assert report["invoices"] == invoices_from(text)
        assert measured["opening_cash"] == flow["closing"]["cash"] and measured["tick"] == flow["closing"]["tick"]
        assert measured["cutter"] == flow["closing"]["cutter"] and measured["hauler"] == flow["closing"]["hauler"]
        # Public Retire/StartStop sets Stopped on just these two fronts.
        # Every other native vehicle field,including every retained part/position/
        # orders/packets and all retained old spoil units,must remain exact.
        old_fleet, new_fleet = saved_table(source, b"VEHS"), saved_table(layout, b"VEHS")
        assert old_fleet.keys() == new_fleet.keys() and len(old_fleet) == measured["fleet"]
        status_key = "train[0].common[0].vehstatus"
        for vehicle, old_part in old_fleet.items():
            expected = dict(old_part)
            if vehicle in (measured["cutter"], measured["hauler"]):
                expected[status_key] |= 2
            assert new_fleet[vehicle] == expected, vehicle
        assert measured["loan"] == 300000 and measured["excavated"] == 112
        assert measured["portal"] == 8671 and measured["target"] == 25055
        assert measured["origin"] != measured["destination"]
        cash = measured["opening_cash"]
        for invoice in report["invoices"]:
            cash -= invoice["cost"]
            assert cash == invoice["cash"]
        assert cash == measured["cash"] > 0
        assert report["capital_paid"] == sum(row["cost"] for row in report["invoices"]) == measured["opening_cash"] - cash > 0
        assert sum(row["action"] == "retire_settled_irregular" for row in report["invoices"]) == 1
        assert sum(row["action"] == "modal_ordinary_connection" for row in report["invoices"]) == 1
        assert measured["supports"] == len(saved_table(source, b"TBMS")) > 0
        for owner in (b"TBMP", b"TBMS", b"TBXA", b"TBXS", b"TBXW"):
            assert saved_table(layout, owner) == {}
        player = saved_table(layout, b"PLYR")[0]
        assert player["money"] == cash and player["current_loan"] == 300000
        portals = {row["key"]: row for row in saved_table(layout, b"LPWK").values()}
        assert portals[8671]["state"] == 2 and portals[25823]["state"] == 0
        tube_coords = [(96, y) for y in range(66, 70)] + [(x, 69) for x in range(97, 104)]
        rail_coords = [(95, y) for y in range(67, 71)] + [(x, 70) for x in range(96, 104)]
        cells = report["cells"]
        assert [row["tile"] for row in cells] == [16384 + y * 128 + x for x, y in tube_coords + rail_coords]
        assert all(row["rail"] == measured["tube"] for row in cells[:len(tube_coords)])
        assert all(row["rail"] == 0 for row in cells[len(tube_coords):])
        cell_by_tile = {row["tile"]: row for row in cells}
        assert cell_by_tile[16384 + 68 * 128 + 96]["station"] == cell_by_tile[16384 + 69 * 128 + 95]["station"] == measured["origin"]
        assert cell_by_tile[16384 + 69 * 128 + 101]["station"] == cell_by_tile[16384 + 70 * 128 + 101]["station"] == measured["destination"]
        assert {row["tile"] for row in cells if row["depot"]} == {measured["tube_home"], measured["tube_far"], measured["freight_far"]}
        geology = geological_payload(layout)
        assert all((geology[39 + (y * 128 + x) // 4] >> ((y * 128 + x) % 4 * 2) & 3) == 2 for x, y in tube_coords + rail_coords)
        records[cfg] = {"report": identity(path), "actual": report, "layout_input": report["saves"]["layout"], "layout": measured,
                        "native_station_random": {"opening": old_random, "closing": new_random}, "unchanged_date_fields": len(old_date) - 2,
                        "exact_retained_fleet": len(old_fleet),
                        "changed_chunks": sorted(key for key in before if before[key] != after[key])}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCDB;actual paid modal layout on original earned irregular floor;useful supply/operation remains pending",
              "controller": identity(__file__), "irregular_work_handoff": identity(handoff), "records": records,
              "classification": "normal_start_modal_layout", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "unchanged_owners": list(owners), "ignored_chunks": [],
              "remaining": "6DCDC actual useful supply and power;6DCDD full native useful freight;6DD native recovery/files;6DE genuine acceptance."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only complete actual layout evidence on both original native lineages."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
