#!/usr/bin/env python3
"""Run real loopback lifecycle peers, an armed-timer late join and local twins.

@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group32.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group32B; actual high-ID source/holding inputs.
Uses the original probe_replay_checks-2.py native socket pattern. All private
runtime/captures/saves are under test-cache/space-fixes; logs and hash-bound
receipts under logs/space-fixes. Every child has a110-second process deadline.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import time

from asteroid_cargo_roundtrip_checks import compare as compare_packets
from asteroid_field_wire_checks import compare as compare_field
from space_fixes_checks import ROOT, identity
from space_legacy_compatibility_checks import field_snapshot
from space_mining_wire_checks import compare as compare_orders

PHASES = ("before-detachment", "after-departure", "credit", "joined-armed", "before-expiry", "after-birth")


def launch(args, mode, source, port, epoch, issuance):
    """Start one actual native endpoint or local twin with its own writable data home.

    @param args Selected current configuration and unique run identity.
    @param mode Server, owning client, later observer or exact local replay mode.
    @param source Immutable source save; clients obtain their actual world over the socket.
    @param port Reserved loopback endpoint for the cloned host.
    @param epoch Original saved field opportunity count.
    @param issuance Original next monotonic body identity.
    @return Child process, output directory, native log path and open log sink.
    """
    output = ROOT / "test-cache/space-fixes" / args.run / mode
    output.mkdir(parents=True, exist_ok=False)
    runtime = output / "runtime"
    runtime.mkdir()
    # Isolated asset/configuration copies retain original content while preventing
    # any endpoint from writing beside the fixture or into another peer's home.
    for folder in ("baseset", "newgrf"):
        shutil.copytree(ROOT / "build" / folder, runtime / folder)
    (runtime / "lang").mkdir()
    for language in ("english.lng", "english_US.lng"):
        shutil.copy2(ROOT / "build/lang" / language, runtime / "lang" / language)
    config = runtime / "peer.cfg"
    # The established socket fixture uses the minimal64x64 native-art startup
    # configuration. Demo play.cfg's new-world settings are unrelated to the
    # joined source and can stall GUI intro generation before any connection.
    settings = (ROOT / "src/tests/fixtures/config/multilayer/native-art.cfg").read_text(encoding="utf-8-sig")
    settings += (f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\n"
                 f"server_game_type = local\nclient_name = Lifecycle {mode}\nmin_active_clients = 0\n"
                 "pause_on_join = true\nframe_freq = 1\nsync_freq = 1\n[gui]\nautosave_on_exit = false\n"
                 f"refresh_rate = {144 if mode == 'client' else 30}\n")
    config.write_text(settings, encoding="utf-8")
    (runtime / "scripts").mkdir()
    (runtime / "scripts/game_start.scr").write_text("# r2sff1-space-fixes-features-1.md32\nspace_lifecycle_peer\n", encoding="utf-8")
    address = f"127.0.0.1:{port}" + ("" if mode == "server" else "#1")
    environment = dict(os.environ, OTTD_LIFECYCLE_SOURCE=str(source), OTTD_LIFECYCLE_CONFIG=str(config),
                       OTTD_LIFECYCLE_MODE=mode, OTTD_LIFECYCLE_OUTPUT=str(output), OTTD_LIFECYCLE_ADDRESS=address,
                       OTTD_LIFECYCLE_EPOCH=str(epoch), OTTD_LIFECYCLE_ISSUANCE=str(issuance),
                       OTTD_LIFECYCLE_BODY=str(args.body), OTTD_LIFECYCLE_HOLDING=str(args.holding))
    options = {}
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = subprocess.SW_HIDE
        options["startupinfo"] = startup
    log = ROOT / "logs/space-fixes" / (args.run + "-" + mode + ".log")
    sink = log.open("w", encoding="utf-8")
    executable = ROOT / "build" / args.configuration / "openttd_test.exe"
    process = subprocess.Popen([str(executable), "[.space-lifecycle-peers]", "--warn", "NoTests"], cwd=runtime,
                               env=environment, stdout=sink, stderr=subprocess.STDOUT, **options)
    child = {"process": process, "output": output, "log": log, "sink": sink,
             "started": time.monotonic(), "executable": identity(executable), "source": identity(source), "config": identity(config)}
    receipt = {"_documentation": __doc__, "mode": mode, "executable": child["executable"],
               "source": child["source"], "config": child["config"], "native_log": log.relative_to(ROOT).as_posix()}
    (output / "process.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return child


def wait_marker(child, marker, seconds):
    """Wait for a reached native world/phase rather than guessing socket readiness.

    @param child Actual invocation-owned process/log state.
    @param marker Exact flushed witness from this native producer.
    @param seconds Bounded readiness/phase deadline.
    """
    deadline = time.monotonic() + seconds
    # Poll only evidence from this exact native child; an unrelated open port
    # cannot qualify a loaded world or a naturally armed renewal timer.
    while marker not in child["log"].read_text(encoding="utf-8", errors="replace"):
        assert child["process"].poll() is None, child["log"]
        assert time.monotonic() < deadline, (marker, child["log"])
        time.sleep(0.1)


def finish(child):
    """Require ordinary native shutdown within this child's original process budget.

    @param child Invocation-owned actual process and log state.
    @return Exact executable/source/config/log and Catch assertion identities.
    """
    remaining = max(0.1, 110 - (time.monotonic() - child["started"]))
    code = child["process"].wait(timeout=remaining)
    child["sink"].close()
    assert code == 0, child["log"]
    text = child["log"].read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", text)
    assert passed and "LIFECYCLE_PEER complete" in text, child["log"]
    assert not re.search(r"(desync detected|assertion failed|load failed|save failed)", text, re.I), child["log"]
    return {"executable": child["executable"], "source": child["source"], "config": child["config"],
            "log": identity(child["log"]), "assertions": int(passed[1]), "cases": int(passed[2]), "exit_code": code}


def opportunities(child):
    """Decode exact pre-field opportunity witnesses, retaining the real assigned frame.

    @param child Completed native output owner.
    @return Actual elapsed field count mapped to frame plus fifteen synchronized observations.
    """
    rows = {}
    for line in (child["output"] / "opportunities.tsv").read_text().splitlines():
        values = tuple(map(int, line.split("\t")))
        assert len(values) == 17 and values[0] not in rows
        rows[values[0]] = values[1:]
    return rows


def compare_peer(left, right, phases, same_frame):
    """Compare every common real opportunity and complete saved native owners.

    @param left Completed authoritative source endpoint.
    @param right Independently running peer or local twin.
    @param phases Actually shared physical checkpoints, excluding phases before a late join.
    @param same_frame Whether both endpoints consume the server's actual assigned frames.
    @return Counts plus complete CPP/typed order/ASTR phase comparisons.
    """
    a, b = opportunities(left), opportunities(right)
    common = sorted(a.keys() & b.keys())
    assert len(common) >= 2
    assert common == list(range(common[0], common[-1] + 1)), "shared ordinary opportunity coverage has a gap"
    for elapsed in common:
        # Local execution has no network frame authority. Joined peers must
        # match both the actual assigned frame and all fifteen semantic owners.
        start = 0 if same_frame else 1
        assert a[elapsed][start:] == b[elapsed][start:], (elapsed, a[elapsed], b[elapsed])
    results = []
    for phase in phases:
        source, target = left["output"] / (phase + ".sav"), right["output"] / (phase + ".sav")
        phase_elapsed = field_snapshot(source)[0][4]
        assert phase_elapsed in common and phase_elapsed + 1 in common, ("saved phase and next decision not compared", phase)
        results.append({"phase": phase, "packets": compare_packets(source, target),
                        "orders": compare_orders(source, target), "field": compare_field(source, target)})
    return {"common_opportunities": len(common), "assigned_frames_compared": same_frame,
            "fifteen_synchronized_owners_each_opportunity": True, "phases": results}


def main():
    """Execute one bounded real network scenario and its exact local storage-order twins.

    @return Zero after actual owning-client wire edit, late join and complete semantic replay.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    parser.add_argument("--body", type=int, default=0, help="Actual held Mine's complete32-bit native source-body ID")
    parser.add_argument("--holding", type=int, default=4, help="Actual already earned positive units aboard the source hull")
    args = parser.parse_args()
    source = (ROOT / args.source).resolve()
    # Only original current predecessor saves within this task's artifact owner
    # may supply this cloned local network. No external/public host is contacted.
    if not source.is_relative_to(ROOT / "test-cache/space-fixes") or not source.is_file() or not re.fullmatch(r"[a-z0-9-]+", args.run):
        raise ValueError("select an actual task-owned phase save and a unique lowercase run")
    original = identity(source)
    header, _, renewal = field_snapshot(source)
    issuance = int.from_bytes(renewal[2:6], "big")
    children, report = {}, {"_documentation": __doc__, "configuration": args.configuration, "source": original, "children": {}}
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
    try:
        children["server"] = launch(args, "server", source, port, header[4], issuance)
        wait_marker(children["server"], "LIFECYCLE_PEER ready server=true", 20)
        children["client"] = launch(args, "client", source, port, header[4], issuance)
        wait_marker(children["server"], "LIFECYCLE_PEER checkpoint=credit ", 25)
        children["observer"] = launch(args, "observer", source, port, header[4], issuance)
        for mode in ("observer", "client", "server"):
            report["children"][mode] = finish(children[mode])
        assert "client_policy_post native_wire=true" in children["client"]["log"].read_text()
        assert "LIFECYCLE_PEER late_join" in children["observer"]["log"].read_text()
        report["owning_client"] = compare_peer(children["server"], children["client"], PHASES, True)
        report["late_join"] = compare_peer(children["server"], children["observer"], PHASES[3:], True)
        # Each local twin starts from the host's actual paused LegacyReturn
        # checkpoint and applies the same checked Full Load edit before any work.
        for mode in ("local", "local-reversed"):
            children[mode] = launch(args, mode, children["server"]["output"] / "peer-input.sav", port, header[4], issuance)
            report["children"][mode] = finish(children[mode])
            report[mode] = compare_peer(children["server"], children[mode], PHASES, False)
        assert identity(source) == original
        report["actual_socket_multiplayer"] = True
        report["completed"] = True
        (ROOT / "logs/space-fixes" / (args.run + "-peers.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("LIFECYCLE_PEERS actual_socket=true late_join_armed=true local_reversed=true complete=true")
    except Exception as error:
        report["completed"] = False
        report["failure"] = repr(error)
        report["process_receipts"] = [identity(child["output"] / "process.json") for child in children.values()]
        (ROOT / "logs/space-fixes" / (args.run + "-failure.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        raise
    finally:
        # Only this invocation's still-running children may be terminated after
        # failure. Keep every native log/partial save for the first mismatch.
        for child in children.values():
            if child["process"].poll() is None:
                child["process"].terminate()
                child["process"].wait(timeout=10)
            child["sink"].close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
