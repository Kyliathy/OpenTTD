"""Seal native Deep readiness on the actual Standard-earned normal originals.

@file normal_ground_deep_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The complete original Underground plane and settled material owners remain unchanged.
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
from normal_ground_start_report import native_world
from normal_ground_deep import ACCESS, ACCESS_SHA, fields
from save_tables import saved_table, geology_snapshot, chunks, read_stream


def geological_payload(path):
    """Read the complete real packed terrain owner without constructing any gameplay state."""
    _, stream = read_stream(path)
    return next(row["payload"] for row in chunks(stream) if row["name"] == b"GEOS")


def accept(report_paths, output):
    """Verify paid larger participants,new native job and unchanged actual access/history/stock owners."""
    handoff = ROOT / ACCESS
    assert identity(handoff)["sha256"] == ACCESS_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        assert report["classification"] == "normal_start_deep_machinery" and report["bootstrap_claim"] is False
        assert report["access_handoff"] == identity(handoff) and report["actual_input"] == flow["access_input"]
        assert report["original_input"] == flow["actual"]["original_input"]
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        ready, repeat = (verify_reference(report["saves"][key]) for key in ("ready", "repeat"))
        before, after = native_world(source), native_world(ready)
        assert after == native_world(repeat)
        assert all(after[owner] == before[owner] for owner in ("GDEP", "GSPN", "GVNT", "SPRR", "INDY"))
        # Ordinary native vehicle construction draws random_bits in train_cmd.
        # Retain both actual states and compare every other field/raw DATE byte.
        old_date, new_date = saved_table(source, b"DATE")[0], saved_table(ready, b"DATE")[0]
        random_fields = ("random_state[0]", "random_state[1]")
        assert old_date.keys() == new_date.keys()
        assert all(new_date[key] == value for key, value in old_date.items() if key not in random_fields)
        old_random = [old_date[key] for key in random_fields]
        new_random = [new_date[key] for key in random_fields]
        old_bytes, new_bytes = struct.pack(">II", *old_random), struct.pack(">II", *new_random)
        assert before["DATE"].count(old_bytes) == after["DATE"].count(new_bytes) == 1
        assert before["DATE"].replace(old_bytes, new_bytes, 1) == after["DATE"]
        old_geo, new_geo = geological_payload(source), geological_payload(ready)
        geo = geology_snapshot(ready)
        plane_bytes = geo["width"] * geo["height"] // 4
        assert old_geo[:39 + plane_bytes] == new_geo[:39 + plane_bytes]
        assert geo["layers"]["underground"] == geology_snapshot(source)["layers"]["underground"]
        assert geo["layers"]["deep"]["excavated"] == geo["layers"]["deep"]["reserved"] == 0
        assert geo["layers"]["deep"]["excavating"] == 36
        measured, quote = report["ready"], report["quote"]
        assert measured == fields(text, "NORMAL_GROUND_DEEP") and quote == fields(text, "NORMAL_GROUND_DEEP_QUOTE")
        assert measured["opening_cash"] == flow["access"]["cash"] and measured["tick"] == flow["access"]["tick"]
        assert measured["loan"] == 300000 and measured["excavated"] == 36 and measured["excavating"] == 36
        assert measured["earth_wagons"] == 2 and measured["rock_wagons"] == 4
        invoices = [{key: value if key == "action" else int(value) for key, value in
                     (item.split("=", 1) for item in row.split())}
                    for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
        assert invoices == report["invoices"]
        assert sum(row["action"] == "machinery_unit" for row in invoices) == 7
        for action in ("deep_platform", "deep_portal", "deep_origin_order", "deep_commission", "deep_assign", "pause_deep"):
            assert sum(row["action"] == action for row in invoices) == 1
        assert next(row["cost"] for row in invoices if row["action"] == "deep_commission") == quote["purchase"] == 803510
        cash = measured["opening_cash"]
        for invoice in invoices:
            cash -= invoice["cost"]
            assert cash == invoice["cash"]
        assert cash == measured["cash"] > 0
        assert report["capital_paid"] == sum(row["cost"] for row in invoices) == 972834
        company = saved_table(ready, b"PLYR")[0]
        assert company["money"] == cash and company["current_loan"] == 300000
        # Native named-table indices are row ordinals,not the monotonic owner IDs.
        projects = saved_table(ready, b"TBMP")
        assert len(projects) == 1
        project = next(iter(projects.values()))
        assert project["id"] == measured["project"] and project["job"] == measured["job"] and project["model"] == 1
        assert project["cutter"] == measured["cutter"] and project["portal"] == measured["portal"] and project["origin"] == measured["origin"]
        assert project["phase"] == 0 and project["blocked"] == 17
        assert all(project[key] == 0 for key in ("phase_ticks", "work_remainder", "active_ticks", "idle_ticks", "cost_remainder"))
        jobs = saved_table(ready, b"XJOB")
        assert len(jobs) == 2
        job = next(row for row in jobs.values() if row["id"] == measured["job"])
        assert all(job[key] == 0 for key in ("cursor", "partial", "earth", "rock", "cost"))
        for owner in (b"XJOB", b"XJRT", b"XJTL"):
            old_rows, new_rows = saved_table(source, owner), saved_table(ready, owner)
            assert all(new_rows[key] == value for key, value in old_rows.items())
        portals = saved_table(ready, b"LPWK")
        assert len(portals) == 2
        source_portal = next(row for row in portals.values() if row["key"] == measured["access"])
        assert source_portal == next(iter(saved_table(source, b"LPWK").values())) and source_portal["state"] == 2
        deep_portal = next(row for row in portals.values() if row["key"] == measured["portal"])
        assert deep_portal["state"] == 1 and deep_portal["project"] == measured["job"] and deep_portal["cutter"] == measured["cutter"]
        assignments, sets, wagons = (saved_table(ready, owner) for owner in (b"TBXA", b"TBXS", b"TBXW"))
        assert len(assignments) == len(sets) == 1 and len(wagons) == 6
        assignment, carried = next(iter(assignments.values())), next(iter(sets.values()))
        assert assignment["project"] == measured["project"] and assignment["hauler"] == measured["hauler"] and assignment["bay"] == measured["portal"]
        assert assignment["set"] == carried["id"] and assignment["phase"] == assignment["exchanges"] == 0
        assert carried["project"] == measured["project"] and carried["state"] == 3 and carried["visited"] == 0 and carried["wagons"] == 6
        assert {row["set"] for row in wagons.values()} == {carried["id"]} and len({row["wagon"] for row in wagons.values()}) == 6
        records[cfg] = {"report": identity(path), "actual": report, "ready_input": report["saves"]["ready"],
                        "ready": measured, "quote": quote, "project": project, "job": job, "assignment": assignment, "set": carried, "geology": geo,
                        "native_purchase_random": {"opening": old_random, "closing": new_random}, "unchanged_date_fields": len(old_date) - 2}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CDD;native Deep readiness paid on Standard-earned accessible normal originals;no Deep cuts yet",
              "controller": identity(__file__), "access_handoff": identity(handoff), "records": records,
              "classification": "normal_start_deep_machinery", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "ignored_chunks": [],
              "remaining": "6D full actual normal-start S1/S2 irregular clearing/useful final-use,recurring haulage and native recovery/runner provenance."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only complete paid Deep readiness on both actual normal-world lineages."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
