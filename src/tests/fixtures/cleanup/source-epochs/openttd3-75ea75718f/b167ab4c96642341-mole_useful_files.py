"""Write actual useful Mole travel/crossing files with full native next-opportunity baselines.

@file mole_useful_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15BC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Producer files alone do not establish ordinary reload, runtime silhouette or remaining model acceptance.
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
from geothermal_tube_binding_files import binding_digest, world_chunks
from geothermal_tube_work_files import compare_chunks, compare_next_step
from tube_useful_route_checks import SOURCES as ROUTE_SOURCES
from tube_work_file_report import OWNERS

FIELDS = ("scene", "step", "utility", "binding", "invoice", "capsule", "freight", "wagon", "packet", "mine", "factory",
          "origin", "interchange", "destination", "source", "tick", "carry", "cap_count", "wagon_count", "factory_wait",
          "income", "delivered", "calendar", "calendar_fraction", "economy", "economy_fraction")
SOURCES = (*ROUTE_SOURCES, "src/tests/tube_useful_tick_fixture.h", "src/tests/transport_tube_useful_load.cpp",
           "src/tests/mine_fixture.h", "src/tests/excavation_fixture.h", "src/tests/layer_save_fixture.h")


def read_mole(context, configuration, point, source, label, action="inspect"):
    """Observe only accepted original participants before pure resave or full-native continuation.

    @param context Invocation-owned native runtime and generated outputs.
    @param configuration Actual compiled ordinary reader configuration.
    @param point Independent original writer-observed identity/quantity/clock expectations.
    @param source Untouched original or previous ordinary current resave.
    @param label Unique native diagnostic child name.
    @param action Pure inspect/resave, one complete native opportunity or actual loaded moving raster inspection.
    @return Current native resave and exact process/input/output evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15BC.\nmole_useful_file_check\n", encoding="utf-8")
    environment = {f"OTTD_MOLE_USEFUL_{key.upper()}": str(point[key]) for key in FIELDS}
    environment.update(OTTD_MOLE_USEFUL_INPUT=str(source), OTTD_MOLE_USEFUL_ACTION=action)
    output, proof = native(context, configuration, "[.tube-useful-mole-native-file]", label, environment)
    match = re.search(r"MOLE_READ action=(inspect|step|raster) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    assert "fronts=2" in output and "full_native_clock=true" in output
    saved = Path(match[2]); assert saved.resolve() != source.resolve()
    proof.update(input=identity(source), output=identity(saved), action=action)
    return saved, proof


def consume(context, args):
    """Run one bounded ordinary repeat or full-native next-step pair against a retained producer.

    @param context Invocation-owned native files and diagnostics.
    @param args Checked CLI configuration, scene and exact writer report selection.
    @return Current independent reader evidence; any changed owner or original file aborts it.
    """
    assert args.writer_report is not None
    writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
    points = {point["step"]: point for point in writer["checkpoints"] if point["scene"] == args.case}
    point = points[0]; source = ROOT / point["input"]["path"]
    assert identity(source) == point["input"]
    report = dict(writer=identity(args.writer_report), checkpoint=point)
    if args.phase == "reload":
        first, proof = read_mole(context, args.config, point, source, "first")
        second, repeat = read_mole(context, args.config, point, first, "repeat")
        report.update(native=[proof, repeat], owner_chunks_exact=compare_chunks(source, first, OWNERS),
                      repeated_complete_chunks_exact=compare_chunks(first, second, None))
    else:
        baseline = points[1]; expected = ROOT / baseline["input"]["path"]
        assert identity(expected) == baseline["input"]
        normal, proof = read_mole(context, args.config, baseline, expected, "baseline")
        continued, resumed = read_mole(context, args.config, point, source, "continued", "step")
        report.update(native=[proof, resumed], uninterrupted_baseline=identity(expected),
                      next_step=compare_next_step(normal, continued, point["tick"], baseline["tick"]))
        assert compare_chunks(normal, continued, OWNERS) == len(OWNERS)
        assert identity(expected) == baseline["input"]
    assert identity(source) == point["input"]
    return report


def main():
    """Run one independent actual writer or one bounded ordinary repeat/continuation pair.

    @return None; changed/missing original owners or a failed native route prevents the writer report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "step"), default="write")
    parser.add_argument("--case", type=int, choices=range(4), default=0)
    parser.add_argument("--writer-report", type=Path)
    args = parser.parse_args()
    context = current_run_context("tubes", "useful-files")
    context.prepare_runtime()
    if args.phase != "write":
        report = {
            "_documentation": "r2mlvs17-underground-transport-tubes.md15BC; ordinary useful Mole repeated/full-native continuation; source/bootstrap boundaries remain disclosed",
            "configuration": args.config, "phase": args.phase, "case": args.case,
            "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
            "sources": [identity(ROOT / name) for name in SOURCES],
            **consume(context, args),
        }
        path = context.output("reports", f"mole-useful-files-{args.config}-{args.phase}-{args.case}.json")
        path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(path, flush=True)
        return
    output, proof = native(context, args.config, "[tube-useful-mole-save-file]", "writer")
    pattern = "MOLE_WORLD " + " ".join(f"{key}=(\\d+)" for key in FIELDS) + r" path=([^\r\n]+)"
    points = []
    # Actual capsule travel, both conventional layers and the useful completed
    # world each retain an untouched next full timer/vehicle-loop opportunity.
    # Scene numbers describe observation only and never set simulation state.
    for match in re.findall(pattern, output):
        point = dict(zip(FIELDS, map(int, match[:-1])))
        path = Path(match[-1])
        point["input"] = identity(path)
        assert binding_digest(path) == point["binding"]
        chunks = world_chunks(path)
        assert len(chunks) == 126 and OWNERS <= chunks.keys()
        points.append(point)
    assert len(points) == 8
    grid = {(point["scene"], point["step"]): point for point in points}
    assert grid.keys() == {(scene, step) for scene in range(4) for step in range(2)}
    for scene in range(4):
        original, baseline = grid[scene, 0], grid[scene, 1]
        assert baseline["tick"] == original["tick"] + 1
        assert (original["cap_count"], original["wagon_count"], original["factory_wait"]) == ((20, 0, 0), (0, 20, 0), (0, 20, 0), (0, 0, 20))[scene]
    assert "source=finite_mine year=1970 autonomous=true" in output and "empty_returns=2" in output
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15BC; actual original useful Mole worlds and uninterrupted full-native baselines; ordinary readers/runtime art remain separate",
        "configuration": args.config, "phase": "write", "native": proof, "checkpoints": points,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"mole-useful-files-{args.config}-write.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
