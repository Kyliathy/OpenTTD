"""Seal real loaded-spoil reload,public recovery and complete native normal-world settlement.

@file normal_ground_recovery_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The input is the actual162-tonne original checkpoint,not a reconstructed loaded fixture.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from normal_ground_boring import WORKING_FILES, WORKING_FILES_SHA, fields, observations
from normal_ground_start_report import native_world
from save_tables import saved_table

TRAIN = "train[0].common[0]."


def accept(reports, output):
    """Reconcile original loaded material,remaining work,cash and physically emptied participants."""
    handoff = identity(ROOT / WORKING_FILES)
    assert handoff["sha256"] == WORKING_FILES_SHA
    basis = json.loads(verify_reference(handoff).read_text(encoding="utf-8"))
    files = identity(ROOT / "logs/acceptance/bootstrap/20261019-native-01/ordinary-files-verification.json")
    assert files["sha256"] == "4fc54a2f9e6312cc434df0a691bc23cc12580abdd2c67bba40dd660e8cec3d6e"
    records = {}
    for name in reports:
        path = ROOT / name
        actual = json.loads(path.read_text(encoding="utf-8"))
        cfg = actual["configuration"]
        assert cfg in basis["records"] and cfg not in records
        selected = basis["records"][cfg]
        opening = selected["checkpoints"]["loaded"]["observed"]
        assert actual["classification"] == "normal_start_loaded_recovery" and actual["bootstrap_claim"] is False
        assert actual["checkpoint_recovery"] == handoff and actual["checkpoint_opening"] == opening
        assert actual["actual_input"] == selected["checkpoints"]["loaded"]["original"]
        assert actual["original_input"] == selected["original_input"] and actual["predecessor"] is None
        assert actual["children_reaped"] and actual["pure_chunks_equal"] == 126
        for ref in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
            verify_reference(ref)
        text = verify_reference(actual["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == actual["native"]["assertions"] > 0 and int(counts[2]) == actual["native"]["cases"] == 1
        closed = fields(text, "NORMAL_GROUND_BORED")
        assert closed == actual["bored"]
        assert closed["first_tick"] == opening["tick"] and closed["opening_cash"] == opening["cash"]
        assert (closed["first_cursor"], closed["first_earth"], closed["first_rock"]) == (3, 54, 108)
        assert closed["complete"] == closed["returned"] == closed["resumed"] == 1
        assert closed["cursor"] == closed["routes"] == 9 and closed["exchanges"] >= 2
        assert closed["phase"] == 7 and closed["blocked"] == 0 and closed["release_error"] == 65535
        assert closed["earth"] == closed["earth_received"] == 108 and closed["rock"] == closed["rock_received"] == 216
        assert closed["work_cost"] == 1080 and closed["fees"] == 864 and closed["excavated"] == 36
        assert closed["earth_processed"] == 104 and closed["earth_waiting"] == 4
        assert closed["rock_processed"] == 216 and closed["rock_waiting"] == 0
        ledger = observations(text, "NORMAL_GROUND_BORE_LEDGER")
        assert ledger == actual["ledger"] and len(ledger) == 13 and {r["category"] for r in ledger} == set(range(13))
        assert closed["cash"] == opening["cash"] - sum(r["cost"] for r in ledger) > 0
        assert [r["action"] for r in actual["controls"]] == ["settled_resume"] and actual["controls"][0]["cost"] == 0
        assert actual["traces"][0]["held"] == 162 and actual["traces"][-1]["held"] == 0
        assert actual["packets"] and all(r["feeder"] == 0 and 16384 <= r["source"] < 32768 for r in actual["packets"])
        source = verify_reference(actual["actual_input"])
        progress, repeat = (verify_reference(actual["saves"][k]) for k in ("progress", "repeat"))
        before, after = native_world(source), native_world(progress)
        assert after == native_world(repeat)
        assert all(before[k] == after[k] for k in ("GDEP", "GSPN", "GVNT", "PATS", "NGRF"))
        assert after["GEOS"] == native_world(verify_reference(selected["settled_input"]))["GEOS"]
        date = saved_table(progress, b"DATE")[0]
        assert date["id"] == saved_table(source, b"DATE")[0]["id"] and date["tick_counter"] == closed["tick"] > opening["tick"]
        assert date["pause_mode"] == saved_table(source, b"DATE")[0]["pause_mode"]
        player = saved_table(progress, b"PLYR")[0]
        assert player["money"] == closed["cash"] and player["current_loan"] == closed["loan"] == 300000
        old_job, job = (saved_table(p, b"XJOB")[0] for p in (source, progress))
        changing = {"state", "reason", "cursor", "partial", "revision", "earth", "rock", "cost"}
        assert {k: v for k, v in old_job.items() if k not in changing} == {k: v for k, v in job.items() if k not in changing}
        assert (job["state"], job["cursor"], job["earth"], job["rock"], job["cost"]) == (4, 9, 108, 216, 1080)
        old_trains, trains = ({k: r for k, r in saved_table(p, b"VEHS").items() if TRAIN + "engine_type" in r} for p in (source, progress))
        assert old_trains.keys() == trains.keys()
        for key, old in old_trains.items():
            assert all(trains[key][TRAIN + field] == old[TRAIN + field] for field in ("owner", "engine_type", "cargo_type", "cargo_cap"))
        spoil = [r for r in trains.values() if r[TRAIN + "engine_type"] in (258, 259)]
        assert len(spoil) == 8 and all(not r[TRAIN + "cargo.packets"] and r[TRAIN + "cargo.action_counts"] == [0] * 4 for r in spoil)
        receivers, industries = saved_table(progress, b"SPRR"), saved_table(progress, b"INDY")
        assert len(receivers) == 2
        for receiver in receivers.values():
            waiting = industries[receiver["industry"]]["accepted[0].waiting"]
            if receiver["kind"] == 0:
                assert receiver["disposed"] == 104 and waiting == 4 and receiver["fees"] == 216
            else:
                assert receiver["crushed"] == 216 and waiting == 0 and receiver["fees"] == 648
                assert receiver["aggregate"] == receiver["aggregate_made"] == 162 and receiver["residue"] == 54
        records[cfg] = {"report": identity(path), "actual": actual, "loaded_input": actual["actual_input"],
                        "settled_input": actual["saves"]["progress"], "closing": closed}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDD;actual loaded-spoil recovery/remaining paid work/disposal/whole empty withdrawal",
              "classification": "normal_start_loaded_recovery", "bootstrap_claim": False, "controller": identity(__file__),
              "working_files": handoff, "ordinary_files": files, "records": records, "native_cases": 2,
              "native_assertions": sum(r["actual"]["native"]["assertions"] for r in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "ignored_chunks": [], "remaining": "6DDD native refusal/final-use safety;6DE genuine publication"}
    target = ROOT / output
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": 2, "native_assertions": result["native_assertions"]}), flush=True)


def main():
    """Seal both exact native loaded original recovery continuations."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
