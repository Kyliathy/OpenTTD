"""Preserve actual unchanged435 one-part Core service and its own ordinary-resave continuations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB
Every input belongs to frozen9CBB's paid world. Original Core stock funds service of deployed20; small/heavy loose parts and production/wire rules remain unchanged.
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

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_onboard_service_checks-1.py")))
read = SHARED["read"]; AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
PHASES = ("prepared", "partial", "paused", "complete")


def native(context, configuration, phase, source):
    """Run one genuine old service operation through actual current owners.

    @param context Exact invocation-owned runtime/log/artifact destination.
    @param configuration Matching completed actual build.
    @param phase Original operation or plain reconstruction.
    @param source Exact accepted paid predecessor save.
    @return Actual successful native assertion count.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("legacy_contained_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_LEGACY_CONTAINED_SOURCE=str(source), OTTD_LEGACY_CONTAINED_PHASE=phase)
    log = context.logs / "legacy-contained.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.legacy-contained-service-native]"], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"LEGACY_CONTAINED native=true phase={phase}" in evidence, log
    return int(match[1])


def focused(configuration):
    """Verify actual shared allocation/whole mutation/retry/effect and the original contained-service contract before dependent native work.

    @param configuration Matching completed build.
    @return Exact current focused proof.
    """
    context = current_run_context("space", "legacy-contained-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[orbital-service-contract],[utility-power-ledger],[utility-power-mutation],[utility-power-effects],[utility-power-stage]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / "legacy-contained-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB; original Core and actual shared allocation/whole settlement/effects/saved retry producer boundary.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def verify(configuration, phase, parent=None):
    """Bind genuine unchanged-wire service to exact original paid-world membership.

    @param configuration Matching actual build.
    @param phase Explicit genuine old service checkpoint.
    @param parent Exact preceding current report, absent only at original frozen434 entry.
    @return Exact current native report.
    """
    context = current_run_context("space", "legacy-contained-" + phase)
    if parent is None:
        assert phase == "prepared"; predecessor, selected = SHARED["entry"](configuration, "exhausted")
    else:
        proof = read(parent); assert proof["configuration"] == configuration and PHASES.index(proof["phase"]) + 1 == PHASES.index(phase)
        predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected; assertions = native(context, configuration, phase, source)
    after = context.saves / f"legacy-contained-{phase}-435.sav"; assert chunks(after)[0] == 435 and identity(source) == selected
    target = context.reports / "legacy-contained.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB; genuine current435 old one-part/148tick Core job for original deployed20 Vehicle18/generation4, using actual pre-existing Core stock. Small15 and heavy21 keep their original loose parts; no new fuel, stock or gameplay rule. Inherited starting procurement remains unverified.",
                                     configuration=configuration, phase=phase, save_version=435, predecessor=predecessor, source=selected, after=identity(after), assertions=assertions,
                                     procurement_acceptance=False, production_or_wire_changed=False,
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def reload(configuration, proof_path, completion=None):
    """Require ordinary reconstruction and next actual service operation from that process's own resave.

    @param configuration Matching actual build.
    @param proof_path Exact accepted current checkpoint report.
    @param completion Optional next actual operation report.
    @return Exact ordinary/own-resave proof.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "legacy-contained-reload-" + proof["phase"]); previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 435; before, after = chunks(source)[1], chunks(resaved)[1]; owners = [context]
    # Physical packets, earned common work, original paid infrastructure/hulls and all other native owner families must reconstruct exactly, not merely match a declared remaining timer.
    for family in EXACT:
        assert before[family] == after[family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB; genuine ordinary reconstruction and next operation from its own native resave.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=0)
    if completion is not None:
        accepted = read(completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "legacy-contained-resumed-" + proof["phase"]); result["assertions"] = native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'legacy-contained-{accepted["phase"]}-435.sav'; expected = ROOT / accepted["after"]["path"]; assert identity(expected) == accepted["after"]
        before, after = chunks(expected)[1], chunks(resumed)[1]
        # Actual continued work and once-only completion must match every original owner; a successful replay of the uninterrupted predecessor is insufficient.
        for family in EXACT:
            assert before[family] == after[family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "legacy-contained-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Collect one bounded genuine native or ordinary/continuation burst without changing the old producer's rules."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("native", "reloads"), default="native"); parser.add_argument("--selector", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("focused",) + PHASES); parser.add_argument("--parent", type=Path)
    args = parser.parse_args(); assert not args.output.exists(); reports = []
    if args.phase is not None:
        reports = [focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    elif args.scope == "native":
        reports = [focused(args.configuration)]; parent = None
        # Each old service milestone consumes only its own exact actual predecessor, preserving the unchanged paid packet/work receipt before the next format is implemented.
        for phase in PHASES:
            parent = verify(args.configuration, phase, parent); reports.append(parent); print(args.configuration, phase, flush=True)
    else:
        selected = read(args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
        states = {read(ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}; assert set(states) == {"focused"} | set(PHASES)
        for index, phase in enumerate(PHASES):
            reports.append(reload(args.configuration, states[phase], states[PHASES[index + 1]] if index + 1 < len(PHASES) else None)); print(args.configuration, "reload", phase, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB; explicit genuine unchanged-wire service evidence membership.",
                                          configuration=args.configuration, scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
