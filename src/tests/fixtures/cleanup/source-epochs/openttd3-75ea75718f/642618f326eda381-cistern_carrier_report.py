"""Accept only the native-content/carrier prerequisite for the1980 Cistern source.

@file cistern_carrier_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBA.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native source production, physical input haulage and useful chemical output remain unchecked after this prerequisite.
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


def main():
    """Require both ordinary original-carrier prerequisites and current finite-mine/processor producer gates.

    @return None; changed references, failed native counts or mismatched original native worlds prevent the absent receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--producer-reports", nargs=4, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    cache, carriers, producers, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # A real original carrier and ordinary content startup precede any source
    # mutation. Existing mine/processor declarations alone cannot satisfy the
    # cross-plan executable producer gate or claim a useful freight shipment.
    for name in (*args.reports, *args.producer_reports):
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in configurations
        if focus == "carrier-contract":
            assert cfg not in carriers and len(report["native"]) == 2
            carriers[cfg] = report
            processes = report["native"]
        else:
            assert focus in ("mine-producer", "factory-producer") and (cfg, focus) not in producers
            producers[cfg, focus] = report
            processes = [report["native"]]
        for process in processes:
            count_native(process, summary[cfg])
        records.append({"report": identity(path), **report})
    assert carriers.keys() == set(configurations)
    assert producers.keys() == {(cfg, focus) for cfg in configurations for focus in ("mine-producer", "factory-producer")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    assert carriers["Debug"]["carrier"] == carriers["RelWithDebInfo"]["carrier"]
    assert carriers["Debug"]["source_contracts"] == carriers["RelWithDebInfo"]["source_contracts"]
    worlds = []
    # Compare complete native worlds, including the empty author input and
    # actual purchased/refitted carrier. Identical scalar capacity or IDs
    # cannot hide changed cargo definitions, RNG or native company metadata.
    for index in range(2):
        left, right = [carriers[cfg]["native"][index]["output"] for cfg in configurations]
        assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
        worlds.append({"boundary": "empty" if index == 0 else "carrier", "left": left, "right": right, "complete_chunks_exact": 126})
    builds = []
    # Retain exact original build bytes; decode only to verify successful
    # current test-target completion, without rewriting prior diagnostics.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBA; ordinary native original-tanker and executable source-owner prerequisites only; useful chemical source remains pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "carrier": carriers["Debug"]["carrier"], "source_contracts": carriers["Debug"]["source_contracts"],
        "records": records, "complete_world_pairs": worlds, "exact_identities_verified": len(cache), "builds": builds,
        "native_version": 434, "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": "An empty prepared world uses native saved-setting defaults, normal calendar/pathfinder options and StartupEconomy, then ordinary application startup supplies actual distributed cargo properties. Explicit author funds purchase/refit an original tanker. Pure source estimates preserve cash/RNG/industry count; no source stock, actual shaft travel or useful chemical output is claimed.",
        "remaining": "15DBB actual finite Salt/original Oil production,15DBC physical delivery/real CHEM conversion and15DBD full source verification precede parent15DB.1/.2 and dependent Cistern routing.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "source_contracts": result["source_contracts"]}))


if __name__ == "__main__":
    main()
