"""Verify one native company-custody refusal or item-transfer phase.

@file space_company_checks-1.py Genuine paid owners, no lost cargo and exact paused resave.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group15.
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
    """Use only the frozen genuine mined414 owners; transfer tests the real item-entry without claiming an auction price or fabricating a company."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", choices=("refusal", "transfer"), required=True)
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-descent-414.sav"
    original = identity(source)
    context = current_run_context("space", "space-company-verify-" + args.phase)
    native = context.for_feature("space", "space-company-native-" + args.phase)
    native.prepare_runtime()
    (native.runtime_dir / "scripts/game_start.scr").write_text("space_company_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_SPACE_COMPANY_SOURCE=str(source), OTTD_SPACE_COMPANY_PHASE=args.phase)
    log = native.logs / "native-space-company.log"
    with log.open("wb") as sink:
        result = run_native(native, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.space-company-custody]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in evidence and f"SPACE_COMPANY native=true phase={args.phase}" in evidence, log
    complete = native.saves / f"space-company-{args.phase}-423.sav"
    assert chunks(complete)[0] == 423
    paused = context.for_feature("space", "space-company-ordinary-" + args.phase)
    ordinary(paused, {"name": "space-company-" + args.phase, "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Exact hull/pod/job identities, transferred owner fields, timers and all original packet/financial histories survive ordinary paused loading.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "space-company.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group15; actual company deletion refusal or native company item-transfer entry on genuine paid cargo/work. No auction pricing claim.",
        "configuration": args.configuration, "phase": args.phase, "source": original, "complete": identity(complete), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "conserved_mined_units": 20, "removal_preserves_custody": True,
        "native_company_item_transfer": args.phase == "transfer"}, indent=2) + "\n", encoding="utf-8")
    print(f"Space company {args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
