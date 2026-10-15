"""Verify one actual Surface vehicle through paid Space/return commodity stage.

@file commodity_loop_checks-1.py Explicit original paid predecessors, native physical delivery and exact owner replay.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group18.
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
STAGES = ("prepare", "deploy", "resupply", "return")


def native(context, configuration, stage, source):
    """Execute one bounded real physical/paid commodity stage.

    @param context Exact isolated runtime and artifact owner.
    @param configuration Built native configuration.
    @param stage Actual rail delivery, paid deployment/resupply or descent/collection.
    @param source Hash-bound original paid world or immediately preceding actual stage.
    @return Actual before/after save paths and native physical quantity/identity witnesses.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("commodity_loop_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_COMMODITY_LOOP_SOURCE=str(source), OTTD_COMMODITY_LOOP_STAGE=stage)
    log = context.logs / "native-commodity-loop.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.surface-space-loop]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"COMMODITY_LOOP native=true stage={stage}" in evidence, log
    line = next(line for line in evidence.splitlines() if line.startswith("COMMODITY_LOOP native=true "))
    row = dict(item.split("=", 1) for item in line.split()[1:])
    witness = {key: value if key in ("native", "stage") else int(value) for key, value in row.items()}
    return context.saves / f"commodity-loop-{stage}-before-423.sav", context.saves / f"commodity-loop-{stage}-after-423.sav", witness


def main():
    """Compare one actual commodity stage with saved-input replay and ordinary before/after ownership preservation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--stage", choices=STAGES, required=True)
    parser.add_argument("--predecessor", type=Path, required=True)
    args = parser.parse_args()
    predecessor = args.predecessor.resolve()
    proof = json.loads(predecessor.read_text(encoding="utf-8")); assert proof["configuration"] == args.configuration
    if args.stage == "prepare":
        assert proof["phase"] == "complete", "Use the exact accepted original paid probe-flight completion."
    else:
        assert proof["stage"] == STAGES[STAGES.index(args.stage) - 1]
    source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", "commodity-loop-verify-" + args.stage)
    before, after, witness = native(context.for_feature("space", "commodity-loop-native-" + args.stage), args.configuration, args.stage, source)
    replay_before, replay_after, replay_witness = native(context.for_feature("space", "commodity-loop-replay-" + args.stage), args.configuration, args.stage, before)
    assert witness == replay_witness
    paused_saves = {}
    # Both sides preserve the actual Surface vehicle, original Space identities, packets, phase/capital/fuel receipts and canonical company ledgers; repeated native input cannot deliver or pay twice.
    for edge, expected_path, replay_path in (("before", before, replay_before), ("after", after, replay_after)):
        assert chunks(expected_path)[0] == 423 and chunks(replay_path)[0] == 423
        expected, actual = chunks(expected_path)[1], chunks(replay_path)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("replay", edge, family)
        paused = context.for_feature("space", "commodity-loop-paused-" + args.stage + "-" + edge)
        ordinary(paused, {"name": "commodity-loop-" + args.stage + "-" + edge, "snapshot": identity(expected_path)}, args.configuration)
        resaved = paused.saves / "ordinary-resave.sav"; actual = chunks(resaved)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("ordinary", edge, family)
        paused_saves[edge] = identity(resaved)
    assert identity(source) == original
    report = context.reports / "commodity-loop.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group18; existing station Goods are declared test content, followed by actual paid Surface rail delivery/orders, original paid rocket/builder and purchased pod, native Craft resupply, descent and separate lorry collection. No fabricated vehicle pose/arrival/phase/timer/cargo and no final Goods delivery income.",
        "configuration": args.configuration, "stage": args.stage, "predecessor": identity(predecessor), "source": original,
        "before": identity(before), "after": identity(after), "replay_before": identity(replay_before), "replay_after": identity(replay_after),
        "ordinary_resaves": paused_saves, "exact_owner_families": len(EXACT), "native_witness": witness}, indent=2) + "\n", encoding="utf-8")
    print(f"Commodity loop/{args.stage}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
