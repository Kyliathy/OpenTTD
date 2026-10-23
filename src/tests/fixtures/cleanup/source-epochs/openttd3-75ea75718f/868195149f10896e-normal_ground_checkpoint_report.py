"""Seal actual complete-frame normal-world cutting and loaded-spoil originals.

@file normal_ground_checkpoint_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The existing paid-work sealer still owns whole material/cash/withdrawal validation.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from normal_ground_boring_report import accept as accept_work
from tube_tbm_files import equal_world
from save_tables import saved_table

TRAIN = "train[0].common[0]."


def accept(reports, output):
    """Bind real saved active/loaded owners to exact readiness and complete native settlement."""
    target = ROOT / output
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    settlement = target.with_name(target.stem + "-settlement.json")
    full = accept_work(reports, settlement.relative_to(ROOT).as_posix())
    records = {}
    for cfg, record in full["records"].items():
        assert len(record["records"]) == 1
        actual = record["records"][0]["actual"]
        assert actual["children_reaped"] and actual["bored"]["complete"] == 1
        observations = {row["label"]: row for row in actual["checkpoint_observations"]}
        assert len(observations) == 4
        raw = verify_reference(actual["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        assert len(re.findall(r"NORMAL_GROUND_CHECKPOINT label=", raw)) == 4
        original = verify_reference(actual["actual_input"])
        original_id = saved_table(original, b"DATE")[0]["id"]
        stages = {}
        for label, refs in actual["checkpoints"].items():
            path, repeat = (verify_reference(refs[key]) for key in ("original", "repeat"))
            equal_world(path, repeat)
            row = observations[label]
            date, company = saved_table(path, b"DATE")[0], saved_table(path, b"PLYR")[0]
            assert date["id"] == original_id and date["tick_counter"] == row["tick"]
            assert date["pause_mode"] == saved_table(original, b"DATE")[0]["pause_mode"]
            assert company["money"] == row["cash"] and company["current_loan"] == row["loan"] == 300000
            job = saved_table(path, b"XJOB")[0]
            assert (job["cursor"], job["routes"], job["earth"], job["rock"], job["cost"]) == (
                row["cursor"], row["routes"], row["earth"], row["rock"], row["work_cost"])
            trains = saved_table(path, b"VEHS")
            cutter = actual["bored"]["cutter"]
            hauler = actual["bored"]["hauler"]
            def held(front):
                """Follow actual native plus-one linked references and sum physical saved cargo."""
                amount, seen = 0, set()
                while front is not None:
                    assert front not in seen
                    seen.add(front)
                    part = trains[front]
                    amount += sum(part[TRAIN + "cargo.action_counts"])
                    next_ref = part[TRAIN + "next"]
                    front = next_ref - 1 if next_ref else None
                return amount
            assert held(cutter) == row["cutter_held"] and held(hauler) == row["hauler_held"]
            if label == "working":
                assert 0 < row["cursor"] < row["routes"] and row["cutter_held"] > 0
            if label == "loaded":
                assert row["hauler_held"] > 0 and row["hauler_tile"] < 16384
            stages[label] = {**refs, "observed": row, "complete_equal_chunks": 126}
        for label in ("working", "loaded"):
            assert observations[label + "-next"]["tick"] == observations[label]["tick"] + 1
        records[cfg] = {"report": record["report"], "original_input": actual["original_input"],
                        "ready_input": actual["actual_input"], "config": actual["config"], "checkpoints": stages,
                        "settled_input": record["settled_input"], "closing": record["closing"]}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDB;actual full-frame working/loaded normal-world originals and untouched native next opportunities",
              "classification": "normal_start_working_files", "bootstrap_claim": False,
              "settlement": identity(settlement), "controller": identity(__file__), "records": records,
              "native_cases": full["native_cases"], "native_assertions": full["native_assertions"],
              "checkpoint_pairs": 8, "complete_chunks_equal": 126, "ignored_chunks": [],
              "remaining": "6DDC ordinary readers;6DDD actual refusal/recovery;6DE genuine publication"}
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "native_assertions": result["native_assertions"]}), flush=True)


def main():
    """Seal exactly both retained native original checkpoint branches."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
