"""Accept the actual1975 Crate useful route before ordinary file and runtime silhouette work.

@file crate_useful_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CBB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source machinery, native town and prepared floor/shafts remain explicit boundaries; no TBM/bootstrap or human acceptance is inferred.
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
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native


def main():
    """Require independent actual routes, current affected producers and four successful target builds.

    @return None; changed identities, missing original custody or inconsistent native observations prevent the absent receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--affected-reports", nargs=6, required=True, help="Current source, Mole and ordered-transfer producers per build.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    receipt = ROOT / "logs/tubes/goods-source/20261018-native-01/verification-2.json"
    assert identity(receipt)["sha256"] == "5885ed12ed02ce80fafb0325102ef4df307c25286ba29eca0b18da7461cbf33c"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    cache, routes, affected, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Current native reports retain all source/binary/log identities. The
    # original stronger source receipt is opaque dated evidence, not a request
    # to compare its historical binary hash with a subsequently rebuilt test.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in routes
        assert report["source_receipt"] == identity(receipt)
        route = report["route"]
        assert route["delivered"] == history["source"]["converted"] == 39
        assert route["income"] > 0 and route["consumed_j"] > 0 and route["native_quarter_history"] > 0
        log = (ROOT / report["native"]["log"]["path"]).read_text(encoding="utf-8")
        source = re.search(r"GOODS_SOURCE year=1975 source=original_forest converted=(\d+) primary_opportunities=(\d+) route_ticks=(\d+) town=(\d+) houses=(\d+) berth=(\d+) wood_income=(\d+)", log)
        assert source is not None
        assert dict(zip(("converted", "primary_opportunities", "route_ticks", "town", "houses", "berth", "wood_income"), map(int, source.groups()))) == history["source"]
        town = re.search(r"CRATE_TOWN berth=(\d+),(\d+) houses=(\d+) preserved=true", log)
        assert town is not None and int(town[3]) == history["source"]["houses"]
        report["native_town_approach"] = dict(zip(("x", "y", "houses"), map(int, town.groups())))
        count_native(report["native"], summary[cfg])
        routes[cfg] = report
        records.append({"report": identity(path), **report})
    # These affected owners are the actual source, shared native movement and
    # joined modal transfer boundaries. No broad regression or unrelated art
    # fixture can replace their current isolated executable evidence.
    for name in args.affected_reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus = report["configuration"], report.get("focus", "source")
        assert cfg in configurations and focus in ("source", "mole", "ordered-producer") and (cfg, focus) not in affected
        if focus == "source":
            assert report["source"] == history["source"]
        count_native(report["native"], summary[cfg])
        affected[cfg, focus] = report
        records.append({"report": identity(path), **report})
    assert routes.keys() == set(configurations)
    assert affected.keys() == {(cfg, focus) for cfg in configurations for focus in ("source", "mole", "ordered-producer")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    assert routes["Debug"]["route"] == routes["RelWithDebInfo"]["route"]
    assert routes["Debug"]["native_town_approach"] == routes["RelWithDebInfo"]["native_town_approach"]
    assert affected["Debug", "mole"]["route"] == affected["RelWithDebInfo", "mole"]["route"]
    builds = []
    # Decode original PowerShell diagnostics only for successful target checks;
    # their exact bytes and every earlier diagnostic/receipt remain immutable.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CBB.1; actual1975 native original Goods conventional/tube/conventional town route and whole empty returns; files/runtime silhouette remain15CC",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "route": routes["Debug"]["route"], "native_town_approach": routes["Debug"]["native_town_approach"],
        "source_receipt": identity(receipt), "exact_identities_verified": len(cache), "builds": builds,
        "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "accounting_contract": "Every39 original Goods unit remains in actual Sawmill/queues/holds or useful town receipt. Native quarter rollover is retained; current and native historical income/cargo records verify no pre-final transfer revenue and no replay after empty returns. No scalar/clock/physical normalization.",
        "fixture_classification": "Completed original machinery, funded generated town, prepared excavated floor/commissioned shafts and positive fixture prices remain boundary inputs. The actual empty Wood consist is stopped/sold through native commands. Native estimate-selected approach and legal town-road crossings preserve every demand house; actual acceptance is rechecked. Capsules never cross either shaft.",
        "remaining": "15CC ordinary files and actual Crate runtime silhouette precede15C.1 closure;1980/1985/2005 models, Underground/Deep conventional chain, actual TBM/bootstrap and human/runtime-gallery/operator gates remain pending.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "route": result["route"]}))


if __name__ == "__main__":
    main()
