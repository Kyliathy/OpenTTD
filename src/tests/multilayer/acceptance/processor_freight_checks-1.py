"""Execute bounded general-hauler freight contracts and exact paid native checkpoint continuations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
The dated factory producer supplies a real paid hauler and inherited stock; no new gameplay fixture is installed.
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

ENTRY = ROOT / "logs/manufacturing-purchase-verification.json"
ENTRY_SHA = "13e9ddaa67992395af5c647f06a86ed8ffa080c0d5a5b5b59bd56555a793f6ce"
TAGS = "[processor-freight],[manufacturing-replacement]"
PARENTS = {"prepared": "inspected", "loaded": "prepared", "travelling": "loaded", "transferred": "travelling", "returned": "transferred"}
PHYSICAL = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR",
            b"STNN", b"LCTR", b"LCHB", b"LHND", b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"TUPH", b"TUPD", b"TUVH", b"TUVD", b"ENGN", b"EIDS", b"ERNW", b"GRPS")


def read(path):
    """Read one explicit original evidence file.

    @param path Exact dated index, semantic report or selector path.
    @return Parsed unchanged evidence, without latest-file selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration):
    """Resolve the original paid17 factory output without reopening historical binaries or source graphs.

    @param configuration Matching original producer build.
    @return Exact original index, semantic report and paid native save identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    report, = [r for r in selected["reports"] if "manufacturing-purchase-verify-complete-17/" in r["path"]]
    assert identity(ROOT / report["path"]) == report
    proof = read(ROOT / report["path"]); source = proof["after"]
    assert proof["configuration"] == configuration and proof["model"] == 17 and proof["phase"] == "complete"
    assert identity(ROOT / source["path"]) == source and chunks(ROOT / source["path"])[0] == 427
    return identity(ENTRY), report, source


def execute(configuration, scope, phase, parent, candidate=None):
    """Run one bounded isolated component child or actual paid-world checkpoint.

    @param configuration Exact current native test build.
    @param scope Focused component fixtures, actual native continuation or unchanged own-resave.
    @param phase Exact checkpoint whose own source or predecessor must be admitted.
    @param parent Explicit original semantic report; absent only for focused or initial native inspection.
    @param candidate Optional invocation-owned malformed438 copy for independent copied/full refusal.
    @return Invocation-owned qualified semantic report path.
    """
    dated, producer, source = predecessor(configuration); parent_reference = None
    if parent:
        previous = read(parent)
        assert previous["configuration"] == configuration and previous["complete"]
        if scope in ("native", "resave"):
            assert previous["phase"] == (phase if scope == "resave" else PARENTS[phase])
        source = previous["after"]; assert identity(ROOT / source["path"]) == source; parent_reference = identity(parent)
    valid_source = source; refusal = scope == "full-refusal"
    assert (candidate is not None) == (scope in ("preview", "full-refusal"))
    if refusal:
        source = identity(candidate)
    context = create_run_context("space", "processor-freight-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    tag = TAGS if scope == "focused" else "[.processor-freight-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("processor_freight_test\n", encoding="utf-8")
        env.update(OTTD_PROCESSOR_FREIGHT_SOURCE=str(ROOT / source["path"]), OTTD_PROCESSOR_FREIGHT_PHASE=phase, OTTD_PROCESSOR_FREIGHT_RESAVE="1" if scope == "resave" else "0")
        if scope == "preview":
            env["OTTD_PROCESSOR_FREIGHT_CORRUPT"] = str(candidate)
            # The native preview reads the original CORRUPT argument. Publish that identical actual input under the recorder's generic SOURCE suffix too, so its immutable process report binds both files without editing old evidence.
            env["OTTD_PROCESSOR_FREIGHT_CANDIDATE_SOURCE"] = str(candidate)
    log = context.logs / "processor-freight.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    if refusal:
        assert result.returncode == 1 and "Broken savegame" in terminal and "Fatal test error" not in terminal and "Assertion failed" not in terminal, log
    else:
        assert result.returncode == 0 and matched, log
    after = None
    if scope == "preview":
        assert int(matched[2]) == 1 and "PROCESSOR_FREIGHT native=true preview_rejected=true authority_unchanged=true" in terminal
    elif scope in ("native", "resave"):
        assert int(matched[2]) == 1 and f"PROCESSOR_FREIGHT native=true phase={phase} added_stock=false resave=" + ("true" if scope == "resave" else "false") in terminal
        saved = context.saves / ("freight-" + phase + "-438.sav"); assert chunks(saved)[0] == 438; after = identity(saved)
        if scope == "resave":
            before, rewritten = chunks(ROOT / source["path"])[1], chunks(saved)[1]
            # Ordinary own-resave must preserve every original physical owner and configured metadata, independently of printed totals or a test success marker.
            for family in (*PHYSICAL, b"CDIM", b"SPRC"):
                assert before[family] == rewritten[family], (phase, family)
    target = context.reports / "processor-freight.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, configuration=configuration, scope=scope, phase=phase, complete=True,
                                     dated_index=dated, producer=producer, parent=parent_reference, source=source if scope != "focused" else None,
                                     after=after, assertions=int(matched[1]) if matched else 0, cases=int(matched[2]) if matched else 0, added_stock=False,
                                     valid_source=valid_source if candidate else None, candidate=identity(candidate) if candidate else None, expected_full_refusal=refusal,
                                     inherited_starting_content_procurement=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Execute exact current freight membership with exclusive evidence and no repeated predecessor work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("focused", "native", "resave", "preview", "full-refusal"), required=True)
    parser.add_argument("--phase", choices=("inspected", *PARENTS), default="inspected")
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists()
    assert (args.parent is not None) == (args.scope in ("resave", "preview", "full-refusal") or (args.scope == "native" and args.phase in PARENTS))
    report = execute(args.configuration, args.scope, args.phase, args.parent.resolve() if args.parent else None, args.candidate.resolve() if args.candidate else None)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, complete=True, reports=[report.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
