"""Verify genuine service/assembly evacuation and exact ordinary native resaves.

@file orbital_evacuation_checks-1.py Actual422 packet/job ownership without synthetic material or hull movement.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group5B.
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

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def native(context, configuration, action, source):
    """Evacuate only actual paid packet/job owners through ordinary native commands.

    @param context Explicit isolated runtime/evidence owner.
    @param configuration Built game/test configuration.
    @param action Held Dock service or genuine partial assembly cancellation.
    @param source Immutable identity-verified paid predecessor.
    @return Native evacuated checkpoint; no manufactured input or private pool deletion is used.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_evacuation_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_EVACUATION_SOURCE=str(source), OTTD_ORBITAL_EVACUATION_ACTION=action)
    log = context.logs / "native-orbital-evacuation.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-evacuation]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_EVACUATION native=true" in evidence, log
    # Preserve actual native process/container provenance when converting only the capture format for review.
    for capture in context.images.glob("*.ppm"):
        with Image.open(capture) as raster:
            save_native_capture(context, raster, capture, capture.with_suffix(".png"))
    return context.saves / "module-evacuated-422.sav"


def service_source(predecessor, checkpoint="partial"):
    """Resolve one explicit held-service checkpoint from its own exact report/proof identities.

    @param predecessor Exact service report or frozen421 Dock proof.
    @param checkpoint Exact declared partial or complete checkpoint role; no arbitrary saved path is accepted.
    @return Immutable genuine paid active-service save; no latest-run discovery is allowed.
    """
    previous = json.loads(predecessor.read_text(encoding="utf-8"))
    if "reports" in previous:
        selected = []
        # Select the declared service role only within this explicitly supplied hash-bound proof.
        for name in previous["reports"]:
            path = ROOT / name
            assert identity(path)["sha256"] == previous["sha256"][name]
            report = json.loads(path.read_text(encoding="utf-8"))
            if report.get("phase") == "service":
                selected.append(report)
        assert len(selected) == 1
        previous = selected[0]
    assert previous["phase"] == "service"
    assert checkpoint in ("partial", "complete")
    source = ROOT / previous[checkpoint]["path"]
    assert identity(source) == previous[checkpoint]
    return source


def main():
    """Verify one bounded cancellation/transfer phase without changing its paid input or any resaved owner."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", required=True, choices=("service", "construction"))
    parser.add_argument("--dock", type=Path, help="Exact native service report or frozen421 proof; required for service")
    args = parser.parse_args()
    predecessor = None
    if args.phase == "service":
        assert args.dock is not None
        predecessor = args.dock.resolve()
        assert predecessor.is_relative_to(ROOT)
        source = service_source(predecessor)
    else:
        source = ROOT / "src/tests/fixtures/demos/space/space-module-power-418.sav"
    original = identity(source)
    context = current_run_context("space", "module-evacuation-" + args.phase + "-verify")
    preparation = context.for_feature("space", "module-evacuation-" + args.phase + "-prepare")
    complete = native(preparation, args.configuration, args.phase, source)
    version, expected = chunks(complete)
    assert version == 422
    ordinary_context = context.for_feature("space", "module-evacuation-" + args.phase + "-ordinary")
    ordinary(ordinary_context, {"name": "module-evacuated", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    actual = chunks(resaved)[1]
    # Loading must preserve cancelled job receipts, moved provenance, unchanged hull custody, utility settlement and every real packet owner.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "orbital-module-evacuation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group5B; actual paid packet/job evacuation.",
        "configuration": args.configuration, "phase": args.phase, "save_version": version,
        "predecessor": identity(predecessor) if predecessor else None, "source": original,
        "complete": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT),
        "quantity_and_provenance_conserved": True, "spent_energy_refunded": False, "manifest_allowance_renewed": False,
        "hull_positions_changed": False, "occupied_completed_module_still_protected": args.phase == "service"}, indent=2) + "\n", encoding="utf-8")
    print(f"Native module evacuation verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
