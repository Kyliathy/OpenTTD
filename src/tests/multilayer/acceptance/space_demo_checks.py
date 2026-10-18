"""Prepare and exercise player demos through the ordinary game application.

Retained producer saves supply their declared starting stock. No fixture callback,
cash, cargo, vehicle, work or date is installed by this runner.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import time

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, console_executable, run_native, start_native, collect_native_screenshots

DEST = ROOT / "src/tests/fixtures/demos/space"
MANIFEST = DEST / "current-checkpoints.json"
GUIDE = "docs/zz-reach2/upgrades/2026-10/d2sa-space-adventures.md"
OWNERS = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF",
          b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND",
          b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"TUPH", b"TUPD", b"TUVH", b"TUVD",
          b"ENGN", b"EIDS", b"ERNW", b"GRPS", b"CDIM", b"SPRC", b"SSOR")
# name, accepted producer, exact checkpoint (or report scenario), first experiment
CASES = (
    ("space-factory", "orbital-art-manufacture-release-1", "manufacture-partial", "Paid construction probe assembly at 74/148 ticks; finish the same native hull"),
    ("space-heavy", "heavy-network-verification", "heavy-network-mining-local", "Paid heavy mining robot pursuing finite ore; watch its larger hold fill"),
    ("space-hauler", "processor-freight-verification", "native/travelling", "Paid orbital hauler travelling with 2 actual Steel units; follow its transfer"),
    ("space-steel", "orbital-processor-verification", "native/steel-partial", "Steelworks partway through a powered batch from actual mined Nickel Ore"),
    ("space-goods", "orbital-processor-verification", "native/goods-partial", "Goodsworks consuming transported Steel and Rare Earth in a paid batch"),
    ("space-refinery", "orbital-processor-verification", "native/refinery-partial", "Refinery partway through Nickel Ore conversion to Refined Metal"),
    ("space-ice", "orbital-processor-verification", "native/ice-partial", "Refinery using the alternative Water Ice recipe; inspect real input/output history"),
    ("space-power", "orbital-art-paid-release-2", "power-partial", "Paid Power module construction at 222/444 ticks; complete then activate"),
    ("space-fuel-store", "orbital-art-paid-release-2", "fuel-frame", "Finished Fuel store frame with final art; activation remains unsupported"),
    ("space-heavy-service", "heavy-dock-service-verification", "heavy-dock-heavy-partial", "Paid heavy service in a two-berth Dock; pause, resume or interrupt its power"),
    ("space-missions", "probe-mission-verification", "probe-mission-local-visit-mine-partial", "Real Visit then Mine queue; inspect saved typed actions and survey knowledge"),
    ("space-products", "processor-returns-verification", "native/released", "Orbital Steel and Goods descending to Earth; inspect Surface receipt and collection links"),
    ("space-steel-outage", "orbital-processor-verification", "native/steel-outage", "Steelworks retains paid partial work while its panel is disabled; restore power"),
)


def read(path):
    """Read a directly selected receipt without replaying its historical graph."""
    return json.loads(path.read_text(encoding="utf-8"))


def checked(ref):
    """Resolve one unchanged repository-owned input and verify its original hash."""
    path = ROOT / ref["path"]
    assert identity(path) == {k: ref[k] for k in ("path", "sha256")}, path
    return path


def predecessor(producer, checkpoint):
    """Select an explicit accepted Release checkpoint, preserving its provenance."""
    index = ROOT / "logs" / (producer + ".json")
    data = read(index)
    references = [identity(index)]
    if "report" in data:
        references.append(data["report"])
        row = read(checked(data["report"]))
    else:
        row = next(r for r in data["configurations"] if r["configuration"] == "RelWithDebInfo")
    if "checkpoints" in row:
        source = row["checkpoints"][checkpoint]
    else:
        ref, = [r for r in row["reports"] if "/" + checkpoint + "/" in r["path"]]
        references.append(ref)
        source = read(checked(ref))["after"]
    checked(source)
    return source, references


def ordinary(context, executable, source, name, advance=False):
    """Load, pause and resave, or earn 3,000 ordinary simulation ticks.

    Each child has a bounded lifetime and an isolated repository data home.
    The native null driver saves on exit without depending on calendar speed.
    """
    output = context.saves / (name + ".sav")
    assert not output.exists(), output
    save = f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n'
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text(
        "pause\nspace_view\nset savegame_format lzma\n" + ("unpause\n" if advance else save),
        encoding="utf-8")
    log = context.logs / (name + ".log")
    original_config = context.config_path.read_bytes()
    autosave = context.runtime_dir / "save/autosave/exit.sav"
    if advance:
        assert b"[gui]" not in original_config
        context.config_path.write_bytes(original_config + b"\n[gui]\nautosave_on_exit = true\n")
        autosave.unlink(missing_ok=True)
    try:
        with log.open("wb") as sink:
            result = run_native(context, [executable, "-v", "null:ticks=3000" if advance else "null:ticks=2",
                                "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-d", "misc=3,sl=3",
                                "-c", str(context.config_path), "-g", str(source)],
                                cwd=context.runtime_dir, stdout=sink, stderr=subprocess.STDOUT,
                                timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=ARGS.configuration))
        if advance:
            assert autosave.is_file(), log
            output.write_bytes(autosave.read_bytes())
            context.write_report(name + "-exit-save.json", Path(__file__).relative_to(ROOT).as_posix(),
                                 [output], [source], ARGS.configuration,
                                 native_writer="VideoDriver_Null::MainLoop -> DoExitSave (3000 normal GameLoop calls)")
    finally:
        context.config_path.write_bytes(original_config)
    assert result.returncode == 0 and output.is_file(), log
    assert not re.search(r"(?:save failed|load failed|error loading|assertion failed)", log.read_text(errors="replace"), re.I), log
    return output


def prepare(context, executable):
    """Create fresh paused, current-format snapshots; never replace a retained save."""
    assert not MANIFEST.exists(), "current demo manifest already exists"
    rows = []
    for number, (name, producer, key, description) in enumerate(CASES, 31):
        source, references = predecessor(producer, key)
        original = checked(source)
        loaded = ordinary(context, executable, original, name + "-prepare")
        version, _ = chunks(loaded)
        target = DEST / f"{name}-{version}.sav"
        assert not target.exists(), target
        target.write_bytes(loaded.read_bytes())
        assert identity(original) == source
        rows.append(dict(menu_number=number, name=name, description=description,
                         snapshot=identity(target), source=source, producers=references,
                         save_version=version, preparation=identity(loaded)))
        print(f"Prepared {number}: {name} ({version})", flush=True)
    MANIFEST.write_text(json.dumps(dict(date="2026-10-18", guide=GUIDE,
        prepared_conditions="Inherited Surface starting stock and terrain remain prepared. Native paid construction, manufacture, mining, freight, processing and return are retained. This is not an unassisted factory-to-orbit bootstrap.",
        checkpoints=rows), indent=2) + "\n", encoding="utf-8")
    return rows


def verify(context, executable):
    """Check launcher inputs, ordinary load/resave stability and live continuation."""
    launcher = (ROOT / "play.ps1").read_text(encoding="utf-8-sig")
    entries = re.findall(r"@\{ Name = '([^']+)'; SaveName = '[^']+';\s*Snapshot = '([^']+)'", launcher)
    assert len(entries) >= 41, f"missing featured demos: {len(entries)}"
    results = []
    current = {row["name"]: row for row in read(MANIFEST)["checkpoints"]}
    for name, relative in entries:
        if ARGS.names and name not in ARGS.names:
            continue
        source = ROOT / relative.replace("\\", "/")
        original = identity(source)
        if name in current:
            assert original == current[name]["snapshot"]
        first = ordinary(context, executable, source, name + "-loaded")
        second = ordinary(context, executable, first, name + "-reloaded")
        version, before = chunks(first)
        next_version, after = chunks(second)
        # DATE/VIEW and session-only native settings are outside physical Space
        # custody. Compare every present actual Space owner, including packets.
        owners = OWNERS
        matched = [key.decode() for key in owners if key in before]
        assert version == next_version and all(before[key.encode()] == after[key.encode()] for key in matched), name
        row = dict(name=name, source=original, loaded=identity(first), reloaded=identity(second),
                   save_version=version, unchanged_owners=matched)
        if ARGS.advance and name in current and name != "space-fuel-store":
            earned = ordinary(context, executable, second, name + "-advanced", advance=True)
            _, live = chunks(earned)
            row["advanced"] = identity(earned)
            row["advance_writer"] = identity(context.reports / (name + "-advanced-exit-save.json"))
            row["changed_owners"] = [key.decode() for key in owners if before.get(key) != live.get(key)]
            assert b"ASTR" in live and live[b"ASTR"] != after[b"ASTR"], "ordinary ticks did not advance"
            expected = {"space-factory": "SPST", "space-heavy": "VEHS", "space-hauler": "VEHS",
                        "space-steel": "SPST", "space-goods": "SPST", "space-refinery": "SPST",
                        "space-ice": "SPST", "space-power": "SPST", "space-heavy-service": "SPST",
                        "space-missions": "VEHS", "space-products": "SPDJ"}.get(name)
            assert expected is None or expected in row["changed_owners"], f"{name}: expected {expected} to advance"
        assert identity(source) == original, "starting save changed"
        results.append(row)
        print(f"Verified {name}: v{version}, {len(matched)} stable owners" +
              (", advanced " + ",".join(row["changed_owners"]) if "advanced" in row else ""), flush=True)
    assert results
    return results


def capture(context, executable):
    """Inspect actual current GUI pixels using only existing native view controls."""
    controls = runpy.run_path(str(Path(__file__).with_name("orbital_final_art_checks-1.py")))
    commands = controls["COMMON"]["console_commands"]
    rows = []
    invocation = context
    for row in read(MANIFEST)["checkpoints"]:
        name = row["name"]
        if name not in (ARGS.names or ["space-factory", "space-steel", "space-heavy-service"]):
            continue
        context = invocation.for_process(name)
        context.prepare_runtime()
        executable = str(console_executable(context, ROOT / "build" / ARGS.configuration / "openttd.exe"))
        config = context.config_path.read_text(encoding="utf-8")
        context.config_path.write_text(config + "\n[misc]\ngui_scale = 125\n", encoding="utf-8")
        source = checked(row["snapshot"])
        saved = context.saves / (name + "-gui.sav")
        startup = context.runtime_dir / "scripts/game_start.scr"
        startup.write_text(f'pause\nspace_view\nsave "{saved.with_suffix("").as_posix()}"\n', encoding="utf-8")
        info = subprocess.STARTUPINFO()
        info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        info.wShowWindow = subprocess.SW_HIDE
        log = context.logs / (name + "-gui.log")
        with log.open("wb") as sink:
            process = start_native(context, [executable, "-v", "win32", "-r", "1440x900", "-b", "8bpp-simple",
                                   "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                                   "-c", str(context.config_path), "-g", str(source)],
                                   cwd=context.runtime_dir, stdout=sink, stderr=subprocess.STDOUT, startupinfo=info)
            try:
                deadline = time.monotonic() + 90
                while not saved.exists():
                    assert process.poll() is None and time.monotonic() < deadline, log
                    time.sleep(.2)
                click = {"space-factory": (600, 350), "space-steel": (600, 365)}.get(name)
                controls["open_inspector"](process, click)
                commands(process, [f'screenshot viewport no_con "{name}"'])
                picture = context.runtime_dir / "screenshot" / (name + ".png")
                while not picture.exists():
                    assert process.poll() is None and time.monotonic() < deadline, log
                    time.sleep(.2)
                # Complete PNG decoding also waits for the writer to finish.
                from PIL import Image
                with Image.open(picture) as raster:
                    raster.load()
            finally:
                # End only this owned paused screenshot process, after its
                # native save and complete PNG. Functional load checks exit cleanly.
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=10)
        pictures = collect_native_screenshots(context)
        assert len(pictures) == 1
        rows.append(dict(name=name, source=identity(source), application=identity(ROOT / "build" / ARGS.configuration / "openttd.exe"),
                         screenshot=identity(pictures[0]), exit_status=process.returncode,
                         termination="owned paused GUI process terminated after complete capture"))
        print(f"Captured {name}: {pictures[0].relative_to(ROOT)}", flush=True)
    assert rows
    return rows


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "verify", "capture"))
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--names", nargs="*")
    parser.add_argument("--advance", action="store_true")
    ARGS = parser.parse_args()
    context = current_run_context("space-demo", ARGS.action + "-" + ARGS.configuration.lower())
    context.prepare_runtime()
    game = ROOT / "build" / ARGS.configuration / "openttd.exe"
    executable = str(console_executable(context, game))
    rows = {"prepare": prepare, "verify": verify, "capture": capture}[ARGS.action](context, executable)
    report = dict(action=ARGS.action, configuration=ARGS.configuration, application=identity(game),
                  manifest=identity(MANIFEST), checkpoints=rows, complete=True,
                  native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))])
    target = ROOT / "logs" / f"space-demo-{ARGS.action}-{ARGS.configuration}.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT), flush=True)
