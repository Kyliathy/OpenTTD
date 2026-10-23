"""Bind the completed selected lifecycle owner receipts for the master return.

@file native_lifecycle_handoff.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15E.
This handoff combines completed technical evidence without re-running native owners
or granting later-profile, network, operator or human acceptance.
"""

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity


def publish(tube_path, thermal_path, output):
    """Seal the exact completed actual tube and thermal owner return.

    @param tube_path Complete accepted current tube lifecycle/native original-file receipt.
    @param thermal_path Complete accepted current thermal lifecycle/paid original-file receipt.
    @param output Absent immutable combined receipt beneath repository logs.
    @return None; originals, public source contracts and gameplay quantities stay unchanged.
    """
    output = (ROOT / output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    paths = [ROOT / tube_path, ROOT / thermal_path]
    tube, thermal = [json.loads(path.read_text(encoding="utf-8")) for path in paths]
    for receipt in (tube, thermal):  # Completed owner receipts remain opaque dated execution evidence; their current validator identity and exact outer bytes bind this audit handoff.
        assert receipt["native_version"] == 434
        assert identity(ROOT / receipt["controller"]["path"]) == receipt["controller"]
        assert receipt["native_cases"] > 0 and receipt["native_assertions"] > 0
    assert tube["artwork"] == thermal["artwork"]
    assert tube["native_cases"] == 52 and thermal["native_cases"] == 70
    assert len(tube["cross_reader_world_pairs"]) == 4 and len(thermal["cross_reader_world_pairs"]) == 10
    for reference in thermal["accepted_original_paid_receipts"]:  # The actual paid-TBM/material-carriage/rebuild/service chain cannot be replaced by isolated lifecycle unit inputs.
        assert identity(ROOT / reference["path"]) == reference
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md15E/master9.5B;completed selected actual tube/vent/collector/bothgenerator/riser lifecycle rows;wholelaterprofile/network/human/operator gates remain pending",
              "controller": identity(__file__), "owner_receipts": [identity(path) for path in paths],
              "native_cases": tube["native_cases"] + thermal["native_cases"], "native_assertions": tube["native_assertions"] + thermal["native_assertions"],
              "complete_126_chunk_cross_reader_pairs": len(tube["cross_reader_world_pairs"]) + len(thermal["cross_reader_world_pairs"]),
              "original_thermal_next_comparisons": thermal["original_next_comparisons"],
              "strict_native_revision_appends": len(thermal["strict_revision_diagnostic_appends"]),
              "focused_revision_refusal_checks": thermal["revision_checks"], "native_version": 434, "artwork": tube["artwork"],
              "selected_rows": {"tube_depot_interchange": "actual occupied refusal/empty loss,original packet/orders/carry,shared unused grants,physical rebuild and exact original files",
                                "tube_company": "surviving custody/fresh paid recovery;explicit native terminal liquidation;no company/vehicle/source IDreuse rights",
                                "vent_collector": "original natural field/capacity/cold claim;actual construction stock/work;retired Heat grants/history;fresh source identity",
                                "both_operating_generators": "actual paid original retirement/rebuild/carriage/construction/useful service,service fractions/settled conversion,company and IDreuse",
                                "native_riser": "same paid machinery after seal/reopen;old physical portal remains terminal after coordinate/revision reuse;new paid edge only,spent joules retained"},
              "policy": "docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-utility-lifecycle-policy.md",
              "master_return": "9.5B complete;next10.8 actualpaidTBM/tube final-use integration",
              "remaining": "WholePlan15-15.3/15.4 undispatched later-profile/orbital rows;actualhost/clientjoins,operator and human gates;future directprocessing factories;master10.8 final-use and later queue."}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "cases": result["native_cases"], "assertions": result["native_assertions"], "world_pairs": result["complete_126_chunk_cross_reader_pairs"]}), flush=True)


def main():
    """Bind explicit complete selected owner receipts into an absent master handoff.

    @return None; exact evidence must already exist before policy closure.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--tube", required=True)
    parser.add_argument("--thermal", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    publish(args.tube, args.thermal, args.output)


if __name__ == "__main__":
    main()
