"""Prepare or continue one actual paid builder/rocket berth-contention burst.

@file orbital_freight_checks-4.py Shared original priority, actual departure and whole module delivery.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11C.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_freight_checks-1.py")))["EXACT"]


def native(context, configuration, action, source):
    """Run one bounded failure observer over a declared genuine paid predecessor.

    @param context Isolated runtime/log/artifact owner.
    @param configuration Built executable configuration.
    @param action Actual contention preparation/continuation or one saved target-loss phase.
    @param source Exact native snapshot; synthetic boundary quantities are never imported here.
    @return Actual423 emitted native phase or completed recovery save.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("module_failure_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_MODULE_FAILURE_SOURCE=str(source), OTTD_MODULE_FAILURE_ACTION=action)
    log = context.logs / "native-module-failure.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-module-failure]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and "MODULE_FAILURE native=true" in evidence, log
    name = {"pad-prepare": "module-flight-pad-blocked-423.sav", "contention-prepare": "module-flight-contention-423.sav", "contention-continue": "module-flight-contention-complete-423.sav",
            "lost-continue": "module-flight-lost-complete-423.sav"}.get(action, "module-flight-lost-423.sav")
    result = context.saves / name
    assert chunks(result)[0] == 423
    return result


def main():
    """Consume an explicit prior report, exercising actual native contention without another launch fee or hull."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", required=True, choices=("prepare", "continue"))
    parser.add_argument("--predecessor", type=Path, required=True, help="Exact positive preparation report or this observer's contention preparation report")
    args = parser.parse_args()
    predecessor = args.predecessor.resolve()
    assert predecessor.is_relative_to(ROOT)
    previous = json.loads(predecessor.read_text(encoding="utf-8"))
    assert previous["configuration"] == args.configuration
    checkpoint = previous["preburn"] if args.phase == "prepare" else previous["checkpoint"]
    source = ROOT / checkpoint["path"]
    assert identity(source) == checkpoint
    context = current_run_context("space", "module-contention-" + args.phase + "-verify")
    complete = native(context.for_feature("space", "module-contention-" + args.phase), args.configuration, "contention-" + args.phase, source)
    paused = context.for_feature("space", "module-contention-ordinary-" + args.phase)
    ordinary(paused, {"name": "module-contention", "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Paused loading preserves either the real queued rocket/occupied berth or its completed physical unload and recovery receipts.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == checkpoint
    report = context.reports / "orbital-module-contention.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11C; actual paid builder/rocket contention and native departure.",
        "configuration": args.configuration, "phase": args.phase, "predecessor": identity(predecessor), "source": checkpoint,
        "checkpoint": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT)}, indent=2) + "\n", encoding="utf-8")
    print(f"Module contention/{args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
