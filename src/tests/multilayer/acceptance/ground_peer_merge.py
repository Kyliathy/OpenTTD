"""Qualify actual merged peer transport against exact retained paid originals.

@file ground_peer_merge.py Separate homes, native join acknowledgments and complete matched saves.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
No old executable, assigned stock or private simulation restoration supplies proof.
"""
import json
import os
from pathlib import Path
import re
import socket
import struct
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from artifact_rebuild import identity
from native_build import verify_source_epoch
from runner import start_native
from normal_ground_merge import checked, read, run_case, CONTENT_SHA
from savegame_tables import complete_world, compare_merged_worlds, saved_owner_table

MANIFEST = "src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/ground-peer-originals.json"
LOCAL_UNITS = ("locale.units_velocity", "locale.units_velocity_nautical", "locale.units_power",
    "locale.units_weight", "locale.units_volume", "locale.units_force", "locale.units_height")


def select_original(scenario, root=ROOT):
    """Resolve one independent original without promoting its historical result.

    @param scenario Exact active/tube/thermal original selection.
    @param root Checkout containing retained originals and immutable handoffs.
    @return Verified original source and historical owner observations.
    """
    row = json.loads((root / MANIFEST).read_text())["scenarios"][scenario]
    handoff = read(row["historical_handoff"], root)
    # The binding writer is explicitly a prepared native component. It must
    # match its own original checkpoint rather than acquiring paid bootstrap
    # provenance from the separate genuine-world records.
    original = handoff["checkpoints"][0] if row.get("classification") == "prepared_native_binding" else handoff["records"][scenario]
    expected = row["original"]["writer_checkpoint"] if row.get("classification") == "prepared_native_binding" else row["original"]
    if original != expected or row["source"]["sha256"] != row["original_source"]["sha256"]:
        raise ValueError("peer original differs from independent sealed handoff")
    checked(row["source"], root)
    return row


def migrate_original(scenario, build_reference, root=ROOT):
    """Reconstruct one original through the existing digest-bound full/copied loader.

    @param scenario Exact original selected by the independent retained contract.
    @param build_reference Actual current selected Tube build.
    @param root Checkout containing current owners and retained inputs.
    @return Current migration report with pure repeated441outputs and untouched finance.
    """
    selected = select_original(scenario, root)
    build = read(build_reference, root)
    context = create_run_context("space", "machinery-lineage-original", run_id="merge67-migrate-" + uuid.uuid4().hex[:10], root=root)
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("machinery_lineage_test\n")
    originals = json.loads((root / "src/tests/fixtures/saveload/lineage-originals.json").read_text())["observations"]
    bootstrap = next(row for row in originals if row["origin"] == "B" and row["version"] == 427)
    bootstrap_ref = {key: bootstrap[key] for key in ("path", "sha256")}
    output = context.saves / "peer-original-441.sav"
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment.update(OTTD_TEST_CONFIGURATION=build["configuration"], OTTD_MACHINERY_SOURCE=str(checked(selected["source"], root)),
        OTTD_MACHINERY_DIGEST=selected["source"]["sha256"], OTTD_MACHINERY_BOOTSTRAP_INPUT=str(checked(bootstrap_ref, root)),
        OTTD_MACHINERY_OUTPUT=str(output))
    process, text, terminal = run_case(context, build["targets"]["openttd_tube_test"], "[.machinery-lineage-native]", environment)
    return seal_migration(scenario, build_reference, context, process, terminal, root)


def seal_migration(scenario, build_reference, context, process, terminal, root=ROOT):
    """Seal an actually completed migration without repeating native work for a report repair.

    @param scenario Exact original scenario, checked against its independent receipt.
    @param build_reference Actual producing build; never relabel an earlier native epoch.
    @param context Actual original migration context and its two completed native saves.
    @param process Actual successful native process identity.
    @param terminal Actual passing native terminal identity from that process.
    @param root Checkout containing these retained/generated inputs.
    @return Sealed actual migration with cosmetic afterload changes explicitly separated.
    """
    selected = select_original(scenario, root)
    output = context.saves / "peer-original-441.sav"
    actual = read(process, root)
    producing = read(build_reference, root)
    if actual["exit_status"] != 0 or actual["executable"] != producing["targets"]["openttd_tube_test"]:
        raise ValueError("migration process/image provenance differs")
    if "Passed 1 test case" not in checked(terminal, root).read_text():
        raise ValueError("migration native terminal did not pass")
    families = compare_merged_worlds(output, Path(str(output) + ".again.sav"))
    before, after = saved_owner_table(checked(selected["source"], root), b"PLYR"), saved_owner_table(output, b"PLYR")
    cosmetic = {}
    # Original donor faces can name an absent empty style. The existing native
    # afterload owner randomizes that cosmetic face; every financial, company
    # and expense field still compares exactly, with no blanket PLYR exclusion.
    for company, row in before.items():
        if row["face_style"] == "":
            replacement = bytes.fromhex(after[company]["face_style"]).decode()
            if replacement not in ("default/face0", "default/face1", "default/face2", "default/face3"):
                raise ValueError("unrecognized native cosmetic fallback")
            cosmetic[company] = {key: dict(original=row.pop(key), merged=after[company].pop(key)) for key in ("face", "face_style")}
    if before != after:
        raise ValueError("peer migration changed original finance/company state")
    report = context.reports / "peer-migration.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, scenario=scenario,
        original=selected, build=build_reference, source=identity(output, root), repeat=identity(Path(str(output) + ".again.sav"), root),
        families=families, cosmetic_afterload=cosmetic, process=process, terminal=terminal), indent=2) + "\n")
    return identity(report, root)


def text(path):
    """Read currently available owned child output.

    @param path Exact invocation log, possibly awaiting its first write.
    @return Available text without strict platform decoding.
    """
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def wait_for(predicate, children, timeout, description):
    """Wait for an actual native witness within this scenario's wall deadline.

    @param predicate Read-only witness predicate; elapsed time never constitutes success.
    @param children Owned peers which must remain alive until the witness.
    @param timeout Maximum seconds for this specific phase.
    @param description Exact missing native phase for a useful refusal.
    """
    deadline = time.monotonic() + timeout
    # Native readiness/ACK witnesses, rather than guessed pauses, admit each
    # subsequent peer phase. Only invocation-owned handles are observed.
    while not predicate():
        if time.monotonic() >= deadline or any(child.poll() is not None for child in children):
            raise ValueError("actual peer phase failed: " + description)
        time.sleep(0.1)


def reject_original(port, revision):
    """Attempt the real pre-authentication join packet used by original peers.

    @param port Current owned merged server's loopback listener.
    @param revision Explicit original source revision lacking the merged ABI suffix.
    @return Exact rejected packet bytes; no old executable is a dependency.
    """
    payload = bytes([2]) + revision.encode("ascii") + b"\0" + struct.pack("<I", 0)
    with socket.create_connection(("127.0.0.1", port), timeout=5) as peer:
        peer.sendall(struct.pack("<H", len(payload) + 2) + payload)
        response = peer.recv(64)
    if response != bytes([4, 0, 3, 8]):
        raise ValueError("original protocol was not refused with WrongRevision: " + response.hex())
    return dict(revision=revision, request=payload.hex(), response=response.hex(), refused_before_authentication=True)


def compare_frames(context, root=ROOT):
    """Compare every original native family at each actual agreed opportunity.

    @param context Shared invocation containing independently written server/client files.
    @param root Checkout with native setting declarations.
    @return Complete matched-frame comparisons; only seven declared local unit preferences may differ.
    """
    records = []
    settings_source = (root / "src/table/settings/locale_settings.ini").read_text()
    # Each native save is checked against its consecutive pure repeat before
    # independently comparing peers. No incomplete feature hash is substituted.
    for server in sorted(context.saves.glob("ground-server-*.sav")):
        if server.stem.endswith("-repeat"):
            continue
        client = server.with_name(server.name.replace("ground-server-", "ground-client-", 1))
        compare_merged_worlds(server, server.with_name(server.stem + "-repeat.sav"))
        compare_merged_worlds(client, client.with_name(client.stem + "-repeat.sav"))
        first, second = complete_world(server), complete_world(client)
        changed = [name for name in first[1] if first[1][name] != second[1][name]]
        if any(name != b"PATS" for name in changed):
            raise ValueError("actual peer owner mismatch " + server.name + ": " + repr(changed))
        one, two = saved_owner_table(server, b"PATS"), saved_owner_table(client, b"PATS")
        if one.keys() != two.keys():
            raise ValueError("peer native settings owner mismatch")
        local = {}
        # Native NoNetworkSync preferences alone remain client-local. Each
        # precise declaration is checked; no settings namespace is ignored.
        for key in LOCAL_UNITS:
            block = re.search(r"var\s*=\s*" + re.escape(key) + r"\s*\n(.*?)(?=\n\[|\Z)", settings_source, re.S)
            if block is None or "SettingFlag::NoNetworkSync" not in block[1]:
                raise ValueError("unrecognized local preference: " + key)
            local[key] = dict(server=one[0].pop(key), client=two[0].pop(key))
        if one != two:
            raise ValueError("synchronized peer settings differ")
        records.append(dict(server=identity(server, root), client=identity(client, root), families=128, local_units=local))
    if len(records) != 6:
        raise ValueError("expected actual joined/next/four later full peer checkpoints")
    return records


def execute(scenario, build_reference, migration_reference, mode, root=ROOT):
    """Run actual isolated current peers and join/command/full-state qualification.

    @param scenario Exact retained active, loaded tube or thermal world selection.
    @param build_reference Actual current selected Tube build and source identity.
    @param migration_reference Same-build pure migration of the exact selected original.
    @param mode Active, supply, power or thermal native observer scope.
    @param root Checkout containing current executable and retained owners.
    @return Current actual connected-peer report with clean exits and all owned handles reaped.
    """
    build, migration = read(build_reference, root), read(migration_reference, root)
    # A completed migration supplies ordinary saved bytes, not a current image.
    # Its actual producing epoch stays bound while current peers independently
    # load the same exact original successor through the current executable.
    if migration["scenario"] != scenario or migration["original"] != select_original(scenario, root):
        raise ValueError("peer original selection mismatch")
    epoch = read(build["source_epoch"], root)
    verify_source_epoch(epoch, root, build["tool_profile"]["path"])
    source = migration["source"]
    context = create_run_context("network", "paid-ground", run_id="merge67-" + mode + "-" + uuid.uuid4().hex[:10], root=root)
    peers = {role: context.for_process(role) for role in ("server", "client")}
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0)); port = listener.getsockname()[1]
    base = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    base["OTTD_TEST_CONFIGURATION"] = build["configuration"]
    original = migration["original"]["original"]
    base["OTTD_GROUND_NETWORK_TICK"] = str(original["clock"])
    if mode == "active":
        base.update(OTTD_GROUND_NETWORK_PROJECT=str(original["project"]["id"]),
            OTTD_GROUND_NETWORK_CURSOR=str(original["job"]["cursor"]), OTTD_GROUND_NETWORK_PHASE=str(original["project"]["phase"]))
    if mode == "thermal":
        plants, risers = original["generators"].values(), original["risers"].values()
        town = next(row for row in original["utility"].values() if row["type"] == 1 and row["town_terminal"] != 65535)
        base.update(OTTD_GROUND_NETWORK_GENERATOR=str(next(row["industry"] for row in plants if row["model"] == 1)),
            OTTD_GROUND_NETWORK_VENT_GENERATOR=str(next(row["industry"] for row in plants if row["model"] == 0)),
            OTTD_GROUND_NETWORK_FIRST_RISER=str(next(row["industry"] for row in risers if row["layer"] == 3)),
            OTTD_GROUND_NETWORK_SECOND_RISER=str(next(row["industry"] for row in risers if row["destination_layer"] == 0)),
            OTTD_GROUND_NETWORK_TOWN_CONSUMER=str(town["id"]))
    if mode == "power":
        base.update(OTTD_GROUND_NETWORK_POWER=str(original["power_meter"]["id"]),
            OTTD_GROUND_NETWORK_CAPSULE=str(original["participants"]["0"]["vehicle"]),
            OTTD_GROUND_NETWORK_PEER=str(original["participants"]["1"]["vehicle"]))
    logs = {role: context.evidence / (mode + "-" + role + ".txt") for role in peers}
    # Each peer has its own configuration, script and writable application home.
    # Both use the same selected canonical bytes and ordinary native simulation.
    for role, peer in peers.items():
        peer.prepare_runtime()
        grf = peer.runtime_dir / "baseset/openttd.grf"
        if identity(grf, root)["sha256"] != CONTENT_SHA:
            raise ValueError("peer content differs from accepted merged container")
        with peer.config_path.open("a") as stream:
            stream.write(f"\n[network]\nserver_port = {port}\nserver_bind_addresses = 127.0.0.1\nserver_game_type = local\nclient_name = Merge {role}\nmin_active_clients = 0\npause_on_join = true\n[gui]\nautosave_on_exit = false\n")
    startup = "ground_network_work_ready\n" if mode in ("active", "supply") else ""
    (peers["server"].runtime_dir / "scripts/game_start.scr").write_text(startup + 'echo "MERGED_GROUND_SERVER_READY"\n')
    (peers["server"].runtime_dir / "scripts/on_server_connect.scr").write_text("unpause\n")
    image = build["targets"]["openttd_tube_test"]
    tag = "[.merged-ground-peer-" + mode + "]"
    children, refusals = [], []
    started = time.monotonic()
    try:
        with logs["server"].open("wb") as host_sink, logs["client"].open("wb") as client_sink:
            environment = dict(base, OTTD_GROUND_NETWORK_ROLE="server", OTTD_GROUND_NETWORK_INPUT=str(checked(source, root)))
            server = start_native(peers["server"], [str(checked(image, root)), tag], cwd=root / "build", env=environment,
                stdout=host_sink, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
            children.append(server)
            wait_for(lambda: "MERGED_GROUND_SERVER_READY" in text(logs["server"]), children, 30, "server exact original loaded")
            if mode == "supply":
                # Original ABI handshakes must fail before authentication or map
                # transfer; no archived executable certifies current behavior.
                refusals = [reject_original(port, revision) for revision in ("d0da7e9d76", "75ea75718f")]
            address = f"127.0.0.1:{port}#" + ("1" if mode == "supply" else "255")
            environment = dict(base, OTTD_GROUND_NETWORK_ROLE="client", OTTD_GROUND_NETWORK_ADDRESS=address)
            client = start_native(peers["client"], [str(checked(image, root)), tag], cwd=root / "build", env=environment,
                stdout=client_sink, stderr=subprocess.STDOUT)
            children.append(client)
            wait_for(lambda: client.poll() is not None, [server], 70, "native client completed while host remained healthy")
            if client.returncode != 0:
                raise ValueError("actual native client failed: " + str(logs["client"]))
            server.stdin.write(b"quit\n"); server.stdin.flush()
            if server.wait(timeout=10) != 0:
                raise ValueError("actual native server failed: " + str(logs["server"]))
    finally:
        # Reap only returned native handles on every failure/success path.
        for child in reversed(children):
            if child.poll() is None:
                child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill(); child.wait(timeout=5)
        # Persist actual returned handles and exits before any report parser can
        # fail, allowing a pure report repair without repeating native work.
        (context.reports / "peer-processes.json").write_text(json.dumps(dict(_documentation=__doc__,
            image=image, source=source, children=[dict(pid=child.pid, argv=child.args, exit_status=child.returncode,
            reaped=child.poll() is not None) for child in children], wall_seconds=time.monotonic()-started,
            original_protocol_refusals=refusals), indent=2) + "\n")
    checked(source, root); checked(image, root)
    verify_source_epoch(epoch, root, build["tool_profile"]["path"])
    return seal_peers(scenario, build_reference, migration_reference, mode, context, logs, refusals,
        time.monotonic() - started, root)


def seal_peers(scenario, build_reference, migration_reference, mode, context, logs, refusals, wall_seconds, root=ROOT):
    """Seal completed actual peer processes without repeating them for report repairs.

    @param scenario Exact original selection.
    @param build_reference Actual producing peer build, preserved at its own epoch.
    @param migration_reference Actual original migration supplying the joined save.
    @param mode Native observer scope used by both peers.
    @param context Actual shared native invocation containing complete snapshots.
    @param logs Exact independently written native server/client terminals.
    @param refusals Actual original-protocol refusal observations, empty outside supply scope.
    @param wall_seconds Measured native phase duration, excluding compilation; None when an older completed run did not record it.
    @param root Checkout containing the original evidence.
    @return Exact complete actual peer report identity.
    """
    build, migration = read(build_reference, root), read(migration_reference, root)
    image, source = build["targets"]["openttd_tube_test"], migration["source"]
    peers = {role: context.for_process(role) for role in ("server", "client")}
    processes = context.reports / "peer-processes.json"
    process_ref = identity(processes, root) if processes.is_file() else None
    if process_ref is not None:
        native = read(process_ref, root)
        if len(native["children"]) != 2 or any(child["exit_status"] != 0 or not child["reaped"] for child in native["children"]):
            raise ValueError("actual peer process exits differ")
    actual = {role: text(path) for role, path in logs.items()}
    acknowledgments = [int(value) for value in re.findall(r"Sent ACK at (\d+)", actual["client"])]
    if not acknowledgments or acknowledgments[-1] - acknowledgments[0] < 400:
        raise ValueError("missing actual synchronized native continuation ACK interval")
    if "Client::ReceiveServerMapDone()" not in actual["client"] or "Client::status = ACTIVE" not in actual["client"]:
        raise ValueError("missing actual native snapshot join")
    # Passing tests cannot conceal native desync diagnostics or an unclean exit.
    for role, output in actual.items():
        if "Sync error detected" in output or "sync_err:" in output or not re.search(r"All tests passed|Passed 1 test case", output):
            raise ValueError("actual peer terminal rejected: " + role)
    comparisons = compare_frames(context, root)
    checked(source, root)
    report = context.reports / (mode + "-peers-" + uuid.uuid4().hex[:8] + ".json")
    classification = "actual_connected_prepared_component_peers" if mode == "geothermal" else "actual_connected_paid_original_peers"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, classification=classification,
        scenario=scenario, mode=mode, build=build_reference, migration=migration_reference, source=source, image=image,
        peer_homes={role: peer.runtime_dir.relative_to(root).as_posix() for role, peer in peers.items()},
        configs={role: identity(peer.config_path, root) for role, peer in peers.items()},
        diagnostics={role: identity(path, root) for role, path in logs.items()},
        first_ack=acknowledgments[0], last_ack=acknowledgments[-1], comparisons=comparisons,
        original_protocol_refusals=refusals, wall_seconds=wall_seconds, processes=process_ref,
        all_owned_children_reaped=True), indent=2) + "\n")
    return identity(report, root)
