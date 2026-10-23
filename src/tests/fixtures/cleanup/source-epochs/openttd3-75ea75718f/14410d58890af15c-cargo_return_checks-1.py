"""Verify one actual cargo recall, explicit paid-pod Unload or precise Visit phase.

@file cargo_return_checks-1.py Genuine Group12/14 predecessors, canonical packet history and exact ordinary resaves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group14.
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
    """Consume the explicitly named genuine admission/unload report; no current-output lookup or privately placed hull is permitted."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", choices=("recall", "unload", "visit"), required=True)
    parser.add_argument("--predecessor", type=Path, required=True)
    args = parser.parse_args()
    predecessor = args.predecessor.resolve()
    proof = json.loads(predecessor.read_text(encoding="utf-8"))
    assert proof["configuration"] == args.configuration
    if args.phase == "visit":
        assert proof["phase"] == "unload"
    else:
        assert proof["actual_navigation"] and proof["outside_refused"] and proof["exact_owner_families"] == 26
    source = ROOT / proof["complete"]["path"]
    original = identity(source)
    assert original == proof["complete"]
    report_identity = identity(predecessor)
    context = current_run_context("space", "cargo-return-verify-" + args.phase)
    native = context.for_feature("space", "cargo-return-native-" + args.phase)
    native.prepare_runtime()
    (native.runtime_dir / "scripts/game_start.scr").write_text("cargo_return_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_CARGO_RETURN_SOURCE=str(source), OTTD_CARGO_RETURN_PHASE=args.phase)
    log = native.logs / "native-cargo-return.log"
    with log.open("wb") as sink:
        result = run_native(native, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.cargo-return-policy]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"CARGO_RETURN native=true phase={args.phase}" in evidence, log
    complete = native.saves / f"cargo-return-{args.phase}-423.sav"
    assert chunks(complete)[0] == 423
    paused = context.for_feature("space", "cargo-return-ordinary-" + args.phase)
    ordinary(paused, {"name": "cargo-return-" + args.phase, "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Current position, typed queue, original paid shell, packet/routing history and all expenses survive ordinary paused loading exactly.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original and identity(predecessor) == report_identity
    report = context.reports / "cargo-return.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group14; actual paid outside-circle return/Unload or precise Visit, no fake position/cargo, no descent payment.",
        "configuration": args.configuration, "phase": args.phase, "predecessor": report_identity, "source": original,
        "complete": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT),
        "conserved_mined_units": 20, "actual_navigation": True, "explicit_owned_target": True, "actual_tick_expense_checked": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Cargo return {args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
