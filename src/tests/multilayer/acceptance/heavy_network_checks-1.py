"""Execute bounded actual heavy local/server twins and their own ordinary-resave continuations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DC
Dated role/cycle report membership supplies original paid inputs; overwritten historical executables are never requalified.
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

CYCLE = runpy.run_path(str(Path(__file__).with_name("heavy_cycle_checks-1.py")))
EXACT = CYCLE["EXACT"]; PHYSICAL = CYCLE["PHYSICAL"]; read = CYCLE["read"]
INDEXES = {"role": ("heavy-role-runtime", "586029c49f5cfab79a02900b5ccfa87478e905b1ccbe4b9e0ae232cf293f14a7"),
           "cycle": ("heavy-cycle", "587d80b60cf6bbb20b488e7ae3394e10d5a1c65213f2517264aaf25633332432")}
CASES = {"mining": ("role", "recovered"), "service": ("cycle", "paused"), "descent": ("cycle", "surface-mined"), "orbital": ("cycle", "orbital-blocked")}
EXPECTED = {("mining", "initial"): "approach", ("mining", "complete"): "partial", ("service", "initial"): "resumed", ("service", "complete"): "serviced", ("orbital", "complete"): "orbital-returned"}
GROUPS = {"a": ("mining", "service"), "b": ("descent", "orbital"), **{case: (case,) for case in CASES}}
MODES = ("local", "network")


def boundary(configuration, case, phase=None):
    """Select exact original paid report/save membership without reopening its old implementation graph.

    @param configuration Matching original configuration.
    @param case Explicit role/cycle consumer.
    @param phase Optional independently accepted result instead of this case's entry.
    @return Dated index, original native report and exact native save identities.
    """
    family, initial = CASES[case]; name, digest = INDEXES[family]; index = ROOT / ("logs/" + name + "-verification.json"); assert identity(index)["sha256"] == digest
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); selected = []
    # Original native membership, rather than an ordinary or resumed copy, supplies each paid incarnation and its retained work.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("phase") == (phase or initial) and "after" in proof:
            selected.append((member, proof["after"]))
    assert len(selected) == 1; report, saved = selected[0]; assert identity(ROOT / saved["path"]) == saved; return identity(index), report, saved


def compare(left, right, label):
    """Compare every original physical owner and configured dimension byte.

    @param left Exact expected accepted save identity.
    @param right Exact actual native/ordinary/own-resave identity.
    @param label Concrete case/stage for diagnostics.
    """
    assert identity(ROOT / left["path"]) == left and identity(ROOT / right["path"]) == right
    a, b = chunks(ROOT / left["path"])[1], chunks(ROOT / right["path"])[1]
    # Native money, work, packet, approach, finite-body and job owners all converge; an identical displayed hold cannot substitute for this complete comparison.
    for family in EXACT:
        assert a[family] == b[family], (label, family)


def native(context, configuration, case, stage, mode, source):
    """Run one actual native command or actual next-action continuation.

    @param context Invocation-owned runtime/save/log directories.
    @param configuration Matching current test executable.
    @param case Original paid mining, service, descent or orbital consumer.
    @param stage Initial command boundary or complete native continuation.
    @param mode Local immediate admission or actual native server serialization/frame dispatch.
    @param source Exact accepted predecessor path.
    @return Passing assertions and actual saved checkpoint.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_network_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_NETWORK_SOURCE=str(source), OTTD_HEAVY_NETWORK_CASE=case, OTTD_HEAVY_NETWORK_STAGE=stage, OTTD_HEAVY_NETWORK_MODE=mode)
    log = context.logs / "heavy-network.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-network-native]"], cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and match and int(match[2]) == 1 and f"HEAVY_NETWORK native=true case={case} stage={stage} mode={mode}" in evidence, log
    saved = context.saves / f"heavy-network-{case}-{stage}-{mode}-437.sav"; assert chunks(saved)[0] == 437; return int(match[1]), saved


def verify(configuration, case, mode):
    """Capture command admission and its uninterrupted actual next action from the same paid predecessor.

    @param configuration Matching current executable and original paid boundary.
    @param case Explicit original consumer.
    @param mode Local or actual native server twin.
    @return Actual paired command/continuation proof path.
    """
    index, producer, source = boundary(configuration, case); context = current_run_context("space", f"heavy-network-{case}-{mode}")
    assertions, saved = native(context, configuration, case, "initial", mode, ROOT / source["path"])
    continuation = context.for_feature("space", f"heavy-network-complete-{case}-{mode}"); resumed_assertions, completed = native(continuation, configuration, case, "complete", mode, saved)
    assert identity(ROOT / source["path"]) == source; expected = {}
    # Existing independently accepted role/service/return results constrain the new dispatcher wherever commands and opportunities are the same; descent deliberately replaces Earth Unload with actual paid release.
    for stage, actual in (("initial", saved), ("complete", completed)):
        if (case, stage) in EXPECTED:
            _, report, reference = boundary(configuration, case, EXPECTED[case, stage]); compare(reference, identity(actual), (case, stage, mode)); expected[stage] = dict(proof=report, save=reference)
    target = context.reports / "heavy-network.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC; original paid local/native-server command and actual uninterrupted next action, without new assets or work fixture.",
                                     configuration=configuration, case=case, mode=mode, entry_index=index, predecessor=producer, source=source, after=identity(saved), complete=identity(completed), expected=expected,
                                     assertions=assertions + resumed_assertions, procurement_acceptance=False, native_reports=[identity(p) for owner in (context, continuation) for p in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path):
    """Execute the same actual next action from this process's own ordinary resave.

    @param configuration Matching current ordinary/test configuration.
    @param proof_path Exact accepted local/server twin proof.
    @return Ordinary reconstruction and actual own-resave continuation evidence.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; case, mode = proof["case"], proof["mode"]; context = current_run_context("space", f"heavy-network-reload-{case}-{mode}")
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=case + "-" + mode, snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    saved = context.saves / "ordinary-resave.sav"; compare(proof["after"], identity(saved), (case, mode, "ordinary")); continuation = context.for_feature("space", f"heavy-network-resumed-{case}-{mode}")
    assertions, completed = native(continuation, configuration, case, "complete", mode, saved); compare(proof["complete"], identity(completed), (case, mode, "own-next-action"))
    target = context.reports / "heavy-network-reload.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC; original ordinary owners and actual next action from that process's own resave.", configuration=configuration,
                                     case=case, mode=mode, proof=identity(proof_path), source=proof["after"], ordinary=identity(saved), resumed=identity(completed), expected=proof["complete"], assertions=assertions,
                                     native_reports=[identity(p) for owner in (context, continuation) for p in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run bounded native scopes and per-case reload scopes; broad reload names remain only for dated compatibility."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("focused", "native-a", "native-b", "reload-a", "reload-b", "reload-mining", "reload-service", "reload-descent", "reload-orbital"), required=True); parser.add_argument("--selector", nargs="+", type=Path); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); states = {}; reports = []
    # Only explicitly completed native membership supplies either local/server twin; another branch or a resumed copy cannot replace an original initial result.
    for selector in args.selector or []:
        selected = read(selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            proof = read(ROOT / member); key = proof["case"], proof["mode"]; assert key not in states; states[key] = ROOT / member
    if args.scope == "focused":
        reports.append(CYCLE["focused"](args.configuration))
    else:
        operation, group = args.scope.split("-", 1)
        # Each original case has independent local and serialized twins, and each own-resave process must continue its own matching initial checkpoint.
        for case in GROUPS[group]:
            for mode in MODES:
                reports.append(verify(args.configuration, case, mode) if operation == "native" else reload(args.configuration, states[case, mode])); print(args.configuration, args.scope, case, mode, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC; explicit bounded original paid local/server membership.", configuration=args.configuration,
                                          scope=args.scope, complete=True, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
