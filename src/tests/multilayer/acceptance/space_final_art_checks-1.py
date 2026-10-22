"""Capture selected retained earned worlds through the ordinary paused application.

@file space_final_art_checks-1.py Explicit current inputs; no progress or publication.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19C.
Historical family acceptance stays historical; this wrapper owns current paused fields.
"""
import ctypes
from ctypes import wintypes
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
from ordinary_art_capture import capture as capture_native

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


def capture(context, configuration, application, source, contract):
    """Observe an existing paused world using the original native screenshot mechanism.

    @param context Fresh explicit configuration-specific output/runtime owner.
    @param configuration Actual independently built current native configuration.
    @param application Explicit current ordinary executable qualified by the coordinator.
    @param source Exact retained already-earned historical fixture save.
    @param contract Independent original physical/metadata owner requirements.
    @return Actual native process, screenshot, resave and owner/clock comparison.
    """
    console = console_commands
    return capture_native(context, configuration, application, source,
        Path(__file__).relative_to(ROOT).as_posix(), "space", console, contract)


def main():
    """Dispatch only the explicit current bounded paused-field recipe for this family."""
    from ordinary_art_review import main as selected_main
    selected_main("space")


if __name__ == "__main__":
    main()
