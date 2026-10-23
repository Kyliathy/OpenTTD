"""Verify actual peer reconstruction timing,shared OS reserves and all native joined/next/later owners.

@file network_performance_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EC.
Seven explicit native NoNetworkSync unit preferences are checked against each real accepted source.
"""

import argparse
import copy
import json
import re

from ground_network import ROOT
from runner import create_run_context
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_stress import BASELINE_SHA, selected_budget
from ground_network_trace import LOCAL_UNITS
from save_tables import saved_table
from network_performance import TAG, FILES_SHA, inputs, native_counts, finish_measurement

PREFIX = "logs/performance/ground-maps/20261019-native-01/network-long-prefix-verification.json"
PREFIX_SHA = "f2d17b712b5050ccd80f41c9a5f7973abbf0c10ff149ee95231a7bb1af8e7612"


class NativeFrameFiles:
    """Artifact paths for the delivered native-file comparer;this never creates or restores simulation state."""

    def __init__(self, saves):
        self.saves = saves

    def output(self, family, name):
        """Put an unexpected physical-comparison diagnostic in a new owned review home."""
        return create_run_context("performance", "network-peer-refusals").output(family, name)


def verify_measurement(actual, baseline, selected):
    """Recheck actual pair provenance,marker times,raw Windows peaks and all completed native frame files."""
    assert actual["classification"] == "native_network_reconstruction_performance" and actual["bootstrap_claim"] is False
    assert actual["status"] == "observed" and actual["failure"] is None and actual["children_reaped"] is True
    assert actual["selected"] == selected
    verify_reference(selected["producer"])
    source = verify_reference(selected["input"])
    assert actual["source_counts"] == native_counts(source)
    source_settings = saved_table(source, b"PATS")[0]
    assert actual["source_local_units"] == {name: source_settings[name] for name in LOCAL_UNITS}
    cfg = actual["configuration"]
    assert actual["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
    for ref in (actual["controller"], actual["binary"], *actual["sources"]):
        verify_reference(ref)
    budget = selected_budget(baseline, cfg, max(128, actual["source_counts"]["width"]))
    assert actual["budget"] == budget
    assert set(actual["native"]) == {"server", "client"}
    address = actual["native"]["client"]["environment_selectors"]
    assert set(address) == {"OTTD_GROUND_NETWORK_ROLE", "OTTD_GROUND_NETWORK_ADDRESS"} and address["OTTD_GROUND_NETWORK_ROLE"] == "client"
    match = re.fullmatch(r"127\.0\.0\.1:(\d+)#255", address["OTTD_GROUND_NETWORK_ADDRESS"])
    assert match and 0 < int(match[1]) < 65536
    for role, native in actual["native"].items():
        assert native["argv"] == [str(verify_reference(actual["binary"])), TAG] and native["cwd"] == "build"
        config = verify_reference(native["config"]).read_text(encoding="utf-8")
        assert f"server_port = {match[1]}" in config and "server_bind_addresses = 127.0.0.1" in config
        assert "server_game_type = local" in config and "max_lag_time = 2000" in config and "pause_on_join = true" in config
        verify_reference(native["log"])
        assert native["exit_status"] == 0 and native["cases"] == 1 and native["assertions"] > 0
        for ref in actual["runtime_content"][role]:
            verify_reference(ref)
    assert actual["native"]["server"]["environment_selectors"] == {"OTTD_GROUND_NETWORK_ROLE": "server", "OTTD_GROUND_NETWORK_INPUT": str(source)}
    assert [(r["bytes"], r["sha256"]) for r in actual["runtime_content"]["server"]] == [(r["bytes"], r["sha256"]) for r in actual["runtime_content"]["client"]]
    memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    observations, events = memory["samples"], memory["events"]
    assert observations and [event["event"] for event in events] == ["server_launched", "client_launched", "client_first_work_snapshots_seen"]
    assert [event["seconds"] for event in events] == sorted(event["seconds"] for event in events)
    assert [row["seconds"] for row in observations] == sorted(row["seconds"] for row in observations)
    assert 0 <= events[0]["seconds"] <= events[-1]["seconds"] <= actual["wall_seconds"] < protocol["child_timeout_seconds"]
    for system in [memory["opening"], memory["closing"], *(row["system"] for row in observations)]:
        assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"]
    for row in observations:
        assert set(row["peers"]) in ({"server"}, {"server", "client"})
        assert all(max(peer["private_bytes"], peer["lifetime_peak_commit_bytes"]) <= protocol["maximum_child_private_bytes"] for peer in row["peers"].values())
        if "client" in row["peers"]:
            assert row["seconds"] >= events[1]["seconds"]
    assert len(actual["comparisons"]) == 6
    saves = verify_reference(actual["comparisons"][0]["files"]["server"]).parent
    recomputed = copy.deepcopy(actual)
    finish_measurement(recomputed, NativeFrameFiles(saves), baseline, observations, events)
    for field in ("native", "joins", "states", "comparisons", "joined_counts", "join_upper_bound_ns", "join_timing_policy",
                  "peak_commit_bytes", "peak_working_bytes", "sampled_combined_private_bytes", "combined_lifetime_peak_commit_bound", "budget_excesses",
                  "checkpoint_basis", "checkpoint_offsets", "trailing_ticks"):
        assert actual[field] == recomputed[field], field
    return {"join_upper_bound_ns": actual["join_upper_bound_ns"], "budget_excesses": actual["budget_excesses"]}


def verify_long_prefix(reference, expected):
    """Keep exact sealed old512-frame evidence dated while current tests/binaries gain the bounded observer."""
    assert reference["sha256"] == PREFIX_SHA and reference == identity(ROOT / PREFIX)
    prefix = json.loads(verify_reference(reference).read_text(encoding="utf-8"))
    assert prefix["peer_pairs"] == 6 and prefix["native_cases"] == 12
    verify_reference(prefix["controller"]); verify_reference(prefix["selection"])
    records = {}
    for entry in prefix["records"]:
        key = tuple(entry["key"])
        actual = json.loads(verify_reference(entry["report"]).read_text(encoding="utf-8"))
        assert actual == entry["actual"] and key in expected and key not in records and actual["selected"] == expected[key]
        assert actual["budget_excesses"] == [] and actual["children_reaped"] is True
        for source in actual["sources"]:
            if not source["path"].startswith("src/tests/"):
                verify_reference(source)
        for role, native in actual["native"].items():
            assert native["argv"][1] == "[.ground-network-delivered]" and native["exit_status"] == 0
            verify_reference(native["log"]); verify_reference(native["config"])
            for content in actual["runtime_content"][role]:
                verify_reference(content)
        memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
        events = {row["event"]: row["seconds"] for row in memory["events"]}
        assert actual["join_upper_bound_ns"] == round((events["client_first_work_snapshots_seen"] - events["client_launched"]) * 1e9)
        for comparison in actual["comparisons"]:
            for binding in (*comparison["files"].values(), *comparison["pure_repeats"].values()):
                verify_reference(binding)
        records[key] = entry
    assert len(records) == 6
    failure = prefix["retained_workflow_failure"]
    assert json.loads(verify_reference(failure["report"]).read_text(encoding="utf-8")) == failure["actual"]
    assert failure["actual"]["children_reaped"] is True and failure["completed_join_upper_bound_ns"] < 45000000000
    return records


def verify_selection(index):
    """Require every selected admitted-map/actual-active/thermal pair and all six native checkpoints."""
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA and index["file_acceptance"]["sha256"] == FILES_SHA
    verify_reference(index["controller"]); verify_reference(index["file_acceptance"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    expected, records, assertions = inputs(), {}, 0
    historical = verify_long_prefix(index["historical_prefix"], expected)
    for row in index["records"]:
        key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert key == (actual["configuration"], actual["label"])
        assert actual["baseline"] == index["baseline"]
        if row["historical"]:
            assert key in historical and row["report"] == historical[key]["report"] and actual == historical[key]["actual"]
            observed = {"join_upper_bound_ns": actual["join_upper_bound_ns"], "budget_excesses": actual["budget_excesses"]}
        else:
            assert key not in historical
            observed = verify_measurement(actual, baseline, expected[key])
        assert observed["budget_excesses"] == [] and row["join_upper_bound_ns"] == observed["join_upper_bound_ns"]
        records[key] = {"key": list(key), "report": row["report"], "actual": actual}
        assertions += sum(peer["assertions"] for peer in actual["native"].values())
    assert set(records) == set(expected) and len(records) == 16
    return list(records.values()), assertions


def accept(selection, checks, output):
    """Close selected live reconstruction performance only after every actual pair and fixed budget passes."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    records, assertions = verify_selection(index)
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 8 tests in [\d.]+s\s+OK\s*$", text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21EC;actual native client reconstruction and matched later work",
              "classification": "network_reconstruction_performance_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "file_acceptance": index["file_acceptance"], "historical_long_prefix": index["historical_prefix"], "records": records,
              "peer_pairs": 16, "native_cases": 32, "native_assertions": assertions, "complete_native_peer_checkpoints": 96,
              "complete126_chunk_pure_pairs": 192,
              "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/network_performance_checks.py"), "log": checks_ref, "tests": 8},
              "scope": "real loopback joins:compact128/representative512/maxadmitted1024Debug2048optimized;4actualground/tubeoriginalstages/build;operating legacy431thermal reconstructed by current434peers",
              "uncertainty": "6sealedearlierpairs retain128/256/384/512laterframechecks;10currentboundedpairs retain8/16/24/32latergameticks+16tailticks;same6nativecheckpoints/fixed45sjoinand110sprocessbudgets;datedDebug1024longhorizonfailurepreserved with32.743scompletedjoin;preflightwarmsfilecache;clientlaunch→flushedjoined-next includesfirstwork/4observersaves/<=100mspolling,conservativebound,onejoin/selection;vehicle_parts includesVEHSeffects;7source-boundNoNetworkSyncunits,allotherowners/settingsexact",
              "remaining": "21F attribution and complete selected master11.6 reconciliation"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "peer_pairs": 16, "native_cases": 32, "assertions": assertions}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.checks, args.output)
