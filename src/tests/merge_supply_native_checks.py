"""Run retained original supply files through the actual native application loader.

@file merge_supply_native_checks.py
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/m2o23-3-come-space.md Groups41B/41C.
Reports/transcripts live in test-cache/merge-supply. The existing runner owns
isolated process homes under mlm/runtime and rebuildable process receipts under
mlm/reviews/space/merge-supply-legacy-a or -b. Merged441successors and rebuilt malformed outputs stay in
test-cache/merge-supply; no original save is written. Only explicit46first-tick mode advances isolated component copies.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from artifact_paths import create_run_context
from runner import run_native
from merged_supply_receipts import migration_receipt


def identity(path):
    """Bind an actual input/product without treating its path as dialect evidence.

    @param path Existing repository-owned input or generated product.
    @return Exact relative path, size and full SHA256 of its current bytes.
    """
    content = path.read_bytes()
    return dict(path=path.relative_to(ROOT).as_posix(), bytes=len(content), sha256=hashlib.sha256(content).hexdigest())


def execute(configuration, origin, a_fixture, invalid_link, merged_refusals, canonical, first_tick):
    """Run one guarded full-loader/copy comparison without resaving legacy stock.

    @param configuration Current selected native test build, Debug or RelWithDebInfo.
    @param origin Original A440roundtrip or B426full/copy comparison, selected explicitly as A/B.
    @param a_fixture Exact retained A440paid-stock or four-state metadata original; both use the existing immutable input manifest.
    @param invalid_link True tests the reproducible B426negative emitted by save-supply-wire-refusal, without installing its book or normalizing its packets.
    @param merged_refusals True additionally rebuilds/refuses20malformed merged copies with valid native recovery after each.
    @param canonical True compares all original-derived merged family names/kinds and exact bytes across both new saves; no whole owner is excluded.
    @param first_tick True additionally runs and compares one actual tick directly from the original and after its merged reload.
    @return Process status; a nonzero result retains diagnostics and never claims completion.
    """
    catalogue = json.loads((ROOT / "src/tests/fixtures/saveload/lineage-originals.json").read_text(encoding="utf-8"))
    version = 440 if origin == "A" else 426
    phase = "merged-a-roundtrip" if origin == "A" else "merged-b-roundtrip"
    prefix = "group44-native-a440-" + a_fixture if origin == "A" else "group44-native-b426"
    if merged_refusals:
        assert origin == "B" and not invalid_link
        prefix += "-refusals"
    if invalid_link:
        assert origin == "B"
        phase = "legacy-b-refusal"
        prefix = "group41c-native-b426-invalid-link"
    if canonical:
        assert not invalid_link
        prefix = prefix.replace("group44-", "group46-")
    if first_tick:
        assert canonical and not merged_refusals
        prefix += "-first-tick"
    # Select the independently catalogued original identity only; this is
    # fixture routing, and the actual loader still classifies the entire raw file.
    original = next(row for row in catalogue["observations"] if row["origin"] == origin and row["version"] == version)
    if origin == "A":
        # The lineage catalogue's declared-input A440sample has artificial
        # zero CDIM weights and is copied-transport evidence only. Use genuine
        # native saved originals, never loosen dimension checks or repair bytes.
        aliases = {
            "paid": "mlm/reviews/space/surface-supply-order-file/20261018-080131-eb756f19e1/saves/surface-supply-paid-fulfilled-440.sav",
            "states": "mlm/reviews/space/surface-supply-order-file/20261018-065053-a5c60ed6e5/saves/surface-supply-order-fixture-440.sav",
        }
        manifest = json.loads((ROOT / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        original = manifest["inputs"][aliases[a_fixture]]
    source = ROOT / original["path"]
    before = identity(source)
    assert before["sha256"] == original["sha256"]
    context = create_run_context("space", "merge-supply-legacy-" + origin.lower())
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_supply_order_test\n", encoding="utf-8")
    executable = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    cache = ROOT / "test-cache/merge-supply"
    cache.mkdir(parents=True, exist_ok=True)
    terminal = cache / (prefix + "-verified.txt")
    stdout = cache / (prefix + "-stdout.log")
    roundtrip = cache / (prefix + "-roundtrip.sav")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                       OTTD_SURFACE_SUPPLY_SOURCE=str(source), OTTD_SURFACE_SUPPLY_PHASE=phase,
                       OTTD_MERGE_SUPPLY_OUTPUT=str(roundtrip))
    if first_tick:
        environment["OTTD_MERGED_FIRST_TICK"] = "1"
    if merged_refusals:
        environment["OTTD_MERGE_SUPPLY_REFUSALS"] = "1"
    if invalid_link:
        bad = cache / "group41c-b426-invalid-link.sav"
        assert bad.is_file(), "First run [save-supply-wire-refusal] to rebuild the explicit negative input"
        environment["OTTD_SURFACE_SUPPLY_BAD"] = str(bad)
    with stdout.open("wb") as sink:
        result = run_native(context, [str(executable), "[.surface-supply-order-native]", "--reporter", "compact",
                                     "--out", str(terminal)], cwd=ROOT / "build", env=environment,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    assert identity(source) == before
    transcript = stdout.read_text(encoding="utf-8", errors="replace")
    # Exit success alone cannot certify a missing observer or unmatched test
    # selection; require the actual post-load comparison's terminal marker too.
    complete = result.returncode == 0 and f"SURFACE_SUPPLY_ORDER_FILE phase={phase}" in transcript
    if first_tick:
        complete = complete and "MERGED_FIRST_TICK_NATIVE" in transcript
    receipt, comparison_error, exact_families = None, None, None
    second = Path(str(roundtrip) + ".again.sav")
    if complete and not invalid_link:
        # Native full/copy comparison is joined to an independent original-byte
        # interpretation. Both first and second441saves retain the exact source
        # Native/LegacyBfacts; original A440fixed bytes are never extended in place.
        try:
            receipt = migration_receipt(source, roundtrip, second, origin)
            if canonical:
                from savegame_tables import compare_merged_worlds
                exact_families = compare_merged_worlds(roundtrip, second)
        except (AssertionError, ValueError, KeyError) as error:
            complete, comparison_error = False, repr(error)
    report = dict(_documentation=__doc__, complete=complete, source=before,
                  executable=identity(executable), terminal=identity(terminal), stdout=identity(stdout),
                  roundtrip=identity(roundtrip) if not invalid_link and roundtrip.exists() else None,
                  second_save=identity(second) if not invalid_link and second.exists() else None,
                  migration_receipt=receipt, independent_comparison_error=comparison_error,
                  exact_merged_families=exact_families,
                  observer=identity(ROOT / "src/tests/surface_supply_order_native.cpp"),
                  receipt_reader=identity(ROOT / "src/tests/merged_supply_receipts.py"),
                  malformed=identity(bad) if invalid_link else None,
                  malformed_producer=identity(ROOT / "src/tests/save_lineage_preflight.cpp") if invalid_link else None,
                  native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    if merged_refusals:
        report["malformed_merged"] = [identity(Path(str(roundtrip) + f".bad-{fault}.sav")) for fault in range(20)
                                      if Path(str(roundtrip) + f".bad-{fault}.sav").exists()]
        report["malformed_producer"] = identity(ROOT / "src/tests/save_merged_refusal_fixture.h")
        if complete:
            assert len(report["malformed_merged"]) == 20
    # Only a completed native observation can certify resumed work; a refused
    # input has no tick outputs that may be substituted by a previous run.
    if first_tick and complete:
        from savegame_tables import first_tick_worlds
        pair = first_tick_worlds(roundtrip)
        report["first_tick"] = dict(before=identity(pair["before"]), direct=identity(pair["direct"]), reloaded=identity(pair["reloaded"]), families=pair["families"])
    (cache / (prefix + "-report.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(terminal.read_text(encoding="utf-8", errors="replace"))
    print(transcript[-4000:])
    if comparison_error:
        print(comparison_error)
    return 0 if complete else result.returncode or 1


def main():
    """Parse one explicit build selection and execute only this native supply case.

    @return Original guarded native process exit status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--origin", choices=("A", "B"), default="B")
    parser.add_argument("--a-fixture", choices=("paid", "states"), default="paid")
    parser.add_argument("--invalid-link", action="store_true")
    parser.add_argument("--merged-refusals", action="store_true")
    parser.add_argument("--canonical", action="store_true")
    parser.add_argument("--first-tick", action="store_true")
    args = parser.parse_args()
    return execute(args.configuration, args.origin, args.a_fixture, args.invalid_link, args.merged_refusals, args.canonical, args.first_tick)


if __name__ == "__main__":
    raise SystemExit(main())
