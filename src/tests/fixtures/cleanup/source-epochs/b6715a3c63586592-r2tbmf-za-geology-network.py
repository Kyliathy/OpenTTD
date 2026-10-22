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

ROOT = Path(__file__).resolve().parents[4]
BUILD = ROOT / "build"
FIXTURES = ROOT / "src/tests/fixtures/haulage"


def main():
    """@return None; retain actual peer logs/join save and restore startup hooks even on refusal or timeout."""
    source = FIXTURES / "tbm-geology-journey-1-regional-midcut.sav"
    original_source = hashlib.sha256(source.read_bytes()).hexdigest()
    serial = 1
    while (ROOT / f"logs/r2tbmf-za-server-{serial}.log").exists(): serial += 1
    server_log = ROOT / f"logs/r2tbmf-za-server-{serial}.log"
    client_log = ROOT / f"logs/r2tbmf-za-client-{serial}.log"
    joined = FIXTURES / f"tbm-geology-network-{serial}-joined.sav"
    scripts = [BUILD / "scripts/game_start.scr", BUILD / "scripts/on_server_connect.scr"]
    backups = {path: path.read_bytes() if path.exists() else None for path in scripts}
    diagnostics = BUILD / "save/autosave/commands-out.log"
    backups[diagnostics] = diagnostics.read_bytes() if diagnostics.exists() else None
    server_diagnostics = ROOT / f"logs/r2tbmf-za-server-state-{serial}.log"
    client_diagnostics = ROOT / f"logs/r2tbmf-za-client-state-{serial}.log"
    with socket.socket() as address:
        address.bind(("127.0.0.1", 0))
        port = address.getsockname()[1]
    configs = []
    for role in ("server", "client"):
        path = BUILD / f"tbm-geology-network-{serial}-{role}.cfg"
        path.write_bytes((BUILD / "native-art.cfg").read_bytes() +
                         f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\nserver_game_type = local\nclient_name = Regional {role}\nmin_active_clients = 0\npause_on_join = true\n[gui]\nautosave_on_exit = false\n".encode())
        configs.append(path)
    # Match run_dedicated.sh's established Windows console-copy approach.
    # Only the PE subsystem field changes; this avoids a GUI executable
    # allocating its own visible console and bypassing redirected diagnostics.
    game = BUILD / "RelWithDebInfo/openttd.exe"
    console = BUILD / f"RelWithDebInfo/tbm_geology_console_{serial}.exe"
    encoded = bytearray(game.read_bytes())
    header = int.from_bytes(encoded[60:64], "little")
    assert encoded[header:header+4] == b"PE\0\0"
    assert int.from_bytes(encoded[header+92:header+94], "little") == 2
    encoded[header+92:header+94] = (3).to_bytes(2, "little")
    console.write_bytes(encoded)
    common = [str(console), "-b", "null", "-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X"]
    debugging = ["-d", "net=9,misc=4,desync=4,sl=1"]
    children = []
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    try:
        # Save the server's loaded digest, then stop its desync stream before
        # the client opens that native personal-directory log. The peer owns
        # the later stream, so two processes cannot interleave its authority.
        scripts[0].write_text('debug_level "net=9,misc=4,desync=0,sl=1"\necho "REGIONAL_SERVER_READY"\n', encoding="utf-8")
        scripts[1].write_text(f'save "{joined.with_suffix("").as_posix()}"\nunpause\n', encoding="utf-8")
        with server_log.open("w", encoding="utf-8") as server_sink, client_log.open("w", encoding="utf-8") as client_sink:
            server = subprocess.Popen([*common, "-D", "-c", str(configs[0]), "-g", str(source), *debugging], cwd=BUILD,
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
            client = subprocess.Popen([*common, "-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-c", str(configs[1]), "-n", f"127.0.0.1:{port}#255", *debugging], cwd=BUILD,
                                      stdout=client_sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            children.append(client)
            deadline = time.monotonic() + 40
            frames = []
            while time.monotonic() < deadline:
                text = client_log.read_text(encoding="utf-8", errors="replace")
                assert "Sync error detected" not in text and "sync_err:" not in text, "Actual network desync"
                frames = [int(value) for value in re.findall(r"Sent ACK at (\d+)", text)]
                if frames and frames[-1] - frames[0] >= 444: break
                assert server.poll() is None and client.poll() is None, "Peer exited before continuation"
                time.sleep(0.1)
            assert frames and frames[-1] - frames[0] >= 444, "Insufficient actual synchronized frame interval"
            server_text = server_log.read_text(encoding="utf-8", errors="replace")
            client_text = client_log.read_text(encoding="utf-8", errors="replace")
            assert "status = ACTIVE" in server_text and joined.exists(), "No actual active join/save"
            client_diagnostics.write_bytes(diagnostics.read_bytes())
            pattern = r"MLVS_STATE[^\n]*digest=([0-9A-F]+)"
            server_state = re.findall(pattern, server_diagnostics.read_text(encoding="utf-8"))
            client_state = re.findall(pattern, client_diagnostics.read_text(encoding="utf-8"))
            assert server_state and client_state and server_state[-1] == client_state[-1], "Joined canonical feature state differs"
            print(f"REGIONAL_NETWORK join_digest={client_state[-1]} first_ack={frames[0]} last_ack={frames[-1]} synchronized_frames={frames[-1]-frames[0]} no_desync=true", flush=True)
    finally:
        # Only children created above are stopped; no unrelated Node/compiler
        # processes or user game instances are enumerated or terminated.
        for process in reversed(children):
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
        for path, original in backups.items():
            if original is None: path.unlink(missing_ok=True)
            else: path.write_bytes(original)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original_source
        console.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
