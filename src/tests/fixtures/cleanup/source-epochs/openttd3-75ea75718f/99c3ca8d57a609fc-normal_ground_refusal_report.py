"""Seal unchanged native original-world refusals and the complete normal file/recovery handoff.

@file normal_ground_refusal_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Existing dated file/checkpoint/recovery receipts remain immutable authorities.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from normal_ground_layout import fields
from normal_ground_refusal import BANK, BANK_SHA, run_one
from tube_tbm_files import equal_world

RECEIPTS = {
    "working-files": "deabd77e157d3b4e9656550137972cbe47bd4d109dbd4f0c653edf550a872ee8",
    "ordinary-files": "4fc54a2f9e6312cc434df0a691bc23cc12580abdd2c67bba40dd660e8cec3d6e",
    "loaded-recovery": "58cb946c70cbbab888d4ca19f1e704a3c22a903bccfc8b49884f167657d38532",
}


def accept(reports, output):
    """Require all actual native refusals and previously sealed complete original loaded recovery."""
    bank_ref = identity(ROOT / BANK)
    assert bank_ref["sha256"] == BANK_SHA
    bank = json.loads(verify_reference(bank_ref).read_text(encoding="utf-8"))
    receipts, retained = {}, {}
    for name, digest in RECEIPTS.items():
        ref = identity(ROOT / f"logs/acceptance/bootstrap/20261019-native-01/{name}-verification.json")
        assert ref["sha256"] == digest
        receipts[name] = ref
        retained[name] = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
    assert retained["ordinary-files"]["native_cases"] == 192 and retained["ordinary-files"]["complete126_chunk_comparisons"] == 744
    assert retained["loaded-recovery"]["ordinary_files"] == receipts["ordinary-files"]
    records = {}
    for name in reports:
        path = ROOT / name
        actual = json.loads(path.read_text(encoding="utf-8"))
        cfg, scope = actual["configuration"], actual["scope"]
        key = (cfg, scope)
        assert cfg in ("Debug", "RelWithDebInfo") and scope in ("active", "final") and key not in records
        assert actual["classification"] == "normal_start_native_refusal" and actual["bootstrap_claim"] is False
        assert actual["bank"] == bank_ref and actual["children_reaped"]
        selected = bank["records"][cfg]["working-bore" if scope == "active" else "final-useful"]
        assert actual["selected"] == selected
        source = verify_reference(selected["original"])
        for ref in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
            verify_reference(ref)
        text = verify_reference(actual["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == actual["native"]["assertions"] > 0 and int(counts[2]) == actual["native"]["cases"] == 1
        observed = fields(text, "NORMAL_GROUND_REFUSAL_" + scope.upper())
        assert observed == actual["observed"]
        if scope == "active":
            assert observed["cursor"] == 1 and observed["earth"] == 36 and observed["rock"] == 72
            assert observed["traffic_error"] > 0 and observed["nonparticipant"] == 2
            assert observed["solid"] == 30574 and observed["rail_error"] != 65535 and observed["release_error"] != 65535
            assert observed["support"] >= 16384 and observed["conversion_error"] != 65535
        else:
            assert observed["home"] == 24928 and observed["purchase_quote"] > observed["cash"] == selected["cash"]
            assert observed["purchase_depot"] == 25319
            assert observed["funds_error"] != 65535
            assert observed["loan"] == selected["loan"] == 300000 and observed["demolition_error"] != 65535
        assert set(actual["saves"]) == set(actual["comparisons"]) == {"before", "after", "repeat"}
        for stage, ref in actual["saves"].items():
            assert actual["comparisons"][stage] == equal_world(source, verify_reference(ref))
        records[key] = {"report": identity(path), "actual": actual}
    assert set(records) == {(cfg, scope) for cfg in ("Debug", "RelWithDebInfo") for scope in ("active", "final")}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDD/6DD.1;exact original native readers,solid/traffic/unsafe-support/retirement/occupied-depot/funds refusals and physical loaded recovery",
              "classification": "normal_start_file_recovery_acceptance", "bootstrap_claim": False,
              "controller": identity(__file__), "bank": bank_ref, "receipts": receipts, "records": list(records.values()),
              "native_cases": 4, "native_assertions": sum(r["actual"]["native"]["assertions"] for r in records.values()),
              "unchanged_original126_chunk_comparisons": 12, "file_cases": 192, "file_complete126_chunk_comparisons": 744,
              "loaded_recovery_cases": 2, "loaded_recovery_assertions": retained["loaded-recovery"]["native_assertions"],
              "ignored_chunks": [], "remaining": "6DE genuine predecessor-bound S1/S2 publication through existing acceptance runner"}
    target = ROOT / output
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": 4, "native_assertions": result["native_assertions"]}), flush=True)


def main():
    """Run missing independently bounded native refusal cases,then seal all four exact originals."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", default=[])
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    reports, done = list(args.report), set()
    for name in reports:
        report = json.loads((ROOT / name).read_text(encoding="utf-8"))
        done.add((report["configuration"], report["scope"]))
    if args.run:
        for cfg in ("Debug", "RelWithDebInfo"):
            for scope in ("active", "final"):
                if (cfg, scope) not in done:
                    reports.append(run_one(cfg, scope).relative_to(ROOT).as_posix())
    accept(reports, args.output)


if __name__ == "__main__":
    main()
