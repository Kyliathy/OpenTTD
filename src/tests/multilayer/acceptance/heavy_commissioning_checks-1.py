"""Run bounded original paid heavy commissioning and exact legacy-receipt acceptance.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B
The immutable clone input contains an independently delivered unconsumed heavy bill.
Its declared starting Surface content/procurement limitation remains explicit.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, run_native

ENTRY_SHA = "d94f2bd7a327463043762c99bb066b2503325eaf89e799b324bd130f73e26c7b"
SOURCE_SHA = "72f077b4afaabe35d0e446fff55ce879a4047c3ee3105063b0d017504211fc9e"
PHASES = ("upgrade", "queued", "reserved", "partial", "before", "complete", "repeat", "cancel")


def entry(configuration):
    """Select one hash-qualified original accepted stock/factory world.

    @param configuration Matching native producer configuration.
    @return Exact original accepted producer report and its saved traffic checkpoint.
    """
    index = ROOT / "logs/manufacturing-clone-verification.json"; assert identity(index)["sha256"] == ENTRY_SHA
    accepted = next(row for row in json.loads(index.read_text(encoding="utf-8"))["configurations"] if row["configuration"] == configuration)
    candidates = []
    # Actual accepted membership, not a latest filename or already consumed replacement bill, supplies this independently delivered recipe.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = json.loads(path.read_text(encoding="utf-8"))
        if proof.get("variant") == "main" and proof.get("phase") == "depart" and proof.get("model") == 16:
            candidates.append((member, proof["after"]))
    assert len(candidates) == 1
    report, source = candidates[0]; assert source["sha256"] == SOURCE_SHA and identity(ROOT / source["path"]) == source
    return report, source


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Execute one actual bounded native milestone or independent admission refusal.

    @param context Current unique artifact/runtime owner.
    @param configuration Matching current completed game/test configuration.
    @param phase Explicit original native operation.
    @param source Exact valid predecessor, or malformed input for full-load refusal.
    @param corrupt Optional independently malformed copied-preview input.
    @param reject Whether full startup must refuse before its observer runs.
    @return Actual assertion count and physical identity/work marker for successful milestones.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_commission_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_COMMISSION_SOURCE=str(source), OTTD_HEAVY_COMMISSION_PHASE=phase)
    environment.pop("OTTD_HEAVY_COMMISSION_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_COMMISSION_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-commissioning.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-commissioning-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode != 0 and not passed and "Broken savegame" in evidence and "HEAVY_COMMISSION native=true" not in evidence, log
        return 0, {}
    assert result.returncode == 0 and passed and int(passed[2]) == 1, log
    if corrupt is not None:
        assert "HEAVY_COMMISSION preview_reject=true authority_unchanged=true" in evidence, log
        return int(passed[1]), {}
    marker = re.search(r"HEAVY_COMMISSION native=true phase=(\S+) fleet=(\d+) next=(\d+) work=(\d+) serial=(\d+) product=(\d+) generation=(\d+)", evidence)
    assert marker and marker[1] == phase, log
    return int(passed[1]), dict(zip(("fleet", "next_generation", "work", "serial", "product", "product_generation"), map(int, marker.groups()[1:])))


def verify(configuration, phase, parent=None):
    """Bind one actual commissioning stage to its immutable original or real saved predecessor.

    @param configuration Matching completed native configuration.
    @param phase Explicit commissioning milestone.
    @param parent Exact preceding accepted report, absent only for genuine428 upgrade.
    @return Current immutable accepted report path.
    """
    context = current_run_context("space", "heavy-commissioning-" + phase)
    if parent is None:
        assert phase == "upgrade"; predecessor, selected = entry(configuration)
    else:
        proof = json.loads(parent.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
        predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected
    assertions, marker = native(context, configuration, phase, source); after = context.saves / f"heavy-commission-{phase}-431.sav"
    assert chunks(after)[0] == 431 and identity(source) == selected
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; independently paid original factory job, actual native work and unchanged older paid receipts. Inherited declared Surface starting-content fixture remains; procurement acceptance is false.",
                  configuration=configuration, phase=phase, save_version=431, predecessor=predecessor, source=selected, after=identity(after),
                  assertions=assertions, procurement_acceptance=False, **marker,
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    target = context.reports / "heavy-commissioning.json"; assert not target.exists(); target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def focused(configuration):
    """Run only the active commissioning contract and its directly affected old producer boundary.

    @param configuration Matching current completed native test build.
    @return Immutable focused proof with actual process/assertion identities.
    """
    context = current_run_context("space", "heavy-commissioning-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-commissioning],[heavy-mining-tuning],[space-content],[manufacturing-materialization]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace"))
    assert result.returncode == 0 and match, log
    target = context.reports / "heavy-commissioning-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; active commissioning/compatibility and direct original producer contract only.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def execute(configuration, output):
    """Run each bounded stage sequentially and retain explicit completed progress membership.

    @param configuration Matching current completed native configuration.
    @param output New current invocation selector.
    """
    reports = [focused(configuration)]; previous = None; states = {}
    # Every next milestone loads its actual saved predecessor; cancellation independently forks the fully reserved final pre-completion state.
    for phase in PHASES:
        parent = states["before"] if phase == "cancel" else previous
        previous = verify(configuration, phase, parent); states[phase] = previous; reports.append(previous)
        value = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; explicit accepted sequential commissioning membership.",
                     configuration=configuration, scope="native", complete=False, reports=[path.relative_to(ROOT).as_posix() for path in reports])
        output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(configuration, phase, previous.relative_to(ROOT).as_posix(), flush=True)
    value["complete"] = True; output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def main():
    """Select an explicit bounded stage or the complete active sequential commissioning burst."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("focused",) + PHASES)
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.phase:
        result = focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)
        print(result.relative_to(ROOT).as_posix(), flush=True)
    else:
        assert args.output is not None and not args.output.exists(); execute(args.configuration, args.output.resolve())


if __name__ == "__main__":
    main()
