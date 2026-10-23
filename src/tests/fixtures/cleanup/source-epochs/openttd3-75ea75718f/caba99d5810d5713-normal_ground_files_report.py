"""Run and seal complete ordinary readers for the exact normal-world original file bank.

@file normal_ground_files_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All native children are isolated and bounded by the shared original-file controller.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from normal_ground_files import run_stage
from tube_tbm_files import equal_world


def accept(reports, bank_path, bank_sha, output):
    """Require both native readers for every original and every complete saved authority."""
    bank_ref = identity(ROOT / bank_path)
    assert bank_ref["sha256"] == bank_sha
    bank = json.loads(verify_reference(bank_ref).read_text(encoding="utf-8"))
    expected = {(origin, reader, stage) for origin, stages in bank["records"].items()
                for reader in ("Debug", "RelWithDebInfo") for stage in stages}
    records = {}
    for name in reports:
        path = ROOT / name
        actual = json.loads(path.read_text(encoding="utf-8"))
        key = (actual["origin"], actual["reader"], actual["stage"])
        assert key in expected and key not in records
        assert actual["bank"] == bank_ref and actual["classification"] == "normal_start_native_files"
        assert actual["selected"] == bank["records"][key[0]][key[2]]
        assert actual["complete_chunks_equal"] == 126 and actual["ignored_chunks"] == []
        for ref in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
            verify_reference(ref)
        assert [case["action"] for case in actual["cases"]] == ["baseline", "inspect", "next", "reload-next"]
        assert actual["native_cases"] == 4 and actual["native_assertions"] == sum(c["native"]["assertions"] for c in actual["cases"])
        original = verify_reference(actual["selected"]["original"])
        baseline = verify_reference(actual["baseline"])
        inspected = None
        for case in actual["cases"]:
            assert case["children_reaped"] and case["native"]["cases"] == 1 and case["native"]["assertions"] > 0
            text = verify_reference(case["native"]["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert counts and int(counts[1]) == case["native"]["assertions"] and int(counts[2]) == 1
            assert f"NORMAL_GROUND_FILE action={case['action']} preview={int(case['action'] != 'baseline')} completed=1" in text
            assert case["input"] == (inspected if case["action"] == "reload-next" else actual["selected"]["original"])
            verify_reference(case["input"])
            outputs = {k: verify_reference(v) for k, v in case["outputs"].items()}
            assert case["comparisons"]["original_read"] == equal_world(original, outputs["read"])
            assert case["comparisons"]["pure_read"] == equal_world(outputs["read"], outputs["repeat"])
            if case["action"] == "inspect":
                inspected = case["outputs"]["read"]
            else:
                assert case["comparisons"]["untouched_next"] == equal_world(baseline, outputs["next"])
                assert case["comparisons"]["pure_next"] == equal_world(outputs["next"], outputs["next-repeat"])
        records[key] = {"report": identity(path), "actual": actual}
    assert set(records) == expected
    cross = []
    for origin, stages in bank["records"].items():
        for stage in stages:
            first, second = (records[(origin, reader, stage)]["actual"] for reader in ("Debug", "RelWithDebInfo"))
            for index in (0, 2, 3):
                a, b = (row["cases"][index]["outputs"]["next"] for row in (first, second))
                cross.append({"origin": origin, "stage": stage, "action": first["cases"][index]["action"],
                              "first": a, "second": b, "comparison": equal_world(verify_reference(a), verify_reference(b))})
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDC;both ordinary readers/preview/pure writes/reload/full untouched next with all native owners",
              "classification": "normal_start_native_file_acceptance", "bootstrap_claim": False,
              "bank": bank_ref, "controller": identity(__file__), "records": list(records.values()), "cross_reader": cross,
              "original_files": bank["original_files"], "native_cases": sum(r["actual"]["native_cases"] for r in records.values()),
              "native_assertions": sum(r["actual"]["native_assertions"] for r in records.values()),
              "complete126_chunk_comparisons": sum(len(c["comparisons"]) for r in records.values() for c in r["actual"]["cases"]) + len(cross),
              "ignored_chunks": [], "remaining": "6DDD meaningful original refusal/loaded recovery;6DE genuine runner publication"}
    target = ROOT / output
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "native_assertions": result["native_assertions"],
                      "complete126_chunk_comparisons": result["complete126_chunk_comparisons"]}), flush=True)


def main():
    """Run remaining exact stages or seal supplied per-stage reports without replacing existing evidence."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--bank", required=True)
    parser.add_argument("--bank-sha", required=True)
    parser.add_argument("--report", action="append", default=[])
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    reports = list(args.report)
    if args.run:
        bank = json.loads((ROOT / args.bank).read_text(encoding="utf-8"))
        assert identity(ROOT / args.bank)["sha256"] == args.bank_sha
        done = set()
        for path in reports:
            report = json.loads((ROOT / path).read_text(encoding="utf-8"))
            done.add((report["origin"], report["reader"], report["stage"]))
        for origin, stages in bank["records"].items():
            for reader in ("Debug", "RelWithDebInfo"):
                for stage in stages:
                    if (origin, reader, stage) not in done:
                        path = run_stage(args.bank, args.bank_sha, origin, reader, stage)
                        reports.append(path.relative_to(ROOT).as_posix())
    accept(reports, args.bank, args.bank_sha, args.output)


if __name__ == "__main__":
    main()
