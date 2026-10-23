"""Accept actual original Cistern ordinary files and complete opposite-reader continuation before runtime raster.

@file cistern_useful_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All original upstream owners remain mandatory; moving silhouette, other models and paid TBM/bootstrap retain later gates.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks, compare_next_step
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native
from cistern_useful_files import validate_points
from cistern_useful_read import WRITER_RECEIPT

FIELDS = ("scene", "step", "tick", "parts", "references", "snapshot")


def main():
    """Require all six opposite-writer ordinary scenes, pure repeats, complete native next steps and current owner targets.

    @return None; changed originals, stale current records, missing participant continuation or unequal complete worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reader-manifest", required=True)
    parser.add_argument("--current-writers", nargs=2, required=True)
    parser.add_argument("--current-routes", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / WRITER_RECEIPT
    assert identity(receipt)["sha256"] == "ba5951c8745c7c51b0227a5038c9f15ab8b58b953744f38196ca7ac7889a06fe"
    accepted = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in accepted["records"] if row.get("phase") == "write"}
    manifest = ROOT / args.reader_manifest; names = json.loads(manifest.read_text(encoding="utf-8-sig"))["reports"]
    assert len(names) == len(set(names)) == 24
    configurations = ("Debug", "RelWithDebInfo"); cache, readers, writers, routes, records = {}, {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Sealed writers retain exact report and original file identities. Their
    # earlier binaries/source hashes are opaque accepted history; all current
    # readers and fresh writer/route boundaries independently verify today.
    for writer in originals.values():
        assert identity(ROOT / writer["report"]["path"]) == writer["report"]
        for point in writer["checkpoints"]:
            assert identity(ROOT / point["input"]["path"]) == point["input"]
    for name in (*names, *args.current_writers, *args.current_routes):
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations
        phase = report.get("phase", "route")
        if phase in ("reload", "step"):
            scene = report["case"]; assert scene in range(6) and (cfg, phase, scene) not in readers
            assert report["writer_receipt"] == identity(receipt) and len(report["native"]) == 2
            readers[cfg, phase, scene] = report; processes = report["native"]
        elif phase == "write":
            assert cfg not in writers; writers[cfg] = report; processes = [report["native"]]
        else:
            assert phase == "route" and report["focus"] == "cistern-useful" and cfg not in routes
            routes[cfg] = report; processes = [report["native"]]
        for process in processes:
            count_native(process, summary[cfg])
        records.append({"report": identity(path), **report})
    assert writers.keys() == routes.keys() == set(configurations)
    assert readers.keys() == {(cfg, phase, scene) for cfg in configurations for phase in ("reload", "step") for scene in range(6)}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    pairs, current_pairs = [], []
    original_grids = {cfg: {(p["scene"], p["step"]): p for p in originals[cfg]["checkpoints"]} for cfg in configurations}
    # Fresh current writers must preserve every accepted original boundary,
    # not merely their final material balance. This also checks that the new
    # reader observer did not alter any shared native driver or save behavior.
    for cfg in configurations:
        original = originals[cfg]; current = writers[cfg]
        prepared = json.loads((ROOT / current["prepare_report"]["path"]).read_text(encoding="utf-8"))
        validate_points(current["checkpoints"], prepared["prepared"], routes[cfg]["result"])
        assert current["prepare_report"] == routes[cfg]["prepare_report"] == original["prepare_report"]
        assert current["native"]["input"] == routes[cfg]["native"]["input"] == original["native"]["input"]
        assert current["binary"] == routes[cfg]["binary"]
        for point in current["checkpoints"]:
            old = original_grids[cfg][point["scene"], point["step"]]
            assert {key: point[key] for key in FIELDS} == {key: old[key] for key in FIELDS}
            assert compare_chunks(ROOT / point["input"]["path"], ROOT / old["input"]["path"], None) == 126
            current_pairs.append({"configuration": cfg, "scene": point["scene"], "step": point["step"],
                                  "current": point["input"], "original": old["input"], "complete_chunks_exact": 126})
        assert compare_chunks(ROOT / routes[cfg]["native"]["output"]["path"], ROOT / original_grids[cfg][5, 0]["input"]["path"], None) == 126
    # Every reader consumes its exact opposite original writer. First load
    # and repeated resave preserve all126 chunks; continuation includes both
    # original clocks, every front/packet/recipe/account and native GLOG.
    for (cfg, phase, scene), report in readers.items():
        opposite = configurations[1 - configurations.index(cfg)]; original = originals[opposite]
        assert report["writer"] == original["report"] and report["checkpoint"] == original_grids[opposite][scene, 0]
        assert report["binary"] == writers[cfg]["binary"]
        first, second = (ROOT / process["output"]["path"] for process in report["native"])
        point, baseline = original_grids[opposite][scene, 0], original_grids[opposite][scene, 1]
        assert all(process["preview_pure"] for process in report["native"])
        if phase == "reload":
            assert report["first_load_complete_chunks_exact"] == report["repeated_complete_chunks_exact"] == 126
            assert all(process["observed_snapshot"] == point["snapshot"] for process in report["native"])
            assert report["native"][0]["input"] == point["input"] and report["native"][1]["input"] == report["native"][0]["output"]
            assert compare_chunks(ROOT / point["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
        else:
            assert report["uninterrupted_baseline"] == baseline["input"]
            assert report["native"][0]["input"] == baseline["input"] and report["native"][1]["input"] == point["input"]
            assert all(process["observed_snapshot"] == baseline["snapshot"] for process in report["native"])
            assert compare_chunks(ROOT / baseline["input"]["path"], first, None) == 126
            assert report["next_step"] == compare_next_step(first, second, point["tick"], baseline["tick"])
            assert report["next_step"]["complete_chunks_exact"] == 126
    # Independent readers must agree on complete original and next-opportunity
    # outputs across builds, with no catalogue, diagnostic or map exclusions.
    for phase in ("reload", "step"):
        for scene in range(6):
            for index in range(2):
                left, right = (readers[cfg, phase, scene]["native"][index]["output"] for cfg in configurations)
                assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
                pairs.append({"phase": phase, "scene": scene, "boundary": index, "left": left, "right": right, "complete_chunks_exact": 126})
    builds = []
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log; builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DDC/original15DD.1; complete original Cistern ordinary files/native next opportunities; runtime raster15DD.2 remains pending",
        "controller": identity(__file__), "writer_receipt": identity(receipt), "reader_manifest": identity(manifest),
        "summary": summary, "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()), "records": records,
        "complete_cross_reader_pairs": pairs, "current_original_writer_pairs": current_pairs,
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{target}.exe") for cfg in configurations for target in ("openttd", "openttd_test")],
        "continuation_contract": "All six actual loaded/final original worlds preserve all nine parts, four industries/two recipes, six stations, raw histories/reserve/unused Oil, packet provenance, native current/old quarters, cargo/engine definitions and paid power. Ordinary first loading, copied preview, repeated saves and every opposite-reader complete next opportunity retain all126 native chunks including GLOG exactly. Both simulation and independent client RNGs are checked for preview/save purity; only saved simulation RNG defines the canonical file boundary. No reconstruction or repair runs.",
        "fixture_classification": writers["Debug"]["fixture_classification"],
        "remaining": "15DD.2 actual moving loaded model11 pixels/picking before15D.1; later Shield/Forge, explicit conventional Underground-Deep chain, paid TBM/bootstrap and human/operator gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "cross_reader_pairs": len(pairs), "current_original_writer_pairs": len(current_pairs)}))


if __name__ == "__main__":
    main()
