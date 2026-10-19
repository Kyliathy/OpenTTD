"""Bind original chemical producer gates and pure full-layout/capital characterization.

@file surface_entry_chemical_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.1.
Only the initial arrangement is affordable here. The second factory still needs
real later capital recovery; this receipt never claims chemical or fuel output.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Validate exact original gates, sources, native processes and useful route terms.

    @param specification Immutable explicit both-build membership and retained-product aliases.
    @return Verified initial producer arrangement, leaving payment and output open.
    """
    spec = json.loads(specification.read_text(encoding="utf-8"))
    foundation = json.loads((ROOT / spec["salt"]).read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["surface_salt"] == 60
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, physical, summaries = set(), [], []

    def checked(reference):
        """@param reference Exact original identity. @return Verified original or explicitly retained artifact."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt or process tree. Verify every original artifact without replaying older groups."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Each independently linked configuration must pass the actual producer
    # gates and preserve the authentic world's entire query boundary.
    for entry in spec["configurations"]:
        receipts = [json.loads((ROOT / entry[name]).read_text(encoding="utf-8")) for name in ("producers", "layout")]
        for value in receipts:
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            for reference in value["operation"]["native_processes"]:
                child = json.loads(checked(reference).read_text(encoding="utf-8"))
                assert child["exit_status"] == 0
                walk(child)
                processes.add(reference["path"])
        gates, op = [value["operation"] for value in receipts]
        assert gates["cases"] == 11 and gates["assertions"] == 1517 and not gates["full_regression"]
        predecessor, = [row["output"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        assert op["source"] == predecessor and op["unchanged_native_families"] == 121 and op["simulation_ticks"] == 0
        before, after = [chunks(checked(op[name]))[1] for name in ("before", "output")]
        assert before == after
        q = op["quotes"]
        world, = q["world"]
        assert world["type"] == 25 and world["multiple_per_town"] and world["ceiling"] == 5000000
        assert world["cash"] == 88274 and world["debt"] == 4250000
        first, = [row for row in q["sites"] if row["anchor"] == 1566]
        second, = [row for row in q["sites"] if row["anchor"] == 1639]
        assert first["salt_station_catchment"] and not second["salt_station_catchment"]
        assert first["x"] + 4 <= second["x"] and first["cost"] == second["cost"] == 703125
        oil, = q["oil"]
        assert oil["industry"] == 8 and oil["anchor"] == 2738 and oil["founder"] == 16
        assert all(any(row["cargo"] == label and row["kind"] == 1 and row["engine"] == 132 and row["capacity"] == 21
                       for row in q["carriers"]) for label in ("OIL_", "CHEM", "PROP"))
        infra, = q["infrastructure"]
        capital, = q["capital"]
        assert infra["bays"] == len(q["bays"]) == 5 and infra["road_cells"] == 93 and infra["cost"] == 21513
        assert capital["initial_total"] == capital["first"] + capital["infrastructure"] + capital["carriers"] == 738253
        assert capital["cutter_recovery"] == 350868 and capital["buffer"] == 100000 and capital["credit"] == 400000
        assert world["cash"] + capital["cutter_recovery"] + capital["credit"] - capital["initial_total"] == 100889
        assert world["debt"] + capital["credit"] + capital["future_headroom"] == world["ceiling"]
        physical.append((before, after))
        summaries.append(dict(configuration=entry["configuration"], predecessor=predecessor, quotes=q, pure_families=121))
    comparisons = 0
    for a, b in zip(*physical, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 4 and comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), salt=identity(ROOT / spec["salt"]),
                configurations=summaries, native_processes=4, producer_cases_per_build=11, producer_assertions_per_build=1517,
                exact_cross_build_gameplay_comparisons=comparisons, first=1566, second=1639, initial_capital=738253,
                proposed_cutter_recovery=350868, proposed_matching_credit=400000, initial_buffer=100000,
                supplied_stock_or_capital=False, chemical_or_fuel_output_complete=False, source=identity(Path(__file__)))


def main():
    """Publish one complete actual initial-arrangement characterization receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    specification, target = [(ROOT / path).resolve() for path in (args.specification, args.output)]
    assert specification.is_relative_to(ROOT / "logs") and specification.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind(specification), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
