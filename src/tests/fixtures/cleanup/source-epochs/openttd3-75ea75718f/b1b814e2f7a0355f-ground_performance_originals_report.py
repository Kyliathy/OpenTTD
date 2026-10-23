"""Seal real original-world performance with native cash,material,packet and job continuity.

@file ground_performance_originals_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DB;master11.6.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Consumes the existing full native observer and earlier frozen performance budgets.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import parse_fields, summarize
from ground_performance_originals import BASELINE_SHA, BANK_SHA, STAGES
from normal_ground_start_report import native_world
from save_tables import saved_table
from tube_tbm_files import equal_world

TRAIN = "train[0].common[0]."


def material_snapshot(path):
    """Reconcile actual native spoil packets,industry stock and processed output with every paid job."""
    jobs, receivers, industries = (saved_table(path, name) for name in (b"XJOB", b"SPRR", b"INDY"))
    trains, packets, stations = (saved_table(path, name) for name in (b"VEHS", b"CAPA", b"STNN"))
    result, seen_packets = {}, set()
    for kind, field, engine in ((0, "earth", 258), (1, "rock", 259)):
        receiver = next(row for row in receivers.values() if row["kind"] == kind)
        cargo = industries[receiver["industry"]]["accepted[0].cargo"]
        held = 0
        for vehicle in trains.values():
            if vehicle.get(TRAIN + "cargo_type") != cargo:
                continue
            references = vehicle[TRAIN + "cargo.packets"]
            if references:
                assert vehicle[TRAIN + "engine_type"] == engine
            total = 0
            for reference in references:
                packet_id = reference - 1
                assert packet_id not in seen_packets and packet_id in packets
                seen_packets.add(packet_id)
                packet = packets[packet_id]
                assert packet["count"] > 0 and packet["feeder_share"] == 0
                total += packet["count"]
            assert total == sum(vehicle[TRAIN + "cargo.action_counts"])
            held += total
        # These exact normal worlds have direct intake;no unobserved station
        # packet stock may be silently omitted from the conservation equation.
        for station in stations.values():
            if "normal[0].goods.count" not in station:
                continue
            prefix = f"normal[0].goods[{cargo}].cargo."
            assert station[prefix + "count"] == station[prefix + "reserved_count"] == 0
        waiting = industries[receiver["industry"]]["accepted[0].waiting"]
        processed = receiver["disposed" if kind == 0 else "crushed"]
        made = sum(job[field] for job in jobs.values())
        assert made == held + waiting + processed
        if kind == 1:
            assert receiver["crushed"] == receiver["aggregate_made"] + receiver["residue"]
        result[field] = {"made_t": made, "held_t": held, "waiting_t": waiting, "processed_t": processed,
                         "fees": receiver["fees"], "aggregate_made_t": receiver["aggregate_made"], "residue_t": receiver["residue"]}
    return result


def accept(selection, output):
    """Require all eight actual original-world observations,physical continuity and fixed-budget passes."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA and index["bank"]["sha256"] == BANK_SHA
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    bank = json.loads(verify_reference(index["bank"]).read_text(encoding="utf-8"))
    expected = {(cfg, stage) for cfg in ("Debug", "RelWithDebInfo") for stage in STAGES}
    records, assertions, comparisons = {}, 0, 0
    for row in index["records"]:
        cfg, stage = key = (row["configuration"], row["stage"])
        assert key in expected and key not in records and row["selected"] == bank["records"][cfg][stage]
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert actual["input"] == row["selected"]["original"] and actual["mode"] == "load" and actual["axis"] == 128
        assert actual["status"] == "observed" and actual["children_reaped"] is True and actual["bootstrap_claim"] is False
        assert actual["configuration"] == cfg and actual["profile"] == "multilayer_industry" and actual["protocol"] == baseline["protocol"]
        for reference in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
            verify_reference(reference)
        native = actual["native"]
        assert native["exit_status"] == 0 and native["cases"] == 1
        assert native["argv"] == [str(verify_reference(actual["binary"])), "[.ground-performance-map]"] and native["cwd"] == "build"
        text = verify_reference(native["log"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == native["assertions"] > 0 and int(counts[2]) == 1
        assert actual["tick_samples"] == parse_fields(text, "GROUND_PERFORMANCE_TICK")
        assert [tick["index"] for tick in actual["tick_samples"]] == list(range(640))
        assert actual["owners"] == parse_fields(text, "GROUND_PERFORMANCE_COUNTS")
        assert actual["synchronous_saves"] == parse_fields(text, "GROUND_PERFORMANCE_SAVE")
        assert actual["ready"] == parse_fields(text, "GROUND_PERFORMANCE_READY")[0]
        assert actual["owners"][1]["tick"] - actual["owners"][0]["tick"] == 768
        for field in ("simulation_ns", "input_ns", "windows_ns"):
            assert actual["timings"][field] == summarize([r[field] for r in actual["tick_samples"]])
        saves = {name: verify_reference(ref) for name, ref in actual["saves"].items()}
        original = verify_reference(actual["input"])
        for phase in ("initial", "final"):
            equal_world(saves[phase + "-world"], saves[phase + "-repeat"])
            comparisons += 1
        before, initial, final = (native_world(p) for p in (original, saves["initial-world"], saves["final-world"]))
        changed = [name for name in before if before[name] != initial[name]]
        transient_pause = False
        if changed:
            assert changed == ["DATE"]
            first_date, initial_date = saved_table(original, b"DATE")[0], saved_table(saves["initial-world"], b"DATE")[0]
            assert {k: (v, initial_date[k]) for k, v in first_date.items() if v != initial_date[k]} == {"pause_mode": (1, 65)}
            positions = [i for i, (a, b) in enumerate(zip(before["DATE"], initial["DATE"])) if a != b]
            assert len(before["DATE"]) == len(initial["DATE"]) and len(positions) == 1
            assert (before["DATE"][positions[0]], initial["DATE"][positions[0]]) == (1, 65)
            transient_pause = True
        assert all(before[name] == final[name] for name in ("GDEP", "GSPN", "GVNT", "NGRF", "PATS"))
        old_date, date = saved_table(saves["initial-world"], b"DATE")[0], saved_table(saves["final-world"], b"DATE")[0]
        assert old_date["id"] == date["id"] and date["tick_counter"] - old_date["tick_counter"] == 768
        old_company, company = saved_table(saves["initial-world"], b"PLYR")[0], saved_table(saves["final-world"], b"PLYR")[0]
        assert old_company["current_loan"] == company["current_loan"] == 300000
        assert old_company["yearly_expenses"][13:] == company["yearly_expenses"][13:]
        expense = [b - a for a, b in zip(old_company["yearly_expenses"][:13], company["yearly_expenses"][:13])]
        assert company["money"] - old_company["money"] == -sum(expense)
        material = {phase: material_snapshot(path) for phase, path in (("initial", saves["initial-world"]), ("final", saves["final-world"]))}
        old_jobs, jobs = saved_table(saves["initial-world"], b"XJOB"), saved_table(saves["final-world"], b"XJOB")
        assert set(old_jobs) == set(jobs)
        for identity_key, job in jobs.items():
            old = old_jobs[identity_key]
            assert all(job[name] == old[name] for name in ("id", "company", "model", "anchor", "routes", "requested", "authorized"))
            assert all(job[name] >= old[name] for name in ("cursor", "earth", "rock", "cost"))
        if stage == "working-bore":
            assert jobs[0]["cursor"] > old_jobs[0]["cursor"] and material["final"]["earth"]["made_t"] > material["initial"]["earth"]["made_t"]
        elif stage == "loaded-spoil":
            assert sum(v["held_t"] for v in material["initial"].values()) == 162
            assert material["final"]["earth"]["processed_t"] > material["initial"]["earth"]["processed_t"]
            assert material["final"]["rock"]["processed_t"] > material["initial"]["rock"]["processed_t"]
        elif stage == "loaded-useful":
            assert all(initial[name] != final[name] for name in ("TUVD", "TUPD", "UPLD"))
            def capsule_poses(path):
                """Read the actual native train bridge's physical pose;the TUVD owner retains tube cargo."""
                return {key: tuple(vehicle[TRAIN + field] for field in ("tile", "x_pos", "y_pos"))
                        for key, vehicle in saved_table(path, b"VEHS").items() if vehicle.get(TRAIN + "engine_type") == 266}
            first_pose, last_pose = capsule_poses(saves["initial-world"]), capsule_poses(saves["final-world"])
            assert first_pose and first_pose.keys() == last_pose.keys() and first_pose != last_pose
        memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
        samples = memory["samples"]
        assert actual["peak_commit_bytes"] == max(r["process"]["lifetime_peak_commit_bytes"] for r in samples)
        protocol = json.loads(verify_reference(actual["protocol"]).read_text(encoding="utf-8"))
        assert all(r["system"]["physical_available"] >= protocol["minimum_physical_available_bytes"] and
                   r["system"]["commit_available"] >= protocol["minimum_commit_available_bytes"] for r in samples)
        budget = next(r for r in baseline["budgets"] if r["configuration"] == cfg and r["axis"] == 128)
        assert row["budget"] == budget
        assert actual["timings"]["simulation_ns"]["p99_ns"] <= budget["simulation_p99_ns"]
        assert actual["timings"]["simulation_ns"]["maximum_ns"] <= budget["simulation_max_ns"]
        assert actual["peak_commit_bytes"] <= budget["peak_commit_bytes"]
        assert all(r["synchronous_ns"] <= budget["save_ns"] for r in actual["synchronous_saves"])
        assert actual["ready"]["startup_to_ready_ns"] <= budget["startup_or_load_ns"]
        records[key] = {"report": row["report"], "actual": actual, "budget": budget, "material": material,
                        "expense_deltas": expense, "cash_delta": company["money"] - old_company["money"], "jobs_initial": old_jobs, "jobs_final": jobs,
                        "native_initial_linkgraph_pause": transient_pause, "initial_changed_owners": changed,
                        "scope": "completed useful idle control" if stage == "final-useful" else "actual active original"}
        assertions += native["assertions"]
    assert set(records) == expected
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DB;exact original active/full-loop performance and conserved owners",
              "classification": "ground_original_performance_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "bank": index["bank"],
              "records": [{"configuration": cfg, "stage": stage, **row} for (cfg, stage), row in records.items()],
              "native_cases": 8, "native_assertions": assertions, "measured_ticks": 5120, "complete126_chunk_pure_pairs": comparisons,
              "remaining": "21DCdense job/route/command stress;21DDutility/tube/view scaling;21Efile/real-join budgets;21Fattribution/master11.6"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": 8, "native_assertions": assertions, "measured_ticks": 5120}), flush=True)


def main():
    """Seal only the explicit complete original-world selection against earlier fixed budgets."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
