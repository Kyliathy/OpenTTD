"""Verify one actual preburn fuel/cancel or two-paid-rocket pad command-order phase.

@file launch_failure_checks-1.py Exact genuine module-flight predecessors and ordinary owner resaves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group15.
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


def main():
    """Consume only the explicitly named paid preburn/pad report; no new hull, copied private pad or source lookup is permitted."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", choices=("fuel-cancel", "pad-order"), required=True)
    parser.add_argument("--predecessor", type=Path, required=True)
    args = parser.parse_args()
    predecessor = args.predecessor.resolve()
    proof = json.loads(predecessor.read_text(encoding="utf-8"))
    assert proof["configuration"] == args.configuration
    source_identity = proof["preburn"]
    source = ROOT / source_identity["path"]
    original = identity(source)
    assert original == source_identity and chunks(source)[0] == 423
    context = current_run_context("space", "launch-failure-verify-" + args.phase)
    native = context.for_feature("space", "launch-failure-native-" + args.phase)
    native.prepare_runtime()
    (native.runtime_dir / "scripts/game_start.scr").write_text("launch_phase_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_LAUNCH_PHASE_SOURCE=str(source), OTTD_LAUNCH_PHASE_ACTION=args.phase)
    log = native.logs / "native-launch-failure.log"
    with log.open("wb") as sink:
        result = run_native(native, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.launch-phase-failure]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"LAUNCH_FAILURE native=true phase={args.phase}" in evidence, log
    complete = native.saves / f"launch-failure-{args.phase}-423.sav"
    assert chunks(complete)[0] == 423
    paused = context.for_feature("space", "launch-failure-ordinary-" + args.phase)
    ordinary(paused, {"name": "launch-failure-" + args.phase, "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Original paid hulls, pad order, cargo/fuel owners and once-only fee/cancel receipts survive ordinary paused loading exactly.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "launch-failure.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group15; actual paid fuel/cancel or two-rocket pad ordering, no synthetic hulls or stock.",
        "configuration": args.configuration, "phase": args.phase, "predecessor": identity(predecessor), "source": original,
        "complete": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT),
        "fuel_preserved_on_preburn_cancel": True, "one_fee_per_accepted_mission": True, "ordered_competing_commands": args.phase == "pad-order"}, indent=2) + "\n", encoding="utf-8")
    print(f"Launch failure {args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
