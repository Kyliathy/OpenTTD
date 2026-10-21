"""Bind pure constructor lifecycle quotes and unchanged ordinary originals.

@file surface_entry_constructor_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZJ.
All craft and embodied materials remain present. No Visit, sale, replacement,
avoided cost or future income has been supplied by these pure estimates.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_constructor_capital_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain unchanged actual constructor quotes and both ordinary reconstructions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"constructor-capital-static",__doc__)
    else:
        result = bind(ROOT/"logs/surface-entry-feeder-reserve-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-constructor-capital/retained.json"],
            ("constructor-capital-query",),"constructor-capital-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/constructor-capital-products" if args.retain else None)
        operations = [json.loads((ROOT/f"logs/surface-entry-constructor-capital-query-{c}.json").read_text(encoding="utf-8"))["operation"]
                      for c in ("RelWithDebInfo","Debug")]
        assert operations[0]["replacement_prerequisites"] == operations[1]["replacement_prerequisites"]
        assert all(not op["retirement_admitted"] and op["home_visit_quoted"] for op in operations)
        result.update(_documentation=__doc__,actual_sale_available=False,actual_sale_complete=False,
            home_visit_quoted=True,constructor_return_complete=False,retirement_admitted=False,
            replacement_prerequisites=operations[0]["replacement_prerequisites"],
            actual_reserve_restored=False,module_capital_complete=False,working_gap=50750,incomplete_module_base_gap=460750)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
