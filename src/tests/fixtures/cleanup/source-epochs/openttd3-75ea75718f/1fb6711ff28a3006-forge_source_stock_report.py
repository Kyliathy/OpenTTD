"""Accept actual original2005 finite Forge raw stocks, native empty refinery and unchanged prerequisite worlds.

@file forge_source_stock_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Ordinary stock-file acceptance, physical raw carriage and manufactured RFMT retain their later tasks.
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
from forge_source_stock import PREREQUISITE_RECEIPT, PREREQUISITE_SHA256


def main():
    """Require exact opposite accepted inputs, conserved raw-source outputs and current native producer/build evidence.

    @return None; altered originals, lost finite quantities or unequal complete native worlds prevent a stock receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stocks", nargs=2, required=True)
    parser.add_argument("--prerequisites", nargs=2, required=True)
    parser.add_argument("--producers", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / PREREQUISITE_RECEIPT
    assert identity(receipt)["sha256"] == PREREQUISITE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in history["records"] if row.get("focus") == "native"}
    configurations = ("Debug", "RelWithDebInfo")
    records, selected, cache, prerequisite_pairs = [], {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Current source reports continue exact opposite accepted normal-loaded
    # empty worlds. Native stocks never authorize a fresh date/funding/content
    # reset, and sealed prerequisite binaries remain opaque historical inputs.
    for focus, names in (("raw-stock", args.stocks), ("native", args.prerequisites), ("producers", args.producers)):
        for name in names:
            path = ROOT / name
            report = json.loads(path.read_text(encoding="utf-8"))
            verify_references(report, cache)
            cfg = report["configuration"]
            assert cfg in configurations and report["focus"] == focus and (cfg, focus) not in selected
            if focus == "raw-stock":
                opposite = configurations[1 - configurations.index(cfg)]
                original = originals[opposite]
                assert report["prerequisite_report"] == original["report"] and report["prerequisite_receipt"] == identity(receipt)
                assert report["native"]["input"] == original["native"][1]["output"]
                source = report["source"]
                assert source["year"] == 2005 and source["tick"] == source["opportunities"] and 0 < source["tick"] < 2048
                assert source["copper"] == source["coal"] == 40 and source["loan"] == source["income"] == 0
                assert 0 < source["cash"] < original["definitions"]["cash"]
                deposits = {row["label"]: row for row in original["deposits"]}
                assert source["deposits"] == [deposits[label]["deposit"] for label in ("CORE", "COAL")]
                assert source["copper_reserve"] + source["copper"] == deposits["CORE"]["reserve"]
                assert source["coal_reserve"] + source["coal"] == deposits["COAL"]["reserve"]
                assert len(source["parts"]) == len(set(source["parts"])) == 4
                assert len(source["industries"]) == len(set(source["industries"])) == 3
                processes = [report["native"]]
            elif focus == "native":
                original = originals[cfg]
                assert report["definitions"] == original["definitions"] and report["deposits"] == original["deposits"]
                assert len(report["native"]) == len(original["native"]) == 2
                for current, previous in zip(report["native"], original["native"]):
                    first, second = current["output"], previous["output"]
                    assert identity(ROOT / second["path"]) == second
                    assert compare_chunks(ROOT / first["path"], ROOT / second["path"], None) == 126
                    prerequisite_pairs.append(dict(configuration=cfg, current=first, original=second, complete_chunks_exact=126))
                processes = report["native"]
            else:
                assert len(report["native"]) == len(CASES)
                assert {row["case"] for row in report["native"]} == {case for case, _ in CASES}
                processes = report["native"]
            for process in processes:
                count_native(process, summary[cfg])
            selected[cfg, focus] = report
            records.append(dict(report=identity(path), **report))
    assert selected.keys() == {(cfg, focus) for cfg in configurations for focus in ("raw-stock", "native", "producers")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    left, right = (selected[cfg, "raw-stock"] for cfg in configurations)
    assert left["source"] == right["source"]
    first, second = left["native"]["output"], right["native"]["output"]
    assert compare_chunks(ROOT / first["path"], ROOT / second["path"], None) == 126
    builds = []
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FBB; original actual finite CORE/Coal stock handoff, no raw dispatch or RFMT manufacture",
        controller=identity(__file__), summary=summary, records=records, source=left["source"],
        native_cases=sum(row["cases"] for row in summary.values()), native_assertions=sum(row["assertions"] for row in summary.values()),
        complete_stock_pair=dict(left=first, right=second, complete_chunks_exact=126), unchanged_prerequisite_pairs=prerequisite_pairs,
        prerequisite_receipt=identity(receipt), builds=builds, exact_identities_verified=len(cache), native_version=434,
        test_binaries=[identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        fixture_classification=left["fixture_classification"],
        remaining="15FBC must accept these exact original stocks through ordinary preview/repeat/opposite-reader/complete-next files before15FCB/15FCC paid raw routes and actual refinery manufacture. Original15F.1/15.2/master10.7 remain pending.",
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "source": result["source"]}))


if __name__ == "__main__":
    main()
