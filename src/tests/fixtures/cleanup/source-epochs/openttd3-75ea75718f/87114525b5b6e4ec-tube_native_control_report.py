"""Seal current native capsule controls and their original intermodal handoff.

@file tube_native_control_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md8C.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
The changed English guidance requires the separate existing-owner8D review refresh.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
from tube_native_control_return import CASES, SOURCES, identity
from tube_fleet_provider_report import HANDOFFS as FEATURE_HANDOFFS
from construction_provider_report import verify_reference

HANDOFFS = (("logs/construction/provider-return/20261019-native-01/tube-fleet-input-verification.json",
             "0d7674f66c044beb86f694093db400a2ccb4858533cca2671bc6c761cc5104f3"), *FEATURE_HANDOFFS)


def accept(selection_path, output_path, language_baseline):
    """Bind actual command/input checks and the exact completed intermodal proof.

    @param selection_path Explicit complete current native report matrix.
    @param output_path Absent immutable receipt beneath repository logs.
    @param language_baseline Actual generated language-version observation before the existing help edit.
    @return Complete selected native control acceptance, leaving8D gallery refresh pending.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    # All command/custody/physical-domain and actual input obligations must
    # pass in both readers; old useful/raster evidence cannot substitute here.
    expected = {(configuration, focus) for configuration in ("Debug", "RelWithDebInfo") for focus in CASES}
    seen, accepted, cache = set(), [], {}
    summary = {configuration: {"cases": 0, "assertions": 0} for configuration in ("Debug", "RelWithDebInfo")}
    # Each exact case/tag runs in its own native process. Shared file hashing
    # is cached only within this acceptance pass, with every reported identity
    # still compared against the actual current owner bytes.
    for row in selection["records"]:
        key = row["configuration"], row["focus"]
        assert key in expected and key not in seen, key
        seen.add(key)
        configuration, focus = key
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == configuration
        assert report["focus"] == focus
        assert report["tag"] == CASES[focus]
        assert report["native_version"] == 434
        assert report["controller"]["path"] == "src/tests/multilayer/tube_native_control_return.py"
        assert report["boundary_runner"]["path"] == "src/tests/multilayer/construction_provider_return.py"
        assert report["binary"]["path"] == f"build/{configuration}/openttd_test.exe"
        assert [reference["path"] for reference in report["sources"]] == list(SOURCES)
        for reference in [report["controller"], report["boundary_runner"], report["binary"], *report["sources"]]:  # Current command/UI/source/binary owners cannot be replaced by a logical or dated report.
            verify_reference(reference, cache)
        log = verify_reference(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert f"Filters: {CASES[focus]}" in log
        assert "NATIVE_EXIT_CODE=0" in log
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log)
        assert counts is not None, row
        assertions, native_cases = int(counts[1]), int(counts[2])
        assert native_cases == 1 and assertions > 0, row
        assert report["native"]["assertions"] == assertions
        assert report["native"]["cases"] == native_cases
        summary[configuration]["cases"] += native_cases
        summary[configuration]["assertions"] += assertions
        accepted.append({"report": identity(path), **report})
    assert seen == expected, expected - seen
    language_path = ROOT / language_baseline
    before = json.loads(language_path.read_text(encoding="utf-8"))
    header = ROOT / before["generated_header"]
    current_version = [line.strip() for line in header.read_text(encoding="utf-8").splitlines() if "LANGUAGE_PACK_VERSION" in line]
    # Editing an existing help string must preserve native language compatibility;
    # current GUI tests also load the actual generated English pack and check fit.
    assert len(current_version) == 1
    assert current_version == before["language_version_lines"]
    handoffs = []
    # Keep original source/useful/intermodal/file/body/public-gallery receipts
    # opaque and dated. The English-fingerprinted gallery source refresh belongs
    # to8D; no changed source hash is ignored or relabelled current here.
    for path, expected_sha in HANDOFFS:
        reference = identity(ROOT / path)
        assert reference["sha256"] == expected_sha, reference
        handoffs.append(reference)
    result = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md8C/original8.2;actual native purchase/refit/orders/manualservice/paidwork/custody/ordinarydomain and original five-model intermodal handoff;changed English-source review remains8D",
              "selection": identity(selection_path), "controller": identity(__file__),
              "reference_verifier": identity(ROOT / "src/tests/multilayer/construction_provider_report.py"),
              "handoff_contract": identity(ROOT / "src/tests/multilayer/tube_fleet_provider_report.py"),
              "summary": summary, "native_cases": sum(item["cases"] for item in summary.values()),
              "native_assertions": sum(item["assertions"] for item in summary.values()), "records": accepted,
              "accepted_dated_handoffs": handoffs, "native_version": 434, "artwork": identity(ROOT / "media/baseset/openttd.grf"),
              "language": {"baseline": identity(language_path), "current_generated_header": identity(header),
                           "current_generated_english": identity(ROOT / "build/lang/english.lng"), "unchanged_native_version": current_version},
              "production_changes": "Existing English tube help now explains ordinary-rail/portal refusal and conventional freight transfer;native C++ simulation,save fields,StringIDs,GRF and source pixels unchanged.",
              "fixture_boundary": "Prepared native geometry/empty UI capsules and explicit command custody faults establish controls;actual useful packet provenance remains in the sealed five-model feature handoff.",
              "remaining": "Mandatory8D existing-owner refresh of English-fingerprinted selected purchase/geothermal/public-gallery evidence before Plan3;actual paid TBM/bootstrap/finaluse and human/network/operator gates."}
    output = (ROOT / output_path).resolve()
    # Acceptance must remain under logs and cannot overwrite earlier proof.
    assert output.is_relative_to(ROOT / "logs") and not output.exists(), output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}), flush=True)
    return result


def main():
    """Choose exact current native reports, language baseline and absent receipt.

    @return None; missing, duplicate, stale or failing evidence refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--language-baseline", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output, args.language_baseline)


if __name__ == "__main__":
    main()
