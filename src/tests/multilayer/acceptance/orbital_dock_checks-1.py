"""Verify delivered Dock construction and service in separate bounded native phases.

@file orbital_dock_checks-1.py Paid native421 deliveries, shared berths and interrupted service.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4C.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
OUTPUT = {"steel": "dock-steel-422.sav", "goods": "dock-ready-422.sav",
          "service": "dock-complete-422.sav", "work": "dock-complete-422.sav", "cancel": "dock-cancelled-422.sav"}


def native(context, configuration, action, source, corrupt=None, reject=False):
    """Drive actual paid native owners without fabricated packets, material receipts or hull positions.

    @param context Explicit isolated runtime and evidence owner.
    @param configuration Built game/test configuration.
    @param action Actual delivery, service, replay, cancellation or preview phase.
    @param source Genuine predecessor or identity-verified preceding output.
    @param corrupt Optional independently malformed same-file preview input.
    @param reject Require full native loader refusal before any observer can run.
    @return Native output checkpoint, or None after verified loader/preview refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_dock_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_DOCK_SOURCE=str(source), OTTD_ORBITAL_DOCK_ACTION=action)
    environment.pop("OTTD_ORBITAL_DOCK_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_ORBITAL_DOCK_CORRUPT"] = str(corrupt)
    log = context.logs / "native-orbital-dock.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-dock]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # A test/startup failure cannot stand in for an actual semantic native loader refusal.
        assert result.returncode != 0 and "Broken savegame" in evidence and "orbital" in evidence.lower() and "All tests passed" not in evidence, log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_DOCK native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    return context.saves / OUTPUT[action]


def main():
    """Run one focused phase; the service phase additionally proves exact replay and ordinary resave."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", required=True, choices=("steel", "goods", "service", "cancel"))
    parser.add_argument("--previous", type=Path, help="Exact preceding Dock phase report; required after steel")
    args = parser.parse_args()
    previous = None
    if args.phase == "steel":
        source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    else:
        assert args.previous is not None
        previous = args.previous.resolve()
        assert previous.is_relative_to(ROOT)
        preceding = json.loads(previous.read_text(encoding="utf-8"))
        assert preceding["configuration"] == args.configuration
        assert preceding["phase"] == {"goods": "steel", "service": "goods", "cancel": "service"}[args.phase]
        checkpoint = preceding["partial" if args.phase == "cancel" else "complete"]
        source = ROOT / checkpoint["path"]
        assert identity(source) == checkpoint
    original = identity(source)
    context = current_run_context("space", "dock-" + args.phase + "-verify")
    preparation = context.for_feature("space", "dock-" + args.phase + "-prepare")
    complete = native(preparation, args.configuration, args.phase, source)
    version, expected = chunks(complete)
    assert version == 422
    report = {"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group4C; actual paid Dock owners.",
              "configuration": args.configuration, "phase": args.phase, "save_version": version,
              "source": original, "complete": identity(complete), "previous": identity(previous) if previous else None}
    if args.phase == "service":
        partial = preparation.saves / "dock-work-422.sav"
        continued = native(context.for_feature("space", "dock-service-resume"), args.configuration, "work", partial)
        actual = chunks(continued)[1]
        # Interrupted work must preserve every actual packet, hull, berth, utility, cost and consumed-material owner.
        for family in EXACT:
            assert expected[family] == actual[family], family
        ordinary_context = context.for_feature("space", "dock-service-ordinary")
        ordinary(ordinary_context, {"name": "dock-complete", "snapshot": identity(complete)}, args.configuration)
        resaved = ordinary_context.saves / "ordinary-resave.sav"
        actual = chunks(resaved)[1]
        # An ordinary paused load cannot repeat activation, parts consumption, service allowance or source registration.
        for family in EXACT:
            assert expected[family] == actual[family], family
        report.update(partial=identity(partial), continued=identity(continued), ordinary_resave=identity(resaved),
                      exact_owner_families=len(EXACT), actual_distinct_berths=2, original_core_competition=True,
                      paused_job_preserves_other_work=True, zero_generation_preserves_parts=True)
    if args.phase == "cancel":
        ordinary_context = context.for_feature("space", "dock-cancel-ordinary")
        ordinary(ordinary_context, {"name": "dock-cancelled", "snapshot": identity(complete)}, args.configuration)
        resaved = ordinary_context.saves / "ordinary-resave.sav"
        actual = chunks(resaved)[1]
        # Returned unconsumed parts and retired settlement history must survive loading without reactivating the disconnected consumer.
        for family in EXACT:
            assert expected[family] == actual[family], family
        report.update(ordinary_resave=identity(resaved), exact_owner_families=len(EXACT), unconsumed_pairs_returned=2, settled_energy_refunded=False)
    assert identity(source) == original
    target = context.reports / ("orbital-dock-" + args.phase + ".json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Native Dock {args.phase} verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
