"""Measure actual admitted files through fresh native reconstruction and independent full-loop outputs.

@file ground_performance_files.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EB.
Ordinary large geometry and actual compact active worlds remain distinct selected workloads.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import parse_fields, run_one, summarize
from ground_performance_originals import BASELINE, BASELINE_SHA, BANK, BANK_SHA, STAGES
from ground_stress import selected_budget
from normal_ground_start_report import native_world
from save_tables import saved_table


def inputs(baseline, bank):
    """Select exact completed sizes and physical originals;never turn declined4096maps into admitted files."""
    result = {}
    for cfg in ("Debug", "RelWithDebInfo"):
        candidates = [row for row in baseline["records"] if row["actual"]["configuration"] == cfg and row["actual"]["profile"] == "multilayer_industry"]
        maximum = max(row["actual"]["axis"] for row in candidates)
        assert next(row["axis"] for row in baseline["largest_completed_under_policy"] if row["configuration"] == cfg and row["profile"] == "multilayer_industry") == maximum
        for axis in (128, 512, maximum):
            producer = next(row for row in candidates if row["actual"]["axis"] == axis)
            result[cfg, f"ordinary-{axis}"] = {"kind": "ordinary_generated_geometry", "axis": axis,
                                               "input": producer["actual"]["saves"]["final-world"], "producer": producer["report"],
                                               "maximum_completed_under_policy": axis == maximum}
        for stage in STAGES:
            selected = bank["records"][cfg][stage]
            result[cfg, "active-" + stage] = {"kind": "original_physical_work" if stage != "final-useful" else "original_completed_idle_control",
                                             "axis": 128, "input": selected["original"], "producer": identity(ROOT / BANK),
                                             "stage": stage, "maximum_completed_under_policy": False}
    assert len(result) == 14
    return result


def excesses(actual, budget):
    """Native startup/fixup/first work,saves,ticks,commands and OS memory retain prior thresholds."""
    result = []
    if actual["first_resumed_work"][0]["load_and_first_work_ns"] > budget["startup_or_load_ns"]:
        result.append("load_and_first_work.ns")
    if any(row["synchronous_ns"] > budget["save_ns"] for row in actual["synchronous_saves"]):
        result.append("save.ns")
    if actual["timings"]["simulation_ns"]["p99_ns"] > budget["simulation_p99_ns"]:
        result.append("simulation.p99_ns")
    if actual["timings"]["simulation_ns"]["maximum_ns"] > budget["simulation_max_ns"]:
        result.append("simulation.maximum_ns")
    if any(row["synchronous_ns"] > budget["command_p99_ns"] for row in actual["commands"]):
        result.append("command.ns")
    if actual["peak_commit_bytes"] > budget["peak_commit_bytes"]:
        result.append("peak_commit_bytes")
    return result


def verify_measurement(actual, baseline, selected):
    """Reparse actual whole file/native first-work observations and every initial owner before accepting timing."""
    assert actual["input"] == selected["input"] and actual["axis"] == selected["axis"]
    assert actual["mode"] == "load" and actual["profile"] == "multilayer_industry" and actual["view_role"] is None
    assert actual["status"] == "observed" and actual["failure"] is None and actual["children_reaped"] is True and actual["bootstrap_claim"] is False
    assert actual["protocol"] == baseline["protocol"]
    for ref in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
        verify_reference(ref)
    native = actual["native"]
    assert native["argv"] == [str(verify_reference(actual["binary"])), "[.ground-performance-map]"] and native["cwd"] == "build"
    assert native["exit_status"] == 0 and native["cases"] == 1
    text = verify_reference(native["log"]).read_text(encoding="utf-8")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert match and int(match[1]) == native["assertions"] > 0 and int(match[2]) == 1
    for field, marker in (("tick_samples", "TICK"), ("owners", "COUNTS"), ("synchronous_saves", "SAVE"), ("commands", "COMMAND"), ("first_resumed_work", "RESUMED")):
        assert actual[field] == parse_fields(text, "GROUND_PERFORMANCE_" + marker)
    assert [row["index"] for row in actual["tick_samples"]] == list(range(640))
    assert [row["phase"] for row in actual["owners"]] == ["ready", "final"]
    assert actual["owners"][1]["tick"] - actual["owners"][0]["tick"] == 768
    assert all(row["width"] == row["height"] == selected["axis"] and row["map_layers"] == 4 for row in actual["owners"])
    assert actual["ready"] == parse_fields(text, "GROUND_PERFORMANCE_READY")[0]
    assert len(actual["synchronous_saves"]) == 2 and len(actual["commands"]) == len(actual["first_resumed_work"]) == 1
    resumed = actual["first_resumed_work"][0]
    assert resumed["tick"] == actual["owners"][0]["tick"] + 1 and resumed["first_simulation_ns"] > 0
    assert resumed["launch_to_first_tick_ns"] - resumed["initial_observer_ns"] == resumed["load_and_first_work_ns"] >= actual["ready"]["startup_to_ready_ns"]
    assert resumed["initial_observer_ns"] >= actual["synchronous_saves"][0]["synchronous_ns"]
    for field in ("simulation_ns", "input_ns", "windows_ns"):
        assert actual["timings"][field] == summarize([row[field] for row in actual["tick_samples"]])
    memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    assert memory["samples"] and actual["wall_seconds"] < protocol["child_timeout_seconds"]
    assert actual["peak_commit_bytes"] == max(row["process"]["lifetime_peak_commit_bytes"] for row in memory["samples"])
    assert actual["peak_working_bytes"] == max(row["process"]["lifetime_peak_working_bytes"] for row in memory["samples"])
    assert actual["peak_commit_bytes"] <= protocol["maximum_child_private_bytes"]
    for system in [memory["opening"], memory["closing"], *(row["system"] for row in memory["samples"])]:
        assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"]
    for phase in ("initial", "final"):
        first, repeat = (actual["saves"][phase + suffix] for suffix in ("-world", "-repeat"))
        verify_reference(first); verify_reference(repeat)
        assert (first["bytes"], first["sha256"]) == (repeat["bytes"], repeat["sha256"])
    before, initial = (native_world(verify_reference(ref)) for ref in (actual["input"], actual["saves"]["initial-world"]))
    changed = [name for name in before if before[name] != initial[name]]
    if changed:
        assert changed == ["DATE"]
        original_date, initial_date = (saved_table(verify_reference(ref), b"DATE")[0] for ref in (actual["input"], actual["saves"]["initial-world"]))
        assert {key: (value, initial_date[key]) for key, value in original_date.items() if value != initial_date[key]} == {"pause_mode": (1, 65)}
        positions = [i for i, (a, b) in enumerate(zip(before["DATE"], initial["DATE"])) if a != b]
        assert len(before["DATE"]) == len(initial["DATE"]) and len(positions) == 1
        assert (before["DATE"][positions[0]], initial["DATE"][positions[0]]) == (1, 65)
    return {"initial_changed_owners": changed, "native_initial_linkgraph_pause": bool(changed),
            "budget_excesses": excesses(actual, selected_budget(baseline, actual["configuration"], selected["axis"]))}


def main():
    """Run two independent fresh native loads per exact file and retain every completed/refused observation."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    baseline_ref, bank_ref = identity(ROOT / BASELINE), identity(ROOT / BANK)
    assert baseline_ref["sha256"] == BASELINE_SHA and bank_ref["sha256"] == BANK_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    bank = json.loads(verify_reference(bank_ref).read_text(encoding="utf-8"))
    selected_inputs = inputs(baseline, bank)
    expected = {(cfg, label, attempt) for cfg, label in selected_inputs for attempt in (0, 1)}
    done, records = set(), []
    for path in args.prior:
        ref = identity(ROOT / path); actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        cfg = actual["configuration"]
        label = next(label for config, label in selected_inputs if config == cfg and selected_inputs[config, label]["input"] == actual["input"])
        key = cfg, label, 0
        assert key in expected and key not in done
        observed = verify_measurement(actual, baseline, selected_inputs[cfg, label])
        records.append({"key": list(key), "report": ref, **observed}); done.add(key)
    index = {"controller": identity(__file__), "baseline": baseline_ref, "bank": bank_ref, "records": records, "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for cfg, label, attempt in sorted(expected):
        if (cfg, label, attempt) not in done:
            selected = selected_inputs[cfg, label]
            report = run_one(cfg, "multilayer_industry", selected["axis"], verify_reference(selected["input"]))
            actual = json.loads(report.read_text(encoding="utf-8"))
            observed = verify_measurement(actual, baseline, selected)
            records.append({"key": [cfg, label, attempt], "report": identity(report), **observed})
            target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    assert len(records) == 28
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "cases": 28, "budget_excesses": sum(len(row["budget_excesses"]) for row in records)}), flush=True)


if __name__ == "__main__":
    main()
