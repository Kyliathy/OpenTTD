"""Run two real loopback peers through an original paid mining mission and original shell recovery.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20
Both native processes independently save identical agreed physical owners; socket traffic and real player commands are required.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import runpy
import socket
import subprocess
import time

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

LEAF = runpy.run_path(str(Path(__file__).with_name("probe_replay_checks-1.py")))


def peer(context, configuration, role, source, port):
    """Execute one actual independent network endpoint with original paid input provenance.

    @param context Explicit peer runtime, output and log owner.
    @param configuration Current native executable configuration.
    @param role Actual socket server or joined owning client.
    @param source Original immutable paid deployment, loaded by the host and passed through its native join stream.
    @param port Invocation-local loopback listener, never a public service.
    @return Complete native process status; run_native retains its original argv/executable/input/artifact report.
    """
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_PROBE_REPLAY_SOURCE=str(ROOT / source["path"]),
               OTTD_PROBE_REPLAY_MODE="socket-" + role, OTTD_PROBE_REPLAY_VIEW="visible" if role == "client" else "hidden",
               OTTD_PROBE_REPLAY_ADDRESS=f"127.0.0.1:{port}#1")
    options = {}
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = subprocess.SW_HIDE
        options["startupinfo"] = startup
    with (context.logs / "peer.log").open("wb") as sink:
        return run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.probe-replay-native]"],
                          cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110, **options)


def execute(configuration):
    """Require a real join and exact peer state through extraction, return, command release and recovery.

    @param configuration Current matching server/client executable configuration.
    @return Complete semantic connected-peer report identity.
    """
    dated, source = LEAF["predecessor"](configuration)
    context = create_run_context("space", "probe-replay-sockets")
    peers = {role: context.for_feature("space", "probe-replay-" + role).for_process(role) for role in ("server", "client")}
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
    # Each peer has its own configuration/scripts/logs. Both retain the actual original simulation settings; only network listener and local presentation policies differ.
    for role, owner in peers.items():
        owner.prepare_runtime()
        with owner.config_path.open("a", encoding="utf-8") as stream:
            stream.write(f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\nserver_game_type = local\nclient_name = Probe {role}\nmin_active_clients = 0\npause_on_join = true\n[gui]\nautosave_on_exit = false\n")
        (owner.runtime_dir / "scripts/game_start.scr").write_text("probe_replay_test\n", encoding="utf-8")
    (peers["server"].runtime_dir / "scripts/on_server_connect.scr").write_text("unpause\n", encoding="utf-8")
    with ThreadPoolExecutor(max_workers=2) as executor:
        server = executor.submit(peer, peers["server"], configuration, "server", source, port)
        deadline = time.monotonic() + 20
        server_log = peers["server"].logs / "peer.log"
        # A native loaded-world readiness witness, rather than a guessed sleep or unrelated open port, authorizes joining this particular paid host.
        while not server_log.exists() or "PROBE_REPLAY ready mode=socket-server" not in server_log.read_text(encoding="utf-8", errors="replace"):
            assert not server.done() and time.monotonic() < deadline, server_log
            time.sleep(0.1)
        client = executor.submit(peer, peers["client"], configuration, "client", source, port)
        assert client.result(timeout=110).returncode == 0, peers["client"].logs
        assert server.result(timeout=110).returncode == 0, server_log
    reports = {}
    # Independently parsed Catch/frame witnesses must agree at every physical phase; passing RNG synchronization alone does not qualify a mission.
    for role, owner in peers.items():
        log = owner.logs / "peer.log"
        assertions, cases, witnesses = LEAF["result"](log)
        terminal = log.read_text(encoding="utf-8", errors="replace")
        assert cases == 1 and tuple(witnesses) == LEAF["STAGES"][:-1], (role, log)
        assert "Sync error detected" not in terminal and "sync_err:" not in terminal and "connected=true added_stock=false" in terminal, log
        if role == "server":
            assert "PROBE_REPLAY access native=true resource_grant=false" in terminal and f"Listening on 127.0.0.1:{port}" in terminal, log
        reports[role] = dict(assertions=assertions, cases=cases, terminal=identity(log), witnesses=witnesses,
                             checkpoints={stage: identity(owner.saves / f"replay-{stage}-438.sav") for stage in witnesses},
                             native_reports=[identity(p) for p in sorted(owner.reports.glob("native-*.json"))])
        assert len(reports[role]["native_reports"]) == 1
    assert reports["server"]["witnesses"] == reports["client"]["witnesses"]
    # Exact saves retain the full original financial, finite-body, hull, packet, job, native Industry/history and payment owners across the actual socket boundary.
    for stage in LEAF["STAGES"][:-1]:
        LEAF["compare"](reports["server"]["checkpoints"][stage], reports["client"]["checkpoints"][stage])
    assert identity(ROOT / source["path"]) == source
    report = context.reports / "connected-peers.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, dated_index=dated, source=source,
                                     peers=reports, actual_socket_multiplayer=True, original_client_commands=True, added_stock=False,
                                     host_access_setup="Native CompanyAllowListControl.AllowAny before the first join on this invocation's explicit loopback-only cloned host; no physical or financial grant",
                                     owner_family_comparisons=6 * len(LEAF["OWNERS"]), metadata_comparisons=6 * len(LEAF["METADATA"])), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Run one bounded two-peer scope and retain its exact accepted selector."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    report = execute(args.configuration)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, report=report), indent=2) + "\n", encoding="utf-8")
    print(f"passed {args.configuration} actual connected probe mission", flush=True)


if __name__ == "__main__":
    main()
