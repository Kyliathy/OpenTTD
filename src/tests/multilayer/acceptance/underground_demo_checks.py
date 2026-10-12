"""Prepare and verify the playable underground checkpoint tour through native game loading.

@file underground_demo_checks.py
@see docs/zz-reach2/upgrades/2026-10/d2um-underground-mines.md
Original worlds and player saves remain immutable. Published demo snapshots are
ordinary paused saves; preparation floor/materials are declared in their manifest.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from runner import current_run_context, run_native, start_native, console_executable, collect_native_screenshots
from savegame_plane_digest import read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS
from native_vehicle_projection import legacy_vehicle_projection

DESTINATION = ROOT / "src/tests/fixtures/demos/underground"
MANIFEST = DESTINATION / "demo-checkpoints.json"
GAME = ROOT / "build/RelWithDebInfo/openttd.exe"


def identity(path):
    """Return the exact repository-relative file identity."""
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def owners(path):
    """Read authoritative saved bytes without creating or changing any game state."""
    version, stream = read_stream(path)
    reader, result = Reader(stream), {}
    selected = {b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"EXJS", b"XJOB", b"XJRT", b"XJTL", b"XPAS",
                b"TBMH", b"TBMP", b"TBMS", b"TBXH", b"TBXA", b"TBXS", b"TBXW", b"VEHS", b"CAPA", b"ORDR",
                b"INDY", b"LMNH", b"LMNR", b"GCBH", b"GCBD", b"GCSH", b"GCSD", b"UPLH", b"UPLD"}
    while True:
        start = reader.pos
        name = reader.take(4)
        if name == bytes(4):
            break
        mode = reader.byte()
        kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            reader.take(length)
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise ValueError("unsupported native save framing")
        if name in selected:
            result[name] = stream[start:reader.pos]
            if name == b"VEHS":
                result[name] = legacy_vehicle_projection(result[name][5:], version)
    assert b"GEOS" in result and b"INDY" in result, "demo lacks native geology/industry owners"
    return version, result


def resave(context, source, name, configuration="RelWithDebInfo"):
    """Pause and resave through the requested actual executable in an isolated data home."""
    output = context.saves / f"{name}.sav"
    assert not output.exists(), output
    original = identity(source)
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text(f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    log = context.logs / f"{name}.log"
    executable = console_executable(context, ROOT / "build" / configuration / "openttd.exe")
    with log.open("wb") as sink:
        run_native(context, [str(executable), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X",
                             "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=90)
    assert output.is_file(), log
    assert identity(source) == original, "source world changed"
    before_version, before = owners(source)
    version, after = owners(output)
    changed = {key for key, value in before.items() if after.get(key) != value}
    migrated = []
    if changed and before_version == 399 and version in (403, 404, 405, 406, 407, 408):
        # These older TBM snapshots contain no utility accounts. Version403
        # adds the paid-riser/share/parent descriptors and a zero share issuer;
        # all original epoch, IDs, quantities and phase flags must stay exact.
        assert changed == {b"UPLH", b"UPLD"}, "unexpected399 migration"
        old_header, new_header = before[b"UPLH"][8:], after[b"UPLH"][8:]
        assert len(old_header) == 104 and len(new_header) == 112
        assert int.from_bytes(old_header[:8], "big") == 7
        assert int.from_bytes(new_header[:8], "big") == 10
        assert old_header[8:] == new_header[8:104] and new_header[104:] == bytes(8)
        assert all(int.from_bytes(old_header[index * 8:(index + 1) * 8], "big") == 0 for index in (5, 6, 7, 8, 11))
        def descriptor_only(raw):
            reader = Reader(raw[5:])
            header = reader.take(reader.gamma() - 1)
            assert reader.gamma() == 0 and reader.pos == len(raw) - 5, "migration unexpectedly contains utility accounts"
            return header
        assert descriptor_only(after[b"UPLD"]) == descriptor_only(before[b"UPLD"])[:-1] + b"\x04\x05riser\x06\x05share\x06\x06parent\x00"
        migrated = sorted(key.decode() for key in changed)
    else:
        assert not changed, "ordinary resave changed a native owner"
    return output, {"source": original, "source_version": before_version, "version": version,
                    "unchanged_owner_chunks": sorted(key.decode() for key in before if key not in changed),
                    "verified_empty_utility_schema_migration": migrated, "log": log.relative_to(ROOT).as_posix()}


def native(context):
    """Generate the actual paid mine freight loop and current collector art stages."""
    mines = context.for_feature("mines", "underground-demo")
    mines.prepare_runtime()
    (mines.runtime_dir / "scripts/game_start.scr").write_text("underground_demo\n", encoding="utf-8")
    log = mines.logs / "prepare.log"
    with log.open("wb") as sink:
        run_native(mines, [str(ROOT / "build/RelWithDebInfo/openttd_test.exe"), "[.underground-demo]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"UNDERGROUND_DEMO (.+)", evidence)
    assert match, log
    fields = dict(item.split("=", 1) for item in match[1].split())
    import collector_runtime_checks as collector
    collectors = context.for_feature("geothermal", "collector-art")
    collector.native(collectors, "RelWithDebInfo")
    report = {"mine_directory": mines.directory.relative_to(ROOT).as_posix(), "mine_log": log.relative_to(ROOT).as_posix(),
              "mine": fields, "collector_directory": collectors.directory.relative_to(ROOT).as_posix()}
    target = context.reports / "native-inputs.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Native preparation: {target.relative_to(ROOT)}", flush=True)


def prepare(context, inputs):
    """Publish fresh current-format snapshots after two ordinary loads and exact owner comparisons."""
    data = json.loads(inputs.read_text(encoding="utf-8"))
    mine_dir = ROOT / data["mine_directory"]
    collector_dir = ROOT / data["collector_directory"]
    collector_report = json.loads((collector_dir / "reports/native-RelWithDebInfo.json").read_text(encoding="utf-8"))
    collector_centre = collector_report["native_centre"]
    collector_x, collector_y = collector_centre % 64, (collector_centre // 64) % 64
    haulage = ROOT / "src/tests/fixtures/haulage"
    scenes = [
        ("tbm-work", haulage / "planning/underground-demo-tbm-399.sav", "Underground", 13, 10),
        ("tbm-plan", haulage / "planning/underground-demo-planning-399.sav", "Underground", 13, 10),
        ("tbm-exit", haulage / "far-exit/underground-demo-exit-399.sav", "Underground", 13, 60),
        ("tbm-tunnel", haulage / "far-exit/underground-demo-tunnel-399.sav", "Underground", 13, 35),
        ("mine-build", mine_dir / "saves/underground-mine-construction.sav", "Underground", int(data["mine"]["mine_x"]), int(data["mine"]["mine_y"])),
        ("mine-haulage", mine_dir / "saves/underground-mine-haulage.sav", "Underground", int(data["mine"]["mine_x"]), int(data["mine"]["mine_y"])),
        ("collector-build", collector_dir / "saves/collector-construction-1.sav", "Deep Underground", collector_x, collector_y),
        ("collector-ready", collector_dir / "saves/collector-ready.sav", "Deep Underground", collector_x, collector_y),
    ]
    DESTINATION.mkdir(parents=True, exist_ok=True)
    records = []
    for name, source, layer, x, y in scenes:
        output, proof = resave(context, source, name)
        reloaded, reload_proof = resave(context, output, name + "-reloaded")
        assert owners(output) == owners(reloaded)
        assert proof["version"] == 403
        target = DESTINATION / f"{name}-403.sav"
        assert not target.exists(), "publish requires fresh retained snapshot names"
        shutil.copyfile(output, target)
        records.append({"name": name, "snapshot": identity(target), "layer": layer, "x": x, "y": y,
                        "paused": True, "ordinary_load": proof, "ordinary_reload": reload_proof})
        print(f"Prepared {name}: two ordinary loads; native owners unchanged", flush=True)
    manifest = {"owner": "docs/zz-reach2/upgrades/2026-10/d2um-underground-mines.md", "date": "2026-10-12",
                "source_commit": "0840083c4054", "game": identity(GAME), "native_inputs": identity(inputs),
                "mine_proof": data["mine"], "mine_fixture_floor": True, "mine_fixture_cargo": False,
                "collector_fixture_floor": True, "collector_fixture_materials": True,
                "transported_first_plant_bootstrap": False, "user_playtest": "pending", "checkpoints": records}
    assert not MANIFEST.exists()
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Retained checkpoint manifest: {MANIFEST.relative_to(ROOT)}", flush=True)


def capture(context, name):
    """Render one retained checkpoint with the ordinary graphical backend and verify its resave."""
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    scene = next(row for row in manifest["checkpoints"] if row["name"] == name)
    source = ROOT / scene["snapshot"]["path"]
    assert identity(source) == scene["snapshot"]
    context.prepare_runtime()
    # The map has64x64 cells per layer. scrollto uses the native world tile;
    # the saved camera deliberately uses Surface coordinates at reload.
    layer = 3 if scene["layer"] == "Deep Underground" else 1
    centre = layer * 4096 + scene["y"] * 64 + scene["x"]
    output = context.saves / f"{name}-graphical.sav"
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text(f'pause\nzoomto 1\nscrollto instant {centre}\nset savegame_format lzma\n'
                       f'save "{output.with_suffix("").as_posix()}"\nscreenshot giant "{name}"\n', encoding="utf-8")
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = subprocess.SW_HIDE
    executable = console_executable(context, GAME)
    log = context.logs / f"{name}-graphical.log"
    with log.open("wb") as sink:
        process = start_native(context, [str(executable), "-v", "win32", "-b", "8bpp-simple", "-s", "null", "-m", "null",
                                        "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)],
                               cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, startupinfo=info)
        try:
            pending = context.runtime_dir / "screenshot" / f"{name}.png"
            deadline = time.monotonic() + 80
            while time.monotonic() < deadline:
                assert process.poll() is None, log
                if pending.is_file() and output.is_file():
                    try:
                        with Image.open(pending) as raster:
                            raster.verify()
                        break
                    except OSError:
                        pass
                time.sleep(0.2)
            else:
                raise TimeoutError(f"ordinary graphical capture timed out: {log}")
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
    frames = collect_native_screenshots(context)
    assert owners(source)[1] == owners(output)[1], "graphical loading changed a native owner"
    assert identity(source) == scene["snapshot"]
    report = {"scene": name, "source": identity(source), "game": identity(GAME), "frames": [identity(p) for p in frames],
              "resave": identity(output), "native_owners_unchanged": True, "user_playtest": "pending"}
    if name.startswith("collector-"):
        from collector_runtime_checks import match_native_hardware
        inputs = json.loads((ROOT / manifest["native_inputs"]["path"]).read_text())
        stem = "collector-ready" if name == "collector-ready" else "collector-construction-1"
        native_image = ROOT / inputs["collector_directory"] / "images" / f"{stem}-zoom-2.png"
        report["native_hardware_match"] = match_native_hardware(native_image, frames[0])
    (context.reports / f"{name}-graphical.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    context.write_report(f"{name}-provenance.json", Path(__file__).relative_to(ROOT).as_posix(),
                         [*frames, output], [source], "RelWithDebInfo", "ordinary screenshot command")
    print(f"Ordinary graphical {name}: screenshot and unchanged native resave passed", flush=True)


def verify(context, configuration="RelWithDebInfo"):
    """Reload every retained snapshot and verify natural reserves, jobs, vehicles, cargo and orders."""
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    results = []
    for row in manifest["checkpoints"]:
        source = ROOT / row["snapshot"]["path"]
        assert identity(source) == row["snapshot"]
        _output, proof = resave(context, source, row["name"] + "-verified", configuration)
        parent = ROOT / row["ordinary_load"]["source"]["path"]
        original, retained = owners(parent)[1], owners(source)[1]
        changed = sorted(key.decode() for key, value in original.items() if retained.get(key) != value)
        assert changed == row["ordinary_load"]["verified_empty_utility_schema_migration"]
        results.append({"scene": row["name"], **proof, "parent_owners_preserved": True})
        print(f"Verified {row['name']}: {len(proof['unchanged_owner_chunks'])} native owner chunks", flush=True)
    report = {"configuration": configuration, "game": identity(ROOT / "build" / configuration / "openttd.exe"), "manifest": identity(MANIFEST), "checkpoints": results}
    target = context.reports / "retained-verification.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Verification: {target.relative_to(ROOT)}", flush=True)


def live(context, name, configuration):
    """Continue a retained mine or collector with the real simulation after full loading."""
    if name not in ("mine-haulage", "collector-build"):
        raise ValueError("live continuation requires mine-haulage or collector-build")
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    scene = next(row for row in manifest["checkpoints"] if row["name"] == name)
    source = ROOT / scene["snapshot"]["path"]
    assert identity(source) == scene["snapshot"]
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("underground_demo\n", encoding="utf-8")
    log = context.logs / f"{name}-live-{configuration}.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.underground-demo-reload]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True,
                   env=dict(os.environ, OTTD_UNDERGROUND_DEMO_SOURCE=str(source)))
    evidence = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"DEMO_RELOAD (.+)", evidence)
    assert match and "All tests passed" in evidence, log
    assert identity(source) == scene["snapshot"]
    report = {"scene": name, "configuration": configuration, "source": identity(source),
              "test_executable": identity(ROOT / "build" / configuration / "openttd_test.exe"),
              "outcome": dict(item.split("=", 1) for item in match[1].split()), "log": log.relative_to(ROOT).as_posix()}
    (context.reports / f"{name}-live-{configuration}.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Live {name}/{configuration}: {match[1]}", flush=True)


def main():
    """Run one bounded native preparation, migration suite or ordinary screenshot case."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("mode", choices=("native", "prepare", "capture", "verify", "live"))
    parser.add_argument("--inputs", type=Path)
    parser.add_argument("--scene")
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    args = parser.parse_args()
    context = current_run_context("acceptance", "underground-demo")
    if args.mode == "native":
        native(context)
    elif args.mode == "prepare":
        if args.inputs is None:
            parser.error("prepare requires the exact --inputs native report")
        prepare(context, args.inputs.resolve())
    elif args.mode == "capture":
        if args.scene is None:
            parser.error("capture requires an explicit --scene")
        capture(context, args.scene)
    elif args.mode == "verify":
        verify(context, args.configuration)
    else:
        if args.scene is None:
            parser.error("live requires an explicit --scene")
        live(context, args.scene, args.configuration)


if __name__ == "__main__":
    main()
