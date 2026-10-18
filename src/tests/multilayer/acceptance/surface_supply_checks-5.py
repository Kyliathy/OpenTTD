"""Exercise one isolated actual native order-file restore, resave or refusal.

@file surface_supply_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC
Persistence fixtures declare order metadata without supply payments or packets.
Every native child has a110second limit; this wrapper never runs a full regression.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess

from space_preview_checks import ROOT, identity, read_stream, chunks
from artifact_paths import create_run_context
from runner import run_native
from launch_center_checks import riff_location


def orders(path):
    """Decode original SSOR bytes independently of the native producer/reader.

    @param path Actual version440 native file. @return Original header and all fixed receipt fields.
    """
    version, raw = read_stream(path)
    assert version == 440
    _, start, length, end = riff_location(raw, b"SSOR")
    next_id, saved_day, count = struct.unpack_from("<QiH", raw, start)
    assert count <= 16384 and length == 14 + count * 53
    rows = []
    # Walk the independent fixed layout, preserving every historical scalar.
    # These quantities are commitments/receipts and never a second stock ledger.
    for index in range(count):
        values = struct.unpack_from("<QBBHHH4sHqQiiHBI", raw, start + 14 + index * 53)
        names = ("id", "purchaser", "owner", "origin", "x", "y", "label", "quantity", "cost",
                 "inflation", "year", "day", "packet_buyer", "state", "issued_packet")
        row = dict(zip(names, values))
        row["label"] = row["label"].decode("ascii")
        rows.append(row)
    assert start + 14 + count * 53 == end
    return dict(next_id=next_id, saved_day=saved_day, rows=rows)


def execute(configuration, phase, source, bad=None):
    """Run one real full-loader entry and its copied/resave boundary in isolation.

    @param configuration Current independently linked native test application.
    @param phase Explicit bounded prepare, inspect, inspect-empty or refusal operation.
    @param source Immutable original valid input, retained across this invocation.
    @param bad Optional separately declared malformed current file, never an older-header forgery.
    @return Original native process, file and independently decoded receipt evidence.
    """
    context = create_run_context("space", "surface-supply-order-file")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_supply_order_test\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                       OTTD_SURFACE_SUPPLY_SOURCE=str(source), OTTD_SURFACE_SUPPLY_PHASE=phase)
    if bad is not None:
        environment["OTTD_SURFACE_SUPPLY_BAD"] = str(bad)
    log = context.logs / "surface-supply-order-file.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-supply-order-native]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, env=environment, timeout=110)
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and f"SURFACE_SUPPLY_ORDER_FILE phase={phase}" in terminal, log
    assert identity(source) == original
    outputs = {path.name: dict(file=identity(path), orders=orders(path)) for path in sorted(context.saves.glob("*.sav"))}
    if phase == "prepare":
        assert read_stream(source)[0] < 440
        empty = outputs["surface-supply-empty-upgrade-440.sav"]["orders"]
        assert empty["next_id"] == 1 and not empty["rows"]
        before = chunks(source)[1]
        after = chunks(ROOT / outputs["surface-supply-empty-upgrade-440.sav"]["file"]["path"])[1]
        # Genuine old absence grants no imported stock, hull, facility or money;
        # compare the original native owners before adding declared order metadata.
        for family in (b"CAPA", b"STNN", b"VEHS", b"INDY", b"PLYR", b"SPST", b"SVST"):
            assert before.get(family) == after.get(family), (family, source)
        book = outputs["surface-supply-order-fixture-440.sav"]["orders"]
        assert book["next_id"] == 5 and [row["state"] for row in book["rows"]] == [0, 1, 2, 3]
    elif phase in ("inspect", "inspect-empty"):
        saved, = outputs.values()
        assert saved["orders"] == orders(source)
        before, after = chunks(source)[1], chunks(ROOT / saved["file"]["path"])[1]
        # A paused metadata resave must retain original physical stock, vehicles,
        # facilities and company finances. No previous gameplay graph is replayed.
        owners = (b"CAPA", b"STNN", b"VEHS", b"INDY", b"PLYR", b"SPST", b"SVST", b"SSOR")
        for family in owners:
            assert before.get(family) == after.get(family), (family, source)
    proof = dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase,
                 source=original, malformed=identity(bad) if bad is not None else None,
                 application=identity(application), terminal=identity(log), outputs=outputs,
                 native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                 sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_supply_order_native.cpp")],
                 declared_metadata_fixture=True, runtime_procurement_acceptance=False)
    target = context.reports / "surface-supply-order-file.json"
    target.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    return identity(target)


def main():
    """Execute one explicit native file boundary and publish its immutable report member."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("prepare", "inspect", "inspect-empty", "preview-refusal", "full-refusal"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--bad", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, args.phase, (ROOT / args.source).resolve(), (ROOT / args.bad).resolve() if args.bad else None)
    target.write_text(json.dumps(dict(_documentation=__doc__, report=result), indent=2) + "\n", encoding="utf-8")
    print(identity(target), flush=True)


if __name__ == "__main__":
    main()
