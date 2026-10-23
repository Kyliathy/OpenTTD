"""Seal real normal-world Deep work without replacing prior Standard material owners.

@file normal_ground_deep_work_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Both actual portal legs,complete paid histories and native batch-tail stocks remain explicit.
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
from normal_ground_boring import DEEP_MACHINERY, DEEP_MACHINERY_SHA, fields, observations
from normal_ground_boring_report import read_chain
from normal_ground_deep_report import geological_payload
from save_tables import saved_table, geology_snapshot


def accept(report_paths, output):
    """Verify every real Deep checkpoint,all13expenses,actual material legs and unchanged earned Underground."""
    handoff = ROOT / DEEP_MACHINERY
    assert identity(handoff)["sha256"] == DEEP_MACHINERY_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    for name in report_paths:
        path = ROOT / name
        chain = read_chain(path)
        cfg = chain[-1]["actual"]["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        initial = verify_reference(flow["ready_input"])
        initial_jobs = saved_table(initial, b"XJOB")
        earlier_jobs = [row for row in initial_jobs.values() if row["id"] != flow["ready"]["job"]]
        prior_earth = sum(row["earth"] for row in earlier_jobs)
        prior_rock = sum(row["rock"] for row in earlier_jobs)
        assert prior_earth == 108 and prior_rock == 216
        prior_fees = sum(row["fees"] for row in saved_table(initial, b"SPRR").values())
        previous_ref, previous = flow["ready_input"], flow["ready"]
        controls, traces, wagons, packets = [], [], [], []
        for entry in chain:
            report = entry["actual"]
            assert report["configuration"] == cfg and report["classification"] == "normal_start_paid_deep_work"
            assert report["bootstrap_claim"] is False and report["children_reaped"] is True
            assert report["machinery_handoff"] == identity(handoff) and report["original_input"] == flow["actual"]["original_input"]
            assert report["actual_input"] == previous_ref and report["pure_chunks_equal"] == 126
            for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
                verify_reference(reference)
            text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert counts and int(counts[1]) == report["native"]["assertions"] > 0 and int(counts[2]) == report["native"]["cases"] == 1
            bored = report["bored"]
            assert bored == fields(text, "NORMAL_GROUND_BORED") and bored["model"] == 1
            assert bored["opening_cash"] == previous["cash"] and bored["first_tick"] == previous["tick"]
            assert bored["loan"] == previous["loan"] == 300000
            assert bored["project"] == flow["ready"]["project"] and bored["job"] == flow["ready"]["job"]
            assert bored["cutter"] == flow["ready"]["cutter"] and bored["hauler"] == flow["ready"]["hauler"]
            assert bored["prior_earth"] == prior_earth and bored["prior_rock"] == prior_rock
            ledger = report["ledger"]
            assert ledger == observations(text, "NORMAL_GROUND_BORE_LEDGER") and len(ledger) == 13
            assert {row["category"] for row in ledger} == set(range(13))
            assert bored["cash"] == bored["opening_cash"] - sum(row["cost"] for row in ledger)
            for key, marker in (("traces", "TRACE"), ("sets", "SET"), ("wagons", "WAGON"), ("packets", "PACKET"), ("receivers", "RECEIVER"), ("parked_haulers", "PARKED")):
                assert report[key] == observations(text, f"NORMAL_GROUND_BORE_{marker}")
            parsed_controls = [{key: value if key == "action" else int(value) for key, value in
                                (item.split("=", 1) for item in row.split())}
                               for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
            assert parsed_controls == report["controls"] and all(row["cost"] == 0 for row in parsed_controls)
            controls.extend(parsed_controls); traces.extend(report["traces"]); wagons.extend(report["wagons"]); packets.extend(report["packets"])
            source = verify_reference(report["actual_input"])
            progress, repeat = (verify_reference(report["saves"][key]) for key in ("progress", "repeat"))
            before, after = native_world(source), native_world(progress)
            assert after == native_world(repeat) and all(after[key] == before[key] for key in ("GDEP", "GSPN", "GVNT"))
            old_date, date = saved_table(source, b"DATE")[0], saved_table(progress, b"DATE")[0]
            assert date["id"] == old_date["id"] and date["tick_counter"] == bored["tick"] > bored["first_tick"]
            assert date["pause_mode"] == old_date["pause_mode"]
            company = saved_table(progress, b"PLYR")[0]
            assert company["money"] == bored["cash"] > 0 and company["current_loan"] == 300000
            jobs = saved_table(progress, b"XJOB")
            assert all(next(row for row in jobs.values() if row["id"] == earlier["id"]) == earlier for earlier in earlier_jobs)
            job = next(row for row in jobs.values() if row["id"] == bored["job"])
            assert job["cursor"] == bored["cursor"] and job["routes"] == bored["routes"]
            assert (job["earth"], job["rock"], job["cost"]) == (bored["earth"], bored["rock"], bored["work_cost"])
            previous_ref, previous = report["saves"]["progress"], bored
        final, bored = chain[-1]["actual"], chain[-1]["actual"]["bored"]
        assert bored["complete"] == bored["returned"] == bored["resumed"] == 1
        assert bored["cursor"] == bored["routes"] and bored["exchanges"] >= 2 and bored["release_error"] == 65535
        assert bored["phase"] == 7 and bored["blocked"] == bored["excavating"] == bored["reserved"] == 0
        assert bored["earth"] == bored["earth_received"] == flow["quote"]["earth"] == 36
        assert bored["rock"] == bored["rock_received"] == flow["quote"]["rock"] == 724
        assert bored["earth_processed"] + bored["earth_waiting"] == prior_earth + bored["earth"]
        assert bored["rock_processed"] + bored["rock_waiting"] == prior_rock + bored["rock"]
        assert bored["fees"] - prior_fees == bored["earth"] * 2 + bored["rock"] * 3 == 2244
        assert sum(row["action"] == "partial_return_and_park" for row in controls) == 1
        assert sum(row["action"] == "settled_resume" for row in controls) == 1
        assert sum(row["action"] == "store_retired_hauler" for row in controls) == 1
        assert sum(row["action"] == "start_retired_hauler" for row in controls) == 1
        access = json.loads(verify_reference(basis["access_handoff"]).read_text(encoding="utf-8"))
        old_hauler = access["records"][cfg]["access"]["hauler"]
        assert bored["stored_haulers"] == len(final["parked_haulers"]) == 1
        assert final["parked_haulers"][0] == {"vehicle": old_hauler, "tile": flow["ready"]["depot"], "stored": 1}
        assert any(row["phase"] == 11 and row["cursor"] < row["routes"] for row in traces)
        assert any(row["hauler_phase"] == 4 and row["held"] > 0 for row in traces) and traces[-1]["held"] == 0
        final_tick = max(row["tick"] for row in wagons)
        assert all(row["amount"] == 0 for row in wagons if row["tick"] == final_tick)
        assert len({row["wagon"] for row in wagons}) == 20 and packets and all(row["feeder"] == 0 for row in packets)
        # Read-only address diagnostics establish both physical portal legs;
        # gameplay always uses the native layer helpers,never these wire ranges.
        assert any(row["front"] == bored["cutter"] and row["amount"] > 0 and 49152 <= row["tile"] < 65536 for row in wagons)
        assert any(row["front"] == bored["hauler"] and row["amount"] > 0 and 16384 <= row["tile"] < 32768 for row in wagons)
        assert any(row["front"] == bored["hauler"] and row["amount"] > 0 and row["tile"] < 16384 for row in wagons)
        final_save = verify_reference(previous_ref)
        receivers, industries = saved_table(final_save, b"SPRR"), saved_table(final_save, b"INDY")
        for observed in final["receivers"]:
            receiver = next(row for row in receivers.values() if row["industry"] == observed["industry"])
            assert industries[observed["industry"]]["accepted[0].waiting"] == observed["waiting"] < observed["batch"] == 8
            assert all(receiver[key] == observed[key] for key in ("kind", "disposed", "crushed", "aggregate", "aggregate_made", "residue", "fees"))
            if observed["kind"] == 1:
                assert observed["aggregate_made"] + observed["residue"] == observed["crushed"]
        start_geo, end_geo = geology_snapshot(initial), geology_snapshot(final_save)
        before, after = geological_payload(initial), geological_payload(final_save)
        assert before[:39] == after[:39]
        old_states = [(byte >> shift) & 3 for byte in before[39:] for shift in (0, 2, 4, 6)]
        new_states = [(byte >> shift) & 3 for byte in after[39:] for shift in (0, 2, 4, 6)]
        changes = [index for index, (a, b) in enumerate(zip(old_states, new_states, strict=True)) if a != b]
        assert len(changes) == 36 and all(index >= 16384 and old_states[index] == 1 and new_states[index] == 2 for index in changes)
        assert end_geo["layers"]["underground"] == start_geo["layers"]["underground"]
        assert end_geo["layers"]["deep"]["excavated"] == 36 and bored["excavated"] == 72
        records[cfg] = {"report": identity(path), "records": chain, "settled_input": previous_ref, "closing": bored,
                        "geology": end_geo, "prior_earth": prior_earth, "prior_rock": prior_rock, "prior_fees": prior_fees, "controls": controls}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DB;actual native Deep cutting/two-leg haulage/settlement retains complete Standard history",
              "controller": identity(__file__), "machinery_handoff": identity(handoff), "records": records,
              "classification": "normal_start_paid_deep_work", "bootstrap_claim": False,
              "native_cases": sum(len(row["records"]) for row in records.values()),
              "native_assertions": sum(entry["actual"]["native"]["assertions"] for row in records.values() for entry in row["records"]),
              "pure_pairs": sum(len(row["records"]) for row in records.values()), "complete_chunks_equal": 126, "ignored_chunks": [],
              "remaining": "6DC original irregular useful final use;6DD native files/refusals/recovery;6DE genuine S1/S2 runner provenance."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only complete real Deep cutting/haulage and cumulative material/cash ownership."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
