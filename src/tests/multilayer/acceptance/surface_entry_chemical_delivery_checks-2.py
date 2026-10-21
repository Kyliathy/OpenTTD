"""Accept real chemical delivery/PCHMproduction and ordinary static restoration.

@file surface_entry_chemical_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CL.3.
The actual27unit delivery yields26new PROP through native batches and leaves
one chemical. Production history is separate from physical fuel collection.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, identity
from surface_gateway_component_delivery_ledger import observe_chemical_delivery as observe
from surface_original_evidence import bind as bind_original, restore

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-chemical-pickup-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/salt-window-source-versions/before-chemical-delivery/retained.json"


def bind(retain):
    """@param retain Preserve exact current authorities for later physical fuel pickup. @return Actual27CHEMdelivery/26PROPproduction and static ordinary acceptance; no launch fuel is granted."""
    directory = ROOT / "test-cache/surface-entry/chemical-delivery-products" if retain else None
    result = bind_original(FOUNDATION, [VERSIONS], ("chemical-delivery-query-01", "chemical-delivery-order", "chemical-delivery-work-01"),
                           "chemical-delivery-static", observe,
                           [Path(__file__), DIRECTORY / "surface_entry_chemical_delivery_checks-1.py"], directory)
    for configuration in result["configurations"]:
        current = configuration["observation"]
        carrier, plant = current["chemical_carrier"], current["plant"]
        assert carrier["home"] and carrier["tile"] == 2443 and carrier["held"] == 0
        assert current["company"]["cash"] >= 100000 and current["company"]["debt"] == 5000000
        assert current["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
        assert plant["delivered"] == 27 and plant["additional_consumed"] == plant["additional_produced"] == 26
        assert plant["chemical_input"] == 1
    result.update(_documentation=__doc__, chemical_delivered_to_pchm=True, new_propellant_produced=26,
                  fuel_collected=False, launch_fuel_delivered=False, caller=identity(Path(__file__)))
    return result


def main():
    """Publish current original PCHMcustody or its ordinary static proof."""
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
        value = restore(args.configuration, ROOT / args.receipt, observe, "chemical-delivery-static", __doc__)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
