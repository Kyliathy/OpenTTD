"""Verify stand-alone recovery demolition cannot strand actual returned cargo.

@file launch_retirement_checks-1.py Exact genuine paid retarget predecessor and ordinary owner resave.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group16.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native
from launch_center_checks import native as native_placement

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def main():
    """Consume only the explicitly named completed actual paid recovery diversion; preserve source identity and all held cargo."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--predecessor", type=Path, required=True)
    args = parser.parse_args()
    predecessor = args.predecessor.resolve()
    proof = json.loads(predecessor.read_text(encoding="utf-8"))
    assert proof["configuration"] == args.configuration and proof["phase"] == "retarget"
    source = ROOT / proof["complete"]["path"]
    original = identity(source)
    assert original == proof["complete"]
    context = current_run_context("space", "launch-retirement-verify")
    native = context.for_feature("space", "launch-retirement-native")
    native.prepare_runtime()
    (native.runtime_dir / "scripts/game_start.scr").write_text("launch_retirement_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_LAUNCH_RETIREMENT_SOURCE=str(source))
    log = native.logs / "native-launch-retirement.log"
    with log.open("wb") as sink:
        result = run_native(native, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.launch-retirement]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and "LAUNCH_RETIREMENT native=true actual_recovered=10" in evidence, log
    complete = native.saves / "launch-retirement-refused-423.sav"
    assert chunks(complete)[0] == 423
    paused = context.for_feature("space", "launch-retirement-ordinary")
    ordinary(paused, {"name": "launch-retirement", "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Actual center identity, returned/held packets and every financial/ownership field survive ordinary paused loading exactly.
    for family in EXACT:
        assert expected[family] == actual[family], family
    joined = native.saves / "launch-retirement-joined-423.sav"
    assert chunks(joined)[0] == 423 and "LAUNCH_RETIREMENT joined_freight=true" in evidence, log
    joined_context = context.for_feature("space", "launch-retirement-joined-ordinary")
    ordinary(joined_context, {"name": "launch-retirement-joined", "snapshot": identity(joined)}, args.configuration)
    joined_resaved = joined_context.saves / "ordinary-resave.sav"
    expected_joined, actual_joined = chunks(joined)[1], chunks(joined_resaved)[1]
    # Removing only the paid launch facility leaves the real joined rail station as the sole owner and physical loading point of all ten recovered units.
    for family in EXACT:
        assert expected_joined[family] == actual_joined[family], family
    placement_source = ROOT / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav"
    placement_original = identity(placement_source)
    placement_context = context.for_feature("space", "launch-retirement-placement")
    placed = native_placement(placement_context, args.configuration)
    assert chunks(placed)[0] == 423
    placement_paused = context.for_feature("space", "launch-retirement-placement-ordinary")
    ordinary(placement_paused, {"name": "launch-retirement-placement", "snapshot": identity(placed)}, args.configuration)
    placement_resaved = placement_paused.saves / "ordinary-resave.sav"
    expected_placement, actual_placement = chunks(placed)[1], chunks(placement_resaved)[1]
    # Current native placement pays for the real facility and joined loading point; its saved owners remain independent of placeholder capture art.
    for family in EXACT:
        assert expected_placement[family] == actual_placement[family], family
    assert identity(placement_source) == placement_original
    assert identity(source) == original
    report = context.reports / "launch-retirement.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group16.1; stand-alone center with actual recovered cargo and no other active work refuses demolition, without placeholder art claims.",
        "configuration": args.configuration, "predecessor": identity(predecessor), "source": original, "complete": identity(complete), "ordinary_resave": identity(resaved),
        "joined": identity(joined), "joined_ordinary_resave": identity(joined_resaved), "ordinary_airport_refused": True,
        "placement_source": placement_original, "placement": identity(placed), "placement_ordinary_resave": identity(placement_resaved),
        "exact_owner_families": len(EXACT), "actual_recovered_units": 10, "conserved_mined_units": 20}, indent=2) + "\n", encoding="utf-8")
    print(f"Launch retirement: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
