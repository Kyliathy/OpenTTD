"""Verify actual paid empty-module removal and generation-safe ordinary continuation.

@file orbital_removal_checks-1.py Native422 spent receipts and exact paused resave.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group5.
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


def native(context, configuration, action, source, corrupt=None, reject=False):
    """Run the real owner observer with no private deletion, packet or pose injection.

    @param context Explicit isolated native/evidence owner.
    @param configuration Built game/test configuration.
    @param action Actual empty removal or independent malformed preview.
    @param source Immutable identity-verified paid predecessor/checkpoint.
    @param corrupt Optional independent malformed preview file.
    @param reject Require full native loader refusal rather than any test/startup failure.
    @return Actual removed-module checkpoint, or None after verified refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_removal_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_REMOVAL_ACTION=action, OTTD_ORBITAL_REMOVAL_SOURCE=str(source))
    environment.pop("OTTD_ORBITAL_REMOVAL_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_ORBITAL_REMOVAL_CORRUPT"] = str(corrupt)
    log = context.logs / "native-orbital-removal.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-removal]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Only an actual orbital decoder refusal is evidence; a missing tag, assertion failure or startup error must fail the wrapper.
        assert result.returncode != 0 and "Broken savegame" in evidence and "orbital" in evidence.lower() and "All tests passed" not in evidence, log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_REMOVAL native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    return context.saves / "module-removed-422.sav"


def main():
    """Use the frozen preceding Dock proof to bind the exact genuine input and compare every saved owner."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--dock", type=Path, required=True, help="Exact native Dock service report (or its Release421 frozen proof)")
    args = parser.parse_args()
    predecessor = args.dock.resolve()
    assert predecessor.is_relative_to(ROOT)
    previous = json.loads(predecessor.read_text(encoding="utf-8"))
    if "reports" in previous:
        reports = [(ROOT / name, json.loads((ROOT / name).read_text(encoding="utf-8"))) for name in previous["reports"]]
        selected = [(path, report) for path, report in reports if report.get("phase") == "service"]
        assert len(selected) == 1
        path, previous = selected[0]
        assert identity(path)["sha256"] == json.loads(predecessor.read_text(encoding="utf-8"))["sha256"][path.relative_to(ROOT).as_posix()]
    assert previous["phase"] == "service"
    source = ROOT / previous["complete"]["path"]
    original = identity(source)
    assert original == previous["complete"]
    context = current_run_context("space", "module-removal-verify")
    complete = native(context.for_feature("space", "module-removal-prepare"), args.configuration, "remove", source)
    version, expected = chunks(complete)
    assert version == 422
    ordinary_context = context.for_feature("space", "module-removal-ordinary")
    ordinary(ordinary_context, {"name": "module-removed", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    actual = chunks(resaved)[1]
    # Receipt aggregates and the removed-generation queue remain native owners, with no reconstruction of demolished material or inherited work.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "orbital-module-removal.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group5; actual empty removal and generation-safe native ownership.",
        "configuration": args.configuration, "predecessor": identity(predecessor), "source": original,
        "complete": identity(complete), "ordinary_resave": identity(resaved), "save_version": version,
        "exact_owner_families": len(EXACT), "actual_empty_frame_removed": True, "spent_materials_retained": True,
        "occupied_dock_refused": True, "stocked_core_refused": True, "reused_id_has_new_generation": True,
        "old_queue_does_not_retarget_replacement": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native module removal verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
