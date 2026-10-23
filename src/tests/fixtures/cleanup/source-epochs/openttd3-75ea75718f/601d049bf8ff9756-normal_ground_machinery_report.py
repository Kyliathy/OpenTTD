"""Seal native Standard acquisition on both exact normally paid yards.

@file normal_ground_machinery_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Reserved jobs own uncut Excavating terrain;no opened floor or delivered spoil is claimed.
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
from normal_ground_machinery import YARD, YARD_SHA, fields
from save_tables import saved_table


def accept(report_paths, output):
    """Verify payments,real project/set/hauler ownership and complete native pure files."""
    handoff = ROOT / YARD
    assert identity(handoff)["sha256"] == YARD_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        yard = basis["records"][cfg]
        assert report["yard_handoff"] == identity(handoff)
        assert report["actual_input"] == yard["paid_input"]
        assert report["original_input"] == yard["actual"]["original_input"]
        assert report["classification"] == "normal_start_standard_machinery" and report["bootstrap_claim"] is False
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        ready = verify_reference(report["saves"]["ready"])
        repeat = verify_reference(report["saves"]["repeat"])
        before, after = native_world(source), native_world(ready)
        assert after == native_world(repeat)
        # Neither geological resources nor receiver stock can originate in acquisition.
        for owner in ("GDEP", "GSPN", "GVNT", "SPRR", "TBMS"):
            assert after[owner] == before[owner]
        measured = report["ready"]
        assert measured == fields(text, "NORMAL_GROUND_MACHINERY")
        assert report["quote"] == fields(text, "NORMAL_GROUND_COMMISSION_QUOTE")
        assert measured["original_solid"] == measured["solid"] + measured["excavating"] == 31752
        assert measured["reserved"] == measured["excavated"] == 0
        date = saved_table(ready, b"DATE")[0]
        previous_date = saved_table(source, b"DATE")[0]
        assert date["id"] == previous_date["id"] and date["tick_counter"] == measured["tick"] == yard["yard"]["tick"]
        assert date["pause_mode"] == previous_date["pause_mode"]
        cash = measured["opening_cash"]
        assert cash == yard["yard"]["cash"]
        native_invoices = [{key: value if key == "action" else int(value) for key, value in
                            (item.split("=", 1) for item in row.split())}
                           for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
        assert native_invoices == report["invoices"]
        for invoice in native_invoices:
            cash -= invoice["cost"]
            assert cash == invoice["cash"]
        assert cash == measured["cash"] > 0
        assert sum(row["cost"] for row in native_invoices) == report["capital_paid"]
        assert next(row["cost"] for row in native_invoices if row["action"] == "standard_commission") == report["quote"]["purchase"]
        player = saved_table(ready, b"PLYR")[0]
        assert player["money"] == cash and player["current_loan"] == measured["loan"] == 300000
        projects, jobs = saved_table(ready, b"TBMP"), saved_table(ready, b"XJOB")
        assert len(projects) == len(jobs) == 1
        project, job = projects[measured["project"]], jobs[measured["job"]]
        assert project["job"] == job["id"] == measured["job"]
        assert project["cutter"] == measured["cutter"] and project["model"] == 0 and project["company"] == 0
        assert project["origin"] == measured["origin"] and project["portal"] == measured["portal"]
        assert project["phase"] == 0 and project["blocked"] == 17
        assert all(project[key] == 0 for key in ("phase_ticks", "work_remainder", "active_ticks", "idle_ticks", "cost_remainder"))
        assert all(job[key] == 0 for key in ("cursor", "partial", "earth", "rock", "cost"))
        portals = saved_table(ready, b"LPWK")
        assert len(portals) == 1
        portal = next(iter(portals.values()))
        assert portal["key"] == measured["portal"] and portal["state"] == 1
        assert portal["project"] == job["id"] and portal["cutter"] == measured["cutter"] and portal["origin"] == measured["origin"]
        assignments, sets, wagons = (saved_table(ready, chunk) for chunk in (b"TBXA", b"TBXS", b"TBXW"))
        assert len(assignments) == len(sets) == 1 and len(wagons) == 4
        assignment = next(iter(assignments.values()))
        assert assignment["project"] == measured["project"] and assignment["hauler"] == measured["hauler"]
        assert assignment["bay"] == measured["portal"] and assignment["phase"] == assignment["exchanges"] == 0
        carried = sets[assignment["set"]]
        assert carried["project"] == measured["project"] and carried["state"] == 3 and carried["visited"] == 0
        assert {row["set"] for row in wagons.values()} == {carried["id"]}
        assert len({row["wagon"] for row in wagons.values()}) == carried["wagons"] == 4
        records[cfg] = {"report": identity(path), "actual": report, "ready_input": report["saves"]["ready"],
                        "ready": measured, "project": project, "job": job, "assignment": assignment, "set": carried}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CDB;native Standard cutter and actual hauler acquired on paid normal-world lineage;no opened floor",
              "controller": identity(__file__), "yard_handoff": identity(handoff), "records": records,
              "classification": "normal_start_standard_machinery", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "ignored_chunks": [],
              "remaining": "6CDC paid Standard access;6CDD Deep readiness;6D full normal-start S1/S2."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only complete native Standard acquisition on both original normal-world lineages."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
