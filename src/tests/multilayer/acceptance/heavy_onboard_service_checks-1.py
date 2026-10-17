"""Run bounded actual heavy onboard service and exact ordinary/own-resave continuations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC
Original paid parts/hulls/quota come from frozen predecessors. The target-loss branch explicitly retires/reinstalls the same paid footprint; procurement acceptance remains false.
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

AUDIT = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-5.py")))
EXACT = AUDIT["EXACT"]
ENTRIES = {
    "exhausted": ("logs/heavy-descent-verification.json", "73dfd9264f301f1b2a0a35c6bf53823cd737439f77f47721dc704c5b7e99dba7", "recovered", 434),
    "premature": ("logs/heavy-supply-custody-verification.json", "02160a11c6f8ce663eefd1e78cedb32799155a559900243cf0bef72d8f2724d6", "complete", 433),
}
PARENTS = {"prepared": None, "partial": "prepared", "paused": "partial", "resumed": "paused", "last-tick": "resumed", "complete": "last-tick",
           "ordered": None, "ordered-complete": "ordered", "target-lost": "ordered", "lost-complete": "target-lost", "premature": None}
MAIN_PHASES = ("prepared", "partial", "paused", "resumed", "last-tick", "complete", "premature")


def read(path):
    """Read one exact retained proof.

    @param path Repository-owned JSON path.
    @return Parsed original proof.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def entry(configuration, kind):
    """Select exact dated producer membership, stopping before overwritten old implementation/executable references.

    @param configuration Matching original build configuration.
    @param kind Exhausted recovered heavy or sufficient-parts partly used allowance.
    @return Exact immutable producer report and original save identity.
    """
    name, expected, phase, version = ENTRIES[kind]; index = ROOT / name; assert identity(index)["sha256"] == expected
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Exact frozen report/save membership supplies the original paid heavy; a matching basename or current serialized copy cannot substitute an older producer.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("phase") == phase and proof.get("save_version") == version:
            selected.append((member, proof["after"]))
    assert len(selected) == 1 and identity(ROOT / selected[0][1]["path"]) == selected[0][1]
    return selected[0]


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Execute one actual operation or independent malformed reader under an owned subprocess limit.

    @param context Explicit invocation-owned runtime, logs and artifacts.
    @param configuration Matching current test/game build.
    @param phase Actual service milestone or detached preview.
    @param source Exact predecessor or owned malformed full-load input.
    @param corrupt Optional malformed copied candidate while the valid original stays open.
    @param reject Whether startup must cleanly refuse before the observer runs.
    @return Successful assertion count; intended clean full refusal returns zero.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_onboard_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_ONBOARD_SOURCE=str(source), OTTD_HEAVY_ONBOARD_PHASE=phase)
    environment.pop("OTTD_HEAVY_ONBOARD_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_ONBOARD_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-onboard.log"
    with log.open("wb") as sink:
        # Malformed readers must reject promptly. subprocess.run kills/waits only this owned child on timeout, including any stuck crash dialog; a killed/crashed child never counts as clean refusal.
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-onboard-service-native]"], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=30 if reject or corrupt is not None else 110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode == 1 and not match and "Broken savegame" in evidence and "HEAVY_ONBOARD native=true" not in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_ONBOARD native=true phase={phase}" in evidence, log
    if corrupt is not None:
        assert "HEAVY_ONBOARD preview_reject=true authority_unchanged=true" in evidence, log
    return int(match[1])


def focused(configuration):
    """Check only the active onboard bill/source/work and directly affected original manifest/action contracts.

    @param configuration Matching completed current build.
    @return Current immutable focused proof path.
    """
    context = current_run_context("space", "heavy-onboard-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-onboard-service],[space-logistics-contract],[probe-order-actions]"], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / "heavy-onboard-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; active immutable bills/source/work and directly affected old manifest/action boundaries.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def verify(configuration, phase, parent=None):
    """Bind actual service to exact paid predecessor membership, preserving a pure refused older input.

    @param configuration Matching actual build.
    @param phase Explicit service milestone.
    @param parent Exact preceding current proof, absent only at a frozen producer entry.
    @return New immutable actual-world proof path.
    """
    context = current_run_context("space", "heavy-onboard-" + phase)
    if parent is None:
        assert PARENTS[phase] is None; predecessor, selected = entry(configuration, "premature" if phase == "premature" else "exhausted")
    else:
        proof = read(parent); assert proof["configuration"] == configuration and proof["phase"] == PARENTS[phase]; predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected
    assertions = native(context, configuration, phase, source); after = context.saves / f"heavy-onboard-{phase}-435.sav"; assert chunks(after)[0] == 435 and identity(source) == selected
    if phase == "premature":
        before, saved = chunks(source)[1], chunks(after)[1]
        # The actual older supply-only world has sufficient parts but partly used quota. A refused start must preserve every native owner exactly; its empty old SPDJ needs no invented receipt normalization.
        for family in EXACT:
            assert before[family] == saved[family], ("premature", family)
    target = context.reports / "heavy-onboard.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; actual paid heavy2RFMT/2ELEC and296 earned opportunities. Target loss explicitly retires/reinstalls the same paid footprint and recovers the original rocket, without new infrastructure acceptance. Inherited Surface starting content remains; procurement acceptance is false.",
                                     configuration=configuration, phase=phase, save_version=435, predecessor=predecessor, source=selected, after=identity(after), assertions=assertions,
                                     procurement_acceptance=False, retired_center_fixture=phase == "target-lost",
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path, completion=None):
    """Compare ordinary reconstruction and the next actual action from that process's own saved output.

    @param configuration Matching actual build.
    @param proof_path Exact checkpoint report.
    @param completion Optional exact accepted next operation, including independent target-loss continuation.
    @return Immutable ordinary/own-resave proof path.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    suffix = "-" + read(completion)["phase"] if completion else ""; context = current_run_context("space", "heavy-onboard-reload-" + proof["phase"] + suffix)
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 435; owners = [context]; before, saved = chunks(source)[1], chunks(resaved)[1]
    # Every physical packet, history, native money/engine, finite resource and actual work owner must remain exact rather than only matching cached manifest totals.
    for family in EXACT:
        assert before[family] == saved[family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; exact ordinary reconstruction and each process's own-resave service continuation.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=0)
    if completion is not None:
        accepted = read(completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "heavy-onboard-resumed-" + proof["phase"] + suffix); result["assertions"] = native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'heavy-onboard-{accepted["phase"]}-435.sav'; expected = ROOT / accepted["after"]["path"]; assert identity(expected) == accepted["after"]
        left, right = chunks(expected)[1], chunks(resumed)[1]
        for family in EXACT:
            assert left[family] == right[family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "heavy-onboard-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Select one bounded native or split reconstruction burst through exact accepted checkpoint membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("native", "reloads-main", "reloads-orders"), default="native"); parser.add_argument("--selector", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("focused",) + tuple(PARENTS)); parser.add_argument("--parent", type=Path)
    args = parser.parse_args(); assert not args.output.exists(); reports = []
    if args.phase is not None:
        reports = [focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    elif args.scope == "native":
        reports = [focused(args.configuration)]; states = {}
        # Each native interruption/ordered/lost-target branch consumes its own original immediate predecessor; independent physical branches cannot lend each other parts or allowance.
        for phase, parent in PARENTS.items():
            states[phase] = verify(args.configuration, phase, states[parent] if parent else None); reports.append(states[phase]); print(args.configuration, phase, flush=True)
    else:
        assert args.selector is not None; selected = read(args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
        states = {read(ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}; assert set(states) == {"focused"} | set(PARENTS)
        # Split the actual ordinary/continuation collector into small manual and ordered bursts; every live branch still starts from its own real resave.
        for phase in PARENTS:
            if (phase in MAIN_PHASES) != (args.scope == "reloads-main"):
                continue
            children = [child for child, parent in PARENTS.items() if parent == phase]
            for child in children or [None]:
                reports.append(reload(args.configuration, states[phase], states[child] if child else None)); print(args.configuration, "reload", phase, child, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; explicit bounded actual native/ordinary acceptance membership.",
                                          configuration=args.configuration, scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
