"""Accept actual native1985 finite RARE source worlds before ordinary source-file and useful-route consumers.

@file shield_source_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original reserve/product stock remains native-owned; no useful delivery or Electronics manufacture is claimed.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native
from shield_source_gate import CASES


def main():
    """Require both real native source producers, current prerequisite owners and complete unchanged original worlds.

    @return None; missing finite conservation, changed inputs, stale current targets or unequal complete worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stocks", nargs=2, required=True)
    parser.add_argument("--gates", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo"); cache, stocks, gates, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Actual original stock and each current owner gate remain mandatory.
    # Matching final units cannot replace native purchases, reserve debit,
    # source provenance or normal content/date initialization.
    for names, focus, selected in ((args.stocks, "stock-producer", stocks), (args.gates, "producer-gate", gates)):
        for name in names:
            path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
            cfg = report["configuration"]; assert cfg in configurations and cfg not in selected and report["focus"] == focus
            if focus == "stock-producer":
                assert len(report["native"]) == 2
                source = report["source"]
                assert source["year"] == 1985 and source["tick"] == source["opportunities"] and 0 < source["tick"] < 2048
                assert source["units"] == source["extracted"] == source["reserve_before"] - source["reserve"] == 20
                assert source["dispatched"] == source["loan"] == source["income"] == 0 and source["cash"] > 0
                assert len(set(source["parts"])) == len(set(source["industries"])) == 2
                assert report["native"][0]["output"] == report["native"][1]["input"]
            else:
                assert [process["case"] for process in report["native"]] == [case for case, _ in CASES]
            for process in report["native"]:
                count_native(process, summary[cfg])
            selected[cfg] = report; records.append({"report": identity(path), **report})
    assert stocks.keys() == gates.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert stocks["Debug"]["source"] == stocks["RelWithDebInfo"]["source"]
    worlds = []
    # Both empty author inputs and actual earned source worlds compare every
    # complete native chunk; no map, definition, packet, clock or account owner
    # is excluded because the observed stock scalar already matches.
    for index, boundary in enumerate(("empty", "stock")):
        left, right = (stocks[cfg]["native"][index]["output"] for cfg in configurations)
        assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
        worlds.append({"boundary": boundary, "left": left, "right": right, "complete_chunks_exact": 126})
    for cfg in configurations:
        assert stocks[cfg]["binary"] == gates[cfg]["binary"]
    builds = []
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15EB.1; actual1985 native finite RARE source/stopped sealed carrier; ordinary-file15EB.2 and useful-route15EC remain pending",
        "controller": identity(__file__), "summary": summary, "records": records, "source": stocks["Debug"]["source"],
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "complete_world_pairs": worlds, "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": stocks["Debug"]["fixture_classification"],
        "remaining": "15EB.2 ordinary repeated/opposite native source loading and untouched next opportunities precede actual paid Shield/conventional Deep-Underground-Surface delivery; runtime silhouette and TBM/bootstrap/human gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "source": result["source"]}))


if __name__ == "__main__":
    main()
