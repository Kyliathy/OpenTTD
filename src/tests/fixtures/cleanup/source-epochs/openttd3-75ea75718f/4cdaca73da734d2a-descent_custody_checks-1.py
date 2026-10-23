"""Execute one bounded genuine mined-cargo descent or service save boundary.

@file descent_custody_checks-1.py Exact inputs, immediate native checkpoints and ordinary 26-family paused resaves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group13.
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
PHASES = ("release", "before-first", "first", "before-second", "second", "service", "before-service", "complete-service", "retarget", "replacement")


def main():
    """Run only the requested explicit phase; continuation sources must be named, never inferred from a latest output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--source", type=Path)
    args = parser.parse_args()
    if args.source is None:
        assert args.phase in ("release", "retarget"), "Every continuation must name its exact genuine predecessor."
        source = ROOT / "src/tests/fixtures/demos/space/space-descent-414.sav"
    else:
        source = args.source.resolve()
    original = identity(source)
    context = current_run_context("space", "descent-custody-verify-" + args.phase)
    native = context.for_feature("space", "descent-custody-native-" + args.phase)
    native.prepare_runtime()
    (native.runtime_dir / "scripts/game_start.scr").write_text("descent_custody_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_DESCENT_CUSTODY_SOURCE=str(source), OTTD_DESCENT_CUSTODY_PHASE=args.phase)
    log = native.logs / "native-descent-custody.log"
    with log.open("wb") as sink:
        result = run_native(native, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.descent-custody]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"DESCENT_CUSTODY native=true phase={args.phase}" in evidence, log
    complete = native.saves / f"descent-{args.phase}-423.sav"
    assert chunks(complete)[0] == 423
    paused = context.for_feature("space", "descent-custody-ordinary-" + args.phase)
    ordinary(paused, {"name": "descent-custody-" + args.phase, "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Every interrupted owner, timer, packet, pending return and service expense survives ordinary paused loading exactly.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "descent-custody.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group13; one genuine paid-world phase, immediate release/completion/service checkpoint and ordinary paused resave. Full-storage and retired-center boundaries are separate isolated tests.",
        "configuration": args.configuration, "phase": args.phase, "source": original, "complete": identity(complete), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "conserved_mined_units": 20, "actual_tick_expense_checked": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Descent custody {args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
