"""Qualify selected original A Space receipts and an actual shared donor craft.

@file space_lineage_native_checks.py
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/m2o23-3-come-space.md Group42.
@see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 21; --late-failures refuses malformed copies late in a full load, then recovers.
Reports/transcripts live in test-cache/merge-supply. Existing runner receipts and
isolated process homes remain under mlm/reviews/space/space-lineage-original and
mlm/runtime. Optional44successors stay in test-cache/merge-supply; no original
is rewritten and the observer advances no work.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

from merge_supply_native_checks import identity

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from runner import run_native


def walk(value):
    """Yield every object nested in a JSON value.

    @param value Parsed catalogue.
    @return Generator over each dictionary, outermost first.
    """
    if isinstance(value, dict):
        yield value
        for item in value.values():
            yield from walk(item)
    elif isinstance(value, list):
        for item in value:
            yield from walk(item)


def execute(configuration, epoch, roundtrip, canonical, first_tick, late_failures, build="build"):
    """Run one exact original through full reconstruction and its copied receipt comparison.

    @param configuration Current Debug or RelWithDebInfo selected native test target.
    @param epoch Explicit A426-438catalogue epoch, donor-space426, or heavy-440 (the catalogued A440 demo), never a header transformation or filename-derived version.
    @param roundtrip True writes and reconstructs a new441successor and second save without advancing original work.
    @param canonical True compares every independently required merged family and its exact bytes across both saves.
    @param first_tick True additionally compares one actual original-first and merged-reload-first native tick over all saved owners.
    @param late_failures True refuses malformed copies late in a full load and reloads the source after each (m2o28 Group 21).
    @param build Single-configuration build tree, relative to the repository, used when no multi-configuration executable exists.
    @return Original native status, with completion requiring the actual post-load observation marker.
    """
    donor = epoch == "donor-space426"
    demo = epoch == "heavy-440"
    # Malformed copies cannot carry a digest hint, so late failures need a
    # source that its own bytes identify: A440 is beyond every colliding era.
    assert not late_failures or (demo and not roundtrip)
    if demo:
        checkpoints = json.loads((ROOT / "src/tests/fixtures/demos/space/current-checkpoints.json").read_text(encoding="utf-8"))
        original = next(row for row in walk(checkpoints) if row.get("path") == "src/tests/fixtures/demos/space/space-heavy-440.sav")
    elif donor:
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        alias = "donor/openttd3-75ea75718f/mlm/reviews/surface-supply/space-custody/20261016-182906-4998e57e76/saves/space-mining-continued-405.sav"
        original = manifest["inputs"][alias]
    else:
        catalogue = json.loads((ROOT / "src/tests/fixtures/saveload/lineage-originals.json").read_text(encoding="utf-8"))
        version = int(epoch)
        # Independent original routing does not supply runtime classification;
        # actual loaders inspect the unchanged complete raw file again.
        original = next(row for row in catalogue["observations"] if row["origin"] == "A" and row["version"] == version)
    source = ROOT / original["path"]
    before = identity(source)
    assert before["sha256"] == original["sha256"]
    if donor:
        # The preserved source basename mentions405, but its actual wire
        # version is426. Identity and header bytes, not the name, select this case.
        assert int.from_bytes(source.read_bytes()[4:6], "big") == 426
    context = create_run_context("space", "space-lineage-original")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_lineage_test\n", encoding="utf-8")
    executable = ROOT / "build" / configuration / "openttd_tube_test.exe"
    if not executable.is_file():
        executable = ROOT / build / "openttd_tube_test"  # Single-configuration (Ninja) build trees.
    cache = ROOT / "test-cache/merge-supply"
    cache.mkdir(parents=True, exist_ok=True)
    prefix = ("group44-space-" if roundtrip else "group42-native-") + epoch
    if canonical:
        assert roundtrip
        prefix = prefix.replace("group44-", "group46-")
    if first_tick:
        assert roundtrip and canonical
        prefix += "-first-tick"
    if late_failures:
        prefix = "m2o28-group21-late-failures-" + epoch
    terminal, stdout = cache / (prefix + "-verified.txt"), cache / (prefix + "-stdout.log")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SPACE_LINEAGE_SOURCE=str(source))
    successor = cache / (prefix + ".sav")
    if roundtrip:
        environment["OTTD_SPACE_LINEAGE_OUTPUT"] = str(successor)
    if first_tick:
        environment["OTTD_MERGED_FIRST_TICK"] = "1"
    if late_failures:
        environment["OTTD_SPACE_LINEAGE_LATE_FAILURES"] = "1"
    if donor:
        environment["OTTD_SPACE_LINEAGE_DONOR_SPACE"] = "1"
    elif not demo:
        # Native startup has no hint option. An independently identifiable
        # original provides only the initial runtime; the observer then loads
        # this exact A source through the scoped digest-bound native API.
        bootstrap = next(row for row in catalogue["observations"] if row["origin"] == "B" and row["version"] == 426)
        bootstrap_path = ROOT / bootstrap["path"]
        assert identity(bootstrap_path)["sha256"] == bootstrap["sha256"]
        environment["OTTD_SPACE_LINEAGE_BOOTSTRAP_INPUT"] = str(bootstrap_path)
        environment["OTTD_SPACE_LINEAGE_A_DIGEST"] = original["sha256"]
    with stdout.open("wb") as sink:
        result = run_native(context, [str(executable), "[.space-lineage-native]", "--reporter", "compact", "--out", str(terminal)],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    assert identity(source) == before
    transcript = stdout.read_text(encoding="utf-8", errors="replace")
    # A successful process with no post-load observer cannot certify any
    # native craft, construction receipt, service or retired processing history.
    complete = result.returncode == 0 and "SPACE_LINEAGE_NATIVE" in transcript
    if first_tick:
        complete = complete and "MERGED_FIRST_TICK_NATIVE" in transcript
    if late_failures:
        complete = complete and "SPACE_LATE_FAILURES" in transcript
    report = dict(_documentation=__doc__, complete=complete, epoch=epoch, original=before,
                  executable=identity(executable), terminal=identity(terminal), stdout=identity(stdout),
                  native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    if roundtrip and complete:
        from merged_supply_receipts import merged_book
        second = Path(str(successor) + ".again.sav")
        first_version, first_book, first_payload = merged_book(successor)
        second_version, second_book, second_payload = merged_book(second)
        assert first_book == second_book and first_payload == second_payload
        report["successor"] = identity(successor)
        report["second_save"] = identity(second)
        if canonical:
            from savegame_tables import compare_merged_worlds
            report["exact_merged_families"] = compare_merged_worlds(successor, second)
            report["family_contract"] = identity(ROOT / "src/tests/fixtures/saveload/merged-family-set.json")
        report["migration_receipt"] = dict(original_dialect="B" if donor else "A", original_version=int.from_bytes(source.read_bytes()[4:6], "big"),
                                            successor_version=first_version, second_version=second_version,
                                            complete_native_space_digests_equal=True, copied_original_receipts_equal=True)
        if first_tick:
            from savegame_tables import first_tick_worlds
            pair = first_tick_worlds(successor)
            report["first_tick"] = dict(before=identity(pair["before"]), direct=identity(pair["direct"]), reloaded=identity(pair["reloaded"]), families=pair["families"])
    (cache / (prefix + "-report.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(terminal.read_text(encoding="utf-8", errors="replace"))
    print(transcript[-4000:])
    return 0 if complete else result.returncode or 1


def main():
    """Select one bounded original case without dispatching an entire regression.

    @return Original selected native process status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--roundtrip", action="store_true")
    parser.add_argument("--canonical", action="store_true")
    parser.add_argument("--first-tick", action="store_true")
    parser.add_argument("--late-failures", action="store_true", help="Refuse malformed copies late in a full load, then reload the source (heavy-440 only)")
    parser.add_argument("--build", default="build", help="Single-configuration build tree, for example build-clang")
    parser.add_argument("--epoch", choices=("426", "427", "428", "429", "431", "434", "436", "437", "438", "donor-space426", "heavy-440"), required=True)
    args = parser.parse_args()
    return execute(args.configuration, args.epoch, args.roundtrip, args.canonical, args.first_tick, args.late_failures, args.build)


if __name__ == "__main__":
    raise SystemExit(main())
