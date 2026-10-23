"""Seal both public S1/S2 records against their real original-world and current native owners.

@file normal_ground_acceptance_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DEC;master11.4.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This consumes the existing public runner;it creates no alternate scenario authority.
"""

import argparse
import importlib.util
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from normal_ground_bootstrap_proof import verify_proof_payload
from tube_tbm_files import equal_world

spec = importlib.util.spec_from_file_location("public_normal_ground_owner", ROOT / "src/tests/multilayer/acceptance/native_ground.py")
owner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(owner)
ART_PATH = "logs/graphics/selected-bootstrap/20261019-native-01/publication-verification.json"
ART_SHA = "ea157f9f4cfcbd1f1efb692c63e849a4a8694adeb2695a012f996d4e6e0db7dd"


def read_reference(reference):
    """Read only exact retained JSON bytes,refusing changed or absent physical evidence."""
    return json.loads(verify_reference(reference).read_text(encoding="utf-8"))


def verify_native(actual, selected, configuration, bank):
    """Recheck actual isolated readers,commands,logs,pure worlds and the untouched next transition."""
    assert actual["classification"] == "normal_start_native_files" and actual["bootstrap_claim"] is False
    assert actual["origin"] == actual["reader"] == configuration and actual["stage"] == "final-useful"
    assert actual["selected"] == selected and actual["bank"] == bank
    assert actual["complete_chunks_equal"] == 126 and actual["ignored_chunks"] == []
    for reference in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
        verify_reference(reference)
    assert verify_reference(actual["config"]).read_bytes() == verify_reference(selected["config"]).read_bytes()
    assert actual["binary"] == identity(ROOT / f"build/{configuration}/openttd_test.exe")
    assert actual["controller"] == identity(ROOT / "src/tests/multilayer/normal_ground_files.py")
    assert [case["action"] for case in actual["cases"]] == ["baseline", "inspect", "next", "reload-next"]
    original, baseline = verify_reference(selected["original"]), verify_reference(actual["baseline"])
    inspected, comparisons, assertions, logs = None, 0, 0, []
    for case in actual["cases"]:
        action, native = case["action"], case["native"]
        assert case["children_reaped"] is True and native["cases"] == 1
        assert native["argv"] == [str(verify_reference(actual["binary"])), "[.normal-ground-file]"] and native["cwd"] == "build"
        assert case["input"] == (inspected if action == "reload-next" else selected["original"])
        selectors = native["environment_selectors"]
        assert selectors["OTTD_NORMAL_GROUND_INPUT"] == str(verify_reference(case["input"]))
        assert selectors["OTTD_NORMAL_GROUND_FILE_ACTION"] == action
        assert selectors["OTTD_NORMAL_GROUND_FILE_LABEL"] == f"{configuration}-{configuration}-final-useful-{action}".lower()
        text = verify_reference(native["log"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == native["assertions"] > 0 and int(counts[2]) == 1
        assert f"NORMAL_GROUND_FILE action={action} preview={int(action != 'baseline')} completed=1" in text
        outputs = {key: verify_reference(value) for key, value in case["outputs"].items()}
        expected = {"original_read": equal_world(original, outputs["read"]),
                    "pure_read": equal_world(outputs["read"], outputs["repeat"])}
        if action == "inspect":
            inspected = case["outputs"]["read"]
        else:
            expected.update(untouched_next=equal_world(baseline, outputs["next"]),
                            pure_next=equal_world(outputs["next"], outputs["next-repeat"]))
        assert case["comparisons"] == expected
        comparisons += len(expected)
        assertions += native["assertions"]
        logs.append(native["log"]["path"])
    assert actual["native_cases"] == 4 and actual["native_assertions"] == assertions
    return inspected, comparisons, assertions, logs


def accept(report_names, check_pairs, output):
    """Publish one complete four-record acceptance after rechecking every retained public claim."""
    proof = verify_proof_payload(read_reference(owner.NORMAL_START_PROOF))
    art_ref = identity(ROOT / ART_PATH)
    assert art_ref["sha256"] == ART_SHA
    publication = read_reference(art_ref)
    assert publication["technical_acceptance"] == "verified" and publication["user_feedback"] == "deferred"
    art_keys = ("public_manifest", "packed_manifest", "runtime_manifest", "gallery")
    for key in (*art_keys, "default_selection", "selection", "contact_sheet", "html"):
        verify_reference(publication[key])
    file_receipt = read_reference(proof["receipts"]["ordinary-files"])
    bank_ref = file_receipt["bank"]
    bank = read_reference(bank_ref)
    records, logs, current_assertions, comparisons = {}, set(), 0, 0
    for name in report_names:
        reference = identity(ROOT / name)
        record = read_reference(reference)
        cfg, scenario = record["configuration"], record["scenario_id"]
        key = (cfg, scenario)
        assert key in {(c, s) for c in ("Debug", "RelWithDebInfo") for s in ("S1", "S2")} and key not in records
        assert record["schema_version"] == owner.SCHEMA_VERSION == 2 and record["status"] == "passed"
        assert record["classification"] == "real_bootstrap" and record["shortcuts"] == [] and record["bootstrap_claim"] is True
        assert record["presentation"] == "headless" and record["execution_mode"] == "verified_original_predecessors_and_current_ordinary_native_readers"
        assert record["claim_scope"] == ("genuine_normal_start_clearing" if scenario == "S1" else "genuine_normal_start_recurring_spoil_logistics")
        assert record["normal_start_proof"] == owner.NORMAL_START_PROOF
        assert record["fixture"] == {"classification": "real_bootstrap", "shortcuts": [], "proof": owner.NORMAL_START_PROOF}
        assert record["lineage"] == proof["records"][cfg] and record["runner"] == identity(ROOT / "src/tests/multilayer/acceptance/native_ground.py")
        assert record["declared_context"] == {"profile_id": 1, "profile_version": 1, "map_seed": 420, "width": 128, "height": 128,
                                              "calendar_start_year": 1980, "starting_money": 100000, "starting_loan": 100000, "maximum_credit": 300000}
        native = record["native_verification"]["actual"]
        assert native == read_reference(record["native_verification"]["report"])
        assert record["executable"] == native["binary"] and record["config"] == native["config"]
        inspection, count, assertions, case_logs = verify_native(native, bank["records"][cfg]["final-useful"], cfg, bank_ref)
        assert not logs.intersection(case_logs) and len(set(case_logs)) == 4
        logs.update(case_logs)
        lineage = record["lineage"]
        assert record["state_digest"] == owner.compare_state_digest(lineage["final_input"]["path"], inspection["path"], complete_native=True)
        ledgers = owner.capture_ledger([lineage["original_input"]["path"], lineage["final_input"]["path"], inspection["path"]], complete_native=True)
        assert record["ledger"] == json.loads(json.dumps(ledgers))
        art = record["art"]
        assert art["evidence_manifest"] == publication["default_selection"] and art["artifacts"][:4] == [publication[k] for k in art_keys]
        for ref in art["artifacts"]:
            verify_reference(ref)
        assert art["scope"] == "selected_M1_69_families" and art["selected_families"] == 69 and art["selected_sprites"] == 10557
        command = art["command"]
        assert command["exit_status"] == 0 and command["cwd"] == "." and 0 <= command["wall_seconds"] < command["timeout_seconds"] == 110
        assert command["argv"][1:] == [str(ROOT / "media/multilayer/review_m1_art.py"), "--evidence-manifest", str(verify_reference(art["evidence_manifest"]))]
        summary = json.loads(verify_reference(command["log"]).read_text(encoding="utf-8"))
        assert summary == dict(publication["summary"], native_frames=publication["native_frames"])
        current_assertions += assertions
        comparisons += count
        records[key] = {"report": reference, "native_report": record["native_verification"]["report"], "digest": record["state_digest"],
                        "inspection": inspection, "next": native["cases"][0]["outputs"]["next"]}
    assert len(records) == 4 and len(logs) == 16
    scenario_pairs = []
    for cfg in ("Debug", "RelWithDebInfo"):
        left, right = records[(cfg, "S1")], records[(cfg, "S2")]
        for phase in ("inspection", "next"):
            comparison = owner.compare_state_digest(left[phase]["path"], right[phase]["path"], complete_native=True)
            scenario_pairs.append({"configuration": cfg, "phase": phase, "comparison": comparison})
    checks = []
    expected_checks = {"src/tests/multilayer/normal_ground_bootstrap_checks.py": 7, "src/tests/multilayer_acceptance.py": 6}
    for script, name in check_pairs:
        assert script in expected_checks and script not in {c["script"]["path"] for c in checks}
        log_ref = identity(ROOT / name)
        data = verify_reference(log_ref).read_bytes()
        text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
        match = re.search(r"Ran (\d+) tests? in [\d.]+s\s+OK\s*$", text)
        assert match and int(match[1]) == expected_checks[script]
        checks.append({"script": identity(ROOT / script), "log": log_ref, "tests": int(match[1])})
    assert len(checks) == 2
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DEC;master11.4;existing public runner genuine acceptance",
              "classification": "normal_start_public_acceptance", "bootstrap_claim": True, "controller": identity(__file__),
              "proof": owner.NORMAL_START_PROOF, "art_publication": art_ref,
              "records": [{"configuration": cfg, "scenario_id": scenario, **value} for (cfg, scenario), value in records.items()],
              "current_native_cases": 16, "current_native_assertions": current_assertions, "current_complete126_chunk_pairs": comparisons,
              "scenario_pairs": scenario_pairs, "ignored_chunks": [], "focused_checks": checks,
              "historical_mainline_native_cases": proof["mainline_native_cases"], "historical_mainline_native_assertions": proof["mainline_native_assertions"],
              "scope": "genuine normal-start ground clearing,recurring disposal,irregular paid useful tube conversion,original native files/refusal/recovery",
              "remaining": "master11.6 representative performance;human art/demo acceptance;whole unrelated orbital scopes remain pending"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "current_native_cases": 16, "current_native_assertions": current_assertions,
                      "complete126_chunk_pairs": comparisons, "scenario_pairs": len(scenario_pairs)}), flush=True)


def main():
    """Consume four actual public records and both exact focused-test logs without overwriting evidence."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--checks", action="append", nargs=2, metavar=("SCRIPT", "LOG"), required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.checks, args.output)


if __name__ == "__main__":
    main()
