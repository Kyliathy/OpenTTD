"""Prepare, reload and graphically inspect the first playable Space preview.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md GroupP1
Retained underground/demo inputs are immutable. Only explicit preparation creates
predeployed survey records; ordinary loading never populates an older world.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import time
from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from runner import current_run_context, run_native, start_native, console_executable, collect_native_screenshots, save_native_capture
from savegame_plane_digest import read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS

DEST = ROOT / "src/tests/fixtures/demos/space"
MANIFEST = DEST / "demo-checkpoints.json"


def identity(path):
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def chunks(path):
    version, stream = read_stream(path)
    reader, result = Reader(stream), {}
    while True:
        name = reader.take(4)
        if name == bytes(4):
            break
        mode = reader.byte()
        kind = mode & 15
        start = reader.pos
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
            raise ValueError("unsupported native framing")
        assert name not in result
        result[name] = stream[start:reader.pos]
    return version, result


def orbital_bytes(saved):
    version, data = saved
    raw = data[b"ASTR"][3:]
    if version < 405:
        return raw
    count = int.from_bytes(raw[27:31], "big")
    return raw[:51] + b"".join(raw[51+i*40:51+i*40+36] for i in range(count)) + raw[51+count*40:]


def native(context, configuration, action, source=None):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_demo\n", encoding="utf-8")
    log = context.logs / f"native-{configuration}.log"
    environment = dict(os.environ, OTTD_SPACE_DEMO_ACTION=action)
    if source is not None:
        environment["OTTD_SPACE_DEMO_SOURCE"] = str(source)
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-demo]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, env=environment, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and "exact=true hidden=true reserves_unchanged=true" in evidence, log
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            assert raster.size == (640, 360)
            assert len(set(raster.get_flattened_data())) > 20
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    print(f"Space {action}/{configuration}: exact2000-tick continuation and hidden-view checks passed", flush=True)
    return log


def prepare(context):
    previous = json.loads(MANIFEST.read_text(encoding="utf-8")) if MANIFEST.exists() else None
    assert previous is None or "calendar_year" not in previous, "retained1990 demo already exists; preparation requires fresh snapshot names"
    source = ROOT / "src/tests/fixtures/demos/underground/mine-haulage-403.sav"
    original = identity(source)
    native(context, "RelWithDebInfo", "prepare")
    DEST.mkdir(parents=True, exist_ok=True)
    records = []
    for name in ("approach", "attached"):
        generated = context.saves / f"space-survey-{name}-1990-404.sav"
        version, data = chunks(generated)
        assert version >= 405 and b"ASTR" in data
        target = DEST / generated.name
        assert not target.exists()
        if name == "approach":
            _, before = chunks(source)
            owners = {b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"VEHS", b"CAPA", b"ORDL", b"INDY", b"LMNH", b"LMNR", b"UPLH", b"UPLD"}
            assert all(before[key] == data[key] for key in owners), "Space preparation changed an underground owner"
        shutil.copyfile(generated, target)
        records.append({"name": name, "snapshot": identity(target), "paused": True, "layer": "Outer Space"})
    assert identity(source) == original
    report = {"owner": "docs/zz-reach2/upgrades/2026-10/d3sp-space-survey.md", "date": "2026-10-12",
              "source": original, "source_commit": "c99b6f0d55e7", "save_version": 404, "calendar_year": 1990,
              "predeployed_survey_probes": 3, "reserve_extraction": False, "launch_center": False,
              "surface_descent": False, "preview_art": "code-authored native sprites; final Plan13 space families pending",
              "native_evidence": context.directory.relative_to(ROOT).as_posix(), "checkpoints": records, "user_playtest": "pending"}
    if previous is not None:
        report["previous_checkpoints"] = previous["checkpoints"]
    MANIFEST.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Published {MANIFEST.relative_to(ROOT)}", flush=True)


def ordinary(context, scene, configuration, graphical=False, inspector=False):
    source = ROOT / scene["snapshot"]["path"]
    assert identity(source) == scene["snapshot"]
    context.prepare_runtime()
    output = context.saves / "ordinary-resave.sav"
    text = f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\n'
    if graphical:
        text += 'space_view\nzoomto 1\nscreenshot viewport no_con "space-inspector"\n' if inspector else 'zoomto 1\nscreenshot giant "space-field"\n'
    else:
        text += "quit\n"
    (context.runtime_dir / "scripts/game_start.scr").write_text(text, encoding="utf-8")
    executable = console_executable(context, ROOT / "build" / configuration / "openttd.exe")
    command = [str(executable), "-v", "win32" if graphical else "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null",
               "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)]
    log = context.logs / "ordinary.log"
    with log.open("wb") as sink:
        if graphical:
            info = subprocess.STARTUPINFO()
            info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            info.wShowWindow = subprocess.SW_HIDE
            process = start_native(context, command, cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, startupinfo=info)
            try:
                pending = context.runtime_dir / ("screenshot/space-inspector.png" if inspector else "screenshot/space-field.png")
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
                    time.sleep(.2)
                else:
                    raise TimeoutError(log)
            finally:
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=10)
            captures = collect_native_screenshots(context)
            assert captures
        else:
            run_native(context, command, cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=90)
    before, after = chunks(source), chunks(output)
    assert before[0] >= 404 and after[0] >= 405 and (before[1][b"ASTR"] == after[1][b"ASTR"] if before[0] >= 405 else orbital_bytes(before) == orbital_bytes(after)), "ordinary reload changed orbital state"
    assert identity(source) == scene["snapshot"]
    print(f"Space ordinary {scene['name']}/{configuration}: exact orbital resave passed", flush=True)


def fresh_world(context, configuration):
    """Exercise the real new-world hook, including Classic's empty-field policy."""
    for profile, expected in (("multilayer_industry", 10), ("classic", 0)):
        owner = context.for_feature("space", "new-" + profile.replace("_", "-"))
        owner.prepare_runtime()
        # Feature views deliberately share one process home. Replace these
        # isolated settings rather than create duplicate INI keys between runs.
        lines = owner.config_path.read_text(encoding="utf-8").splitlines()
        lines = [line for line in lines if line.split("=", 1)[0].strip() not in ("gameplay_profile", "starting_year")]
        owner.config_path.write_text("\n".join(lines) + f"\ngameplay_profile = {profile}\nstarting_year = 2000\n", encoding="utf-8")
        output = owner.saves / "new-world.sav"
        (owner.runtime_dir / "scripts/game_start.scr").write_text(
            f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        executable = console_executable(owner, ROOT / "build" / configuration / "openttd.exe")
        log = owner.logs / "new-world.log"
        with log.open("wb") as sink:
            run_native(owner, [str(executable), "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X",
                               "-I", "original_dos", "-c", str(owner.config_path), "-g", "-G", "12345"],
                       cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        version, data = chunks(output)
        assert version >= 405 and b"ASTR" in data
        # Riff size prefix is3 bytes for this bounded family. Fields are explicit big-endian bytes.
        header = struct.unpack(">HIIQQBIIiiii", data[b"ASTR"][3:54])
        assert header[6] == expected and header[7] == 0, (profile, header)
        assert header[0] == (1 if expected else 0)
        print(f"New {profile}: {expected} asteroids, no free probes", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("prepare", "verify", "capture", "inspect"))
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    context = current_run_context("space", args.mode)
    if args.mode == "prepare":
        prepare(context)
        return
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if args.mode == "verify":
        fresh_world(context, args.configuration)
    for scene in manifest["checkpoints"]:
        source = ROOT / scene["snapshot"]["path"]
        assert identity(source) == scene["snapshot"]
        owner = context.for_feature("space", args.mode + "-" + scene["name"])
        if args.mode in ("capture", "inspect"):
            ordinary(owner, scene, args.configuration, graphical=True, inspector=args.mode == "inspect")
        else:
            ordinary(owner, scene, args.configuration)
            native(context.for_feature("space", "resume-" + scene["name"]), args.configuration, "verify", source)


if __name__ == "__main__":
    main()
