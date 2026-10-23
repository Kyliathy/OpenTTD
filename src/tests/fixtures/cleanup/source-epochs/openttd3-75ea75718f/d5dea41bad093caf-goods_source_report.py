"""Accept the original1975 Goods producer before dependent Crate routing.

@file goods_source_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only this Surface source/receiver prerequisite is accepted; capsule files/rendering and bootstrap retain later gates.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native


def main():
    """Verify both independent original native producer runs and their exact current test builds.

    @return None; stale identities, different earned material/route or failed native logs prevent the absent receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True, help="Current Debug and optimized test targets; production code is unchanged.")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    cache, records, sources, summary = {}, [], {}, {}
    # Exact original diagnostics and current source/binary hashes accompany
    # both independently earned worlds; a native count alone is insufficient.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in sources
        summary[cfg] = {"cases": 0, "assertions": 0}
        count_native(report["native"], summary[cfg])
        source = report["source"]
        assert source["converted"] > 30 and source["primary_opportunities"] > 0 and source["route_ticks"] > 0
        assert source["houses"] > 0 and source["town"] > 0 and source["berth"] > 0 and source["wood_income"] > 0
        sources[cfg] = source
        records.append({"report": identity(path), **report})
    assert sources.keys() == set(configurations)
    assert sources["Debug"] == sources["RelWithDebInfo"]
    assert summary["Debug"] == summary["RelWithDebInfo"]
    builds = []
    # Retain byte-exact build diagnostics, decoding their BOM only to verify
    # successful test target publication; no historical producer is overwritten.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CB.1; actual1975 native Forest/physical Wood train/Sawmill Goods output and real town demand gate only",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "source": sources["Debug"], "exact_identities_verified": len(cache), "builds": builds,
        "binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "native_version": 434, "packed_art": packed,
        "fixture_classification": "Prepared completed native industry footprints and generated funded town are boundary inputs. Original Forest batches exceed its unchanged30-unit dispatch minimum. Real orders, native loading and physical complete empty returns earn all39 Wood-to-Goods units; no manufactured stock, arrival or order skip. Positive fixture prices establish debits, not balance. No production source changed.",
        "remaining": "Actual same-world1975 Crate/conventional/town delivery, ordinary files/runtime silhouette, later model dates and Underground/Deep chain remain original15.2; actual TBM/bootstrap and human/operator gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "source": sources["Debug"]}))


if __name__ == "__main__":
    main()
