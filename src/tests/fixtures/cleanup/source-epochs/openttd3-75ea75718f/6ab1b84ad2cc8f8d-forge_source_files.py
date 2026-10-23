"""Verify original Forge raw-source files through normal preview, repeat and complete native continuation.

@file forge_source_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Exact original native roles are borrowed; neither writers nor readers construct source participants or repair quantities.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from geothermal_tube_binding_files import world_chunks
from geothermal_tube_work_files import compare_chunks, compare_next_step
from shield_source_files import parse_snapshot
from forge_source_stock import SOURCES as STOCK_SOURCES

STOCK_RECEIPT = "logs/tubes/forge-source/20261018-native-01/stock-verification.json"
STOCK_SHA256 = "219509cd72e634046615ab8b9e6c71b34df9a8aa52e76d625820789e77296714"
SOURCES = (*STOCK_SOURCES, "src/tests/forge_source_snapshot_fixture.h", "src/tests/multilayer/forge_source_files.py",
           "src/saveload/afterload.cpp", "src/saveload/industry_sl.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/company_sl.cpp",
           "src/saveload/layer_mine_sl.cpp", "src/saveload/layer_processor_sl.cpp")


def source_environment(source):
    """Transport only exact original accepted participant roles and independent raw checkpoint facts.

    @param source Original observed native raw-source result retained by the immutable stock receipt.
    @return Native borrowed-loader environment, without inferred cargo/industry IDs or source reconstruction.
    """
    return {
        "OTTD_FORGE_SOURCE_PARTS": ",".join(map(str, source["parts"])),
        "OTTD_FORGE_SOURCE_REFERENCES": ",".join(map(str, (*source["industries"], *source["deposits"], *source["homes"]))),
        "OTTD_FORGE_SOURCE_STOCKS": f"{source['copper']},{source['coal']}",
    }


def validate_point(point, source):
    """Independently check the original raw quantities/roles and exactly one native work opportunity.

    @param point Complete writer boundary, immutable original file identity and canonical native observations.
    @param source Accepted original raw stocks, reserves, participants and company facts.
    @return None; changed owner references, manufactured inputs/output or lost reserve/work history refuse a point.
    """
    values = point["snapshot"]
    assert point["step"] in (0, 1) and point["tick"] == values["tick"] == source["tick"] + point["step"]
    assert point["parts"] == source["parts"] and point["references"] == source["industries"] + source["deposits"] + source["homes"]
    assert values["calendar_year"] == values["economy_year"] == 2005
    assert values["vehicles"] == 4 and values["industries"] == 3 and values["stations"] == values["packets"] == 0
    assert values["cash"] == source["cash"] and values["loan"] == values["current_income"] == values["quarters"] == 0
    assert [values[f"part{index}_id"] for index in range(4)] == source["parts"]
    assert all(values[f"part{index}_units"] == 0 for index in range(4))
    # Both native mines retain independent physical and historical owners.
    # The original next opportunity adds work once without another batch or
    # dispatch; neither reserve can be normalized to match the other source.
    for index, meaning in enumerate(("copper", "coal")):
        assert values[f"reserve{index}"] == source[meaning + "_reserve"]
        assert values[f"initial_reserve{index}"] == source[meaning + "_reserve"] + source[meaning]
        assert values[f"mine{index}_extracted"] == values[f"industry{index}_output0_waiting"] == source[meaning]
        assert values[f"mine{index}_dispatched"] == 0 and values[f"mine{index}_work"] == point["step"]
        assert values[f"mine{index}_deposit"] == values[f"deposit{index}"] == source["deposits"][index]
        assert values[f"home{index}"] == source["homes"][index]
    # The refinery has received nothing. Its physical input/output slots and
    # native processor history cannot turn these raw stocks into invented RFMT.
    for kind, count in (("input", "inputs"), ("output", "outputs")):
        assert all(values[f"industry2_{kind}{slot}_waiting"] == 0 for slot in range(values[f"industry2_{count}"]))
    assert values["industry2_dispatched"] == 0


def read_source(context, configuration, point, source, label, environment, action="inspect"):
    """Load original owners normally, preserve copied preview, then pure-resave or advance one shared opportunity.

    @param context Invocation-owned runtime, native diagnostics and world outputs.
    @param configuration Actual compiled current reader configuration.
    @param point Exact original writer boundary and complete canonical snapshot.
    @param source Untouched original native file or previous ordinary resave.
    @param label Unique bounded native process diagnostic name.
    @param environment Original accepted native roles and raw-stock facts.
    @param action Pure inspection or one complete shared clock/front/industry opportunity.
    @return Ordinary native resave path and complete process/input/output/observation evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FBC; original raw-source native file observer.\nforge_source_file_check\n", encoding="utf-8")
    variables = dict(environment, OTTD_FORGE_SOURCE_READ_INPUT=str(source), OTTD_FORGE_SOURCE_ACTION=action,
                     OTTD_FORGE_SOURCE_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    output, proof = native(context, configuration, "[.tube-forge-source-read-native]", label, variables)
    match = re.search(r"FORGE_SOURCE_READ action=(inspect|step) original_parts=4 native_industries=3 preview_pure=true full_native_clock=true snapshot=([^\s]+) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    snapshot = parse_snapshot(match[2])
    saved = Path(match[3])
    assert saved.resolve() != source.resolve() and len(world_chunks(saved)) == 126
    proof.update(input=identity(source), output=identity(saved), action=action, preview_pure=True, observed_snapshot=snapshot)
    return saved, proof


def main():
    """Write or ordinarily consume exact opposite original native raw-source boundaries without reconstruction.

    @return None; changed originals, preview/save mutation or unequal complete native continuation prevent a file report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "step"), required=True)
    parser.add_argument("--stock-report", type=Path)
    parser.add_argument("--writer-report", type=Path)
    args = parser.parse_args()
    receipt = ROOT / STOCK_RECEIPT
    assert identity(receipt)["sha256"] == STOCK_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    source_result = history["source"]
    environment = source_environment(source_result)
    context = current_run_context("tubes", "forge-source")
    context.prepare_runtime()
    report = dict(_documentation="r2mlvs17-underground-transport-tubes.md15FBC; exact original raw-source files/pure preview/repeat/complete next opportunity",
                  configuration=args.config, phase=args.phase, stock_receipt=identity(receipt), controller=identity(__file__),
                  binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES])
    if args.phase == "write":
        assert args.stock_report is not None
        selected = identity(args.stock_report)
        assert selected in [row["report"] for row in history["records"] if row.get("focus") == "raw-stock"]
        stock = json.loads(args.stock_report.read_text(encoding="utf-8"))
        assert stock["configuration"] != args.config
        original = stock["native"]["output"]
        source = ROOT / original["path"]
        assert identity(source) == original
        startup = context.runtime_dir / "scripts/game_start.scr"
        startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FBC; untouched original raw-source boundaries.\nforge_source_boundaries_check\n", encoding="utf-8")
        output, proof = native(context, args.config, "[.tube-forge-source-boundaries-native]", "writer", dict(environment, OTTD_FORGE_SOURCE_READ_INPUT=str(source)))
        points = []
        # Every canonical CSV and complete native file remains exact. Saving
        # original0 cannot add a private opportunity before the native1 baseline.
        for step, tick, parts, references, snapshot, name in re.findall(r"FORGE_SOURCE_WORLD step=(\d+) tick=(\d+) parts=([\d,]+) references=([\d,]+) snapshot=([^\s]+) path=([^\r\n]+)", output):
            values = parse_snapshot(snapshot)
            saved = Path(name)
            assert len(world_chunks(saved)) == 126
            point = dict(step=int(step), tick=int(tick), parts=list(map(int, parts.split(","))), references=list(map(int, references.split(","))), snapshot=values, input=identity(saved))
            validate_point(point, source_result)
            points.append(point)
        assert len(points) == 2 and {point["step"] for point in points} == {0, 1}
        grid = {point["step"]: point for point in points}
        assert compare_chunks(source, ROOT / grid[0]["input"]["path"], None) == 126 and identity(source) == original
        proof.update(input=original)
        report.update(native=[proof], checkpoints=points, stock_report=selected, original_complete_chunks_exact=126)
    else:
        assert args.writer_report is not None
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        assert writer["configuration"] != args.config and writer["phase"] == "write" and writer["stock_receipt"] == identity(receipt)
        assert writer["stock_report"] in [row["report"] for row in history["records"] if row.get("focus") == "raw-stock"]
        points = {point["step"]: point for point in writer["checkpoints"]}
        point = points[0]
        source = ROOT / point["input"]["path"]
        assert identity(source) == point["input"]
        report.update(writer=identity(args.writer_report), checkpoint=point)
        if args.phase == "reload":
            first, proof = read_source(context, args.config, point, source, "first", environment)
            second, repeat = read_source(context, args.config, point, first, "repeat", environment)
            assert proof["observed_snapshot"] == repeat["observed_snapshot"] == point["snapshot"]
            report.update(native=[proof, repeat], first_load_complete_chunks_exact=compare_chunks(source, first, None), repeated_complete_chunks_exact=compare_chunks(first, second, None))
        else:
            baseline = points[1]
            expected = ROOT / baseline["input"]["path"]
            assert identity(expected) == baseline["input"]
            first, proof = read_source(context, args.config, baseline, expected, "baseline", environment)
            second, continued = read_source(context, args.config, point, source, "continued", environment, "step")
            assert proof["observed_snapshot"] == continued["observed_snapshot"] == baseline["snapshot"]
            report.update(native=[proof, continued], uninterrupted_baseline=baseline["input"], next_step=compare_next_step(first, second, point["tick"], baseline["tick"]))
            assert identity(expected) == baseline["input"]
        assert identity(source) == point["input"]
    path = context.output("reports", f"forge-source-files-{args.config}-{args.phase}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
