"""Seal current five-model native providers and actual model/fleet input.

@file tube_fleet_provider_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md8B.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
Original useful/intermodal/runtime evidence stays with its sealed native owners.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
from tube_fleet_provider_return import CASES, SOURCES, identity
from construction_provider_report import verify_reference

HANDOFFS = (("logs/tubes/forge-files/20261018-native-01/five-model-verification.json",
             "28158c1b1aa03cefc608da94fa7cf7b25ef85f4c116269b8b57161b9cde53d60"),
            ("logs/graphics/selected-gallery/20261018-native-01/publication-verification.json",
             "e83b269f66841e7a580c1917c19dedac289eec657aa6387e0e04f9ae564d7d8b"))


def accept(selection_path, output_path):
    """Bind complete current native providers/input and exact completed handoffs.

    @param selection_path Explicit complete report matrix, never a latest-file search.
    @param output_path Absent immutable receipt beneath repository logs.
    @return Accepted metadata after every actual source, binary and native log agrees.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    # Every current provider and actual mouse obligation must run in both
    # readers; a dated raster or logical row cannot replace native control input.
    expected = {(configuration, focus) for configuration in ("Debug", "RelWithDebInfo") for focus in CASES}
    seen, accepted, cache = set(), [], {}
    summary = {configuration: {"cases": 0, "assertions": 0} for configuration in ("Debug", "RelWithDebInfo")}
    # Reports remain individually isolated native runs. Validate their full
    # chains while hashing shared large sources/binaries once during this pass.
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
        assert report["controller"]["path"] == "src/tests/multilayer/tube_fleet_provider_return.py"
        assert report["boundary_runner"]["path"] == "src/tests/multilayer/construction_provider_return.py"
        assert report["binary"]["path"] == f"build/{configuration}/openttd_test.exe"
        assert [reference["path"] for reference in report["sources"]] == list(SOURCES)
        for reference in [report["controller"], report["boundary_runner"], report["binary"], *report["sources"]]:  # Every current copied identity still belongs to the exact original source/executable owner.
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
    handoffs = []
    # These complete original useful/source/file/raster/gallery receipts are
    # sealed dated evidence; do not demand their historical transient binaries
    # or recreate their cargo production merely to accept present UI entry.
    for path, expected_sha in HANDOFFS:
        reference = identity(ROOT / path)
        assert reference["sha256"] == expected_sha, reference
        handoffs.append(reference)
    result = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md8B.2/original8.1;five native identities/canonical roles/dates/distinct art/real fleet/model dropdown and original vehicle views;purchase/refit/orders/service remain8C",
              "selection": identity(selection_path), "controller": identity(__file__),
              "reference_verifier": identity(ROOT / "src/tests/multilayer/construction_provider_report.py"),
              "summary": summary, "native_cases": sum(item["cases"] for item in summary.values()),
              "native_assertions": sum(item["assertions"] for item in summary.values()), "records": accepted,
              "accepted_dated_handoffs": handoffs, "native_version": 434, "artwork": identity(ROOT / "media/baseset/openttd.grf"),
              "art_fixture_boundary": "Mechanical GUI tests use blank mock bounds; actual distinct loaded runtime art belongs to the exact accepted five-model/public-gallery receipts.",
              "production_changes": "None required; actual native dropdown selection and original capsule view entry passed.",
              "remaining": "Original8.2/native purchase-refit-orders-service8C;actual paid TBM/bootstrap/finaluse and human/network/operator gates."}
    output = (ROOT / output_path).resolve()
    # Preserve earlier acceptance and the designated logs owner; reruns must
    # select an explicitly absent output rather than replace dated evidence.
    assert output.is_relative_to(ROOT / "logs") and not output.exists(), output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}), flush=True)
    return result


def main():
    """Choose the exact completed native report matrix and absent receipt.

    @return None; missing, duplicate, stale or failing obligations refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
