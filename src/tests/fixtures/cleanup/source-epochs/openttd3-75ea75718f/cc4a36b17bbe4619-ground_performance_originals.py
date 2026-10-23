"""Measure exact original normal-world work,loaded spoil and useful freight through the native loop.

@file ground_performance_originals.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This consumes genuine-world originals as performance inputs;their acceptance authority stays unchanged.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import run_one

BASELINE = "logs/performance/ground-maps/20261019-native-01/ordinary-baseline-verification.json"
BASELINE_SHA = "27c6131b6952f0174b9155d6fdc7d484fb92c46dbc90c69ac63a9200d68db264"
BANK = "logs/acceptance/bootstrap/20261019-native-01/original-file-bank.json"
BANK_SHA = "fb2c8d058fe5d1ba5be3ab2b60c557a47225908834ce6c7ca873fffb86e16570"
STAGES = ("working-bore", "loaded-spoil", "loaded-useful", "final-useful")


def main():
    """Run one serial exact eight-input selection with current native observers and fixed earlier budgets."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    baseline_ref, bank_ref = identity(ROOT / BASELINE), identity(ROOT / BANK)
    assert baseline_ref["sha256"] == BASELINE_SHA and bank_ref["sha256"] == BANK_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    bank = json.loads(verify_reference(bank_ref).read_text(encoding="utf-8"))
    index = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DB;mutable actual original-world performance selection",
             "classification": "ground_original_performance_selection", "bootstrap_claim": False,
             "controller": identity(__file__), "baseline": baseline_ref, "bank": bank_ref, "records": [], "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for cfg in ("Debug", "RelWithDebInfo"):
        budget = next(row for row in baseline["budgets"] if row["configuration"] == cfg and row["axis"] == 128)
        for stage in STAGES:
            selected = bank["records"][cfg][stage]
            original = verify_reference(selected["original"])
            report = run_one(cfg, "multilayer_industry", 128, original)
            actual = json.loads(report.read_text(encoding="utf-8"))
            assert actual["input"] == selected["original"] and actual["status"] == "observed" and actual["children_reaped"]
            index["records"].append({"configuration": cfg, "stage": stage, "selected": selected,
                                      "budget": budget, "report": identity(report)})
            target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "native_cases": len(index["records"])}), flush=True)


if __name__ == "__main__":
    main()
