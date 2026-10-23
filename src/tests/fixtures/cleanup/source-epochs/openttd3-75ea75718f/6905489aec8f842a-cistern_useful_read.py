"""Accept one original useful Cistern boundary through ordinary full-application preview/repeat/continuation.

@file cistern_useful_read.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Exact opaque original writer identities select the nine real parts and every upstream/receiver/power owner.
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
from cistern_useful_checks import PREPARE_RECEIPT, original_route_environment
from cistern_useful_files import SOURCES as WRITER_SOURCES

WRITER_RECEIPT = "logs/tubes/cistern-files/20261018-native-01/producer-verification.json"
SOURCES = (*WRITER_SOURCES, "src/tests/multilayer/cistern_useful_read.py", "src/fios.h", "src/saveload/afterload.cpp")


def read_cistern(context, configuration, point, source, label, environment, action="inspect"):
    """Load original owners normally, check pure preview, then pure-resave or advance one complete shared opportunity.

    @param context Invocation-owned runtime, world output and native diagnostic locations.
    @param configuration Actual compiled current native reader.
    @param point Exact writer-observed original participant snapshot for this input boundary.
    @param source Untouched original native file or previous ordinary resave.
    @param label Unique process diagnostic name inside this bounded pair.
    @param environment Original source/route roles and ready-world quantities from exact accepted lineage.
    @param action Pure inspect/resave, one complete native opportunity or an actual moving native raster observation.
    @return Native resave path and complete original input/output/process/observation evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DDC; ordinary original Cistern file observer.\ncistern_useful_file_check\n", encoding="utf-8")
    variables = dict(environment, OTTD_CISTERN_USEFUL_INPUT=str(source), OTTD_CISTERN_USEFUL_ACTION=action,
                     OTTD_CISTERN_USEFUL_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    output, proof = native(context, configuration, "[.tube-cistern-useful-native-file]", label, variables)
    match = re.search(r"CISTERN_READ action=(inspect|step|raster) original_parts=9 fronts=5 full_native_clock=true preview_pure=true snapshot=([^\s]+) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    snapshot = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+),", match[2])}
    assert ",".join(f"{key}={value}" for key, value in snapshot.items()) + "," == match[2]
    saved = Path(match[3]); assert saved.resolve() != source.resolve() and len(world_chunks(saved)) == 126
    proof.update(input=identity(source), output=identity(saved), action=action, observed_snapshot=snapshot, preview_pure=True)
    return saved, proof


def main():
    """Verify one complete opposite-writer original boundary without constructing or normalizing any participant.

    @return None; changed originals, preview mutation or unequal complete native owners prevent a reader report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("reload", "step"), required=True)
    parser.add_argument("--case", type=int, choices=range(6), required=True)
    parser.add_argument("--writer-report", type=Path, required=True)
    args = parser.parse_args(); receipt = ROOT / WRITER_RECEIPT
    assert identity(receipt)["sha256"] == "ba5951c8745c7c51b0227a5038c9f15ab8b58b953744f38196ca7ac7889a06fe"
    accepted = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(args.writer_report)
    assert selected in [row["report"] for row in accepted["records"] if row.get("phase") == "write"]
    writer = json.loads(args.writer_report.read_text(encoding="utf-8")); assert writer["configuration"] != args.config
    preparation = ROOT / PREPARE_RECEIPT; assert identity(preparation) == writer["prepare_receipt"]
    original = ROOT / writer["prepare_report"]["path"]; assert identity(original) == writer["prepare_report"]
    prepared = json.loads(original.read_text(encoding="utf-8")); lineage = json.loads(preparation.read_text(encoding="utf-8"))
    environment = original_route_environment(prepared, lineage)
    points = {row["step"]: row for row in writer["checkpoints"] if row["scene"] == args.case}
    point = points[0]; source = ROOT / point["input"]["path"]; assert identity(source) == point["input"]
    context = current_run_context("tubes", "cistern-files"); context.prepare_runtime()
    report = dict(writer=selected, checkpoint=point)
    if args.phase == "reload":
        first, proof = read_cistern(context, args.config, point, source, "first", environment)
        second, repeat = read_cistern(context, args.config, point, first, "repeat", environment)
        assert proof["observed_snapshot"] == repeat["observed_snapshot"] == point["snapshot"]
        report.update(native=[proof, repeat], first_load_complete_chunks_exact=compare_chunks(source, first, None),
                      repeated_complete_chunks_exact=compare_chunks(first, second, None))
    else:
        baseline = points[1]; expected = ROOT / baseline["input"]["path"]; assert identity(expected) == baseline["input"]
        normal, proof = read_cistern(context, args.config, baseline, expected, "baseline", environment)
        continued, resumed = read_cistern(context, args.config, point, source, "continued", environment, "step")
        assert proof["observed_snapshot"] == resumed["observed_snapshot"] == baseline["snapshot"]
        report.update(native=[proof, resumed], uninterrupted_baseline=baseline["input"],
                      next_step=compare_next_step(normal, continued, point["tick"], baseline["tick"]))
        assert identity(expected) == baseline["input"]
    assert identity(source) == point["input"]
    report.update(_documentation="r2mlvs17-underground-transport-tubes.md15DDC; exact original Cistern full-app loading/pure preview/repeat/shared native continuation",
                  configuration=args.config, phase=args.phase, case=args.case, writer_receipt=identity(receipt),
                  controller=identity(__file__), binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"),
                  sources=[identity(ROOT / name) for name in SOURCES])
    path = context.output("reports", f"cistern-useful-read-{args.config}-{args.phase}-{args.case}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
