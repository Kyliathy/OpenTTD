"""Bind actual paid empty train and genuine finite Salt haulage to Surface depot custody.

@file surface_entry_salt_mine_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.3.
Native command invoices, original packets and real train poses prove the
transport boundary. Ordinary reload/continuation remains a subsequent task.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify exact completed-mine ancestry and real same-world Salt delivery to a Surface depot.

    @param specification Explicit accepted construction foundation, fleet/haulage receipts and retained products.
    @return Completed physical Salt-custody boundary with reload/fuel still open.
    """
    spec = json.loads(specification.read_text())
    foundation = json.loads((ROOT / spec["construction"]).read_text())
    assert foundation["complete"] and foundation["mined_salt"] == 159
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons = set(), [], [], 0

    def checked(reference):
        """@param reference Exact original artifact. @return Its verified original or retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate original receipt. Verify every artifact without historical graph replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Only the actual completed mine's empty purchase and earned native
    # movement can produce this physical60Salt Surface custody checkpoint.
    # Failed setup/route trials never enter the selected original chain.
    for entry in spec["configurations"]:
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        physical, earned = [], 0
        for index, path in enumerate(entry["work"]):
            value = json.loads((ROOT / path).read_text())
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            op = value["operation"]
            assert op["source"] == previous
            for reference in op["native_processes"]:
                child = json.loads(checked(reference).read_text())
                assert child["exit_status"] == 0
                walk(child)
                processes.add(reference["path"])
            if index == 0:
                assert value["phase"] == "fleet" and op["initial"]["cash"] - op["final"]["cash"] == 164559
                assert op["native"]["wagon"]["held"] == 0
            earned += op["earned_ticks"]
            previous = op["output"]
            physical.append(chunks(checked(previous))[1])
        native = op["native"]
        assert earned == 3000 and native["surface_custody"] and native["wagon"]["held"] == 60
        assert native["wagon"]["tile"] == 1373 and native["wagon"]["track"] == 128
        assert native["mine"]["reserve"] == 23679 and native["mine"]["mine"]["extracted"] == 321
        assert op["final"]["debt"] == 4250000 and op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
        outputs.append(physical)
        summaries.append(dict(configuration=entry["configuration"], output=previous, work=entry["work"], custody=native, products=op["final"]))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 4 and comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), construction=identity(ROOT / spec["construction"]),
                configurations=summaries, native_processes=4, exact_cross_build_gameplay_comparisons=comparisons,
                paid_train=164559, capacity=60, earned_ticks_per_build=3000, surface_salt=60, salt_source_industry=14,
                original_remaining_reserve=23679, supplied_stock_or_pose=False, reload_or_fuel_complete=False, source=identity(Path(__file__)))


def main():
    """Publish real Surface Salt custody from exact already completed native work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    specification, target = [(ROOT / item).resolve() for item in (args.specification, args.output)]
    assert specification.is_relative_to(ROOT) and specification.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind(specification), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
