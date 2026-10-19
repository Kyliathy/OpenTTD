"""Bind actual paid Surface factory construction before the physical freight group.

@file surface_entry_checks-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BC.
This revalidates exact original memberships without rerunning closed native work.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def read(path):
    """@param path Explicit original JSON. @return Its parsed unchanged membership."""
    return json.loads(path.read_text(encoding="utf-8"))


def bind(specification):
    """Validate original invoices, retained tested products and exact construction ancestry.

    @param specification Explicit accepted bank/funding/tick/readiness memberships and aliases.
    @return Complete paid-factory prerequisite with physical freight still open.
    """
    spec = read(specification)
    aliases = {item["sha256"]: ROOT / item["path"] for item in spec["retained"]}

    def checked(reference):
        """@param reference Exact original identity. @return Its verified original or explicit retained copy."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    for reference in spec["retained"]:
        checked(reference)
    credit = read(ROOT / spec["credit"])
    assert credit["complete"] and credit["cash"] == credit["debt"] == 2000000
    memberships, products, processes = [], [], set()
    # Each phase must consume the exact previous output, never a same-seed
    # substitute or a second fixture. All original native artifacts remain
    # digest-bound even where later focused sources relink the same target.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        bank = read(checked(next(row["membership"] for row in credit["configurations"]
                                if read(checked(row["membership"]))["configuration"] == configuration)))
        previous = bank["native"]["snapshots"][-1]
        outputs = []
        ticks = 0
        for phase, membership_path in zip(("fund", "advance", "advance", "ready"), entry["memberships"], strict=True):
            membership = ROOT / membership_path
            value = read(membership)
            assert value["complete"] and value["phase"] == phase and value["configuration"] == configuration
            operation = value["operation"]
            assert operation["source"] == previous
            for reference in value["sources"]:
                checked(reference)
            for key in ("source", "output", "application", "terminal"):
                checked(operation[key])
            for reference in operation.get("snapshots", []):
                checked(reference)
            for reference in operation["native_processes"]:
                child = read(checked(reference))
                assert child["exit_status"] == 0
                checked(child["executable"])
                for artifact in child["inputs"] + child["artifacts"]:
                    checked(artifact)
                processes.add(reference["path"])
            if phase == "fund":
                assert [row["invoice"] for row in operation["invoices"]] == [629882, 609375]
            if phase == "advance":
                ticks += operation["ticks"]
                assert operation["debt"] == 2000000
            previous = operation["output"]
            outputs.append(chunks(checked(previous))[1])
            memberships.append(identity(membership))
        assert ticks == 6000
        products.append(outputs)
    comparisons = 0
    # Ordinary applications have their declared clean/modified revision rows;
    # every actual gameplay family must match, including all tile bytes, dates,
    # loans, expenses, histories, native cargo and naturally generated owners.
    for first, second in zip(*products, strict=True):
        assert first.keys() == second.keys()
        changed = {key for key in first if first[key] != second[key]}
        assert changed <= {b"GLOG"}, changed
        comparisons += len(first) - len(changed)
    assert len(processes) == 8
    return dict(_documentation=__doc__, complete=True, specification=identity(specification),
                credit=identity(ROOT / spec["credit"]), memberships=memberships,
                native_factory_processes=len(processes), native_credit_and_factory_processes=12,
                exact_cross_configuration_comparisons=comparisons, ordinary_ticks_per_build=6000,
                factory_invoices=[629882, 609375], completed_factory_cells=26,
                cargo_or_progress_supplied=False, physical_freight_complete=False,
                genuine_fuel_or_launch_entry_complete=False, source=identity(Path(__file__)))


def main():
    """Publish a fresh bounded factory acceptance index from explicit original evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind((ROOT / args.specification).resolve()), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
