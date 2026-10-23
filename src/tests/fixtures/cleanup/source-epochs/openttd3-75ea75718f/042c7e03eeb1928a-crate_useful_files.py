"""Write genuine1975 Crate worlds and untouched full-native continuation baselines.

@file crate_useful_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CC.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original native town/industry/current-and-historical accounts accompany all five actual train parts and live packets.
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
from goods_source_checks import SOURCES as SOURCE_SOURCES
from tube_work_file_report import OWNERS as VEHICLE_OWNERS

# The first-load boundary must retain native town/quarter/payment owners too.
# Source quantities cannot be reconstructed from a matching capsule scalar.
OWNERS = VEHICLE_OWNERS | {b"CITY", b"PLYR", b"ECMY", b"DATE", b"CHTS", b"SRND", b"CAPY", b"EIDS", b"IIDS", b"TIDS", b"HIDS"}
SOURCES = (*SOURCE_SOURCES, "src/tests/transport_tube_crate_routes.cpp", "src/tests/crate_useful_world_fixture.h",
           "src/tests/tube_useful_tick_fixture.h", "src/tests/layer_save_fixture.h", "src/saveload/afterload.cpp",
           "src/saveload/vehicle_sl.cpp", "src/saveload/town_sl.cpp", "src/saveload/company_sl.cpp", "src/saveload/cargopacket_sl.cpp",
           "src/tests/transport_tube_crate_load.cpp")
PRODUCER_RECEIPT = "logs/tubes/crate-route/20261018-native-01/verification.json"


def read_crate(context, configuration, point, source, label, action="inspect"):
    """Observe accepted original native participants, then pure-resave or advance one complete opportunity.

    @param context Invocation-owned native runtime/output/diagnostic locations.
    @param configuration Actual compiled ordinary reader configuration.
    @param point Exact original writer-observed identities, canonical native fields and tick.
    @param source Untouched original or previous ordinary current resave.
    @param label Unique bounded native child diagnostic name.
    @param action Pure inspection/resave, one complete shared native opportunity or actual moving native raster observation.
    @return Current native resave and complete original process/input/output/snapshot evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15CC.1.\ncrate_useful_file_check\n", encoding="utf-8")
    environment = {
        "OTTD_CRATE_USEFUL_PARTS": ",".join(map(str, point["parts"])),
        "OTTD_CRATE_USEFUL_REFERENCES": ",".join(map(str, point["references"])),
        "OTTD_CRATE_USEFUL_SNAPSHOT": ",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",",
        "OTTD_CRATE_USEFUL_INPUT": str(source), "OTTD_CRATE_USEFUL_ACTION": action,
    }
    output, proof = native(context, configuration, "[.tube-useful-crate-native-file]", label, environment)
    match = re.search(r"CRATE_READ action=(inspect|step|raster) fronts=3 original_parts=5 full_native_clock=true snapshot=([^\s]+) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    snapshot = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+),", match[2])}
    saved = Path(match[3]); assert saved.resolve() != source.resolve()
    proof.update(input=identity(source), output=identity(saved), action=action, observed_snapshot=snapshot)
    return saved, proof


def consume(context, args):
    """Run one bounded ordinary repeat or complete-native next-opportunity pair against an exact retained writer.

    @param context Invocation-owned native runtime, worlds and diagnostics.
    @param args Checked CLI configuration/scene and exact independent original writer selection.
    @return Current reader evidence; changed original identities or failed native owner comparisons refuse it.
    """
    assert args.writer_report is not None
    writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
    points = {point["step"]: point for point in writer["checkpoints"] if point["scene"] == args.case}
    point = points[0]; source = ROOT / point["input"]["path"]
    assert identity(source) == point["input"]
    report = dict(writer=identity(args.writer_report), checkpoint=point)
    if args.phase == "reload":
        first, proof = read_crate(context, args.config, point, source, "first")
        second, repeat = read_crate(context, args.config, point, first, "repeat")
        assert proof["observed_snapshot"] == repeat["observed_snapshot"] == point["snapshot"]
        report.update(native=[proof, repeat], owner_chunks_exact=compare_chunks(source, first, OWNERS),
                      repeated_complete_chunks_exact=compare_chunks(first, second, None))
    else:
        baseline = points[1]; expected = ROOT / baseline["input"]["path"]
        assert identity(expected) == baseline["input"]
        normal, proof = read_crate(context, args.config, baseline, expected, "baseline")
        continued, resumed = read_crate(context, args.config, point, source, "continued", "step")
        assert proof["observed_snapshot"] == resumed["observed_snapshot"] == baseline["snapshot"]
        report.update(native=[proof, resumed], uninterrupted_baseline=identity(expected),
                      next_step=compare_next_step(normal, continued, point["tick"], baseline["tick"]))
        assert compare_chunks(normal, continued, OWNERS) == len(OWNERS)
        assert identity(expected) == baseline["input"]
    assert identity(source) == point["input"]
    return report


def main():
    """Retain every actual loaded modal state and its exact uninterrupted next opportunity.

    @return None; missing real movement, changed canonical owners or incomplete original files refuse the writer report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "step"), default="write")
    parser.add_argument("--case", type=int, choices=range(6), default=0)
    parser.add_argument("--writer-report", type=Path)
    args = parser.parse_args()
    receipt = ROOT / PRODUCER_RECEIPT
    assert identity(receipt)["sha256"] == "1d58ae27a4a7ef37cb2d2564e791c6f813d82e0a8e53bdd4ced89ad7cf3c424f"
    context = current_run_context("tubes", "crate-files")
    context.prepare_runtime()
    if args.phase != "write":
        report = {
            "_documentation": "r2mlvs17-underground-transport-tubes.md15CC.1; ordinary Crate application reload/full-native continuation with original town/packet/history owners",
            "configuration": args.config, "phase": args.phase, "case": args.case,
            "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
            "sources": [identity(ROOT / name) for name in SOURCES], **consume(context, args),
        }
        path = context.output("reports", f"crate-useful-files-{args.config}-{args.phase}-{args.case}.json")
        path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(path, flush=True)
        return
    output, proof = native(context, args.config, "[tube-useful-crate-save-file]", "writer")
    pattern = r"CRATE_WORLD scene=(\d+) step=(\d+) tick=(\d+) parts=([\d,]+) references=([\d,]+) snapshot=([^\s]+) path=([^\r\n]+)"
    points = []
    # Scene numbers observe actual movement only. Each native snapshot keeps
    # named original values rather than a manufactured simplified cargo state.
    for scene, step, tick, parts, references, snapshot, name in re.findall(pattern, output):
        values = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+),", snapshot)}
        assert ",".join(f"{key}={value}" for key, value in values.items()) + "," == snapshot
        point = dict(scene=int(scene), step=int(step), tick=int(tick), parts=list(map(int, parts.split(","))),
                     references=list(map(int, references.split(","))), snapshot=values, input=identity(name))
        assert len(point["parts"]) == len(set(point["parts"])) == 5 and len(point["references"]) == 8
        assert point["tick"] == values["tick"] and values["vehicles"] == 5 and values["calendar_year"] == 1975
        chunks = world_chunks(Path(name))
        assert len(chunks) == 126 and OWNERS <= chunks.keys()
        points.append(point)
    assert len(points) == 12
    grid = {(point["scene"], point["step"]): point for point in points}
    assert grid.keys() == {(scene, step) for scene in range(6) for step in range(2)}
    # Genuine loaded feed/capsule/output legs must retain their physical
    # movement layer. The final world keeps the useful original town receipt
    # and native quarter history, with no recreated packet after consumption.
    for scene in range(6):
        original, baseline = grid[scene, 0], grid[scene, 1]
        assert baseline["tick"] == original["tick"] + 1 and baseline["parts"] == original["parts"] and baseline["references"] == original["references"]
        values = original["snapshot"]
        if scene < 5:
            part = (1, 1, 2, 4, 4)[scene]
            assert values[f"part{part}_units"] > 0 and values[f"part{part}_layer"] == (0, 1, 1, 1, 0)[scene]
        else:
            assert values["total_delivered"] == 39 and values["packets"] == 0 and values["quarters"] > 0
            assert all(values[f"part{part}_units"] == 0 for part in (1, 2, 4))
    assert "CRATE_USEFUL year=1975 delivered=39" in output and "returns=3" in output
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CC.1; genuine ordinary Crate worlds and full-native baselines; ordinary reader/runtime silhouette acceptance remains separate",
        "configuration": args.config, "phase": "write", "native": proof, "checkpoints": points,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES], "producer_receipt": identity(receipt),
    }
    path = context.output("reports", f"crate-useful-files-{args.config}-write.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
