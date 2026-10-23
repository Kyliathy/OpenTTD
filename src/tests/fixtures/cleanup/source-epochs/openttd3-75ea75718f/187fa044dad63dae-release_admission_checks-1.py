"""Verify current owned cargo-release quotes during real orbital navigation.

@file release_admission_checks-1.py Actual center/foreign overlap, off-center/outside admission and current recovery reservation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group12.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def main():
    """Load only the retained real mined414 cargo/pod owner and quote through normal synchronized movement; preserve all source identities."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-descent-414.sav"
    original = identity(source)
    context = current_run_context("space", "release-admission-verify")
    native = context.for_feature("space", "release-admission-native")
    native.prepare_runtime()
    (native.runtime_dir / "scripts/game_start.scr").write_text("release_admission_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_RELEASE_ADMISSION_SOURCE=str(source))
    log = native.logs / "native-release-admission.log"
    with log.open("wb") as sink:
        result = run_native(native, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.orbital-release-admission]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and "RELEASE_ADMISSION native=true" in evidence, log
    assert "actual_navigation=true" in evidence and "foreign_overlap=true" in evidence and "pure_quotes=true" in evidence, log
    complete = native.saves / "release-admission-423.sav"
    assert chunks(complete)[0] == 423
    paused = context.for_feature("space", "release-admission-ordinary")
    ordinary(paused, {"name": "release-admission", "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Actual ordered position, held cargo/paid shell, incoming reservations and financial/provenance history remain unchanged by ordinary paused loading.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "release-admission.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group12; real owned release admission and normal orbital travel; exact boundary/diagonals use the focused shared geometry contract.",
        "configuration": args.configuration, "source": original, "complete": identity(complete), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "actual_navigation": True, "foreign_overlapping_center_refused": True,
        "off_center_admitted": True, "outside_refused": True, "quotes_preserve_packet_and_financial_owners": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Owned release admission: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
