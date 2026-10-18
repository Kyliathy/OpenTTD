"""Capture final Space art from retained earned states in the ordinary game.

@file space_final_art_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BH
No vehicle, pose, work, survey receipt or cargo is granted. Native screenshots
and ordinary resaves retain their actual executable, input and writer identities.
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

AUDIT = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-3.py")))
PRODUCERS = {
    "inspection": ("logs/asteroid-inspection-verification.json", "dd738d03ac7c633c3749fa6307b25ec3e06ac6dcb19a428f7eaf10e20f7c2294"),
    "attachment": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af"),
    "class": ("logs/probe-class-verification.json", "058483cef0fa7fcaa6e51186ea1a3c8f70ced6854a5aef419345af88a94b960e"),
}
CASES = {
    "selection": ("inspection", "native/unknown/after"),
    "scan": ("inspection", "native/scan/after"),
    "known": ("inspection", "native/known/after"),
    "construction": ("inspection", "native/construction/after"),
    "mining": ("inspection", "native/mining/after"),
    "pod": ("inspection", "native/pending/after"),
    "attachment": ("attachment", "native/stopped"),
    "nickel": ("class", "0/mine/loaded"),
    "ice": ("class", "1/mine/loaded"),
    "rich": ("class", "2/mine/loaded"),
}


def read(path):
    """Read one explicitly selected repository evidence document.

    @param path Exact retained JSON path.
    @return Original parsed document; no historical executable discovery.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, phase):
    """Resolve the original earned saved state through its accepted producer.

    @param configuration Matching accepted source configuration.
    @param phase Explicit required gameplay/presentation state.
    @return Direct producer identity and immutable source-save identity.
    """
    owner, checkpoint = CASES[phase]
    relative, sha = PRODUCERS[owner]
    path = ROOT / relative
    assert identity(path)["sha256"] == sha, path
    selected, = [row for row in read(path)["configurations"] if row["configuration"] == configuration]
    source = selected["checkpoints"][checkpoint]
    assert identity(ROOT / source["path"]) == source
    return identity(path), source


def console_commands(process, commands):
    """Send ordinary console input only to this invocation's native game window.

    @param process Exact still-owned ordinary game subprocess.
    @param commands View/pause/save/screenshot commands, with no gameplay grants.
    @return None; native command execution and resulting saves/pixels qualify success.
    """
    user = ctypes.WinDLL("user32", use_last_error=True)
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user.PostMessageW.restype = wintypes.BOOL
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    windows = []

    @callback_type
    def own_window(window, unused):
        """Find only the toplevel window belonging to the exact native child.

        @param window Windows-enumerated top-level handle.
        @param unused Required EnumWindows callback parameter.
        @return Continue enumeration without changing any other application.
        """
        pid = wintypes.DWORD()
        user.GetWindowThreadProcessId(window, ctypes.byref(pid))
        name = ctypes.create_unicode_buffer(256)
        user.GetClassNameW(window, name, len(name))
        # IME/helper windows may share this process. Only its registered OTTD main window owns native game console input.
        if pid.value == process.pid and name.value == "OTTD":
            windows.append(window)
        return True

    user.EnumWindows(own_window, 0)
    assert len(windows) == 1, windows
    window, = windows
    assert user.PostMessageW(window, 0x100, 0xC0, 41 << 16 | 1)
    time.sleep(.3)
    # Ordinary console input pauses earned work and saves its real resulting world before requesting native pixels. No resource, pose, clock or job counter is assigned.
    for command in commands:
        for character in command:
            assert user.PostMessageW(window, 0x102, ord(character), 1)
        assert user.PostMessageW(window, 0x100, 13, 1)
        assert user.PostMessageW(window, 0x102, 13, 1)
        assert user.PostMessageW(window, 0x101, 13, 1 << 31 | 1)
        time.sleep(.3)


def capture(configuration, phase, view, packed_path, live=False):
    """Observe one ordinary load boundary and optionally genuine earned construction.

    @param configuration Current already-built Debug or RelWithDebInfo game.
    @param phase Required original paid/scan/construction/descent checkpoint.
    @param view Whole Space field or ordinary selection/status inspector.
    @param packed_path Explicit accepted packing index for these exact application bytes.
    @param live Let the ordinary native loop earn construction work after its unchanged load/resave; never grant a work step or pose.
    @return Current report path binding actual native pixels and owner comparison.
    """
    producer, source = predecessor(configuration, phase)
    packed_path = (ROOT / packed_path).resolve()
    packed = read(packed_path)
    game = ROOT / "build" / configuration / "openttd.exe"
    accepted, = [row for row in packed["configurations"] if row["configuration"] == configuration]
    assert packed["complete"] and identity(game) == accepted["application"]
    assert identity(ROOT / packed["payload"]["path"]) == packed["payload"]
    context = current_run_context("space", "final-art-" + phase + "-" + view)
    context.prepare_runtime()
    output = context.saves / "ordinary-final-art-438.sav"
    startup = context.runtime_dir / "scripts/game_start.scr"
    screenshot = "giant" if view == "field" else "viewport no_con"
    # Paused reloads cannot authorize construction work. A separately disclosed live capture uses the real ordinary loop, while its startup resave still proves the load itself preserved all physical owners.
    if live:
        assert phase == "construction" and view == "field"
    startup.write_text(
        "# @see r2mlvs10-asteroids-and-mining-probes.md14BH; view-only native capture.\n"
        f'pause\nset savegame_format lzma\nspace_view\nzoomto 1\nsave "{output.with_suffix("").as_posix()}"\n'
        + ('unpause\n' if live else f'screenshot {screenshot} "space-final-{phase}"\n'), encoding="utf-8")
    executable = console_executable(context, game)
    arguments = [str(executable), "-v", "win32", "-b", "8bpp-simple", "-s", "null", "-m", "null",
                 "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(ROOT / source["path"])]
    log = context.logs / "ordinary-final-art.log"
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = subprocess.SW_HIDE
    with log.open("wb") as sink:
        process = start_native(context, arguments, cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, startupinfo=info)
        try:
            pending = context.runtime_dir / "screenshot" / ("space-final-" + phase + ".png")
            deadline = time.monotonic() + 90
            live_after = context.saves / "ordinary-earned-construction-438.sav"
            if live:
                # Wait for the real ordinary load/resave, then let its normal loop earn work. Pausing and screenshotting use native console input after that progress, rather than an immediate startup screenshot before the first tick.
                while not output.is_file():
                    assert process.poll() is None
                    assert time.monotonic() < deadline
                    time.sleep(.2)
                time.sleep(1.25)
                commands = ["pause", f'save "{live_after.with_suffix("").as_posix()}"', f'screenshot giant "space-final-{phase}"']
                console_commands(process, commands)
            # Native screenshot writing is asynchronous. Accept only a complete image and its actual ordinary resave, while the exact owned child remains alive.
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
            # This existing ordinary-game screenshot route has no scripted delayed exit. Stop only its owned paused process after capture, retaining its actual exit status explicitly.
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
    captures = collect_native_screenshots(context)
    assert len(captures) == 1
    after = identity(output)
    AUDIT["compare"](source, after)
    assert identity(ROOT / source["path"]) == source
    earned = identity(live_after) if live else None
    screenshot_path, = captures
    with Image.open(screenshot_path) as raster:
        dimensions = list(raster.size)
        pixels = hashlib.sha256(raster.convert("RGB").tobytes()).hexdigest()
    native = context.write_report("ordinary-final-art-process.json", Path(__file__).relative_to(ROOT).as_posix(),
                                  [output, screenshot_path, *([live_after] if live else [])], [ROOT / source["path"]], configuration,
                                  native_writer="ordinary openttd.exe native screenshot command")
    process_record = read(native)
    process_record.update(argv=arguments, pid=process.pid, exit_status=process.returncode,
                          termination="owned paused screenshot process terminated after complete capture",
                          executable=identity(executable), original_application=identity(game), startup=identity(startup),
                          runtime=context.runtime_dir.relative_to(ROOT).as_posix(), terminal=identity(log))
    native.write_text(json.dumps(process_record, indent=2) + "\n", encoding="utf-8")
    report = context.reports / "space-final-art.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase, view=view,
                                     dated_producer=producer, source=source, after=after, packed_acceptance=identity(packed_path),
                                     native_process=identity(native), capture=identity(screenshot_path), dimensions=dimensions,
                                     rgb_pixel_sha256=pixels, physical_owner_families=30, metadata_families=2,
                                     captured_live=live, resave_precedes_live_progress=live,
                                     earned_after=earned, live_console_commands=commands if live else [],
                                     added_stock=False, granted_work=False, granted_pose=False,
                                     ordinary_game_capture=True, personal_review_complete=False), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Capture exactly one bounded current-group ordinary-game state per invocation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=CASES, required=True)
    parser.add_argument("--view", choices=("field", "inspector"), default="field")
    parser.add_argument("--packed", type=Path, default=Path("logs/space-art-packed-verification.json"))
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    report = capture(args.configuration, args.phase, args.view, args.packed, args.live)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, report=identity(report)), indent=2) + "\n", encoding="utf-8")
    print(identity(report), flush=True)


if __name__ == "__main__":
    main()
