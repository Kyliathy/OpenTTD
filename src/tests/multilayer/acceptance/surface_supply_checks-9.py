"""Check actual paid orders across complete native restore, copied preview and custody.

@file surface_supply_checks-9.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCD
The retained starting world is declared ancestry. Only original new finance and
packet deltas prove procurement; driven construction and final factory acceptance remain separate.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, identity, chunks
from artifact_paths import create_run_context
from runner import run_native

FILES = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-5.py")))
COMPONENTS = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-8.py")))
PHYSICAL = (b"CAPA", b"STNN", b"VEHS", b"INDY", b"SPST", b"SVST")


def compare_families(first, second, families):
    """Require exact original native owner bytes at a paused boundary.

    @param first Immutable original input.
    @param second Actual native resave.
    @param families Explicit physical/accounting families whose ownership cannot change at this boundary.
    """
    before, after = chunks(first)[1], chunks(second)[1]
    # Each original native owner proves its own unchanged data; metadata totals
    # cannot substitute for packet, station, company or construction custody.
    for family in families:
        assert before.get(family) == after.get(family), (family, first, second)


def execute(configuration, phase, source):
    """Run exactly one bounded native paid-file operation.

    @param configuration Independently linked focused native application.
    @param phase Explicit paid-prepare, paid-restore or paid-resave operation.
    @param source Exact immutable valid original native input.
    @return Original process, financial markers and independently decoded native outputs.
    """
    context = create_run_context("space", "surface-supply-order-file")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_supply_order_test\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                       OTTD_SURFACE_SUPPLY_SOURCE=str(source), OTTD_SURFACE_SUPPLY_PHASE=phase)
    log = context.logs / "surface-supply-paid-file.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-supply-order-native]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, env=environment, timeout=110)
    terminal = log.read_text(errors="replace")
    marker = re.search(rf"SURFACE_SUPPLY_PAID phase={phase} before=(-?\d+) after=(-?\d+) charged=(\d+) refunded=(\d+)", terminal)
    assert result.returncode == 0 and "All tests passed" in terminal and marker, log
    assert identity(source) == original
    finance = dict(zip(("before", "after", "charged", "refunded"), map(int, marker.groups())))
    outputs = {path.name: dict(file=identity(path), orders=FILES["orders"](path)) for path in sorted(context.saves.glob("*.sav"))}
    if phase == "paid-prepare":
        assert not FILES["orders"](source)["rows"]
        baseline = outputs["surface-supply-paid-baseline-440.sav"]
        pending = outputs["surface-supply-paid-pending-440.sav"]
        rows = pending["orders"]["rows"]
        assert baseline["orders"]["next_id"] == 1 and not baseline["orders"]["rows"]
        assert pending["orders"]["next_id"] == 5 and [row["state"] for row in rows] == [0, 0, 0, 0]
        assert [row["label"] for row in rows] == ["STEL", "RFMT", "ELEC", "BATY"]
        assert [row["quantity"] for row in rows] == [2, 3, 1, 1]
        assert sum(row["cost"] for row in rows) == finance["charged"] > 0
        assert finance["before"] - finance["after"] == finance["charged"] and finance["refunded"] == 0
        compare_families(ROOT / baseline["file"]["path"], ROOT / pending["file"]["path"], PHYSICAL)
    elif phase == "paid-restore":
        before = FILES["orders"](source)
        assert [row["state"] for row in before["rows"]] == [0, 0, 0, 0]
        cancelled = outputs["surface-supply-paid-cancelled-440.sav"]
        fulfilled = outputs["surface-supply-paid-fulfilled-440.sav"]
        assert [row["state"] for row in cancelled["orders"]["rows"]] == [0, 0, 0, 2]
        assert [row["state"] for row in fulfilled["orders"]["rows"]] == [1, 1, 1, 2]
        assert finance["refunded"] == before["rows"][3]["cost"] > 0 and finance["charged"] == 0
        assert finance["after"] - finance["before"] == finance["refunded"]
        # Frozen original commitments survive the real full loader unchanged;
        # only explicit native cancellation/whole custody may change their state/issuance history.
        for old, new in zip(before["rows"], fulfilled["orders"]["rows"]):
            assert {key: value for key, value in old.items() if key not in ("state", "issued_packet")} == {
                key: value for key, value in new.items() if key not in ("state", "issued_packet")}
        compare_families(source, ROOT / cancelled["file"]["path"], PHYSICAL)
        compare_families(ROOT / cancelled["file"]["path"], ROOT / fulfilled["file"]["path"], (b"PLYR",))
    else:
        assert phase == "paid-resave"
        saved, = outputs.values()
        assert saved["orders"] == FILES["orders"](source)
        assert finance["before"] == finance["after"] and finance["charged"] == finance["refunded"] == 0
        compare_families(source, ROOT / saved["file"]["path"], PHYSICAL + (b"PLYR", b"SSOR"))
    proof = dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase,
                 source=original, application=identity(application), terminal=identity(log), finance=finance,
                 outputs=outputs, native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                 sources=[identity(ROOT / name) for name in (*COMPONENTS["SOURCES"], "src/tests/surface_supply_order_native.cpp")]
                         + [identity(Path(__file__)), identity(Path(__file__).with_name("surface_supply_checks-5.py"))],
                 actual_paid_native_continuation=True, driven_delivery_acceptance=False, final_surface_factory_acceptance=False)
    report = context.reports / "surface-supply-paid-file.json"
    report.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Publish a fresh repository-owned membership for one actual paid native file boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("paid-prepare", "paid-restore", "paid-resave"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, output = (ROOT / args.source).resolve(), (ROOT / args.output).resolve()
    assert source.is_relative_to(ROOT) and source.is_file()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(dict(_documentation=__doc__, report=execute(args.configuration, args.phase, source)), indent=2) + "\n", encoding="utf-8")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
