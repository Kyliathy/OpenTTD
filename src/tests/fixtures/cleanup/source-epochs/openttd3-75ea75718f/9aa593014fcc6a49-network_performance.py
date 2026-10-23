"""Measure real host/client reconstruction under shared Windows reserves and exact native frame comparisons.

@file network_performance.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EC.
Client launch through its first completed native work/pure snapshots is a conservative join upper bound.
"""

import argparse
import json
import os
import re
import socket
import subprocess
import time

from ground_network import ROOT, CONTENT_SHA, read_log
from runner import create_run_context, start_native
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import hardware_identity, memory_status, process_memory
from ground_stress import BASELINE, BASELINE_SHA, selected_budget
from ground_network_trace import LOCAL_UNITS, compare_frames
from save_tables import saved_table

FILES = "logs/performance/ground-maps/20261019-native-01/file-performance-verification.json"
FILES_SHA = "10d63ef739d31bbb98230dde0505ae73d30a7ae5d079dafabd003d78dbdf0adf"
DELIVERED = "logs/network/paid-ground/20261019-native-01/delivered-inputs.json"
DELIVERED_SHA = "2e99895ece641555d4c6cdf93fd57ccb0263cff95294ad53477251f424b32af1"
TAG = "[.ground-network-performance]"
SOURCES = ("src/tests/tbm_reuse.cpp", "src/tests/ground_network_fixture.h", "src/tests/layer_save_fixture.h",
           "src/tests/multilayer/ground_network_trace.py", "src/tests/multilayer/ground_network.py",
           "src/tests/multilayer/ground_performance.py", "src/tests/multilayer/guarded_native_build.py",
           "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py",
           "src/network/network_server.cpp", "src/network/network_client.cpp", "src/network/network_command.cpp",
           "src/saveload/saveload.cpp", "src/saveload/afterload.cpp", "src/saveload/map_sl.cpp",
           "src/multilayer_state.cpp", "src/utility_power.cpp", "src/transport_tube_runtime.cpp",
           "src/geothermal_generator.cpp", "src/table/settings/locale_settings.ini")


def inputs():
    """Use exact accepted files and the actual operating legacy thermal producer,without preparing a new world."""
    file_ref, delivered_ref = identity(ROOT / FILES), identity(ROOT / DELIVERED)
    assert file_ref["sha256"] == FILES_SHA and delivered_ref["sha256"] == DELIVERED_SHA
    files = json.loads(verify_reference(file_ref).read_text(encoding="utf-8"))
    delivered = json.loads(verify_reference(delivered_ref).read_text(encoding="utf-8"))
    selected = {}
    for row in files["records"]:
        cfg, label, attempt = row["key"]
        if attempt == 0:
            selected[cfg, label] = {"input": row["selected"]["input"], "producer": file_ref, "kind": row["selected"]["kind"]}
    for cfg in ("Debug", "RelWithDebInfo"):
        selected[cfg, "thermal-operating"] = {"input": delivered["records"]["thermal-operating"]["input"],
                                               "producer": delivered_ref, "kind": "original_operating_legacy_thermal"}
    assert len(selected) == 16
    return selected


def native_counts(path):
    """Read actual native geometry,job,vehicle,packet and utility counts without creating simulation state."""
    maps = saved_table(path, b"MAPS")[0]
    utility = saved_table(path, b"UPLD")
    return {"width": maps["dim_x"], "height": maps["dim_y"], "layers": maps["layers"],
            "jobs": len(saved_table(path, b"XJOB")), "vehicle_parts": len(saved_table(path, b"VEHS")),
            "packets": len(saved_table(path, b"CAPA")), "utility_records": len(utility),
            "utility_producers": sum(row["type"] == 0 for row in utility.values()),
            "utility_consumers": sum(row["type"] == 1 for row in utility.values())}


def run_one(configuration, label):
    """Own two real loopback peers,record OS peaks/marker times and always reap only these children."""
    selected = inputs()[configuration, label]
    source = verify_reference(selected["input"])
    source_counts = native_counts(source)
    source_settings = saved_table(source, b"PATS")[0]
    host_units = {name: source_settings[name] for name in LOCAL_UNITS}
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    budget = selected_budget(baseline, configuration, max(128, source_counts["width"]))
    context = create_run_context("network", "paid-ground")
    peers = {role: context.for_process(role) for role in ("server", "client")}
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary, controller, sources = identity(executable), identity(__file__), [identity(ROOT / name) for name in SOURCES]
    with socket.socket() as address:
        address.bind(("127.0.0.1", 0)); port = address.getsockname()[1]
    logs = {role: context.logs / f"performance-{role}.log" for role in peers}
    runtime_content = {}
    for role, peer in peers.items():
        peer.prepare_runtime()
        assert identity(peer.runtime_dir / "baseset/openttd.grf")["sha256"] == CONTENT_SHA
        runtime_content[role] = [identity(path) for path in sorted(peer.runtime_dir.rglob("*.grf"))]
        settings = (f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\nserver_game_type = local\n"
                    f"client_name = Performance {role}\nmin_active_clients = 0\npause_on_join = true\nmax_lag_time = 2000\n"
                    "[gui]\nautosave_on_exit = false\n")
        peer.config_path.write_bytes(peer.config_path.read_bytes() + settings.encode())
    assert [(ref["bytes"], ref["sha256"]) for ref in runtime_content["server"]] == [(ref["bytes"], ref["sha256"]) for ref in runtime_content["client"]]
    header = "# Plan16-21EC;actual native loopback reconstruction performance.\n"
    (peers["server"].runtime_dir / "scripts/game_start.scr").write_text(header + 'pause\necho "PAID_GROUND_SERVER_READY"\n', encoding="utf-8")
    (peers["server"].runtime_dir / "scripts/on_server_connect.scr").write_text(header + "unpause\n", encoding="utf-8")
    startup = subprocess.STARTUPINFO(); startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW; startup.wShowWindow = subprocess.SW_HIDE
    environments = {"server": {"OTTD_GROUND_NETWORK_ROLE": "server", "OTTD_GROUND_NETWORK_INPUT": str(source)},
                    "client": {"OTTD_GROUND_NETWORK_ROLE": "client", "OTTD_GROUND_NETWORK_ADDRESS": f"127.0.0.1:{port}#255"}}
    opening, started = memory_status(), time.perf_counter()
    assert opening["physical_available"] >= protocol["minimum_physical_available_bytes"] and opening["commit_available"] >= protocol["minimum_commit_available_bytes"]
    children, launch, observations, events, failure = {}, {}, [], [], None
    joined_next_seen, last_log_check, quit_sent = None, -1.0, False
    print(json.dumps({"phase": "network_performance_started", "configuration": configuration, "label": label,
                      "source": selected["input"], "counts": source_counts, "logs": {k: str(v.relative_to(ROOT)) for k, v in logs.items()}}), flush=True)
    try:
        with logs["server"].open("wb") as server_sink, logs["client"].open("wb") as client_sink:
            launch["server"] = time.perf_counter() - started
            children["server"] = start_native(peers["server"], [str(executable), TAG], cwd=ROOT / "build", env=dict(os.environ, **environments["server"]),
                                              stdout=server_sink, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                                              creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            events.append({"event": "server_launched", "seconds": launch["server"]})
            while True:
                elapsed, system = time.perf_counter() - started, memory_status()
                measurements = {role: process_memory(child) for role, child in children.items()}
                observations.append({"seconds": elapsed, "system": system, "peers": measurements})
                assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"], "shared system reserve crossed"
                assert all(max(row["private_bytes"], row["lifetime_peak_commit_bytes"]) <= protocol["maximum_child_private_bytes"] for row in measurements.values()), "owned peer memory ceiling crossed"
                assert elapsed < protocol["child_timeout_seconds"], "owned peer pair110s deadline exceeded"
                if elapsed - last_log_check >= .1:
                    last_log_check = elapsed
                    if "client" not in children:
                        assert children["server"].poll() is None, logs["server"]
                        if "PAID_GROUND_SERVER_READY" in read_log(logs["server"]):
                            launch["client"] = time.perf_counter() - started
                            children["client"] = start_native(peers["client"], [str(executable), TAG], cwd=ROOT / "build", env=dict(os.environ, **environments["client"]),
                                                              stdout=client_sink, stderr=subprocess.STDOUT,
                                                              creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
                            events.append({"event": "client_launched", "seconds": launch["client"]})
                        else:
                            assert elapsed < 80, "native host startup/load deadline exceeded"
                    elif joined_next_seen is None and "stage=joined-next " in read_log(logs["client"]):
                        joined_next_seen = time.perf_counter() - started
                        events.append({"event": "client_first_work_snapshots_seen", "seconds": joined_next_seen})
                if "client" in children and children["client"].poll() is not None:
                    assert children["client"].returncode == 0, logs["client"]
                    if not quit_sent:
                        children["server"].stdin.write(b"quit\n"); children["server"].stdin.flush(); quit_sent = True
                    if children["server"].poll() is not None:
                        assert children["server"].returncode == 0, logs["server"]
                        break
                else:
                    assert children["server"].poll() is None, logs["server"]
                time.sleep(protocol["memory_sample_seconds"])
            assert joined_next_seen is not None
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    finally:
        for child in reversed(list(children.values())):
            if child.poll() is None:
                child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill(); child.wait(timeout=5)
        assert identity(source) == selected["input"] and identity(executable) == binary and identity(__file__) == controller
        assert all(identity(ROOT / ref["path"]) == ref for ref in sources)
    observations.append({"seconds": time.perf_counter() - started, "system": memory_status(),
                         "peers": {role: process_memory(child) for role, child in children.items()}})
    memory_file = context.output("reports", "performance-memory.json")
    memory_file.write_text(json.dumps({"opening": opening, "samples": observations, "events": events, "closing": memory_status()}, indent=2) + "\n", encoding="utf-8")
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21EC;actual native peers,not replay or inferred ACK timing",
              "classification": "native_network_reconstruction_performance", "bootstrap_claim": False, "configuration": configuration, "label": label,
              "selected": selected, "source_counts": source_counts, "source_local_units": host_units, "baseline": baseline_ref, "budget": budget,
              "controller": controller, "sources": sources, "binary": binary, "hardware": hardware_identity(), "runtime_content": runtime_content,
              "native": {role: {"argv": [str(executable), TAG], "cwd": "build", "environment_selectors": environments[role],
                                "config": identity(peer.config_path), "log": identity(logs[role]), "exit_status": children[role].returncode if role in children else None}
                         for role, peer in peers.items()}, "memory_samples": identity(memory_file),
              "children_reaped": len(children) == 2 and all(child.poll() is not None for child in children.values()),
              "wall_seconds": time.perf_counter() - started, "failure": failure, "status": "failed" if failure else "observed"}
    if failure is None:
        try:
            finish_measurement(result, context, baseline, observations, events)
        except BaseException as error:
            failure = f"{type(error).__name__}: {error}"
            result.update(failure=failure, status="failed")
    target = context.output("reports", f"network-performance-{configuration}-{label}.json")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "status": result["status"], "failure": failure,
                      "join_upper_bound_ns": result.get("join_upper_bound_ns"), "excesses": result.get("budget_excesses")}), flush=True)
    if failure:
        raise RuntimeError(failure + ";retained=" + str(target))
    return target


def finish_measurement(result, context, baseline, observations, events):
    """Parse completed actual native peers and require all joined/next/later owners,not merely matching RNG."""
    joins, states = {}, {}
    for role, native in result["native"].items():
        text = verify_reference(native["log"]).read_text(encoding="utf-8")
        match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert match and int(match[1]) > 0 and int(match[2]) == 1 and native["exit_status"] == 0
        native.update(assertions=int(match[1]), cases=1)
        joins[role] = [{"stage": stage, "frame": int(frame), "tick": int(tick)} for stage, frame, tick in
                       re.findall(r"GROUND_NETWORK_JOIN role=\w+ stage=(joined(?:-next)?) frame=(\d+) tick=(\d+)", text)]
        states[role] = [{"frame": int(frame), "tick": int(tick), "rng0": int(rng0), "rng1": int(rng1), "observations": int(count)}
                        for frame, tick, rng0, rng1, count in re.findall(r"GROUND_NETWORK_STATE role=\w+ frame=(\d+) tick=(\d+) rng0=(\d+) rng1=(\d+) observations=(\d+)", text)]
        assert [row["stage"] for row in joins[role]] == ["joined", "joined-next"] and len(states[role]) == 4
        assert joins[role][1]["frame"] - joins[role][0]["frame"] == joins[role][1]["tick"] - joins[role][0]["tick"] == 1
        assert [row["observations"] for row in states[role]] == [1, 2, 3, 4]
        assert [row["tick"] - joins[role][0]["tick"] for row in states[role]] == [8, 16, 24, 32]
    assert joins["server"] == joins["client"] and states["server"] == states["client"]
    checkpoints = [(row["stage"], row["frame"]) for row in joins["server"]]
    checkpoints.extend((f"f{row['frame']}", row["frame"]) for row in states["server"])
    comparisons = compare_frames(context, checkpoints, expected_host_units=result["source_local_units"])
    counts = {role: native_counts(verify_reference(comparisons[0]["files"][role])) for role in result["native"]}
    assert counts["server"] == counts["client"]
    assert all(counts[role][field] == result["source_counts"][field] for role in counts for field in ("width", "height", "layers"))
    # Each pure native file's DATE must identify the observed agreed tick;equal
    # pairs cannot be metadata aliases to another valid saved opportunity.
    for comparison, observation in zip(comparisons, [*joins["server"], *states["server"]]):
        for role, reference in comparison["files"].items():
            assert saved_table(verify_reference(reference), b"DATE")[0]["tick_counter"] == observation["tick"]
    launched = next(event["seconds"] for event in events if event["event"] == "client_launched")
    seen = next(event["seconds"] for event in events if event["event"] == "client_first_work_snapshots_seen")
    assert seen >= launched
    result.update(joins=joins, states=states, comparisons=comparisons, joined_counts=counts,
                  checkpoint_basis="game_tick", checkpoint_offsets=[8, 16, 24, 32], trailing_ticks=16,
                  join_upper_bound_ns=round((seen - launched) * 1e9),
                  join_timing_policy="client launch to flushed joined-next marker after first native work and two complete pure snapshot pairs;includes observer saves and <=100ms marker sampling;conservative upper bound",
                  peak_commit_bytes={role: max(row["peers"][role]["lifetime_peak_commit_bytes"] for row in observations if role in row["peers"]) for role in result["native"]},
                  peak_working_bytes={role: max(row["peers"][role]["lifetime_peak_working_bytes"] for row in observations if role in row["peers"]) for role in result["native"]},
                  sampled_combined_private_bytes=max(sum(peer["private_bytes"] for peer in row["peers"].values()) for row in observations))
    result["combined_lifetime_peak_commit_bound"] = sum(result["peak_commit_bytes"].values())
    result["budget_excesses"] = []
    if result["join_upper_bound_ns"] > result["budget"]["join_reconstruction_ns"]:
        result["budget_excesses"].append("join_upper_bound.ns")
    for role, peak in result["peak_commit_bytes"].items():
        if peak > result["budget"]["peak_commit_bytes"]:
            result["budget_excesses"].append(role + ".peak_commit_bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", required=True, choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--label", required=True)
    args = parser.parse_args()
    run_one(args.configuration, args.label)
