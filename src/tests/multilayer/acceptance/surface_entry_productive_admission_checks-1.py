"""Query real affordable productive recovery without changing original owners.

@file surface_entry_productive_admission_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZU.
Actual native demand, command invoices and full operating costs supply evidence;
neither a pure quote nor an unbanked asset authorizes purchase or earned work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_productive_native import execute as native

HERE = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(HERE / "surface_entry_goods_rail_delivery_checks-1.py"))
observe = RAIL["observe"]
PHASE = "productive-query"


def execute(configuration, recover_run=None):
    """Inspect one exact current original through the isolated pure native target.

    @param configuration Original independently linked native build configuration.
    @param recover_run Exact previous native invocation to inspect without repeating it.
    @return Pure native demand, model/refit and whole-bill quotes with all original owners intact.
    @throws AssertionError On changed originals, native mutation or unpriced funded model.
    """
    receipt = ROOT / f"logs/surface-entry-arrival-settlement-001-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
    assert previous["settlement_scope"]["ended"] and previous["rail_payment_posted"]
    assert all(identity(ROOT / ref["path"]) == ref for ref in previous["sources"])
    archive = ROOT / "logs/surface-entry-productive-admission-before-edit.json"
    protected = json.loads(archive.read_text(encoding="utf-8"))["files"]
    assert all(identity(ROOT / ref["path"])["sha256"] == ref["sha256"] for ref in protected
               if ref["path"].endswith(".exe"))
    files = [ROOT / ref["path"] for ref in previous["sources"]] + [Path(__file__), ROOT / "CMakeLists.txt"]
    attempt = ROOT / f"logs/surface-entry-productive-admission-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the original query invocation instead of repeating it"
        attempt.write_text(json.dumps(dict(receipt=identity(receipt), native_invocations=1, earned_ticks=0),
            indent=2)+"\n", encoding="utf-8")
    else:
        assert attempt.is_file()
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="PRODUCTIVE_SOURCE", demand="PRODUCTIVE_DEMAND", models="PRODUCTIVE_MODEL",
             vehicles="PRODUCTIVE_VEHICLE", refits="PRODUCTIVE_REFIT"), files, recover_run=recover_run)
    assert json.loads(json.dumps(operation["initial"])) == previous["final"]
    assert operation["initial"] == operation["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["cash"] == 45080 and source["reserve"] == 40000 and source["debt"] == 5000000
    assert source["paid"] == source["elapsed"] == source["future_income_counted"] == 0
    assert operation["rows"]["demand"] and operation["rows"]["models"]
    # A funded model means its entire actual invoice, added upkeep, all current
    # company costs and the existing reserve fit current cash. It remains a
    # read-only financial quote, granting no route or passenger profitability.
    for model in operation["rows"]["models"]:
        assert model["new_body_running_cost_priced"] and not model["work_permission"]
        assert model["target"] == model["invoice"]+model["existing_operating"]+model["added_upkeep"]+40000
        assert model["funded"] == (model["target"] <= 45080)
    operation.update(before_edit=identity(archive), earned_ticks=0, paid=0,
        protected_original_applications=[ref for ref in protected if ref["path"].endswith(".exe")],
        funded_models=[model for model in operation["rows"]["models"] if model["funded"]],
        productive_consumer_admitted=False, startup_reserve=40000)
    return operation


def main():
    """Retain exactly one original pure productive admission or recover its exact native invocation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / f"logs/surface-entry-productive-admission-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase=PHASE, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), "funded models", operation["funded_models"], flush=True)


if __name__ == "__main__":
    main()
