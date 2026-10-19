"""Drive the first genuine paid deployment through independently saved native phases.

@file surface_entry_first_mission_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15C.
Only native BeginLaunchFlight and synchronized work mutate the actual world.
Original paid identities, conventional services and sole cargo owners persist.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_native import execute as native
from surface_gateway_mission_ledger import observe
from surface_gateway_payload_ledger import observe as ground
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-3.py"))["trains"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
PHASES = ("mission-query", "mission-begin", "mission-ascent", "mission-orbit", "mission-deploy", "mission-recover", "mission-service")
EXPECTED = {"mission-begin": 1, "mission-ascent": 2, "mission-orbit": 3, "mission-deploy": 4, "mission-recover": 0, "mission-service": 0}


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused native build. @param phase Explicit pure/departure/flight/service boundary.
    @param receipt Exact selected original predecessor. @param recover_run Successful native run to inspect without repeating work.
    @return Independently reconciled native phase, paid hull, packet, fuel and company evidence.
    """
    assert phase in PHASES
    prefixes = dict(budget="ASSEMBLY_BUDGET", plan="FLIGHT_PLAN", state="MISSION_STATE", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED")
    files = [Path(__file__)] + [DIRECTORY / name for name in ("surface_gateway_mission_ledger.py", "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py")]
    current = native(configuration, phase, receipt, observe, prefixes, files, recover_run)
    before, output = [ROOT / current[field]["path"] for field in ("before", "output")]
    initial, final, rows = [current[key] for key in ("initial", "final", "rows")]
    first, last = [chunks(path)[1] for path in (before, output)]
    assert RAIL(before) == RAIL(output) and POLICY(ROUTES(before)) == POLICY(ROUTES(output))
    assert first[b"SPST"] == last[b"SPST"] and initial["plant"] == final["plant"]
    assert initial["embodied_inputs"] == final["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    assert initial["company"]["debt"] == final["company"]["debt"] == 4960000
    if phase == "mission-query":
        assert first == last and initial == final and not rows["invoices"]
        budget, = rows["budget"]; plan, = rows["plan"]
        assert budget["planned"] == 868000 and budget["target"] == 870000 and budget["credit"] == 0
        assert plan["target"] == 21 and plan["target_generation"] == 1 and plan["fee"] == 5000 and not plan["begun"]
        source = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
        assert json.loads(json.dumps(ground(before))) == source["final"]
    else:
        state, = rows["state"]; flight = final["flight"]
        assert state["phase"] == flight["phase"] == EXPECTED[phase]
        assert state["fuel"] + state["burned"] == 30 and state["burned"] == final["fuel_burned"]
        assert state["started"] == 1 and state["completed"] == flight["completed"] and state["canceled"] == 0
        assert state["constructor"] == 38 and state["constructor_generation"] == 2 and state["steel"] == 16 and not state["core"]
        # Original ground permission must still refuse every actual flight
        # or deployed constructor; explicit mission custody is indispensable.
        try:
            ground(output)
        except AssertionError:
            pass
        else:
            raise AssertionError("Ground contract admitted a real flight/deployed hull")
        if phase == "mission-begin":
            assert first[b"DATE"] == last[b"DATE"] and initial["totals"] == final["totals"]
            assert len(rows["invoices"]) == 1 and rows["invoices"][0]["cost"] == 5000
            assert initial["company"]["cash"] - final["company"]["cash"] == 5000 and final["fuel_burned"] == 0
            assert flight["remaining"] == 148
        else:
            earned, = rows["earned"]; ticks = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
            assert earned["earned"] == ticks and not rows["invoices"]
            assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
            if phase == "mission-service":
                assert ticks == 3000 and not earned["reached"]
            else:
                assert 0 < ticks < 3000 and earned["reached"]
    return current


def main():
    """Retain one actual original mission phase without overwriting or repeating successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
