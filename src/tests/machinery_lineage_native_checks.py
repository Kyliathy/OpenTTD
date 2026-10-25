"""Qualify original donor machinery through full native loading and copied receipts.

@file machinery_lineage_native_checks.py
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/m2o23-3-come-space.md Group43.
Reports and rebuildable negatives live in test-cache/merge-supply. Existing
runner receipts/homes remain under mlm/reviews/space/machinery-lineage-original
and mlm/runtime. Original saves remain immutable. Only explicit46first-tick mode advances isolated component copies.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from merge_supply_native_checks import identity

sys.stdout.reconfigure(errors="backslashreplace")

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from runner import run_native


def execute(configuration, epoch, binding_phase, negatives, legacy_external, prototype433, roundtrip, lifecycle, canonical, first_tick, input_case, command_estimates=False):
    """Run one bounded original with immutable input/executable/process identities.

    @param configuration Selected Debug or RelWithDebInfo native test build.
    @param epoch Original B427-434 catalogue boundary unless input_case selects another exact original; no header rewriting.
    @param binding_phase Optional original B434 binding lifecycle phase0..4.
    @param negatives True additionally checks reproducible malformed binding copies emitted by save-machinery-refusal.
    @param legacy_external True selects a genuine B433 nonempty external capsule original without the later provider columns.
    @param prototype433 True retains expected physical refusal for the earlier compact-cargo B433 transport prototype.
    @param roundtrip True writes/reconstructs a new merged441successor and second save while retaining all actual native owner digests.
    @param lifecycle True additionally alternates genuine A/B/merged full loads, repeated previews and late native failure/recovery for binding phase0.
    @param canonical True requires exact complete original-derived merged family names/kinds and bytes across both paused441saves.
    @param first_tick True compares one actual native tick directly from the original with its441reload, including every saved owner byte.
    @param input_case Optional independently catalogued fractional/face/bay/queued-provider component original, without modifying its epoch or state.
    @param command_estimates True checks successful/stale synchronized estimates after full merged reload without granting rights or changing live owners.
    @return Process status; completion requires actual post-load native observation.
    """
    catalogue = json.loads((ROOT / "src/tests/fixtures/saveload/lineage-originals.json").read_text(encoding="utf-8"))
    original = next(row for row in catalogue["observations"] if row["origin"] == "B" and row["version"] == epoch)
    prefix = "group43-native-b" + str(epoch)
    if epoch == 433 and not prototype433:
        # Two genuine433 originals exist. The earlier catalogue transport
        # prototype used compact unit slots; the later independent saved original
        # has shipped slots and supplies full physical-admission qualification.
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        alias = "donor/openttd3-75ea75718f/mlm/reviews/tubes/geothermal-source-files/20261017-170510-e8f000c11d/saves/geothermal-source-3.sav"
        original = manifest["inputs"][alias]
    if prototype433:
        assert epoch == 433
        prefix = "group43-native-prototype433-refusal"
    if binding_phase is not None:
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        alias = f"donor/openttd3-75ea75718f/mlm/reviews/tubes/geothermal-bindings/20261017-183312-1b5d7d4a28/saves/geothermal-binding-{binding_phase}.sav"
        original = manifest["inputs"][alias]
        prefix = "group43-native-binding-" + str(binding_phase)
    if legacy_external:
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        alias = "donor/openttd3-75ea75718f/mlm/reviews/tubes/geothermal-source-files/20261017-171059-cddac4e77d/saves/legacy387-tube-meter-bound-service.sav"
        original = manifest["inputs"][alias]
        prefix = "group43-native-legacy-external433"
    if input_case:
        assert binding_phase is None and not legacy_external and not prototype433 and not negatives
        aliases = {
            "fractional": "donor/openttd3-75ea75718f/mlm/reviews/geothermal/atomic-conversion/20261016-222730-cc33c0ee32/saves/heat-conversion-fractional.sav",
            "face-held": "donor/openttd3-75ea75718f/mlm/reviews/demo-fixes/face-loop/20261016-040520-e7e13ab341/saves/face-held.sav",
            "face-bay": "donor/openttd3-75ea75718f/mlm/reviews/demo-fixes/face-loop/20261016-040523-9ad0b5a791/saves/face-bay-return.sav",
            "queued-provider": "donor/openttd3-75ea75718f/mlm/reviews/tubes/queue-files-read/20261018-014451-b3cda2ed9e/saves/tube-queue-7-0-inspect-inspect.sav",
        }
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        original = manifest["inputs"][aliases[input_case]]
        prefix = "group46-native-" + input_case
    source = ROOT / original["path"]
    before = identity(source)
    assert before["sha256"] == original["sha256"]
    context = create_run_context("space", "machinery-lineage-original")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("machinery_lineage_test\n", encoding="utf-8")
    executable = ROOT / "build" / configuration / "openttd_tube_test.exe"
    cache = ROOT / "test-cache/merge-supply"
    cache.mkdir(parents=True, exist_ok=True)
    if roundtrip:
        assert not prototype433
        prefix = prefix.replace("group43-", "group44-") + "-merged"
    if lifecycle:
        assert roundtrip and binding_phase == 0 and not negatives
        prefix = "group45-native-candidate-lifecycle"
    if canonical:
        assert roundtrip and not lifecycle
        prefix = prefix.replace("group44-", "group46-")
    if first_tick:
        assert roundtrip and canonical and not lifecycle
        prefix += "-first-tick"
    if command_estimates:
        assert roundtrip and canonical and binding_phase == 0
        prefix = "group47-native-command-estimates"
    terminal, stdout = cache / (prefix + "-verified.txt"), cache / (prefix + "-stdout.log")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_MACHINERY_SOURCE=str(source))
    # CLI cannot carry an exact origin hint. Bootstrap an identifiable native
    # original, then the observer loads this source through the scoped B API.
    bootstrap = next(row for row in catalogue["observations"] if row["origin"] == "B" and row["version"] == 427)
    bootstrap_path = ROOT / bootstrap["path"]
    assert identity(bootstrap_path)["sha256"] == bootstrap["sha256"]
    environment["OTTD_MACHINERY_BOOTSTRAP_INPUT"] = str(bootstrap_path)
    environment["OTTD_MACHINERY_DIGEST"] = original["sha256"]
    lifecycle_original = None
    if lifecycle:
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        alias = "mlm/reviews/space/surface-supply-order-file/20261018-080131-eb756f19e1/saves/surface-supply-paid-fulfilled-440.sav"
        a_original = manifest["inputs"][alias]
        lifecycle_original = identity(ROOT / a_original["path"])
        assert lifecycle_original["sha256"] == a_original["sha256"]
        environment["OTTD_MACHINERY_LIFECYCLE_A"] = str(ROOT / a_original["path"])
        environment["OTTD_MACHINERY_LIFECYCLE_A_DIGEST"] = a_original["sha256"]
    # The catalogue's numerical B428 original is deliberately townless. Its
    # normal native refusal stays required; editor full reconstruction inspects
    # the same unmodified machinery bytes without inventing a town or gameplay.
    townless = input_case is None and epoch == 428 and binding_phase is None and not legacy_external
    if prototype433:
        environment["OTTD_MACHINERY_PROTOTYPE433"] = "1"
    if townless:
        environment["OTTD_MACHINERY_TOWNLESS"] = "1"
    if legacy_external:
        environment["OTTD_MACHINERY_LEGACY_EXTERNAL"] = "1"
    successor = cache / (prefix + ".sav")
    if roundtrip:
        environment["OTTD_MACHINERY_OUTPUT"] = str(successor)
    if first_tick:
        environment["OTTD_MERGED_FIRST_TICK"] = "1"
    if command_estimates:
        environment["OTTD_MERGED_COMMAND_ESTIMATES"] = "1"
    if negatives:
        assert binding_phase == 0
        environment["OTTD_MACHINERY_BAD_DIR"] = str(cache)
    with stdout.open("wb") as sink:
        result = run_native(context, [str(executable), "[.machinery-lineage-native]", "--reporter", "compact", "--out", str(terminal)],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    assert identity(source) == before
    transcript = stdout.read_text(encoding="utf-8", errors="replace")
    # Process exit alone cannot certify original work, ratios, carry or once-only
    # dispatch receipts. Require the actual post-adoption observation marker.
    complete = result.returncode == 0 and "MACHINERY_LINEAGE_NATIVE" in transcript
    if lifecycle:
        complete = complete and "CANDIDATE_LIFECYCLE_NATIVE" in transcript
        assert identity(ROOT / a_original["path"]) == lifecycle_original
    if first_tick:
        complete = complete and "MERGED_FIRST_TICK_NATIVE" in transcript
    if command_estimates:
        complete = complete and "MERGED_COMMAND_ESTIMATES_NATIVE" in transcript
    report = dict(_documentation=__doc__, complete=complete, epoch=epoch, input_case=input_case, source_version=int.from_bytes(source.read_bytes()[4:6], "big"), binding_phase=binding_phase, townless_editor=townless, prototype433_expected_refusal=prototype433, original=before,
                  executable=identity(executable), terminal=identity(terminal), stdout=identity(stdout),
                  observer=identity(ROOT / "src/tests/machinery_lineage_native.cpp"),
                  native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    if lifecycle:
        report["alternating_original_a"] = lifecycle_original
        report["lifecycle_observer"] = identity(ROOT / "src/tests/save_candidate_lifecycle_fixture.h")
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
        report["migration_receipt"] = dict(original_dialect="B", original_version=int.from_bytes(source.read_bytes()[4:6], "big"),
                                            successor_version=first_version, second_version=second_version,
                                            complete_native_machinery_digests_equal=True, copied_original_fields_equal=True)
        if first_tick:
            from savegame_tables import first_tick_worlds
            pair = first_tick_worlds(successor)
            report["first_tick"] = dict(before=identity(pair["before"]), direct=identity(pair["direct"]), reloaded=identity(pair["reloaded"]), families=pair["families"])
    if negatives:
        report["negative_inputs"] = [identity(cache / f"group43-b434-bad-{fault}.sav") for fault in range(23)]
    (cache / (prefix + "-report.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(terminal.read_text(encoding="utf-8", errors="replace"))
    print(transcript[-5000:])
    return 0 if complete else result.returncode or 1


def main():
    """Select one focused original instead of dispatching an entire regression.

    @return Selected original native process status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--epoch", type=int, choices=range(427, 435), default=434)
    parser.add_argument("--binding-phase", type=int, choices=range(5))
    parser.add_argument("--negatives", action="store_true")
    parser.add_argument("--legacy-external", action="store_true")
    parser.add_argument("--prototype433", action="store_true")
    parser.add_argument("--roundtrip", action="store_true")
    parser.add_argument("--lifecycle", action="store_true")
    parser.add_argument("--canonical", action="store_true")
    parser.add_argument("--first-tick", action="store_true")
    parser.add_argument("--input-case", choices=("fractional", "face-held", "face-bay", "queued-provider"))
    parser.add_argument("--command-estimates", action="store_true")
    args = parser.parse_args()
    return execute(args.configuration, args.epoch, args.binding_phase, args.negatives, args.legacy_external, args.prototype433, args.roundtrip, args.lifecycle, args.canonical, args.first_tick, args.input_case, args.command_estimates)


if __name__ == "__main__":
    raise SystemExit(main())
