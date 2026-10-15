"""Verify both release-command orders on the original materially paid moving miner.

@file release_command_order_checks-1.py Native current-position/target admission, exact saved-input replay and ordinary owner resaves.
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


def native(context, configuration, ordering, source):
    """Run current native command admission through actual outward/return navigation.

    @param context Isolated native writer/runtime owner.
    @param configuration Built native configuration.
    @param ordering Explicit valid-first or invalid-first public command sequence.
    @param source Original paid mined world, or the exact saved input for replay.
    @return Native before/after snapshots retaining the sole original paid sender and shell.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("release_order_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_RELEASE_ORDER_SOURCE=str(source), OTTD_RELEASE_ORDER_MODE=ordering)
    log = context.logs / "native-release-order.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.release-command-order]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in text and f"RELEASE_ORDER native=true ordering={ordering}" in text, log
    assert "stale_inside_quote_refused=true" in text and "explicit_target=true" in text and "once_only=true" in text
    return context.saves / f"release-order-{ordering}-before-423.sav", context.saves / f"release-order-{ordering}-after-423.sav"


def main():
    """Verify one bounded public-command ordering, replay and ordinary 26-owner save preservation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--ordering", choices=("valid-first", "invalid-first"), required=True)
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-descent-414.sav"
    original = identity(source)
    assert original["sha256"] == "46fa2a166184269ca89e8af53c302bcb771a63827ac5ee1d1ca53878668235d3"
    context = current_run_context("space", "release-order-verify-" + args.ordering)
    before, after = native(context.for_feature("space", "release-order-native-" + args.ordering), args.configuration, args.ordering, source)
    replay_before, replay_after = native(context.for_feature("space", "release-order-replay-" + args.ordering), args.configuration, args.ordering, before)
    paused_saves = {}
    # Both sides retain exact current positions, explicit recovery references, packet/capital/provenance owners and pending return reservations; invalid or repeated commands earn no second transfer.
    for edge, expected_path, replay_path in (("before", before, replay_before), ("after", after, replay_after)):
        assert chunks(expected_path)[0] == 423 and chunks(replay_path)[0] == 423
        expected, actual = chunks(expected_path)[1], chunks(replay_path)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("replay", edge, family)
        paused = context.for_feature("space", "release-order-paused-" + args.ordering + "-" + edge)
        ordinary(paused, {"name": "release-order-" + args.ordering + "-" + edge, "snapshot": identity(expected_path)}, args.configuration)
        saved = paused.saves / "ordinary-resave.sav"; actual = chunks(saved)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("ordinary", edge, family)
        paused_saves[edge] = identity(saved)
    assert identity(source) == original
    report = context.reports / "release-command-order.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group18.4. Original paid miner/shell/ten mined units navigate outside and return inside through native orders. A formerly valid inside quote refuses after movement. Foreign/valid commands in either sequence preserve current target authority; repeat release refuses after once-only physical custody change. No private positions, cargo, phases or timers. Ordinary/replayed26-owner pairs.",
        "configuration": args.configuration, "ordering": args.ordering, "source": original, "before": identity(before), "after": identity(after),
        "replay_before": identity(replay_before), "replay_after": identity(replay_after), "ordinary_resaves": paused_saves,
        "exact_owner_families": len(EXACT), "actual_navigation": True, "current_position_rechecked": True, "foreign_or_duplicate_no_mutation": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Release command/{args.ordering}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
