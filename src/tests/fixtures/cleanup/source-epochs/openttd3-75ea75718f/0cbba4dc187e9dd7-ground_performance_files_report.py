"""Accept actual file reconstruction with independent native owner equality and earlier frozen budgets.

@file ground_performance_files_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EB.
Native first resumed work includes actual asynchronous waits;only measured observer overhead is removed.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance_files import BASELINE_SHA, BANK_SHA, inputs, verify_measurement
from ground_performance_views import ORIGINAL, ORIGINAL_SHA
from tube_tbm_files import equal_world
from save_tables import saved_table


def verify_selection(index):
    """Require two fresh processes per exact selected file and complete equal native continuation outputs."""
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA and index["bank"]["sha256"] == BANK_SHA
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    bank = json.loads(verify_reference(index["bank"]).read_text(encoding="utf-8"))
    selected_inputs = inputs(baseline, bank)
    expected = {(cfg, label, attempt) for cfg, label in selected_inputs for attempt in (0, 1)}
    records, assertions = {}, 0
    original_ref = identity(ROOT / ORIGINAL)
    assert original_ref["sha256"] == ORIGINAL_SHA
    original = json.loads(verify_reference(original_ref).read_text(encoding="utf-8"))
    old = {(row["configuration"], row["stage"]): row for row in original["records"]}
    historical_pairs = []
    for row in index["records"]:
        cfg, label, attempt = key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        selected = selected_inputs[cfg, label]
        verify_reference(selected["producer"])
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert actual["configuration"] == cfg
        observed = verify_measurement(actual, baseline, selected)
        assert observed["budget_excesses"] == [] and all(row[field] == value for field, value in observed.items())
        for phase, owner in zip(("initial", "final"), actual["owners"]):
            date = saved_table(verify_reference(actual["saves"][phase + "-world"]), b"DATE")[0]
            assert date["tick_counter"] == owner["tick"]
        record = {"key": list(key), "report": row["report"], "actual": actual, "selected": selected, **observed}
        if label.startswith("active-"):
            dated = old[cfg, selected["stage"]]
            assert json.loads(verify_reference(dated["report"]).read_text(encoding="utf-8")) == dated["actual"]
            comparison = equal_world(verify_reference(dated["actual"]["saves"]["final-world"]), verify_reference(actual["saves"]["final-world"]))
            historical_pairs.append({"key": list(key), "original_final": dated["actual"]["saves"]["final-world"], "comparison": comparison})
        records[key] = record; assertions += actual["native"]["assertions"]
    assert set(records) == expected and len(records) == 28 and len(historical_pairs) == 16
    independent = []
    for cfg, label in selected_inputs:
        first, second = (records[cfg, label, attempt]["actual"] for attempt in (0, 1))
        comparison = equal_world(verify_reference(first["saves"]["final-world"]), verify_reference(second["saves"]["final-world"]))
        independent.append({"configuration": cfg, "label": label, "comparison": comparison})
    return list(records.values()), assertions, independent, historical_pairs, original_ref


def accept(selection, checks, output):
    """Close actual selected files and first-native-work scope;real network reconstruction remains separate."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    records, assertions, independent, historical, original = verify_selection(index)
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 8 tests in [\d.]+s\s+OK\s*$", text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21EB;actual selected files/read/fixup/first resumed work,not realjoin",
              "classification": "ground_file_reconstruction_performance_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "bank": index["bank"], "original_physical_performance": original,
              "records": records, "native_cases": 28, "native_assertions": assertions, "measured_ticks": 28 * 640,
              "unique_inputs": 14, "whole_file_pure_pairs": 56, "complete126_chunk_independent_finals": independent,
              "complete126_chunk_original_active_finals": historical,
              "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/ground_performance_files_checks.py"), "log": checks_ref, "tests": 8},
              "scope": "Multilayer compact128/representative512/largestcompleted1024Debug2048optimized ordinary files;4actualcompactoriginalstages/build;2freshprocesses/input",
              "uncertainty": "filesystem cache uncontrolled;ordinary large geometry versus compact active physical owners explicit;launch-to-first-completed-native-frame excludes only retained initial observer inspection/pure-save/unpause overhead;actual LinkGraphwaits retained;only caches exercised by selected full native work claimed;null raster",
              "remaining": "21EC real network join reconstruction;21F attribution/master11.6"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "cases": 28, "assertions": assertions,
                      "whole_file_pure_pairs": 56, "complete126_final_pairs": 30}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.checks, args.output)
