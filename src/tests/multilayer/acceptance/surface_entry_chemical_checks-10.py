"""Bind actual original ingredient transport and genuine native chemical output.

@file surface_entry_chemical_checks-10.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.2.
Original paid construction and real mined Salt are the only predecessor. The
recipe consumes actual delivered Oil/Salt; physical depot collection/reload follows.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_chemical_ledger import chemical_ledger, salt_ledger


def bind(specification):
    """Verify every actual purchase/order/work boundary in the selected chemical lineage.

    @param specification Explicit immutable both-build membership and retained source/product aliases.
    @return Actual chemical output acceptance, leaving useful collection/reload open.
    """
    spec = json.loads(specification.read_text(encoding="utf-8"))
    foundation = json.loads((ROOT / spec["construction"]).read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["surface_salt"] == 60 and not foundation["chemical_output_complete"]
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, states, summaries = set(), [], []

    def checked(reference):
        """@param reference Exact original identity. @return Verified original or explicitly retained source/product."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate original artifact tree. Verify every identity without replaying accepted predecessor groups."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Separate actual original order edits, real depot arrival, empty paid
    # purchases and earned input/work ticks. No packet or vehicle pose is installed.
    for entry in spec["configurations"]:
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        physical, earned = [], 0
        for index, name in enumerate(entry["work"]):
            value = json.loads((ROOT / name).read_text(encoding="utf-8"))
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            op = value["operation"]
            assert op["source"] == previous and op["final"]["debt"] == 4650000
            for reference in op["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8"))
                assert process["exit_status"] == 0
                walk(process)
                processes.add(reference["path"])
            if index == 0:
                assert value["phase"] == "recall" and op["earned_ticks"] == 0
            elif index == 1:
                assert value["phase"] == "settle" and op["earned_ticks"] == 86
                assert sum(row["cost"] for row in op["invoices"]) == -139602
            elif index == 2:
                assert value["phase"] == "fleet" and sum(row["cost"] for row in op["invoices"]) == 13615
                assert len(op["final"]["tankers"]) == 3 and not any(row["held"] for row in op["final"]["tankers"])
            else:
                assert value["phase"] == "drive" and op["earned_ticks"] == 3000
            earned += op["earned_ticks"]
            previous = op["output"]
            physical.append(chunks(checked(previous))[1])
        assert len(physical) == 5 and earned == 6086
        chemicals, salt = chemical_ledger(checked(previous)), salt_ledger(checked(previous))
        assert chemicals["produced"] == chemicals["dispatched"] == chemicals["reserve"]["held"] == 14
        assert chemicals["accepted"]["SALT"] == salt["delivered"] == 60 and chemicals["accepted"]["OIL_"] == 21
        assert chemicals["factory"]["inputs"]["SALT"] == 46 and chemicals["factory"]["inputs"]["OIL_"] == 7
        assert chemicals["ordinary_station_loss"] == 0 and salt["wagon_held"] == 0
        assert op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
        assert op["final"]["cash"] == 196458
        states.append(physical)
        summaries.append(dict(configuration=entry["configuration"], output=previous, work=entry["work"],
                              chemicals=chemicals, salt=salt, products=op["final"]))
    comparisons = 0
    for a, b in zip(*states, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 10 and comparisons == 600
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), construction=identity(ROOT / spec["construction"]),
                configurations=summaries, native_processes=10, exact_cross_build_gameplay_comparisons=comparisons,
                actual_hauler_recovery=139602, actual_empty_tanker_invoice=13615, earned_ticks_per_build=6086,
                actual_salt_delivered=60, actual_oil_delivered=21, chemicals_produced=14,
                supplied_stock_or_pose=False, useful_collection_or_reload_complete=False, source=identity(Path(__file__)))


def main():
    """Publish genuine native chemical output from actual original ingredient transport."""
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
