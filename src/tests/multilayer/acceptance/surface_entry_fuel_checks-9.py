"""Bind genuine one-town service admission or its actual original paid construction.

@file surface_entry_fuel_checks-9.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIB.2.
The original loan increases matching cash/debt without income. No town expense
may occur until real sufficient treasury admits the complete native placement.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_income_ledger import TABLE


def bind(stage):
    """@param stage Complete pre-purchase admission or actual original town purchase. @return Both-build native proof."""
    foundation_path = ROOT / "logs/surface-entry-income-bank-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and not foundation["full_propellant_affordability"]
    aliases = {}
    manifest = json.loads((ROOT / "test-cache/surface-entry/income-town-query-products/retained.json").read_text(encoding="utf-8"))
    for row in manifest["retained"]:
        aliases[row["sha256"]] = ROOT / row["path"]

    def checked(reference):
        """@param reference Exact original artifact. @return Verified original or explicitly retained alias."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Check complete original artifact membership."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    phases = ("query", "credit") if stage == "plan" else ("query", "credit", "buy")
    physical, summaries, processes, field_rows = [], [], set(), []
    for configuration in ("RelWithDebInfo", "Debug"):
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        snapshots, receipts, rows = [], [], []
        for phase in phases:
            path = ROOT / f"logs/surface-entry-income-town-{phase}-{configuration}.json"
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            op = value["operation"]
            assert value["complete"] and value["configuration"] == configuration and value["phase"] == "town-" + phase
            assert op["source"] == previous; previous = op["output"]
            assert op["initial"]["reserves"] == op["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
            assert {row["town"] for row in op["rows"]["service_sources"]} == {0}
            before, after = [chunks(checked(op[name]))[1] for name in ("before", "output")]
            if phase == "query":
                assert before == after and len(before) == 121
                quote, = op["rows"]["service_quotes"]
                assert quote == dict(town=0, existing=False, admitted=False, native_cost=140625, quote_cost=0, rating_bonus=26)
            elif phase == "credit":
                assert all(before[key] == after[key] for key in before if key != b"PLYR")
                bank, = op["rows"]["service_credit"]
                assert bank["amount"] == 140000 and bank["spendable"] == 685143 and bank["ceiling"] == 5000000
                quote = op["rows"]["service_quotes"][-1]
                assert quote["admitted"] and quote["native_cost"] == quote["quote_cost"] == 140625
                assert op["final"]["company"] == dict(cash=245143, debt=4560000)
            else:
                assert op["rows"]["service_bought"] == [dict(towns=1, invoice=140625, cash=104518, debt=4560000, supplied_stock=False)]
                assert op["final"]["company"] == dict(cash=104518, debt=4560000)
                cities = [TABLE(checked(op[name]), b"CITY")[0] for name in ("before", "output")]
                assert cities[0]["statues"] == 0 and cities[1]["statues"] == 1
            for reference in op["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            snapshots.extend((before, after)); receipts.append(identity(path))
            # Later parsing adds explicit empty service-credit/purchase families.
            # Their original absence represents the same zero action observation.
            normalized = dict(op["rows"])
            for key in ("service_credit", "service_bought"):
                normalized.setdefault(key, [])
            rows.append(normalized)
        physical.append(snapshots); field_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=receipts, output=previous))
    assert field_rows[0] == field_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 2 * len(phases) and comparisons == 240 * len(phases)
    return dict(_documentation=__doc__, complete=True, stage=stage, foundation=identity(foundation_path), configurations=summaries,
                native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                service_actually_bought=stage == "purchase", full_propellant_affordability=False, source=identity(Path(__file__)))


def main():
    """Retain the native service admission or actual purchase without claiming its future income."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("plan", "purchase"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.stage), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
