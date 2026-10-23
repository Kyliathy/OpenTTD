"""Accept original tube refusals and affected conventional portal travel.

@file tube_refusal_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15A.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Adversarial boundary fixtures do not establish useful introduction-date routes or bootstrap.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from tube_refusal_checks import TAGS
from tube_sprite_report import verify_references


def main():
    """Require every isolated refusal/affected travel scope and successful current targets.

    @return None; stale evidence, an empty native selection or any failed check forbids closure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2 * len(TAGS), required=True)
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True, help="Absent immutable receipt under repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    cache, grid, records = {}, set(), []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Each actual native refusal or affected portal driver is isolated. A
    # passing declaration/query cannot replace the callback that formerly
    # admitted a capsule, or ordinary freight/TBM movement through that owner.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in configurations and focus in TAGS and (cfg, focus) not in grid
        process = report["native"]
        log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
        count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert count is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
        assert process["cases"] > 0 and process["assertions"] > 0
        grid.add((cfg, focus))
        summary[cfg]["cases"] += process["cases"]
        summary[cfg]["assertions"] += process["assertions"]
        records.append({"report": identity(path), **report})
    assert grid == {(cfg, focus) for cfg in configurations for focus in TAGS}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    builds = []
    # Preserve native PowerShell build diagnostics byte-for-byte. Decoding
    # only checks successful targets, never rewrites historical/current logs.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15A/original15.1; exact command/native refusal and affected ordinary portal drivers; useful15.2 routes remain pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "exact_identities_verified": len(cache), "builds": builds,
        "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "reproduced_callback_gap": identity(ROOT / "logs/tubes/refusal-boundaries/20261018-015045-dac385b463/cross-layer.log"),
        "refusal_contract": "Solid/locked floors, independent Deep/model dates, ordinary/capsule domain, unsafe retained work support, disconnected service/missing grants and actual opposite-layer/shaft entry refuse through native owners. Original loaded packet/physical/order/geology/cash/RNG/source observations remain exact. The actual shaft callback checks capsule domain before magazine/visibility handling; ordinary freight/TBM admission remains producer-owned.",
        "fixture_classification": "Prepared floor, commissioned portal, mechanical adversarial masks and loaded17-unit stock are explicit boundary inputs; no useful-route/bootstrap, runtime gallery or human acceptance claim.",
        "remaining": "15.2 useful introduction-date routes, selected runtime gallery and human/operator gates remain pending.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}))


if __name__ == "__main__":
    main()
