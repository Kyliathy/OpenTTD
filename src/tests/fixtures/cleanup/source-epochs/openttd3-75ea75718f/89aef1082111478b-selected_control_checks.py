"""Run isolated native widget-policy and original selected-control presentation boundaries.

@file selected_control_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BD.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
Prepared mechanical unit inputs remain separate from the retained paid-world ordinary renderer proof.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native

CASES = ("[selected-control-icons-selection]", "[selected-control-icons-window]", "[m1-control-selection]", "[geothermal-window-policy]", "[transport-tube-controls-construction]")
SOURCES = ("src/multilayer_sprite.h", "src/multilayer_sprite.cpp", "src/underground_gui.cpp", "src/underground_gui.h", "src/transport_tube_gui.cpp", "src/geothermal_gui.cpp",
           "src/widgets/underground_widget.h", "src/widget.cpp", "src/tests/selected_control_icons.cpp", "src/tests/transport_tube_fixture.h",
           "src/tests/multilayer_control_selection.cpp", "src/tests/geothermal_window_sites.cpp", "src/tests/transport_tube_controls.cpp", "src/tests/multilayer/selected_control_checks.py")


def focused(configuration):
    """Verify current explicit icon/widget status and native command-policy boundaries in separate processes.

    @param configuration Current compiled Debug or optimized native test target.
    @return None; any refused profile/date/player/pressed-state or owner-purity mismatch prevents publication.
    """
    context = current_run_context("graphics", "selected-control-icons"); proofs = []
    for index, case in enumerate(CASES):  # Each actual window/profile/native-map owner starts independently, preserving the project's isolated-tag failure policy.
        _, proof = native(context, configuration, case, f"controls-{index}"); proofs.append({"case": case, **proof})
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BD; actual native control selection/widget boundaries,not a paid-world pixel claim",
              "configuration": configuration, "native": proofs, "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in SOURCES], "remaining": "84BD.2 actual current paid-world source/widget pixels and four-target acceptance,then84B.2/84C/84D."}
    path = context.output("reports", f"selected-control-icons-{configuration}.json"); path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def main():
    """Select one bounded current native control-icon burst.

    @return None; every exact current isolated native tag must pass.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args(); focused(args.config)


if __name__ == "__main__":
    main()
