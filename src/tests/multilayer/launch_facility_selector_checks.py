"""Qualify native facility/Surface component selectors without claiming full rendered-game acceptance.

@file launch_facility_selector_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB.
The real native art application executes facility plus preceding rocket cases.
Each process has an isolated repository invocation and60second timeout. Full
occupied raster review and ordinary paid-game evidence remain separate gates.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import time
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import cargo_rocket_art as art
from support.artifact_paths import create_run_context
import launch_rocket_selector_checks as rocket

OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-facility-selectors-v1"
SOURCES = ("src/launch_facility_sprite.h", "src/launch_facility_sprite.cpp", "src/station_cmd.cpp",
           "src/vehicle.cpp", "src/window.cpp", "src/tests/launch_facility_sprite.cpp",
           "src/tests/multilayer/launch_facility_selector_checks.py")


def source_cue_scope():
    """Require native row-major cells and workshop-only status pixels in the actual qualified atlas.

    @return Complete source row count and unchanged original three-pixel cue coverage.
    @throws ValueError On a source cell/order mismatch or a cue outside its own native workshop.
    """
    handoff = art.read_json(rocket.HANDOFF)
    accepted = handoff["sources"]["facility.launch-center"]
    report = art.read_json(ROOT / accepted["report"]["path"])
    mapping_ref = report["state_mapping"]
    if art.source.identity(ROOT / mapping_ref["path"]) != mapping_ref:
        raise ValueError("qualified facility native state/zone source mapping changed")
    mapping = art.read_json(ROOT / mapping_ref["path"])
    rows = report["sprites"]
    lookup = {(row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]): row for row in rows}
    changed = {}
    with Image.open(ROOT / accepted["atlas"]["path"]) as image:
        # Every logical source alias keeps its physical row-major cell. The
        # three original glazing pixels must lie in that view's native workshop,
        # so selecting a local cue cannot accidentally recolor another zone.
        for row in rows:
            view = mapping["native_views"][row["direction"]]
            x, y = row["world_cell"]
            if row["tile_variant"] != x + y * view["footprint"][0]:
                raise ValueError("facility source cell disagrees with native row-major layout")
            ready = lookup[("ready", 0, row["direction"], row["tile_variant"], row["part"])]
            bx, by, width, height = row["box"]
            rx, ry, rw, rh = ready["box"]
            actual = image.crop((bx, by, bx + width, by + height)).tobytes()
            original = image.crop((rx, ry, rx + rw, ry + rh)).tobytes()
            count = sum(a != b for a, b in zip(actual, original, strict=True))
            if row["state"] in ("working", "blocked"):
                key = (row["state"], row["direction"], row["frame"])
                changed[key] = changed.get(key, 0) + count
                sx, sy, sw, sh = view["zones"]["workshop"]["rect"]
                if count and not (sx <= x < sx + sw and sy <= y < sy + sh):
                    raise ValueError("facility cue pixels leave their actual native workshop")
            elif count:
                raise ValueError("static facility source alias changed original hardware")
    # All declared aliases and all state/view/frame cue groups remain present;
    # partial coverage cannot qualify the native local-workshop selector.
    if len(rows) != 1536 or len(changed) != 32 or any(count != 3 for count in changed.values()):
        raise ValueError("facility original status-pixel coverage changed")
    return dict(logical_source_rows=1536, cue_state_view_frames=32, original_glazing_pixels_per_cue=3)


def boundary():
    """Qualify the original source boundary with an explicit narrow station-render adaptation.

    @return Frozen source policy and current actual component sources.
    @throws ValueError On unrelated native-owner, source, payload or historical-receipt drift.
    """
    policy, protected = rocket.protected_boundary(native_station_draw=True)
    protected["facility_sources"] = [art.source.identity(ROOT / name) for name in SOURCES]
    protected["facility_source_cue_scope"] = source_cue_scope()
    return policy, protected


def run(configuration, current_renderer=False):
    """Execute current facility and rocket component cases in one actual native process.

    @param configuration Debug or RelWithDebInfo from the existing separate native art target.
    @param current_renderer Preserve initial component receipts while checking the integrated contact/renderer change.
    @return Scoped component receipt, explicitly leaving full raster/game acceptance open.
    @throws ValueError On failed/overdue native tests, missing phase parity or changed inputs.
    """
    policy, before = boundary()
    application = ROOT / "build" / configuration / "openttd_space_art_test.exe"
    executable = art.source.identity(application)
    context = create_run_context("multilayer-art", "launch-facility-selectors", run_id="facility-selectors-" + str(time.time_ns()),
                                 root=ROOT, profile="launch-fleet-art", process=configuration.lower())
    label = "launch-facility-current-selectors" if current_renderer else "launch-facility-selectors"
    terminal = ROOT / ("logs/" + label + "-" + configuration + ".log")
    arguments = [str(application), "[launch-facility-art],[launch-rocket-art]", "--reporter", "console", "--durations", "yes"]
    started = time.monotonic()
    with terminal.open("w", encoding="utf-8") as output:
        result = subprocess.run(arguments, cwd=ROOT / "build", env=context.environment(), stdout=output,
                                stderr=subprocess.STDOUT, timeout=60, check=False, creationflags=subprocess.CREATE_NO_WINDOW)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions in 9 test cases\)", text)
    # Both the five new facility cases and four preceding rocket cases must
    # run; a zero exit with a partial filter is not component qualification.
    if result.returncode != 0 or passed is None:
        raise ValueError("focused facility selectors failed; see " + str(terminal.relative_to(ROOT)))
    comparisons = rocket.verify_observations(text, policy)
    refusals = rocket.verify_refusals(text, policy)
    # Current native queries may dirty derived view rectangles but must retain
    # original simulation/source owners and executable/data bytes throughout.
    if boundary()[1] != before or art.source.identity(application) != executable:
        raise ValueError("facility selector inputs changed during native observation")
    proof = dict(_documentation=__doc__, complete=True, component_qualification_only=True,
                 configuration=configuration, application=executable, terminal=art.source.identity(terminal),
                 arguments=arguments[1:], cases=9, assertions=int(passed.group(1).replace(",", "")),
                 rocket_source_policy_comparisons=comparisons, detached_refusals=refusals,
                 elapsed_seconds=round(time.monotonic() - started, 3), boundary=before,
                 paid_world_loaded=False, simulation_ticks=0, full_renderer_reviewed=False,
                 ordinary_game_reviewed=False, public_runtime_activated=False)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    proof_name = "current-components" if current_renderer else "components"
    (OUTPUT / (proof_name + "-" + configuration + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(configuration + ": " + str(proof["assertions"]) + " assertions /9cases; " + str(comparisons) + " original-source comparisons", flush=True)
    return proof


def main():
    """Qualify one explicit configuration, retaining every other accepted proof.

    @return Zero only after actual native component observation and protected-boundary checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--current-renderer", action="store_true")
    args = parser.parse_args()
    run(args.configuration, args.current_renderer)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
