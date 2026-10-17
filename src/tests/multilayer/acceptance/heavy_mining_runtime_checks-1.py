"""Run bounded actual heavy mining and exact ordinary/resumed native owner checks.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C
The immutable9B consumer supplies the paid native hull; no observer prepares new physical stock.
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

ENTRY_SHA = "bbe592959ba830cbfd24ba3c84f4f0d5b78743c4defd64c6d498c0e5ae42ffde"
AUDIT = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-5.py")))
EXACT = AUDIT["EXACT"]
PHASES = ("approach", "working", "blocked", "returned")


def read(path):
    """Read an explicit retained proof.

    @param path Repository-owned exact JSON path.
    @return Parsed original proof.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def entry(configuration):
    """Select the original separately paid native heavy hull from accepted membership.

    @param configuration Matching actual producer configuration.
    @return Exact immutable producer report and original saved consumer.
    """
    index = ROOT / "logs/heavy-commissioning-verification.json"; assert identity(index)["sha256"] == ENTRY_SHA
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Membership and hashes supply the original; a matching basename, current output or unrelated completed factory cannot substitute it.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("phase") == "complete" and proof.get("save_version") == 431:
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
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_mining_runtime_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_MINING_SOURCE=str(source), OTTD_HEAVY_MINING_PHASE=phase)
    environment.pop("OTTD_HEAVY_MINING_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_MINING_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-mining-runtime.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-mining-runtime-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode != 0 and not match and "Broken savegame" in evidence and "HEAVY_MINING native=true" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_MINING native=true phase={phase}" in evidence, log
    if corrupt is not None:
        assert "HEAVY_MINING preview_reject=true authority_unchanged=true" in evidence, log
    return int(match[1])


def focused(configuration):
    """Check only the active runtime and its directly affected small-miner/action contracts.

    @param configuration Matching completed current build.
    @return Current immutable focused proof path.
    """
    context = current_run_context("space", "heavy-mining-runtime-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-mining-runtime],[heavy-mining-tuning],[probe-shell],[probe-order-actions]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace"))
    assert result.returncode == 0 and match, log
    target = context.reports / "heavy-mining-runtime-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; focused native extraction/role/refit/return and directly affected old contracts.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def verify(configuration, phase, parent=None):
    """Bind one real mission stage to its exact accepted paid predecessor.

    @param configuration Matching actual native configuration.
    @param phase Explicit mission milestone.
    @param parent Exact preceding current proof, absent only at the immutable9B entry.
    @return New retained native proof path.
    """
    context = current_run_context("space", "heavy-mining-runtime-" + phase)
    if parent is None:
        assert phase == "approach"; predecessor, selected = entry(configuration)
    else:
        proof = read(parent); assert proof["configuration"] == configuration; predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected
    assertions = native(context, configuration, phase, source); after = context.saves / f"heavy-mining-{phase}-432.sav"
    assert chunks(after)[0] == 432 and identity(source) == selected
    target = context.reports / "heavy-mining-runtime.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; original paid heavy hull, actual finite mining and explicit module return. Inherited starting-content fixture remains; procurement acceptance is false.",
                                     configuration=configuration, phase=phase, save_version=432, predecessor=predecessor, source=selected, after=identity(after),
                                     assertions=assertions, procurement_acceptance=False,
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
    context = current_run_context("space", "heavy-mining-runtime-reload-" + proof["phase"])
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration
        ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    assertions = 0; resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 432
    before, after = chunks(source)[1], chunks(resaved)[1]
    # These complete physical owners must remain byte-exact; equal queue or fleet digests cannot substitute actual cargo, finite reserve, money, engine and paid work custody.
    for family in EXACT:
        assert before[family] == after[family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; exact ordinary heavy mining reconstruction and own-resave continuation.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=assertions)
    owners = [context]
    if completion is not None:
        accepted = read(completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "heavy-mining-runtime-resumed-" + proof["phase"])
        result["assertions"] += native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'heavy-mining-{accepted["phase"]}-432.sav'; expected = ROOT / accepted["after"]["path"]
        assert identity(expected) == accepted["after"]
        # Continue from this process's own ordinary resave, never from the already accepted uninterrupted input.
        for family in EXACT:
            assert chunks(expected)[1][family] == chunks(resumed)[1][family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "heavy-mining-runtime-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Select one bounded active burst with explicit accepted predecessor membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("native", "reloads"), default="native"); parser.add_argument("--selector", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("focused",) + PHASES); parser.add_argument("--parent", type=Path)
    args = parser.parse_args(); assert not args.output.exists(); reports = []
    if args.phase is not None:
        reports = [focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    elif args.scope == "native":
        reports = [focused(args.configuration)]; parent = None
        # Every native stage consumes its exact immediately preceding accepted save; stage order is part of the proof.
        for phase in PHASES:
            parent = verify(args.configuration, phase, parent); reports.append(parent); print(args.configuration, phase, flush=True)
    else:
        assert args.selector is not None; selected = read(args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
        states = {read(ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}
        # Each ordinary checkpoint receives its own reconstruction; the three live predecessors also earn their next real action from that actual saved output.
        for index, phase in enumerate(PHASES):
            reports.append(reload(args.configuration, states[phase], states[PHASES[index + 1]] if index + 1 < len(PHASES) else None)); print(args.configuration, "reload", phase, flush=True)
    value = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; explicit current bounded acceptance membership.", configuration=args.configuration,
                 scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports])
    args.output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
