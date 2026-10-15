"""Verify real company lifecycle and safe Core removal in bounded sequential phases.

@file orbital_lifecycle_checks-1.py Native422 takeover/reuse and dependency-local safe removal.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group5C.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

shared = runpy.run_path(str(Path(__file__).with_name("orbital_evacuation_checks-1.py")))
EXACT, service_source = shared["EXACT"], shared["service_source"]
OUTPUT = {"dependencies": "module-dependencies-422.sav", "ownership": "module-lifecycle-unowned-422.sav",
          "dock-remove": "module-dock-removed-422.sav"}


def native(context, configuration, action, source):
    """Run only actual paid owner commands; no replacement Core receipt, hull pose or physical material is fabricated.

    @param context Explicit isolated runtime/evidence owner.
    @param configuration Built game/test configuration.
    @param action Current native company, completed-Dock or Core dependency-removal phase.
    @param source Immutable identity-verified genuine predecessor/checkpoint.
    @return This phase's actual native output checkpoint.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_lifecycle_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_LIFECYCLE_SOURCE=str(source), OTTD_ORBITAL_LIFECYCLE_ACTION=action)
    log = context.logs / "native-orbital-lifecycle.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-module-lifecycle]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_LIFECYCLE native=true" in evidence, log
    return context.saves / OUTPUT[action]


def main():
    """Verify one bounded phase with explicit predecessor identity; final owner states also undergo exact ordinary resave."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", required=True, choices=tuple(OUTPUT))
    parser.add_argument("--dock", type=Path, help="Exact service report or frozen421 Dock proof")
    args = parser.parse_args()
    assert args.dock is not None
    predecessor = args.dock.resolve()
    assert predecessor.is_relative_to(ROOT)
    source = service_source(predecessor, "partial" if args.phase == "ownership" else "complete")
    original = identity(source)
    context = current_run_context("space", "module-lifecycle-" + args.phase + "-verify")
    complete = native(context.for_feature("space", "module-lifecycle-" + args.phase + "-prepare"), args.configuration, args.phase, source)
    version, expected = chunks(complete)
    assert version == 422
    report = {"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group5C; genuine paid lifecycle owners.",
              "configuration": args.configuration, "phase": args.phase, "save_version": version,
              "predecessor": identity(predecessor), "source": original, "complete": identity(complete)}
    if args.phase in ("ownership", "dependencies", "dock-remove"):
        ordinary_context = context.for_feature("space", "module-lifecycle-" + args.phase + "-ordinary")
        ordinary(ordinary_context, {"name": "module-lifecycle", "snapshot": identity(complete)}, args.configuration)
        resaved = ordinary_context.saves / "ordinary-resave.sav"
        actual = chunks(resaved)[1]
        # Company loss and actual Core removal must preserve every retired source, spent receipt, held job and surviving native identity on ordinary reload.
        for family in EXACT:
            assert expected[family] == actual[family], family
        report.update(ordinary_resave=identity(resaved), exact_owner_families=len(EXACT))
    assert identity(source) == original
    target = context.reports / "orbital-module-lifecycle.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Native module lifecycle verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
