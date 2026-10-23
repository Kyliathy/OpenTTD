"""Verify actual multi-cargo recovery and separate Surface boundary collection.

@file surface_collection_checks-1.py Original paid hull/pod recovery, off-center descent and partial handling replay.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group19.
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
STAGES = ("release", "handling", "settle", "unlink", "expand", "delete")


def native(context, configuration, stage, source):
    """Execute one bounded original paid recovery, handling or lifecycle stage.

    @param context Exact isolated runtime and artifact owner.
    @param configuration Built native configuration.
    @param stage Actual release, handling/settlement or unlink/expansion/deletion variant.
    @param source Hash-bound original mined world or explicitly named accepted previous stage.
    @return Actual before/after save paths and native physical quantity/identity witnesses.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_collection_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_COLLECTION_SOURCE=str(source), OTTD_COLLECTION_STAGE=stage)
    log = context.logs / "native-collection.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.surface-collection]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"SURFACE_COLLECTION native=true stage={stage}" in evidence, log
    line = next(line for line in evidence.splitlines() if line.startswith("SURFACE_COLLECTION native=true "))
    row = dict(item.split("=", 1) for item in line.split()[1:])
    witness = {key: int(value) if key in ("ore", "goods") else value for key, value in row.items()}
    return context.saves / f"collection-{stage}-before-423.sav", context.saves / f"collection-{stage}-after-423.sav", witness


def main():
    """Compare one actual recovery/collection stage with saved-input replay and ordinary before/after ownership preservation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--stage", choices=STAGES, required=True)
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    predecessor = None
    if args.stage == "release":
        assert args.predecessor is None, "Initial release uses only the immutable actual paid mined world."
        source = ROOT / "src/tests/fixtures/demos/space/space-descent-414.sav"
        original = identity(source)
        assert original["sha256"] == "46fa2a166184269ca89e8af53c302bcb771a63827ac5ee1d1ca53878668235d3"
    else:
        assert args.predecessor is not None, "Name the exact accepted preceding stage."
        predecessor = args.predecessor.resolve()
        proof = json.loads(predecessor.read_text(encoding="utf-8"))
        assert proof["configuration"] == args.configuration and proof["stage"] == ("release" if args.stage == "handling" else "handling")
        source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", "collection-verify-" + args.stage)
    before, after, witness = native(context.for_feature("space", "collection-native-" + args.stage), args.configuration, args.stage, source)
    replay_before, replay_after, replay_witness = native(context.for_feature("space", "collection-replay-" + args.stage), args.configuration, args.stage, before)
    assert witness == replay_witness
    paused_saves = {}
    # Both sides preserve actual separate stations, original paid Space identities, packet owners, phase/capital/fuel receipts and company ledgers; replay cannot recover, settle or pay twice.
    for edge, expected_path, replay_path in (("before", before, replay_before), ("after", after, replay_after)):
        assert chunks(expected_path)[0] == 423 and chunks(replay_path)[0] == 423
        expected, actual = chunks(expected_path)[1], chunks(replay_path)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("replay", edge, family)
        paused = context.for_feature("space", "collection-paused-" + args.stage + "-" + edge)
        ordinary(paused, {"name": "collection-" + args.stage + "-" + edge, "snapshot": identity(expected_path)}, args.configuration)
        resaved = paused.saves / "ordinary-resave.sav"; actual = chunks(resaved)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("ordinary", edge, family)
        paused_saves[edge] = identity(resaved)
    assert identity(source) == original
    report = context.reports / "collection.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group19; original genuinely mined20 NORE and declared builder40 GOOD payload recover through the original paid hulls/purchased shells after real service/fuelled deployment and off-center navigation. Actual boundary rail/lorry members receive10 ore/20 Goods each. Off-center descent and partial handling preserve26 ordinary/replayed owner families. No fabricated vehicle pose/cargo/phase/timer or final-delivery payment.",
        "configuration": args.configuration, "stage": args.stage, "predecessor": None if predecessor is None else identity(predecessor), "source": original,
        "before": identity(before), "after": identity(after), "replay_before": identity(replay_before), "replay_after": identity(replay_after),
        "ordinary_resaves": paused_saves, "exact_owner_families": len(EXACT), "native_witness": witness}, indent=2) + "\n", encoding="utf-8")
    print(f"Surface collection/{args.stage}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
