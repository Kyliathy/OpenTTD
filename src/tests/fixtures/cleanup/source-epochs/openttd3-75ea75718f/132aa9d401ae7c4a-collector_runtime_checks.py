"""Run bounded native collector rendering or ordinary-game checkpoint captures.

@file collector_runtime_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Inputs use fixture-supplied floor/materials. No transported bootstrap is claimed.
"""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import sys
import time

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native, save_native_capture, report_provenance, console_executable, start_native, collect_native_screenshots
from artifact_paths import resolve_input_path
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from savegame_plane_digest import read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS


def collector_owners(path):
    """Read exact native construction, stock, source and shared-utility bytes.

    @param path Actual current checkpoint or its ordinary-game resave.
    @return Authoritative native chunk bytes; camera and pause policy are client/control state.
    """
    _version, stream = read_stream(path)
    names = {b"GCBH", b"GCBD", b"GCSH", b"GCSD", b"GVNT", b"UPLH", b"UPLD", b"INDY"}
    reader, result = Reader(stream), {}
    # Traverse existing native chunk framing without interpreting quantities.
    # Exact original bytes identify ownership; unrelated chunks are only skipped.
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
            # Native table rows have gamma lengths and a zero terminator. Skip
            # descriptors and records together so their whole encoding is checked.
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise ValueError("unsupported native collector chunk transport")
        if name in names:
            assert name not in result, "duplicate native collector owner"
            result[name] = stream[start:reader.pos]
    assert len(result) == len(names), "collector checkpoint is missing a required native owner"
    return result


def fingerprint(path):
    """Identify a complete immutable input or actual native output.

    @param path Existing file with its producer-selected repository path.
    @return Repository-relative path and exact byte SHA256.
    """
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def match_native_hardware(native_path, ordinary_path):
    """Locate exact native machinery pixels in the ordinary game's full-layer raster.

    @param native_path This state's actual640x360 native viewport capture at zoom2.
    @param ordinary_path Actual ordinary-game screenshot of the same native checkpoint.
    @return Exact source-frame/patch identity and its unique ordinary-image position.
    """
    native_image = Image.open(native_path).convert("RGB")
    ordinary_image = Image.open(ordinary_path).convert("RGB")
    assert native_image.size == (640, 360)
    # The central24x24 installed drum/pipe window avoids animated cabinet glass.
    # Match every rendered RGB byte; varied terrain alone cannot satisfy this
    # proof that the ordinary executable drew the actual selected machinery.
    box = (306, 154, 330, 178)
    patch = native_image.crop(box)
    wanted, data = patch.tobytes(), ordinary_image.tobytes()
    stride, row, start, matches = ordinary_image.width * 3, patch.width * 3, 0, []
    # Search exact first-row candidates, then compare every remaining row.
    # The source window is a pixel observation, never a recreated model or art edit.
    while True:
        hit = data.find(wanted[:row], start)
        if hit < 0:
            break
        if hit % 3 == 0 and all(data[hit + y * stride:hit + y * stride + row] == wanted[y * row:(y + 1) * row] for y in range(patch.height)):
            matches.append([(hit % stride) // 3, hit // stride])
        start = hit + 1
    assert len(matches) == 1, "ordinary game lacks one exact native collector machinery window"
    return {"native_frame": fingerprint(native_path), "native_box": list(box), "ordinary_origin": matches[0],
            "rgb_sha256": hashlib.sha256(wanted).hexdigest(), "pixels_exact": True}


def native(context, configuration):
    """Capture actual stages through the native observer and retain source identities.

    @param context Explicit geothermal/collector-art invocation and runtime home.
    @param configuration Native executable configuration built for this group.
    @return None; write the exact native raster/checkpoint evidence on success.
    """
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    original = startup.read_bytes() if startup.exists() else None
    source = resolve_input_path(ROOT / "src/tests/fixtures/geothermal/fixture389-collector-funded-read.sav")
    identity = fingerprint(source)
    log = context.logs / f"native-{configuration}.log"
    try:
        startup.write_text("# @see r2mlvs14-remaining-assets-and-runtime-integration.md79C disclosed native art observer.\ncollector_art\n", encoding="utf-8")
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.collector-art-runtime]"],
                       cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        assert "All tests passed" in evidence
        assert "COLLECTOR_ART_RUNTIME frames=14 construction_stages=4 completed=true current_heat=true render_owners_unchanged=true" in evidence
        location = re.search(r"COLLECTOR_ART_LOCATION origin=(\d+) centre=(\d+) layer=(\d+)", evidence)
        assert location is not None and int(location[3]) == 3
        frames = []
        # Only this observer's fourteen actual viewport rasters are converted.
        # Palette-resolved pixels, dimensions and renderer provenance stay exact.
        names = [f"construction-{stage}" for stage in range(4)] + ["ready", "working", "transparent"]
        for name in names:
            for zoom in (1, 2):
                ppm = context.images / f"collector-{name}-zoom-{zoom}.ppm"
                raster = Image.open(ppm)
                assert raster.size == (640, 360)
                assert len(set(raster.get_flattened_data())) > 8
                target = ppm.with_suffix(".png")
                save_native_capture(context, raster, ppm, target)
                assert Image.open(target).tobytes() == raster.tobytes()
                frames.append(fingerprint(target))
                ppm.unlink()  # Disposable generated container from this exact repository invocation.
        report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md79C; native collector observer",
                  "source": identity["path"], "source_sha256": identity["sha256"], "log": log.relative_to(ROOT).as_posix(),
                  "test_executable": fingerprint(ROOT / "build" / configuration / "openttd_test.exe"), "frames": frames,
                  "saves": [fingerprint(context.saves / f"collector-{name}.sav") for name in names],
                  "fixture_floor": True, "fixture_material_delivery": True, "transported_bootstrap": False,
                  "native_origin": int(location[1]), "native_centre": int(location[2]), "native_layer": int(location[3]),
                  "simulation_owners_unchanged_during_render": True, "user_review": "pending"}
        report_provenance(context, report, __file__, configuration)
        target = context.reports / f"native-{configuration}.json"
        target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Native collector: fourteen real frames and seven current saves; report={target.relative_to(ROOT)}", flush=True)
    finally:
        if original is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(original)
        assert fingerprint(source) == identity, "immutable native source changed"


def normal(context, configuration, source, centre):
    """Load one current checkpoint with the ordinary game and capture its real viewport.

    @param context Explicit bounded invocation; native checkpoint and normal capture may share its run ID.
    @param configuration Actual ordinary-game build to exercise.
    @param source Exact existing native checkpoint; never discover a latest save.
    @param centre Actual Deep world tile reported by the native observer for this source.
    @return None; retain screenshot and normal resave with actual executable/input identities.
    """
    context.prepare_runtime()
    source = resolve_input_path(source)
    identity = fingerprint(source)
    startup = context.runtime_dir / "scripts/game_start.scr"
    original = startup.read_bytes() if startup.exists() else None
    output = context.saves / f"{source.stem}-ordinary-{configuration}.sav"
    log = context.logs / f"{source.stem}-ordinary-{configuration}.log"
    screenshot = f"{source.stem}-ordinary-{configuration}"
    process = None
    try:
        # Layer selection is client state and reload starts on Surface. An
        # explicit native world tile selects Deep through ordinary scrollto;
        # pause freezes the accepted scene for the real screenshot renderer.
        startup.write_text("# @see r2mlvs14-remaining-assets-and-runtime-integration.md79C ordinary-game art proof.\n"
                           f"pause\nzoomto 1\nscrollto instant {centre}\nset savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\n'
                           f'screenshot giant "{screenshot}"\n', encoding="utf-8")
        executable = console_executable(context, ROOT / "build" / configuration / "openttd.exe")
        startup_info = subprocess.STARTUPINFO()
        startup_info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup_info.wShowWindow = subprocess.SW_HIDE
        with log.open("wb") as sink:
            process = start_native(context, [str(executable), "-v", "win32", "-b", "8bpp-simple", "-s", "null", "-m", "null",
                                    "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)],
                           cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, startupinfo=startup_info,
                           env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
            pending = context.runtime_dir / "screenshot" / f"{screenshot}.png"
            deadline = time.monotonic() + 90
            # The real graphical backend drains the screenshot queue; null video
            # forces a null blitter and cannot produce ordinary rendered evidence.
            # Wait only for this owned process's complete PNG and native resave.
            while time.monotonic() < deadline:
                assert process.poll() is None, "ordinary game exited before capture"
                if pending.is_file() and output.is_file():
                    try:
                        with Image.open(pending) as capture:
                            capture.verify()
                        break
                    except OSError:
                        pass  # The owned renderer may still be finishing PNG compression.
                time.sleep(0.2)
            else:
                raise TimeoutError("ordinary collector screenshot did not complete within90 seconds")
            process.terminate()  # Only this paused generated review process; its inputs and supplied user saves are untouched.
            process.wait(timeout=10)
        collect_native_screenshots(context)
        target = context.images / f"{screenshot}.png"
        assert target.is_file(), "ordinary game did not render the queued screenshot"
        image = Image.open(target)
        assert image.width > 0 and image.height > 0
        assert len(set(image.convert("RGB").get_flattened_data())) > 8
        assert output.is_file(), "ordinary game did not resave the loaded native checkpoint"
        assert collector_owners(source) == collector_owners(output), "ordinary-game capture changed actual collector owners"
        hardware = match_native_hardware(context.images / f"{source.stem}-zoom-2.png", target)
        report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md79C; ordinary game, disclosed fixture inputs",
                  "source": identity["path"], "source_sha256": identity["sha256"], "resave": fingerprint(output),
                  "log": log.relative_to(ROOT).as_posix(), "frames": [fingerprint(target)],
                  "game_executable": fingerprint(ROOT / "build" / configuration / "openttd.exe"),
                  "fixture_floor": True, "fixture_material_delivery": True, "transported_bootstrap": False,
                  "native_owner_chunks_exact": True, "user_review": "pending"}
        report["native_centre"] = centre
        report["capture"] = {"command": "screenshot giant", "layer": 3, "size": list(image.size)}
        report["native_hardware"] = hardware
        report["shutdown"] = "owned_paused_process_terminated_after_complete_capture"
        context.write_report(f"{screenshot}-producer.json", Path(__file__).relative_to(ROOT).as_posix(), [target, output], [source], configuration)
        report_provenance(context, report, __file__, configuration)
        report_path = context.reports / f"{source.stem}-ordinary-{configuration}.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Ordinary game collector: screenshot and current resave; report={report_path.relative_to(ROOT)}", flush=True)
    finally:
        # An observer failure must also close its own game before restoring the
        # isolated startup hook; no other game/process is selected by name.
        if process is not None and process.poll() is None:
            process.terminate()
            process.wait(timeout=10)
        if original is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(original)
        assert fingerprint(source) == identity, "native checkpoint changed during ordinary loading"


def main():
    """Dispatch one bounded native or ordinary-game case, keeping each process below two minutes.

    @return None; failed build/loading/render assertions propagate to the caller.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("mode", choices=("native", "normal"))
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--centre", type=int)
    args = parser.parse_args()
    context = current_run_context("geothermal", "collector-art")
    if args.mode == "native":
        native(context, args.configuration)
    else:
        if args.source is None:
            parser.error("normal requires an explicit current native --source")
        if args.centre is None:
            parser.error("normal requires the observer's explicit Deep --centre tile")
        normal(context, args.configuration, args.source, args.centre)


if __name__ == "__main__":
    main()
