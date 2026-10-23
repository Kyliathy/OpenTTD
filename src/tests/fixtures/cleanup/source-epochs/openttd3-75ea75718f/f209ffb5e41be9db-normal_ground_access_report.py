"""Seal public release and paid ordinary access on actual normally earned floor.

@file normal_ground_access_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Complete geological histories and receiver stocks survive native release/conversion.
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
from normal_ground_access import WORK, WORK_SHA, fields
from save_tables import saved_table


def accept(report_paths, output):
    """Verify actual native final release,each conversion invoice and unchanged earned material owners."""
    handoff = ROOT / WORK
    assert identity(handoff)["sha256"] == WORK_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    records = {}
    owners = ("GEOS", "GDEP", "GSPN", "GVNT", "EXJS", "XJOB", "XJRT", "XJTL", "XPAS", "SPRR", "INDY", "DATE")
    for name in report_paths:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        flow = basis["records"][cfg]
        assert report["classification"] == "normal_start_ordinary_underground_access" and report["bootstrap_claim"] is False
        assert report["work_handoff"] == identity(handoff) and report["actual_input"] == flow["settled_input"]
        assert report["original_input"] == flow["records"][0]["actual"]["original_input"]
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        for reference in (report["controller"], report["binary"], report["config"], report["startup"], *report["sources"]):
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        source = verify_reference(report["actual_input"])
        access, repeat = (verify_reference(report["saves"][key]) for key in ("access", "repeat"))
        before, after = native_world(source), native_world(access)
        assert after == native_world(repeat)
        assert all(after[owner] == before[owner] for owner in owners)
        measured = report["access"]
        assert measured == fields(text, "NORMAL_GROUND_ACCESS")
        assert measured["opening_cash"] == flow["closing"]["cash"] and measured["tick"] == flow["closing"]["tick"]
        assert measured["cutter"] == flow["closing"]["cutter"] and measured["hauler"] == flow["closing"]["hauler"]
        assert measured["loan"] == 300000 and measured["excavated"] == 36 and measured["state"] == 2
        invoices = [{key: value if key == "action" else int(value) for key, value in
                     (item.split("=", 1) for item in row.split())}
                    for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
        assert invoices == report["invoices"]
        assert sum(row["action"] == "retire_settled_standard" for row in invoices) == 1
        assert sum(row["action"] == "ordinary_access" for row in invoices) == 1
        assert sum(row["action"] == "ordinary_support" for row in invoices) == measured["support"]
        assert measured["support"] == len(saved_table(source, b"TBMS")) == 9
        assert measured["conversion"] == sum(row["cost"] for row in invoices if row["action"] == "ordinary_support")
        cash = measured["opening_cash"]
        for invoice in invoices:
            cash -= invoice["cost"]
            assert cash == invoice["cash"]
        assert cash == measured["cash"] > 0
        assert report["capital_paid"] == sum(row["cost"] for row in invoices) == measured["conversion"] == 675
        company = saved_table(access, b"PLYR")[0]
        assert company["money"] == cash and company["current_loan"] == 300000
        for owner in (b"TBMP", b"TBMS", b"TBXA", b"TBXS", b"TBXW"):
            assert saved_table(access, owner) == {}
        portals = saved_table(access, b"LPWK")
        assert len(portals) == 1
        portal = next(iter(portals.values()))
        assert portal["key"] == measured["portal"] and portal["state"] == 2 and portal["company"] == 0
        assert portal["project"] == 4294967295 and portal["cutter"] == 1048575 and portal["origin"] == 65535
        records[cfg] = {"report": identity(path), "actual": report, "access_input": report["saves"]["access"], "access": measured}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CDCC;actual native settled release and paid ordinary connection;earned floor/materials unchanged",
              "controller": identity(__file__), "work_handoff": identity(handoff), "records": records,
              "classification": "normal_start_ordinary_underground_access", "bootstrap_claim": False,
              "native_cases": 2, "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_chunks_equal": 126, "unchanged_owners": list(owners), "ignored_chunks": [],
              "remaining": "6CDD actual Deep machinery readiness;6D full normal-start S1/S2."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only paid native ordinary access on both actually earned original floors."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
