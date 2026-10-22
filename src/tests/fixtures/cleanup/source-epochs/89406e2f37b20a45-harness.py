"""Drive original rail protection, external chemicals and useful Propellant custody.

@file surface_entry_propellant_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.2.
All work/movement comes from native ticks; original commands own every order and
empty carrier purchase. Existing protected Steel/Goods remain their same packets.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, identity
from surface_propellant_ledger import observe, reserve_identity
from artifact_paths import create_run_context, RunContext, identifier
from runner import run_native

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
SOURCES = {"traffic-order": ("build",), "traffic-work": ("traffic-order", "traffic-work"),
           "chemical-order": ("traffic-work",), "chemical-work": ("chemical-order", "chemical-work"),
           "process": ("chemical-work", "process"), "carrier": ("chemical-work", "process"),
           "collect-order": ("carrier", "collect-work"), "collect-work": ("collect-order", "collect-work"),
           "resume": ("collect-work",)}


def execute(configuration, receipt, phase, recover_run=None):
    """@param configuration Actual focused native application. @param receipt Immediate selected original predecessor.
    @param phase Bounded native order, work, purchase or resumption boundary.
    @param recover_run Optional exact successful invocation needing only receipt finalization.
    @return Original command/payment/packet owners and real material conversion/custody.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration and selected["phase"] in SOURCES[phase]
    foundation_path = ROOT / "logs/surface-entry-propellant-construction-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_empty_factory_ready"]
    original = selected["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    initial_source = observe(source)
    if phase == "traffic-order":
        expected, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        assert original == expected and initial_source["chemical_feed"]["held"] == 21
    if phase in ("chemical-order", "chemical-work", "process", "carrier", "collect-order", "collect-work", "resume"):
        # Useful chemical/fuel movement may begin only after every actual
        # crossing owner has parked; a successful order quote is insufficient.
        assert initial_source["traffic_halted"]
    if phase in ("process", "carrier"):
        assert initial_source["chemical_feed"]["held"] == 0 and initial_source["chemical_feed"]["depot"]
    if phase == "carrier":
        assert initial_source["plant"]["produced"] == 20 and initial_source["propellant_truck"] is None
    if phase == "collect-order":
        assert initial_source["propellant_truck"]["depot"] and initial_source["propellant_truck"]["stopped"]
        assert initial_source["propellant_truck"]["held"] < 20
    if phase == "resume":
        assert initial_source["propellant_truck"]["depot"] and initial_source["propellant_truck"]["stopped"]
        assert initial_source["propellant_truck"]["held"] == 20
    context = (RunContext(ROOT, "space", "surface-entry-propellant-" + phase, identifier(recover_run)) if recover_run
               else create_run_context("space", "surface-entry-propellant-" + phase))
    retained = context.directory / "harness.py"
    if not recover_run:
        context.prepare_runtime(); retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_propellant\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / (phase + ".log")
    if recover_run:
        # Exact successful input/output artifacts authorize receipt repair;
        # no paid command or elapsed work may be replayed to improve a result.
        report, = list(context.reports.glob("native-*.json"))
        completed = json.loads(report.read_text(encoding="utf-8"))
        assert completed["exit_status"] == 0 and completed["configuration"] == configuration and original in completed["inputs"]
        for reference in completed["inputs"] + completed["artifacts"]:
            assert identity(ROOT / reference["path"]) == reference
        exit_status = completed["exit_status"]
    else:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-propellant]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_PROPELLANT_PHASE=phase))
        exit_status = result.returncode
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert exit_status == 0 and "All tests passed" in terminal and "SURFACE_PROPELLANT_COMPLETE phase=" + phase in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-propellant-{name}-440.sav" for name in ("before-" + phase, phase)]
    initial, final = observe(before), observe(output)
    assert initial_source == initial and reserve_identity(initial) == reserve_identity(final)
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    prefixes = dict(invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", park_orders="PARK_ORDER",
                    park_states="PARK_STATE", road_orders="ROAD_ORDER", carrier="CARRIER", resumed="RESUMED", state="STATE")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_PROPELLANT_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    state, = rows["state"]
    plant_fields = ("industry", "chemicals", "consumed", "produced", "buffered", "dispatched", "batches")
    assert {key: state[key] for key in plant_fields} == {key: final["plant"][key] for key in plant_fields}
    if phase in ("traffic-work", "chemical-work", "process", "collect-work"):
        earned, = rows["earned"]
        assert 0 <= earned["earned"] <= 3000 and not rows["invoices"]
        assert earned["cash_before"] == initial["company"]["cash"] and earned["cash"] == final["company"]["cash"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == earned["cash_before"] - earned["cash"]
        assert earned["income"] == -sum(row["amount"] for row in rows["expenses"] if row["type"] in (7, 8, 9, 10))
        if phase == "traffic-work":
            assert earned["reached"] == final["traffic_halted"]
        elif phase == "process":
            assert earned["reached"] == (final["plant"]["produced"] == 20)
        elif phase == "collect-work":
            prop = final["propellant_truck"]
            assert earned["reached"] == (prop["held"] == 20 and prop["depot"] and prop["stopped"] and prop["tile"] == 2443)
    else:
        assert not rows["earned"] and not rows["expenses"]
        assert initial["company"]["cash"] - final["company"]["cash"] == sum(row["cost"] for row in rows["invoices"])
        if phase == "carrier":
            carrier, = rows["carrier"]
            assert carrier["empty"] and carrier["invoice"] == 6234 and carrier["capacity"] == 27
            assert final["propellant_truck"]["held"] == 0 and final["propellant_truck"]["depot"]
        else:
            assert all(row["cost"] == 0 for row in rows["invoices"])
    if phase in ("chemical-order", "chemical-work", "process", "carrier", "collect-order", "collect-work"):
        assert final["traffic_halted"]
    if phase == "resume":
        assert len(rows["resumed"]) == 3 and final["propellant_truck"]["held"] == 20
    return dict(receipt=identity(receipt), foundation=identity(foundation_path), source=original, before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), initial=initial, final=final, rows=rows, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_propellant_ledger.py")),
                         identity(ROOT / "src/tests/surface_entry_propellant.cpp"), identity(ROOT / "src/tests/surface_entry_work.h")])


def main():
    """Retain one physical native boundary and resume only from its actual first output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--phase", choices=tuple(SOURCES), required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, output = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, receipt, args.phase, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=output.relative_to(ROOT).as_posix(), phase=args.phase, company=operation["final"]["company"],
                          traffic_halted=operation["final"]["traffic_halted"], plant=operation["final"]["plant"],
                          collected=operation["final"]["propellant_truck"]["held"] if operation["final"]["propellant_truck"] else 0)), flush=True)


if __name__ == "__main__":
    main()
