"""Accept actual current capsule work/source switches through ordinary native434 files.

@file geothermal_tube_work_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BDDB/14B/14EC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each invocation owns one writer or one checkpoint's bounded repeat/continuation pair.
Prepared physical inputs remain separate from normal-world bootstrap acceptance.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from geothermal_tube_binding_files import AUTHORITY, SOURCES as BINDING_SOURCES, binding_digest, world_chunks
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks

SOURCES = (*BINDING_SOURCES, "src/tests/geothermal_tube_work_fixture.h", "src/tests/geothermal_tube_switch_fixture.h",
           "src/tests/transport_tube_geothermal_work_load.cpp", "src/train_cmd.cpp", "src/vehicle.cpp", "src/fios_gui.cpp", "src/fios.h")
OWNER_CHUNKS = AUTHORITY | {b"CAPA"}
FIELDS = ("phase", "step", "source", "binding", "utility", "native", "invoice", "packet", "vehicle", "tick", "carry", "stopped")


def read_work(context, configuration, checkpoint, source, label, action="inspect", corrupt=None, expected_error=None, participants=None):
    """Inspect actual original owners before any optional next native opportunity.

    @param context Invocation-owned runtime, saves and diagnostics.
    @param configuration Actual compiled native reader configuration.
    @param checkpoint Writer-observed original identities/history, checked independently against current transport.
    @param source Genuine unchanged original or prior ordinary resave.
    @param label Unique native diagnostic name.
    @param action Pure inspect/resave, cache rebuild, one/all-front native opportunity or malformed refusal/recovery.
    @param corrupt Optional separately written malformed copy; original source is never modified.
    @param expected_error Exact producer refusal text proving semantic parsing rather than a compression-envelope error.
    @param participants Optional original queue identity tuple; both fronts/packets must survive every native read.
    @return Ordinary resave and native process evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6BDDB.\ngeothermal_work_file_check\n", encoding="utf-8")
    environment = {f"OTTD_GEOTHERMAL_WORK_{key.upper()}": str(checkpoint[key]) for key in FIELDS if key not in ("phase", "step")}
    environment.update(OTTD_GEOTHERMAL_WORK_INPUT=str(source), OTTD_GEOTHERMAL_WORK_ACTION=action)
    if participants is not None:
        environment.update(OTTD_GEOTHERMAL_WORK_SECONDARY_VEHICLE=str(participants["secondary"]),
                           OTTD_GEOTHERMAL_WORK_SECONDARY_PACKET=str(participants["secondary_packet"]))
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_WORK_CORRUPT"] = str(corrupt)
        assert expected_error is not None
        environment["OTTD_GEOTHERMAL_WORK_ERROR"] = expected_error
    output, proof = native(context, configuration, "[.tube-geothermal-work-native-file]", label, environment)
    match = re.search(r"GEOTHERMAL_WORK_READ action=(inspect|step-all|step|cache|preview-refusal|full-refusal) path=([^\r\n]+)", output)
    assert match is not None and match[1] == action
    saved = Path(match[2]); assert saved.resolve() != source.resolve()
    proof.update(input=identity(source), output=identity(saved), action=action, checkpoint=checkpoint)
    if participants is not None:
        loaded = re.search(r"TUBE_QUEUE_LOADED primary=(\d+) secondary=(\d+) primary_packet=(\d+) secondary_packet=(\d+) fronts=2", output)
        assert loaded is not None
        actual = dict(zip(("primary", "secondary", "primary_packet", "secondary_packet"), map(int, loaded.groups())))
        assert actual == {key: participants[key] for key in actual}
        proof["participants"] = actual
    if action == "step-all":
        step = re.search(r"TUBE_QUEUE_STEP fronts=2 tick=(\d+)", output)
        assert step is not None and int(step[1]) == checkpoint["tick"] + 1
        proof.update(fronts_advanced=2, common_opportunities=1, continued_tick=int(step[1]))
    if action == "cache":
        cache = re.search(r"GEOTHERMAL_WORK_CACHE external=(\d+) native=(\d+) rebuilds=3 external_connected=true native_connected=(true|false) before=([^\r\n]+)", output)
        assert cache is not None
        assert int(cache[1]) > 0 and int(cache[2]) > 0
        proof.update(cache_before=identity(Path(cache[4])), cache_rebuilds=3,
                     original_external_sources=int(cache[1]), original_native_sources=int(cache[2]),
                     external_connected=True, native_connected=cache[3] == "true")
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
        assert expected_error in output
        proof["expected_semantic_refusal"] = expected_error
    return saved, proof


def compare_chunks(first, second, names):
    """Require exact complete serialized owners or complete current worlds.

    @param first Earlier actual ordinary file.
    @param second Later native output to compare.
    @param names Explicit authoritative chunk names, or None for every complete current chunk.
    @return Number of exactly matching complete chunks; differences abort rather than normalize owned data.
    """
    before, after = world_chunks(first), world_chunks(second)
    selected = before.keys() if names is None else names
    assert set(selected) <= before.keys() and set(selected) <= after.keys()
    differences = [name.decode() for name in selected if before[name] != after[name]]
    assert not differences, (first, second, differences)
    if names is None:
        assert before.keys() == after.keys()
    return len(selected)


def compare_next_step(first, second, original_tick, baseline_tick):
    """Compare complete actual worlds while independently validating native load-event timing.

    @param first Ordinary resave of the uninterrupted next-step baseline.
    @param second Actual next-step output after loading the earlier checkpoint.
    @param original_tick Actual earlier checkpoint tick at its load boundary.
    @param baseline_tick Actual later baseline tick at its separate load boundary.
    @return Exact complete chunk count plus checked original diagnostic load events.
    """
    before, after = world_chunks(first), world_chunks(second)
    assert before.keys() == after.keys()
    exact = compare_chunks(first, second, before.keys() - {b"GLOG"})
    events = []
    if before[b"GLOG"] == after[b"GLOG"]:
        return dict(complete_chunks_exact=exact + 1, native_load_events=events)
    earlier = next(row for row in chunks(read_stream(first)[1]) if row["name"] == b"GLOG")
    later = next(row for row in chunks(read_stream(second)[1]) if row["name"] == b"GLOG")
    assert earlier["kind"] == later["kind"] == 3
    assert earlier["rows"][0] == later["rows"][0] and len(earlier["rows"]) == len(later["rows"])
    # GLOG stores the tick of the actual load event, not the subsequent work.
    # Both files are loaded at their own original tick. Accept only precisely
    # that native Load action's U64 tick difference, with every other byte exact.
    for row_a, row_b in zip(earlier["rows"][1:], later["rows"][1:]):
        if row_a == row_b:
            continue
        assert row_a[0] == row_b[0] == 1 and row_a[9:] == row_b[9:]
        assert int.from_bytes(row_a[1:9], "big") == baseline_tick
        assert int.from_bytes(row_b[1:9], "big") == original_tick
        events.append(dict(action="Load", baseline_tick=baseline_tick, resumed_tick=original_tick, other_bytes_exact=True))
    assert events
    return dict(complete_chunks_exact=exact, native_load_events=events)


def main():
    """Run one bounded writer, loaded cache rebuild, pure repeat or next-step comparison.

    @return None; changed input identity, empty native selection or owner/continuation mismatch forbids acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "step", "cache"), required=True)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--case", type=int, choices=range(6), default=0)
    args = parser.parse_args()
    context = current_run_context("tubes", "geothermal-work-files"); context.prepare_runtime()
    report = dict(_documentation="r2mlvs17-underground-transport-tubes.md6BDDB/14B; actual current work/switch/cache files; prepared inputs are not normal bootstrap or host-client acceptance",
                  configuration=args.config, phase=args.phase, case=args.case,
                  binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), controller=identity(__file__),
                  sources=[identity(ROOT / name) for name in SOURCES])
    if args.phase == "write":
        output, proof = native(context, args.config, "[tube-geothermal-work-save-file]", "writer")
        pattern = "GEOTHERMAL_WORK_SAVE " + " ".join(f"{key}=(\\d+)" for key in FIELDS) + r" path=([^\r\n]+)"
        checkpoints = []
        # Every original work checkpoint needs its own uninterrupted baseline;
        # an idle source fixture or missing scene cannot count as live acceptance.
        for match in re.findall(pattern, output):
            checkpoint = dict(zip(FIELDS, map(int, match[:-1])))
            source = Path(match[-1]); checkpoint["input"] = identity(source)
            assert binding_digest(source) == checkpoint["binding"]
            assert OWNER_CHUNKS <= world_chunks(source).keys()
            checkpoints.append(checkpoint)
        assert {(point["phase"], point["step"]) for point in checkpoints} == {(phase, step) for phase in range(6) for step in range(2)}
        assert len(checkpoints) == 12
        report.update(native=[proof], checkpoints=checkpoints)
    else:
        assert args.writer_report is not None
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        checkpoint = next(point for point in writer["checkpoints"] if point["phase"] == args.case and point["step"] == 0)
        baseline = next(point for point in writer["checkpoints"] if point["phase"] == args.case and point["step"] == 1)
        source = ROOT / checkpoint["input"]["path"]; original = identity(source); assert original == checkpoint["input"]
        report.update(writer=identity(args.writer_report), checkpoint=checkpoint)
        if args.phase == "reload":
            first, proof = read_work(context, args.config, checkpoint, source, "first")
            second, repeat = read_work(context, args.config, checkpoint, first, "second")
            report.update(native=[proof, repeat], owner_chunks_exact=compare_chunks(source, first, OWNER_CHUNKS),
                          repeated_complete_chunks_exact=compare_chunks(first, second, None),
                          first_changed_chunks=[name.decode() for name, raw in world_chunks(source).items() if raw != world_chunks(first).get(name)])
        elif args.phase == "cache":
            cached, proof = read_work(context, args.config, checkpoint, source, "cache", "cache")
            report.update(native=[proof], owner_chunks_exact=compare_chunks(source, cached, OWNER_CHUNKS),
                          cache_complete_chunks_exact=compare_chunks(ROOT / proof["cache_before"]["path"], cached, None))
        else:
            expected = ROOT / baseline["input"]["path"]; assert identity(expected) == baseline["input"]
            normal, proof = read_work(context, args.config, baseline, expected, "uninterrupted-baseline")
            continued, resumed = read_work(context, args.config, checkpoint, source, "continued", "step")
            report.update(native=[proof, resumed], uninterrupted_baseline=identity(expected),
                          next_step=compare_next_step(normal, continued, checkpoint["tick"], baseline["tick"]))
            assert identity(expected) == baseline["input"]
        assert identity(source) == original
    destination = context.output("reports", f"geothermal-work-files-{args.config}-{args.phase}-{args.case}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(destination, flush=True)


if __name__ == "__main__":
    main()
