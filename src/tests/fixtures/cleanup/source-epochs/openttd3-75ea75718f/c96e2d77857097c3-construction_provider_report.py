"""Seal current delivered tube provider and actual native input acceptance.

@file construction_provider_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md4B.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
Prepared test geometry proves interface/command integration, not paid bootstrap.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
from construction_input_checks import CASES as INPUT_CASES, SOURCES as INPUT_SOURCES
from construction_provider_return import CASES as PROVIDER_CASES, SOURCES as PROVIDER_SOURCES
from construction_input_checks import identity

CONFIGURATIONS = ("Debug", "RelWithDebInfo")
OWNERS = {"provider": (PROVIDER_CASES, PROVIDER_SOURCES, "construction_provider_return.py"),
          "input": (INPUT_CASES, INPUT_SOURCES, "construction_input_checks.py")}


def verify_reference(reference, cache):
    """Require an exact retained source, executable or native diagnostic identity.

    @param reference Repository-relative path, byte count and SHA256 from its owner.
    @param cache Actual identities already computed during this single acceptance pass.
    @return Existing file after current bytes match the complete declared identity.
    """
    path = (ROOT / reference["path"]).resolve()
    assert path.is_relative_to(ROOT), reference
    # Shared sources and large native executables occur in many independent
    # reports; hashing once preserves exact comparison without repeated disk IO.
    if path not in cache:
        cache[path] = identity(path)
    assert cache[path] == reference, reference
    return path


def accept(selection_path, output_path):
    """Bind both readers' complete current provider and real mouse obligations.

    @param selection_path Explicit complete index of reports, never a newest-file search.
    @param output_path Absent immutable receipt under repository logs.
    @return Complete accepted receipt after every report/log/source check succeeds.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    # Cover the full bounded provider/input matrix in both readers without
    # accepting unrelated tests as substitutes for a dispatched obligation.
    expected = {(kind, configuration, focus) for kind, (cases, _, _) in OWNERS.items()
                for configuration in CONFIGURATIONS for focus in cases}
    seen = set()
    summary = {configuration: {"cases": 0, "assertions": 0} for configuration in CONFIGURATIONS}
    accepted = []
    cache = {}
    # Every independently isolated provider/input obligation must be present once
    # in each reader; a passing UI case cannot substitute for a command boundary.
    for row in selection["records"]:
        key = row["kind"], row["configuration"], row["focus"]
        # Duplicate or unrelated reports would conceal a missing dispatched
        # obligation, so neither may contribute to the acceptance total.
        assert key in expected and key not in seen, key
        seen.add(key)
        kind, configuration, focus = key
        cases, sources, controller = OWNERS[kind]
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == configuration
        assert report["focus"] == focus
        assert report["binary"]["path"] == f"build/{configuration}/openttd_test.exe"
        assert report["controller"]["path"] == f"src/tests/multilayer/{controller}"
        assert [item["path"] for item in report["sources"]] == list(sources)
        # Reported sources and the actual selected executable must still be
        # current; old callback-only receipts cannot close new mouse acceptance.
        for reference in [report["controller"], report["binary"], *report["sources"]]:
            verify_reference(reference, cache)
        log = verify_reference(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert f"Filters: {cases[focus]}" in log, row
        assert "NATIVE_EXIT_CODE=0" in log, row
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log)
        assert counts is not None, row
        assertions, native_cases = int(counts[1]), int(counts[2])
        # Exactly one compiled case owns each focus; Catch generators exercise
        # main/extra views and widget choices inside that independently run case.
        assert native_cases == 1 and assertions > 0, row
        assert report["native"]["assertions"] == assertions
        assert report["native"]["cases"] == native_cases
        summary[configuration]["cases"] += native_cases
        summary[configuration]["assertions"] += assertions
        accepted.append({"kind": kind, "report": identity(path), **report})
    assert seen == expected, expected - seen
    artwork = identity(ROOT / "media/baseset/openttd.grf")
    result = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md4B.2/original4.1;current native tube provider commands,distinct capsule fleet and actual main/extra viewport mouse acceptance;prepared geometry is not paid final-use/bootstrap/human acceptance",
              "selection": identity(selection_path), "controller": identity(__file__), "summary": summary,
              "native_cases": sum(item["cases"] for item in summary.values()),
              "native_assertions": sum(item["assertions"] for item in summary.values()),
              "native_version": 434, "artwork": artwork, "records": accepted,
              "production_changes": "None required for this delivered-provider return; test-only actual mouse characterization passed.",
              "remaining": "Delivered geothermal/date rows4.2/4.3B,next actual five-model fleet Group8,paid TBM/bootstrap/final-use and human/network/operator gates;future plant actions remain pending."}
    output = (ROOT / output_path).resolve()
    # Accepted evidence belongs to logs and cannot replace an earlier immutable
    # receipt; a rerun must explicitly choose its own output name.
    assert output.is_relative_to(ROOT / "logs") and not output.exists(), output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}), flush=True)
    return result


def main():
    """Select explicit completed reports and an absent current acceptance receipt.

    @return None; stale, incomplete, duplicate or failing native evidence refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
