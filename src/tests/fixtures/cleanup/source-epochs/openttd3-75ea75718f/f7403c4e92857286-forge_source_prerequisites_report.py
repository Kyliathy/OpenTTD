"""Accept actual2005 Forge prerequisites and unchanged original1985 Shield worlds after shared author extraction.

@file forge_source_prerequisites_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBA.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Passing definitions do not establish source stock, manufactured RFMT, useful movement or TBM bootstrap.
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
from forge_source_prerequisites import CASES

SHIELD_RECEIPT = "logs/tubes/shield-source/20261018-native-01/stock-verification.json"


def main():
    """Require both current native/producer gates, original Shield preservation and completed test builds.

    @return None; missing native owners, changed original files or unequal full worlds prevent the prerequisite handoff.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--producer-reports", nargs=2, required=True)
    parser.add_argument("--shield-reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    receipt = ROOT / SHIELD_RECEIPT
    assert identity(receipt)["sha256"] == "152161e841195806a4437c3d74f463115b6dfe8f18f0372aa7766e66144abe3a"
    historical = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in historical["records"] if row.get("focus") == "stock-producer"}
    records, selected, cache = [], {}, {}
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    shield_pairs = []
    # All current reports bind current sources/binaries/native logs. Earlier
    # accepted Shield receipts remain opaque; only their exact original world
    # identities are re-read to prove the shared author did not change them.
    for focus, names in (("native", args.reports), ("producers", args.producer_reports), ("stock-producer", args.shield_reports)):
        for name in names:
            path = ROOT / name
            report = json.loads(path.read_text(encoding="utf-8"))
            verify_references(report, cache)
            cfg = report["configuration"]
            assert cfg in configurations and report["focus"] == focus and (cfg, focus) not in selected
            if focus == "native":
                values = report["definitions"]
                assert values["year"] == 2005 and values["cash"] == 32000000 and values["capacity"] == 80
                assert all(values[key] == 0 for key in ("tick", "loan", "vehicles", "industries", "stations", "packets"))
                assert {row["label"] for row in report["deposits"]} == {"CORE", "COAL"}
                assert len(report["native"]) == 2
            elif focus == "producers":
                assert {row["case"] for row in report["native"]} == {case for case, _ in CASES}
                assert len(report["native"]) == len(CASES)
            else:
                original = originals[cfg]
                assert report["source"] == original["source"]
                assert len(report["native"]) == len(original["native"]) == 2
                for current, previous in zip(report["native"], original["native"]):
                    left, right = current["output"], previous["output"]
                    assert identity(ROOT / right["path"]) == right
                    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
                    shield_pairs.append(dict(configuration=cfg, current=left, original=right, complete_chunks_exact=126))
            for process in report["native"]:
                count_native(process, summary[cfg])
            selected[cfg, focus] = report
            records.append(dict(report=identity(path), **report))
    assert selected.keys() == {(cfg, focus) for cfg in configurations for focus in ("native", "producers", "stock-producer")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    left, right = (selected[cfg, "native"] for cfg in configurations)
    assert left["definitions"] == right["definitions"] and left["deposits"] == right["deposits"]
    world_pairs = []
    # Both independently authored and normally reconstructed2005 worlds retain
    # every native chunk. No engine/cargo/account or diagnostic owner is dropped
    # merely because both empty inventories happen to contain zero units.
    for index in range(2):
        first, second = left["native"][index]["output"], right["native"][index]["output"]
        assert compare_chunks(ROOT / first["path"], ROOT / second["path"], None) == 126
        world_pairs.append(dict(boundary=index, left=first, right=second, complete_chunks_exact=126))
    builds = []
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FBA; actual2005 native Forge source prerequisite handoff, no cargo/route achievement",
        controller=identity(__file__), summary=summary, records=records,
        native_cases=sum(row["cases"] for row in summary.values()), native_assertions=sum(row["assertions"] for row in summary.values()),
        complete_world_pairs=world_pairs, preserved_shield_original_pairs=shield_pairs,
        definitions=left["definitions"], deposits=left["deposits"], shield_receipt=identity(receipt),
        builds=builds, exact_identities_verified=len(cache), native_version=434,
        test_binaries=[identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        fixture_classification=left["fixture_classification"],
        remaining="15FBB actual paid finite CORE/Coal stocks/real stopped carriers/CFMT precede ordinary raw files, physical refinery inputs/manufacture and useful Forge route/files/raster. Prepared floor/machinery and actual TBM/bootstrap/human/gallery gates stay explicit.",
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(world_pairs), "preserved_shield_pairs": len(shield_pairs)}))


if __name__ == "__main__":
    main()
