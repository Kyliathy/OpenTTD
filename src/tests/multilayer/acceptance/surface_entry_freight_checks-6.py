"""Bind genuine Surface ore, Steel and Goods production without replaying closed producers.

@file surface_entry_freight_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.
Explicit retained products preserve every actual tested executable/source version.
Fuel, launch entry and the aggregate unassisted orbital chain remain unfinished.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

OBSERVATIONS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def read(path):
    """@param path Exact original JSON. @return Its unchanged decoded membership."""
    return json.loads(path.read_text(encoding="utf-8"))


def bind(specification):
    """Verify actual ancestry, native processes, product origins and deterministic continuation.

    @param specification Explicit accepted setup/drive/restore/replay memberships and original retained aliases.
    @return Bounded physical-production acceptance, with downstream fuel/launch still open.
    """
    spec = read(specification)
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}

    def checked(reference):
        """@param reference Original digest-bound identity. @return Its verified original or explicitly retained copy."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """Verify every original identity in one immediate membership.

        @param value Original nested membership, never recursively inferred historical producer graphs.
        """
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    walk(spec["retained"])
    foundation = read(ROOT / spec["foundation"])
    assert foundation["complete"] and foundation["cargo_or_progress_supplied"] is False
    processes, memberships, all_outputs, final_products = set(), [], [], []

    def operation(membership, configuration):
        """Verify one exact accepted immediate native operation and all original child artifacts.

        @param membership Explicit original phase JSON path.
        @param configuration Expected independent native build.
        @return Original operation and completed physical output identity.
        """
        path = ROOT / membership
        value = read(path)
        assert value["complete"] and value["configuration"] == configuration
        walk(value)
        op = value["operation"] if "operation" in value else value["result"]["process"]
        for reference in op["native_processes"]:
            child = read(checked(reference))
            assert child["exit_status"] == 0
            walk(child)
            processes.add(reference["path"])
        memberships.append(identity(path))
        return value, op

    # Each build must consume its exact immutable ready factory source, then
    # its own paid roads, empty trucks, separate ore intake and productive ticks.
    # A matching same-seed world or a later fixture cannot replace any link.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        ready_factory = read(ROOT / entry["factory"])["operation"]["output"]
        previous, outputs = ready_factory, []
        for key in ("roads", "fleet", "capacity"):
            value, op = operation(entry[key], configuration)
            assert op["source"] == previous and op["simulation_ticks"] == 0 and op["supplied_stock"] is False
            previous = op["output"]
            outputs.append(chunks(checked(previous))[1])
        earned = 0
        for membership in entry["driving"]:
            value, op = operation(membership, configuration)
            assert op["source"] == previous and value["phase"] == "advance"
            earned += value["result"]["ticks"]
            previous = op["output"]
            outputs.append(chunks(checked(previous))[1])
        assert earned == 12000
        products = OBSERVATIONS["observe"](checked(previous))
        assert products["ready"] and products["iron_received"] == products["steel_produced"] == 128
        assert products["steel_received_by_goods"] == 36
        assert products["parked_steel"] == 36 and products["parked_goods"] == 14
        assert products["cash"] == 699555 and products["debt"] == 2000000
        original_output = previous
        value, restored = operation(entry["restored"], configuration)
        assert restored["source"] == original_output and value["result"]["ticks"] == 0
        assert value["result"]["initial"] == value["result"]["final"] == products
        outputs.append(chunks(checked(restored["output"]))[1])
        replay_outputs = []
        for scenario, source in (("original", original_output), ("resaved", restored["output"])):
            value, replay = operation(entry["replay"][scenario], configuration)
            assert value["scenario"] == scenario and replay["source"] == source and replay["earned_ticks"] == 3000
            replay_outputs.append(chunks(checked(replay["output"]))[1])
            outputs.append(replay_outputs[-1])
        assert replay_outputs[0] == replay_outputs[1]
        all_outputs.append(outputs)
        final_products.append(products)
    assert final_products[0] == final_products[1]
    comparisons = 0
    # Ordinary applications retain their declared original clean/modified GLOG
    # revision difference. Every physical/gameplay owner must remain exact,
    # including native map bytes, packets, histories, cash, debt and game UUID.
    for first, second in zip(*all_outputs, strict=True):
        assert first.keys() == second.keys()
        changed = {key for key in first if first[key] != second[key]}
        assert changed <= {b"GLOG"}, changed
        comparisons += len(first) - len(changed)
    assert len(processes) == 20 and comparisons == 1200
    return dict(_documentation=__doc__, complete=True, specification=identity(specification),
                foundation=identity(ROOT / spec["foundation"]), memberships=memberships,
                native_processes=20, exact_cross_configuration_comparisons=1200,
                complete_native_replay_comparisons=242, ordinary_productive_ticks_per_build=12000,
                exact_native_continuation_ticks_per_process=3000, infrastructure_expense=12744, fleet_expense=26433,
                products=final_products[0], cargo_progress_or_capital_granted=False,
                underground_fuel_complete=False, paid_launch_entry_complete=False, aggregate_group15_complete=False,
                source=identity(Path(__file__)))


def main():
    """Publish one fresh exact physical-production index from explicit immediate evidence."""
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
