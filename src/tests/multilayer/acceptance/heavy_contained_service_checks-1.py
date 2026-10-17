"""Run bounded actual heavy Core work, failure branches and genuine old contained-service upgrades.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC
Frozen paid worlds supply every input. All ordinary continuations consume their own actual resave; current files never impersonate genuine legacy producers.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from runner import current_run_context, run_native

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_onboard_service_checks-1.py")))
LAYOUT = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-4.py")))
read = SHARED["read"]; AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
PARENTS = {"prepared": None, "partial": "prepared", "paused": "partial", "resumed": "paused", "last-tick": "resumed", "complete": "last-tick",
           "cancelled": "partial", "restarted": "cancelled", "outage": "partial", "restored": "outage", "sender-lost": "partial", "orphan-cancelled": "sender-lost",
           "legacy-upgrade": None, "legacy-complete": "legacy-upgrade", "pretopup": None, "premature": "pretopup"}
GROUPS = {"main": tuple(PARENTS)[:6], "failures": tuple(PARENTS)[6:12], "legacy": tuple(PARENTS)[12:]}
LEGACY_INDEX = ("logs/legacy-contained-service-verification.json", "042474085fd47f9750430043494901ee648c24e0712ec64d710a614d2a2cf698")


def entry(configuration, phase):
    """Select exact matching original paid input, stopping at its dated report/save boundary.

    @param configuration Matching original build configuration.
    @param phase Exhausted heavy, sufficient-parts partly used heavy or genuine old partial service entry.
    @return Exact original report and saved input identities.
    """
    if phase != "legacy-upgrade":
        return SHARED["entry"](configuration, "premature" if phase == "pretopup" else "exhausted")
    index = ROOT / LEGACY_INDEX[0]; assert identity(index)["sha256"] == LEGACY_INDEX[1]
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Actual frozen435 membership, not a matching filename or current relabelled job, supplies the genuine pre-heavy contained format.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("phase") == "partial" and proof.get("save_version") == 435:
            selected.append((member, proof["after"]))
    assert len(selected) == 1 and identity(ROOT / selected[0][1]["path"]) == selected[0][1]; return selected[0]


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Execute one actual operation or independently owned malformed reader.

    @param context Exact invocation-owned runtime, logs and artifacts.
    @param configuration Matching actual current build.
    @param phase Explicit native operation or copied preview.
    @param source Exact predecessor or deliberately malformed full-load candidate.
    @param corrupt Optional malformed copied candidate while the original valid world remains open.
    @param reject Whether startup must cleanly refuse before the observer runs.
    @return Actual successful assertions, or zero for a clean intended full refusal.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_contained_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_CONTAINED_SOURCE=str(source), OTTD_HEAVY_CONTAINED_PHASE=phase)
    environment.pop("OTTD_HEAVY_CONTAINED_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_CONTAINED_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-contained.log"
    with log.open("wb") as sink:
        # A timeout kills/waits only this owned child, including a stuck crash dialog. A killed/asserting/crashed child never counts as a clean saved-state refusal.
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-contained-service-native]"], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=30 if reject or corrupt is not None else 110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode == 1 and not match and "Broken savegame" in evidence and "HEAVY_CONTAINED native=true" not in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_CONTAINED native=true phase={phase}" in evidence, log
    if corrupt is not None:
        assert "HEAVY_CONTAINED preview_reject=true authority_unchanged=true" in evidence, log
    return int(match[1])


def focused(configuration):
    """Check active contained bills/roles, original Core/Dock admission and the actual common allocation/whole mutation/effect/retry producer.

    @param configuration Matching completed current build.
    @return Exact current focused proof.
    """
    context = current_run_context("space", "heavy-contained-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-contained-service],[orbital-service-contract],[orbital-dock-contract],[utility-power-ledger],[utility-power-mutation],[utility-power-effects],[utility-power-stage]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / "heavy-contained-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; active immutable bill/native work, retained Core/Dock and actual shared useful-power producer boundaries.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def verify(configuration, phase, parent=None):
    """Bind actual contained work or genuine legacy upgrade to exact physical predecessor membership.

    @param configuration Matching actual build.
    @param phase Explicit current work/failure/legacy milestone.
    @param parent Exact accepted immediate predecessor, absent only at an immutable entry.
    @return Exact current native proof.
    """
    context = current_run_context("space", "heavy-contained-" + phase)
    if parent is None:
        assert PARENTS[phase] is None; predecessor, selected = entry(configuration, phase)
    else:
        proof = read(parent); assert proof["configuration"] == configuration and proof["phase"] == PARENTS[phase]; predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected; assertions = native(context, configuration, phase, source)
    after = context.saves / f"heavy-contained-{phase}-436.sav"; assert chunks(after)[0] == 436 and identity(source) == selected
    if phase in ("legacy-upgrade", "premature"):
        before, saved = chunks(source)[1], chunks(after)[1]
        # A genuine old job gains only two unknown-role append bytes per contained owner. A fully funded precise-berth top-up refusal changes no owner at all, including the current SPST payload.
        for family in EXACT:
            if family == b"SPST" and phase == "legacy-upgrade":
                assert before[family][3:] == LAYOUT["legacy_payload"](read_stream(after)[1])
            else:
                assert before[family] == saved[family], (phase, family)
    target = context.reports / "heavy-contained.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; actual original heavy Core bill/work and genuine435 legacy migration. Explicit sender destruction leaves its actual paid shell drifting and retains service-owned parts; no replacement or compensation. Inherited starting procurement remains unverified.",
                                     configuration=configuration, phase=phase, save_version=436, predecessor=predecessor, source=selected, after=identity(after), assertions=assertions,
                                     procurement_acceptance=False, sender_destruction_fixture=phase == "sender-lost",
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def reload(configuration, proof_path, completion=None):
    """Compare ordinary reconstruction and next actual operation from that process's own resave.

    @param configuration Matching current build.
    @param proof_path Exact current checkpoint proof.
    @param completion Optional exact accepted next operation, including independent failure branches.
    @return Exact ordinary/own-resave continuation proof.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    suffix = "-" + read(completion)["phase"] if completion else ""; context = current_run_context("space", "heavy-contained-reload-" + proof["phase"] + suffix)
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 436; before, saved = chunks(source)[1], chunks(resaved)[1]; owners = [context]
    # Every real packet/work/serial/hull/native money and utility owner must reconstruct exactly; cached service counters alone cannot prove conservation.
    for family in EXACT:
        assert before[family] == saved[family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; exact physical reconstruction and each process's own actual continuation.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=0)
    if completion is not None:
        accepted = read(completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "heavy-contained-resumed-" + proof["phase"] + suffix); result["assertions"] = native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'heavy-contained-{accepted["phase"]}-436.sav'; expected = ROOT / accepted["after"]["path"]; assert identity(expected) == accepted["after"]
        before, saved = chunks(expected)[1], chunks(resumed)[1]
        for family in EXACT:
            assert before[family] == saved[family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "heavy-contained-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Collect small explicit native or ordinary bursts while retaining the complete physical predecessor graph."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=tuple(prefix + group for prefix in ("native-", "reloads-") for group in GROUPS), default="native-main")
    parser.add_argument("--selector", nargs="+", type=Path); parser.add_argument("--output", type=Path, required=True); parser.add_argument("--phase", choices=("focused",) + tuple(PARENTS)); parser.add_argument("--parent", type=Path)
    args = parser.parse_args(); assert not args.output.exists(); reports = []; states = {}
    # Only explicit current native selectors supply other physical branch parents; a newest file or another accepted configuration cannot be substituted.
    for selector in args.selector or []:
        selected = read(selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            path = ROOT / member; proof = read(path); assert proof["configuration"] == args.configuration
            if proof["phase"] != "focused":
                assert proof["save_version"] == 436 and proof["phase"] not in states; states[proof["phase"]] = path
    if args.phase is not None:
        reports = [focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    else:
        prefix, group = args.scope.split("-", 1)
        if prefix == "native":
            if group == "main":
                reports.append(focused(args.configuration))
            for phase in GROUPS[group]:
                parent = PARENTS[phase]; states[phase] = verify(args.configuration, phase, states[parent] if parent else None); reports.append(states[phase]); print(args.configuration, phase, flush=True)
        else:
            assert set(states) == set(PARENTS)
            # Forked failures each require a separate ordinary instance of their actual parent. Split scopes keep these real process pairs below a two-minute focused burst.
            for phase in GROUPS[group]:
                children = [child for child, parent in PARENTS.items() if parent == phase]
                for child in children or [None]:
                    reports.append(reload(args.configuration, states[phase], states[child] if child else None)); print(args.configuration, "reload", phase, child, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; explicit bounded current Core/legacy evidence membership.",
                                          configuration=args.configuration, scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
