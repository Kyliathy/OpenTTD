"""Accept actual new launch fuel, native handling fees and remaining capital admission.

@file surface_entry_propellant_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CN.5.
Original center21must physically hold38PROP/32Steel/50Goods. Current
module/material/fuel quotes remain separate from actual paid module creation.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, identity
from surface_gateway_component_delivery_ledger import observe_propellant_delivery as observe
from surface_original_evidence import bind as bind_original, restore

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-propellant-pickup-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/salt-window-source-versions/before-propellant-delivery/retained.json"


def bind(retain):
    """@param retain Preserve current exact tested authorities. @return Actual38fuel storage and unpaid module/mission capital state, without claiming new modules."""
    directory = ROOT / "test-cache/surface-entry/propellant-delivery-products" if retain else None
    names = ("propellant-delivery-query-01", "propellant-delivery-order", "propellant-delivery-work-01",
             "propellant-handling-query", "propellant-handling-start", "propellant-handling-work", "modules-after-fuel-query")
    result = bind_original(FOUNDATION, [VERSIONS], names, "propellant-delivery-static", observe,
                           [Path(__file__), DIRECTORY / "surface_entry_propellant_delivery_checks-1.py"], directory)
    capital = []
    for configuration in result["configurations"]:
        current = configuration["observation"]
        assert current["new_fuel_at_center"] and current["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
        assert current["delivery_stocks"][21] == {"STEL": 32, "GOOD": 50, "PROP": 38}
        assert current["company"]["cash"] >= 100000 and current["company"]["debt"] == 5000000
        assert current["propellant_carrier"]["held"] == 0 and current["propellant_carrier"]["home"]
        assert not current["handling"]["jobs"]
        path = ROOT / f"logs/surface-entry-modules-after-fuel-query-{configuration['configuration']}.json"
        quote = json.loads(path.read_text(encoding="utf-8"))
        budget, = quote["operation"]["rows"]["budget"]
        fuel, = quote["operation"]["rows"]["fuel"]
        assert fuel["held"] == 38 and fuel["needed"] == 30 and fuel["missing"] == 0
        handling = ROOT / f"logs/surface-entry-propellant-handling-work-{configuration['configuration']}.json"
        operation = json.loads(handling.read_text(encoding="utf-8"))["operation"]
        assert operation["actual_handling_fee"] == 26
        capital.append(dict(configuration=configuration["configuration"], current_quote=identity(path), budget=budget))
    result.update(_documentation=__doc__, new_fuel_launch_delivered=26, launch_propellant=38, actual_handling_fee=26,
                  fuel_logistics_complete=True, module_capital=capital, modules_built=False, caller=identity(Path(__file__)))
    return result


def main():
    """Publish physical current launch fuel or its ordinary static proof."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "bind":
        value = bind(args.retain)
    else:
        assert args.configuration is not None and args.receipt is not None
        value = restore(args.configuration, ROOT / args.receipt, observe, "propellant-delivery-static", __doc__)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
