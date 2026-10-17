"""Execute actual processor-product return checkpoints from the exact accepted paid batch world.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12B
Original finite inputs, paid shells, reusable flights and native handling alone supply this continuation.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

ENTRY = ROOT / "logs/orbital-processor-verification.json"
ENTRY_SHA = "45ddf32a5b13afccea72e1af6753cfe87cfcea5119d351ba063f9ee875406747"
PHASES = ("inspected", "ore-delivered", "steel-stock", "steel-supplied", "goods-supplied", "products-loaded", "historical-cleared", "released", "recovered", "handling", "collected")
PARENTS = dict(zip(PHASES[1:], PHASES))
TAGS = "[surface-collection-contract],[space-logistics-contract]"
PHYSICAL = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR",
            b"STNN", b"LCTR", b"LCHB", b"LHND", b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"TUPH", b"TUPD", b"TUVH", b"TUVD", b"ENGN", b"EIDS", b"ERNW", b"GRPS")


def read(path):
    """Read one explicit unchanged retained evidence file.

    @param path Exact original semantic report, index or bounded selector.
    @return Parsed evidence, without choosing a newest directory.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration):
    """Resolve the actual original paid processing output and its exact producer membership.

    @param configuration Matching accepted batch producer configuration.
    @return Original immutable index, native ice-batch report and actual438 save identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"]["native/ice-batch"]; assert identity(ROOT / saved["path"]) == saved
    candidates = [r for r in selected["reports"] if Path(r["path"]).name == "orbital-processor-ice-batch.json"]
    found = []
    # Own-resaves and resumed actions have the same phase basename. Only exact native-operation and after-save membership identify the original accepted output, never historical executable/source hashes.
    for reference in candidates:
        assert identity(ROOT / reference["path"]) == reference; proof = read(ROOT / reference["path"])
        if proof["operation"] == "native" and proof["after"] == saved:
            assert proof["configuration"] == configuration and proof["complete"]; found.append(reference)
    report, = found
    assert chunks(ROOT / saved["path"])[0] == 438
    return identity(ENTRY), report, saved


def execute(configuration, scope, phase, parent):
    """Run one bounded component child, original native action or unchanged own-resave.

    @param configuration Current native test executable build.
    @param scope Focused producer boundary, native continuation or resave operation.
    @param phase Exact ordered paid input, flight, release or collection checkpoint.
    @param parent Exact original same-phase or preceding semantic report; absent only for focused/initial inspection.
    @return Exclusive semantic proof path binding actual process, source, terminal and ordinary output.
    """
    dated, producer, source = predecessor(configuration); parent_reference = None
    if parent:
        previous = read(parent); assert previous["configuration"] == configuration and previous["complete"]
        assert previous["phase"] == (phase if scope == "resave" else PARENTS[phase])
        source = previous["after"]; assert identity(ROOT / source["path"]) == source; parent_reference = identity(parent)
    context = create_run_context("space", "processor-returns-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    tag = TAGS if scope == "focused" else "[.processor-returns-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("processor_returns_test\n", encoding="utf-8")
        env.update(OTTD_PROCESSOR_RETURNS_SOURCE=str(ROOT / source["path"]), OTTD_PROCESSOR_RETURNS_PHASE=phase, OTTD_PROCESSOR_RETURNS_RESAVE="1" if scope == "resave" else "0")
    log = context.logs / "processor-returns.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log
    after = None
    if scope != "focused":
        assert int(match[2]) == 1 and f"PROCESSOR_RETURNS native=true phase={phase} added_stock=false resave=" + ("true" if scope == "resave" else "false") in terminal
        saved = context.saves / ("products-" + phase + "-438.sav"); version, families = chunks(saved); assert version == 438 and b"SPRC" in families; after = identity(saved)
        if scope == "resave":
            original = chunks(ROOT / source["path"])[1]
            # Every original physical owner and configured processing receipt must survive unchanged. Printed counters cannot substitute for actual ordinary file preservation.
            for family in (*PHYSICAL, b"CDIM", b"SPRC"):
                assert original[family] == families[family], (phase, family)
    target = context.reports / "processor-returns.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, configuration=configuration, scope=scope, phase=phase, complete=True,
                                     dated_index=dated, producer=producer, parent=parent_reference, source=source if scope != "focused" else None,
                                     after=after, assertions=int(match[1]), cases=int(match[2]), added_stock=False, inherited_starting_content_procurement=False,
                                     terminal=identity(log), native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Keep each requested actual checkpoint independently bounded with exclusive source and output identities."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("focused", "native", "resave"), required=True)
    parser.add_argument("--phase", choices=PHASES, default="inspected")
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists()
    assert (args.parent is not None) == (args.scope == "resave" or (args.scope == "native" and args.phase in PARENTS))
    report = execute(args.configuration, args.scope, args.phase, args.parent.resolve() if args.parent else None)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, complete=True, reports=[report.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
