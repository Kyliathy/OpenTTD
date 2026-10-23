"""Compare exact original ground/utility worlds under actual native viewport selections and a non-GUI driver.

@file ground_performance_views.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDD.
The null renderer measures native simulation/window dispatch;it does not measure raster frames per second.
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
from tube_tbm_files import equal_world

VIEWS = ("headless", "surface", "underground", "hidden")
ORIGINAL = "logs/performance/ground-maps/20261019-native-01/original-performance-verification.json"
ORIGINAL_SHA = "5cd7cfe3987218597147bfdbcadc4a596cde33d28fc279ac96e8f309825063bc"
COMPONENTS = (("logs/performance/ground-maps/20261019-native-01/utility-performance-verification.json", "2bc8deb7260e7ba2963826db28e1f4836e99626dd83ead42a31d04dbe9586d9b"),
              ("logs/performance/ground-maps/20261019-native-01/tube-performance-verification.json", "956f67a0bdbc849f7ef61b495487edf8dc261e08406247f3527fff4319008e9e"))


def selections():
    """Declare all original stages and view roles before observing timings."""
    for cfg in ("Debug", "RelWithDebInfo"):
        for stage in STAGES:
            for view in VIEWS:
                yield cfg, stage, view


def budget_excesses(actual, budget):
    """Retain the exact previously declared native work/command/file/memory budgets."""
    result = []
    if actual["timings"]["simulation_ns"]["p99_ns"] > budget["simulation_p99_ns"]:
        result.append("simulation.p99_ns")
    if actual["timings"]["simulation_ns"]["maximum_ns"] > budget["simulation_max_ns"]:
        result.append("simulation.maximum_ns")
    if actual["peak_commit_bytes"] > budget["peak_commit_bytes"]:
        result.append("peak_commit_bytes")
    if actual["ready"]["startup_to_ready_ns"] > budget["startup_or_load_ns"]:
        result.append("startup_to_ready.ns")
    if any(row["synchronous_ns"] > budget["save_ns"] for row in actual["synchronous_saves"]):
        result.append("save.ns")
    if any(row["synchronous_ns"] > budget["command_p99_ns"] for row in actual["commands"]):
        result.append("command.ns")
    return result


def verify_measurement(actual, baseline, view, selected):
    """Reparse actual full-loop samples and complete native original/next owners without suppressing a view's work."""
    assert actual["view_role"] == view and actual["input"] == selected["original"]
    assert actual["mode"] == "load" and actual["profile"] == "multilayer_industry" and actual["axis"] == 128
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
    observations = parse_fields(text, "GROUND_PERFORMANCE_VIEW")
    expected = {"role": view, "layer": {"headless": 0, "surface": 0, "underground": 1, "hidden": 3}[view],
                "gui_capability": 0 if view == "headless" else 1, "logical_viewport": 1, "raster": 0}
    assert actual["view_observations"] == observations == [expected]
    assert actual["tick_samples"] == parse_fields(text, "GROUND_PERFORMANCE_TICK")
    assert [row["index"] for row in actual["tick_samples"]] == list(range(640))
    assert actual["owners"] == parse_fields(text, "GROUND_PERFORMANCE_COUNTS")
    assert [row["phase"] for row in actual["owners"]] == ["ready", "final"]
    assert actual["owners"][1]["tick"] - actual["owners"][0]["tick"] == 768
    assert all(row["width"] == row["height"] == 128 and row["map_layers"] == 4 for row in actual["owners"])
    assert actual["ready"] == parse_fields(text, "GROUND_PERFORMANCE_READY")[0]
    assert actual["synchronous_saves"] == parse_fields(text, "GROUND_PERFORMANCE_SAVE") and len(actual["synchronous_saves"]) == 2
    assert actual["commands"] == parse_fields(text, "GROUND_PERFORMANCE_COMMAND") and len(actual["commands"]) == 1
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
    saves = {name: verify_reference(ref) for name, ref in actual["saves"].items()}
    for phase in ("initial", "final"):
        equal_world(saves[phase + "-world"], saves[phase + "-repeat"])
    original = verify_reference(actual["input"])
    before, initial = native_world(original), native_world(saves["initial-world"])
    changed = [name for name in before if before[name] != initial[name]]
    if changed:
        assert changed == ["DATE"]
        first_date, initial_date = saved_table(original, b"DATE")[0], saved_table(saves["initial-world"], b"DATE")[0]
        assert {key: (value, initial_date[key]) for key, value in first_date.items() if value != initial_date[key]} == {"pause_mode": (1, 65)}
        positions = [i for i, (a, b) in enumerate(zip(before["DATE"], initial["DATE"])) if a != b]
        assert len(before["DATE"]) == len(initial["DATE"]) and len(positions) == 1
        assert (before["DATE"][positions[0]], initial["DATE"][positions[0]]) == (1, 65)
    return {"initial_changed_owners": changed, "native_initial_linkgraph_pause": bool(changed),
            "budget_excesses": budget_excesses(actual, selected_budget(baseline, actual["configuration"], 128))}


def main():
    """Run every actual original/view pair serially and checkpoint immutable observations."""
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
    expected, done, records = set(selections()), set(), []
    for path in args.prior:
        ref = identity(ROOT / path); actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        cfg, view = actual["configuration"], actual["view_role"]
        stage = next(stage for stage in STAGES if bank["records"][cfg][stage]["original"] == actual["input"])
        key = cfg, stage, view
        assert key in expected and key not in done
        observed = verify_measurement(actual, baseline, view, bank["records"][cfg][stage])
        records.append({"key": list(key), "report": ref, **observed}); done.add(key)
    index = {"controller": identity(__file__), "baseline": baseline_ref, "bank": bank_ref, "records": records, "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for key in selections():
        if key not in done:
            cfg, stage, view = key
            report = run_one(cfg, "multilayer_industry", 128, verify_reference(bank["records"][cfg][stage]["original"]), view=view)
            actual = json.loads(report.read_text(encoding="utf-8"))
            observed = verify_measurement(actual, baseline, view, bank["records"][cfg][stage])
            records.append({"key": list(key), "report": identity(report), **observed})
            target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    assert len(records) == 32
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "cases": 32, "budget_excesses": sum(len(row["budget_excesses"]) for row in records)}), flush=True)


if __name__ == "__main__":
    main()
