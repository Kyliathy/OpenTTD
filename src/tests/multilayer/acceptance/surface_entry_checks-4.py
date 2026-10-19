"""Bind the authentic new-game and pure route baseline without native reruns.

@file surface_entry_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BB.3.
This accepts the starting-world prerequisite only. Factories, freight, fuel,
paid terrain and launch construction remain their own unfinished producers.
"""

import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def read(path):
    """@param path Explicit original JSON. @return Its unchanged parsed membership."""
    return json.loads(path.read_text(encoding="utf-8"))


def verify(reference):
    """@param reference Original exact repository path/digest. @return Its verified path."""
    path = ROOT / reference["path"]
    assert identity(path) == reference, path
    return path


def bind(specification):
    """Validate original source/products/processes and both-build zero-grant owners.

    @param specification Exact canonical generation/query/ordinary memberships.
    @return Complete authentic baseline evidence, with downstream work explicitly open.
    """
    spec = read(specification)
    processes = set()

    def child(reference):
        """Verify one actual native child and all its observed immutable outputs.

        @param reference Runner-generated original process report.
        """
        value = read(verify(reference))
        assert value["exit_status"] == 0
        verify(value["executable"])
        for item in value["inputs"] + value["artifacts"]:
            verify(item)
        processes.add(reference["path"])

    original = read(verify(read(ROOT / spec["original"])["report"]))
    assert original["complete"] and original["money"] == original["loan"] == 100000
    assert original["cash_granted_by_harness"] == original["packet_count"] == original["company_vehicle_count"] == 0
    for reference in (original["application"], original["original"], original["config"], original["startup_script"], original["terminal"], *original["sources"]):
        verify(reference)
    for reference in original["native_processes"]:
        child(reference)
    entries = []
    snapshots = []
    # Each application must restore the same selected real game UUID and bank
    # authority; independently generated same-seed worlds cannot substitute.
    for entry in spec["configurations"]:
        queries = read(verify(read(ROOT / entry["queries"])["report"]))
        baseline = read(ROOT / entry["baseline"])
        assert queries["configuration"] == baseline["configuration"] == entry["configuration"]
        assert queries["source"] == baseline["source"] == original["original"]
        assert baseline["complete"] and baseline["exact_second_resave"]
        for reference in (queries["application"], queries["terminal"], *queries["sources"], baseline["harness"]):
            verify(reference)
        for reference in queries["native_processes"]:
            child(reference)
        for reference in queries["snapshots"]:
            verify(reference)
        assert chunks(verify(queries["snapshots"][0])) == chunks(verify(queries["snapshots"][1]))
        for operation in baseline["processes"]:
            assert operation["money"] == operation["loan"] == 100000
            assert operation["packet_count"] == operation["company_vehicle_count"] == 0
            for key in ("source", "output", "application", "terminal"):
                verify(operation[key])
            for reference in operation["native_processes"]:
                child(reference)
        entries.append(dict(configuration=entry["configuration"], queries=identity(ROOT / entry["queries"]),
                            baseline=identity(ROOT / entry["baseline"]), factories=queries["factories"],
                            launch=queries["launch"], fuel=queries["fuel"]))
        snapshots.append(chunks(verify(baseline["processes"][-1]["output"]))[1])
    assert entries[0]["factories"] == entries[1]["factories"]
    assert entries[0]["launch"] == entries[1]["launch"] and entries[0]["fuel"] == entries[1]["fuel"]
    # The ordinary Debug application appends only its qualified same-commit
    # clean/modified revision record. All120other native families are exact.
    assert snapshots[0].keys() == snapshots[1].keys()
    changed = [key for key in snapshots[0] if snapshots[0][key] != snapshots[1][key]]
    assert changed == [b"GLOG"], changed
    assert len(processes) == 7
    return dict(_documentation=__doc__, complete=True, specification=identity(specification),
                original=original["original"], configurations=entries, native_processes=len(processes),
                cross_configuration_owner_families=len(snapshots[0]) - 1,
                qualified_metadata_difference="GLOG same-commit clean/modified build revision only",
                initial_cash=100000, initial_bank_debt=100000, maximum_bank_setting=5000000,
                cash_or_stock_supplied_by_harness=False, original_inputs_preserved=True,
                factories_funded=False, launch_entry_complete=False, unassisted_chain_complete=False,
                sources=[identity(Path(__file__))])


def main():
    """Publish one fresh baseline index from explicit immediate original receipts."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = bind((ROOT / args.specification).resolve())
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
