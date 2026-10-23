"""Write genuine competing-junction and grant-starved native tube checkpoints.

@file tube_queue_writer.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14EB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Producer evidence alone does not establish ordinary reload or all-front continuation acceptance.
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
from geothermal_tube_work_files import FIELDS
from geothermal_tube_binding_files import binding_digest, world_chunks
from tube_work_file_report import OWNERS

SOURCES = (
    "src/tests/transport_tube_queue_load.cpp", "src/tests/geothermal_tube_switch_fixture.h",
    "src/tests/geothermal_tube_work_fixture.h", "src/tests/geothermal_tube_capsule_fixture.h",
    "src/tests/geothermal_tube_power_fixture.h", "src/tests/geothermal_generator_operating_fixture.h",
    "src/transport_tube_runtime.cpp", "src/transport_tube_topology.cpp", "src/transport_tube_power.cpp",
    "src/transport_tube_geothermal.cpp", "src/utility_power.cpp", "src/train_cmd.cpp", "src/pbs.cpp",
    "src/pbs.h", "src/table/track_data.h", "src/table/settings/pathfinding_settings.ini",
    "src/tests/CMakeLists.txt",
)


def main():
    """Run one original native writer and retain its actual queued/starved/cleared worlds.

    @return None; empty/failed native selection or missing original owner prevents a producer report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "queue-files")
    context.prepare_runtime()
    output, proof = native(context, args.config, "[tube-queue-save-file]", "writer")
    pattern = "GEOTHERMAL_WORK_SAVE " + " ".join(f"{key}=(\\d+)" for key in FIELDS) + r" path=([^\r\n]+)"
    checkpoints = []
    # Actual queued/starved originals each need their own uninterrupted next
    # common opportunity, and the original repaired world must really clear.
    # Every original file retains all participants and all126 native chunks.
    for match in re.findall(pattern, output):
        point = dict(zip(FIELDS, map(int, match[:-1])))
        path = Path(match[-1]); point["input"] = identity(path)
        assert binding_digest(path) == point["binding"]
        chunks = world_chunks(path)
        assert len(chunks) == 126 and OWNERS <= chunks.keys()
        checkpoints.append(point)
    assert {(point["phase"], point["step"]) for point in checkpoints} == {(6, 0), (6, 1), (7, 0), (7, 1), (8, 0)}
    assert len(checkpoints) == 5
    queued = re.search(r"TUBE_QUEUED primary=(\d+) secondary=(\d+) primary_packet=(\d+) secondary_packet=(\d+) junction=(\d+) mask=(\d+) winner=(\d+) powered=true stuck=true tick=(\d+)", output)
    assert queued is not None
    fields = dict(zip(("primary", "secondary", "primary_packet", "secondary_packet", "junction", "mask", "winner", "tick"), map(int, queued.groups())))
    assert fields["primary"] == fields["winner"] and fields["primary"] != fields["secondary"]
    assert fields["primary_packet"] != fields["secondary_packet"] and fields["mask"] == 36
    original = next(point for point in checkpoints if (point["phase"], point["step"]) == (6, 0))
    assert (original["vehicle"], original["packet"], original["tick"]) == (fields["primary"], fields["primary_packet"], fields["tick"])
    assert "TUBE_QUEUE_STARVED electrical_connected=true native_grant=false packets_intact=true" in output
    cleared = re.search(r"TUBE_QUEUE_CLEARED both_arrived=true reservation_released=true original_packets=true ticks=(\d+) tick=(\d+)", output)
    assert cleared is not None and int(cleared[1]) > 0 and int(cleared[2]) > fields["tick"]
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md14EB; actual normal reservation contender/Heat-loss grant starvation/repair and two arrivals; prepared floor/material/stock remain boundary inputs, not file acceptance or useful bootstrap",
        "configuration": args.config, "phase": "write", "native": proof,
        "checkpoints": checkpoints, "queue": fields, "clearing_ticks": int(cleared[1]),
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-queue-{args.config}-write.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
