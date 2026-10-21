"""Accept actual fuel collection after a retained empty dispatch-activation round.

@file surface_entry_propellant_pickup_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CM.3.
The first real round left26Source6units at station10; a separately funded
second round actually stored them in original reserve2at depot2443.
"""
import argparse
import json
from pathlib import Path
import re

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_propellant_pickup as observe
from surface_original_evidence import bind as bind_original, restore
from surface_propellant_ledger import FACTORY
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-chemical-delivery-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/salt-window-source-versions/before-propellant-pickup/retained.json"


def waiting_source(path):
    """Read actual dispatched Source6fuel after the first empty native depot return.

    @param path Exact first-round original output, never a recreated simulation.
    @return Actual pickup station, sole native packet references and26physical waiting units.
    """
    raw = chunks(path)[1][b"MAP2"][3:]
    assert int.from_bytes(raw[1896 * 2:1896 * 2 + 2], "big") == 10
    labels = FACTORY["observe_factory"](path)["cargo_labels"]
    packets, station = TABLE(path, b"CAPA"), TABLE(path, b"STNN")[10]
    actual = []
    # Follow only this actual pickup station's PROPpacket references. Neither
    # the original twelve at center21nor a historical dispatched counter may
    # replace the26units that the second native round actually found waiting.
    for field, references in station.items():
        match = re.fullmatch(r"normal\[0\]\.goods\[(\d+)\]\.cargo\[\d+\]\.second", field)
        if match is None:
            continue
        if labels[int(match.group(1))] != "PROP":
            continue
        for reference in references:
            packet = packets[reference - 1]
            assert packet["source_type"] == 0 and packet["source_id"] == 6
            actual.append(dict(packet=reference - 1, **packet))
    assert sum(packet["count"] for packet in actual) == 26
    return dict(station=10, tile=1896, waiting=26, packets=actual, world=identity(path))


def bind(retain):
    """@param retain Preserve exact tested current authorities. @return Both actual rounds and ordinary loading of26stored Source6fuel, without claiming launch delivery."""
    directory = ROOT / "test-cache/surface-entry/propellant-pickup-products" if retain else None
    names = ("propellant-pickup-query-01", "propellant-pickup-order", "propellant-pickup-work-01",
             "propellant-pickup-query-02", "propellant-pickup-order-02", "propellant-pickup-work-02")
    result = bind_original(FOUNDATION, [VERSIONS], names, "propellant-pickup-static", observe,
                           [Path(__file__), DIRECTORY / "surface_entry_propellant_pickup_checks-1.py"], directory)
    first_round_sources = []
    for configuration in result["configurations"]:
        current = configuration["observation"]
        carrier = current["propellant_carrier"]
        assert carrier["home"] and carrier["tile"] == 2443 and carrier["stored"] == 26 and carrier["reserved"] == 0
        assert current["company"]["cash"] >= 100000 and current["company"]["debt"] == 5000000
        assert current["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
        assert current["delivery_stocks"][21] == {"STEL": 32, "GOOD": 50, "PROP": 12}
        first = ROOT / f"logs/surface-entry-propellant-pickup-work-01-{configuration['configuration']}.json"
        receipt = json.loads(first.read_text(encoding="utf-8"))
        progress, = receipt["operation"]["rows"]["progress"]
        assert progress["round_home"] and not progress["collected"] and progress["held"] == 0
        first_round_sources.append(dict(configuration=configuration["configuration"], receipt=identity(first),
                                        source=waiting_source(ROOT / receipt["operation"]["output"]["path"])))
    result.update(_documentation=__doc__, new_fuel_collected=26, first_empty_round_sources=first_round_sources,
                  launch_fuel_delivered=False, caller=identity(Path(__file__)))
    return result


def main():
    """Publish exact original collected fuel or its ordinary static proof."""
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
        value = restore(args.configuration, ROOT / args.receipt, observe, "propellant-pickup-static", __doc__)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
