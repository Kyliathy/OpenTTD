"""Consume successful compiled truck-stop geometry before dependent original art.

@file ground_terminal_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups25B/25.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native sequence order, real one-tile cells and command admission stay authoritative.
This descriptor neither activates sprites nor creates another road/loading owner.
"""

import argparse
import json
from pathlib import Path
import re
import sys

# Direct repository-root CLI execution needs the existing media helper package;
# imports and module execution resolve the same canonical source owners.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.ground_station_contract import ROOT, digest, source_digest


DESCRIPTOR = ROOT / "media/multilayer/contracts/conventional-terminal-geometry.json"
DIRECTIONS = ("NE", "SE", "SW", "NW", "X", "Y")
PARTS = ("parent-0", "parent-1", "parent-2")
OWNERS = ("src/conventional_terminal_art.h", "src/conventional_terminal_art.cpp", "src/station_cmd.cpp",
          "src/station_map.h", "src/table/station_land.h", "src/sprite.h", "src/sprite.cpp",
          "src/excavation_guard.cpp", "src/tests/conventional_terminal_art.cpp")
RENDERERS = ("src/landscape.h", "src/viewport.cpp", "src/viewport_func.h", "src/sprite.cpp")


def validate_geometry(document):
    """Require all six genuine layouts, native parent semantics and complete successful proof.

    @param document Checked descriptor or candidate complete native publication.
    @return Same document after current-owner/shape/proof validation.
    @throws ValueError On stale owners, incomplete parents or invented fallback directions.
    """
    # Exact source ownership and a complete successful focused producer prevent
    # copied draft boxes or a failed partial fixture from authorizing new art.
    if (document.get("schema_version") != 1 or document.get("owner") != "GetStationTileLayout(StationType::Truck, graphics)"
            or document.get("part_order") != list(PARTS) or document.get("native_source_hash_policy") != "utf8_lf"
            or set(document.get("native_source_sha256", {})) != set(OWNERS)):
        raise ValueError("unsupported or incomplete native terminal geometry owner")
    for name in OWNERS:
        if document["native_source_sha256"][name] != source_digest(ROOT / name):
            raise ValueError("native terminal geometry owner changed: " + name)
    proof = document.get("native_proof", {})
    # Cold checkouts need not retain ignored binaries/logs, but every published
    # descriptor must name the exact successful compiled command/loading run.
    if (proof.get("exit_code") != 0 or proof.get("test_cases") != 5 or proof.get("assertions", 0) <= 0
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("binary_sha256", ""))
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("log_sha256", ""))):
        raise ValueError("terminal geometry requires complete successful native proof")
    rows = document.get("layouts", [])
    if [row.get("graphics") for row in rows] != list(range(6)):
        raise ValueError("terminal geometry must cover six actual default graphics exactly once")
    # Bay entrances and through axes have different topology and parent counts.
    # An axis alias cannot replace a missing directed bay or third sorting owner.
    for row, direction in zip(rows, DIRECTIONS):
        through = row["graphics"] >= 4
        parents = row.get("parents", [])
        if (row.get("direction") != direction or row.get("drive_through") != through
                or row.get("entrance") != (None if through else direction)
                or row.get("axis") != (direction if through else None) or len(parents) != 3
                or parents[0] is None or parents[1] is None or (parents[2] is None) != through):
            raise ValueError("native terminal entrance/axis/parent coverage mismatch")
        for parent in parents:
            if parent is None:
                continue
            # Source registration uses actual positive native volumes inside
            # one paid tile. Unsupported offsets or heights need a new scoped
            # review rather than an inferred geometry normalization.
            if (set(parent) != {"origin", "extent", "offset"}
                    or any(len(parent[key]) != 3 for key in parent)
                    or any(type(value) is not int for key in parent for value in parent[key])
                    or any(value <= 0 for value in parent["extent"]) or parent["offset"] != [0, 0, 0]
                    or parent["origin"][2] != 0 or parent["extent"][2] != (16 if through else 10)
                    or any(parent["origin"][axis] < 0 or parent["origin"][axis] + parent["extent"][axis] > 16 for axis in (0, 1))):
                raise ValueError("invalid native terminal parent bounds")
    return document


def load_geometry():
    """Read the checked complete native producer before any dependent source operation.

    @return Validated actual truck-stop layout/source/proof descriptor.
    """
    return validate_geometry(json.loads(DESCRIPTOR.read_text(encoding="utf-8")))


def packed_anchor(layout, parent_index):
    """Compensate the actual native parent origin on the shared source canvas.

    @param layout Checked compiled truck-stop geometry row.
    @param parent_index Actual native sorting sequence index0..2.
    @return Packed image anchor, or None for the genuinely absent through parent.
    """
    parent = layout["parents"][parent_index]
    if parent is None:
        return None
    x, y, z = [origin + offset for origin, offset in zip(parent["origin"], parent["offset"])]
    return [-31 - 2 * (y - x), -33 - (x + y - z)]


def terminal_contract():
    """Publish exact modular road-terminal source topology without changing native ranges.

    @return Public geometry/anchor/state fields for four bays and two true through axes.
    """
    native = load_geometry()
    return {
        "directions": list(DIRECTIONS), "footprint": [1, 1], "tile_variants": 1,
        "parts": list(PARTS), "frames": 4, "states": ["construction", "ready", "working", "blocked"],
        "native_size": [64, 64], "anchor": [-31, -33], "bounds": [0, 0, 64, 64],
        "direction_semantics": "native_bay_entrance_or_drive_through_axis", "variant_semantics": "single_native_stop_cell",
        "native_graphics": {direction: index for index, direction in enumerate(DIRECTIONS)},
        "geometry_descriptor": DESCRIPTOR.relative_to(ROOT).as_posix(),
        "nullable_parts_by_direction": {"parent-2": ["X", "Y"]},
        "layout_owner": "GetStationTileLayout(StationType::Truck, graphics); four directed bays with three native parents, two drive-through axes with two native parents; no elevated roof or child",
        "footprint_owner": "CmdBuildRoadStop variable width/length groups real one-tile RoadStop records into native Station; no fixed2x2 source campus",
        "approach_owner": "CheckExcavationConstructionArea validates complete footprint plus bay entrance boundary or both through ends before clearing/allocation",
        "ground_owner": "Native truck-stop ground, road type overlays/catenary and vehicle turning/lane controller remain independent; no pavement, road marking, truck or ground baked into building source parents",
        "parent_order": "parent-0/parent-1/parent-2 follow the exact native sequence. Their physical front/back role varies by genuine bay direction; through parent-2 is absent",
        "native_packed_anchors": {row["direction"]: {part: packed_anchor(row, index) for index, part in enumerate(PARTS)} for row in native["layouts"]},
        "native_anchor_renderer_sha256": {name: source_digest(ROOT / name) for name in RENDERERS},
        "native_anchor_policy": "Common64x64 source(-31,-33); packed image offsets compensate RemapCoords(native origin+offset). Preserve every original sorting parent and native ground owner; decoded/game proof remains10.4D",
        "native_state_policy": {
            "construction": "Atomic native build preview only; no installed construction stages or work owner",
            "ready": "Eligible installed native truck-stop tile without a physical actual loading road-vehicle part on that tile",
            "working": "Actual Station.loading_vehicles RoadVehicle chain contains a physical part on queried tile; no station-wide loading extrapolation or art-local cargo",
            "blocked": "Refused construction preview only; no persistent default road-stop Blocked state",
            "world_states": ["ready", "working"], "preview_states": ["construction", "blocked"],
            "frame_owner": "construction/ready/blocked phase0 aliases; working existing synchronized TimerGameTick::counter controls presentation only",
            "frame_aliases": {"construction": [0, 0, 0, 0], "ready": [0, 0, 0, 0], "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0]},
        },
    }


def validate_terminal_contract(row):
    """Reject draft campuses, missing third bay parents and invented loading/preview owners.

    @param row Shared public facility.road-terminal source family.
    @return None after exact native source-field validation.
    """
    # Every required topology, anchor and state field affects physical source
    # coverage or later native selection; partial matches cannot authorize art.
    for key, expected in terminal_contract().items():
        if row.get(key) != expected:
            raise ValueError("native road-terminal source mismatch: " + key)


def publish_geometry(native_log, binary):
    """Publish only six native rows from one complete successful focused producer run.

    @param native_log Exact geometry/command/loading log with actual process exit sentinel.
    @param binary Exact executable used by the focused native proof.
    @return Newly checked descriptor; a failed or partial log performs no write.
    """
    native_log, binary = Path(native_log).resolve(), Path(binary).resolve()
    output = native_log.read_text(encoding="utf-8-sig")
    summary = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    # A partial or concatenated success cannot supply missing native commands
    # or layouts. Record the actual executable and complete process status.
    if (summary is None or int(summary[2]) != 5 or output.count("All tests passed") != 1
            or not output.rstrip().endswith("NATIVE_EXIT_CODE=0") or output.count("NATIVE_EXIT_CODE=") != 1):
        raise ValueError("incomplete or failed native road-terminal proof")
    rows = [json.loads(line.removeprefix("TERMINAL_ART_GEOMETRY ")) for line in output.splitlines() if line.startswith("TERMINAL_ART_GEOMETRY ")]
    document = {
        "_documentation": {"owner": "docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group25B",
                           "architecture": "docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
                           "scope": "Compiled native source geometry and command/loading boundaries; explicit fixture-open floors, no free paid excavation or driven-road claim"},
        "schema_version": 1, "owner": "GetStationTileLayout(StationType::Truck, graphics)", "part_order": list(PARTS),
        "native_source_hash_policy": "utf8_lf", "native_source_sha256": {name: source_digest(ROOT / name) for name in OWNERS},
        "native_proof": {"log": native_log.relative_to(ROOT).as_posix(), "log_sha256": digest(native_log),
                         "binary": binary.relative_to(ROOT).as_posix(), "binary_sha256": digest(binary),
                         "exit_code": 0, "assertions": int(summary[1]), "test_cases": int(summary[2]),
                         "fixture": "Actual bay/through variable footprint normal commands on both rock layers; native purchased-truck packet loading uses explicit fixture arrival, not driven route acceptance"},
        "layouts": rows,
    }
    validate_geometry(document)
    DESCRIPTOR.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return document


def main():
    """Publish explicit successful native evidence or inspect an existing checked descriptor.

    @return None; incomplete producer inputs refuse without public changes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-log", type=Path)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    # Both proof inputs are required together; inspection is read-only and an
    # absent executable cannot turn plausible rows into compiled native proof.
    if (args.native_log is None) != (args.binary is None):
        parser.error("--native-log and --binary must be supplied together")
    result = load_geometry() if args.native_log is None else publish_geometry(args.native_log, args.binary)
    print(f"Native truck-stop geometry: {len(result['layouts'])} layouts; four directed bays, two drive-through axes")


if __name__ == "__main__":
    main()
