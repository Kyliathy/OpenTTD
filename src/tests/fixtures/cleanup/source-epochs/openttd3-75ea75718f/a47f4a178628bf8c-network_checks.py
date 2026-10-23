"""Bounded local two-process join and synchronized regional-rock continuation.

@see r2tbmf-tbm-fixes.md Group10; architecture/savegame/archi-savegame.md.
Only this runner's children and repository-owned scripts/configuration are touched.
"""

import hashlib
import os
from pathlib import Path
import re
import socket
import subprocess
import time

# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, start_native, console_executable

CONTEXT = current_run_context("geology", "network")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

BUILD = ROOT / "build"
FIXTURES = ROOT / "src/tests/fixtures/haulage"


def main():
    """@return None; retain actual peer logs/join save and restore startup hooks even on refusal or timeout."""
    source = resolve_input_path(FIXTURES / "tbm-geology-journey-1-regional-midcut.sav")
    original_source = hashlib.sha256(source.read_bytes()).hexdigest()
    serial = 1
    while (CONTEXT.logs / f"r2tbmf-za-server-{serial}.log").exists(): serial += 1
    server_log = CONTEXT.logs / f"r2tbmf-za-server-{serial}.log"
    client_log = CONTEXT.logs / f"r2tbmf-za-client-{serial}.log"
    joined = CONTEXT.saves / f"tbm-geology-network-{serial}-joined.sav"
    server_context = CONTEXT.for_process("server")
    client_context = CONTEXT.for_process("client")
    server_context.prepare_runtime()
    client_context.prepare_runtime()
    server_startup = server_context.runtime_dir / "scripts/game_start.scr"
    # OpenTTD executes on_server_connect on the host. The old shared script
    # directory hid this ownership; isolated peer homes must put it on the host.
    server_connect = server_context.runtime_dir / "scripts/on_server_connect.scr"
    diagnostics = server_context.runtime_dir / "save/autosave/commands-out.log"
    client_state_path = client_context.runtime_dir / "save/autosave/commands-out.log"
    server_diagnostics = CONTEXT.logs / f"server-state-{serial}.log"
    client_diagnostics = CONTEXT.logs / f"client-state-{serial}.log"
    with socket.socket() as address:
        address.bind(("127.0.0.1", 0))
        port = address.getsockname()[1]
    configs = []
    # Each peer has its own config, startup scripts and desync stream; a
    # join must compare independently observed state, never an interleaved log.
    for role, context in (("server", server_context), ("client", client_context)):
        path = context.config_path
        path.write_bytes(context.config_path.read_bytes() +
                         f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\nserver_game_type = local\nclient_name = Regional {role}\nmin_active_clients = 0\npause_on_join = true\n[gui]\nautosave_on_exit = false\n".encode())
        configs.append(path)
    console = console_executable(server_context, BUILD / "RelWithDebInfo/openttd.exe")
    common = [str(console), "-b", "null", "-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X"]
    debugging = ["-d", "net=9,misc=4,desync=4,sl=1"]
    children = []
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    try:
        # Snapshot the loaded server state before continuation; the client
        # independently records its received world in its own data home.
        server_startup.write_text('debug_level "net=9,misc=4,desync=0,sl=1"\necho "REGIONAL_SERVER_READY"\n', encoding="utf-8")
        server_connect.write_text(f'save "{joined.with_suffix("").as_posix()}"\nunpause\n', encoding="utf-8")
        with server_log.open("w", encoding="utf-8") as server_sink, client_log.open("w", encoding="utf-8") as client_sink:
            server = start_native(server_context, [*common, "-D", "-c", str(configs[0]), "-g", str(source), *debugging], cwd=BUILD,
                                      stdout=server_sink, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                                      creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            children.append(server)
            deadline = time.monotonic() + 20
            # Readiness is actual loaded metadata, not an arbitrary startup
            # delay or an unrelated process listening on the selected port.
            while "REGIONAL_SERVER_READY" not in server_log.read_text(encoding="utf-8", errors="replace"):
                assert server.poll() is None and time.monotonic() < deadline, "Server did not load regional world"
                time.sleep(0.1)
            server_diagnostics.write_bytes(diagnostics.read_bytes())
            # The null driver's unpaced loop can run ahead of incoming sync
            # packets. Use the ordinary Windows timing loop with its first
            # ShowWindow hidden by STARTUPINFO, preserving real client timing.
            client = start_native(client_context, [*common, "-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-c", str(configs[1]), "-n", f"127.0.0.1:{port}#255", *debugging], cwd=BUILD,
                                      stdout=client_sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            children.append(client)
            deadline = time.monotonic() + 40
            frames = []
            while time.monotonic() < deadline:
                text = client_log.read_text(encoding="utf-8", errors="replace")
                assert "Sync error detected" not in text and "sync_err:" not in text, "Actual network desync"
                frames = [int(value) for value in re.findall(r"Sent ACK at (\d+)", text)]
                # Stop only after actual acknowledgements cover the required
                # synchronized interval; an initial ACK alone proves no continuation.
                if frames and frames[-1] - frames[0] >= 444: break
                assert server.poll() is None and client.poll() is None, "Peer exited before continuation"
                time.sleep(0.1)
            assert frames and frames[-1] - frames[0] >= 444, "Insufficient actual synchronized frame interval"
            server_text = server_log.read_text(encoding="utf-8", errors="replace")
            client_text = client_log.read_text(encoding="utf-8", errors="replace")
            assert "status = ACTIVE" in server_text and joined.exists(), "No actual active join/save"
            client_diagnostics.write_bytes(client_state_path.read_bytes())
            pattern = r"MLVS_STATE[^\n]*digest=([0-9A-F]+)"
            server_state = re.findall(pattern, server_diagnostics.read_text(encoding="utf-8"))
            client_state = re.findall(pattern, client_diagnostics.read_text(encoding="utf-8"))
            assert server_state and client_state and server_state[-1] == client_state[-1], "Joined canonical feature state differs"
            CONTEXT.write_report("network-provenance.json", Path(__file__).relative_to(ROOT).as_posix(),
                                 [joined], [source], "RelWithDebInfo", "local server/client synchronized continuation")
            print(f"REGIONAL_NETWORK join_digest={client_state[-1]} first_ack={frames[0]} last_ack={frames[-1]} synchronized_frames={frames[-1]-frames[0]} no_desync=true", flush=True)
    finally:
        # Only children created above are stopped; no unrelated Node/compiler
        # processes or user game instances are enumerated or terminated.
        for process in reversed(children):
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original_source



if __name__ == "__main__":
    main()
