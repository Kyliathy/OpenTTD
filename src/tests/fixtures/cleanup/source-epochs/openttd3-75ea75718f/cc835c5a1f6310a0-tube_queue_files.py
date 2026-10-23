"""Read genuine competing and grant-starved ordinary tube worlds with all original fronts.

@file tube_queue_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14EC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The retained14EB producer owns actual contention/repair evidence; this runner never supplies movement or quantities.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import SOURCES, read_work, compare_chunks, compare_next_step
from tube_work_file_report import OWNERS


def main():
    """Run one bounded repeat or all-front next-opportunity pair against an untouched producer.

    @return None; any changed original, missing participant or full-owner mismatch aborts the report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("reload", "step"), required=True)
    parser.add_argument("--case", type=int, choices=(6, 7, 8), required=True)
    parser.add_argument("--writer-report", type=Path, required=True, help="Exact independent14EB writer report.")
    args = parser.parse_args()
    writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
    assert writer["configuration"] != args.config and writer["phase"] == "write"
    points = {point["step"]: point for point in writer["checkpoints"] if point["phase"] == args.case}
    checkpoint = points[0]
    source = ROOT / checkpoint["input"]["path"]
    assert identity(source) == checkpoint["input"]
    context = current_run_context("tubes", "queue-files-read")
    context.prepare_runtime()
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md14EC; actual ordinary repeated/all-front queue file acceptance; prepared producer inputs remain separate from useful bootstrap",
        "configuration": args.config, "phase": args.phase, "case": args.case,
        "writer": identity(args.writer_report), "checkpoint": checkpoint, "queue": writer["queue"],
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in (*SOURCES, "src/tests/transport_tube_queue_load.cpp", "src/tests/multilayer/utilities/geothermal_tube_work_files.py")],
    }
    if args.phase == "reload":
        first, proof = read_work(context, args.config, checkpoint, source, "first", participants=writer["queue"])
        second, repeat = read_work(context, args.config, checkpoint, first, "repeat", participants=writer["queue"])
        report.update(native=[proof, repeat], owner_chunks_exact=compare_chunks(source, first, OWNERS),
                      repeated_complete_chunks_exact=compare_chunks(first, second, None))
    else:
        assert args.case in (6, 7) and 1 in points
        baseline = points[1]
        expected = ROOT / baseline["input"]["path"]
        assert identity(expected) == baseline["input"]
        normal, proof = read_work(context, args.config, baseline, expected, "baseline", participants=writer["queue"])
        continued, resumed = read_work(context, args.config, checkpoint, source, "continued", "step-all", participants=writer["queue"])
        report.update(native=[proof, resumed], uninterrupted_baseline=identity(expected),
                      next_step=compare_next_step(normal, continued, checkpoint["tick"], baseline["tick"]))
        assert compare_chunks(normal, continued, OWNERS) == len(OWNERS)
        assert identity(expected) == baseline["input"]
    assert identity(source) == checkpoint["input"]
    destination = context.output("reports", f"tube-queue-files-{args.config}-{args.phase}-{args.case}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(destination, flush=True)


if __name__ == "__main__":
    main()
