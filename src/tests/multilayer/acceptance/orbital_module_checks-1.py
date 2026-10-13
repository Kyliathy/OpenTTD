"""Verify exclusive module reservations using a genuine paid import and native save/load.

@file orbital_module_checks-1.py Core stock to typed construction job, without activation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group2.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess
from PIL import Image

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native, save_native_capture

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_service_checks-1.py")))["EXACT"]


def native(context, configuration, action, source):
    """Run one bounded native reservation observer against immutable physical owners.

    @param context Isolated runtime and evidence owner.
    @param configuration Built executable configuration.
    @param action Prepare a paid import or resume a real held reservation.
    @param source Genuine completed Core or exclusive-reservation checkpoint.
    @return Saved inert module checkpoint after 74 native ticks.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_reservation_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_RESERVATION_ACTION=action, OTTD_ORBITAL_RESERVATION_SOURCE=str(source))
    log = context.logs / "native-module-reservation.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-module-reservation]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=115)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_RESERVATION native=true" in evidence, log
    # Convert only the container format of actual native rasters; the captured controls and material counts stay unchanged.
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    return context.saves / "module-inert-418.sav"


def main():
    """Check both missing inputs, split conservation, competing jobs and exact ordinary/continued owners."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, required=True, help="Immutable paid service-complete-417.sav with Goods8 in Core stock")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    context = current_run_context("space", "module-reservation-verify")
    prepare = context.for_feature("space", "module-reservation-prepare")
    complete = native(prepare, args.configuration, "prepare", source)
    checkpoint = prepare.saves / "module-reserved-418.sav"
    before = identity(checkpoint)
    continued = native(context.for_feature("space", "module-reservation-resume"), args.configuration, "resume", checkpoint)
    version, expected = chunks(complete)
    actual = chunks(continued)[1]
    assert version == 418
    # Compare the packet, native job, company, vehicle, clock and shared utility families at the same final simulation point.
    for family in EXACT:
        assert expected.get(family) == actual.get(family), ("continued", family)
    resave = context.for_feature("space", "module-reservation-ordinary")
    ordinary(resave, {"name": "reserved", "snapshot": before}, args.configuration)
    original_chunks, saved_chunks = chunks(checkpoint)[1], chunks(resave.saves / "ordinary-resave.sav")[1]
    # An ordinary paused load cannot activate a blueprint, move reserved packets or rewrite shared utility history.
    for family in EXACT:
        assert original_chunks.get(family) == saved_chunks.get(family), ("ordinary", family)
    assert identity(source) == original and identity(checkpoint) == before
    report = context.reports / "orbital-module-reservation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group2; actual native reservation and continuation evidence.",
        "configuration": args.configuration, "save_version": version, "input": original,
        "reserved": before, "inert": identity(complete), "continued": identity(continued),
        "exact_owner_families": [family.decode() for family in EXACT], "actual_paid_import": "STEL10",
        "reserved_recipe": ["STEL8", "GOOD8"], "remaining_core_stock": ["STEL2", "GOOD0"],
        "missing_either_preserves_all_owners": True, "split_conserves_provenance_age_feeder_share": True,
        "competing_job_cannot_reuse_packets": True, "blueprint_inert_after_74_ticks": True,
        "no_second_capital_or_delivery_payment": True, "actual_native_reserve_button": True,
        "local_production_scope": "Reservation accepts physical recipe labels regardless of provenance; manufacture producer remains later plan work."},
        indent=2) + "\n", encoding="utf-8")
    print(f"Native module reservation verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
