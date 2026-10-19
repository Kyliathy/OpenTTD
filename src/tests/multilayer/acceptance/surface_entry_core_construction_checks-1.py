"""Build the genuine first Core through original deliveries, native work and activation.

@file surface_entry_core_construction_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CD.
Native typed orders deliver Goods to the real job; no supplied packet, work,
frame or panel enters the continuous original acceptance chain.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_native import execute as native
from surface_gateway_construction_ledger import observe
from surface_gateway_resupply_ledger import observe as resupply
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-3.py"))["trains"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
PHASES = ("construction-query", "construction-build", "construction-service")


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused native build. @param phase Explicit pure/build/service boundary.
    @param receipt Exact selected original predecessor. @param recover_run Successful native invocation to inspect without repeating work.
    @return Exact original material/job/work/frame/module/panel and company evidence, including real intermediate saves.
    """
    assert phase in PHASES
    prefixes = dict(plan="CONSTRUCTION_PLAN", state="CONSTRUCTION_STATE", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED")
    files = [Path(__file__)] + [DIRECTORY / name for name in ("surface_gateway_construction_ledger.py", "surface_gateway_resupply_ledger.py", "surface_gateway_core_ledger.py", "orbital_save_checks-1.py", "surface_gateway_mission_ledger.py", "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py")]
    current = native(configuration, phase, receipt, observe, prefixes, files, recover_run)
    before, output = [ROOT / current[field]["path"] for field in ("before", "output")]
    initial, final, rows = [current[key] for key in ("initial", "final", "rows")]
    first, last = [chunks(path)[1] for path in (before, output)]
    assert RAIL(before) == RAIL(output) and POLICY(ROUTES(before)) == POLICY(ROUTES(output))
    assert first[b"SPFL"] == last[b"SPFL"] and initial["plant"] == final["plant"]
    assert initial["company"]["debt"] == final["company"]["debt"] == 4960000
    assert initial["embodied_inputs"] == final["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    if phase == "construction-query":
        assert first == last and initial == final and not rows["invoices"]
        source = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
        assert json.loads(json.dumps(resupply(before))) == source["final"]
        plan, = rows["plan"]
        assert plan["site"] == 0 and plan["generation"] == 1 and plan["constructor"] == 38 and plan["constructor_generation"] == 2
        assert plan["work"] == 5000 and plan["reserve"] == 150000 and plan["planned"] == 155000 and plan["target"] == 160000 and plan["credit"] == 0
        assert plan["ticks"] == 740 and plan["activation"] == 74 and plan["order_admitted"]
    else:
        ticks = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
        assert sum(row["earned"] for row in rows["earned"]) == ticks
        expense = sum(row["amount"] for row in rows["expenses"])
        assert expense == sum(row["net_expense"] for row in rows["earned"])
        assert all(row["cost"] == 0 for row in rows["invoices"])
        assert initial["company"]["cash"] - final["company"]["cash"] == expense
        assert final["operational"] and final["core"]["row"]["consumed"] == [16, 24]
        assert final["totals"] == {"STEL": 4, "GOOD": 2, "PROP": 12} and final["goods_holds"] == {54: 0, 38: 0}
        # Earlier resupply permission must still refuse delivered/consumed Core
        # work and functionality; only this explicit construction owner admits it.
        try:
            resupply(output)
        except AssertionError:
            pass
        else:
            raise AssertionError("Original resupply contract admitted Core work/activation")
        if phase == "construction-build":
            assert len(rows["invoices"]) == 2 and len(rows["earned"]) == 4 and all(row["reached"] for row in rows["earned"])
            assert 0 < rows["earned"][0]["earned"] < 3000 and [row["earned"] for row in rows["earned"]][1:] == [370, 370, 74]
            references, observed = [], []
            # Each saved native boundary independently proves ownership:
            # delivered inputs survive partial work, disappear exactly at740,
            # and only real activation creates the shared original panel.
            for name in ("ordered", "reserved", "half", "frame", "activating", "operational"):
                path = output.parent / f"surface-gateway-construction-{name}-440.sav"; state = observe(path)
                references.append(dict(name=name, saved=identity(path), observed=state)); observed.append(state)
            assert [state["core"]["row"]["work"] for state in observed] == [0, 0, 370, 740, 740, 740]
            assert [state["core"]["row"]["consumed"] for state in observed] == [[0, 0]] * 3 + [[16, 24]] * 3
            assert [state["core"]["row"]["inputs"][1]["quantity"] for state in observed] == [0, 24, 24, 0, 0, 0]
            assert [state["goods_holds"][38] for state in observed] == [24, 0, 0, 0, 0, 0]
            assert [state["operational"] for state in observed] == [False] * 5 + [True]
            assert observed[4]["core"]["row"]["module"]["activation"] == 74 and all(state["panel"] is None for state in observed[:4])
            assert len(rows["state"]) == 6
            for diagnostic, state in zip(rows["state"], observed, strict=True):
                row = state["core"]["row"]
                assert diagnostic["work"] == row["work"] and diagnostic["consumed_steel"] == row["consumed"][0] and diagnostic["consumed_goods"] == row["consumed"][1]
                assert diagnostic["operational"] == state["operational"]
            current["stages"] = references
        else:
            assert not rows["invoices"] and ticks == 3000 and len(rows["earned"]) == 1 and not rows["earned"][0]["reached"]
            assert final["panel"]["a"] == 30000
    return current


def main():
    """Retain actual original Core construction without overwriting or repeating successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
