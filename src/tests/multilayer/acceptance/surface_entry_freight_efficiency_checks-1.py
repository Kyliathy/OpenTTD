"""Quote actual unchanged original freight timeliness and available-load policies.

@file surface_entry_freight_efficiency_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSB.
Native command tests change no orders, cargo, clock, assets or financial books.
Current loss remains explicit while an actual zero-purchase proposal is priced.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))
observe = PREVIOUS["observe"]


def execute(configuration, receipt, recover_run=None):
    """Run one pure current geometry, order-policy and whole operating quote.

    @param configuration Independently linked actual focused native application.
    @param receipt Current actual partial reserve-recovery original, never a restored-reserve substitute.
    @param recover_run Successful pure invocation to inspect without repeating it.
    @return Exact actual train/cargo/cash state and native zero-cost pickup-change admission.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-chemical-capital-recovery-work"
    previous, = predecessor["operation"]["rows"]["progress"]
    assert previous["partial"] and not previous["restored"] and previous["cash"] == 80157
    files = PREVIOUS["caller_files"]() + [Path(__file__), ROOT / "src/tests/surface_entry_freight_efficiency.h",
        ROOT / "src/economy.cpp", ROOT / "src/train_cmd.cpp", ROOT / "src/ground_vehicle.hpp"]
    operation = native(configuration, "staged-chemical-capital-throughput-query", receipt, observe,
        dict(admission="RECOVERY_ADMISSION", heads="THROUGHPUT_HEAD", platforms="THROUGHPUT_PLATFORM",
             orders="THROUGHPUT_ORDER", cargo="THROUGHPUT_CARGO", proposal="THROUGHPUT_PROPOSAL"),
        files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == predecessor["operation"]["final"]
    assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    rows = operation["rows"]
    admission, = rows["admission"]
    proposal, = rows["proposal"]
    assert admission["funded"] and admission["cash"] == 80157 and admission["target"] <= 80157
    assert admission["working_shortfall"] == 19843 and admission["module_base_shortfall"] == 429843
    assert admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == admission["purchases"] == 0
    assert len(rows["heads"]) == 3 and len(rows["platforms"]) == 6 and len(rows["orders"]) == 3
    assert sum(row["parts"] for row in rows["heads"]) == 35
    assert all(row["train_length"] <= row["tiles"] * 16 and row["overhang"] == 0 for row in rows["platforms"])
    assert all(row["current_load"] in (2, 3) and row["proposed_load"] == 0 and row["command_cost"] == 0 and row["admitted"] for row in rows["orders"])
    assert all(row["receivers_unchanged"] and row["capacity_unchanged"] for row in rows["orders"])
    actual = {(row["vehicle"], row["label"]): (row["capacity"], row["held"]) for row in rows["cargo"]}
    assert actual == PREVIOUS["saved_consists"](ROOT / operation["output"]["path"])
    assert proposal["wagons"] == 32 and proposal["parts"] == 35 and proposal["quoted_order_changes"] == 3
    assert proposal["purchases"] == proposal["sales"] == proposal["coupling"] == proposal["settings_changes"] == proposal["changed_orders"] == 0
    assert not proposal["guaranteed_income"] and not proposal["reserve_restored"] and not proposal["module_capital_complete"]
    return operation


def main():
    """Publish one immutable pure freight-efficiency admission under project logs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase="staged-chemical-capital-throughput-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
