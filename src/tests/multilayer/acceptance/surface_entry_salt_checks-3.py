"""Bind pure authentic Salt access characterization and focused public producer gates.

@file surface_entry_salt_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.1.
No fixture balance/floor/vehicle is admitted as genuine game ancestry. Original
finite Salt, Surface products and actual bank authority remain unchanged.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def read(path):
    """@param path Explicit original JSON. @return Its unchanged parsed membership."""
    return json.loads(path.read_text(encoding="utf-8"))


def bind(specification):
    """Verify exact original query inputs, retained native products and actual component gates.

    @param specification Explicit accepted query/producer memberships and retained aliases.
    @return Complete pure characterization prerequisite, with all paid entry work still open.
    """
    spec = read(specification)
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes = set()

    def checked(reference):
        """@param reference Original digest-bound identity. @return Its verified original or explicit retained copy."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Nested original immediate evidence. Verify every original artifact identity without historical graph traversal."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    def membership(path):
        """@param path Explicit actual query or component membership. @return Its verified original JSON."""
        value = read(ROOT / path)
        assert value["complete"]
        walk(value)
        for reference in value["operation"]["native_processes"]:
            child = read(checked(reference))
            assert child["exit_status"] == 0
            walk(child)
            processes.add(reference["path"])
        return value

    walk(spec["retained"])
    queries, summaries = [], []
    # Each build must inspect its exact ordinary paused productive game. The
    # separate component fixtures validate public boundaries only; their
    # prepared companies/maps may never enter this game's physical ancestry.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        original = read(ROOT / entry["source_membership"])["result"]["process"]["output"]
        query = membership(entry["query"])
        op = query["operation"]
        assert query["configuration"] == configuration and op["source"] == original
        assert op["original_products_preserved"] and op["supplied_floor_or_capital"] is False
        assert chunks(checked(op["before"])) == chunks(checked(op["output"]))
        queries.append(op)
        counts = []
        for phase, path in entry["producers"].items():
            producer = membership(path)
            assert producer["configuration"] == configuration and producer["phase"] == phase
            assert producer["operation"]["component_fixture"] and producer["operation"]["genuine_world_predecessor"] is False
            expected = (11, 1242) if phase == "portal-tbm" else (13, 33313)
            actual = producer["operation"]["cases"], producer["operation"]["assertions"]
            assert actual == expected
            counts.append(dict(phase=phase, cases=actual[0], assertions=actual[1]))
        summaries.append(dict(configuration=configuration, query=identity(ROOT / entry["query"]),
                              source=original, producer_counts=counts))
    for key in ("deposits", "portals", "receivers", "haulage", "service", "published_machinery_terms"):
        assert queries[0][key] == queries[1][key], key
    first, second = [chunks(checked(value["output"]))[1] for value in queries]
    assert first.keys() == second.keys()
    assert {key for key in first if first[key] != second[key]} == {b"GLOG"}
    assert len(processes) == 6
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), configurations=summaries,
                native_processes=6, unchanged_native_families_per_query=len(first), exact_cross_configuration_gameplay_families=len(first) - 1,
                deposits=queries[0]["deposits"], example_portals=queries[0]["portals"], example_receivers=queries[0]["receivers"],
                haulage=queries[0]["haulage"], service=queries[0]["service"], machinery=queries[0]["published_machinery_terms"],
                supplied_floor_or_capital=False, additional_bank_credit_acquired=False, paid_underground_access_complete=False,
                source=identity(Path(__file__)))


def main():
    """Publish one fresh pure-characterization/producer index without native reruns."""
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
