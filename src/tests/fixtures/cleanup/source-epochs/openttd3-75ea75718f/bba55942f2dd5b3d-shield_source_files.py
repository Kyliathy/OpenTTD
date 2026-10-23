"""Retain original1985 Shield source boundaries and verify ordinary full-native source-file continuation.

@file shield_source_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EBB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All source roles come from exact accepted native originals; readers construct no machinery, registry or stock.
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
from shield_source_checks import SOURCES as STOCK_SOURCES

STOCK_RECEIPT = "logs/tubes/shield-source/20261018-native-01/stock-verification.json"
SOURCES = (*STOCK_SOURCES, "src/tests/shield_source_world_fixture.h", "src/tests/multilayer/shield_source_files.py",
           "src/saveload/afterload.cpp", "src/saveload/industry_sl.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/company_sl.cpp",
           "src/saveload/layer_mine_sl.cpp", "src/saveload/layer_processor_sl.cpp")


def source_environment(source):
    """Transport exact original accepted source roles and independent finite-stock facts.

    @param source Observed original source result retained by the immutable stock receipt.
    @return Native reader environment; runtime cargo labels remain actual ordinary-loaded definitions.
    """
    return {
        "OTTD_SHIELD_SOURCE_PARTS": ",".join(map(str, source["parts"])),
        "OTTD_SHIELD_SOURCE_REFERENCES": ",".join(map(str, (*source["industries"], source["deposit"], source["home"]))),
        "OTTD_SHIELD_SOURCE_FACTS": f"{source['units']},{source['reserve']}",
    }


def parse_snapshot(text):
    """Decode a complete canonical named native observation without dropping duplicates or suffixes.

    @param text Original native CSV preserving every exact integer field.
    @return Ordered original scalar observations; malformed or repeated field names refuse transport.
    """
    values = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+),", text)}
    assert ",".join(f"{key}={value}" for key, value in values.items()) + "," == text
    return values


def read_source(context, configuration, point, source, label, environment, action="inspect"):
    """Load/preview exact native owners, then pure-resave or advance one complete shared source opportunity.

    @param context Invocation-owned native runtime, world outputs and diagnostics.
    @param configuration Actual compiled current reader configuration.
    @param point Exact original writer boundary and canonical native fields.
    @param source Untouched original or previous ordinary native resave.
    @param label Unique bounded child diagnostic name.
    @param environment Accepted original source roles and finite-stock facts.
    @param action Pure inspection or one complete native source opportunity.
    @return Current native resave and complete process/input/output/observation evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15EBB; original source-file observer.\nshield_source_file_check\n", encoding="utf-8")
    variables = dict(environment, OTTD_SHIELD_SOURCE_READ_INPUT=str(source), OTTD_SHIELD_SOURCE_ACTION=action,
                     OTTD_SHIELD_SOURCE_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    output, proof = native(context, configuration, "[.tube-shield-source-read-native]", label, variables)
    match = re.search(r"SHIELD_SOURCE_READ action=(inspect|step) original_parts=2 native_industries=2 preview_pure=true full_native_clock=true snapshot=([^\s]+) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    snapshot = parse_snapshot(match[2]); saved = Path(match[3]); assert saved.resolve() != source.resolve() and len(world_chunks(saved)) == 126
    proof.update(input=identity(source), output=identity(saved), action=action, preview_pure=True, observed_snapshot=snapshot)
    return saved, proof


def main():
    """Write or ordinarily consume exact opposite original source boundaries without reconstruction.

    @return None; changed originals, failed preview/save purity or unequal complete continuation prevents a source-file report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "step"), required=True)
    parser.add_argument("--stock-report", type=Path)
    parser.add_argument("--writer-report", type=Path)
    args = parser.parse_args(); receipt = ROOT / STOCK_RECEIPT
    assert identity(receipt)["sha256"] == "152161e841195806a4437c3d74f463115b6dfe8f18f0372aa7766e66144abe3a"
    history = json.loads(receipt.read_text(encoding="utf-8")); source_result = history["source"]; environment = source_environment(source_result)
    context = current_run_context("tubes", "shield-source"); context.prepare_runtime()
    report = dict(_documentation="r2mlvs17-underground-transport-tubes.md15EBB; original native source-file boundaries/preview/repeat/complete next opportunity",
                  configuration=args.config, phase=args.phase, stock_receipt=identity(receipt), controller=identity(__file__),
                  binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES])
    if args.phase == "write":
        assert args.stock_report is not None; selected = identity(args.stock_report)
        assert selected in [row["report"] for row in history["records"] if row["focus"] == "stock-producer"]
        stock = json.loads(args.stock_report.read_text(encoding="utf-8")); assert stock["configuration"] != args.config
        original = stock["native"][1]["output"]; source = ROOT / original["path"]; assert identity(source) == original
        startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15EBB; untouched original source boundaries.\nshield_source_boundaries_check\n", encoding="utf-8")
        output, proof = native(context, args.config, "[.tube-shield-source-boundaries-native]", "writer", dict(environment, OTTD_SHIELD_SOURCE_READ_INPUT=str(source)))
        points = []
        # Original boundary0 and its next complete source opportunity1 keep
        # real definitions/work counters; no simplified quantity fixture can
        # stand in for the retained full native source world.
        for step, tick, parts, references, snapshot, name in re.findall(r"SHIELD_SOURCE_WORLD step=(\d+) tick=(\d+) parts=([\d,]+) references=([\d,]+) snapshot=([^\s]+) path=([^\r\n]+)", output):
            values = parse_snapshot(snapshot); saved = Path(name); assert len(world_chunks(saved)) == 126
            points.append(dict(step=int(step), tick=int(tick), parts=list(map(int, parts.split(","))), references=list(map(int, references.split(","))), snapshot=values, input=identity(saved)))
        assert len(points) == 2 and {p["step"] for p in points} == {0, 1}
        grid = {point["step"]: point for point in points}
        for step, point in grid.items():
            values = point["snapshot"]
            assert point["tick"] == values["tick"] == source_result["tick"] + step
            assert point["parts"] == source_result["parts"] and point["references"] == [*source_result["industries"], source_result["deposit"], source_result["home"]]
            assert values["calendar_year"] == 1985 and values["vehicles"] == values["industries"] == 2 and values["stations"] == values["packets"] == 0
            assert values["reserve"] == source_result["reserve"] and values["mine_extracted"] == values["industry0_output0_waiting"] == 20
            assert values["mine_dispatched"] == 0 and values["part0_units"] == values["part1_units"] == 0
        assert compare_chunks(source, ROOT / grid[0]["input"]["path"], None) == 126 and identity(source) == original
        proof.update(input=original); report.update(native=[proof], checkpoints=points, stock_report=selected, original_complete_chunks_exact=126)
    else:
        assert args.writer_report is not None; writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        assert writer["configuration"] != args.config and writer["phase"] == "write" and writer["stock_receipt"] == identity(receipt)
        assert writer["stock_report"] in [row["report"] for row in history["records"] if row["focus"] == "stock-producer"]
        points = {point["step"]: point for point in writer["checkpoints"]}; point = points[0]
        source = ROOT / point["input"]["path"]; assert identity(source) == point["input"]
        report.update(writer=identity(args.writer_report), checkpoint=point)
        if args.phase == "reload":
            first, proof = read_source(context, args.config, point, source, "first", environment)
            second, repeat = read_source(context, args.config, point, first, "repeat", environment)
            assert proof["observed_snapshot"] == repeat["observed_snapshot"] == point["snapshot"]
            report.update(native=[proof, repeat], first_load_complete_chunks_exact=compare_chunks(source, first, None), repeated_complete_chunks_exact=compare_chunks(first, second, None))
        else:
            baseline = points[1]; expected = ROOT / baseline["input"]["path"]; assert identity(expected) == baseline["input"]
            first, proof = read_source(context, args.config, baseline, expected, "baseline", environment)
            second, continued = read_source(context, args.config, point, source, "continued", environment, "step")
            assert proof["observed_snapshot"] == continued["observed_snapshot"] == baseline["snapshot"]
            report.update(native=[proof, continued], uninterrupted_baseline=baseline["input"], next_step=compare_next_step(first, second, point["tick"], baseline["tick"]))
            assert identity(expected) == baseline["input"]
        assert identity(source) == point["input"]
    destination = context.output("reports", f"shield-source-files-{args.config}-{args.phase}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(destination, flush=True)


if __name__ == "__main__":
    main()
