"""Bounded native processor runtime and ordinary-file transport contracts.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
Exported input worlds explicitly declare isolated consumed frames and real unit input packets; paid gameplay procurement remains Group11.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

TAGS = "[orbital-activation-contract],[orbital-service-contract],[orbital-freight-contract],[layer-recipe-batch],[utility-power-recipe],[layer-recipe-work],[orbital-recipe-inventory],[orbital-recipe-runtime]"
BASE_SOURCE = ROOT / "src/tests/fixtures/demos/underground/mine-haulage-403.sav"
BASE_SOURCE_SHA = "04b90a26658bc08dbd15ed43cefd0c5ad6de00d26f5a4fd7846e6467a35abcee"
LEGACY_INDEX = ROOT / "logs/heavy-network-verification.json"
LEGACY_INDEX_SHA = "e2587e78690a8e31643e17cdf92e4e6f38c53efff02c4e716dab4c17a1d15d03"


def legacy_source(configuration):
    """Resolve one exact genuine437 paid-world boundary without requalifying its historical executables.

    @param configuration Matching original producer configuration.
    @return Exact original save and retained dated index/report identities.
    """
    assert identity(LEGACY_INDEX)["sha256"] == LEGACY_INDEX_SHA
    data = json.loads(LEGACY_INDEX.read_text(encoding="utf-8"))
    selected = next(item for item in data["configurations"] if item["configuration"] == configuration)
    # Only original explicit mining/local membership supplies this legacy world; another resave or a newest matching filename cannot impersonate its source schema.
    for reference in selected["reports"]:
        path = ROOT / reference["path"]
        assert identity(path) == reference
        report = json.loads(path.read_text(encoding="utf-8"))
        if report.get("case") == "mining" and report.get("mode") == "local":
            assert report["configuration"] == configuration and report["complete"]
            saved = ROOT / report["after"]["path"]
            assert identity(saved) == report["after"] and chunks(saved)[0] == 437
            return saved, dict(index=identity(LEGACY_INDEX), report=reference)
    raise AssertionError("Missing original genuine437 mining/local source")


def run(configuration, scope, phase, source=None, corrupt=None, reject=False):
    """Execute one isolated real process and retain explicit assertion/artifact membership.

    @param configuration Matching current native test executable build.
    @param scope Focused runtime contracts, isolated ordinary export or actual ordinary loader.
    @param phase Exact source checkpoint kind; it never injects native progress into a loader.
    @param source Explicit original ordinary input file for the loader, never a newest-file lookup.
    @param corrupt Optional independently malformed copied-reader candidate while the valid source stays open.
    @param reject Whether the ordinary full loader must cleanly refuse the explicit malformed source before its observer runs.
    @return Invocation-owned proof of actual process, terminal, files and bounded assertion count.
    """
    scenario = "orbital-recipe-runtime" if scope == "focused" else "heavy-joined" if scope == "legacy" else "orbital-recipe-" + scope
    # Each actual process owns a fresh invocation, including the two readers in one refusal helper; shared feature views would overwrite its first terminal and semantic proof.
    context = create_run_context("space", scenario)
    context.prepare_runtime()
    if scope == "export":
        assert source is None and identity(BASE_SOURCE)["sha256"] == BASE_SOURCE_SHA
        source = BASE_SOURCE
    dated = None
    if scope == "legacy":
        assert source is None and corrupt is None and not reject
        source, dated = legacy_source(configuration)
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_ORBITAL_RECIPE_PHASE=phase)
    env.pop("OTTD_ORBITAL_RECIPE_SOURCE", None)
    env.pop("OTTD_ORBITAL_RECIPE_CORRUPT", None)
    if source is not None:
        env["OTTD_ORBITAL_RECIPE_SOURCE"] = str(source)
    if corrupt is not None:
        env["OTTD_ORBITAL_RECIPE_CORRUPT"] = str(corrupt)
    if scope in ("file", "export"):
        (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_recipe_file_test\n", encoding="utf-8")
    if scope == "legacy":
        env["OTTD_HEAVY_JOINED_SOURCE"] = str(source)
        env.pop("OTTD_HEAVY_JOINED_CORRUPT", None)
        env.pop("OTTD_HEAVY_JOINED_CAPTURE", None)
        (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_joined_test\n", encoding="utf-8")
    tag = TAGS if scope == "focused" else "[.orbital-recipe-" + scope + "]"
    if scope == "legacy":
        tag = "[.heavy-joined-native]"
    log = context.logs / (scope + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build",
                            env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    if reject:
        # An expected ordinary reader refusal must occur before native observation, with a real broken-save error and no assertion crash masquerading as validation.
        assert result.returncode == 1 and not match and "Broken savegame" in terminal and "ORBITAL_RECIPE_FILE native=true" not in terminal and "Fatal test error" not in terminal and "Assertion failed" not in terminal, log
    else:
        assert result.returncode == 0 and match, log
        if scope == "export":
            assert "ORBITAL_RECIPE_EXPORT declared_unit_inputs=true phase=" + phase in terminal, log
        elif scope == "legacy":
            assert "HEAVY_JOINED native=true saved=true" in terminal, log
        elif scope == "file":
            expected = "ORBITAL_RECIPE_FILE copied_rejected=true authority_unchanged=true" if corrupt else "ORBITAL_RECIPE_FILE native=true declared_unit_inputs=true phase=" + phase
            assert expected in terminal, log
            if corrupt is not None:
                assert "ORBITAL_RECIPE_FILE preview_error=" in terminal, log
    saves = []
    # Only outputs of this exact invocation enter its proof. Every accepted modern ordinary file must contain the complete required processing family.
    for path in sorted(context.saves.glob("*.sav")):
        version, data = chunks(path)
        assert version == 438 and b"SPRC" in data, path
        saves.append(identity(path))
    if scope == "file" and not reject:
        own = context.saves / "orbital-recipe-own.sav"
        assert chunks(source)[1][b"SPRC"] == chunks(own)[1][b"SPRC"], own
    if scope == "legacy":
        assert len(saves) == 1
        own = ROOT / saves[0]["path"]
        assert chunks(own)[1][b"SPRC"][3:] == bytes(11), own
        assert chunks(source)[1][b"SPST"] == chunks(own)[1][b"SPST"], own
    target = context.reports / ("orbital-runtime-" + scope + ".json")
    target.write_text(json.dumps(dict(_documentation=__doc__, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]) if match else 0, cases=int(match[2]) if match else 0,
                                     expected_full_refusal=reject, source=identity(source) if source else None,
                                     copied_candidate=identity(corrupt) if corrupt else None,
                                     saves=saves, terminal=identity(log), declared_unit_inputs=scope != "legacy", procurement_acceptance=False, dated_producer=dated,
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run one bounded process scope so no combined native-file burst exceeds the two-minute limit."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("focused", "export", "file", "legacy"), required=True)
    parser.add_argument("--phase", choices=("partial", "complete", "cancelled", "unpowered", "retired", "fractional"), default="partial")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--corrupt", type=Path)
    parser.add_argument("--reject", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert (args.source is not None) == (args.scope == "file")
    assert not args.reject or args.scope == "file" and args.corrupt is None
    proof = run(args.configuration, args.scope, args.phase, args.source.resolve() if args.source else None,
                args.corrupt.resolve() if args.corrupt else None, args.reject)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, scope=args.scope,
                                          phase=args.phase, complete=True, reports=[proof.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
