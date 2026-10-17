"""Run bounded actual heavy descent, sender-loss custody and genuine433 recovery.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB
All cargo, shells and hulls come from immutable paid predecessors. Sender loss explicitly destroys its native hull and onboard parts; procurement acceptance remains false.
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
    "supply": ("logs/heavy-supply-custody-verification.json", "02160a11c6f8ce663eefd1e78cedb32799155a559900243cf0bef72d8f2724d6", "complete"),
    "legacy": ("logs/legacy-descent-verification.json", "dc10eb579476c65df77cf7c52cde4b40876c1d3c80f38dea6269060174c095e5", "partial"),
}
PARENTS = {"returning": None, "partial": "returning", "recovered": "partial", "sender-lost": "partial", "orphan-recovered": "sender-lost", "legacy-upgrade": None, "legacy-recovered": "legacy-upgrade"}


def read(path):
    """Read one exact retained proof.

    @param path Repository-owned JSON path.
    @return Parsed original proof.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def entry(configuration, kind):
    """Select immutable matching producer membership without requalifying overwritten producer executables.

    @param configuration Matching original build configuration.
    @param kind Heavy supply or genuine older descent producer.
    @return Exact dated report and its original save identity.
    """
    name, expected, phase = ENTRIES[kind]; index = ROOT / name; assert identity(index)["sha256"] == expected
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Only exact accepted report/save membership supplies the original hull and physical job; a matching name or newly relabelled format cannot substitute it.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("phase") == phase and proof.get("save_version") == 433:
            selected.append((member, proof["after"]))
    assert len(selected) == 1 and identity(ROOT / selected[0][1]["path"]) == selected[0][1]
    return selected[0]


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Execute one bounded original-world operation or independent malformed reader.

    @param context Explicit invocation-owned runtime, logs and artifacts.
    @param configuration Current matching test/game build.
    @param phase Actual milestone or copied-preview check.
    @param source Exact original predecessor or malformed full-load input.
    @param corrupt Optional malformed copied candidate while the original remains open.
    @param reject Whether full startup must refuse before invoking the observer; a30-second subprocess timeout also terminates a stuck crash dialog owned by this test.
    @return Successful assertion count; intended full refusal returns zero.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_descent_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_DESCENT_SOURCE=str(source), OTTD_HEAVY_DESCENT_PHASE=phase)
    environment.pop("OTTD_HEAVY_DESCENT_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_DESCENT_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-descent.log"
    with log.open("wb") as sink:
        # A malformed startup must reject promptly; subprocess.run kills and waits for only this owned child on timeout, so a crash dialog cannot wait indefinitely for user interaction.
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-descent-native]"], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=30 if reject else 110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        # A clean rejected startup returns the expected failed-observer status. A crash, assertion or signal must never count as corrupt-file admission coverage, even if a diagnostic preceded it.
        assert result.returncode == 1 and not match and "Broken savegame" in evidence and "HEAVY_DESCENT native=true" not in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_DESCENT native=true phase={phase}" in evidence, log
    if corrupt is not None:
        assert "HEAVY_DESCENT preview_reject=true authority_unchanged=true" in evidence, log
    return int(match[1])


def focused(configuration):
    """Check immutable new receipts and both small/heavy isolated full/lost recovery boundaries.

    @param configuration Matching completed build.
    @return Current focused proof path.
    """
    context = current_run_context("space", "heavy-descent-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[heavy-descent-contract],[orbital-release-geometry],[descent-custody-contract]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / "heavy-descent-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; focused original geometry, immutable role/mass and declared small/heavy receiver boundaries.",
                                     configuration=configuration, phase="focused", assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def normalize_legacy(payload):
    """Remove only explicitly absent new receipt fields from a genuine old job's current serialization.

    @param payload Actual434 SPDJ bytes including its ordinary RIFF length prefix.
    @return Exactly comparable old433 SPDJ bytes, preserving every original scalar/reference.
    """
    # chunks() retains the three low RIFF length bytes after its mode; recover that exact payload before removing five new provenance bytes per original row.
    assert int.from_bytes(payload[:3], "big") == len(payload) - 3; raw = payload[3:]; count = int.from_bytes(raw[8:10], "big"); result = bytearray(raw[:10]); position = 10
    for _ in range(count):
        assert raw[position + 56:position + 61] == b"\xff\xff\0\0\0"; packets = raw[position + 61]
        result += raw[position:position + 56] + raw[position + 61:position + 62 + 6 * packets]; position += 62 + 6 * packets
    assert position == len(raw); return len(result).to_bytes(3, "big") + result


def verify(configuration, phase, parent=None):
    """Bind one actual milestone to its exact paid predecessor and preserve genuine legacy normalization.

    @param configuration Matching actual native build.
    @param phase Explicit heavy or old-small operation.
    @param parent Exact current predecessor proof, absent only at frozen producer entry.
    @return New immutable native proof path.
    """
    context = current_run_context("space", "heavy-descent-" + phase)
    if parent is None:
        assert PARENTS[phase] is None; predecessor, selected = entry(configuration, "legacy" if phase == "legacy-upgrade" else "supply")
    else:
        proof = read(parent); assert proof["configuration"] == configuration and proof["phase"] == PARENTS[phase]; predecessor, selected = identity(parent), proof["after"]
    source = ROOT / selected["path"]; assert identity(source) == selected
    assertions = native(context, configuration, phase, source); after = context.saves / f"heavy-descent-{phase}-434.sav"; assert chunks(after)[0] == 434 and identity(source) == selected
    if phase == "legacy-upgrade":
        assert chunks(source)[0] == 433
        # Upgrade grants no work/cargo/new role: remove only the explicit unknown/zero append and compare all physical families against the genuine433 producer.
        for family in EXACT:
            left, right = chunks(source)[1][family], chunks(after)[1][family]
            assert left == (normalize_legacy(right) if family == b"SPDJ" else right), ("legacy-upgrade", family)
    target = context.reports / "heavy-descent.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; original paid heavy40t descent or genuine433 small20t continuation. Sender loss is an explicit native destruction fixture including destroyed onboard parts; inherited Surface starting stock remains and procurement acceptance is false.",
                                     configuration=configuration, phase=phase, save_version=434, predecessor=predecessor, source=selected, after=identity(after), assertions=assertions,
                                     procurement_acceptance=False, sender_destruction_fixture=phase == "sender-lost",
                                     native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path, completion=None):
    """Compare ordinary reconstruction and the next actual action from that process's own resave.

    @param configuration Matching current build.
    @param proof_path Exact original checkpoint report.
    @param completion Optional accepted next operation, including the separate original-sender-loss branch.
    @return Immutable ordinary/continuation proof path.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    suffix = "-" + read(completion)["phase"] if completion else ""; context = current_run_context("space", "heavy-descent-reload-" + proof["phase"] + suffix)
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=proof["phase"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = context.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 434; owners = [context]
    # Whole native families, including packets/fees/engines/finite reserves, must remain exact rather than merely matching an observer's cached digest.
    for family in EXACT:
        assert chunks(source)[1][family] == chunks(resaved)[1][family], (proof["phase"], family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; exact ordinary physical reconstruction and own-resave next action.",
                  configuration=configuration, phase=proof["phase"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT), assertions=0)
    if completion is not None:
        accepted = read(completion); assert accepted["configuration"] == configuration and accepted["predecessor"] == identity(proof_path)
        resumed_context = context.for_feature("space", "heavy-descent-resumed-" + proof["phase"] + suffix); result["assertions"] = native(resumed_context, configuration, accepted["phase"], resaved)
        resumed = resumed_context.saves / f'heavy-descent-{accepted["phase"]}-434.sav'; expected = ROOT / accepted["after"]["path"]; assert identity(expected) == accepted["after"]
        for family in EXACT:
            assert chunks(expected)[1][family] == chunks(resumed)[1][family], ("resumed", proof["phase"], family)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed)); owners.append(resumed_context)
    result["native_reports"] = [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]
    target = context.reports / "heavy-descent-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Select explicit bounded native stages or their ordinary/resumed owner checks."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("native", "reloads"), default="native"); parser.add_argument("--selector", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("focused",) + tuple(PARENTS)); parser.add_argument("--parent", type=Path)
    args = parser.parse_args(); assert not args.output.exists(); reports = []
    if args.phase is not None:
        reports = [focused(args.configuration) if args.phase == "focused" else verify(args.configuration, args.phase, args.parent.resolve() if args.parent else None)]
    elif args.scope == "native":
        reports = [focused(args.configuration)]; states = {}
        # Each branch consumes its own exact immediate predecessor; the actual old producer and heavy sender-loss branch remain independently identified.
        for phase, parent in PARENTS.items():
            states[phase] = verify(args.configuration, phase, states[parent] if parent else None); reports.append(states[phase]); print(args.configuration, phase, flush=True)
    else:
        assert args.selector is not None; selected = read(args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
        states = {read(ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}; assert set(states) == {"focused"} | set(PARENTS)
        for phase in PARENTS:
            children = [child for child, parent in PARENTS.items() if parent == phase]
            for child in children or [None]:
                reports.append(reload(args.configuration, states[phase], states[child] if child else None)); print(args.configuration, "reload", phase, child, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; explicit bounded current native membership.",
                                          configuration=args.configuration, scope=args.scope, complete=args.phase is None, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
