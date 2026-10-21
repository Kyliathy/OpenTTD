"""Bind actual first short-feeder transfer, native receiving packets and ordinary originals.

@file surface_entry_farm_feeder_follow_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGR.
Real Station15packets prove receipt; their preserved feeder share remains
virtual until final native rail delivery pays the actual company.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore
from surface_regional_ledger import TABLE

CALLER = Path(__file__).with_name("surface_entry_farm_feeder_follow_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def transferred_packets(path):
    """Inspect only actual sole native receiving-station packets from the new pickup.

    @param path Genuine current native original or ordinary resave after first transfer.
    @return Actual received Industry7packet quantities/first station/feeder shares.
    """
    station, packets = TABLE(path, b"STNN")[15], TABLE(path, b"CAPA")
    references = [ref for key, values in station.items()
                  if key.startswith("normal[0].goods[4].cargo[") and key.endswith(".second") for ref in values]
    received = [dict(packet=ref-1, **packets[ref-1]) for ref in references if packets[ref-1]["source"] == 22]
    assert received and all(p["source_type"] == 0 and p["source_id"] == 7 and p["source_xy"] == 1316 for p in received)
    assert sum(p["count"] for p in received) == 9 and sum(p["feeder_share"] for p in received) == 3
    return received


def main():
    """Retain exact transfer/ordinary evidence while leaving final payment and capital gates open."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-feeder-follow-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-feeder-work-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-feeder-follow/retained.json"],
            ("farm-feeder-follow-01",), "farm-feeder-follow-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/farm-feeder-follow-products" if args.retain else None)
        received = []
        # Each real original must retain the same nine-unit receiving owner
        # and three virtual money units, with no final company fare claimed.
        for row in result["configurations"]:
            received.append(transferred_packets(ROOT / row["output"]["path"]))
            assert row["observation"]["farm_road"]["held"] == 0 and row["observation"]["company"]["cash"] == 58044
        assert received[0] == received[1]
        result.update(_documentation=__doc__, actual_first_transfer=True, received_packets=received[0],
                      actual_reserve_restored=False, feeder_credit_is_cash=False, final_payment_proven=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
