"""Seal actual earned capital and every native predecessor on both normal worlds.

@file normal_ground_financing_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CB.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Dated predecessor receipts retain their actual source/binary identities;they are not rerun against later observers.
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
from save_tables import saved_table

CONSTRUCTION = "logs/acceptance/bootstrap/20261019-native-01/freight-construction-verification.json"
CONSTRUCTION_SHA = "e196685abb22ef3ca1f3db1145170d8658e8d7bfd48a157b42c628b4453e2298"


def inspect_lineage(final_path, configuration, foundation):
    """Verify exact input/output files,real native ledgers and complete pure saves back to sealed construction."""
    root = foundation["records"][configuration]
    selected = identity(ROOT / final_path)
    current = selected
    records = []
    seen = set()
    original = root["actual"]["original_input"]
    original_id = saved_table(verify_reference(original), b"DATE")[0]["id"]
    while True:
        path = verify_reference(current)
        assert current["path"] not in seen
        seen.add(current["path"])
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == configuration and report["original_input"] == original
        assert report["children_reaped"] is True and report["pure_chunks_equal"] == 126
        assert report["bootstrap_claim"] is False
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        earned = report["classification"] == "normal_start_freight_earnings"
        closing = report["earned" if earned else "ready"]
        save = verify_reference(report["saves"]["earned" if earned else "ready"])
        assert native_world(save) == native_world(verify_reference(report["saves"]["repeat"]))
        assert saved_table(save, b"DATE")[0]["id"] == original_id
        player = saved_table(save, b"PLYR")[0]
        assert player["money"] == closing["cash"] and player["current_loan"] == closing["loan"] == 300000
        if earned:
            assert report["earned"]["income"] > 0 and report["earned"]["delivered"] > 0
            assert report["earned"]["cash"] == report["earned"]["opening_cash"] - sum(row["cost"] for row in report["ledger"])
            assert [row["category"] for row in report["ledger"]] == list(range(13))
            assert sum(row["amount"] for row in report["deliveries"]) == report["earned"]["delivered"]
            assert all((row["visited"] > 20 and row["peak_load"] > 0) or
                       (row.get("waiting", 0) == 1 and 0 <= row["held"] < row["capacity"]) for row in report["trains"])
        else:
            assert report["classification"] in ("normal_start_paid_freight", "normal_start_paid_freight_extension")
            assert sum(row["cost"] for row in report["invoices"]) == report["construction_expense"]
        records.append({"report": current, "actual": report})
        if current == root["report"]:
            assert report == root["actual"]
            break
        prior = json.loads(verify_reference(report["predecessor"]).read_text(encoding="utf-8"))
        prior_earned = prior["classification"] == "normal_start_freight_earnings"
        prior_closing = prior["earned" if prior_earned else "ready"]
        assert report["actual_input"] == prior["saves"]["earned" if prior_earned else "ready"]
        verify_reference(report["actual_input"])
        if earned:
            assert closing["first_tick"] == prior_closing["tick"]
            assert closing["opening_cash"] == prior_closing["cash"]
            assert closing["front"] == prior_closing["front"]
            if not prior_earned:
                assert all(row["visited"] > 20 and row["peak_load"] > 0 for row in report["trains"])
        else:
            assert closing["opening_cash"] == prior_closing["cash"]
            assert closing["cash"] == closing["opening_cash"] - report["construction_expense"]
        current = report["predecessor"]
    records.reverse()
    paid = sum(row["actual"].get("construction_expense", 0) for row in records)
    income = sum(row["actual"].get("earned", {}).get("income", 0) for row in records)
    operating = sum(row["actual"].get("earned", {}).get("expenses", 0) for row in records)
    overhead = sum(row["actual"].get("earned", {}).get("overhead", 0) for row in records)
    last = records[-1]["actual"]
    closing = last["earned"] if last["classification"] == "normal_start_freight_earnings" else last["ready"]
    assert closing["cash"] == 300000 - paid + income + operating - overhead
    return {"final": selected, "records": records, "capital_paid": paid, "income": income,
            "operating": operating, "overhead": overhead,
            "final_input": last["saves"]["earned" if last["classification"] == "normal_start_freight_earnings" else "ready"],
            "closing": closing,
            "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records)}


def accept_network(report_paths, output):
    """Seal all four paid routes before financial travel;compare every cross-build byte except the original native ID."""
    foundation_path = ROOT / CONSTRUCTION
    assert identity(foundation_path)["sha256"] == CONSTRUCTION_SHA
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    records, worlds, dates = {}, {}, {}
    for name in report_paths:
        cfg = json.loads((ROOT / name).read_text(encoding="utf-8"))["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        records[cfg] = inspect_lineage(name, cfg, foundation)
        lineage = records[cfg]["records"]
        assert len(lineage) == 4
        final = lineage[-1]["actual"]
        assert final["ready"]["fleet"] == 4 and final["ready"]["tick"] == 1280
        assert records[cfg]["capital_paid"] == 143413
        assert final["ready"]["cash"] == 156587
        save = verify_reference(final["saves"]["ready"])
        worlds[cfg] = native_world(save)
        dates[cfg] = saved_table(save, b"DATE")[0]
    assert set(records) == {"Debug", "RelWithDebInfo"}
    assert [key for key in worlds["Debug"] if worlds["Debug"][key] != worlds["RelWithDebInfo"][key]] == ["DATE"]
    assert {key: value for key, value in dates["Debug"].items() if key != "id"} == {
        key: value for key, value in dates["RelWithDebInfo"].items() if key != "id"}
    before, after = (bytes.fromhex(dates[cfg]["id"]) for cfg in ("Debug", "RelWithDebInfo"))
    assert before != after and len(before) == len(after) == 32
    assert worlds["Debug"]["DATE"].count(before) == worlds["RelWithDebInfo"]["DATE"].count(after) == 1
    assert worlds["Debug"]["DATE"].replace(before, after, 1) == worlds["RelWithDebInfo"]["DATE"]
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CB.2;actual paid four-route financing predecessor,not enough earned capital or underground readiness",
              "controller": identity(__file__), "construction_handoff": identity(foundation_path), "records": records,
              "classification": "normal_start_paid_freight_network", "bootstrap_claim": False,
              "native_cases": 8, "native_assertions": sum(row["native_assertions"] for row in records.values()),
              "pure_pairs": 8, "complete_chunks_equal": 125, "other_date_fields_equal": len(dates["Debug"]) - 1,
              "ignored_chunks": [], "remaining": "6CB.2 enough actual earned capital;6CC/6CD paid underground readiness;6D normal-start S1/S2."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def accept(report_paths, quote_paths, output):
    """Require enough genuine native earned capital for quoted receivers/models and a declared500000construction reserve."""
    foundation_path = ROOT / CONSTRUCTION
    assert identity(foundation_path)["sha256"] == CONSTRUCTION_SHA
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    records, quotes = {}, {}
    for name in report_paths:
        cfg = json.loads((ROOT / name).read_text(encoding="utf-8"))["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        records[cfg] = inspect_lineage(name, cfg, foundation)
    for name in quote_paths:
        path = ROOT / name
        quote = json.loads(path.read_text(encoding="utf-8"))
        cfg = quote["configuration"]
        assert cfg in records and cfg not in quotes
        final = records[cfg]["records"][-1]["actual"]
        assert quote["classification"] == "normal_start_readiness_quote"
        assert quote["children_reaped"] is True and quote["pure_chunks_equal"] == 126
        for reference in (quote["controller"], quote["binary"], quote["config"], quote["startup"], *quote["sources"]):
            verify_reference(reference)
        text = verify_reference(quote["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == quote["native"]["assertions"] > 0
        assert int(counts[2]) == quote["native"]["cases"] == 1
        assert len(quote["receivers"]) == 2 and {row["kind"] for row in quote["receivers"]} == {0, 1}
        assert len(quote["models"]) == 2 and {row["id"] for row in quote["models"]} == {0, 1}
        assert all(row["capital"] > 0 for row in quote["receivers"])
        assert all(row["purchase"] > 0 for row in quote["models"])
        assert quote["predecessor"] == records[cfg]["final"]
        assert quote["actual_input"] == final["saves"]["earned"]
        assert native_world(verify_reference(quote["saves"]["before"])) == native_world(verify_reference(quote["saves"]["after"]))
        reserve = 500000
        required = sum(row["capital"] for row in quote["receivers"]) + sum(row["purchase"] for row in quote["models"]) + reserve
        assert final["earned"]["cash"] == quote["readiness"]["cash"] >= required
        quotes[cfg] = {"report": identity(path), "actual": quote, "construction_reserve": reserve, "required": required}
    assert set(records) == set(quotes) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CB.2;actual native normal-start freight capital and immutable paid/earned lineage",
              "controller": identity(__file__), "construction_handoff": identity(foundation_path), "records": records, "quotes": quotes,
              "classification": "normal_start_earned_capital", "bootstrap_claim": False,
              "native_cases": sum(len(row["records"]) for row in records.values()),
              "native_assertions": sum(row["native_assertions"] for row in records.values()),
              "ignored_chunks": [], "remaining": "6CC actual paid site/yard;6CD real hauler/commissioning/earned access;6D normal-start S1/S2."}
    target = ROOT / output
    assert target.resolve().is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Seal only actual sufficient earned capital from both native builds."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--quote", action="append")
    parser.add_argument("--network", action="store_true", help="Seal paid four-route predecessors before actual earnings")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    if args.network:
        assert not args.quote
        accept_network(args.report, args.output)
    else:
        assert args.quote
        accept(args.report, args.quote, args.output)


if __name__ == "__main__":
    main()
