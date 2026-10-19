"""Bind real chemical capital, construction and scoped ordinary station losses.

@file surface_entry_chemical_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.2.
This prerequisite has no chemical output: all machinery readiness is earned,
and selected60Salt remains in its original paid Surface train.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_chemical_ledger import salt_ledger


def bind(specification):
    """Verify original actual capital/construction lineage in both configurations.

    @param specification Explicit immutable producer/work receipts and retained-product aliases.
    @return Actual completed empty machinery and original Surface material custody.
    """
    spec = json.loads(specification.read_text(encoding="utf-8"))
    foundation = json.loads((ROOT / spec["plan"]).read_text(encoding="utf-8"))
    assert foundation["complete"] and not foundation["chemical_or_fuel_output_complete"]
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, physical, summaries = set(), [], []

    def checked(reference):
        """@param reference Exact original identity. @return Verified original or explicitly retained artifact."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Verify artifacts without replaying older accepted groups."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    def process_receipt(value):
        """@param value One accepted receipt. Verify original successful native process membership."""
        walk(value)
        for reference in value["operation"]["native_processes"]:
            process = json.loads(checked(reference).read_text(encoding="utf-8"))
            assert process["exit_status"] == 0
            walk(process)
            processes.add(reference["path"])

    # Each actual chain starts at the selected first60Salt checkpoint. Paid
    # machinery construction never imports a ready facility or recipe input.
    for entry in spec["configurations"]:
        previous, = [row["predecessor"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        decay = json.loads((ROOT / entry["station_loss"]).read_text(encoding="utf-8"))
        assert decay["complete"] and decay["configuration"] == entry["configuration"]
        assert decay["operation"]["cases"] == 1 and decay["operation"]["assertions"] == 20
        process_receipt(decay)
        states, earned = [], 0
        for index, name in enumerate(entry["work"]):
            value = json.loads((ROOT / name).read_text(encoding="utf-8"))
            assert value["complete"] and value["configuration"] == entry["configuration"]
            process_receipt(value)
            op = value["operation"]
            assert op["source"] == previous and op["final"]["debt"] == 4650000
            assert op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
            if index == 0:
                assert value["phase"] == "capital" and op["credit"] == 400000 and op["final"]["cash"] == 839142
                assert sum(row["cost"] for row in op["invoices"]) == -350868
            elif index == 1:
                assert value["phase"] == "fund" and op["final"]["cash"] == 114504
                assert not op["factory"]["complete"] and sum(row["cost"] for row in op["invoices"]) == 724638
            else:
                assert value["phase"] == "advance" and op["earned_ticks"] == 3000
                earned += op["earned_ticks"]
            previous = op["output"]
            states.append(chunks(checked(previous))[1])
        assert len(states) == 4 and earned == 6000
        factory = op["factory"]
        salt = salt_ledger(checked(previous))
        assert factory["complete"] and len(factory["cells"]) == 16
        assert not any(factory["inputs"].values()) and not any(factory["outputs"].values())
        assert salt["wagon_held"] == 60 and salt["surface_custody"] and salt["delivered"] == 0
        assert salt["ordinary_station_loss"] == 1 and salt["mine"]["extracted"] == 646 and salt["reserve"] == 23354
        assert op["final"]["cash"] == 92341
        physical.append(states)
        summaries.append(dict(configuration=entry["configuration"], output=previous, work=entry["work"], factory=factory,
                              salt=salt, capital=op["final"]))
    comparisons = 0
    for a, b in zip(*physical, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 10 and comparisons == 480
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), plan=identity(ROOT / spec["plan"]),
                configurations=summaries, native_processes=10, exact_cross_build_gameplay_comparisons=comparisons,
                actual_cutter_recovery=350868, actual_matching_credit=400000, debt=4650000,
                paid_factory=703125, paid_infrastructure=21513, earned_ticks_per_build=6000,
                surface_salt=60, ordinary_unserved_salt_loss=1, chemical_output_complete=False,
                source=identity(Path(__file__)))


def main():
    """Publish genuine completed empty chemical works without claiming chemical output."""
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
