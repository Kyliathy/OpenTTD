"""Seal original-portal riser admission and retain the actual paid first-plant/export chain.

@file portal_riser_consumer_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md Group12C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Current native boundary fixtures remain distinct from original paid construction/useful service.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from portal_consumer_return import ROOT, RISER_CASES
from portal_consumer_report import CONFIGURATIONS, checked, verify_matrix
from construction_provider_return import identity

HANDOFFS = {
    "tube-consumers": ("logs/portals/consumer-return/20261019-native-01/tube-verification.json", "dd6df6012dcca1f30565fcef9714927a9ad6a2a24fd08baca2331d23fcce6aac"),
    "paid-excavation": ("logs/geothermal/first-plant/20261017-native-01/verification.json", "0ba8cda739824125f949af3c51d254e0b632a3a7230244cbf47e5549146619ee"),
    "paid-utilities": ("logs/geothermal/first-plant/20261017-native-01/construction-utilities.json", "7f6bf4eb8b4c64eeefa0c8d78f79c02503a690d36038b2bcf36a822b0240a557"),
    "paid-useful-export": ("logs/geothermal/first-plant/20261017-native-01/first-plant-power-verification.json", "905afa4cff910932cd87a5009617c2e699448c30bf5d65333161926cdf27e666"),
    "selected-lifecycle": ("logs/lifecycle/owner-return/20261018-native-01/handoff-verification.json", "0a4a92014879dc589be18118b955b54033b0872a93c7fa1818fb810f3323244d"),
}


def file_reference(value):
    """Select only exact file identity fields from an original owner checkpoint.

    @param value Original checkpoint that may additionally expose retained cash or state observations.
    @return Original path/bytes/SHA256; observations never participate as invented file identity fields.
    """
    return {key: value[key] for key in ("path", "bytes", "sha256")}


def accept(selection_path, output_path):
    """Verify current riser consumers and seal their actual original paid-world authority.

    @param selection_path Exact complete current both-reader riser report/build index.
    @param output_path Absent accepted receipt beneath repository logs.
    @return Complete integration receipt after exact native, original-save and sealed-handoff checks.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    summary, records, cache = verify_matrix(selection, RISER_CASES)
    handoffs, documents = {}, {}
    # The owning paid extraction, Surface carriage, physical construction,
    # actual useful export and lifecycle receipts retain their full evidence.
    # Do not reopen disposable dated binaries or repeat their paid histories.
    for role, (name, sha256) in HANDOFFS.items():
        path = ROOT / name
        reference = identity(path)
        assert reference["sha256"] == sha256, role
        handoffs[role] = reference
        documents[role] = json.loads(path.read_text(encoding="utf-8"))
    utilities, power = documents["paid-utilities"], documents["paid-useful-export"]
    assert utilities["all_independent_complete_chunks_equal"]
    assert utilities["underlying_map_and_original_bill_history_preserved_by_installation"]
    assert power["all_independent_complete_chunks_equal"]
    checked(file_reference(utilities["ready_for_generation"]), cache)
    checked(power["same_world_ready"], cache)
    comparison_ref = power["comparison"]
    comparison = json.loads(checked(comparison_ref, cache).read_text(encoding="utf-8"))
    assert comparison["same_original_risers"] == power["same_original_owners"]["risers"]
    assert len(comparison["same_original_risers"]) == 2
    assert comparison["retained1980_generator_exact"]
    assert comparison["construction_materials_and_original_floor_vent_machines_unchanged"]
    original_comparisons = comparison["both_actual74_tick_comparisons"]
    assert set(original_comparisons) == {"vent_tap", "magma_loop"}
    # Both actual model comparisons used the same original paid risers and
    # earned useful Surface service; preserve every original input/output file
    # rather than inferring delivered electricity from a native test fixture.
    for model, original in original_comparisons.items():
        assert original["active_ticks"] == 74, model
        assert original["original_heat_consumed"] > 0 and original["electrical_generated"] > 0
        assert original["useful_electricity"] > 0 and original["cash_spent"] > 0
        checked(original["input"], cache)
        checked(original["output"], cache)
    lifecycle = documents["selected-lifecycle"]
    assert lifecycle["native_version"] == 434
    assert lifecycle["complete_126_chunk_cross_reader_pairs"] == 14
    tube = documents["tube-consumers"]
    assert tube["native_version"] == 434 and tube["native_cases"] == 16
    build_logs = {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS}
    result = {
        "_documentation": "r2mlvs3-sealed-portals-and-deep-rail.md12C/original12.1; delivered native tube/riser commissioning consumers and exact retained paid first-plant/intermodal/lifecycle chain;master10.8 paid TBM final use remains separate",
        "controller": identity(__file__), "matrix_verifier": identity(ROOT / "src/tests/multilayer/portal_consumer_report.py"),
        "funding_owner": identity(ROOT / "src/industry_cmd.cpp"), "selection": identity(selection_path),
        "summary": summary, "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "combined_current_native_cases": tube["native_cases"] + sum(row["cases"] for row in summary.values()),
        "combined_current_native_assertions": tube["native_assertions"] + sum(row["assertions"] for row in summary.values()),
        "native_version": 434, "build_logs": build_logs, "records": records,
        "retained_handoffs": handoffs, "original_paid_comparison": comparison_ref,
        "original_same_world_risers": comparison["same_original_risers"], "original_useful_service": original_comparisons,
        "artwork": identity(ROOT / "media/baseset/openttd.grf"),
        "classification": "Current admission/funding/construction fixtures use disclosed prepared floor/materials;separate exact original paid extraction/carriage/riser/export receipts retain actual bootstrap authority.",
        "remaining": "Return master10.8 to remaining Plan4 Group15 then21/Plan17 Group16 actual paid TBM conversion/final use;whole master and human/network/operator gates remain pending.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "combined_native_cases": result["combined_current_native_cases"], "combined_native_assertions": result["combined_current_native_assertions"]}), flush=True)
    return result


def main():
    """Select one explicit current riser matrix and absent retained handoff receipt.

    @return None; stale native sources/logs, missing originals or altered upstream proof refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
