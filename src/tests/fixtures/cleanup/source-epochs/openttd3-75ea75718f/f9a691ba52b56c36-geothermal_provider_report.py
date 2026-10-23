"""Seal selected native geothermal provider entry or actual input acceptance.

@file geothermal_provider_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md4C.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
No later factory, paid bootstrap, human or network gate is completed here.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
from geothermal_provider_return import ENTRY_CASES, CASES, SOURCES, identity
from construction_provider_report import verify_reference


def accept(selection_path, output_path, phase):
    """Bind the exact selected provider matrix to current sources and native logs.

    @param selection_path Explicit complete report index beneath repository logs.
    @param output_path Absent immutable entry/acceptance receipt beneath logs.
    @param phase Entry requires producer boundaries; input also requires actual mouse cases.
    @return Complete accepted metadata after every original identity and case passes.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    cases = ENTRY_CASES if phase == "entry" else CASES
    # Both readers must independently discharge every selected obligation;
    # unrelated passing tests cannot stand in for a missing control family.
    expected = {(configuration, focus) for configuration in ("Debug", "RelWithDebInfo") for focus in cases}
    seen, accepted, cache = set(), [], {}
    summary = {configuration: {"cases": 0, "assertions": 0} for configuration in ("Debug", "RelWithDebInfo")}
    # Each row retains its actual native log and complete current source chain.
    # Entry callback proofs remain distinct from the later real mouse cases.
    for row in selection["records"]:
        key = row["configuration"], row["focus"]
        assert key in expected and key not in seen, key
        seen.add(key)
        configuration, focus = key
        report_path = ROOT / row["report"]
        report = json.loads(report_path.read_text(encoding="utf-8"))
        assert report["configuration"] == configuration
        assert report["focus"] == focus
        assert report["tag"] == cases[focus]
        assert report["native_version"] == 434
        assert report["controller"]["path"] == "src/tests/multilayer/geothermal_provider_return.py"
        assert report["binary"]["path"] == f"build/{configuration}/openttd_test.exe"
        assert report["boundary_runner"]["path"] == "src/tests/multilayer/construction_provider_return.py"
        assert [item["path"] for item in report["sources"]] == list(SOURCES)
        # Shared identities are hashed once per pass; every reported copy must
        # still match the exact current bytes, including the real test binary.
        for reference in [report["controller"], report["binary"], report["boundary_runner"], *report["sources"]]:
            verify_reference(reference, cache)
        log = verify_reference(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert f"Filters: {cases[focus]}" in log
        assert "NATIVE_EXIT_CODE=0" in log
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log)
        assert counts is not None, row
        assertions, native_cases = int(counts[1]), int(counts[2])
        assert native_cases == 1 and assertions > 0, row
        assert report["native"]["assertions"] == assertions
        assert report["native"]["cases"] == native_cases
        summary[configuration]["cases"] += native_cases
        summary[configuration]["assertions"] += assertions
        accepted.append({"report": identity(report_path), **report})
    assert seen == expected, expected - seen
    result = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md4C;selected delivered geothermal tools and canonical dates;prepared native geometry,not paid final-use/bootstrap/human acceptance",
              "phase": phase, "selection": identity(selection_path), "controller": identity(__file__),
              "reference_verifier": identity(ROOT / "src/tests/multilayer/construction_provider_report.py"),
              "summary": summary, "native_cases": sum(item["cases"] for item in summary.values()),
              "native_assertions": sum(item["assertions"] for item in summary.values()),
              "native_version": 434, "artwork": identity(ROOT / "media/baseset/openttd.grf"), "records": accepted,
              "remaining": "Future directprocessing factories,paid TBM/bootstrap/finaluse and human/network/operator gates;entry alone does not close actual input."}
    output = (ROOT / output_path).resolve()
    # A fresh receipt cannot replace earlier retained evidence or escape the
    # designated diagnostic owner. Explicit reruns choose their own names.
    assert output.is_relative_to(ROOT / "logs") and not output.exists(), output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}), flush=True)
    return result


def main():
    """Select the explicit completed provider/input phase and absent receipt.

    @return None; incomplete or stale native evidence refuses before publication.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--phase", choices=("entry", "input"), required=True)
    args = parser.parse_args()
    accept(args.selection, args.output, args.phase)


if __name__ == "__main__":
    main()
