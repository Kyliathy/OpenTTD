"""Bind genuine original paid assembly stages before dependent workshop actions.

@file surface_entry_gateway_assembly_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQM.
Real SVST/Vehicle/slot receipts justify embodied inputs. Actual remaining
station packets and native company books retain all physical/financial owners.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_assembly_ledger import observe
from surface_gateway_material_ledger import observe as delivery

FOUNDATION = ROOT / "logs/surface-entry-gateway-supply-reload-verification.json"
STEM = "surface-entry-gateway-assembly"


def load(path):
    """@param path Existing immutable accepted receipt. @return Complete receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind):
    """@param kind Completed current pure, rocket purchase/readiness, constructor purchase or both-stored boundary.
    @return Exact original paid lineage, material/slot ownership and both-build gameplay equality.
    """
    allowed = ("plan", "rocket", "rocket-ready", "constructor", "stored"); count = allowed.index(kind)
    phases = ["query", "rocket", "rocket-work", "constructor", "constructor-work"][:count + 1]
    foundation = load(FOUNDATION); summaries, worlds, rows, processes = [], [], [], set()
    # Compare both independently linked configurations at each exact native
    # stage; future workshop progress or copied-world income cannot substitute
    # for either build's actual selected original paid predecessor.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        baseline = load(previous)["operation"]; assert json.loads(json.dumps(delivery(ROOT / baseline["output"]["path"]))) == baseline["final"]
        stages, comparable, references = [], [], []
        for phase in phases:
            path = ROOT / f"logs/{STEM}-{phase}-{configuration}.json"; value = load(path); operation = value["operation"]
            assert value["configuration"] == configuration and operation["receipt"] == identity(previous)
            assert operation["source"] == load(previous)["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                reference = operation[field]; assert identity(ROOT / reference["path"]) == reference
            for reference in operation["sources"] + operation["native_processes"]:
                if identity(ROOT / reference["path"]) == reference:
                    continue
                # The pure/initial rocket boundaries used this exact retained
                # decoder before its completed-rocket queue branch was added.
                # Their empty queue branch is unchanged; no work, paid craft
                # receipt, native binary or other source receives a waiver.
                versions = {"surface_gateway_facility_ledger.py": "facility", "surface_gateway_assembly_ledger.py": "assembly"}
                name = Path(reference["path"]).name
                assert reference["path"] == f"src/tests/multilayer/acceptance/{name}" and name in versions
                old = ROOT / f"test-cache/surface-entry/gateway-assembly-source-versions/before-rocket-queue-fix-{versions[name]}.py"
                assert identity(old)["sha256"] == reference["sha256"]
                assert not operation["rows"]["earned"] and not operation["final"]["facility"].get("queue")
            for reference in operation["native_processes"]:
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                processes.add(reference["path"])
            output = ROOT / operation["output"]["path"]; snapshot = observe(output)
            assert json.loads(json.dumps(snapshot)) == operation["final"]
            stages.append(chunks(output)[1]); comparable.append(operation["rows"]); references.append(identity(path)); previous = path
        if kind == "plan":
            assert snapshot["paid_craft"] == [] and snapshot["totals"] == {"STEL": 36, "GOOD": 50, "PROP": 42}
            budget, = operation["rows"]["budget"]; assert budget["planned"] == 1368000 and budget["target"] == 1370000
        else:
            # The old default must still reject a real consumed/occupied state.
            # Assembly permission is explicit and cannot leak into accepted
            # historical delivery or empty-facility consumers.
            try:
                delivery(output)
            except AssertionError:
                pass
            else:
                raise AssertionError("Original closed delivery contract admitted paid hull consumption")
            models = {craft["model"] for craft in snapshot["paid_craft"]}
            assert models == ({14} if count < 3 else {14, 20})
            if kind in ("rocket-ready", "stored"):
                assert all(craft["placement"] == 2 and craft["remaining"] == 0 for craft in snapshot["paid_craft"])
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company, global_period=40,
                              spendable=company["cash"] + 5000000 - company["debt"], totals=snapshot["totals"], embodied_inputs=snapshot["embodied_inputs"],
                              paid_craft=snapshot["paid_craft"], receipts=references, actual_rows=operation["rows"], service_lives=snapshot["service_lives"]))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_hulls_complete=kind == "stored", actual_launch_complete=False, foundation=identity(FOUNDATION),
                stage=kind, configurations=summaries, native_processes=len(processes), cross_build_gameplay_comparisons=len(phases) * 120, source=identity(Path(__file__)))


def main():
    """Retain only the completed next original assembly boundary before further paid/work actions."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "rocket", "rocket-ready", "constructor", "stored"), required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
