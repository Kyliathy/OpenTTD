"""Bind genuine mine/gallery characterization, settled retirement and new public producer gates.

@file surface_entry_mine_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.1.
The accepted zero-cost quote is permission to request actual later work, never
excavated mine floor, a funded mine or produced Salt cargo.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify exact native ancestry, original products and both-build whole gallery quotes.

    @param specification Explicit genuine characterization/release/plan and component receipts.
    @return Completed preparation boundary with all additional cutting still unfinished.
    """
    spec = json.loads(specification.read_text())
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, quotes = set(), [], [], []

    def checked(reference):
        """@param reference Exact original artifact identity. @return Its verified original or explicit retained copy."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate nested receipt. Verify every original digest without revisiting closed producer graphs."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    def member(path, configuration):
        """@param path Accepted original JSON. @param configuration Its expected build. @return Verified original operation."""
        value = json.loads((ROOT / path).read_text())
        assert value["complete"] and value["configuration"] == configuration
        walk(value)
        op = value["operation"]
        for reference in op["native_processes"]:
            child = json.loads(checked(reference).read_text())
            assert child["exit_status"] == 0
            walk(child)
            processes.add(reference["path"])
        return op

    # Retained invocation wrappers preserve the original early characterization
    # version whose retirement verifier later gained the explicit kernel action
    # coordinate check. Its already accepted native process is never replaced.
    for entry in spec["configurations"]:
        for path in entry["preparation"].values():
            ref = json.loads((ROOT / path).read_text())["operation"]["retained_harness"]
            aliases[ref["sha256"]] = ROOT / ref["path"]
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        source = json.loads((ROOT / entry["predecessor"]).read_text())["operation"]["output"]
        for phase, path in entry["producers"].items():
            op = member(path, configuration)
            assert op["component_fixture"] and op["genuine_world_predecessor"] is False
            assert (op["cases"], op["assertions"]) == dict(admission=(12, 2646), production=(9, 912), reuse=(1, 16992))[phase]
        physical, previous = [], source
        for phase, path in entry["preparation"].items():
            op = member(path, configuration)
            assert op["source"] == previous and op["simulation_ticks"] == 0
            assert op["final"]["cash"] == 170698 and op["final"]["debt"] == 3500000
            assert op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
            if phase == "release":
                previous = op["output"]
            else:
                assert chunks(checked(op["before"])) == chunks(checked(op["output"]))
            physical.append(chunks(checked(op["output"]))[1])
            if phase == "plan":
                quote, = op["quotes"]
                assert quote["purchase"] == 0 and quote["cells"] == 12 and quote["earth"] == 36 and quote["rock"] == 72
                quotes.append(quote)
        outputs.append(physical)
        summaries.append(dict(configuration=configuration, retired_source=previous, preparation=entry["preparation"], producers=entry["producers"]))
    assert quotes[0] == quotes[1]
    comparisons = 0
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 12 and comparisons == 360
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), configurations=summaries,
                native_processes=12, exact_cross_build_gameplay_comparisons=comparisons, quote=quotes[0],
                cash=170698, debt=3500000, paid_retained_cutter=7, actual_mine_funded=False,
                additional_floor_earned=False, supplied_stock_or_floor=False, source=identity(Path(__file__)))


def main():
    """Publish bounded genuine mine preparation evidence without native reruns."""
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
