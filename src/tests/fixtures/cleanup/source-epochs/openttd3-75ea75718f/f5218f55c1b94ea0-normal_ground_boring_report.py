"""Seal actual native paid Standard clearing and recurring spoil settlement.

@file normal_ground_boring_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Consume exact acquisition and full native checkpoint chains;never recreate a world.
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
from normal_ground_boring import MACHINERY, MACHINERY_SHA, fields, observations
from save_tables import saved_table, geology_snapshot


def read_chain(path):
    """Resolve exact earlier report references and reject reused or mutated intermediates."""
    result, seen = [], set()
    while path is not None:
        reference = identity(path)
        assert reference["path"] not in seen
        seen.add(reference["path"])
        report = json.loads(path.read_text(encoding="utf-8"))
        result.append({"report": reference, "actual": report})
        path = None if report["predecessor"] is None else verify_reference(report["predecessor"])
    return list(reversed(result))


def accept(report_paths, output):
    """Require both complete actual clearing/haulage/processing/withdrawal lineages and exact costs."""
    handoff = ROOT / MACHINERY
    assert identity(handoff)["sha256"] == MACHINERY_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    for name in report_paths:
        path = ROOT / name
        chain = read_chain(path)
        cfg = chain[-1]["actual"]["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        previous_ref, previous = flow["ready_input"], flow["ready"]
        controls, all_traces, all_wagons, all_packets = [], [], [], []
        for entry in chain:
            report = entry["actual"]
            assert report["configuration"] == cfg and report["classification"] == "normal_start_paid_standard_work"
            assert report["bootstrap_claim"] is False and report["children_reaped"] is True
            assert report["machinery_handoff"] == identity(handoff) and report["original_input"] == flow["actual"]["original_input"]
            assert report["actual_input"] == previous_ref and report["pure_chunks_equal"] == 126
            for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
                verify_reference(reference)
            text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert counts and int(counts[1]) == report["native"]["assertions"] > 0
            assert int(counts[2]) == report["native"]["cases"] == 1
            bored = report["bored"]
            assert bored == fields(text, "NORMAL_GROUND_BORED")
            assert bored["opening_cash"] == previous["cash"] and bored["first_tick"] == previous["tick"]
            assert bored["loan"] == previous["loan"] == 300000
            assert bored["project"] == flow["ready"]["project"] and bored["job"] == flow["ready"]["job"]
            assert bored["cutter"] == flow["ready"]["cutter"] and bored["hauler"] == flow["ready"]["hauler"]
            ledger = report["ledger"]
            assert ledger == observations(text, "NORMAL_GROUND_BORE_LEDGER")
            assert {row["category"] for row in ledger} == set(range(13)) and len(ledger) == 13
            assert bored["cash"] == bored["opening_cash"] - sum(row["cost"] for row in ledger)
            for key, marker in (("traces", "TRACE"), ("sets", "SET"), ("wagons", "WAGON"), ("packets", "PACKET"), ("receivers", "RECEIVER")):
                assert report[key] == observations(text, f"NORMAL_GROUND_BORE_{marker}")
            parsed_controls = [{key: value if key == "action" else int(value) for key, value in
                                (item.split("=", 1) for item in row.split())}
                               for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
            assert parsed_controls == report["controls"] and all(row["cost"] == 0 for row in parsed_controls)
            controls.extend(parsed_controls)
            all_traces.extend(report["traces"]); all_wagons.extend(report["wagons"]); all_packets.extend(report["packets"])
            source = verify_reference(report["actual_input"])
            progress, repeat = (verify_reference(report["saves"][key]) for key in ("progress", "repeat"))
            before, after = native_world(source), native_world(progress)
            assert after == native_world(repeat)
            assert all(after[key] == before[key] for key in ("GDEP", "GSPN", "GVNT"))
            old_date, date = saved_table(source, b"DATE")[0], saved_table(progress, b"DATE")[0]
            assert date["id"] == old_date["id"] and date["tick_counter"] == bored["tick"] > bored["first_tick"]
            assert date["pause_mode"] == old_date["pause_mode"]
            company = saved_table(progress, b"PLYR")[0]
            assert company["money"] == bored["cash"] > 0 and company["current_loan"] == 300000
            job = saved_table(progress, b"XJOB")[bored["job"]]
            assert job["cursor"] == bored["cursor"] and job["routes"] == bored["routes"]
            assert (job["earth"], job["rock"], job["cost"]) == (bored["earth"], bored["rock"], bored["work_cost"])
            previous_ref, previous = report["saves"]["progress"], bored
        final = chain[-1]["actual"]
        bored = final["bored"]
        assert bored["complete"] == bored["returned"] == bored["resumed"] == 1
        assert bored["cursor"] == bored["routes"] and bored["exchanges"] >= 2
        assert bored["release_error"] == 65535 and bored["phase"] == 7 and bored["blocked"] == 0
        assert bored["earth"] == bored["earth_received"] == flow["actual"]["quote"]["earth"]
        assert bored["rock"] == bored["rock_received"] == flow["actual"]["quote"]["rock"]
        assert bored["earth_processed"] + bored["earth_waiting"] == bored["earth"]
        assert bored["rock_processed"] + bored["rock_waiting"] == bored["rock"]
        saved_receivers = saved_table(verify_reference(previous_ref), b"SPRR")
        industries = saved_table(verify_reference(previous_ref), b"INDY")
        assert len(saved_receivers) == len(final["receivers"]) == 2
        for observed in final["receivers"]:
            receiver = next(row for row in saved_receivers.values() if row["industry"] == observed["industry"])
            industry = industries[observed["industry"]]
            assert industry["accepted.count"] == 1 and industry["accepted[0].waiting"] == observed["waiting"] < observed["batch"] == 8
            for field in ("kind", "disposed", "crushed", "aggregate", "aggregate_made", "residue", "fees"):
                assert receiver[field] == observed[field]
            if observed["kind"] == 0:
                assert observed["disposed"] + observed["waiting"] == bored["earth"]
                assert observed["fees"] == 2 * bored["earth"]
            else:
                assert observed["disposed"] + observed["crushed"] + observed["waiting"] == bored["rock"]
                assert observed["aggregate_made"] + observed["residue"] == observed["crushed"]
                assert observed["fees"] == 3 * bored["rock"]
        assert bored["fees"] > 0 and bored["work_cost"] > 0 and bored["excavating"] == bored["reserved"] == 0
        assert sum(row["action"] == "partial_return_and_park" for row in controls) == 1
        assert sum(row["action"] == "settled_resume" for row in controls) == 1
        assert any(row["phase"] == 11 and row["cursor"] < row["routes"] for row in all_traces)
        assert any(row["hauler_phase"] == 4 and row["held"] > 0 for row in all_traces)
        assert all_traces[-1]["held"] == 0
        final_tick = max(row["tick"] for row in all_wagons)
        assert all(row["amount"] == 0 for row in all_wagons if row["tick"] == final_tick)
        assert len({row["wagon"] for row in all_wagons}) == 8
        assert all_packets and all(row["feeder"] == 0 and 16384 <= row["source"] < 32768 for row in all_packets)
        # This compares actual complete geological owner headers/planes,including their unrequested rim.
        start_geo = geology_snapshot(verify_reference(flow["ready_input"]))
        end_geo = geology_snapshot(verify_reference(previous_ref))
        assert start_geo["header"] == end_geo["header"] and start_geo["layers"]["deep"] == end_geo["layers"]["deep"]
        before, after = start_geo["layers"]["underground"], end_geo["layers"]["underground"]
        assert after["solid"] == before["solid"] and after["excavating"] == after["reserved"] == 0
        assert after["excavated"] == before["excavated"] + before["excavating"] == bored["excavated"] == 36
        records[cfg] = {"report": identity(path), "records": chain, "settled_input": previous_ref, "closing": bored,
                        "geology": end_geo, "controls": controls}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CDCB;actual normal-world paid clearing and repeated physical spoil settlement;final release/access pending",
              "controller": identity(__file__), "machinery_handoff": identity(handoff), "records": records,
              "classification": "normal_start_paid_standard_work", "bootstrap_claim": False,
              "native_cases": sum(len(row["records"]) for row in records.values()),
              "native_assertions": sum(entry["actual"]["native"]["assertions"] for row in records.values() for entry in row["records"]),
              "pure_pairs": sum(len(row["records"]) for row in records.values()), "complete_chunks_equal": 126, "ignored_chunks": [],
              "remaining": "6CDCC final release/support conversion/ordinary access;6CDD Deep readiness;6D full S1/S2."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only complete native clearing and recurring Surface settlement on both normal originals."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
