"""Compare complete native paid-ground saves at actual agreed host/client frames.

@file ground_network_trace.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Groups17B-21C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The test-only timer observes real application loops; no copied-state simulator runs.
"""

import argparse
import hashlib
import json
import os
import re
import socket
import subprocess
import sys
import time

sys.dont_write_bytecode = True
from ground_network import ROOT, HANDOFF, HANDOFF_SHA, CONTENT_SHA, SOURCES, read_log, wait_ready, wait_continuation
from runner import current_run_context, start_native
from construction_provider_return import identity
from save_tables import chunks, read_stream, saved_table

FRAMES = (256, 384, 512, 640)
TAG = "[.ground-network-peer]"
SCENARIO_TAGS = {"baseline": TAG, "company": "[.ground-network-company]",
                 "standard-active": "[.ground-network-active]", "deep-active": "[.ground-network-active]",
                 "wagon-wait": "[.ground-network-active]", "tube-queue": "[.ground-network-delivered]",
                 "thermal-operating": "[.ground-network-delivered]", "pause-calendar": "[.ground-network-pause]"}
CLOCK_MODES = ("calendar-default", "wallclock-default", "wallclock-slow", "wallclock-frozen")
SCENARIO_TAGS.update({mode: "[.ground-network-clock-peer]" for mode in CLOCK_MODES})
SCENARIO_TAGS["power-loss"] = "[.ground-network-power-loss]"
SCENARIO_TAGS["power-recovery"] = "[.ground-network-power-recovery]"
SCENARIO_TAGS["thermal-riser"] = "[.ground-network-thermal-riser]"
LOCAL_UNITS = {"locale.units_velocity": 1, "locale.units_velocity_nautical": 1,
               "locale.units_power": 1, "locale.units_weight": 1, "locale.units_volume": 1,
               "locale.units_force": 2, "locale.units_height": 1}


def world(path):
    """Decode every native chunk without loading or mutating the observed world.

    @param path Complete ordinary native checkpoint written by one actual peer.
    @return All126 named complete native transport chunks,including their schemas.
    """
    version, stream = read_stream(path)
    assert version == 434
    result = {item["name"].decode("ascii"): item["raw"] for item in chunks(stream)}
    assert len(result) == 126
    return result


def compare_frames(context, checkpoints=None, expected_host_units=None):
    """Compare complete same-frame peers and demand pure repeated observations.

    @param context Explicit shared invocation containing both peers' native saves.
    @param checkpoints File-stage/actual-frame pairs;paused stages may provide actual per-peer frames for one frozen gameplay tick.
    @param expected_host_units Exact seven local unit values from an accepted source;None preserves the earlier zero-valued fixture contract.
    @return Exact per-frame identities with seven explicitly native client-only unit fields separated.
    """
    records = []
    # Ordinary generated inputs retain native default local units;earlier
    # isolated paid originals retain zero. Both are real saved source policy.
    # Defaults preserve the earlier producer contract exactly.
    host_units = dict.fromkeys(LOCAL_UNITS, 0) if expected_host_units is None else expected_host_units
    assert set(host_units) == set(LOCAL_UNITS)
    expected_changed = ["PATS"] if any(host_units[name] != value for name, value in LOCAL_UNITS.items()) else []
    checkpoints = checkpoints if checkpoints is not None else [(f"f{frame}", frame) for frame in FRAMES]
    # Every checkpoint covers all serialized native gameplay owners,not merely
    # a feature digest. Consecutive same-peer writes must match every chunk.
    for stage, frame in checkpoints:
        paths = {role: context.saves / f"ground-{role}-{stage}.sav" for role in ("server", "client")}
        data = {role: world(path) for role, path in paths.items()}
        repeats = {}
        for role, path in paths.items():
            repeated = path.with_name(path.stem + "-repeat.sav")
            assert data[role] == world(repeated), f"Native observation mutated serialized owners: {path}"
            repeats[role] = identity(repeated)
        changed = [name for name in data["server"] if data["server"][name] != data["client"][name]]
        # Native NoNetworkSync unit preferences remain local during join. Only
        # these seven named PATS fields may differ;VIEW's saved plane position
        # intentionally strips layer selection and is exact in this scenario.
        if changed != expected_changed:
            failure = {"frame": frame, "changed_chunks": changed, "files": {role: identity(path) for role, path in paths.items()}}
            context.output("reports", "comparison-failure.json").write_text(json.dumps(failure, indent=2) + "\n", encoding="utf-8")
            raise AssertionError(f"Unexpected actual peer owner mismatch: {failure}")
        settings = {role: saved_table(path, b"PATS") for role, path in paths.items()}
        assert set(settings["server"]) == set(settings["client"]) == {0}
        assert settings["server"][0].keys() == settings["client"][0].keys()
        local = {}
        # Validate each actual native NoNetworkSync declaration and exact
        # expected original/default value. No unknown field can be normalized
        # merely because it belongs to the same locale/PATS namespace.
        definitions = (ROOT / "src/table/settings/locale_settings.ini").read_text(encoding="utf-8")
        for name, default in LOCAL_UNITS.items():
            block = re.search(r"var\s*=\s*" + re.escape(name) + r"\s*\n(.*?)(?=\n\[|\Z)", definitions, re.S)
            assert block is not None
            assert "SettingFlag::NoNetworkSync" in block[1]
            assert re.search(r"def\s*=\s*" + str(default) + r"\s*\n", block[1]) is not None
            assert settings["server"][0][name] == host_units[name]
            assert settings["client"][0][name] == default
            local[name] = {role: table[0][name] for role, table in settings.items()}
        canonical = {role: {name: value for name, value in table[0].items() if name not in LOCAL_UNITS}
                     for role, table in settings.items()}
        assert canonical["server"] == canonical["client"], "Native gameplay setting differs"
        records.append({"stage": stage, "frame": None if isinstance(frame, dict) else frame,
                        "actual_peer_frames": frame if isinstance(frame, dict) else {role: frame for role in paths},
                        "comparison_kind": "frozen-gameplay-tick" if isinstance(frame, dict) else "agreed-network-frame",
                        "files": {role: identity(path) for role, path in paths.items()},
                        "pure_repeats": repeats, "complete_peer_chunks_equal": 126 - len(changed),
                        "native_local_units": local, "canonical_settings_equal": len(canonical["server"]),
                        "complete_chunk_sha256": {
                            role: {name: hashlib.sha256(raw).hexdigest() for name, raw in native.items()}
                            for role, native in data.items()}})
    return records


def wait_paused(server, client, logs):
    """Wait until both actual peers finish two read-only frozen observations.

    @param server Owned live dedicated host;native console receives the later unpause.
    @param client Owned independently joined client,never a copied-state replay.
    @param logs Exact per-role live diagnostic paths for this invocation.
    @return None;the caller may resume only after both native paused-b markers exist.
    """
    deadline = time.monotonic() + 45
    # A wall-time delay alone cannot prove the client observed pause. Both
    # actual native peers must independently finish their frozen-state saves
    # before the administrator's normal unpause command can enter the queue.
    while time.monotonic() < deadline:
        ready = [f"GROUND_NETWORK_PAUSE role={role} stage=paused-b" in read_log(path) for role, path in logs.items()]
        if all(ready):
            return
        assert server.poll() is None, logs["server"]
        assert client.poll() is None, logs["client"]
        time.sleep(0.1)
    raise AssertionError(f"Actual native frozen observations timed out: {logs}")


def run_one(configuration, focus="baseline"):
    """Run one real native peer pair with complete read-only frame observations.

    @param configuration Exact native test application build,Debug or RelWithDebInfo.
    @param focus Baseline/company/active/delivered,pause,starting-clock or original paid power-loss scenario.
    @return Successful report path after clean native exits and complete comparisons.
    """
    clock_case = focus in CLOCK_MODES
    loss_case = focus == "power-loss"
    recovery_case = focus == "power-recovery"
    thermal_case = focus == "thermal-riser"
    power_case = loss_case or recovery_case
    active = clock_case or focus in ("standard-active", "deep-active", "wagon-wait")
    pause_case = focus == "pause-calendar"
    delivered = focus in ("tube-queue", "thermal-operating", "pause-calendar", "power-loss", "power-recovery", "thermal-riser")
    active_owner = None
    delivered_owner = None
    if clock_case:
        selected = ROOT / "logs/network/paid-ground/20261019-native-01/clock-inputs.json"
        assert identity(selected)["sha256"] == "3a07d19b75969ff44033a9ed28fb0db4a13cd0fcca5fdca83cbaa8d529fd731d"
        active_owner = json.loads(selected.read_text(encoding="utf-8"))["records"][configuration][focus]
        source_ref = active_owner["input"]
        capsule = 0
        handoff = identity(selected)
    elif active:
        selected = ROOT / "logs/network/paid-ground/20261019-native-01/active-inputs.json"
        assert identity(selected)["sha256"] == "0c247a8383a9e1b91157d0ce41d5f6cb63e4e64042100eafaf2651f4d967b6bb"
        active_owner = json.loads(selected.read_text(encoding="utf-8"))["records"][focus]
        source_ref = active_owner["input"]
        capsule = 0
        handoff = identity(selected)
    elif recovery_case:
        selected = ROOT / "logs/network/paid-ground/20261019-native-01/power-loss-verification.json"
        assert identity(selected)["sha256"] == "2739375a1c6143fdfaf63c5f2962af2f4ac38a3d0dad0f860b71c3d9060c5ea6"
        delivered_owner = json.loads(selected.read_text(encoding="utf-8"))["records"][configuration]
        source_ref = delivered_owner["broken_input"]
        capsule = delivered_owner["actual"]["original_delivered"]["participants"]["0"]["vehicle"]
        handoff = identity(selected)
    elif delivered:
        selected = ROOT / "logs/network/paid-ground/20261019-native-01/delivered-inputs.json"
        assert identity(selected)["sha256"] == "2e99895ece641555d4c6cdf93fd57ccb0263cff95294ad53477251f424b32af1"
        input_focus = "thermal-operating" if thermal_case else "tube-queue" if pause_case or power_case else focus
        delivered_owner = json.loads(selected.read_text(encoding="utf-8"))["records"][input_focus]
        source_ref = delivered_owner["input"]
        capsule = 0
        if power_case:
            capsule = delivered_owner["participants"]["0"]["vehicle"]
        handoff = identity(selected)
    else:
        assert identity(ROOT / HANDOFF)["sha256"] == HANDOFF_SHA
        accepted = json.loads((ROOT / HANDOFF).read_text(encoding="utf-8"))
        source_ref = accepted["records"][configuration]["outputs"]["queued"]
        capsule = accepted["records"][configuration]["useful_participants"]["capsule"]
        handoff = identity(ROOT / HANDOFF)
    company_commands = focus == "company"
    tag = SCENARIO_TAGS[focus]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    context = current_run_context("network", "paid-ground")
    peers = {role: context.for_process(role) for role in ("server", "client")}
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    with socket.socket() as address:
        address.bind(("127.0.0.1", 0))
        port = address.getsockname()[1]
    logs = {role: context.logs / f"trace-{role}.log" for role in peers}
    # Isolate each real application/config/command stream while retaining the
    # same explicitly selected source and distribution as the transport proof.
    for role, peer in peers.items():
        peer.prepare_runtime()
        assert identity(peer.runtime_dir / "baseset/openttd.grf")["sha256"] == CONTENT_SHA
        settings = (f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\n"
                    f"server_game_type = local\nclient_name = Trace {role}\nmin_active_clients = 0\n"
                    "pause_on_join = true\n"
                    # Complete native writes and real rendering can briefly
                    # delay this longer client on a busy host. Use the native
                    # bounded connection policy; never relax state equality,
                    # command ordering or the case's existing wall deadline.
                    + ("max_lag_time = 2000\n" if recovery_case else "")
                    + "[gui]\nautosave_on_exit = false\n")
        peer.config_path.write_bytes(peer.config_path.read_bytes() + settings.encode())
    prepare = 'ground_network_work_ready\n' if active else ''
    if recovery_case:
        # Unlike original fixture sources,the exact broken peer checkpoint
        # was saved unpaused. The real startup console pauses it before first
        # simulation;normal on-connect unpause then starts both original peers.
        prepare = 'pause\n'
    owner_header = "# Owner: r2mlvs15-savegame-determinism-and-lifecycle.md; archi-map-layers.md.\n"
    (peers["server"].runtime_dir / "scripts/game_start.scr").write_text(
        owner_header + prepare + 'echo "PAID_GROUND_SERVER_READY"\n', encoding="utf-8")
    # The retained single-player company has a closed native allow-list.
    # Enrol this invocation's actual first client's authenticated key through
    # the normal administrator console,then move that real peer into company1.
    # No key,client_playas value or received command authority is written here.
    connected = 'authorized_key add company:1 client:2\nmove 2 1\nunpause\n' if company_commands else 'unpause\n'
    (peers["server"].runtime_dir / "scripts/on_server_connect.scr").write_text(owner_header + connected, encoding="utf-8")
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    children = []
    active_env = {}
    if active:
        active_env = {"OTTD_GROUND_NETWORK_PROJECT": str(active_owner["project"]["id"]),
                      "OTTD_GROUND_NETWORK_CURSOR": str(active_owner["job"]["cursor"]),
                      "OTTD_GROUND_NETWORK_PHASE": str(active_owner["project"]["phase"]),
                      "OTTD_GROUND_NETWORK_TICK": str(active_owner["clock"])}
    # Both failure and pause observe exact original opportunities;only the
    # selected failure hook may post the owning native power command.
    if pause_case or power_case or thermal_case:
        active_env["OTTD_GROUND_NETWORK_TICK"] = str(delivered_owner["clock"])
    if thermal_case:
        # Resolve only the sealed original machinery and service,never build a
        # replacement plant or synthesize a utility participant for networking.
        original_generators = list(delivered_owner["generators"].values())
        original_risers = list(delivered_owner["risers"].values())
        town = next(row for row in delivered_owner["utility"].values() if row["type"] == 1 and row["town_terminal"] != 65535)
        active_env.update({
            "OTTD_GROUND_NETWORK_GENERATOR": str(next(row["industry"] for row in original_generators if row["model"] == 1)),
            "OTTD_GROUND_NETWORK_VENT_GENERATOR": str(next(row["industry"] for row in original_generators if row["model"] == 0)),
            "OTTD_GROUND_NETWORK_FIRST_RISER": str(next(row["industry"] for row in original_risers if row["layer"] == 3)),
            "OTTD_GROUND_NETWORK_SECOND_RISER": str(next(row["industry"] for row in original_risers if row["destination_layer"] == 0)),
            "OTTD_GROUND_NETWORK_TOWN_CONSUMER": str(town["id"]),
        })
    if power_case:
        lineage = delivered_owner["actual"]["original_delivered"] if recovery_case else delivered_owner
        active_env["OTTD_GROUND_NETWORK_POWER"] = str(lineage["power_meter"]["id"])
        active_env["OTTD_GROUND_NETWORK_PEER"] = str(lineage["participants"]["1"]["vehicle"])
    recovery_participants = None
    if recovery_case:
        assert identity(ROOT / HANDOFF)["sha256"] == HANDOFF_SHA
        recovery_participants = json.loads((ROOT / HANDOFF).read_text(encoding="utf-8"))["records"][configuration]["useful_participants"]
        for field in ("freight", "mine", "factory"):
            active_env["OTTD_GROUND_NETWORK_" + field.upper()] = str(recovery_participants[field])
    try:
        with logs["server"].open("wb") as server_sink, logs["client"].open("wb") as client_sink:
            server_env = dict(os.environ, **active_env, OTTD_GROUND_NETWORK_ROLE="server", OTTD_GROUND_NETWORK_INPUT=str(source), OTTD_GROUND_NETWORK_CAPSULE=str(capsule))
            server = start_native(peers["server"], [executable, tag], cwd=ROOT / "build", env=server_env,
                                  stdout=server_sink, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                                  creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            children.append(server)
            wait_ready(server, logs["server"])
            company_selector = 1 if company_commands else 255
            client_env = dict(os.environ, **active_env, OTTD_GROUND_NETWORK_ROLE="client", OTTD_GROUND_NETWORK_ADDRESS=f"127.0.0.1:{port}#{company_selector}", OTTD_GROUND_NETWORK_CAPSULE=str(capsule))
            client = start_native(peers["client"], [executable, tag], cwd=ROOT / "build", env=client_env,
                                  stdout=client_sink, stderr=subprocess.STDOUT,
                                  creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=startup)
            children.append(client)
            if pause_case:
                wait_paused(server, client, logs)
                server.stdin.write(b"unpause\n")
                server.stdin.flush()
            first_ack, last_ack = wait_continuation(server, client, logs["client"])
            assert client.wait(timeout=100 if recovery_case else 20) == 0, logs["client"]
            server.stdin.write(b"quit\n")
            server.stdin.flush()
            assert server.wait(timeout=15) == 0, logs["server"]
    finally:
        # Only this invocation's owned handles are reaped. Even a comparison
        # failure cannot leave a hidden game running or stop another process.
        for process in reversed(children):
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        assert identity(source) == source_ref
        assert identity(executable) == binary
    native = {}
    commands = {}
    states = {}
    views = {}
    command_states = {}
    work = {}
    joins = {}
    frame_schedule = {}
    pauses = {}
    power = {}
    recovery = {}
    thermal = {}
    # Each independently executed Catch case must close cleanly and retain its
    # complete command stream. An ACK pass cannot hide observer assertion failure.
    for role, peer in peers.items():
        output = read_log(logs[role])
        frame_schedule[role] = list(FRAMES)
        # Clock-mode peers share the same actual initial/next writer as
        # delivered worlds;their later samples use unpaused tick offsets.
        if delivered or clock_case:
            joins[role] = re.findall(r"GROUND_NETWORK_JOIN role=\w+ stage=(joined(?:-next)?) frame=(\d+) tick=(\d+)", output)
            assert [row[0] for row in joins[role]] == ["joined", "joined-next"]
            # Actual load duration cannot choose or omit a simulation sample.
            # Both peers anchor the same future offsets to their agreed first
            # unpaused frame,allowing a slow fresh snapshot to finish honestly.
            start = int(joins[role][0][1])
            frame_schedule[role] = [start + offset for offset in (128, 256, 384, 512)]
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
        assert counts is not None, logs[role]
        native[role] = {"log": identity(logs[role]), "assertions": int(counts[1]), "cases": int(counts[2])}
        state_log = context.logs / f"trace-{role}-commands.log"
        state_log.write_bytes((peer.runtime_dir / "save/autosave/commands-out.log").read_bytes())
        commands[role] = re.findall(r"\] (cmd[f]?: [^\r\n]+)", read_log(state_log))
        states[role] = re.findall(r"GROUND_NETWORK_STATE role=\w+ frame=(\d+) tick=(\d+) rng0=(\d+) rng1=(\d+) observations=(\d+)", output)
        # Pause and clock policies must compare the same paid opportunities,
        # independent of join duration or paused network-frame advancement.
        if pause_case or clock_case or power_case or thermal_case:
            # Native simulation ticks,not advancing paused network frames,
            # select these four actual samples. Exact peer frame equality is
            # still required below,after each agreed command has executed.
            source_clock = active_owner["clock"] if clock_case else delivered_owner["clock"]
            offsets = (128, 1024, 2048, 3072) if recovery_case else (128, 256, 384, 512)
            assert [int(row[1]) for row in states[role]] == [source_clock + offset + 1 for offset in offsets]
            frame_schedule[role] = [int(row[0]) for row in states[role]]
        if pause_case:
            pauses[role] = [{key: value for key, value in (field.split("=", 1) for field in row.split())}
                            for row in re.findall(r"GROUND_NETWORK_PAUSE role=\w+ ([^\r\n]+)", output)]
            assert [row["stage"] for row in pauses[role]] == ["paused-a", "paused-b"]
        if loss_case:
            power[role] = {stage: [{key: int(value) for key, value in (field.split("=", 1) for field in row.split())}
                                  for row in re.findall(rf"GROUND_NETWORK_POWER_{stage.upper()} role=\w+ ([^\r\n]+)", output)]
                           for stage in ("closed", "held")}
            assert len(power[role]["closed"]) == 1
            assert len(power[role]["held"]) == 4
        if recovery_case:
            recovery[role] = [{key: int(value) for key, value in (field.split("=", 1) for field in row.split())}
                              for row in re.findall(r"GROUND_NETWORK_POWER_RECOVERY role=\w+ ([^\r\n]+)", output)]
            assert len(recovery[role]) == 6
        if thermal_case:
            thermal[role] = [{key: int(value) for key, value in (field.split("=", 1) for field in row.split())}
                             for row in re.findall(r"GROUND_NETWORK_THERMAL role=\w+ ([^\r\n]+)", output)]
            assert len(thermal[role]) == 6
        views[role] = re.findall(r"GROUND_NETWORK_VIEW role=\w+ frame=(\d+) layer=(\d+) detail=(true|false) animation=(true|false)", output)
        assert [int(row[0]) for row in views[role]] == frame_schedule[role]
        assert all(int(row[1]) == (0 if role == "server" else 1) for row in views[role])
        if role == "client":
            assert all(row[2:] == ("false", "false") for row in views[role])
        if company_commands:
            command_states[role] = re.findall(r"GROUND_NETWORK_COMMAND_STATE role=\w+ frame=(\d+) vehicle=(\d+) owner=(\d+) stopped=(true|false) name=([^\r\n]*)", output)
            assert [int(row[0]) for row in command_states[role]] == list(FRAMES)
            assert all(int(row[1]) == capsule for row in command_states[role])
            assert all(row[2] == "0" for row in command_states[role])
            assert [row[3] for row in command_states[role]] == ["false", "true", "false", "false"]
        if active:
            work[role] = [{key: int(value) for key, value in (field.split("=", 1) for field in row.split())}
                          for row in re.findall(r"GROUND_NETWORK_WORK role=\w+ ([^\r\n]+)", output)]
    if company_commands:
        assert command_states["server"] == command_states["client"]
    assert states["server"] == states["client"], "Actual native same-frame clocks/RNG differ"
    assert [int(row[0]) for row in states["client"]] == frame_schedule["client"]
    assert commands["client"], "No actual agreed native commands were executed"
    assert commands["server"][-len(commands["client"]):] == commands["client"], "Actual native execution payload/authority differs"
    incoming = re.findall(r"Client::ReceiveServerCommand\(\): cmd=(\d+), frame=(\d+)", read_log(logs["client"]))
    assert len(incoming) == len(commands["client"])
    checkpoints = None
    if active:
        assert work["server"] == work["client"], "Actual reconstructed/continued paid-work owners differ"
        assert len(work["client"]) == 6
        assert work["client"][0]["tick"] == active_owner["clock"] + 1
        assert work["client"][0]["cursor"] == active_owner["job"]["cursor"]
        assert work["client"][0]["phase"] == active_owner["project"]["phase"]
        checkpoints = [("joined", work["client"][0]["frame"]), ("joined-next", work["client"][1]["frame"]),
                       *[(f"f{frame}", frame) for frame in frame_schedule["client"]]]
        if clock_case:
            assert joins["server"] == joins["client"]
            assert [int(row[2]) for row in joins["client"]] == [active_owner["clock"] + 1, active_owner["clock"] + 2]
    if delivered:
        assert joins["server"] == joins["client"]
        assert [row[0] for row in joins["client"]] == ["joined", "joined-next"]
        assert int(joins["client"][0][2]) == delivered_owner["clock"] + 1
        assert int(joins["client"][1][2]) == delivered_owner["clock"] + 2
        assert int(joins["client"][1][1]) == int(joins["client"][0][1]) + 1
        checkpoints = [(row[0], int(row[1])) for row in joins["client"]] + [(f"f{frame}", frame) for frame in frame_schedule["client"]]
    if pause_case:
        # Paused peers may be polled at different network frames while the
        # actual serialized gameplay tick is frozen. Keep both observed frame
        # numbers explicit;never label them one agreed network frame.
        checkpoints += [(stage, {role: int(pauses[role][index]["frame"]) for role in peers})
                        for index, stage in enumerate(("paused-a", "paused-b"))]
    frames = compare_frames(context, checkpoints)
    report = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md17B;actual native peers,all126 pure repeated chunks,125 exact same-frame chunks and every gameplay PATS field equal;seven native NoNetworkSync units remain local;real command bytes/authority and received agreed frames retained",
              "configuration": configuration, "focus": focus, "tag": tag, "native_version": 434, "controller": identity(__file__),
              "observer": identity(ROOT / "src/tests/ground_network_fixture.h"), "binary": binary,
              "sources": [identity(ROOT / path) for path in (*SOURCES, "src/saveload/settings_sl.cpp", "src/table/settings/locale_settings.ini")], "handoff": handoff,
              "original_input": source_ref, "native": native, "states": states, "views": views, "frames": frames,
              "command_states": command_states, "original_capsule": capsule,
              "work": work, "original_work": active_owner,
              "joins": joins, "original_delivered": delivered_owner,
              "pauses": pauses, "power": power, "recovery": recovery,
              "thermal": thermal,
              "recovery_participants": recovery_participants,
              "scenario_observer": identity(ROOT / ("src/tests/ground_network_thermal_fixture.h" if thermal_case else "src/tests/ground_network_recovery_fixture.h" if recovery_case else "src/tests/ground_network_power_fixture.h")) if power_case or thermal_case else None,
              "commands": commands, "client_received_agreed_commands": incoming,
              "first_ack": first_ack, "last_ack": last_ack, "acknowledged_frames": last_ack - first_ack,
              "diagnostics": {role: identity(context.logs / f"trace-{role}-commands.log") for role in peers},
              "content": identity(ROOT / "build/baseset/openttd.grf"),
              "configs": {role: identity(peer.config_path) for role, peer in peers.items()},
              "children_reaped": all(process.poll() is not None for process in children),
              "remaining": "Active excavation/wagon waits,additional player-command/failure/time scenarios and orbital combined acceptance remain separate."}
    target = context.output("reports", f"paid-ground-{configuration}-{focus}-trace.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "frames": frame_schedule["client"], "complete_peer_chunks_equal": 125}), flush=True)
    return target


def main():
    """Select one bounded real peer trace scenario without running a full suite.

    @return None; first missing owner,purity,command or same-frame mismatch refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=SCENARIO_TAGS, default="baseline")
    args = parser.parse_args()
    run_one(args.config, args.focus)


if __name__ == "__main__":
    main()
