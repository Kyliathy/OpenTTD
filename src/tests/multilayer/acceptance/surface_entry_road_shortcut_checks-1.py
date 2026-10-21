"""Admit only a complete native dry-road shortcut from current genuine cash.

@file surface_entry_road_shortcut_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGL.
Native reciprocal edges and accepted full-route quotes remain observations;
temporary route mathematics supplies no map tiles, rights, cargo or money.
"""
import argparse
import heapq
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_intertown_checks-1.py"))
FREIGHT = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-road-shortcut-query"


def distances(graph, source):
    """@param graph Actual reciprocal road adjacencies or explicitly temporary proposed additions.
    @param source Existing native endpoint. @return Shortest weighted physical lengths, never predicted time or fare.
    """
    result, queue = {source: 0}, [(0, source)]
    # Existing bridges/tunnels retain their physical span length; ordinary
    # road edges cost one. This mathematics cannot modify the original map.
    while queue:
        current, tile = heapq.heappop(queue)
        if current != result[tile]:
            continue
        for neighbor, length in graph[tile].items():
            proposed = current + length
            if proposed < result.get(neighbor, float("inf")):
                result[neighbor] = proposed
                heapq.heappush(queue, (proposed, neighbor))
    return result


def execute(configuration, receipt, recover_run=None):
    """Run one pure static-route, native authority and whole startup-cost admission.

    @param configuration Independently linked focused native application.
    @param receipt Exact immediately preceding actual intertown native original.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged original owners and complete funded route or its literal refusal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASES"]["work"]
    assert previous["operation"]["final"]["company"]["cash"] == 73667
    files = FREIGHT["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_intertown_checks-1.py",
        DIRECTORY / "surface_entry_farm_cycle_checks-1.py", DIRECTORY / "surface_entry_farm_rebalance_checks-1.py",
        ROOT / "src/tests/surface_entry_road_shortcut.h", ROOT / "src/tests/surface_entry_intertown.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_farm_upgrade.h",
        ROOT / "src/tests/surface_entry_module_capital.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/road_cmd.cpp", ROOT / "src/road_map.h", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="SHORTCUT_SOURCE", search="SHORTCUT_SEARCH", edges="SHORTCUT_EDGE", proposal="SHORTCUT_PROPOSAL",
             corridor="SHORTCUT_TILE", admission="SHORTCUT_ADMISSION"), files, recover_run=recover_run, chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    search, = operation["rows"]["search"]
    proposal, = operation["rows"]["proposal"]
    assert source["vehicle"] == 9 and source["held"] == 5
    assert source["one_way"] == source["purchases"] == source["work"] == 0
    assert proposal["strict_drag"] and not proposal["paid"] and not proposal["changed_map"]
    graph = {}
    # Actual native unique edges reconstruct the current static road graph.
    # Do not replace a refused physical edge with coordinate proximity.
    for edge in operation["rows"]["edges"]:
        first, last = edge["first"], edge["last"]
        length = edge["length"]
        assert first < last and length > 0 and last not in graph.get(first, {})
        graph.setdefault(first, {})[last] = length
        graph.setdefault(last, {})[first] = length
    assert len(graph) == source["nodes"] == search["nodes"]
    assert sum(edge["length"] > 1 for edge in operation["rows"]["edges"]) == source["bridge_jumps"]
    stations = TABLE(output, b"STNN")
    bus_source, goods_source = [stations[key]["normal[0].base[0].xy"] for key in (17, 19)]
    bus, goods = distances(graph, bus_source), distances(graph, goods_source)
    assert len(bus) == len(goods) == len(graph)
    assert bus[1055] == search["old_bus"] and goods[731] == search["old_goods"]
    if proposal["found"]:
        assert proposal["old_bus"] == bus[1055] and proposal["old_goods"] == goods[731]
        assert proposal["saving"] == proposal["old_bus"] - proposal["bus"] + 2 * (proposal["old_goods"] - proposal["goods"]) > 0
        corridor = operation["rows"]["corridor"]
        assert [row["index"] for row in corridor] == list(range(proposal["length"] + 1))
        route = [row["tile"] for row in corridor]
        assert 2 <= proposal["length"] <= 48 and len(route) == len(set(route))
        assert route[0] == proposal["first"] and route[-1] == proposal["last"]
        tentative = {key: dict(value) for key, value in graph.items()}
        # Proposed dry-road additions remain isolated mathematics. Existing
        # intermediate junctions may improve on the native endpoint-only bound.
        for first, last in zip(route, route[1:]):
            tentative.setdefault(first, {})[last] = 1
            tentative.setdefault(last, {})[first] = 1
        assert distances(tentative, bus_source)[1055] <= proposal["bus"]
        assert distances(tentative, goods_source)[731] <= proposal["goods"]
        cases = operation["rows"]["admission"]
        assert [row["reserve"] for row in cases] == [100000, 40000] and not cases[0]["funded"]
        for row in cases:
            assert row["ticks"] == 2000 and row["invoice"] == proposal["invoice"]
            assert row["target"] == row["reserve"] + row["invoice"] + row["operating"] and row["cash"] == 73667
            assert row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        operation["road_producer_admitted"] = bool(cases[1]["funded"])
    else:
        assert not operation["rows"]["admission"] and not search["funded_candidate"]
        operation["road_producer_admitted"] = False
    return operation


def main():
    """Publish one immutable pure native shortcut admission without construction or earned work."""
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
                                     phase=PHASE, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
