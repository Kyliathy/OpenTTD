"""Bind only actual finite return-delivery originals and ordinary reloads.

@file surface_entry_return_passenger_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZA.
No native interval follows a real payment/goal/refusal; retained exact current
products preserve default source contracts and actual posted company books.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_return_passenger_delivery_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain the actual finite original sequence with exact payment and ordinary evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--windows",type=int,choices=(1,2,3),default=1)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"return-passenger-delivery-static",__doc__)
    else:
        names = tuple(f"return-passenger-delivery-{i:02d}" for i in range(1,args.windows+1))
        # A posted return fare, actual working goal or refused full funding
        # ends this finite scope; later windows cannot silently reopen it.
        for configuration in ("RelWithDebInfo","Debug"):
            for name in names[:-1]:
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                assert not value["work_refused"] and not value["return_payment_posted"]
                assert value["final"]["company"]["cash"] < 100000
        result = bind(ROOT/"logs/surface-entry-return-passenger-follow-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-return-passenger-delivery/retained.json",
             ROOT/"test-cache/surface-entry/salt-window-source-versions/before-return-payment-observation/retained.json"],
            names,"return-passenger-delivery-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/return-passenger-delivery-products" if args.retain else None)
        finals = [json.loads((ROOT/f"logs/surface-entry-{names[-1]}-{c}.json").read_text(encoding="utf-8"))["operation"]
                  for c in ("RelWithDebInfo","Debug")]
        assert finals[0]["return_payment_posted"] == finals[1]["return_payment_posted"]
        result.update(_documentation=__doc__,return_payment_posted=finals[0]["return_payment_posted"],
            actual_reserve_restored=all(v["final"]["company"]["cash"] >= 100000 for v in finals),module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
