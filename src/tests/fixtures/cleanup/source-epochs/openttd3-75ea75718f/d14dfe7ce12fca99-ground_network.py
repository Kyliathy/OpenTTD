"""Exercise one real, isolated host/client join on the accepted paid ground world.

@file ground_network.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group16B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Reuses geology/network_checks.py transport and shared runner mechanics. Initial
feature diagnostics and native ACKs are transport evidence, not a full ledger trace.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import socket
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import console_executable, current_run_context, start_native
from construction_provider_return import identity

HANDOFF = "logs/tubes/tbm-final-use/20261019-native-01/queue-verification.json"
HANDOFF_SHA = "8f52c4afddab198b36f44b48cfba2ca3ccffe59a4878ae44adc569a4ab86ebe0"
CONTENT_SHA = "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
SOURCES = ("src/network/network_command.cpp", "src/network/network_server.cpp",
           "src/network/network_client.cpp", "src/multilayer_state.cpp",
           "src/multilayer_state.h", "src/tests/multilayer/support/runner.py",
           "src/tests/multilayer/support/artifact_paths.py")


def read_log(path):
    """Read one live child diagnostic without decoding platform bytes strictly.

    @param path Invocation-owned native log which may still be receiving bytes.
    @return Available text, or empty text before the first child write.
    """
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def wait_ready(server, path):
    """Wait for actual successful native loading, bounded independently of peers.

    @param server Dedicated child created by this invocation.
    @param path Its isolated diagnostic log containing the startup-script marker.
    @return None; early child exit or missing loaded-world marker refuses the run.
    """
    deadline = time.monotonic() + 30
    # A listening socket or elapsed delay cannot establish that the exact paid
    # world loaded. Only this child's post-load startup hook admits the client.
    while "PAID_GROUND_SERVER_READY" not in read_log(path):
        assert server.poll() is None, f"Host exited before loading: {path}"
        assert time.monotonic() < deadline, f"Host load timed out: {path}"
        time.sleep(0.1)


def wait_continuation(server, client, client_log):
    """Require actual synchronized continuation after a fresh snapshot transfer.

    @param server Dedicated native host created by this invocation.
    @param client Independently joining hidden native Windows client.
    @param client_log Client-owned network diagnostics containing actual ACKs.
    @return First and last acknowledged frames covering at least444 simulation frames.
    """
    deadline = time.monotonic() + 55
    # Network progression, not wall-clock delay or an initial ACK, proves that
    # both independent peers continued beyond the received snapshot together.
    while time.monotonic() < deadline:
        output = read_log(client_log)
        assert "Sync error detected" not in output, f"Actual peer desync: {client_log}"
        assert "sync_err:" not in output, f"Actual peer desync: {client_log}"
        frames = [int(value) for value in re.findall(r"Sent ACK at (\d+)", output)]
        # A real positive ACK interval is required; a successful TCP handshake
        # and loaded metadata alone cannot authorize a continuation claim.
        if frames and frames[-1] - frames[0] >= 444:
            return frames[0], frames[-1]
        assert server.poll() is None, f"Host exited before continuation: {client_log}"
        assert client.poll() is None, f"Client exited before continuation: {client_log}"
        time.sleep(0.1)
    raise AssertionError(f"Insufficient acknowledged simulation interval: {client_log}")


def run_one(configuration):
    """Retain one actual peer transport proof without altering accepted originals.

    @param configuration Exact Debug or RelWithDebInfo application executable.
    @return Invocation report path after all owned child processes have been reaped.
    """
    handoff_path = ROOT / HANDOFF
    assert identity(handoff_path)["sha256"] == HANDOFF_SHA
    handoff = json.loads(handoff_path.read_text(encoding="utf-8"))
    original = handoff["records"][configuration]["outputs"]["queued"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("network", "paid-ground")
    peers = {role: context.for_process(role) for role in ("server", "client")}
    executable = ROOT / f"build/{configuration}/openttd.exe"
    build_identity = identity(executable)
    children = []
    first_ack = last_ack = None
    joined = context.saves / "paid-ground-active-join.sav"
    server_log = context.logs / "server.log"
    client_log = context.logs / "client.log"
    server_state = context.logs / "server-initial-state.log"
    client_state = context.logs / "client-joined-state.log"
    with socket.socket() as address:
        address.bind(("127.0.0.1", 0))
        port = address.getsockname()[1]
    # The same distribution bytes and separate writable homes are mandatory:
    # installed user content and a shared desync/config directory would make
    # successful join diagnostics ambiguous or silently mutate another peer.
    for role, peer in peers.items():
        peer.prepare_runtime()
        assert identity(peer.runtime_dir / "baseset/openttd.grf")["sha256"] == CONTENT_SHA
        settings = (f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\n"
                    f"server_game_type = local\nclient_name = Paid ground {role}\n"
                    "min_active_clients = 0\npause_on_join = true\n[gui]\nautosave_on_exit = false\n")
        peer.config_path.write_bytes(peer.config_path.read_bytes() + settings.encode())
    console = console_executable(peers["server"], executable)
    common = [str(console), "-b", "null", "-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X"]
    debug = ["-d", "net=9,misc=4,desync=4,sl=1"]
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    (peers["server"].runtime_dir / "scripts/game_start.scr").write_text(
        'debug_level "net=9,misc=4,desync=0,sl=1"\necho "PAID_GROUND_SERVER_READY"\n', encoding="utf-8")
    (peers["server"].runtime_dir / "scripts/on_server_connect.scr").write_text(
        f'save "{joined.with_suffix("").as_posix()}"\nunpause\n', encoding="utf-8")
    try:
        with server_log.open("wb") as server_sink, client_log.open("wb") as client_sink:
            server = start_native(peers["server"], [*common, "-D", "-c", str(peers["server"].config_path),
                                  "-g", str(source), *debug], cwd=ROOT / "build", stdout=server_sink,
                                  stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                                  creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            children.append(server)
            wait_ready(server, server_log)
            server_state.write_bytes((peers["server"].runtime_dir / "save/autosave/commands-out.log").read_bytes())
            # The existing transport uses native paced Windows video for the
            # client: null video runs unpaced and can outrun incoming frames.
            client = start_native(peers["client"], [*common, "-v", "win32:no_threads", "-b", "8bpp-simple",
                                  "-r", "640x480", "-c", str(peers["client"].config_path),
                                  "-n", f"127.0.0.1:{port}#255", *debug], cwd=ROOT / "build", stdout=client_sink,
                                  stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                                  startupinfo=startup)
            children.append(client)
            first_ack, last_ack = wait_continuation(server, client, client_log)
            assert "status = ACTIVE" in read_log(server_log), server_log
            assert "Client::ReceiveServerMapDone()" in read_log(client_log), client_log
            assert joined.is_file(), "The actual active-client hook never saved its world"
            client_state.write_bytes((peers["client"].runtime_dir / "save/autosave/commands-out.log").read_bytes())
            digests = {}
            # Compare independently captured afterload feature owners only.
            # Full native vehicles/packets/orders/clocks remain the next group.
            for role, path in (("server", server_state), ("client", client_state)):
                values = re.findall(r"MLVS_STATE[^\n]*digest=([0-9A-F]+)", read_log(path))
                assert values, f"Missing actual afterload feature diagnostic: {path}"
                digests[role] = values[-1]
            assert digests["server"] == digests["client"], "Actual joined feature owners differ"
    finally:
        # Reap only handles returned to this invocation. No unrelated game,
        # compiler, browser or user's active process is enumerated or stopped.
        for process in reversed(children):
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        assert identity(source) == original, "Accepted original input was modified"
        assert identity(executable) == build_identity, "Native application changed during peer run"
    report = {
        "_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md16B;actual snapshot transfer,active ACKs and initial feature diagnostics;not full native simulation trace acceptance",
        "configuration": configuration, "native_version": 434, "controller": identity(__file__),
        "handoff": identity(handoff_path), "original_input": original, "application": build_identity,
        "console_projection": identity(console), "sources": [identity(ROOT / path) for path in SOURCES],
        "content": identity(ROOT / "build/baseset/openttd.grf"), "loopback_port": port,
        "first_ack": first_ack, "last_ack": last_ack, "acknowledged_frames": last_ack - first_ack,
        "initial_feature_digests": digests, "joined_snapshot": identity(joined),
        "diagnostics": {"server": identity(server_log), "client": identity(client_log),
                        "server_initial": identity(server_state), "client_joined": identity(client_state)},
        "configs": {role: identity(peer.config_path) for role, peer in peers.items()},
        "runtime_homes": {role: peer.runtime_dir.relative_to(ROOT).as_posix() for role, peer in peers.items()},
        "children_reaped": all(process.poll() is not None for process in children),
        "remaining": "Actual agreed-frame complete native state/command traces,different cameras/rendering,active excavation/wagon-wait join,time modes and selected lifecycle failures remain pending.",
    }
    target = context.output("reports", f"paid-ground-{configuration}-transport.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "acknowledged_frames": last_ack - first_ack,
                      "join_digest": digests["client"], "children_reaped": report["children_reaped"]}), flush=True)
    return target


def main():
    """Select exactly one native configuration for a bounded two-peer invocation.

    @return None; failed joins retain logs and never publish a successful report.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    run_one(args.config)


if __name__ == "__main__":
    main()
