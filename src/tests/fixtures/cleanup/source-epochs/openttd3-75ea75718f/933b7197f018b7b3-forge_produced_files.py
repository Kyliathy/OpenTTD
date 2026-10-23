"""Verify original manufactured RFMT through ordinary preview, repeat and complete native continuation.

@file forge_produced_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original finite mines, unused raw inputs, RFMT, packets and native account histories remain actual saved owners.
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
from forge_raw_delivery import SOURCES as DELIVERY_SOURCES, original_environment

DELIVERY_RECEIPT = "logs/tubes/forge-raw-route/20261018-native-01/verification.json"
DELIVERY_SHA256 = "a3e2722972933777893beed9c981bef902ee9c976f39a0e3a8c99224b044039a"
SOURCES = (*DELIVERY_SOURCES, "src/tests/multilayer/forge_produced_files.py")


def produced_environment(history, delivery):
    """Borrow exact original roles from sealed manufacturing/preparation lineage.

    @param history Immutable accepted manufacturing receipt with exact preparation identity.
    @param delivery Exact original manufactured-cargo report.
    @return Original source/route loader environment, without reconstructing or repairing any owner.
    """
    receipt = ROOT / history["prepare_receipt"]["path"]
    assert identity(receipt) == history["prepare_receipt"]
    prepared_receipt = json.loads(receipt.read_text(encoding="utf-8"))
    prepared_path = ROOT / delivery["prepare_report"]["path"]
    assert identity(prepared_path) == delivery["prepare_report"]
    prepared = json.loads(prepared_path.read_text(encoding="utf-8"))
    return original_environment(prepared, prepared_receipt)


def validate_point(point, delivery):
    """Check exact original manufactured owners and the next complete opportunity while raw trains stay stopped.

    @param point Original writer boundary with canonical snapshot and complete native file identity.
    @param delivery Accepted original manufactured-cargo result and its independently retained snapshot.
    @return None; altered quantities, source lineage, clocks or participant roles prevent acceptance.
    """
    values = point["snapshot"]
    original = delivery["snapshot"]
    assert point["step"] in (0, 1) and point["tick"] == values["tick"] == delivery["result"]["tick"] + point["step"]
    assert values.keys() == original.keys()
    assert values["calendar_year"] == values["economy_year"] == 2005
    assert values["vehicles"] == 4 and values["industries"] == values["stations"] == 3
    if point["step"] == 0:
        assert values == original
    # Whole stopped original raw trains cannot deliver another input or replay
    # income. Their exact accepted boundary is six work ticks into the next
    # native mining batch, so this single complete opportunity changes work
    # and clocks without another reserve debit or manufactured cargo credit.
    stable = ("cash", "loan", "quarters", "current_income", "current_expenses", "packets", "reserve0", "reserve1")
    for key in stable:
        assert values[key] == original[key]
    for index in range(2):
        for field in ("extracted", "dispatched", "industry", "deposit", "owner", "phase", "fraction", "scale"):
            key = f"mine{index}_{field}"
            assert values[key] == original[key]
        assert values[f"mine{index}_work"] == original[f"mine{index}_work"] + point["step"]
        assert values[f"mine{index}_revision"] == original[f"mine{index}_revision"] + point["step"]
        assert values[f"initial_reserve{index}"] == 32000
    # Recipe input/output and every original carrier/station queue remain the
    # same actual stocks while only the legitimate source work counter advances.
    for key, value in original.items():
        if key.startswith("industry2_") or key.startswith("part") or key.endswith("units"):
            assert values[key] == value, key


def read_produced(context, configuration, point, source, label, environment, action="inspect"):
    """Ordinarily load exact original owners, verify copied preview, then pure-save or advance once.

    @param context Invocation-owned runtime, native logs and outputs.
    @param configuration Actual current reader configuration.
    @param point Original boundary and exact canonical native snapshot.
    @param source Untouched original file or previous ordinary resave.
    @param label Unique isolated native process diagnostic name.
    @param environment Borrowed original source/route references and independent reserve facts.
    @param action Pure inspection or precisely one complete native opportunity.
    @return Ordinary output path and full process/input/output/observation evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FCD; original manufactured cargo native file observer.\nforge_produced_file_check\n", encoding="utf-8")
    variables = dict(environment, OTTD_FORGE_PRODUCED_INPUT=str(source), OTTD_FORGE_PRODUCED_ACTION=action,
                     OTTD_FORGE_PRODUCED_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    output, proof = native(context, configuration, "[.tube-forge-produced-read-native]", label, variables)
    match = re.search(r"FORGE_PRODUCED_READ action=(inspect|step) preview_pure=true full_native_clock=true snapshot=([^\s]+) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    snapshot = parse_snapshot(match[2])
    saved = Path(match[3])
    assert saved.resolve() != source.resolve() and len(world_chunks(saved)) == 126
    proof.update(input=identity(source), output=identity(saved), action=action, preview_pure=True, observed_snapshot=snapshot)
    return saved, proof


def main():
    """Write or ordinarily consume exact opposite original manufactured RFMT with every real source/history retained.

    @return None; changed originals, impure preview/repeat or unequal complete native continuation prevent a report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "step"), required=True)
    parser.add_argument("--delivery-report", type=Path)
    parser.add_argument("--writer-report", type=Path)
    args = parser.parse_args()
    receipt = ROOT / DELIVERY_RECEIPT
    assert identity(receipt)["sha256"] == DELIVERY_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["report"]["path"]: row for row in history["records"]}
    if args.phase == "write":
        assert args.delivery_report is not None
        selected = identity(args.delivery_report)
        assert selected == originals[selected["path"]]["report"]
        delivery = json.loads(args.delivery_report.read_text(encoding="utf-8"))
    else:
        assert args.writer_report is not None
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        assert writer["configuration"] != args.config and writer["phase"] == "write" and writer["delivery_receipt"] == identity(receipt)
        selected = writer["delivery_report"]
        assert selected == originals[selected["path"]]["report"]
        delivery = originals[selected["path"]]
    environment = produced_environment(history, delivery)
    context = current_run_context("tubes", "forge-produced")
    context.prepare_runtime()
    report = dict(_documentation="r2mlvs17-underground-transport-tubes.md15FCD; exact original manufactured RFMT files/pure preview/repeat/complete next opportunity",
                  configuration=args.config, phase=args.phase, delivery_receipt=identity(receipt), delivery_report=selected,
                  controller=identity(__file__), binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES])
    if args.phase == "write":
        assert delivery["configuration"] != args.config
        original = delivery["native"]["output"]
        source = ROOT / original["path"]
        assert identity(source) == original
        startup = context.runtime_dir / "scripts/game_start.scr"
        startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FCD; untouched original RFMT boundaries.\nforge_produced_boundaries_check\n", encoding="utf-8")
        variables = dict(environment, OTTD_FORGE_PRODUCED_INPUT=str(source), OTTD_FORGE_PRODUCED_SNAPSHOT=",".join(f"{key}={value}" for key, value in delivery["snapshot"].items()) + ",")
        output, proof = native(context, args.config, "[.tube-forge-produced-boundaries-native]", "writer", variables)
        points = []
        # Boundary0 remains the exact accepted original; boundary1 is the
        # very next complete shared native opportunity, with no intervening load.
        for step, tick, snapshot, name in re.findall(r"FORGE_PRODUCED_WORLD step=(\d+) tick=(\d+) snapshot=([^\s]+) path=([^\r\n]+)", output):
            saved = Path(name)
            assert len(world_chunks(saved)) == 126
            point = dict(step=int(step), tick=int(tick), snapshot=parse_snapshot(snapshot), input=identity(saved))
            validate_point(point, delivery)
            points.append(point)
        assert len(points) == 2 and {point["step"] for point in points} == {0, 1}
        grid = {point["step"]: point for point in points}
        assert compare_chunks(source, ROOT / grid[0]["input"]["path"], None) == 126 and identity(source) == original
        proof.update(input=original)
        report.update(native=[proof], checkpoints=points, original_complete_chunks_exact=126)
    else:
        points = {point["step"]: point for point in writer["checkpoints"]}
        point = points[0]
        source = ROOT / point["input"]["path"]
        assert identity(source) == point["input"]
        report.update(writer=identity(args.writer_report), checkpoint=point)
        if args.phase == "reload":
            first, proof = read_produced(context, args.config, point, source, "first", environment)
            second, repeat = read_produced(context, args.config, point, first, "repeat", environment)
            assert proof["observed_snapshot"] == repeat["observed_snapshot"] == point["snapshot"]
            report.update(native=[proof, repeat], first_load_complete_chunks_exact=compare_chunks(source, first, None), repeated_complete_chunks_exact=compare_chunks(first, second, None))
        else:
            baseline = points[1]
            expected = ROOT / baseline["input"]["path"]
            assert identity(expected) == baseline["input"]
            first, proof = read_produced(context, args.config, baseline, expected, "baseline", environment)
            second, continued = read_produced(context, args.config, point, source, "continued", environment, "step")
            assert proof["observed_snapshot"] == continued["observed_snapshot"] == baseline["snapshot"]
            report.update(native=[proof, continued], uninterrupted_baseline=baseline["input"], next_step=compare_next_step(first, second, point["tick"], baseline["tick"]))
            assert compare_chunks(first, second, None) == 126 and identity(expected) == baseline["input"]
        assert identity(source) == point["input"]
    path = context.output("reports", f"forge-produced-files-{args.config}-{args.phase}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
