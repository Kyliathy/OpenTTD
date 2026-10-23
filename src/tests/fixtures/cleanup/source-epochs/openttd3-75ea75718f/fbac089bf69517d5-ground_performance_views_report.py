"""Seal native view-independent original work and exact closed utility/tube performance components.

@file ground_performance_views_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDD.
Complete native final equality carries the original physical ledger;no gameplay owner is ignored.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance_views import BANK_SHA, BASELINE_SHA, COMPONENTS, ORIGINAL, ORIGINAL_SHA, selections, verify_measurement
from tube_tbm_files import equal_world


def verify_component(path, sha):
    """Consume exact dated outer receipts;physical outputs and unchanged production stay exact after later observer builds."""
    ref = identity(ROOT / path)
    assert ref["sha256"] == sha
    receipt = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
    for entry in (*receipt["records"], *receipt.get("regressions", [])):
        actual = json.loads(verify_reference(entry["report"]).read_text(encoding="utf-8"))
        assert actual == entry["actual"]
        native = actual.get("native", actual)
        for binding in (native["log"], actual["config"], actual["memory_samples"]):
            verify_reference(binding)
        for binding in actual["sources"]:
            if not binding["path"].startswith("src/tests/"):
                verify_reference(binding)
    return ref


def verify_selection(index):
    """Require every current source-bound view run and all original final ownership at exactly the same tick."""
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA and index["bank"]["sha256"] == BANK_SHA
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    bank = json.loads(verify_reference(index["bank"]).read_text(encoding="utf-8"))
    original_ref = identity(ROOT / ORIGINAL)
    assert original_ref["sha256"] == ORIGINAL_SHA
    original = json.loads(verify_reference(original_ref).read_text(encoding="utf-8"))
    old = {(row["configuration"], row["stage"]): row for row in original["records"]}
    expected, records, assertions = set(selections()), {}, 0
    for row in index["records"]:
        cfg, stage, view = key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert actual["configuration"] == cfg
        observed = verify_measurement(actual, baseline, view, bank["records"][cfg][stage])
        assert observed["budget_excesses"] == [] and all(row[field] == value for field, value in observed.items())
        dated = old[cfg, stage]
        assert json.loads(verify_reference(dated["report"]).read_text(encoding="utf-8")) == dated["actual"]
        # Final state includes every money,packet,geology,job,vehicle,utility,
        # RNG,clock,settings and diagnostic owner,plus normalized native VIEW.
        equality = equal_world(verify_reference(dated["actual"]["saves"]["final-world"]), verify_reference(actual["saves"]["final-world"]))
        records[key] = {"key": list(key), "report": row["report"], "actual": actual,
                        "original_final": dated["actual"]["saves"]["final-world"], "complete_final_equality": equality,
                        "physical_ledger_producer": dated["report"], **observed}
        assertions += actual["native"]["assertions"]
    assert set(records) == expected and len(records) == 32
    return list(records.values()), assertions, original_ref


def accept(selection, checks, output):
    """Close selected utility/tube/view scope only;actual large files,joins and overall performance remain later work."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    records, assertions, original = verify_selection(index)
    components = [verify_component(path, sha) for path, sha in COMPONENTS]
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 8 tests in [\d.]+s\s+OK\s*$", text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DDD;complete selected21DD/original21D.2utility,tube and native view-independent workload",
              "classification": "ground_utility_tube_view_performance_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "bank": index["bank"], "original_physical_performance": original,
              "component_receipts": components, "records": records, "native_cases": 32, "native_assertions": assertions,
              "measured_ticks": 32 * 640, "complete126_chunk_pure_pairs": 64, "complete126_chunk_original_final_pairs": 32,
              "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/ground_performance_views_checks.py"), "log": checks_ref, "tests": 8},
              "uncertainty": "native full GameLoop/InputLoop/UpdateWindows;null raster in every role;Surface/Underground/hidden Deep selections viaSetViewportLayer;headless HasGUI=false retains ordinary logical viewport;no rasterFPSclaim;OScache/desktopnoise and startup-to-ready versus later cache-ready scope explicit",
              "remaining": "21E actual large files/rebuilt caches/realjoins;21F attribution/master11.6;human raster review separate"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "cases": 32, "assertions": assertions, "complete126_pairs": 96}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.checks, args.output)
