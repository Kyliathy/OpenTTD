"""Verify one actual paid carried-probe phase pair and its normalized replay.

@file paid_probe_phase_checks-1.py Canonical before/after owner ledgers and ordinary resaves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group17.
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
PHASES = ("preparing", "ascent", "delivery", "return", "turnaround", "complete")


def native(context, configuration, phase, source):
    """Advance one existing paid phase without commissioning or paying another mission.

    @param context Isolated native runtime and artifact owner.
    @param configuration Exact built executable configuration.
    @param phase Canonical requested phase, including the just-completed idle witness.
    @param source Explicit genuine original or hash-bound predecessor snapshot.
    @return Actual before/after save paths and normalized canonical ledger witnesses.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("paid_probe_phase_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_PAID_PROBE_PHASE_SOURCE=str(source), OTTD_PAID_PROBE_PHASE_STAGE=phase)
    log = context.logs / "native-paid-probe-phase.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.paid-probe-phase]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"PAID_PROBE_PHASE native=true phase={phase}" in evidence, log
    ledger = []
    # Normalize only actual owner numbers emitted by the native observer; artifact/runtime paths never participate in gameplay or accounting equality.
    for line in evidence.splitlines():
        if line.startswith("PAID_PROBE_LEDGER "):
            row = dict(item.split("=", 1) for item in line.split()[1:])
            ledger.append({key: value if key == "edge" else int(value) for key, value in row.items()})
    assert len(ledger) == 2 and [row["edge"] for row in ledger] == ["before", "after"]
    return (context.saves / f"probe-flight-{phase}-before-423.sav",
            context.saves / f"probe-flight-{phase}-after-423.sav", ledger)


def main():
    """Run one bounded phase, replay its saved input and compare every affected financial/custody owner."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    predecessor = None
    if args.phase == "preparing":
        assert args.predecessor is None
        source = ROOT / "src/tests/fixtures/demos/space/space-launch-414.sav"
        assert identity(source)["sha256"] == "02eb9fef7bb6f81f8fcf8d63d2991d1e12043855fa74d45b0149f3204a6bf6c5"
    else:
        assert args.predecessor is not None, "Continuation needs its exact previous phase report."
        predecessor = args.predecessor.resolve()
        proof = json.loads(predecessor.read_text(encoding="utf-8"))
        assert proof["configuration"] == args.configuration and proof["phase"] == PHASES[PHASES.index(args.phase) - 1]
        source = ROOT / proof["after"]["path"]
        assert identity(source) == proof["after"]
    original = identity(source)
    context = current_run_context("space", "paid-probe-phase-verify-" + args.phase)
    before, after, ledger = native(context.for_feature("space", "paid-probe-phase-native-" + args.phase), args.configuration, args.phase, source)
    replay_before, replay_after, replay_ledger = native(context.for_feature("space", "paid-probe-phase-replay-" + args.phase), args.configuration, args.phase, before)
    assert ledger == replay_ledger, "Same saved phase must produce the same normalized financial and custody ledger."
    paused_saves = {}
    # Compare each side of the phase pair independently. Exact financial, packet, reservation, vehicle and generation owners must agree across interrupted replay and ordinary loading.
    for edge, expected_path, replay_path in (("before", before, replay_before), ("after", after, replay_after)):
        assert chunks(expected_path)[0] == 423 and chunks(replay_path)[0] == 423
        expected, actual = chunks(expected_path)[1], chunks(replay_path)[1]
        for family in EXACT:
            assert expected[family] == actual[family], (edge, family)
        paused = context.for_feature("space", "paid-probe-phase-paused-" + args.phase + "-" + edge)
        ordinary(paused, {"name": "paid-probe-phase-" + args.phase + "-" + edge, "snapshot": identity(expected_path)}, args.configuration)
        resaved = paused.saves / "ordinary-resave.sav"
        actual = chunks(resaved)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("ordinary", edge, family)
        paused_saves[edge] = identity(resaved)
    assert identity(source) == original
    report = context.reports / "paid-probe-phase.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group17; actual paid original rocket/carried probe phase pair, once-only deployment and native running/pad expenses. Ordinary final-delivery payment is separate.",
        "configuration": args.configuration, "phase": args.phase, "predecessor": identity(predecessor) if predecessor else None,
        "source": original, "before": identity(before), "after": identity(after), "replay_before": identity(replay_before), "replay_after": identity(replay_after),
        "ordinary_resaves": paused_saves, "exact_owner_families": len(EXACT), "normalized_ledger": ledger,
        "no_new_mission_fee": True, "no_cargo_income": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Paid probe phase/{args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
