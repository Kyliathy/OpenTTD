"""Bind each sequential real regional-service boundary across both native builds.

@file surface_entry_regional_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQA.
Each published stage follows its exact predecessor and compares every gameplay
family, without counting a query, replay or projected fare as earned capital.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives

PHASES = ("civic-query", "query", "infrastructure", "buy", "earn")


def bind(phase):
    """@param phase The one completed next boundary. @return Exact current native products, funds and cross-build gameplay evidence."""
    index = PHASES.index(phase); operations, summaries, worlds, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / f"logs/surface-entry-regional-{phase}-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
        operation = value["operation"]
        previous = ROOT / (f"logs/surface-entry-regional-{PHASES[index - 1]}-{configuration}.json" if index else
                           f"logs/surface-entry-coordinated-continued-{configuration}-original.json")
        predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
        assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"]
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            assert identity(ROOT / operation[field]["path"]) == operation[field]
        for reference in operation["sources"] + operation["native_processes"]:
            assert identity(ROOT / reference["path"]) == reference
        processes.update(reference["path"] for reference in operation["native_processes"])
        source = ROOT / operation["output"]["path"]; snapshot = observe(source)
        assert json.loads(json.dumps(snapshot)) == operation["final"]
        company = snapshot["company"]; service = lives(source)
        assert min(row["remaining"] for row in service) > 732
        if phase == "civic-query":
            civic, = operation["rows"]["civic"]; assert not civic["admitted"] and civic["trees"] < civic["needed"]
        elif phase == "query":
            civic, = predecessor["rows"]["civic"]; assert not civic["admitted"]
        elif phase == "infrastructure":
            assert predecessor["rows"]["layout"] == operation["rows"]["layout"] and predecessor["rows"]["roads"] == operation["rows"]["roads"]
            assert predecessor["rows"]["stops"] == operation["rows"]["stops"] and predecessor["rows"]["plan"] == operation["rows"]["plan"]
        elif phase == "buy":
            quoted, = predecessor["rows"]["plan"]; paid, = operation["rows"]["purchase"]
            assert paid["cost"] == 3 * quoted["unit"] and len(service) == 11
        operations.append(operation["rows"]); worlds.append(chunks(source)[1])
        summaries.append(dict(configuration=configuration, receipt=identity(path), output=operation["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], service_lives=service,
                              regional_buses=snapshot["regional_buses"], actual_rows=operation["rows"]))
    assert operations[0] == operations[1] and len(worlds[0]) == len(worlds[1]) == 121 and worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, phase=phase, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=120, source=identity(Path(__file__)), actual_capital_complete=False)


def main():
    """Publish a single completed regional task boundary without repeating native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    value = bind(args.phase); output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
