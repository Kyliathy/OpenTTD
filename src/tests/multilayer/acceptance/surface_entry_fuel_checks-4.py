"""Bind the complete native commercial railway and conditional operating forecast.

@file surface_entry_fuel_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.1.
Native quotes preserve all owners. Revenue remains a stated planning assumption;
the following group must actually earn the required money through native ticks.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(receipts):
    """Verify both complete physical plans and the original native payment gate.

    @param receipts Exact independent RelWithDebInfo and Debug query receipts.
    @return Complete pre-purchase arrangement, without any earned income claim.
    """
    foundation_path = ROOT / "logs/surface-entry-fuel-capital-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["producer_assertions_per_build"] == 88
    physical, quotes, processes, summaries = [], [], set(), []

    def walk(value):
        """@param value Current receipt/process tree. Verify every immediate retained artifact identity."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                assert identity(ROOT / value["path"]) == value, value
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Each build must independently admit the entire physical network, fleet
    # and positive conditional forecast in the original genuine chemical world.
    for configuration, receipt in zip(("RelWithDebInfo", "Debug"), receipts, strict=True):
        value = json.loads(receipt.read_text(encoding="utf-8"))
        assert value["complete"] and value["configuration"] == configuration
        walk(value)
        op = value["operation"]
        assert op["selected_income_route"] and not op["supplied_stock_or_capital"] and op["simulation_ticks"] == 0
        before, after = [chunks(ROOT / op[name]["path"])[1] for name in ("before", "output")]
        assert before == after and len(before) == 121
        q = op["quotes"]
        assert len(q["rail_termini"]) == 6 and len(q["rail_lines"]) == 3 and len(q["fleet"]) == 7
        assert q["rail_infrastructure"] == [dict(lines=3, cost=44861, forbid_90=True)]
        assert q["commercial_capital"] == [dict(infrastructure=44861, vehicles=126807, whole=171668, annual_head_running=8682)]
        assert q["rail_bridges"] == [dict(role=2, near=3153, far=3150, type=9, cost=1535, cells=4)]
        bound, = q["planning_bound"]
        assert bound["monthly_each"] == 126 and bound["capture_percent"] == 70 and bound["captured_each"] == 88
        assert bound["transit_periods"] == 32 and bound["annual_net"] > 0 and bound["earned_income"] == 0
        assert bound["goods_captured"] == 123 and bound["coal_captured"] == 75
        assert bound["interest"] == 93000 and bound["contingency"] == 10000 and not bound["infrastructure_maintenance"]
        assert bound["annual_net"] == bound["annual_gross"] - sum(bound[key] for key in ("interest", "new_running", "existing_running", "overhead", "contingency"))
        owners, platform_cells, track_cells = set(), set(), {}
        for terminal in q["rail_termini"]:
            step = 1 if terminal["axis"] == 0 else 64
            cells = {terminal["anchor"] + index * step for index in range(terminal["length"])}
            if terminal["depot"] != 4294967295:
                cells.add(terminal["depot"])
            assert not owners & cells
            owners |= cells
            platform_cells |= cells
        for track in q["rail_tracks"]:
            assert track["tile"] not in owners
            owners.add(track["tile"])
            track_cells[track["tile"]] = track
        for bridge in q["rail_bridges"]:
            assert bridge["near"] not in owners and bridge["far"] not in owners
            step = (bridge["far"] - bridge["near"]) // (bridge["cells"] - 1)
            cells = {bridge["near"] + index * step for index in range(bridge["cells"])}
            assert not cells & platform_cells
            crossed = [track_cells[cell] for cell in cells if cell in track_cells]
            assert crossed and all(row["role"] < bridge["role"] and row["track"] == 1 for row in crossed)
        assert sum(row["cost"] for row in q["rail_tracks"] + q["rail_termini"] + q["rail_bridges"]) == 44861
        assert 181985 + 145769 - 171668 == 156086
        assert 156086 > q["capital"][0]["buffer"]
        for reference in op["native_processes"]:
            process = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert process["exit_status"] == 0
            walk(process)
            processes.add(reference["path"])
        physical.append((before, after))
        quotes.append(q)
        summaries.append(dict(configuration=configuration, receipt=identity(receipt), source=op["source"], output=op["output"], quotes=q))
    assert quotes[0] == quotes[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 2 and comparisons == 240
    return dict(_documentation=__doc__, complete=True, capital_and_payment_boundary=identity(foundation_path), configurations=summaries,
                selected_income_route=True, actual_earned_income=0, infrastructure=44861, vehicles=126807, whole_capital=171668,
                planning_bound=quotes[0]["planning_bound"][0], native_processes=2, exact_cross_build_gameplay_comparisons=comparisons,
                source=identity(Path(__file__)))


def main():
    """Publish the complete quoted arrangement before any actual recovery or spending."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipts", nargs=2, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipts = [(ROOT / path).resolve() for path in args.receipts]
    output = (ROOT / args.output).resolve()
    assert all(path.is_relative_to(ROOT / "logs") and path.is_file() for path in receipts)
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(receipts), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
