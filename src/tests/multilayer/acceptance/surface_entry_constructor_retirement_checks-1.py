"""Bank only the actual empty-home builder refund and preserve consumed material history.

@file surface_entry_constructor_retirement_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVED.
No original freight, clock, loan, useful inventory or replacement is supplied.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_regional_ledger import TABLE
from surface_constructor_home_ledger import observe as old_present
from surface_constructor_retirement_ledger import observe
from surface_constructor_retirement_native import execute as native
from surface_evidence_identity_cache import IdentityCache

HERE = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(HERE / "surface_entry_constructor_home_checks-3.py"))
VERSIONS = PREVIOUS["VERSIONS"] + [ROOT / "logs/surface-entry-constructor-retirement-before-edit.json"]
OWNERS = [HERE / name for name in ("surface_gateway_core_ledger.py", "surface_gateway_assembly_ledger.py",
    "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_resupply_ledger.py",
    "surface_gateway_construction_ledger.py", "surface_gateway_component_delivery_ledger.py", "surface_farm_backhaul_ledger.py",
    "surface_surplus_recovery_ledger.py", "surface_constructor_home_ledger.py")]


def execute(configuration, recover_run=None):
    """Perform or recover precisely one actual public home sale of original38/gen2.

    @param configuration Independently linked isolated native application.
    @param recover_run Exact successful original invocation; no sale or earned work is ever repeated.
    @return Actual capital, live-body removal and unchanged physical stock plus retired embodied history.
    """
    receipt = ROOT / f"logs/surface-entry-constructor-home-work-001-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    assert prior["home_scope"]["end_reason"] == "home_arrived_stopped" and prior["arrived_stopped"]
    assert prior["final"]["company"]["cash"] == 41417 and prior["builder_refund_banked"] == 0
    cache = IdentityCache()
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS, identity_reader=cache)
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"] and json.loads(json.dumps(observe(source))) == prior["final"]
    attempt = ROOT / f"logs/surface-entry-constructor-retirement-pay-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the exact actual sale instead of repeating native commands"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), reserve=40000,
            original_cash=41417, expected_refund=29927, elapsed=0), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    inherited = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS, identity_reader=cache)]
    files = inherited + OWNERS + [Path(__file__), HERE / "surface_constructor_retirement_ledger.py",
        HERE / "surface_evidence_identity_cache.py", ROOT / "src/tests/surface_entry_constructor_retirement.h"] + VERSIONS
    # Repeated references do not add provenance. Retain each actual current
    # source path once, after its exact current or archived identity was checked.
    files = list(dict.fromkeys(files))
    operation = native(configuration, "constructor-retirement-pay", receipt, observe,
        dict(paid="CONSTRUCTOR_RETIREMENT_PAID", admission="CONSTRUCTOR_RETIREMENT_ADMISSION", invoices="INVOICE"),
        files, recover_run=recover_run, work_limit=64)
    cache.assert_unchanged()
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    assert first["company"]["cash"] == 41417 and last["company"]["cash"] == 71344
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    paid, = rows["paid"]
    invoice, = rows["invoices"]
    assert invoice["cost"] == paid["invoice"] == -29927 and paid["refund_banked"] == 29927
    assert paid["elapsed"] == paid["components_recovered"] == paid["freight_income"] == paid["avoided_cost_realized"] == 0
    assert paid["retired_steel"] == 4 and paid["retired_goods"] == 8 and paid["unused_returns_extinguished"] == 2
    assert not paid["replacement_bought"] and paid["incomplete_base"] == 610000 and not paid["replacement_launch_priced"]
    a, b = [TABLE(path, b"VEHS") for path in (before, output)]
    assert b.keys() == a.keys()-{38} and all(a[key] == b[key] for key in b)
    x, y = chunks(before)[1], chunks(output)[1]
    changed = {key for key in x if x[key] != y[key]}
    assert changed == {b"VEHS", b"PLYR", b"SVST", b"SPSU", b"SPOR"}, changed
    assert TABLE(before, b"DATE") == TABLE(output, b"DATE")
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert last["constructor_retired"] and last["goods_holds"] == {54: 0}
    assert last["embodied_inputs"] == {"STEL": 12, "GOOD": 16, "PROP": 0}
    assert last["retired_embodied_inputs"] == {"STEL": 4, "GOOD": 8, "PROP": 0}
    assert last["orbital_consumed"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    assert last["space_orders"]["generations"] == {54: 1} and last["manifests"].keys() == {54}
    assert [(craft["vehicle"], craft["generation"], craft["model"]) for craft in last["paid_craft"]] == [(54, 1, 14)]
    rejected = False
    try:
        old_present(output)
    except AssertionError:
        rejected = True
    assert rejected, "historical present-builder defaults must reject this actual retired incarnation"
    assert all(row["cash"] == 71344 and row["reserve"] == 40000 and row["target"] == row["operating"]+40000
        and row["future_income_counted"] == row["avoided_cost_realized"] == row["unbanked_refund_counted"] == 0
        and not row["work_permission"] for row in rows["admission"])
    assert rows["admission"][0]["ticks"] == 2000 and rows["admission"][0]["funded"]
    operation.update(earned_ticks=0, actual_cash=71344, refund_banked=29927, actual_retirement_complete=True,
        avoided_cost_realized=0, startup_reserve=40000, historical_present_rejected=True,
        historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=0, actual_fares=0, capital_recovered=29927, cash_delta=29927),
        incomplete_module_base=610000, incomplete_module_base_gap=538656, working_gap=28656,
        future_builder=dict(capital=100000, steel=4, goods=8, work=148, launch_service_priced=False, funded=False),
        remaining_scope=["actual first farm Goods payment", "real working/module/continuous-chain capital",
            "genuinely new builder/material/work/launch before dependent construction"])
    return operation


def main():
    """Retain only the original actual home retirement and its successful native process."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / f"logs/surface-entry-constructor-retirement-pay-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="constructor-retirement-pay", operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["rows"], flush=True)


if __name__ == "__main__":
    main()
