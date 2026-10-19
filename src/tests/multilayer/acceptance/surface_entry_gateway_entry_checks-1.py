"""Bind genuine Surface procurement to the paid, loaded launch-entry checkpoint.

@file surface_entry_gateway_entry_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15B/15BIE.
Historical receipts retain their original native versions. This read-only join
checks exact save ancestry without replaying their accepted executable graphs.
The continuous orbital mission remains a separate, unfinished acceptance task.
"""
import json
from pathlib import Path

from space_preview_checks import ROOT, identity
from surface_gateway_payload_ledger import observe


def operation(value):
    """@param value Historical receipt in its actual published schema. @return Native source/output operation, or an empty mapping."""
    current = value.get("operation") or value.get("result", {}).get("process")
    if current:
        return current
    native = value.get("native", {})
    # The original credit producer published ordered snapshots rather than an
    # output field. Its final snapshot is the actually paid command result.
    if native.get("snapshots"):
        return dict(native, output=native["snapshots"][-1])
    return {}


def checked(reference):
    """@param reference Exact workspace-relative path and SHA256. @return Existing immutable referenced path."""
    path = ROOT / reference["path"]
    assert identity(path) == reference, reference
    return path


def main():
    """Verify both original ancestries, current products and actual sole-owner funded launch entry; publish only the bounded entry acceptance."""
    foundation_path = ROOT / "logs/surface-entry-gateway-payload-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["actual_ground_payload_complete"]
    assert foundation["actual_flight_admitted"] and not foundation["actual_launch_complete"]
    baseline_path = ROOT / "logs/surface-entry-baseline-verification.json"
    baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
    original_path = ROOT / "logs/surface-entry-original-RelWithDebInfo.json"
    original = json.loads(checked(json.loads(original_path.read_text())["report"]).read_text())
    assert original["original"] == baseline["original"]
    assert original["money"] == original["loan"] == 100000
    assert original["packet_count"] == original["company_vehicle_count"] == original["cash_granted_by_harness"] == 0
    assert original["maximum_bank_setting"] == 5000000
    assert baseline["complete"] and not baseline["cash_or_stock_supplied_by_harness"]
    checked(original["original"])
    receipts = {path.relative_to(ROOT).as_posix(): json.loads(path.read_text(encoding="utf-8"))
                for path in (ROOT / "logs").glob("surface-entry-*.json")}
    index = {}
    # Index actual producer outputs only. Summary/configuration aliases cannot
    # silently replace a missing predecessor in the selected physical lineage.
    for name, value in receipts.items():
        current = operation(value)
        if current.get("source") and current.get("output"):
            output = current["output"]
            index.setdefault((output["path"], output["sha256"]), []).append(name)
    products = json.loads(checked(foundation["products"]).read_text(encoding="utf-8"))
    for product in products["products"]:
        checked(product["source"]); checked(product["retained"])
    summaries = []
    # Each independently tested branch must reach the same authentic new-game
    # root through exact output-to-input identities. Copies never contribute
    # additional cash, packets or work to the selected original branch.
    for entry in foundation["configurations"]:
        current_path = checked(entry["receipt"])
        chain, seen, processes = [], set(), set()
        while True:
            name = current_path.relative_to(ROOT).as_posix()
            assert name not in seen, name
            seen.add(name); value = receipts[name]; current = operation(value)
            assert value["complete"] and value["configuration"] == entry["configuration"]
            checked(current["source"]); checked(current["output"])
            for reference in current["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8"))
                assert process["exit_status"] == 0, reference
                processes.add(reference["path"])
            chain.append(dict(receipt=identity(current_path), source=current["source"], output=current["output"]))
            if current["source"] == baseline["original"]:
                assert name == f"logs/surface-entry-credit-{entry['configuration']}.json"
                assert current["borrowed"] == 1900000 and current["final_cash"] == current["final_debt"] == 2000000
                break
            reference = current.get("receipt")
            if reference:
                predecessor = checked(reference)
            else:
                source = current["source"]
                candidates = index.get((source["path"], source["sha256"]), [])
                assert len(candidates) == 1, (name, candidates)
                predecessor = ROOT / candidates[0]
            assert operation(receipts[predecessor.relative_to(ROOT).as_posix()])["output"] == current["source"]
            current_path = predecessor
        snapshot = observe(checked(entry["output"]))
        assert json.loads(json.dumps(snapshot)) == receipts[entry["receipt"]["path"]]["operation"]["final"]
        assert snapshot["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 42}
        assert snapshot["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
        assert snapshot["company"] == {"cash": 878761, "debt": 4960000}
        assert min(row["remaining"] for row in snapshot["service_lives"]) == 1116
        summaries.append(dict(configuration=entry["configuration"], receipt=entry["receipt"], output=entry["output"],
                              company=snapshot["company"], spendable=entry["spendable"], lineage=chain,
                              lineage_receipts=len(chain), successful_historical_processes=len(processes),
                              totals=snapshot["totals"], embodied_inputs=snapshot["embodied_inputs"],
                              paid_craft=snapshot["paid_craft"], manifests=snapshot["manifests"], facility=snapshot["facility"]))
    output = ROOT / "logs/surface-entry-gateway-entry-verification.json"
    assert not output.exists()
    result = dict(_documentation=__doc__, complete=True, genuine_surface_and_launch_entry_complete=True,
                  continuous_orbital_chain_complete=False, native_reruns=0, foundation=identity(foundation_path),
                  baseline=identity(baseline_path), original_membership=identity(original_path),
                  configurations=summaries, products=foundation["products"], retained_products=len(products["products"]),
                  source=identity(Path(__file__)))
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), [(row["configuration"], row["lineage_receipts"]) for row in summaries], flush=True)


if __name__ == "__main__":
    main()
