"""Recheck final portal/TBM ownership boundaries with one guarded native tag per process.

@file ground_final_boundaries.py
@see r2mlvs3mp-master-plan.md12.4 and portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md.
Consumes exact closed integration evidence; does not grant human art/demo acceptance.
"""

import argparse
import json
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from bounded_performance import observe, verify_observation
from ground_performance_acceptance import strict_reference

TAGS = ("portal-date", "portal-script", "portal-profile", "portal-contract", "portal-lifecycle", "multilayer-content-contract",
        "utility-power-company", "utility-power-performance", "tbm-controls", "tbm-recovery", "tbm-exchange-lifecycle", "layer-industry-lifecycle")
SOURCES = tuple("src/tests/" + name + ".cpp" for name in (
    "portal_date", "portal_profile", "portal_work_state", "portal_lifecycle", "multilayer_content", "utility_power_company",
    "utility_power_performance", "tbm_movement", "tbm_recovery", "tbm_exchange_lifecycle", "layer_industry_lifecycle")) + (
    "src/tests/CMakeLists.txt", "src/tests/ground_network_fixture.h", "src/tests/ground_performance_fixture.h",
    "src/layer_portal_work.cpp", "src/layer_portal_cmd.cpp", "src/tbm_work.cpp", "src/tbm_project.cpp", "src/train_cmd.cpp", "src/utility_power.cpp")
HANDOFFS = {
    "multiplayer": ("logs/network/paid-ground/20261019-native-01/selected-handoff.json", "afc95a4778bda9349394d51bed358dfff2264682e1770d9b8dfbb941be71f6b4"),
    "genuine-bootstrap": ("logs/acceptance/bootstrap/20261019-native-01/genuine-bootstrap-verification.json", "acf7837bcf2d0f5dd9506cde1f393abcee8a12d37631a05d2a127f6316711846"),
    "technical-art": ("logs/graphics/selected-bootstrap/20261019-native-01/publication-verification.json", "ea157f9f4cfcbd1f1efb692c63e849a4a8694adeb2695a012f996d4e6e0db7dd"),
    "performance": ("logs/performance/ground-maps/20261019-native-01/selected-performance-verification.json", "3023a8aced290e07439f2e36150782a3cb8f3f3988888b151c1e5ad554186a97"),
}


def run(output, configurations=("Debug", "RelWithDebInfo"), tags=TAGS):
    """Verify closed handoffs and run both configurations sequentially with isolated tag state."""
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    handoffs = {}
    for key, (path, expected) in HANDOFFS.items():
        reference = identity(ROOT / path)
        assert reference["sha256"] == expected
        strict_reference(reference)
        handoffs[key] = reference
    assert all((ROOT / source).is_file() for source in SOURCES)
    records, cases, assertions = [], 0, 0
    for cfg in configurations:
        for name in tags:
            tag = "[" + name + "]"
            context, actual, baseline = observe(cfg, tag, {}, SOURCES, "final-boundaries", __file__)
            result = context.output("reports", "boundary-" + name + ".json")
            result.write_text(json.dumps(actual, indent=2) + "\n", encoding="utf-8")
            verify_observation(actual, baseline, tag, {})
            records.append({"configuration": cfg, "tag": tag, "report": identity(result), "actual": actual})
            cases += actual["native"]["cases"]
            assertions += actual["native"]["assertions"]
            print(json.dumps({"configuration": cfg, "tag": tag, "cases": actual["native"]["cases"], "assertions": actual["native"]["assertions"]}), flush=True)
    assert len(records) == len(configurations) * len(tags) and {row["configuration"] for row in records} == set(configurations)
    result = {"_documentation": "r2mlvs3mp-master-plan.md12.4;fresh isolated final ownership boundaries and exact dated integration handoffs",
              "controller": identity(__file__), "complete": True, "selection": {"configurations": configurations, "tags": tags}, "handoffs": handoffs, "records": records,
              "native_cases": cases, "native_assertions": assertions, "isolated_invocations": len(records),
              "remaining": "human art/direction/state/zoom/occlusion/sound feedback;user demo acceptance and gated fixture cleanup;whole unrelated profiles"}
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "cases": cases, "assertions": assertions}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--focus", choices=TAGS)
    args = parser.parse_args()
    run(args.output, (args.configuration,) if args.configuration else ("Debug", "RelWithDebInfo"), (args.focus,) if args.focus else TAGS)
