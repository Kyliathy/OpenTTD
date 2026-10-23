"""Accept the complete actual1980 chemical source handoff before Cistern transport.

@file cistern_source_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Exact original carrier/stock/haul receipts remain immutable; this gate verifies current original-world loading and producer boundaries.
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
from cistern_source_ready import HAUL_RECEIPT
from cistern_calendar_files import compare_capsule_calendar


def main():
    """Require exact original source provenance, current ordinary loaded owners/next work, affected producers and four successful targets.

    @return None; changed originals, missing current gates or unequal complete native worlds prevent the absent handoff receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--producer-reports", nargs=4, required=True)
    parser.add_argument("--calendar-reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / HAUL_RECEIPT
    assert identity(receipt)["sha256"] == "31c37be8f6f7cf061eee94c8e66ff8790774c9937c3097b6bfb3d49f4a5688b1"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    configurations = ("Debug", "RelWithDebInfo")
    hauls = {row["configuration"]: row for row in history["records"]}
    cache, readers, producers, calendars, records, pairs = {}, {}, {}, {}, [], []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Current acceptance continues the exact completed original source. Sealed
    # carrier/stock/haul histories are verified as opaque immutable artifacts,
    # never recursively reinterpreted as current compiler/source identities.
    for name in (*args.reports, *args.producer_reports, *args.calendar_reports):
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg, focus = report["configuration"], report["focus"]; assert cfg in configurations
        if focus == "source-ready":
            assert cfg not in readers and len(report["native"]) == 2
            opposite = configurations[1 - configurations.index(cfg)]; original = hauls[opposite]; result = original["result"]
            assert report["haul_receipt"] == identity(receipt) and report["haul_report"] == original["report"]
            readers[cfg] = report; processes = report["native"]
            for step, proof in enumerate(processes):
                assert proof["input"] == original["native"]["output"]
                observed = proof["observed"]; assert observed["step"] == step and observed["tick"] == result["tick"] + step
                for key in ("chemicals", "unused_oil", "unused_salt", "income", "loan", "reserve"):
                    assert observed[key] == result[key]
                source, before, first, repeat = [ROOT / proof[key]["path"] for key in ("input", "before", "output", "repeat")]
                assert proof["original_load"] == compare_capsule_calendar(source, before, proof["capsule_definitions"], result["year"])
                assert compare_chunks(first, repeat, None) == 126
                if step == 0:
                    assert compare_chunks(before, first, None) == 126
        elif focus == "capsule-calendar":
            assert cfg not in calendars and len(report["native"]) == 5
            calendars[cfg] = report; processes = report["native"]
        else:
            assert focus in ("mine-producer", "factory-producer") and (cfg, focus) not in producers
            producers[cfg, focus] = report; processes = [report["native"]]
        for process in processes:
            count_native(process, summary[cfg])
        records.append({"report": identity(path), **report})
    assert readers.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert producers.keys() == {(cfg, focus) for cfg in configurations for focus in ("mine-producer", "factory-producer")}
    assert calendars.keys() == set(configurations)
    # Match every untouched/next and repeated output across both independent
    # native readers, including complete company/industry/history/vehicle and
    # map/clock/RNG ownership rather than a selected aggregate cargo scalar.
    for step in (0, 1):
        assert readers["Debug"]["native"][step]["observed"] == readers["RelWithDebInfo"]["native"][step]["observed"]
        for boundary in ("before", "output", "repeat"):
            left, right = [readers[cfg]["native"][step][boundary] for cfg in configurations]
            assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
            pairs.append({"step": step, "boundary": boundary, "left": left, "right": right, "complete_chunks_exact": 126})
    ancestors = [identity(receipt)]; cursor = history
    # Retain the original actual physical source lineage down to the carrier
    # prerequisite. These receipts preserve exact inputs/outputs and their
    # explicit scenario boundaries after later test instrumentation changes.
    for key in ("stock_receipt", "carrier_receipt"):
        path = ROOT / cursor[key]["path"]; assert identity(path) == cursor[key]
        ancestors.append(identity(path)); cursor = json.loads(path.read_text(encoding="utf-8"))
    builds = []
    # All four current targets must finish successfully before this actual
    # source can become a dependent consumer's accepted native file handoff.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log; builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBD/original15DB.2 strengthened by15DCBA canonical native capsule launch dates; actual Cistern route remains pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "complete_world_pairs": pairs, "exact_identities_verified": len(cache), "source_lineage": ancestors,
        "source": history["result"], "original_source_worlds": [hauls[cfg]["native"]["output"] for cfg in configurations],
        "current_source_worlds": [readers[cfg]["native"][0]["output"] for cfg in configurations],
        "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "file_contract": "Each current opposite-build reader loads the exact completed original source. All125 non-ENGN original chunks, including GLOG, remain exact before work. Within ENGN only the five exact native capsule catalogue intro dates and date-valid pending public availability change; every other byte and ordinary engine remains exact. Complete126-chunk repeated/untouched/cross-build outputs match. One real full-native opportunity retains output, unused inputs, payments and whole stopped empty consists without fixture repair, reconstruction or replay.",
        "fixture_classification": "Real command-funded source/rail and bounded original Oil/finite Salt production plus physically driven native deliveries earn20CHEM and six genuine unused Oil. Inherited prepared floor/completed machinery/Connected shaft and pre-acquisition author funds remain explicit boundaries. No useful Cistern route, receiver operation, paid TBM/bootstrap, runtime gallery or human acceptance is claimed.",
        "remaining": "15DC must consume these exact original source worlds to earn the1980 Cistern/conventional CHEM-to-PCHM useful route;15DD owns its files and native raster.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "source": result["source"]}))


if __name__ == "__main__":
    main()
