"""Measure actual physical tube source,capsule and combined partial-job workloads.

@file tube_performance.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDC.
Prepared connected portals,funding and boundary-loaded packets remain explicit,not bootstrap or useful freight.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import parse_fields
from ground_stress import percentiles, selected_budget
from bounded_performance import observe, verify_observation

TAG = "[.tube-work-performance]"
FIELDS = ("connections", "path_length", "vehicles", "jobs", "motion")
SOURCES = ("src/tests/tube_performance_fixture.h", "src/tests/transport_tube_power.cpp", "src/tests/transport_tube_power_fixture.h",
           "src/tests/transport_tube_fixture.h", "src/tests/ground_job_fixture.h", "src/tests/excavation_fixture.h",
           "src/tests/movement_fixture.h", "src/transport_tube.h", "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp",
           "src/transport_tube_engine.cpp", "src/transport_tube_topology.cpp", "src/transport_tube_model.h", "src/multilayer_bootstrap.cpp",
           "src/utility_power.cpp", "src/utility_power.h", "src/company_cmd.cpp", "src/train_cmd.cpp", "src/vehicle.cpp",
           "src/excavation_jobs.cpp", "src/tests/multilayer/ground_stress.py")


def selectors(connections, path_length, vehicles, jobs, motion):
    """Explicit native workload selection;no count is inferred from a filename."""
    return dict(zip(("OTTD_TUBE_CONNECTIONS", "OTTD_TUBE_PATH", "OTTD_TUBE_VEHICLES", "OTTD_TUBE_JOBS", "OTTD_TUBE_MOTION"),
                    map(str, (connections, path_length, vehicles, jobs, motion))))


def timing_summary(ticks):
    """Keep every real per-tick phase and complete selected native work sum;no interval average becomes a tail."""
    return {op: percentiles([sum(row[field + "_ns"] for field in ("preparation", "work", "jobs")) if op == "combined"
                             else row[op + "_ns"] for row in ticks]) for op in ("preparation", "work", "jobs", "combined")}


def excesses(actual, budget):
    """Use immutable prior512shape budgets and keep each measured excess explicit."""
    result = []
    for op in ("preparation", "work", "jobs", "combined"):
        values = actual["timings"][op]
        if values["p99_ns"] > budget["simulation_p99_ns"]:
            result.append(op + ".p99_ns")
        if values["maximum_ns"] > budget["simulation_max_ns"]:
            result.append(op + ".maximum_ns")
    if actual["complete"][0]["setup_ns"] > budget["startup_or_load_ns"]:
        result.append("setup.ns")
    if actual["peak_commit_bytes"] > budget["peak_commit_bytes"]:
        result.append("peak_commit_bytes")
    return result


def verify_measurement(actual, baseline):
    """Reparse real native ticks,invoices,original cargo assertions and OS peaks against earlier budgets."""
    values = tuple(actual[field] for field in FIELDS)
    connections, path_length, vehicles, jobs, motion = values
    assert 0 <= connections <= 256 and path_length in (2, 64, 128) and connections * path_length <= 16384
    assert vehicles in (0, 32, 256, 4096) and jobs in (0, 128) and motion in (0, 1)
    assert (vehicles == 0 or connections > 0) and (motion == 0 or vehicles == connections)
    text = verify_observation(actual, baseline, TAG, selectors(*values))
    assert actual["native"]["cases"] == 1 and actual["bootstrap_claim"] is False
    assert actual["classification"] == "prepared_physical_tube_performance"
    complete, intervals, ticks = (parse_fields(text, "TUBE_WORK_" + marker) for marker in ("COMPLETE", "SAMPLE", "TICK"))
    assert complete == actual["complete"] and len(complete) == 1 and intervals == actual["intervals"] and ticks == actual["ticks"]
    rounds = 64 if motion else 1
    assert [row["index"] for row in intervals] == list(range(5))
    assert [(row["index"], row["round"]) for row in ticks] == [(sample, tick) for sample in range(5) for tick in range(rounds)]
    end = complete[0]
    assert tuple(end[field] for field in FIELDS) == values
    assert end["width"] == 512 and end["path_cells"] == connections * path_length and end["warmup_ticks"] == rounds
    assert end["measured_ticks"] == rounds * 5 and end["samples"] == 5 and end["exact"] == 1 and end["rendering"] == 0
    assert end["generated_j"] == connections * rounds * 6 * 2700
    assert 0 <= end["consumed_j"] <= end["generated_j"]
    assert end["consumed_j"] == end["usage_cost"] * 1000 + end["remainder_j"]
    assert end["invoiced"] == connections * 20 + end["usage_cost"]
    assert end["remainder_j"] < max(1, connections * 1000)
    for interval in intervals:
        selected = [row for row in ticks if row["index"] == interval["index"]]
        assert interval["ticks"] == rounds
        for field in ("preparation_ns", "work_ns", "jobs_ns", "moved", "serviced", "waiting", "checks_ns"):
            assert interval[field] == sum(row[field] for row in selected)
    if motion:
        assert end["moved"] > 0 and end["serviced"] == end["waiting"] == 0 and end["consumed_j"] > 0
        assert all(row["moved"] <= vehicles and row["serviced"] == row["waiting"] == 0 for row in ticks)
    else:
        per_tick = sum(min(5, vehicles // connections + (index < vehicles % connections)) for index in range(connections)) if connections else 0
        assert end["serviced"] == per_tick * 6 and end["waiting"] == (vehicles - per_tick) * 6 and end["moved"] == 0
        assert end["consumed_j"] == end["serviced"] * 540
        assert all(row["serviced"] == per_tick and row["waiting"] == vehicles - per_tick and row["moved"] == 0 for row in ticks)
    assert actual["timings"] == timing_summary(ticks)
    budget = selected_budget(baseline, actual["configuration"], 512)
    assert actual["budget"] == budget and actual["budget_excesses"] == excesses(actual, budget)
    return end


def run_one(configuration, connections, path_length, vehicles, jobs, motion):
    """Run and retain exactly one real physical-owner selection under the common process guard."""
    context, result, baseline = observe(configuration, TAG, selectors(connections, path_length, vehicles, jobs, motion),
                                        SOURCES, "tube-work", __file__)
    result.update(zip(FIELDS, (connections, path_length, vehicles, jobs, motion)))
    result.update(_documentation="r2mlvs16-integrated-acceptance-and-balance.md21DDC;actual prepared physical native work,not bootstrap/useful freight",
                  classification="prepared_physical_tube_performance", bootstrap_claim=False,
                  preparation="restored fixture floor/Connected portal pairs;funded real kernel capsule purchases;one original boundary-loaded packet each;direct native depot/track construction")
    if result["failure"] is None:
        text = verify_reference(result["native"]["log"]).read_text(encoding="utf-8")
        result["complete"], result["intervals"], result["ticks"] = (parse_fields(text, "TUBE_WORK_" + marker) for marker in ("COMPLETE", "SAMPLE", "TICK"))
        result["timings"] = timing_summary(result["ticks"])
        result["budget"] = selected_budget(baseline, configuration, 512)
        result["budget_excesses"] = excesses(result, result["budget"])
        verify_measurement(result, baseline)
    target = context.output("reports", f"tube-{configuration}-{connections}-{path_length}-{vehicles}-{jobs}-{motion}.json")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "status": result["status"], "failure": result["failure"],
                      "excesses": result.get("budget_excesses"), "timings": result.get("timings")}), flush=True)
    if result["failure"]:
        raise RuntimeError(result["failure"] + ";retained=" + str(target))
    return target


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", required=True, choices=("Debug", "RelWithDebInfo"))
    for field in FIELDS:
        parser.add_argument("--" + field.replace("_", "-"), type=int, required=True)
    args = parser.parse_args()
    run_one(args.configuration, *(getattr(args, field) for field in FIELDS))
