"""Observe orbital presentation in unchanged paid ordinary-game predecessors.

@file orbital_final_art_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L
Only existing saves, view/pause/screenshot controls and ordinary native ticks
are used. No inventory, hull, job, pose, date or work receipt is installed.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import time

from PIL import Image
from space_preview_checks import ROOT, identity
from runner import current_run_context, start_native, console_executable, collect_native_screenshots

COMMON = runpy.run_path(str(Path(__file__).with_name("space_final_art_checks-1.py")))
PRODUCERS = {
    "processor": ("logs/orbital-processor-verification.json", "45ddf32a5b13afccea72e1af6753cfe87cfcea5119d351ba063f9ee875406747"),
    "freight": ("logs/processor-freight-verification.json", "6b6479c4e5ee1c8ba31b3329253c67fc487bc56036171229e793efb95c5a9fc0"),
    "heavy": ("logs/heavy-network-verification.json", "e2587e78690a8e31643e17cdf92e4e6f38c53efff02c4e716dab4c17a1d15d03"),
    "dock": ("logs/heavy-dock-service-verification.json", "2b70758f523be0bbdbdb825f4b392ab22f3e9e07c894764968df213dc4a31d3a"),
    "construction": ("logs/orbital-art-paid-{configuration}-1.json", None),
    "manufacture": ("logs/orbital-art-manufacture-{configuration}-1.json", None),
}


def read(path):
    """Read only one explicitly selected evidence document.

    @param path Repository-owned current receipt or immutable producer index.
    @return Parsed original JSON without replaying a historical source graph.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, owner, checkpoint):
    """Resolve an actual paid checkpoint through its accepted frozen producer.

    @param configuration Matching native configuration of the original producer.
    @param owner Explicit accepted processor, paid-hauler, heavy, paid-Dock or current paid module/factory producer.
    @param checkpoint Original native/own checkpoint key, never a fabricated export.
    @return Dated producer identity and unchanged actual native save identity.
    """
    relative, sha = PRODUCERS[owner]
    if owner in ("construction", "manufacture"):
        # Each native configuration supplies its own fully accepted paid-world
        # membership. Its independent ordinary view need not await the other
        # build; the combined group remains open until both are bound/reviewed.
        path = ROOT / relative.format(configuration={"RelWithDebInfo": "release", "Debug": "debug"}[configuration])
        reference = read(path)["report"]
        assert identity(ROOT / reference["path"]) == reference
        row = read(ROOT / reference["path"])
        assert row["complete"] and row["configuration"] == configuration
        assert not any(row[field] for field in ("added_stock", "granted_work", "granted_pose"))
        source = row["checkpoints"][checkpoint]
        assert identity(ROOT / source["path"]) == source
        return identity(path), source
    path = ROOT / relative
    assert identity(path)["sha256"] == sha
    row, = [row for row in read(path)["configurations"] if row["configuration"] == configuration]
    if owner in ("processor", "freight"):
        source = row["checkpoints"][checkpoint]
    else:
        # The older heavy/Dock indexes publish direct scenario reports rather
        # than a flat checkpoint map. Select only the requested native original
        # report; never replay its historical executable/source graph.
        folder = "heavy-network-" + checkpoint + "-local" if owner == "heavy" else "heavy-dock-" + checkpoint
        filename = "heavy-network.json" if owner == "heavy" else "heavy-dock.json"
        reference, = [ref for ref in row["reports"] if ("/" + folder + "/") in ref["path"] and ref["path"].endswith("/reports/" + filename)]
        report_path = ROOT / reference["path"]
        assert identity(report_path) == reference
        report = read(report_path)
        assert report["configuration"] == configuration
        source = report["after"]
    assert identity(ROOT / source["path"]) == source
    return identity(path), source


def compare_ordinary_owners(source, after, configuration):
    """Compare actual owners while admitting only the explicit empty legacy metadata upgrade.

    @param source Exact retained paid native source identity.
    @param after This current ordinary process's unchanged paused own-resave.
    @param configuration Same accepted native profile/configuration for legacy metadata comparison.
    @return Exact comparison scope, including any disabled empty438 metadata migration.
    """
    version, before = COMMON["AUDIT"]["chunks"](ROOT / source["path"])
    current, decoded = COMMON["AUDIT"]["chunks"](ROOT / after["path"])
    assert current == 438
    if version == 438:
        COMMON["AUDIT"]["compare"](source, after)
        return dict(physical_owners=30, unchanged_metadata=["CDIM", "SPRC"], empty_processing_upgrade=False)
    assert version in (436, 437)
    # These accepted preprocessor worlds already use the unchanged436 physical
    # framing. They may gain only disabled empty processing metadata, never a
    # private work receipt, batch, output, power account or extra physical owner.
    for family in COMMON["AUDIT"]["PHYSICAL"]:
        assert family in before and before[family] == decoded[family], (source["path"], family)
    if b"CDIM" in before:
        assert before[b"CDIM"] == decoded[b"CDIM"]
    else:
        #436 predates configured-dimension metadata. Require the native
        # migration to publish the exact already-accepted438 profile dimensions;
        # a capture helper cannot supply a replacement cargo or mass table.
        assert version == 436
        _, canonical_source = predecessor(configuration, "processor", "native/ice-batch")
        canonical = COMMON["AUDIT"]["chunks"](ROOT / canonical_source["path"])[1]
        assert decoded[b"CDIM"] == canonical[b"CDIM"]
    assert b"SPRC" not in before
    parser = runpy.run_path(str(Path(__file__).with_name("orbital_processor_checks-3.py")))
    _, raw = parser["read_stream"](ROOT / after["path"])
    assert parser["records"](raw) == {}
    return dict(physical_owners=30, unchanged_metadata=["CDIM"] if b"CDIM" in before else [],
                canonical_cargo_dimension_upgrade=b"CDIM" not in before, empty_processing_upgrade=True)


def open_inspector(process, click=None):
    """Open the ordinary Space construction window through its existing F12 hotkey.

    @param process This invocation's exact native game child.
    @param click Optional native client coordinates selecting an existing visible site row.
    @return None; only actual ordinary screenshots establish the resulting view.
    """
    user = ctypes.WinDLL("user32", use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user.PostMessageW.restype = wintypes.BOOL
    windows = []

    @callback_type
    def select_window(window, unused):
        """Identify only the owned child's original game window.

        @param window Enumerated top-level handle. @param unused Required native callback data.
        @return Continue enumeration without changing another application.
        """
        pid = wintypes.DWORD()
        user.GetWindowThreadProcessId(window, ctypes.byref(pid))
        name = ctypes.create_unicode_buffer(256)
        user.GetClassNameW(window, name, len(name))
        # A helper/IME or another game cannot receive this invocation's input.
        if pid.value == process.pid and name.value == "OTTD":
            windows.append(window)
        return True

    user.EnumWindows(select_window, 0)
    window, = windows
    assert user.PostMessageW(window, 0x100, 0x7B, 1)
    assert user.PostMessageW(window, 0x101, 0x7B, 1 << 31 | 1)
    time.sleep(.5)
    if click is not None:
        x, y = click
        assert 0 <= x < 65536 and 0 <= y < 65536
        assert user.PostMessageW(window, 0x200, 0, x | y << 16)
        assert user.PostMessageW(window, 0x201, 1, x | y << 16)
        assert user.PostMessageW(window, 0x202, 0, x | y << 16)
        time.sleep(.5)


def capture(configuration, owner, checkpoint, live=False, view="field", click=None, warmup=0):
    """Capture actual ordinary field pixels and independently compare original owners.

    @param configuration Current accepted game binary, without a rebuild.
    @param owner Exact dated paid predecessor family.
    @param checkpoint Original native/own checkpoint in that family.
    @param live Let only the normal native loop earn progress for several captures.
    @param view Ordinary whole field or existing construction/status inspector.
    @param click Optional original client coordinates selecting a visible inspector row.
    @param warmup Real unpaused seconds before live screenshots, allowing original travel to reach work.
    @return New report identity; personal visual qualification remains separate.
    """
    producer, source = predecessor(configuration, owner, checkpoint)
    assert not (live and view == "inspector")
    assert click is None or view == "inspector"
    assert 0 <= warmup <= 45 and (live or warmup == 0)
    accepted_path = ROOT / "logs/orbital-art-final-verification.json"
    accepted = read(accepted_path)
    binary_path = ROOT / "logs/orbital-art-ordinary-binary-verification.json"
    # A same-group presentation correction may append a focused binary receipt.
    # Keep the original packed/native acceptance immutable and use that later
    # receipt only for the changed actual application, never for another GRF.
    if not binary_path.is_file():
        binary_path = accepted_path
    binary_acceptance = read(binary_path)
    game = ROOT / "build" / configuration / "openttd.exe"
    assert identity(game) == binary_acceptance["builds"][configuration]["application"]
    assert identity(ROOT / accepted["grf"]["path"])["sha256"] == accepted["grf"]["sha256"]
    phase = owner + "-" + checkpoint.replace("/", "-")
    context = current_run_context("orbital-art", "ordinary-" + phase)
    context.prepare_runtime()
    if view == "inspector":
        # Automatic GUI scaling grows with resolution and can hide the existing
        # lower actions. A fixed client-only scale keeps the complete ordinary
        # window visible without changing world geometry or saved gameplay.
        config = context.config_path.read_text(encoding="utf-8")
        assert "[misc]" not in config
        context.config_path.write_text(config + "\n; @see r2mlvs11-orbital-stations-and-manufacturing.md13L; view only.\n[misc]\ngui_scale = 125\n", encoding="utf-8")
    output = context.saves / "ordinary-orbital-view-438.sav"
    earned = context.saves / "ordinary-orbital-earned-438.sav"
    inspected = context.saves / "ordinary-orbital-inspected-438.sav"
    startup = context.runtime_dir / "scripts/game_start.scr"
    count = 5 if live else 1
    names = ["orbital-final-" + str(i) for i in range(count)]
    # The first ordinary paused resave proves that loading/viewing preserved
    # original owners. Live work, when requested, starts only after that resave.
    startup.write_text(
        "# @see r2mlvs11-orbital-stations-and-manufacturing.md13L; ordinary view only.\n"
        f'pause\nset savegame_format lzma\nspace_view\nzoomto 1\nsave "{output.with_suffix("").as_posix()}"\n'
        + ('unpause\n' if live else (f'screenshot giant "{names[0]}"\n' if view == "field" else '')), encoding="utf-8")
    executable = console_executable(context, game)
    arguments = [str(executable), "-v", "win32", "-b", "8bpp-simple", "-s", "null", "-m", "null",
                 "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(ROOT / source["path"])]
    # The ordinary construction window must fit its existing action rows in
    # the captured client area; resolution is a view option, never gameplay state.
    if view == "inspector":
        arguments[1:1] = ["-r", "1440x900"]
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = subprocess.SW_HIDE
    log = context.logs / "ordinary-orbital-art.log"
    commands = []
    with log.open("wb") as sink:
        process = start_native(context, arguments, cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, startupinfo=info)
        try:
            deadline = time.monotonic() + 90
            if view == "inspector":
                # Loading/resaving must finish before native GUI input. Opening
                # the existing inspector and selecting a row cannot earn work.
                while not output.is_file():
                    assert process.poll() is None
                    assert time.monotonic() < deadline
                    time.sleep(.2)
                open_inspector(process, click)
                commands = [f'save "{inspected.with_suffix("").as_posix()}"', f'screenshot viewport no_con "{names[0]}"']
                COMMON["console_commands"](process, commands)
            if live:
                # Screenshots run while the normal game is unpaused so activity
                # glass reflects real current work. Pause only after the final
                # capture request; no work parity or elapsed time is assigned.
                while not output.is_file():
                    assert process.poll() is None
                    assert time.monotonic() < deadline
                    time.sleep(.2)
                # Some accepted paid sources begin genuine pursuit rather than
                # extraction. Only ordinary unpaused wall time may reach their
                # real work site; no native date, position or counter is assigned.
                time.sleep(.5 + warmup)
                commands = [f'screenshot giant "{name}"' for name in names]
                commands += ["pause", f'save "{earned.with_suffix("").as_posix()}"']
                COMMON["console_commands"](process, commands)
            pending = [context.runtime_dir / "screenshot" / (name + ".png") for name in names]
            # Native screenshot/save writes are asynchronous. Require every
            # complete requested image and its actual resave from this own PID.
            while time.monotonic() < deadline:
                assert process.poll() is None, log
                # Field activity needs its earned resave; an inspector instead
                # needs a second paused resave proving the actual GUI was pure.
                if output.is_file() and all(path.is_file() for path in pending) and (not live or earned.is_file()) and (view != "inspector" or inspected.is_file()):
                    try:
                        for path in pending:
                            with Image.open(path) as raster:
                                raster.verify()
                        break
                    except OSError:
                        pass
                time.sleep(.2)
            else:
                raise TimeoutError(log)
        finally:
            # Stop only this owned hidden capture process after the bounded
            # observation; never address another game or any user application.
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
    captures = collect_native_screenshots(context)
    assert len(captures) == count
    comparison = compare_ordinary_owners(source, identity(output), configuration)
    inspected_comparison = compare_ordinary_owners(identity(output), identity(inspected), configuration) if view == "inspector" else None
    assert (producer, source) == predecessor(configuration, owner, checkpoint)
    screenshots = []
    # Retain exact original indexed pixels for later whole-module, source-part
    # and foreground review; RGB fingerprints are a separate visual identity.
    for path in captures:
        with Image.open(path) as raster:
            assert raster.mode == "P"
            screenshots.append(dict(**identity(path), dimensions=list(raster.size),
                                    indexed_sha256=hashlib.sha256(raster.tobytes()).hexdigest(),
                                    rgb_sha256=hashlib.sha256(raster.convert("RGB").tobytes()).hexdigest()))
    artifacts = [output, *captures, *([earned] if live else []), *([inspected] if view == "inspector" else [])]
    native = context.write_report("ordinary-orbital-process.json", Path(__file__).relative_to(ROOT).as_posix(),
                                  artifacts, [ROOT / source["path"]], configuration,
                                  native_writer="ordinary openttd.exe native screenshot command")
    record = read(native)
    record.update(argv=arguments, pid=process.pid, exit_status=process.returncode,
                  termination="owned capture process stopped after complete native writes",
                  executable=identity(executable), original_application=identity(game), startup=identity(startup),
                  runtime=context.runtime_dir.relative_to(ROOT).as_posix(), terminal=identity(log))
    native.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    report = context.reports / "ordinary-orbital-art.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                     owner=owner, checkpoint=checkpoint, dated_producer=producer, source=source,
                                     after=identity(output), packed_acceptance=identity(accepted_path),
                                     application_acceptance=identity(binary_path),
                                     native_process=identity(native), captures=screenshots, captured_live=live,
                                     view=view, inspector_click=click,
                                     warmup_seconds=warmup,
                                     owner_comparison=comparison,
                                     inspected_after=identity(inspected) if view == "inspector" else None,
                                     inspected_owner_comparison=inspected_comparison,
                                     earned_after=identity(earned) if live else None, console_commands=commands,
                                     physical_owner_families=30, added_stock=False, granted_work=False,
                                     granted_pose=False, personal_review_complete=False), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Capture one bounded ordinary paid predecessor; require an explicit membership output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--owner", choices=PRODUCERS, required=True)
    parser.add_argument("--checkpoint", required=True)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--view", choices=("field", "inspector"), default="field")
    parser.add_argument("--click", nargs=2, type=int)
    parser.add_argument("--warmup", type=float, default=0)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs")
    assert not output.exists()
    report = capture(args.configuration, args.owner, args.checkpoint, args.live, args.view, args.click, args.warmup)
    output.write_text(json.dumps(dict(_documentation=__doc__, report=report), indent=2) + "\n", encoding="utf-8")
    print(report, flush=True)


if __name__ == "__main__":
    main()
