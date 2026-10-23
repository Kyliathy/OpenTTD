"""Bind exact genuine normal-start predecessors for the existing acceptance runner.

@file normal_ground_bootstrap_proof.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DEB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This artifact is a checked prerequisite;only acceptance/native_ground.py publishes scenarios.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from normal_ground_file_bank import RECEIPTS as PHYSICAL_RECEIPTS
from normal_ground_start_report import native_world
from save_tables import saved_table, geology_snapshot

HOME = "logs/acceptance/bootstrap/20261019-native-01/"
PINS = dict(PHYSICAL_RECEIPTS, **{
    "financing": "a090e3a26566eeb305108aa551b6eac41b71f26efeef8fe73ec1c9844293e932",
    "surface-yard": "6cd491b51db7a70138868f37082b49e2fe10a67f319d3b6c97540757765631f2",
    "ordinary-access": "4cc933379184d4502af1bd7f85a84d1fa8b798711fe1564f427f746afb9a4b98",
    "working-files": "deabd77e157d3b4e9656550137972cbe47bd4d109dbd4f0c653edf550a872ee8",
    "ordinary-files": "4fc54a2f9e6312cc434df0a691bc23cc12580abdd2c67bba40dd660e8cec3d6e",
    "loaded-recovery": "58cb946c70cbbab888d4ca19f1e704a3c22a903bccfc8b49884f167657d38532",
    "original-recovery": "520a5d841c6e57f1d81f3cea417924dbdda3dc792c818f2e8db48b4bbce90d9a",
})
ORDER = ("normal-start", "financing", "surface-yard", "standard-machinery", "standard-work", "ordinary-access",
         "deep-machinery", "deep-work", "irregular-readiness", "irregular-work", "modal-layout", "useful-supply", "useful-operation")
TRAIN = "train[0].common[0]."


def collect_proof():
    """Verify exact immutable physical edges,actual native artifacts and conserved endpoint owners."""
    checked, dated, production = {}, {}, {}
    def verify(ref):
        """Verify an actual retained artifact once,refusing repository escapes or changed bytes."""
        path = (ROOT / ref["path"]).resolve()
        assert path.is_relative_to(ROOT) and path.is_file()
        key = (ref["path"], ref["bytes"], ref["sha256"])
        if key not in checked:
            assert identity(path) == ref, f"Changed retained artifact: {path}"
            checked[key] = ref
        return path
    def artifact_bindings(value):
        """Retain dated overwritten implementations;verify actual evidence and unchanged production owners."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256", "bytes"}:
                path = value["path"].replace("\\", "/")
                old_test = path.startswith("src/tests/") and path.endswith((".cpp", ".h", ".py", "/CMakeLists.txt"))
                old_binary = path.startswith("build/") and path.endswith(".exe")
                if old_test or old_binary:
                    dated[(path, value["sha256"])] = value
                else:
                    verify(value)
                    if path.startswith("src/"):
                        assert path not in production or production[path] == value
                        production[path] = value
                return
            for item in value.values():
                artifact_bindings(item)
        elif isinstance(value, list):
            for item in value:
                artifact_bindings(item)
    receipts, refs = {}, {}
    for name, digest in PINS.items():
        path = ROOT / (HOME + name + "-verification.json")
        ref = identity(path)
        assert ref["sha256"] == digest
        refs[name] = ref
        receipts[name] = json.loads(verify(ref).read_text(encoding="utf-8"))
        # The complete file/refusal/recovery receipts also bind their branch
        # reports,logs,configs and saves. Verify those retained bytes now,
        # while preserving dated overwritten test/binary implementations.
        artifact_bindings(receipts[name])
    records = {}
    for cfg in ("Debug", "RelWithDebInfo"):
        previous, original, original_id = None, None, None
        steps, assertions, native_cases = [], 0, 0
        for name in ORDER:
            record = receipts[name]["records"][cfg]
            entries = record["records"] if "records" in record else [{"report": record["report"], "actual": record["actual"]}]
            for entry in entries:
                actual = entry["actual"]
                assert json.loads(verify(entry["report"]).read_text(encoding="utf-8")) == actual
                assert actual["configuration"] == cfg and actual["classification"].startswith("normal_start_")
                assert actual["bootstrap_claim"] is False and actual["children_reaped"]
                assert actual.get("pure_chunks_equal", 126 if name == "normal-start" else None) == 126
                artifact_bindings(actual)
                text = verify(actual["native"]["log"]).read_text(encoding="utf-8", errors="replace")
                counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
                assert counts and int(counts[1]) == actual["native"]["assertions"] > 0 and int(counts[2]) == actual["native"]["cases"] == 1
                outputs = [ref for key, ref in actual["saves"].items() if "repeat" not in key]
                repeats = [ref for key, ref in actual["saves"].items() if "repeat" in key]
                assert len(outputs) == len(repeats) == 1
                output = outputs[0]
                after = native_world(verify(output))
                assert after == native_world(verify(repeats[0]))
                date, player = saved_table(verify(output), b"DATE")[0], saved_table(verify(output), b"PLYR")[0]
                source = None
                if name == "normal-start":
                    assert previous is None
                    original, original_id = output, date["id"]
                    owners = actual["starting_owners"]
                    assert (owners["seed"], owners["width"], owners["height"], owners["year"]) == (420, 128, 128, 1980)
                    assert player["money"] == player["current_loan"] == 100000 and owners["max_loan"] == 300000
                    assert owners["opened"] == owners["fleet"] == 0 and owners["solid"] == 31752
                    opening_cash, opening_loan, opening_tick = player["money"], player["current_loan"], date["tick_counter"]
                else:
                    source = actual.get("actual_input", actual.get("original_input"))
                    assert source == previous, f"Broken physical predecessor: {cfg}/{name}"
                    before_path = verify(source)
                    before_date, before_player = saved_table(before_path, b"DATE")[0], saved_table(before_path, b"PLYR")[0]
                    assert before_date["id"] == original_id
                    opening_cash, opening_loan, opening_tick = before_player["money"], before_player["current_loan"], before_date["tick_counter"]
                    before_world = native_world(before_path)
                    assert all(after[key] == before_world[key] for key in ("GDEP", "GSPN", "GVNT"))
                assert date["id"] == original_id and date["tick_counter"] >= opening_tick
                loan_delta = player["current_loan"] - opening_loan
                net_expense = opening_cash + loan_delta - player["money"]
                steps.append({"phase": name, "report": entry["report"], "input": source, "output": output,
                              "tick_before": opening_tick, "tick_after": date["tick_counter"],
                              "cash_before": opening_cash, "cash_after": player["money"], "loan_before": opening_loan,
                              "loan_after": player["current_loan"], "loan_delta": loan_delta, "net_native_expense": net_expense,
                              "native": actual["native"], "cost_owners": {key: actual[key] for key in (
                                  "invoices", "ledger", "capital_paid", "construction_expense", "parking_cash_delta") if key in actual}})
                previous = output
                assertions += actual["native"]["assertions"]
                native_cases += 1
        final = verify(previous)
        world = native_world(final)
        initial_world = native_world(verify(original))
        assert all(world[key] == initial_world[key] for key in ("GDEP", "GSPN", "GVNT", "PATS", "NGRF"))
        jobs = saved_table(final, b"XJOB")
        assert len(jobs) == 3 and {r["id"] for r in jobs.values()} == {0, 1, 2}
        assert all(r["state"] == 4 and r["cursor"] == r["routes"] for r in jobs.values())
        assert sum(r["earth"] for r in jobs.values()) == 264 and sum(r["rock"] for r in jobs.values()) == 1180
        geology = geology_snapshot(final)
        assert geology["layers"]["underground"]["excavated"] == 76 and geology["layers"]["deep"]["excavated"] == 36
        assert all(geology["layers"][layer]["excavating"] == geology["layers"][layer]["reserved"] == 0 for layer in ("underground", "deep"))
        receivers, industries = saved_table(final, b"SPRR"), saved_table(final, b"INDY")
        assert len(receivers) == 2
        earth = next(r for r in receivers.values() if r["kind"] == 0)
        rock = next(r for r in receivers.values() if r["kind"] == 1)
        assert earth["disposed"] == 264 and industries[earth["industry"]]["accepted[0].waiting"] == 0
        assert rock["crushed"] == 1176 and rock["aggregate_made"] == 882 and rock["residue"] == 294
        assert industries[rock["industry"]]["accepted[0].waiting"] == 4 and earth["fees"] + rock["fees"] == 4068
        trains = {k: r for k, r in saved_table(final, b"VEHS").items() if TRAIN + "engine_type" in r}
        spoil = [r for r in trains.values() if r[TRAIN + "engine_type"] in (258, 259)]
        assert len(spoil) == 20 and all(not r[TRAIN + "cargo.packets"] and r[TRAIN + "cargo.action_counts"] == [0] * 4 for r in spoil)
        useful = receipts["useful-operation"]["records"][cfg]
        assert useful["settled_input"] == previous and useful["delivered"] == 64
        assert useful["actual_loaded_curve"] and useful["both_empty_returns"]
        closing = useful["closing"]
        assert closing["cash"] == steps[-1]["cash_after"] and closing["loan"] == 300000
        assert sum(r["loan_delta"] for r in steps) == 200000
        assert sum(r["net_native_expense"] for r in steps) == 300000 - closing["cash"]
        records[cfg] = {"original_input": original, "final_input": previous, "original_id": original_id, "steps": steps,
                        "native_cases": native_cases, "native_assertions": assertions, "closing": closing,
                        "paid_cells": 112, "earth_t": 264, "rock_t": 1180, "receiver_fees": 4068,
                        "earth_disposed_t": 264, "rock_crushed_t": 1176, "rock_waiting_t": 4,
                        "aggregate_made_t": 882, "residue_t": 294, "retained_empty_spoil_wagons": 20, "useful_coal_delivered_t": 64}
    files, recovery = receipts["ordinary-files"], receipts["original-recovery"]
    assert files["native_cases"] == 192 and files["complete126_chunk_comparisons"] == 744 and files["original_files"] == 24
    assert recovery["receipts"]["ordinary-files"] == refs["ordinary-files"] and recovery["receipts"]["loaded-recovery"] == refs["loaded-recovery"]
    assert recovery["native_cases"] == 4 and recovery["unchanged_original126_chunk_comparisons"] == 12
    return {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DEB;complete exact genuine normal-start physical prerequisite graph,not scenario publication",
            "classification": "normal_start_complete_prerequisites", "bootstrap_claim": False, "controller": identity(__file__),
            "receipts": refs, "records": records, "verified_artifacts": list(checked.values()),
            "dated_implementation_bindings": list(dated.values()), "current_production_sources": list(production.values()),
            "mainline_native_cases": sum(r["native_cases"] for r in records.values()),
            "mainline_native_assertions": sum(r["native_assertions"] for r in records.values()),
            "ordinary_reader_cases": 192, "complete126_chunk_file_comparisons": 744,
            "remaining": "6DEC genuine public S1/S2 runner records and focused provenance/refusal gates"}


def verify_proof_payload(payload):
    """Refuse incomplete,altered or relabelled payload against exact original physical/native authorities."""
    assert payload == collect_proof(), "Normal-start proof differs from its complete exact predecessor graph"
    return payload


def main():
    """Publish a new repository-owned prerequisite artifact after every actual predecessor passes."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = ROOT / args.output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    result = collect_proof()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"proof": identity(target), "mainline_native_cases": result["mainline_native_cases"], "mainline_native_assertions": result["mainline_native_assertions"]}), flush=True)


if __name__ == "__main__":
    main()
