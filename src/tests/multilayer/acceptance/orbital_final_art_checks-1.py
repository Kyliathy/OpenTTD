"""Capture selected retained earned worlds through the ordinary paused application.

@file orbital_final_art_checks-1.py Explicit current inputs; no progress or publication.
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

def capture(context, configuration, application, source, contract):
    """Observe an existing paused world using the original native screenshot mechanism.

    @param context Fresh explicit configuration-specific output/runtime owner.
    @param configuration Actual independently built current native configuration.
    @param application Explicit current ordinary executable qualified by the coordinator.
    @param source Exact retained already-earned historical fixture save.
    @param contract Independent original physical/metadata owner requirements.
    @return Actual native process, screenshot, resave and owner/clock comparison.
    """
    import runpy
    transport = runpy.run_path(str(Path(__file__).with_name("space_final_art_checks-1.py")))
    console = transport["console_commands"]
    return capture_native(context, configuration, application, source,
        Path(__file__).relative_to(ROOT).as_posix(), "orbital", console, contract)


def main():
    """Dispatch only the explicit current bounded paused-field recipe for this family."""
    from ordinary_art_review import main as selected_main
    selected_main("orbital")


if __name__ == "__main__":
    main()
