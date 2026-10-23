"""Seal native irregular readiness using actually retained normal-world machinery.

@file normal_ground_irregular_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Canonical paid track maintenance/reuse preserves all earlier material,clock and policy owners.
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
from normal_ground_boring import MACHINERY, MACHINERY_SHA, observations
from normal_ground_irregular import DEEP_WORK, DEEP_WORK_SHA, fields
from normal_ground_deep_report import geological_payload
from save_tables import saved_table, geology_snapshot


def accept(report_paths, output):
    """Require actual reuse,unchanged prior histories/default policy and a real seven-bend native proposal."""
    handoff, original_machinery = ROOT / DEEP_WORK, ROOT / MACHINERY
    assert identity(handoff)["sha256"] == DEEP_WORK_SHA
    assert identity(original_machinery)["sha256"] == MACHINERY_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    retained = json.loads(original_machinery.read_text(encoding="utf-8"))
    records = {}
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        assert report["classification"] == "normal_start_irregular_readiness" and report["bootstrap_claim"] is False
        assert report["deep_work_handoff"] == identity(handoff) and report["actual_input"] == flow["settled_input"]
        assert report["original_input"] == flow["records"][0]["actual"]["original_input"]
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0 and int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        ready, repeat = (verify_reference(report["saves"][key]) for key in ("ready", "repeat"))
        before, after = native_world(source), native_world(ready)
        assert after == native_world(repeat)
        assert all(after[key] == before[key] for key in ("GDEP", "GSPN", "GVNT", "SPRR", "INDY", "TBMS", "DATE", "PATS", "NGRF"))
        measured, quotes = report["ready"], report["quotes"]
        assert measured == fields(text, "NORMAL_GROUND_IRREGULAR") and quotes == observations(text, "NORMAL_GROUND_IRREGULAR_QUOTE")
        assert measured["opening_cash"] == flow["closing"]["cash"] and measured["tick"] == flow["closing"]["tick"]
        assert measured["cutter"] == retained["records"][cfg]["ready"]["cutter"] and measured["hauler"] == retained["records"][cfg]["ready"]["hauler"]
        assert measured["loan"] == 300000 and measured["original_forbid90"] == measured["forbid90"] == 1
        assert measured["attempts"] == len(quotes) and quotes[-1]["ok"] == 1 and quotes[-1]["purchase"] == 0
        assert all(row["ok"] == 0 for row in quotes[:-1]) and any(row["error"] == 4265 for row in quotes)
        assert any(row["error"] == 4267 for row in quotes)
        assert quotes[-1]["first"] == measured["first"] and quotes[-1]["last"] == measured["last"]
        invoices = [{key: value if key == "action" else int(value) for key, value in
                     (item.split("=", 1) for item in row.split())}
                    for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
        assert invoices == report["invoices"]
        assert all(row["action"] in ("retire_settled_deep", "reseal_standard_source", "irregular_source_clear", "irregular_support_piece", "reuse_standard", "irregular_assign", "pause_irregular") for row in invoices)
        for action in ("retire_settled_deep", "reseal_standard_source", "reuse_standard", "irregular_assign", "pause_irregular"):
            assert sum(row["action"] == action for row in invoices) == 1
        assert next(row["cost"] for row in invoices if row["action"] == "reuse_standard") == 0
        cash = measured["opening_cash"]
        for invoice in invoices:
            cash -= invoice["cost"]
            assert cash == invoice["cash"]
        assert cash == measured["cash"] > 0 and report["capital_paid"] == sum(row["cost"] for row in invoices)
        company = saved_table(ready, b"PLYR")[0]
        assert company["money"] == cash and company["current_loan"] == 300000
        for owner in (b"XJOB", b"XJRT", b"XJTL"):
            old_rows, new_rows = saved_table(source, owner), saved_table(ready, owner)
            assert all(new_rows[key] == value for key, value in old_rows.items())
        jobs = saved_table(ready, b"XJOB")
        job = next(row for row in jobs.values() if row["id"] == measured["job"])
        assert len(jobs) == 3 and job["requested"] == 15 and job["routes"] == 13
        assert all(job[key] == 0 for key in ("cursor", "partial", "earth", "rock", "cost"))
        projects = saved_table(ready, b"TBMP")
        assert len(projects) == 1
        project = next(iter(projects.values()))
        assert project["id"] == measured["project"] and project["job"] == measured["job"] and project["cutter"] == measured["cutter"] and project["model"] == 0
        assert project["phase"] == 0 and project["blocked"] == 17 and project["active_ticks"] == project["idle_ticks"] == 0
        route = sorted((row for row in saved_table(ready, b"XJRT").values() if row["job"] == measured["job"]), key=lambda row: row["ordinal"])
        turns = [(b["heading"] - a["heading"]) % 8 for a, b in zip(route, route[1:])]
        assert all(turn in (0, 1, 7) for turn in turns) and sum(turn != 0 for turn in turns) == measured["bends"] == 7
        assignments, sets, wagons = (saved_table(ready, key) for key in (b"TBXA", b"TBXS", b"TBXW"))
        assert len(assignments) == len(sets) == 1 and len(wagons) == 4
        assignment, carried = next(iter(assignments.values())), next(iter(sets.values()))
        assert assignment["project"] == measured["project"] and assignment["hauler"] == measured["hauler"] and assignment["set"] == carried["id"]
        assert carried["project"] == measured["project"] and carried["state"] == 3 and carried["visited"] == 0 and carried["wagons"] == 4
        portals = saved_table(ready, b"LPWK")
        surface = next(row for row in portals.values() if row["key"] == measured["portal"])
        assert surface["state"] == 1 and surface["project"] == measured["job"] and surface["cutter"] == measured["cutter"]
        deep = next(row for row in portals.values() if row["key"] != measured["portal"])
        assert deep["state"] == 0 and deep["project"] == 4294967295 and deep["cutter"] == 1048575
        before_geo, after_geo = geological_payload(source), geological_payload(ready)
        assert before_geo[:39] == after_geo[:39]
        old_states = [(byte >> shift) & 3 for byte in before_geo[39:] for shift in (0, 2, 4, 6)]
        new_states = [(byte >> shift) & 3 for byte in after_geo[39:] for shift in (0, 2, 4, 6)]
        changes = [index for index, (a, b) in enumerate(zip(old_states, new_states, strict=True)) if a != b]
        assert len(changes) == 40 and all(index < 16384 and old_states[index] == 0 and new_states[index] == 1 for index in changes)
        assert measured["excavated"] == 72 and measured["excavating"] == 40
        records[cfg] = {"report": identity(path), "actual": report, "ready_input": report["saves"]["ready"], "ready": measured,
                        "quote": quotes[-1], "project": project, "job": job, "assignment": assignment, "set": carried, "geology": geology_snapshot(ready)}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCB;retained actual Standard machinery accepts canonical seven-bend irregular proposal;no new cut yet",
              "controller": identity(__file__), "deep_work_handoff": identity(handoff), "original_machinery": identity(original_machinery), "records": records,
              "classification": "normal_start_irregular_readiness", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "ignored_chunks": [],
              "remaining": "6DCC actual irregular paid work;6DCD useful final conversion;6DD native recovery/files;6DE genuine S1/S2 publication."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only real retained-participant irregular readiness with unchanged native policy and paid history."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
