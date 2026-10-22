"""Capture the selected original Ground native observer with explicit current inputs.

@file ground_transport_runtime_checks.py Original native writer and pixel/owner scope remain binding.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79FHB/79FHC/79FHCC/79FHD.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from ground_viewport_capture import run_capture


def capture(context, mode, configuration, application, source, world, source_view):
    """Delegate the original bounded native observer without historical cache or build discovery.

    @param context Fresh explicit current output/runtime invocation.
    @param mode This original producer's exact registered native capture mechanism.
    @param configuration Actual independently built current native configuration.
    @param application Explicit current viewport executable closed against its selected build.
    @param source Exact retained original review world, never an active continuation.
    @param world Independent original complete-raster comparison contract for this mode.
    @param source_view Explicit checked original Ground/M1 source/pose view.
    @return Actual native process, source/pixel/container provenance and unchanged-input facts.
    """
    if mode not in ('purchases', 'rail', 'road'):
        raise ValueError("this original Ground producer does not own the selected capture mode")
    return run_capture(context, mode, configuration, application, source, world, source_view,
                       Path(__file__).relative_to(ROOT).as_posix())


def main():
    """Dispatch only this original producer's current explicitly selected native review scope."""
    from ground_viewport_review import main as selected_main
    selected_main(default_mode=None, allowed_modes=('purchases', 'rail', 'road'))


if __name__ == "__main__":
    main()
