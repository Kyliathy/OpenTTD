"""Query and purchase only the genuine native launch-site water prerequisite.

@file surface_entry_launch_site_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEC.
Original commands and company books own every payment and town-rating change.
Read-only recovery can finalize a successful native save without repurchasing it.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import TABLE, observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
FUEL_IDENTITY = runpy.run_path(str(Path(__file__).with_name("surface_entry_propellant_checks-8.py")))["fuel_identity"]


def execute(configuration, phase, receipt=None, recover_run=None):
    """@param configuration Actual focused native build. @param phase Pure quote or admitted original civic work.
    @param receipt Immediate selected predecessor, or canonical completed fuel input.
    @param recover_run Existing successful native invocation to inspect without executing again.
    @return Immutable actual payment, physical ownership and native-process evidence.
    """
    foundation_path = ROOT / "logs/surface-entry-passenger-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    if receipt is None:
        entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
        original = entry["output"]
    else:
        predecessor = json.loads(receipt.read_text(encoding="utf-8"))
        assert predecessor["complete"] and predecessor["configuration"] == configuration
        original = predecessor["operation"]["output"]
    source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-launch-site-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_launch\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-launch]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_LAUNCH_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_LAUNCH_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-launch-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key != b"GLOG")
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == final[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith(prefix + " ")]
            for name, prefix in dict(ground="SURFACE_LAUNCH_GROUND", paid="SURFACE_LAUNCH_GROUND_PAID", invoices="SURFACE_PASSENGER_INVOICE").items()}
    initial, observed = observe(before), observe(output)
    ground, = rows["ground"]
    assert ground["reserve"] == 50000 and ground["cash"] - ground["preparation"] >= 50000 and not ground["center_admitted"]
    assert initial["company"]["cash"] == ground["cash"]
    assert {k: v for k, v in initial.items() if k != "company"} == {k: v for k, v in observed.items() if k != "company"}
    differences = [key.decode("ascii") for key in first if first[key] != final[key]]
    width = 4 if ground["rotation"] % 2 == 0 else 3; height = 3 if width == 4 else 4
    x, y = ground["anchor"] % 64, ground["anchor"] // 64
    cells = [(y + dy) * 64 + x + dx for dy in range(height) for dx in range(width)]
    heights = first[b"MAPH"][3:]; types = first[b"MAPT"][3:]
    assert all(types[index] >> 4 in (0, 4) for index in cells)
    assert len({heights[(y + dy) * 64 + x + dx] for dy in range(height + 1) for dx in range(width + 1)}) == 1
    canal = ground["canal"]; assert canal not in cells and any(abs(canal % 64 - index % 64) + abs(canal // 64 - index // 64) == 1 for index in cells)
    assert first[b"MAPH"] == final[b"MAPH"]
    if phase == "ground-query":
        assert first == final and initial == observed and not rows["paid"] and not rows["invoices"]
    else:
        paid, = rows["paid"]; invoice, = rows["invoices"]
        assert invoice["command"] == "CmdBuildCanal" and invoice["tile"] == canal
        assert paid["cost"] == invoice["cost"] == ground["preparation"] == initial["company"]["cash"] - observed["company"]["cash"]
        assert paid["center"] == ground["center_bound"] >= 500000 and not paid["center_built"]
        assert paid["anchor"] == ground["anchor"] and paid["canal"] == ground["canal"] and paid["member"] == ground["member"]
        assert initial["company"]["debt"] == observed["company"]["debt"] == 5000000
        assert observed["company"]["cash"] >= 50000
        assert [index for index, (old, new) in enumerate(zip(types, final[b"MAPT"][3:])) if old != new] == [canal]
        assert final[b"MAPT"][3 + canal] >> 4 == 6
        assert final[b"MAPO"][3 + canal] & 31 == 0 and (final[b"MAPO"][3 + canal] >> 5) & 3 == 1
        first_date, final_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]
        rng_fields = {"random_state[0]", "random_state[1]"}
        assert {k: v for k, v in first_date.items() if k not in rng_fields} == {k: v for k, v in final_date.items() if k not in rng_fields}
        state, other = [first_date[f"random_state[{index}]"] for index in (0, 1)]; mixed = other ^ 0x1234567F
        # Original MakeCanal receives exactly one native Random result. Check
        # the pure copied transition, preserving every other DATE field.
        expected = [(state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF]
        assert [final_date[f"random_state[{index}]"] for index in (0, 1)] == expected
        assert set(differences) <= {"MAPT", "MAPO", "MAP2", "M3LO", "M3HI", "MAP5", "MAPE", "MAP7", "MAP8", "DATE", "CITY", "PLYR"}
        assert all(first[key] == final[key] for key in (b"CAPA", b"STNN", b"VEHS", b"LPCR", b"LPCC"))
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(foundation=identity(foundation_path), receipt=identity(receipt) if receipt else None, source=original,
                before=identity(before), output=identity(output), application=identity(application), terminal=identity(log), rows=rows,
                initial=initial, final=observed, changed_families=differences, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_launch.cpp"),
                identity(ROOT / "src/tests/surface_passenger_layout.h"), identity(ROOT / "src/tests/multilayer/acceptance/surface_propellant_ledger.py")])


def main():
    """Run or recover one explicitly selected boundary and retain its exact original output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("ground-query", "ground"), required=True)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve() if args.receipt else None, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
