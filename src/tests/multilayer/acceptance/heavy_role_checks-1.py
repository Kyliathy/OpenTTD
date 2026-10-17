"""Execute bounded original two-miner role/lease/upkeep checkpoints and their own-resave continuations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9D
The exact joined437 upgrade of genuine433 descent supplies both paid deployed miners. No hull, pod, pose, reserve, work or quota fixture is introduced.
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

JOINED = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-1.py")))
PHYSICAL = JOINED["EXACT"]; EXACT = (*PHYSICAL, b"CDIM")
ENTRY = ("logs/heavy-joined-verification.json", "255a3cf940bc341a0145e1b0283cd12d56f298f09be3e1bf4288c4a7be5d3dfe")
PARENTS = {"recovered": None, "approach": "recovered", "attached": "approach", "partial": "attached", "contended": "partial", "legacy-attached": "contended", "legacy-work": "legacy-attached"}
GROUPS = {"start": tuple(PARENTS)[:4], "end": tuple(PARENTS)[4:]}
read = JOINED["read"]


def entry(configuration):
    """Select the actual current upgrade of the genuine older small-miner descent.

    @param configuration Exact matching original producer configuration.
    @return Dated index, actual native upgrade report and its unchanged saved input identities.
    """
    index = ROOT / ENTRY[0]; assert identity(index)["sha256"] == ENTRY[1]
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Only exact native old-descent membership supplies both real deployed hulls; an ordinary copy or another branch cannot impersonate this paid original.
    for reference in accepted["reports"]:
        path = ROOT / reference["path"]; assert identity(path) == reference; proof = read(path)
        if proof.get("case") == "old-descent" and "after" in proof:
            assert proof["source_version"] == 433 and proof["save_version"] == 437; selected.append((reference, proof["after"]))
    assert len(selected) == 1; producer, source = selected[0]; assert identity(ROOT / source["path"]) == source
    return identity(index), producer, source


def native(context, configuration, phase, source):
    """Run one actual original-world native role checkpoint.

    @param context Invocation-owned runtime/log/save directories.
    @param configuration Exact current test executable configuration.
    @param phase Explicit normal recovery, actual pursuit/work/lease or pure inspect operation.
    @param source Exact accepted physical predecessor.
    @return Actual passing assertion count with retained terminal/executable membership.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_role_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_ROLE_SOURCE=str(source), OTTD_HEAVY_ROLE_PHASE=phase)
    environment.pop("OTTD_HEAVY_ROLE_CORRUPT", None); log = context.logs / "heavy-role.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-role-native]"], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and match and int(match[2]) == 1 and "HEAVY_ROLE native=true phase=" + phase in evidence, log
    return int(match[1])


def focused(configuration):
    """Verify only active per-model work/service and same-file join contracts before native acceptance.

    @param configuration Exact current test executable configuration.
    @return Focused producer-boundary proof with retained actual process evidence.
    """
    context = current_run_context("space", "heavy-role-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-mining-tuning],[heavy-mining-runtime],[heavy-onboard-service],[heavy-joined-validation]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / "heavy-role-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9D; immutable role/native work/service and independent copied joins. Isolated unit inputs claim no procurement.", configuration=configuration,
                                     phase="focused", assertions=int(match[1]), cases=int(match[2]), native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def verify(configuration, phase, parent=None):
    """Bind one actual role checkpoint to its exact accepted predecessor.

    @param configuration Matching current native configuration.
    @param phase Explicit original-world action in the source task order.
    @param parent Actual immediate native predecessor, absent only at genuine recovery entry.
    @return Current assertion/save/process proof, without recursive historical implementation claims.
    """
    if parent is None:
        assert phase == "recovered"; index, predecessor, selected = entry(configuration)
    else:
        proof = read(parent); assert proof["configuration"] == configuration and proof["phase"] == PARENTS[phase]; index, predecessor, selected = None, identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected; context = current_run_context("space", "heavy-role-" + phase)
    assertions = native(context, configuration, phase, source); saved = context.saves / ("heavy-role-" + phase + "-437.sav"); assert chunks(saved)[0] == 437 and identity(source) == selected
    target = context.reports / "heavy-role.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9D; two original paid miners, real moving finite body/lease, canonical expense and actual work. No asset/work/pose/quota fixture.",
                                     configuration=configuration, phase=phase, save_version=437, entry_index=index, predecessor=predecessor, source=selected, after=identity(saved), assertions=assertions,
                                     procurement_acceptance=False, native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path, completion=None):
    """Compare ordinary reconstruction and its own actual next action, including terminal admission.

    @param configuration Matching current native/product configuration.
    @param proof_path Exact accepted native checkpoint proof.
    @param completion Exact accepted immediate child; terminal checkpoints use pure native inspect.
    @return Ordinary/own-resave proof with complete actual process membership and byte-exact expected physical families.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-role-reload-" + proof["phase"]); previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    saved = context.saves / "ordinary-resave.sav"; assert chunks(saved)[0] == 437; original, reconstructed = chunks(source)[1], chunks(saved)[1]
    # Every original owner and configured-dimension byte survives actual ordinary reconstruction before any next action is attempted.
    for family in EXACT:
        assert original[family] == reconstructed[family], (proof["phase"], family)
    expected, phase = proof["after"], "inspect"
    if completion is not None:
        child = read(completion); assert child["configuration"] == configuration and child["predecessor"] == identity(proof_path); expected, phase = child["after"], child["phase"]
    resumed_context = context.for_feature("space", "heavy-role-resumed-" + proof["phase"]); assertions = native(resumed_context, configuration, phase, saved)
    resumed = resumed_context.saves / ("heavy-role-" + phase + "-437.sav"); expected_families, actual_families = chunks(ROOT / expected["path"])[1], chunks(resumed)[1]
    # Own-resave continuation executes the same genuine action from its own reconstructed input; a borrowed later checkpoint cannot supply its result.
    for family in EXACT:
        assert expected_families[family] == actual_families[family], (proof["phase"], phase, family)
    target = context.reports / "heavy-role-reload.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9D; exact ordinary owners and actual own-resave next action or terminal native admission.", configuration=configuration,
                                     phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(saved), completion=identity(completion) if completion else None,
                                     resumed=identity(resumed), expected=expected, assertions=assertions, native_reports=[identity(p) for owner in (context, resumed_context) for p in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run explicit small native or own-resave scopes in original task order; no full regression or newest-input selection."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("focused", "native-start", "native-end", "reload-start", "reload-end"), required=True); parser.add_argument("--selector", nargs="+", type=Path)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); states = {}; reports = []
    # Only exact completed explicit native scope membership supplies downstream action inputs; dated joined entry membership is handled independently by entry().
    for selector in args.selector or []:
        selected = read(selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            path = ROOT / member; proof = read(path); assert proof["phase"] not in states; states[proof["phase"]] = path
    if args.scope == "focused":
        reports.append(focused(args.configuration))
    else:
        mode, group = args.scope.split("-", 1)
        # Each bounded action consumes its own actual immediate predecessor; later ordinary scopes execute the same original next action from each process's own resave.
        for phase in GROUPS[group]:
            if mode == "native":
                parent = PARENTS[phase]; states[phase] = verify(args.configuration, phase, states[parent] if parent else None); reports.append(states[phase])
            else:
                child = next((p for p, parent in PARENTS.items() if parent == phase), None); reports.append(reload(args.configuration, states[phase], states[child] if child else None))
            print(args.configuration, args.scope, phase, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9D; explicit bounded original paid role/mining membership.", configuration=args.configuration,
                                          scope=args.scope, complete=True, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
