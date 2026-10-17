"""Qualify current linked Surface collection and useful native processor producer boundaries.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group19
Component stock is explicitly isolated; later original paid delivery evidence must supply actual returned packets.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

TAGS = "[surface-collection-contract],[layer-surface-manufacturing]"
AUDIT = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-3.py")))
ENTRY = ROOT / "logs/probe-class-verification.json"
ENTRY_SHA = "058483cef0fa7fcaa6e51186ea1a3c8f70ced6854a5aef419345af88a94b960e"
PARENTS = {"collect": "setup", "deliver": "collect", "own": "deliver"}
INDUSTRY = (b"INDY", b"LPCH", b"LPCR", b"LPCC", b"CAPY")


def read(path):
    """Read explicit original evidence.

    @param path Exact retained selector, proof or dated index.
    @return Parsed evidence without recency selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def compare(left, right):
    """Compare every original physical owner plus native industry stock/history and retained payment ownership.

    @param left Exact original saved checkpoint identity.
    @param right Matching unchanged ordinary checkpoint identity.
    """
    AUDIT["compare"](left, right)
    before, after = chunks(ROOT / left["path"])[1], chunks(ROOT / right["path"])[1]
    # Industries own the useful input buffer and CAPY owns pending settlement; orbital/Surface packets alone cannot prove complete final delivery or unchanged income.
    for family in INDUSTRY:
        assert before[family] == after[family], (family, left, right)


def predecessor(configuration):
    """Resolve the actual earned Nickel mission's original paid Surface recovery.

    @param configuration Matching native build configuration.
    @return Dated index and original twenty-unit paid return identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"]["0/return/recovered"]
    assert identity(ROOT / saved["path"]) == saved
    return identity(ENTRY), saved


def native(configuration, mode, phase, parent):
    """Execute one actual paid Surface construction, pickup, delivery or ordinary resave boundary.

    @param configuration Current native executable build.
    @param mode Actual linked rail or road carrier family.
    @param phase Setup, collected original packets, useful final delivery, or unchanged own resave.
    @param parent Exact same-mode immediately preceding native selector.
    @return Complete original process proof identity.
    """
    dated, source = predecessor(configuration)
    original_parent = None
    if parent:
        selected = read(parent)
        assert selected["complete"] and selected["configuration"] == configuration
        original_parent = selected["report"]
        proof = read(ROOT / original_parent["path"])
        assert proof["mode"] == mode and proof["phase"] == PARENTS[phase]
        source = proof["after"]
        assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "surface-return-" + mode + "-" + phase)
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_return_delivery_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_RETURN_SOURCE=str(ROOT / source["path"]),
               OTTD_SURFACE_RETURN_MODE=mode, OTTD_SURFACE_RETURN_PHASE=phase)
    log = context.logs / "surface-return.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.surface-return-delivery-native]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    # A real native command/tick result must accompany the exact selected physical delivery marker, rather than an early application exit.
    assert result.returncode == 0 and match, log
    witness = re.search(rf"SURFACE_RETURN native=true mode={mode} phase={phase} carrier=(\d+) factory=(\d+) added_stock=false", terminal)
    assert witness, log
    after = identity(context.saves / f"surface-return-{mode}-{phase}-438.sav")
    assert chunks(ROOT / after["path"])[0] == 438
    if phase == "own":
        compare(source, after)
    report = context.reports / "surface-return.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope="native", mode=mode, phase=phase,
                                     dated_index=dated, source=source, parent=original_parent, after=after, carrier=int(witness[1]), factory=int(witness[2]),
                                     assertions=int(match[1]), cases=int(match[2]), added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def focused(configuration):
    """Execute the current native membership/custody and actual factory recipe producer boundary.

    @param configuration Current native test executable configuration.
    @return Original retained focused process proof identity, without claiming paid procurement or physical delivery.
    """
    context = create_run_context("space", "probe-surface-producers")
    context.prepare_runtime()
    log = context.logs / "surface-producers.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), TAGS], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    # Complete native assertions must accompany process success; fixture teardown alone cannot publish a delivered producer boundary.
    assert result.returncode == 0 and match, log
    report = context.reports / "surface-producers.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope="focused",
                                     assertions=int(match[1]), cases=int(match[2]), terminal=identity(log), paid_delivery_claim=False,
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Run one independently bounded producer burst or original paid Surface delivery segment."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--scope", choices=("focused", "native"), default="focused")
    parser.add_argument("--mode", choices=("rail", "road"))
    parser.add_argument("--phase", choices=("setup", *PARENTS))
    parser.add_argument("--parent", type=Path)
    args = parser.parse_args()
    assert not args.output.exists()
    if args.scope == "focused":
        assert args.mode is None and args.phase is None and args.parent is None
        report = focused(args.configuration)
    else:
        assert args.mode is not None and args.phase is not None
        assert (args.parent is not None) == (args.phase != "setup")
        report = native(args.configuration, args.mode, args.phase, args.parent)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope,
                                         mode=args.mode, phase=args.phase, report=report), indent=2) + "\n", encoding="utf-8")
    print(f"passed {args.configuration} Surface {args.scope} {args.mode or ''} {args.phase or ''}", flush=True)


if __name__ == "__main__":
    main()
