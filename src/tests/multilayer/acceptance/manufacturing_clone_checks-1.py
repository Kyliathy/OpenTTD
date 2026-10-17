"""Verify one exact bounded native paid clone/source/configuration boundary.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
Initial Surface scenario content is declared before real traffic; no orbital stock, capital, hull or procurement proof is granted.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from runner import current_run_context, run_native
from space_preview_checks import ROOT, chunks, identity

MODELS = (15, 19, 20, 16, 17)
PHASES = ("focused", "prepared", "clear-core", "steel", "goods", "parts", "electronics", "service-metal", "service-electronics", "depart", "queued", "reserved", "before", "complete", "repeat", "cancel", "guards", "mutate", "source-loss", "transfer", "earth", "earth-complete", "upgrade", "legacy-complete")
VARIANTS = ("main", "mutated", "lost", "transferred", "cancelled", "guards", "earth", "legacy")
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
ENTRY = ROOT / "logs/manufacturing-purchase-verification.json"
ENTRY_SHA256 = "13e9ddaa67992395af5c647f06a86ed8ffa080c0d5a5b5b59bd56555a793f6ce"


def native(context, configuration, phase, model, source, original=None):
    """Run one exact loaded-world observer through real native owners with bounded execution.

    @param context Explicit unique operation/variant artifact and native data-home owner.
    @param configuration Matching current game/test configuration.
    @param phase One bounded real traffic, clone, completion or original-source lifecycle operation.
    @param model Actual original canonical role from the accepted producer.
    @param source Exact immutable native predecessor save.
    @param original Previously observed actual source incarnation; absent only for first producer/legacy entry.
    @return Actual output checkpoint, assertion count, declared Surface additions and observed source metadata.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("manufacturing_clone_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_CLONE_SOURCE=str(source), OTTD_CLONE_ACTION=phase,
                       OTTD_CLONE_MODEL=str(model), OTTD_TEST_CONFIGURATION=configuration)
    environment.pop("OTTD_CLONE_ORIGINAL_ID", None); environment.pop("OTTD_CLONE_ORIGINAL_GENERATION", None)
    if original is not None:
        environment.update(OTTD_CLONE_ORIGINAL_ID=str(original["vehicle"]), OTTD_CLONE_ORIGINAL_GENERATION=str(original["generation"]))
    log = context.logs / "native-manufacturing-clone.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.manufacturing-clone-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    marker = re.search(r"MANUFACTURE_CLONE native=true phase=(\S+) model=(\d+) original_id=(\d+) original_generation=(\d+) original_alive=(true|false) original_assets=(\d+) procurement=false", evidence)
    # An absent tag, unrelated successful case or observer that never reached its actual ownership checks cannot provide this boundary's evidence.
    assert result.returncode == 0 and passed and int(passed[2]) == 1 and marker, log
    assert (marker[1], int(marker[2])) == (phase, model), log
    observed = dict(vehicle=int(marker[3]), generation=int(marker[4]))
    additions = [dict(cargo_index=int(cargo), quantity=int(quantity)) for cargo, quantity in re.findall(r"CLONE_SURFACE_FIXTURE cargo=(\d+) added=(\d+)", evidence)]
    # Only independent initial scenario preparation may declare Surface content; every later delivery/work/source operation uses actual already saved owners.
    assert not additions or phase == "prepared", log
    if original is not None:
        assert observed == original, (log, "original source incarnation changed")
    return context.saves / f"clone-{phase}-{model}-428.sav", int(passed[1]), additions, observed, marker[5] == "true", int(marker[6])


def verify(configuration, phase, model=15, predecessor=None, variant="main"):
    """Bind one immutable producer/current predecessor to an independent accepted operation report.

    @param configuration Matching completed native product/test configuration.
    @param phase One focused or actual bounded native observer operation.
    @param model Actual canonical role commissioned by the original recorded producer.
    @param predecessor Exact accepted original/current report; no latest-run lookup or copied private fixture state.
    @param variant Independent branch owner so interrupted/mutated/lost-source completion cannot overwrite the main proof.
    @return Exact current accepted hash-qualified proof report.
    """
    assert variant in VARIANTS and phase in PHASES and model in MODELS
    context = current_run_context("space", f"manufacturing-clone-verify-{variant}-{phase}-{model}")
    report = context.reports / "manufacturing-clone.json"
    assert not report.exists(), "A distinct variant/invocation must own every immutable accepted proof before a native child can write."
    if phase == "focused":
        assert predecessor is None
        log = context.logs / "manufacturing-clone-focused.log"
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[manufacturing-clone]"],
                       cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed and int(passed[2]) == 6, "All six actual current clone contracts must be present."
        result = dict(configuration=configuration, phase=phase, model=model, variant=variant,
                      assertions=int(passed[1]), cases=int(passed[2]), log=identity(log))
    else:
        assert predecessor is not None
        previous = json.loads(predecessor.read_text(encoding="utf-8")); assert previous["configuration"] == configuration
        entry = phase in ("prepared", "upgrade", "legacy-complete")
        if entry:
            # Original427 purchase proofs pin actual paid source hulls/factories. Their dated overwritten source/executable metadata remains an explicit historical boundary.
            assert identity(ENTRY)["sha256"] == ENTRY_SHA256
            accepted = next(row for row in json.loads(ENTRY.read_text(encoding="utf-8"))["configurations"] if row["configuration"] == configuration)
            assert identity(predecessor) in accepted["reports"]
            expected = ("before", 16) if phase == "legacy-complete" else ("complete", model)
            assert (previous["phase"], previous["model"]) == expected and previous["save_version"] == 427
            original = None
        else:
            assert previous["save_version"] == 428 and previous["model"] == model
            expected = {"clear-core": ("prepared",), "steel": ("clear-core",), "goods": ("steel",), "parts": ("goods",), "electronics": ("parts",),
                        "service-metal": ("goods",), "service-electronics": ("service-metal",),
                        "depart": ("electronics",) if model in (16, 17) else ("service-electronics",) if model == 20 else ("goods",),
                        "queued": ("depart",), "reserved": ("queued", "mutate"), "before": ("reserved",), "complete": ("before", "source-loss", "transfer"),
                        "repeat": ("complete",), "cancel": ("before",), "guards": ("depart",), "mutate": ("queued",),
                        "source-loss": ("before",), "transfer": ("before",), "earth": ("depart",), "earth-complete": ("earth",)}
            assert previous["phase"] in expected[phase], (phase, previous["phase"])
            original = previous["original"]
        if phase in ("parts", "electronics"):
            assert model in (16, 17)
        if phase in ("service-metal", "service-electronics", "transfer"):
            assert model == 20
        if phase in ("guards", "mutate", "earth", "earth-complete"):
            assert model == 15
        if phase == "source-loss":
            assert model == 19
        if phase == "legacy-complete":
            assert model == 16
        selected = previous["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
        after, assertions, additions, observed, alive, assets = native(context, configuration, phase, model, source, original)
        assert chunks(after)[0] == 428 and identity(source) == selected
        legacy = phase in ("upgrade", "legacy-complete")
        if legacy:
            assert observed["generation"] == 0
        else:
            assert observed["generation"] > 0
        if not entry:
            # Real source loading in depart20 and actual source sale have explicit ownership effects; clone/work/name/order/transfer operations preserve the original physical asset cohort.
            changed_assets = phase == "source-loss" or (phase == "depart" and model == 20)
            if not changed_assets:
                assert alive == previous["original_alive"] and assets == previous["original_asset_digest"], (phase, "original assets changed")
        result = dict(configuration=configuration, phase=phase, model=model, variant=variant, assertions=assertions,
                      predecessor=identity(predecessor), source=selected, after=identity(after), original=None if legacy else observed,
                      original_alive=alive, original_asset_digest=assets, surface_fixture_added=additions,
                      additional_surface_starting_content=bool(additions) or previous.get("additional_surface_starting_content", False))
    result.update(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BB; actual ordinary paid clone, original source assets and independent captured configuration. Declared Surface scenario content before real traffic; no procurement claim.",
                  save_version=428, surface_input_fixture=True, procurement_acceptance=False,
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    assert not report.exists(), "A distinct variant/invocation must own every immutable accepted proof."
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact phase/model/variant/configuration and its explicit immutable predecessor."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--model", type=int, choices=MODELS, default=15)
    parser.add_argument("--variant", choices=VARIANTS, default="main")
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.model, args.predecessor.resolve() if args.predecessor else None, args.variant).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
