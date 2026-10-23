"""Seal actual paid excavation, safe withdrawal and non-rail construction acceptance.

@file tbm_nonrail_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md Group15B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The separate original paid first-plant receipt retains its own construction/export authority.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from tbm_nonrail_return import CASES, SOURCES, ROOT
from construction_provider_return import identity
from portal_consumer_report import CONFIGURATIONS, checked

HANDOFF = ("logs/portals/consumer-return/20261019-native-01/riser-verification.json",
           "3c2d976407e3de64ba2d6a1e821b5575c38a40e8999106964099449b871e4de3")


def accept(selection_path, output_path):
    """Accept only a complete exact current matrix and unchanged original paid handoff.

    @param selection_path Explicit report/build-log index; discovery of a latest run is forbidden.
    @param output_path Absent retained receipt beneath repository logs.
    @return Complete acceptance record after current sources, logs and native observations agree.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    cache, seen, records, paid = {}, set(), [], {}
    expected = {(cfg, focus) for cfg in CONFIGURATIONS for focus in CASES}
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in CONFIGURATIONS}
    # Every actual paid workflow and each active/occupied/date guard must pass
    # independently in both readers; another focus cannot fill a missing guard.
    for row in selection["records"]:
        cfg, focus = row["configuration"], row["focus"]
        key = cfg, focus
        assert key in expected and key not in seen, key
        seen.add(key)
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert (report["configuration"], report["focus"], report["tag"]) == (cfg, focus, CASES[focus])
        assert report["native_version"] == 434
        assert report["controller"]["path"] == "src/tests/multilayer/tbm_nonrail_return.py"
        assert report["boundary_runner"]["path"] == "src/tests/multilayer/construction_provider_return.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        # Check each current identity once; dated binaries or changed source
        # cannot silently supply fresh construction acceptance.
        for ref in [report["controller"], report["boundary_runner"], report["binary"], *report["sources"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw
        assert f"Filters: {CASES[focus]}" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assertions, cases = map(int, counts.groups())
        assert cases == report["native"]["cases"] == 1
        assert assertions == report["native"]["assertions"] and assertions > 0
        summary[cfg]["cases"] += cases
        summary[cfg]["assertions"] += assertions
        if focus == "paid":
            paid[cfg] = report["metrics"]
            assert min(paid[cfg].values()) > 0
            assert "protected_retreat=1 withdrawn=1" in raw
        records.append({"report": identity(path), **report})
    assert seen == expected, expected - seen
    assert paid["Debug"] == paid["RelWithDebInfo"]
    handoff_path = ROOT / HANDOFF[0]
    handoff = identity(handoff_path)
    assert handoff["sha256"] == HANDOFF[1]
    retained = json.loads(handoff_path.read_text(encoding="utf-8"))
    assert retained["native_version"] == 434
    assert retained["combined_current_native_cases"] == 28
    assert len(retained["original_same_world_risers"]) == 2
    originals = retained["original_useful_service"]
    assert set(originals) == {"vent_tap", "magma_loop"}
    # Preserve both actual original paid construction/useful comparisons rather
    # than recreating the plant with staged floor, stock or utility participants.
    for model, original in originals.items():
        assert original["active_ticks"] == 74, model
        assert original["useful_electricity"] > 0 and original["cash_spent"] > 0
        checked(original["input"], cache)
        checked(original["output"], cache)
    build_logs = {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS}
    result = {
        "_documentation": "r2mlvs4-tunnel-boring-machines.md15B/original15.1;current actual paid cutting,spoil disposal,whole withdrawal,tube terminal/depot,mine,road and open chamber;original paid geothermal construction/export retained separately",
        "controller": identity(__file__), "selection": identity(selection_path),
        "identity_verifier": identity(ROOT / "src/tests/multilayer/portal_consumer_report.py"),
        "native_version": 434, "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "actual_paid_observations": paid["Debug"], "records": records, "build_logs": build_logs,
        "retained_portal_paid_plant_handoff": handoff, "original_paid_useful_service": originals,
        "artwork": identity(ROOT / "media/baseset/openttd.grf"),
        "classification": "Generated world,company,initial commissioned logistics and receivers are disclosed scenario preconditions;all subsequent cutting,physical spoil carriage,withdrawal and replacement construction use native owners and real company invoices.",
        "remaining": "Plan4-21.4/Plan17 Group16 actual curved/irregular useful tube route,ordinary files/power recovery and later master/human/network/operator gates remain pending.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "actual_paid_observations": paid["Debug"]}), flush=True)
    return result


def main():
    """Select the exact final construction matrix and an absent closure receipt.

    @return None; incomplete, duplicate, stale or altered original evidence refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
