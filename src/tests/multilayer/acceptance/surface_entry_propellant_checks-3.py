"""Bind focused PCHM producers and the complete unchanged genuine-world arrangement.

@file surface_entry_propellant_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.1.
Only actual original capital and external chemical custody may fund production.
No component fixture, quoted output or temporary depot order counts as delivered.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]


def bind():
    """@return Both-build actual PCHM selection, pure native quotes and exact material/financial predecessor proof."""
    foundation_path = ROOT / "logs/surface-entry-income-capital-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["full_propellant_affordability"]
    checked_ids, processes, physical, quotes, summaries = set(), set(), [], [], []

    def checked(reference):
        """@param reference Exact immediate source/process artifact. @return Verified repository-local original path."""
        path = ROOT / reference["path"]; key = reference["path"], reference["sha256"]
        if key not in checked_ids:
            assert identity(path) == reference, path
            checked_ids.add(key)
        return path

    def walk(value):
        """@param value Immediate receipt tree. Validate native membership without replaying accepted financial ancestors."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    for configuration in ("RelWithDebInfo", "Debug"):
        paths = [ROOT / f"logs/surface-entry-propellant-{name}-{configuration}.json" for name in ("producers", "query")]
        values = [json.loads(path.read_text(encoding="utf-8")) for path in paths]
        for value in values:
            walk(value); assert value["complete"] and value["configuration"] == configuration
            for reference in value["operation"]["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
        producer, operation = [value["operation"] for value in values]
        assert producer["cases"] == 2 and producer["assertions"] == 573 and producer["component_fixture"]
        source, = [entry["output"] for entry in foundation["configurations"] if entry["configuration"] == configuration]
        assert operation["source"] == source and operation["unchanged_native_families"] == 121
        original, before, output = [chunks(checked(operation[name]))[1] for name in ("source", "before", "output")]
        assert original.keys() == before.keys() and all(original[key] == before[key] for key in original if key != b"GLOG")
        if original[b"GLOG"] != before[b"GLOG"]:
            REVISION(checked(source), checked(operation["before"]))
        assert before == output and operation["initial"] == operation["final"]
        plan, = operation["quotes"]["plan"]
        assert plan["factory"] == 703125 and plan["engine"] == 134 and plan["tanker"] == 6234 and plan["capacity"] == 27
        assert plan["credit"] == 640000 and plan["debt"] + plan["credit"] == plan["ceiling"] == 5000000
        assert plan["requirement"] == 809359 and plan["expected_product"] == 20 and not plan["full_load"]
        assert [row["crossings"] for row in operation["quotes"]["routes"]] == [2, 2]
        assert len(operation["quotes"]["traffic"]) == 3 and all(row["halt_admitted"] for row in operation["quotes"]["traffic"])
        physical.append((before, output)); quotes.append(operation["quotes"])
        summaries.append(dict(configuration=configuration, producers=identity(paths[0]), query=identity(paths[1]),
                              source=source, output=operation["output"], arrangement=operation["quotes"]))
    assert quotes[0] == quotes[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 4 and comparisons == 240
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=4, exact_cross_build_gameplay_comparisons=240, pure_native_families=121,
                actual_factory_or_propellant_complete=False, source=identity(Path(__file__)))


def main():
    """Publish complete legal quotes before any matching credit, factory, carrier or chemical movement."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
