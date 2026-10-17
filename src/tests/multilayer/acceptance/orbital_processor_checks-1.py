"""Execute paid orbital processor, congestion and ordinary-file acceptance.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group11
Only an exact accepted original Core checkpoint supplies starting physical owners; no replacement stock, date or pose is installed.
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

ENTRY = ROOT / "logs/orbital-core-verification.json"
ENTRY_SHA = "d901a9cb257411e01a8093a68e7b33a34dfa7fd3f09330efa30a1383051964e6"
PARENTS = {"steel": "inspected", "steel-frame": "steel", "steel-active": "steel-frame", "miner-deployed": "steel-active",
           "ore-delivered": "miner-deployed", "steel-partial": "ore-delivered", "steel-batch": "steel-partial", "steel-stock": "steel-batch",
           "goods-frame": "steel-stock", "goods-active": "goods-frame", "rare-delivered": "goods-active",
           "goods-partial": "rare-delivered", "goods-batch": "goods-partial", "refinery-ore": "goods-batch",
           "refinery-stock": "refinery-ore", "refinery-frame": "refinery-stock", "refinery-active": "refinery-frame", "refinery-nore": "refinery-active",
           "refinery-partial": "refinery-nore", "refinery-metal": "refinery-partial", "refinery-retarget": "refinery-partial",
           "ice-delivered": "refinery-metal", "ice-partial": "ice-delivered", "ice-batch": "ice-partial",
           "steel-outage": "steel-partial", "steel-restored": "steel-outage", "steel-cancelled": "steel-partial", "steel-restarted": "steel-cancelled"}
PHYSICAL = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF",
            b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND",
            b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"TUPH", b"TUPD", b"TUVH", b"TUVD", b"ENGN", b"EIDS", b"ERNW", b"GRPS")


def predecessor(configuration):
    """Resolve the exact original paid Core, without reopening its historical binary/source graph.

    @param configuration Matching accepted original configuration.
    @return Exact original index, producer report and paid native save identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    entry = json.loads(ENTRY.read_text(encoding="utf-8"))
    reference = entry["configurations"][configuration]["reports"][0]
    assert identity(ROOT / reference["path"]) == reference
    report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
    saved = report["completed"]
    assert report["configuration"] == configuration and identity(ROOT / saved["path"]) == saved
    assert chunks(ROOT / saved["path"])[0] == 416
    return identity(ENTRY), reference, saved


def inspect(configuration, phase, parent=None, resave=False):
    """Run one actual ordered native checkpoint or its unchanged own resave with exclusive evidence.

    @param configuration Exact current native test executable configuration.
    @param phase Exact ordered inspection, paid delivery, construction or activation checkpoint.
    @param parent Exact preceding semantic proof; absent only for inspection of the original accepted Core.
    @param resave Whether to admit and rewrite that exact same checkpoint without repeating its original controls or work.
    @return Semantic proof path retaining original source, actual process/terminal and ordinary resave.
    """
    index, report, source = predecessor(configuration)
    parent_reference = None
    if parent is not None:
        parent = parent.resolve()
        previous = json.loads(parent.read_text(encoding="utf-8"))
        expected = phase if resave else PARENTS[phase]
        assert previous["configuration"] == configuration and previous["phase"] == expected and previous["complete"]
        source = previous["after"]
        assert identity(ROOT / source["path"]) == source
        parent_reference = identity(parent)
    context = create_run_context("space", "orbital-processor-native")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_processor_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_ORBITAL_PROCESSOR_SOURCE=str(ROOT / source["path"]), OTTD_ORBITAL_PROCESSOR_PHASE=phase,
               OTTD_ORBITAL_PROCESSOR_RESAVE="1" if resave else "0")
    log = context.logs / "orbital-processor.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-processor-native]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match and int(match[2]) == 1 and "ORBITAL_PROCESSOR native=true phase=" + phase + " added_stock=false" in terminal, log
    assert "resave=" + ("true" if resave else "false") in terminal, log
    saved = context.saves / ("processor-" + phase + "-438.sav")
    version, families = chunks(saved)
    assert version == 438 and b"SPRC" in families
    if resave:
        before = chunks(ROOT / source["path"])[1]
        # Every original physical owner and configured processing receipt must survive an actual own resave, independently of native printed totals or success markers.
        for family in (*PHYSICAL, b"CDIM", b"SPRC"):
            assert family in before and family in families and before[family] == families[family], (phase, family)
    target = context.reports / ("orbital-processor-" + phase + ".json")
    target.write_text(json.dumps(dict(_documentation=__doc__, configuration=configuration, phase=phase, operation="resave" if resave else "native", complete=True,
                                     dated_index=index, producer=report, parent=parent_reference, source=source, after=identity(saved), assertions=int(match[1]),
                                     added_stock=False, procurement_acceptance=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Execute one exact paid native boundary in a bounded burst with explicit predecessor membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("inspected", *PARENTS), default="inspected")
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--resave", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert (args.parent is not None) == (args.resave or args.phase in PARENTS)
    report = inspect(args.configuration, args.phase, args.parent, args.resave)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, complete=True,
                                          reports=[report.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
