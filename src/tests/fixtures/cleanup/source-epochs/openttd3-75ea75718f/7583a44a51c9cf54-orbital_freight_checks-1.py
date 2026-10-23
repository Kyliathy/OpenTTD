"""Prepare one direct module flight from the immutable genuine paid418 world.

@file orbital_freight_checks-1.py Actual workshop launch and preburn/ascent/delivery checkpoints.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess
from PIL import Image

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, run_native, save_native_capture

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def native(context, configuration, action, source, corrupt=None, reject=False):
    """Run a bounded native module-flight observer against one explicit predecessor.

    @param context Exact runtime, log and artifact owner.
    @param configuration Built executable configuration.
    @param action Prepare the paid departure or continue its actual saved phase.
    @param source Immutable original world or declared native checkpoint.
    @param corrupt Optional malformed snapshot for independent same-file preview.
    @param reject Require a full native loader refusal rather than observer completion.
    @return Delivery or completed423 snapshot; None for an explicitly verified refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("module_flight_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_MODULE_FLIGHT_SOURCE=str(source), OTTD_MODULE_FLIGHT_ACTION=action)
    environment.pop("OTTD_MODULE_FLIGHT_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_MODULE_FLIGHT_CORRUPT"] = str(corrupt)
    log = context.logs / "native-module-flight.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-module-flight]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # A framed flight-family refusal must come from the actual decoder; startup/fixture failures cannot count as save acceptance.
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence, log
        assert any(marker in evidence for marker in ("Space transition", "module flight", "Space freight", "Space approach", "Space order", "Space production", "Space fleet")), log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "MODULE_FLIGHT native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    return context.saves / ("module-flight-delivery-423.sav" if action == "prepare" else "module-flight-complete-423.sav")


def main():
    """Emit exact paid departure identities and native workshop captures in one focused burst."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    original = identity(source)
    assert original["sha256"] == "0f10c5e085846c1ee2fca8c3d8945296e3b5366243ddaadff6d09e9250e00dca"
    context = current_run_context("space", "module-flight-prepare-verify")
    preparation = context.for_feature("space", "module-flight-prepare")
    delivery = native(preparation, args.configuration, "prepare", source)
    assert chunks(delivery)[0] == 423
    captures = []
    # Publish only real native renders with inherited provenance; format conversion grants no replacement artwork.
    for name in ("module-workshop-unselected", "module-workshop-ready"):
        ppm = preparation.images / (name + ".ppm")
        with Image.open(ppm) as raster:
            save_native_capture(preparation, raster, ppm, ppm.with_suffix(".png"))
        captures.append(identity(ppm.with_suffix(".png")))
    assert identity(source) == original
    report = context.reports / "orbital-module-flight-prepare.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11; paid direct departure, not final flight acceptance.",
        "configuration": args.configuration, "save_version": 423, "source": original, "delivery": identity(delivery),
        "preburn": identity(preparation.saves / "module-flight-preburn-423.sav"),
        "ascent": identity(preparation.saves / "module-flight-ascent-423.sav"), "captures": captures,
        "actual_workshop_launch": True, "one_departure_fee": True, "actual_space_entry": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared module flight: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
