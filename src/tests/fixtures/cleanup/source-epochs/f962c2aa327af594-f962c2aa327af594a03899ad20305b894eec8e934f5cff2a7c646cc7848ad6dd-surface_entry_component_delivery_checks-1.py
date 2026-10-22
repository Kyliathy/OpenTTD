"""Run paid original component delivery without redefining acquired quantities.

@file surface_entry_component_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CG.
Native orders, road movement and reverse handling retain actual packet owners.
Original useful freight income remains independent of this transferred batch.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe
from surface_gateway_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = ["surface_gateway_component_delivery_ledger.py", "surface_gateway_components_ledger.py", "surface_gateway_construction_ledger.py",
         "surface_gateway_resupply_ledger.py", "surface_gateway_core_ledger.py", "orbital_save_checks-1.py", "surface_gateway_mission_ledger.py",
         "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py"]


def execute(configuration, phase, receipt, recover_run=None, record_native_abandonment=False):
    """@param configuration Actual independently linked native build. @param phase Explicit pure admission, native delivery/handling or short service.
    @param receipt Exact selected original predecessor. @param recover_run Successful native invocation to inspect without repeating paid work.
    @param record_native_abandonment Explicit first transport boundary exposing the known native eightSteel/twoGoods ordinary-offer loss.
    @return Actual original native payments, sole cargo owners and fixed acquired36/50conservation.
    """
    assert phase in ("component-delivery-query", "component-delivery-dispatch", "component-delivery-work", "component-delivery-handling-query",
                     "component-delivery-handling", "component-delivery-handling-work", "component-delivery-service")
    prefixes = dict(route="COMPONENT_DELIVERY_ROUTE", capacities="COMPONENT_DELIVERY_CAPACITY", carriers="COMPONENT_DELIVERY_CARRIER",
                    plan="COMPONENT_DELIVERY_PLAN", handling="COMPONENT_DELIVERY_HANDLING", settled="COMPONENT_DELIVERY_SETTLED",
                    progress="COMPONENT_DELIVERY_PROGRESS", funding="COMPONENT_FUND", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", invoices="INVOICE")
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))["operation"]["final"]
    before_loss = predecessor.get("cargo_abandoned", {"STEL": 0, "GOOD": 0, "PROP": 0})
    assert before_loss in ({"STEL": 0, "GOOD": 0, "PROP": 0}, {"STEL": 8, "GOOD": 2, "PROP": 0})
    assert not record_native_abandonment or phase == "component-delivery-work"
    after_abandonment = record_native_abandonment or before_loss["STEL"] == 8

    def inspect(path):
        """@param path Hash-qualified before/output save from this exact native phase. @return Explicit pre-loss or already-retained material contract for that actual boundary."""
        before = path.name == "surface-gateway-before-component-delivery-work-440.sav"
        # Only the explicitly selected first transport boundary may change
        # the loss receipt. Later phases preserve those already lost units;
        # no missing packet is restored or reclassified as usable supply.
        allow = before_loss["STEL"] == 8 if record_native_abandonment and before else after_abandonment
        return observe(path, allow_abandonment=allow)

    authorities = [ROOT / name for name in ("src/station_cmd.cpp", "src/cargopacket.cpp", "src/source_type.h", "src/surface_handling.cpp")]
    current = native(configuration, phase, receipt, inspect, prefixes, [Path(__file__)] + [DIRECTORY / name for name in FILES] + authorities, recover_run)
    initial, final, rows = current["initial"], current["final"], current["rows"]
    for label, acquired_physical in {"STEL": 40, "GOOD": 52, "PROP": 12}.items():
        assert initial["totals"][label] + initial["cargo_abandoned"][label] == final["totals"][label] + final["cargo_abandoned"][label] == acquired_physical
    assert initial["additional_collected"] == final["additional_collected"] == {"STEL": 36, "GOOD": 50, "PROP": 0}
    assert initial["flight"] == final["flight"] and initial["orbital_consumed"] == final["orbital_consumed"]
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    assert final["company"]["cash"] >= 100061 and not any(row["credit"] for row in rows["funding"])
    if record_native_abandonment:
        assert initial["cargo_abandoned"] == {"STEL": 0, "GOOD": 0, "PROP": 0}
        assert final["cargo_abandoned"] == {"STEL": 8, "GOOD": 2, "PROP": 0} and final["component_carriers_returned"]
        assert final["delivery_stocks"][0] == {"STEL": 28, "GOOD": 48, "PROP": 0}
        current["native_abandonment"] = dict(_documentation="Actual before/output physical deficit inferred from the sole Industry-source transport custody and native TruncateAbandonedOffers. CompanySupply protection does not apply; these lost units are unavailable. No persisted loss counter is claimed.",
                                             before=current["before"], output=current["output"], lost=final["cargo_abandoned"], authority=current["sources"][-4:])
    # Both geometry and stock-backed handling queries must preserve every
    # family, not merely the selected aggregate quantities or printed quotes.
    if phase in ("component-delivery-query", "component-delivery-handling-query"):
        assert chunks(ROOT / current["before"]["path"])[1] == chunks(ROOT / current["output"]["path"])[1]
        assert initial == final and not rows["invoices"]
        if phase == "component-delivery-query":
            route, = rows["route"]; plan, = rows["plan"]
            assert route["member"] == 0 and route["depot"] == 2443 and route["crossings"] == route["infrastructure"] == 0
            assert len(rows["carriers"]) == 5 and len(rows["capacities"]) == 2
            assert (plan["center"], plan["generation"], plan["member"], plan["member_generation"]) == (21, 1, 0, 1)
            assert plan["handling_fee"] == 61 and plan["handling_ticks"] == 11 and plan["working_reserve"] == 100000 and plan["target"] == 118061
        else:
            assert initial["components_at_member"] and len(rows["handling"]) == 2
    # Commands reserve or route real packets but have zero direct price;
    # handling fees belong to later actual throughput, never this reservation.
    elif phase in ("component-delivery-dispatch", "component-delivery-handling"):
        assert initial["company"] == final["company"] and rows["invoices"] and all(row["cost"] == 0 for row in rows["invoices"])
        if phase == "component-delivery-handling":
            assert initial["components_at_member"] and len(final["handling"]["jobs"]) == 2
            assert final["delivery_stocks"][0] == {"STEL": 0, "GOOD": 0, "PROP": 0}
    else:
        earned, = rows["earned"]; assert 0 < earned["earned"] <= 3000
        assert earned["cash_before"] == initial["company"]["cash"] and earned["cash"] == final["company"]["cash"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert earned["cash"] == earned["cash_before"] - earned["net_expense"]
        if phase == "component-delivery-handling-work":
            settled, = rows["settled"]; assert settled["stored"] and settled["actual_other"] == settled["expected_fee"] == 52
            assert settled["mass"] == 832 and settled["earned"] == 10 and final["components_at_center"]
        if phase == "component-delivery-service":
            assert initial["components_at_center"] and final["components_at_center"] and earned["earned"] == 3000
    return current


def main():
    """Publish one immutable native delivery boundary from the selected original world."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--record-native-abandonment", action="store_true"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run, args.record_native_abandonment)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
