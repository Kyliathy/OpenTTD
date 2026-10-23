"""Seal current target closure while preserving the master's two explicit human gates.

@file target_closure_documentation.py
@see r2mlvs3mp-master-plan.md12.1-12.6 and both target Completion Conditions.
Earlier document/native receipts stay dated;fresh condition observations bind current observers.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_closure_documentation import git, added_markdown, verify_links, verify_python, PLANS
from ground_performance_acceptance import strict_reference

TARGET = ("logs/acceptance/target-completion/20261020-native-01/target-completion-verification.json", "bedac682c11c0c0187afada549718c5f08602f783a5a3a3506f0bb39d68472e2")
INVENTORY = ("logs/acceptance/fixture-retention/20261020-inventory-01/fixture-retention-inventory.json", "76fe933adf33d518455f36c674cacd64254fbff6c4d1c5a010f0050c69c4483a")


def audit(output):
    """Verify actual complete runtime targets,all retained inputs and still-unfulfilled human requirements."""
    receipts = {}
    for name, (path, sha) in (("runtime-conditions", TARGET), ("retention-preparation", INVENTORY)):
        reference = identity(ROOT / path)
        assert reference["sha256"] == sha
        receipts[name] = {"reference": reference, "actual": json.loads(strict_reference(reference).read_text(encoding="utf-8"))}
    target, inventory = (receipts[name]["actual"] for name in ("runtime-conditions", "retention-preparation"))
    assert target["accepted_runtime_conditions"] is True and target["native_cases"] == 116 and target["isolated_invocations"] == 42
    for row in (*target["fresh_condition_records"], *target["current_original370_compatibility"]):
        actual = row["actual"]
        assert actual["children_reaped"] is True and actual["failure"] is None and actual["native"]["exit_status"] == 0
        for reference in (row["report"], actual["controller"], actual["binary"], actual["config"], actual["memory_samples"], actual["native"]["log"], *actual["sources"]):
            strict_reference(reference)
    for reference in target["closed_integration_receipts"].values():
        strict_reference(reference)
    performance = json.loads(strict_reference(target["closed_integration_receipts"]["performance"]).read_text(encoding="utf-8"))
    for reference in performance["current_production_and_physical_artifacts"]:
        strict_reference(reference)
    assert inventory["deleted_files"] == [] and inventory["haulage_scope_files"] == 1070
    for row in inventory["records"]:
        strict_reference(row["file"])
    pending, counts = {}, {}
    expected = {"r2mlvs3mp-master-plan.md": {"11.7", "12.6"}, "r2mlvs3-sealed-portals-and-deep-rail.md": set(), "r2mlvs4-tunnel-boring-machines.md": set()}
    for name, remaining in expected.items():
        text = (ROOT / PLANS / name).read_text(encoding="utf-8-sig")
        actual = set(re.findall(r"^- \[ \] ([0-9A-Z]+(?:\.[0-9A-Z]+)*)\s", text, re.M))
        assert actual == remaining, (name, actual)
        pending[name] = sorted(actual)
        counts[name] = len(re.findall(r"^- \[X\] ", text, re.M))
    assert counts["r2mlvs3-sealed-portals-and-deep-rail.md"] == 68 and counts["r2mlvs4-tunnel-boring-machines.md"] == 72
    parent = (ROOT / PLANS / "r2mlvs-multilayer-vehicles-and-structures.md").read_text(encoding="utf-8-sig")
    for name in ("r2mlvs3-sealed-portals-and-deep-rail.md", "r2mlvs4-tunnel-boring-machines.md"):
        assert any(line.startswith("| [X] |") and name in line for line in parent.splitlines())
    changed = added_markdown()
    links, syntax = verify_links(changed), verify_python()
    git("diff", "--check")
    result = {"_documentation": "r2mlvs3mp-master-plan.mdcurrentrequirement-specificclosure;notfullgoalcompletion",
              "controller": identity(__file__), "runtime_target_closure_verified": True, "master_complete": False,
              "pending": pending, "checked_task_counts": counts, "receipts": {name: row["reference"] for name, row in receipts.items()},
              "closed_integration_receipts": target["closed_integration_receipts"], "unchanged_strict_artifacts": len(performance["current_production_and_physical_artifacts"]),
              "retained_fixture_files": len(inventory["records"]), "documents": [identity(ROOT / name) for name in sorted(changed)],
              "added_local_links": links, "parsed_python_files": syntax, "git_diff_check": "passed",
              "dated_evidence": "Earlier final boundary/document receipts bind their actual earlier observer bytes;new42conditioninvocations bind the optionalprepare observer extension. Current production/physical refs remain strict.",
              "remaining": "Record actual human direction/state/zoom/occlusion/sound feedback for11.7;obtain refreshed-demo/TBMfix11.5feedback before12.6consumer-selection/deletion. Do not infer either from silence."}
    destination = (ROOT / output).resolve()
    assert destination.is_relative_to(ROOT / "logs") and not destination.exists()
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(destination), "documents": len(changed), "links": len(links), "python_sources": len(syntax),
                      "strict_artifacts": result["unchanged_strict_artifacts"], "retained_fixtures": result["retained_fixture_files"], "master_complete": False}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    audit(parser.parse_args().output)
