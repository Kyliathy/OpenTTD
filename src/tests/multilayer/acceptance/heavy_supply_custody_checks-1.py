"""Run bounded actual paid heavy resupply and exact ordinary/resumed native owner checks.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB
The immutable9C consumer supplies the actual returned heavy hull. One separate fixture phase declares only missing Earth fuel after the inherited input is exhausted; procurement remains unverified.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

ENTRY_SHA = "36c4f1680a1d63a01832cee9a2185cb2567979bc9fd66433835f08dc761d3799"
AUDIT = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-5.py")))
EXACT = AUDIT["EXACT"]
PHASES = ("starting-fuel", "overfull", "prepared", "ascent", "busy", "delivered", "complete")


def read(path):
    """Read an explicit retained proof.

    @param path Repository-owned exact JSON path.
    @return Parsed original proof.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def entry(configuration):
    """Select the original already mined and returned paid native heavy hull from accepted membership.

    @param configuration Matching actual producer configuration.
    @return Exact immutable producer report and original saved consumer.
    """
    index = ROOT / "logs/heavy-mining-runtime-verification.json"; assert identity(index)["sha256"] == ENTRY_SHA
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Membership and hashes supply the original; a matching basename, current output or unrelated completed factory cannot substitute it.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("phase") == "returned" and proof.get("save_version") == 432:
            selected.append((member, proof["after"]))
    assert len(selected) == 1 and identity(ROOT / selected[0][1]["path"]) == selected[0][1]
    return selected[0]


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Execute one original native milestone or independently bounded admission refusal.

    @param context Explicit invocation-owned runtime, logs and artifacts.
    @param configuration Matching current test/game build.
    @param phase Actual mission operation, ordinary resave or malformed check.
    @param source Exact real predecessor or independently malformed full-load input.
    @param corrupt Optional copied-preview candidate with the valid original open.
    @param reject Whether startup must fail before the native observer runs.
    @return Actual assertion count; full-load refusal returns zero.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_supply_custody_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_SUPPLY_SOURCE=str(source), OTTD_HEAVY_SUPPLY_PHASE=phase)
    environment.pop("OTTD_HEAVY_SUPPLY_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_SUPPLY_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-supply-custody.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-supply-custody-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode != 0 and not match and "Broken savegame" in evidence and "HEAVY_SUPPLY native=true" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_SUPPLY native=true phase={phase}" in evidence, log
    if corrupt is not None:
        assert "HEAVY_SUPPLY preview_reject=true authority_unchanged=true" in evidence, log
    return int(match[1])


def focused(configuration):
    """Check only the active supply/custody runtime and directly affected existing logistics contract.

    @param configuration Matching completed current build.
    @return Current immutable focused proof path.
    """
    context = current_run_context("space", "heavy-supply-custody-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-supply-custody],[space-logistics-contract]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace"))
    assert result.returncode == 0 and match, log
    target = context.reports / "heavy-supply-custody-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; focused native supply/custody/source admission and directly affected old contracts.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def verify(configuration, phase, parent=None):
    """Bind one real mission stage to its exact accepted paid predecessor.

    @param configuration Matching actual native configuration.
    @param phase Explicit supply milestone.
    @param parent Exact preceding current proof, absent only at the immutable9C entry.
    @return New retained native proof path.
    """
    context = current_run_context("space", "heavy-supply-custody-" + phase)
    if parent is None:
        assert phase == "starting-fuel"; predecessor, selected = entry(configuration)
    else:
        proof = read(parent); assert proof["configuration"] == configuration; predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected
    assertions = native(context, configuration, phase, source); after = context.saves / f"heavy-supply-{phase}-433.sav"
    assert chunks(after)[0] == 433 and identity(source) == selected
    if parent is None:
        # Only this separately identified phase declares missing Earth fixture fuel; pure old-to-current reconstruction is independently verified by upgrade.
        assert chunks(source)[0] == 432
    target = context.reports / "heavy-supply-custody.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; original paid heavy receiver, actual paid supply and exact original shell custody. Inherited starting-content fixture remains; procurement acceptance is false.",
                                     configuration=configuration, phase=phase, save_version=433, predecessor=predecessor, source=selected, after=identity(after),
                                     assertions=assertions, procurement_acceptance=False, declared_surface_fuel_fixture=phase == "starting-fuel",
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path, completion=None):
    """Compare ordinary reconstruction and an actual next action from that reconstructed input.

    @param configuration Matching current native configuration.
    @param proof_path Exact original accepted checkpoint proof.
    @param completion Optional exact ordinary next milestone to reproduce from the real resave.
    @return Immutable reconstruction/continuation proof.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-supply-custody-reload-" + proof["phase"])
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration
        ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    assertions = 0; resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 433
    before, after = chunks(source)[1], chunks(resaved)[1]
    # These complete physical owners must remain byte-exact; equal queue or fleet digests cannot substitute actual cargo, finite reserve, money, engine and paid work custody.
    for family in EXACT:
        assert before[family] == after[family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; exact ordinary heavy mining reconstruction and own-resave continuation.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=assertions)
    owners = [context]
    if completion is not None:
        accepted = read(completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "heavy-supply-custody-resumed-" + proof["phase"])
        result["assertions"] += native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'heavy-supply-{accepted["phase"]}-433.sav'; expected = ROOT / accepted["after"]["path"]
        assert identity(expected) == accepted["after"]
        # Continue from this process's own ordinary resave, never from the already accepted uninterrupted input.
        for family in EXACT:
            assert chunks(expected)[1][family] == chunks(resumed)[1][family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "heavy-supply-custody-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def upgrade(configuration):
    """Reconstruct the genuine mining-only input through the ordinary game before any supply commands.

    @param configuration Matching completed game configuration.
    @return Immutable old-to-current proof preserving every physical owner byte.
    """
    predecessor, selected = entry(configuration); source = ROOT / selected["path"]
    context = current_run_context("space", "heavy-supply-custody-upgrade"); previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration
        ordinary(context, dict(name="heavy-supply-entry", snapshot=selected), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    upgraded = context.saves / "ordinary-resave.sav"; before, after = chunks(source), chunks(upgraded)
    assert before[0] == 432 and after[0] == 433 and identity(source) == selected
    # Supply admission introduces no wire/content migration: cargo, quotas, provenance and every original paid owner remain exactly unchanged.
    for family in EXACT:
        assert before[1][family] == after[1][family], ("genuine432-upgrade", family)
    target = context.reports / "heavy-supply-custody-upgrade.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; ordinary genuine432 upgrade without physical owner rewrites or granted supply.",
                                     configuration=configuration, phase="upgrade", predecessor=predecessor, source=selected, after=identity(upgraded), exact_owner_families=len(EXACT),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Select one bounded active burst with explicit accepted predecessor membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("native", "reloads"), default="native"); parser.add_argument("--selector", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("focused",) + PHASES); parser.add_argument("--parent", type=Path)
    args = parser.parse_args(); assert not args.output.exists(); reports = []
    if args.phase is not None:
        reports = [focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    elif args.scope == "native":
        reports = [focused(args.configuration), upgrade(args.configuration)]; parent = None
        # Every native stage consumes its exact immediately preceding accepted save; stage order is part of the proof.
        for phase in PHASES:
            parent = verify(args.configuration, phase, parent); reports.append(parent); print(args.configuration, phase, flush=True)
    else:
        assert args.selector is not None; selected = read(args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
        states = {read(ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}
        # Each ordinary checkpoint receives its own reconstruction; the five live predecessors also earn their next real action from that actual saved output.
        for index, phase in enumerate(PHASES):
            reports.append(reload(args.configuration, states[phase], states[PHASES[index + 1]] if index + 1 < len(PHASES) else None)); print(args.configuration, "reload", phase, flush=True)
    value = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; explicit current bounded acceptance membership.", configuration=args.configuration,
                 scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports])
    args.output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
