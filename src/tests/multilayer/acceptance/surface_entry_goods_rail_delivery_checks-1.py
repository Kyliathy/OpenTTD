"""Follow actual first13Goods on original49through acceptance and whole native unloading payment.

@file surface_entry_goods_rail_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZH.
Original packet identity, native train movement and actual CargoPayment own
the result. A whole unload fare is never attributed solely to this new cohort.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks,identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
TRANSFER = runpy.run_path(str(DIRECTORY/"surface_entry_goods_rail_transfer_checks-1.py"))
observe = TRANSFER["observe"]
PHASES = dict(work="staged-chemical-capital-goods-rail-delivery",payment="staged-chemical-capital-goods-rail-payment")
TRAIN = "train[0].common[0]."


def rail_stock(path):
    """Read only real first13Goods across the original whole49consist.

    @param path Genuine original native checkpoint.
    @return Actual rail cohort quantity/share and sole packet references across native split/merge.
    """
    packets,vehicles = TABLE(path,b"CAPA"),TABLE(path,b"VEHS")
    cohort = []
    # Waiting cargo and old first12Goods own no part of the boarded new
    # cohort. Actual native references, not reused packet numbers, identify
    # the original whole consist's current physical first13inventory.
    for id in [49]+list(range(26,38)):
        for ref in vehicles[id][TRAIN+"cargo.packets"]:
            packet = packets[ref-1]
            if packet["source"] != 13:
                continue
            assert packet["source_type"] == 0 and packet["source_id"] in (5,11)
            cohort.append(dict(vehicle=id,packet=ref-1,**packet))
    assert len({p["packet"] for p in cohort}) == len(cohort)
    return dict(quantity=sum(p["count"] for p in cohort),share=sum(p["feeder_share"] for p in cohort),packets=cohort)


def execute(configuration,receipt,recover_run=None):
    """Earn one freshly funded transport or explicitly qualified pending-payment interval.

    @param configuration Independently linked focused native application.
    @param receipt Exact first complete road-transfer endpoint or this consumer's prior actual interval.
    @param recover_run Successful invocation to inspect without repeating earned work.
    @return Real rail boarding/acceptance, whole payment ownership and measured company cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] in (TRANSFER["PHASE"],*PHASES.values())
    anchor = receipt if previous["phase"] == TRANSFER["PHASE"] else ROOT/previous["operation"]["transfer_receipt"]["path"]
    root = json.loads(anchor.read_text(encoding="utf-8"))
    assert root["complete"] and root["configuration"] == configuration and root["phase"] == TRANSFER["PHASE"]
    assert root["operation"]["measured_cash_flow"]["first_transfer_complete"] and root["operation"]["actual_feeder_cargo"]["quantity"] > 0
    accepted_before = 0
    if previous["phase"] in PHASES.values():
        assert identity(anchor) == previous["operation"]["transfer_receipt"]
        assert not previous["operation"]["work_refused"] and not previous["operation"]["rail_payment_posted"]
        accepted_before = previous["operation"]["cumulative_rail_accepted"]
    assert previous["operation"]["final"]["company"]["cash"] < 100000
    source = ROOT/previous["operation"]["output"]["path"]
    assert json.loads(json.dumps(observe(source))) == previous["operation"]["final"]
    pending_before = [r for r in TABLE(source,b"CAPY").values() if r["front"] == 50]
    initial = rail_stock(source)
    # Only a preceding real accepted-cohort receipt can qualify a pending
    # payment-only continuation. An unrelated old first12fare cannot end the
    # new-cohort transport consumer or provide cash before native settlement.
    settlement = bool(accepted_before > 0 and initial["quantity"] == 0 and pending_before and pending_before[0]["route_profit"] > 0)
    phase = PHASES["payment"] if settlement else PHASES["work"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),ROOT/"src/tests/surface_entry_goods_rail_delivery.h"]
    operation = native(configuration,phase,receipt,observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",selection="GOODS_RAIL_DELIVERY_SELECTION",
             service="GOODS_RAIL_DELIVERY_SERVICE",earned="EARNED",expenses="EXPENSE"),
        files,recover_run=recover_run,chemical_limit=2000)
    operation.update(transfer_receipt=identity(anchor),banked_sale=root["operation"]["banked_sale"])
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert initial == rail_stock(before)
    final = rail_stock(output)
    operation["actual_rail_cargo"] = final
    selected, = rows["selection"]
    candidates = rows["admission"]
    assert selected["ceiling"] == 2000 and selected["settlement"] == settlement and selected["initial"] == initial["quantity"]
    assert all(r["ticks"] <= 2000 and r["unpaid"] == 0 and r["target"] == r["operating"]+40000 and r["future_income_counted"] == 0 for r in candidates)
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    if not selected["found"]:
        assert not any(r["funded"] for r in candidates) and not rows["service"] and not rows["earned"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        operation.update(work_refused=True,rail_payment_posted=False,cumulative_rail_accepted=accepted_before)
        return phase,operation
    assert candidates[-1]["funded"] and not any(r["funded"] for r in candidates[:-1])
    assert candidates[-1]["ticks"] == selected["ticks"] and candidates[-1]["target"] <= first["company"]["cash"]
    service, = rows["service"]
    earned, = rows["earned"]
    assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= 2000
    assert service["initial"] == initial["quantity"] and service["aboard"] == final["quantity"] and service["share"] == final["share"]
    assert service["initial"]+service["boarded"] == service["accepted"]+service["aboard"]
    assert service["settlement"] == settlement and not service["cohort_only_fare_claimed"] and not service["feeder_credit_is_cash"]
    pending = [r for r in TABLE(output,b"CAPY").values() if r["front"] == 50]
    assert service["payment_pending"] == bool(pending)
    cumulative = accepted_before+service["accepted"]
    if service["payment_posted"]:
        assert cumulative > 0 and service["observed_whole_unload_fare"] > 0 and not pending and final["quantity"] == 0
        assert TABLE(output,b"VEHS")[49][TRAIN+"last_station_visited"] == 11
    assert earned["reached"] == (service["payment_posted"] or last["company"]["cash"] >= 100000)
    assert earned["reached"] or elapsed == selected["ticks"]
    a,b = [TABLE(p,b"VEHS")[49] for p in (before,output)]
    profit = b[TRAIN+"profit_this_year"]-a[TRAIN+"profit_this_year"]
    if a[TRAIN+"profit_last_year"] != b[TRAIN+"profit_last_year"]:
        profit += b[TRAIN+"profit_last_year"]
    assert service["profit_delta_256"] == profit
    expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"] and last["company"]["cash"] >= 40000
    operation["measured_cash_flow"] = dict(ticks=elapsed,selected_cap=selected["ticks"],actual_fares=-expenses[7]-expenses[8],
        interest=expenses[11],other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
        cash_delta=last["company"]["cash"]-first["company"]["cash"],rail_initial=initial["quantity"],rail_boarded=service["boarded"],
        rail_accepted=service["accepted"],rail_aboard=final["quantity"],rail_share=final["share"],rail_profit_delta_256=profit,
        rail_pending_payment=pending,observed_whole_unload_fare=service["observed_whole_unload_fare"],
        cohort_only_fare_claimed=False,annual_projection=False)
    operation.update(work_refused=False,rail_payment_posted=service["payment_posted"],cumulative_rail_accepted=cumulative)
    return phase,operation


def main():
    """Publish one actual rail transport/qualified-payment interval or unchanged refusal."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    phase,operation = execute(args.configuration,ROOT/args.receipt,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=phase,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
