"""Build an actual paid Dock from the original recovered heavy world in bounded native steps.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC
Only a separately saved Surface fuel shortfall may extend declared starting content. Orbital material, work and facilities always come from actual commands and packets.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, read_stream
from runner import current_run_context, run_native

CORE = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-1.py")))
PARENTS = {"inspected": None, "starting-fuel": "inspected", "blueprint": "starting-fuel", "steel": "blueprint", "goods": "steel",
           "frame": "goods", "activating": "frame", "ready": "activating"}
PRODUCER = tuple(PARENTS)
WORK_STEPS = ("prepared", "partial", "paused", "outage", "restored", "first-last", "first-complete", "second-last", "complete", "cancelled")
# The two role orderings fork from the same actually paid Dock; cancellation forks from that branch's own paused work instead of borrowing another branch's bill.
for role in ("heavy", "legacy"):
    previous = "ready"
    for step in WORK_STEPS:
        phase = role + "-" + step; PARENTS[phase] = role + "-paused" if step == "cancelled" else previous
        if step != "cancelled":
            previous = phase
GROUPS = {"producer": PRODUCER, **{role: tuple(role + "-" + step for step in WORK_STEPS) for role in ("heavy", "legacy")}}
RELOADS = {"material": PRODUCER[:4], "frame": PRODUCER[4:],
           **{role + "-start": GROUPS[role][:4] for role in ("heavy", "legacy")},
           **{role + "-end": GROUPS[role][4:] for role in ("heavy", "legacy")}}
EXACT = CORE["EXACT"]


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Run one actual original-world command boundary or independently owned malformed reader.

    @param context Invocation-owned runtime, logs and artifact directories.
    @param configuration Exact matching current test build.
    @param phase Native construction or copied-preview operation.
    @param source Exact actual predecessor, or deliberately malformed full-load candidate.
    @param corrupt Optional malformed copied candidate while the valid world remains open.
    @param reject Whether startup must cleanly refuse before its observer can run.
    @return Actual passing assertions with terminal evidence, or zero for an intended clean full-load refusal.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_dock_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_DOCK_SOURCE=str(source), OTTD_HEAVY_DOCK_PHASE=phase)
    environment.pop("OTTD_HEAVY_DOCK_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_DOCK_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-dock.log"
    with log.open("wb") as sink:
        # Each malformed child owns a short timeout and native termination; a crash, assertion or killed dialog cannot count as clean saved-state admission.
        tag = "[.heavy-dock-service-native]" if phase in PRODUCER else "[.heavy-dock-work-native]"
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=30 if reject or corrupt is not None else 110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode == 1 and not match and "Broken savegame" in evidence and "HEAVY_DOCK native=true" not in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_DOCK native=true phase={phase}" in evidence, log
    if corrupt is not None:
        assert "HEAVY_DOCK preview_reject=true authority_unchanged=true" in evidence, log
    return int(match[1]), evidence


def verify(configuration, phase, parent=None):
    """Bind one actual Dock producer checkpoint to its original paid physical predecessor.

    @param configuration Matching current native configuration.
    @param phase Explicit inspection, declared fuel or real construction boundary.
    @param parent Exact immediate accepted checkpoint, absent only at the immutable recovered entry.
    @return Invocation-owned proof path with actual saved state and native process membership.
    """
    context = current_run_context("space", "heavy-dock-" + phase)
    if parent is None:
        assert PARENTS[phase] is None; predecessor, selected = CORE["entry"](configuration, "prepared")
    else:
        proof = CORE["read"](parent); assert proof["configuration"] == configuration and proof["phase"] == PARENTS[phase]; predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected; assertions, evidence = native(context, configuration, phase, source)
    after = context.saves / f"heavy-dock-{phase}-436.sav"; version, families = chunks(after); assert version == 436 and identity(source) == selected
    if phase in ("inspected", "starting-fuel"):
        before = chunks(source)[1]
        # Ordinary upgrade appends only unknown contained roles. A declared fuel fixture may change only actual Surface stock and its native packet table, preserving every orbital/work/money owner.
        for family in EXACT:
            if phase == "inspected" and family == b"SPST":
                assert before[family][3:] == CORE["LAYOUT"]["legacy_payload"](read_stream(after)[1])
            elif phase == "starting-fuel" and family in (b"CAPA", b"STNN"):
                continue
            else:
                assert before[family] == families[family], (phase, family)
    details = {}
    if phase == "inspected":
        match = re.search(r"HEAVY_DOCK stock steel=(\d+) goods=(\d+) surface_fuel=(\d+) rocket_fuel=(\d+)", evidence); assert match
        details["inspected_stock"] = dict(zip(("steel", "goods", "surface_fuel", "rocket_fuel"), map(int, match.groups())))
    elif phase == "starting-fuel":
        match = re.search(r"HEAVY_DOCK starting_fuel_fixture=true added=(\d+) retained=(\d+) procurement_acceptance=false", evidence); assert match
        details.update(declared_surface_fuel_fixture=True, fuel_added=int(match[1]), fuel_retained=int(match[2]))
    target = context.reports / "heavy-dock.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC; actual original rocket/builder Dock construction. The empty original Storage blueprint is cancelled without refund before a separately paid Dock occupies its released site. Only separately disclosed Surface fuel extends starting content; procurement remains unverified.",
                                     configuration=configuration, phase=phase, save_version=436, predecessor=predecessor, source=selected, after=identity(after), assertions=assertions,
                                     procurement_acceptance=False, native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))], **details), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path, completion=None):
    """Verify ordinary reconstruction and the next original action from that process's own actual resave.

    @param configuration Matching current actual build.
    @param proof_path Exact accepted native checkpoint proof.
    @param completion Exact required child action, absent only at a native leaf.
    @return Ordinary and own-resave proof with complete actual process membership.
    """
    proof = CORE["read"](proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    suffix = "-" + CORE["read"](completion)["phase"] if completion else ""; context = current_run_context("space", "heavy-dock-reload-" + proof["phase"] + suffix)
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; CORE["ordinary"](context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 436; before, saved = chunks(source)[1], chunks(resaved)[1]; owners = [context]
    # Every original material/age/share, bill, utility grant, paid hull and native money owner must survive ordinary reconstruction exactly.
    for family in EXACT:
        assert before[family] == saved[family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC; actual paid Dock reconstruction and each process's own real continuation.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=0)
    if completion is not None:
        accepted = CORE["read"](completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "heavy-dock-resumed-" + proof["phase"] + suffix); result["assertions"], _ = native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'heavy-dock-{accepted["phase"]}-436.sav'; expected = ROOT / accepted["after"]["path"]; assert identity(expected) == accepted["after"]
        before, saved = chunks(expected)[1], chunks(resumed)[1]
        for family in EXACT:
            assert before[family] == saved[family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "heavy-dock-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Execute only explicit small native or own-resave scopes, never selecting a newest or coincidental predecessor."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PARENTS)); parser.add_argument("--parent", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--scope", choices=tuple("native-" + group for group in GROUPS) + tuple("reloads-" + group for group in RELOADS), default="native-producer")
    parser.add_argument("--selector", nargs="+", type=Path); args = parser.parse_args(); assert not args.output.exists(); reports = []; states = {}
    # Explicit current selectors provide actual saved parents only; incomplete smoke runs and another configuration cannot replace required branch membership.
    for selector in args.selector or []:
        selected = CORE["read"](selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            path = ROOT / member; proof = CORE["read"](path); phase = proof["phase"]; assert proof["configuration"] == args.configuration
            if phase != "focused":
                assert proof["save_version"] == 436 and phase not in states; states[phase] = path
    if args.phase is not None:
        reports = [verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    else:
        prefix, group = args.scope.split("-", 1)
        if prefix == "native":
            if group == "producer":
                reports.append(CORE["focused"](args.configuration))
            # Every construction/service stage consumes its own exact predecessor; native elapsed work and actual delivery cannot be replaced with fixture fields.
            for phase in GROUPS[group]:
                parent = PARENTS[phase]; states[phase] = verify(args.configuration, phase, states[parent] if parent else None); reports.append(states[phase]); print(args.configuration, phase, flush=True)
        else:
            assert set(states) == set(PARENTS)
            # Each fork requires a separate ordinary parent and its own next actual action; split scopes keep real process pairs comfortably below two minutes.
            for phase in RELOADS[group]:
                children = [child for child, parent in PARENTS.items() if parent == phase]
                for child in children or [None]:
                    reports.append(reload(args.configuration, states[phase], states[child] if child else None)); print(args.configuration, "reload", phase, child, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC; explicit original paid Dock producer membership.",
                                          configuration=args.configuration, scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
