"""Accept actual27chemical depot custody and ordinary static restoration.

@file surface_entry_chemical_pickup_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CK.3.
Only the original renewed tanker supplies later delivery. Native PCHM/fuel
history remains unchanged; a completed pickup grants no produced Propellant.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, identity
from surface_gateway_component_delivery_ledger import observe_chemical_pickup as observe
from surface_original_evidence import bind as bind_original, restore

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-chemical-renewal-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/salt-window-source-versions/before-chemical-pickup/retained.json"


def bind(retain):
    """@param retain Preserve current exact tested authorities. @return Actual pickup, original company/owners and ordinary static acceptance without claiming fuel."""
    directory = ROOT / "test-cache/surface-entry/chemical-pickup-products" if retain else None
    result = bind_original(FOUNDATION, [VERSIONS], ("chemical-pickup-query-01", "chemical-pickup-order", "chemical-pickup-work-01"),
                           "chemical-pickup-static", observe,
                           [Path(__file__), DIRECTORY / "surface_entry_chemical_pickup_checks-1.py"], directory)
    for configuration in result["configurations"]:
        current = configuration["observation"]
        carrier = current["chemical_carrier"]
        assert carrier["home"] and carrier["tile"] == 2443 and carrier["stored"] == 27 and carrier["reserved"] == 0
        assert current["company"]["cash"] >= 100000 and current["company"]["debt"] == 5000000
        assert current["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
        assert current["plant"] == dict(consumed=42, produced=42, dispatched=42, batches=21)
    result.update(_documentation=__doc__, chemicals_collected=True, chemical_delivered_to_pchm=False,
                  fuel_procured=False, caller=identity(Path(__file__)))
    return result


def main():
    """Publish exact current original chemical custody or its ordinary static proof."""
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
        value = restore(args.configuration, ROOT / args.receipt, observe, "chemical-pickup-static", __doc__)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
