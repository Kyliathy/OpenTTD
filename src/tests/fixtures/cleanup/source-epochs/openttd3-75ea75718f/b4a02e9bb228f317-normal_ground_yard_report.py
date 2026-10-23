"""Seal native paid Surface yards on both exact normally financed originals.

@file normal_ground_yard_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every paid command and native owner is retained;sealed housing creates no excavated floor.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from construction_provider_return import identity
from normal_ground_start_report import native_world
from normal_ground_yard import FINANCE, FINANCE_SHA
from save_tables import saved_table


def accept(report_paths, output):
    """Verify actual native payment,registered receivers,sealed work authority and unchanged original rock owners."""
    handoff = ROOT / FINANCE
    assert identity(handoff)["sha256"] == FINANCE_SHA
    funded = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    geological_owners = ("GEOS", "EXJS", "XJOB", "XJRT", "XJTL", "XPAS", "TBMH", "TBMP", "TBMS")
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = funded["records"][cfg]
        assert report["financing_handoff"] == identity(handoff)
        assert report["actual_input"] == flow["final_input"]
        assert report["classification"] == "normal_start_paid_surface_yard" and report["bootstrap_claim"] is False
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        paid = verify_reference(report["saves"]["paid"])
        repeat = verify_reference(report["saves"]["repeat"])
        before, after = native_world(source), native_world(paid)
        assert after == native_world(repeat)
        assert all(before[owner] == after[owner] for owner in geological_owners)
        yard = report["yard"]
        assert yard["opening_cash"] == flow["closing"]["cash"]
        assert yard["loan"] == flow["closing"]["loan"] == 300000
        assert yard["tick"] == flow["closing"]["tick"]
        date = saved_table(paid, b"DATE")[0]
        assert date["id"] == saved_table(source, b"DATE")[0]["id"]
        assert date["tick_counter"] == yard["tick"]
        assert date["pause_mode"] == saved_table(source, b"DATE")[0]["pause_mode"]
        cash = yard["opening_cash"]
        for invoice in report["invoices"]:
            cash -= invoice["cost"]
            assert cash == invoice["cash"]
        assert cash == yard["cash"] > 0
        assert sum(row["cost"] for row in report["invoices"]) == report["capital_paid"] == 1409803
        player = saved_table(paid, b"PLYR")[0]
        assert player["money"] == cash and player["current_loan"] == 300000
        receivers = saved_table(paid, b"SPRR")
        assert len(receivers) == 2
        assert {row["kind"] for row in receivers.values()} == {0, 1}
        industries = saved_table(paid, b"INDY")
        for row in receivers.values():
            assert row["industry"] == yard["earth" if row["kind"] == 0 else "rock"]
            assert industries[row["industry"]]["founder"] == 0
            assert all(row[key] == 0 for key in ("interval_ticks", "interval_intake", "aggregate", "disposed", "crushed", "aggregate_made", "residue", "fees"))
        portals = saved_table(paid, b"LPWK")
        assert len(portals) == 1
        portal = next(iter(portals.values()))
        assert portal["key"] == yard["portal"] and portal["state"] == 0 and portal["company"] == 0
        assert portal["project"] == 4294967295 and portal["origin"] == 65535
        assert saved_table(paid, b"XJOB") == {}
        records[cfg] = {"report": identity(path), "actual": report, "paid_input": report["saves"]["paid"], "yard": yard}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CC;native earned funds buy actual Surface disposal/logistics/sealed housing;all rock remains original",
              "controller": identity(__file__), "financing_handoff": identity(handoff), "records": records,
              "classification": "normal_start_paid_surface_yard", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "unchanged_geological_owners": list(geological_owners), "ignored_chunks": [],
              "remaining": "6CD real hauler/commissioning/earned access;6D original normal-start S1/S2 clearing and recurring spoil logistics."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only actual paid Surface readiness predecessors."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
