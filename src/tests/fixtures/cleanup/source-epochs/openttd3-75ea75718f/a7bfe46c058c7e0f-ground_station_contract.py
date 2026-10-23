"""Consume compiled conventional station geometry without creating gameplay state.

@file ground_station_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The checked descriptor projects the native default drawing owner. It supplies
source geometry only; packing, rendering, station commands and cargo stay with
their existing owners. No private geometry table is parsed or duplicated here.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[3]
DESCRIPTOR = ROOT / "media/multilayer/contracts/conventional-station-geometry.json"
OWNERS = (
    "src/conventional_station_art.h", "src/conventional_station_art.cpp",
    "src/station_cmd.cpp", "src/station_map.h", "src/table/station_land.h",
    "src/sprite.h", "src/sprite.cpp", "src/excavation_guard.cpp", "src/tests/conventional_station_art.cpp",
)
ROLES = ("platform", "loading-building", "canopy-rear", "canopy-front")
SOURCE_ANCHOR = (-31, -33)
ANCHOR_RENDERERS = ("src/landscape.h", "src/viewport.cpp", "src/viewport_func.h", "src/sprite.cpp")


def digest(path):
    """Identify an exact source or native proof input.

    @param path Existing file to hash without modifying it.
    @return SHA256 hexadecimal identity.
    """
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def source_digest(path):
    """Identify native source text consistently across Git checkout line endings.

    @param path Existing UTF8 native source file; its content is read only.
    @return SHA256 of UTF8 text with normalized LF line endings.
    """
    return hashlib.sha256(Path(path).read_text(encoding="utf-8").encode("utf-8")).hexdigest()


def validate_geometry(document):
    """Check complete compiled geometry and its current native source owners.

    @param document Checked-in native descriptor, or a candidate for publication.
    @return The same descriptor after complete validation.
    @throws ValueError If geometry, native source ownership or proof is incomplete.
    """
    # A complete native run and exact source ownership prevent a plausible
    # draft, failed test or renderer fallback from authorizing dependent art.
    if (document.get("schema_version") != 2 or document.get("owner") != "GetStationTileLayout(StationType::Rail, graphics)"
            or document.get("part_order") != ["back", "front", "roof"]):
        raise ValueError("unsupported conventional station geometry owner")
    owners = document.get("native_source_sha256", {})
    # Git's Windows/Unix line-ending policy cannot invalidate identical native
    # geometry. Native source proof uses canonical text; executable/art inputs
    # continue to retain their exact byte identities.
    if set(owners) != set(OWNERS) or document.get("native_source_hash_policy") != "utf8_lf":
        raise ValueError("incomplete native station geometry source ownership")
    # Changes to a renderer, layout, axis or approach owner require a new
    # compiled proof; a Python-only copied table cannot silently bless them.
    for name in OWNERS:
        if owners[name] != source_digest(ROOT / name):
            raise ValueError("native station geometry owner changed: " + name)
    proof = document.get("native_proof", {})
    # Retain provenance without requiring ignored build/log files in a cold
    # checkout. Current native source hashes remain the reproducibility gate.
    if (proof.get("exit_code") != 0 or proof.get("test_cases") != 4 or proof.get("assertions", 0) <= 0
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("binary_sha256", ""))
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("log_sha256", ""))):
        raise ValueError("station geometry requires complete successful native proof")
    rows = document.get("layouts", [])
    if [row.get("graphics") for row in rows] != list(range(8)):
        raise ValueError("station geometry must cover all eight default graphics exactly once")
    # Each native graphics pair names one modular role. Validate native parent
    # structure and axis-aligned platform corridors without inventing bounds for
    # the small building or normalizing the asymmetric native canopy pillars.
    for row in rows:
        graphics = row["graphics"]
        canopy = graphics >= 4
        parts = row.get("parts", [])
        if (row.get("axis") != ("Y" if graphics & 1 else "X") or row.get("role") != graphics // 2
                or row.get("roof_children") != int(canopy) or len(parts) != 3
                or parts[0] is None or parts[1] is None or (parts[2] is not None) != canopy):
            raise ValueError("native station axis/role/parent coverage mismatch")
        # Positive native boxes and explicit zero offsets are required for
        # predictable source registration. Future offset changes need a scoped
        # source-contract revision, even when the upstream drawing is valid.
        for part in parts:
            if part is None:
                continue
            if (set(part) != {"origin", "extent", "offset"}
                    or any(len(part[key]) != 3 for key in ("origin", "extent", "offset"))
                    or any(type(value) is not int for key in part for value in part[key])
                    or any(value <= 0 for value in part["extent"]) or part["offset"] != [0, 0, 0]):
                raise ValueError("invalid native station parent bounds")
        back, front, roof = parts
        expected_front = [11, 0, 0] if graphics & 1 else [0, 11, 0]
        expected_strip = [5, 16] if graphics & 1 else [16, 5]
        if (back["origin"] != [0, 0, 0] or front["origin"] != expected_front
                or back["extent"][:2] != expected_strip or front["extent"] != expected_strip + [2]):
            raise ValueError("native station platform corridor mismatch")
        if canopy:
            if roof != {"origin": [0, 0, 16], "extent": [16, 16, 10], "offset": [0, 0, 0]}:
                raise ValueError("station roof must retain its separate elevated sorting parent")
        # Glass is a native screen-space child of that roof, rather than a
        # fourth bounding box or opaque pixels baked into the structural roof.
        # Compiled proof checks the actual palette token, modifier and offset.
        expected_glass = {"screen_offset": [0, 0], "transparent": True, "palette": "PALETTE_TO_TRANSPARENT"} if canopy else None
        if row.get("roof_glass") != expected_glass:
            raise ValueError("station glass must retain its native transparent roof-child attachment")
    return document


def load_geometry():
    """Read the checked descriptor before any dependent station source selection.

    @return Fully validated native geometry and proof provenance.
    """
    return validate_geometry(json.loads(DESCRIPTOR.read_text(encoding="utf-8")))


def native_packed_anchor(layout, part):
    """Map the common source canvas to the actual native parent/child image origin.

    @param layout Checked compiled station geometry row, including actual image offsets.
    @param part Back/front/roof parent or the native relative glass child.
    @return Packed sprite offset in base pixels, or None for a structurally absent part.
    @throws ValueError For an unknown source part.
    """
    if part == "glass":
        # ViewportDrawParentSprites places a relative child at parent.left/top,
        # which already includes the roof image's packed offset. A full-canvas
        # glass mask therefore uses zero image offset, retaining native(0,0)
        # child displacement instead of applying the source anchor twice.
        return [0, 0] if layout["roof_glass"] is not None else None
    names = ("back", "front", "roof")
    if part not in names:
        raise ValueError("unknown conventional station source part")
    bounds = layout["parts"][names.index(part)]
    if bounds is None:
        return None
    x, y, z = [bounds["origin"][index] + bounds["offset"][index] for index in range(3)]
    # AddSortableSpriteToDraw remaps origin+offset before applying the GRF
    # image offset. Compensate that exact projection in the packed anchor so
    # all source pixels keep their common tile canvas, without changing any
    # native sorting box. RemapCoords multiplies both terms by ZOOM_BASE later.
    return [SOURCE_ANCHOR[0] - 2 * (y - x), SOURCE_ANCHOR[1] - (x + y - z)]


def station_contract():
    """Define source poses for native modular cells and actual existing state owners.

    @return Shared manifest fields, leaving source delivery/packing untouched.
    """
    geometry = load_geometry()
    return {
        "directions": ["NE", "SE"], "footprint": [1, 1], "tile_variants": 4,
        "parts": ["back", "front", "roof", "glass"], "frames": 4,
        "anchor": list(SOURCE_ANCHOR), "native_size": [64, 64], "bounds": [0, 0, 64, 64],
        "states": ["construction", "ready", "working", "blocked"],
        "variant_semantics": "native_station_graphics_role",
        "native_graphics": {"NE": [0, 2, 4, 6], "SE": [1, 3, 5, 7]},
        "geometry_descriptor": DESCRIPTOR.relative_to(ROOT).as_posix(),
        "layout_owner": "RailStationTileLayout<StationType::Rail>; variable platform count/length on native AxisX/Y, no fixed multi-tile building",
        "approach_owner": "CmdBuildRailStation/CheckExcavationConstructionArea; entire native footprint and both end approaches of every platform, before allocation/clearing",
        "direction_axes": {"NE": {"axis": "X", "approaches": ["NE", "SW"]},
                           "SE": {"axis": "Y", "approaches": ["NW", "SE"]}},
        "nullable_parts": {"roof": [0, 1], "glass": [0, 1]},
        "roof_owner": "Separate native structure parent at world z16; absent on platform/loading-building roles; never merged into a low foreground wall or opaque glass composite",
        "glass_owner": "Actual native roof child, separate original source with screen offset(0,0), transparency modifier and PALETTE_TO_TRANSPARENT; attached to the roof parent, no independent sorting box",
        "native_child_parts": {"glass": {"parent": "roof", "screen_offset": [0, 0], "transparent": True, "palette": "PALETTE_TO_TRANSPARENT"}},
        # Every supported native graphics/part gets its actual image origin;
        # missing roofs/glass remain absent and the renderer hashes bind this
        # derived packing rule to the inspected parent/child implementation.
        "native_packed_anchors": {str(layout["graphics"]): {part: native_packed_anchor(layout, part)
                                                         for part in ("back", "front", "roof", "glass")}
                                  for layout in geometry["layouts"]},
        "native_anchor_renderer_sha256": {name: source_digest(ROOT / name) for name in ANCHOR_RENDERERS},
        "native_anchor_policy": "Source frames share(-31,-33). Packed parent offsets compensate actual RemapCoords(origin+offset); relative native glass uses packed(0,0) from roof image left/top. Preserve original three sorting boxes and native child displacement; decoded/ordinary-game proof remains master10.4D.",
        "ground_owner": "Actual RailType ground/overlay, reservation and catenary; station originals contain no track, train or replacement floor",
        "native_state_policy": {
            "construction": "Build preview only; ordinary CmdBuildRailStation is atomic and has no installed construction stage",
            "ready": "Eligible installed native tile with no actual loading train part on that tile",
            "working": "Actual Station::loading_vehicles Train chain has a physical part on the queried station tile; no station-wide or off-platform extrapolation",
            "blocked": "Refused construction preview only; default station tile flags have no persistent Blocked state",
            "world_states": ["ready", "working"], "preview_states": ["construction", "blocked"],
            "frame_owner": "construction/ready/blocked fixed phase0 aliases; atomic default station build preview has no construction stages. Working uses existing synchronized TimerGameTick::counter presentation phases only, without cargo/work mutation",
            "frame_aliases": {"construction": [0, 0, 0, 0], "ready": [0, 0, 0, 0],
                              "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0]},
        },
    }


def validate_station_contract(row):
    """Reject draft footprints, lost roof parents and invented station state owners.

    @param row Public facility.rail-station manifest record to inspect.
    @return None after exact native source contract validation.
    @throws ValueError If any required geometry/state ownership field changed.
    """
    # Each declared field controls source coverage or physical sorting. A
    # partial match must not approve a simpler building or new gameplay state.
    for key, expected in station_contract().items():
        if row.get(key) != expected:
            raise ValueError("conventional station native source mismatch: " + key)


def publish_geometry(native_log, binary):
    """Publish native rows only from a complete successful focused command run.

    @param native_log Exact log containing geometry records, Catch summary and exit sentinel.
    @param binary Actual focused native executable whose identity accompanies the run.
    @return Newly written checked descriptor.
    @throws ValueError Before writing on failed, incomplete or ambiguous evidence.
    """
    native_log, binary = Path(native_log).resolve(), Path(binary).resolve()
    output = native_log.read_text(encoding="utf-8-sig")
    summary = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    # Failed tests and repeated partial runs cannot masquerade as one complete
    # producer. The command wrapper appends its actual process exit status.
    if (summary is None or int(summary[2]) != 4 or output.count("All tests passed") != 1
            or not output.rstrip().endswith("NATIVE_EXIT_CODE=0") or output.count("NATIVE_EXIT_CODE=") != 1):
        raise ValueError("incomplete or failed native station proof")
    rows = [json.loads(line.removeprefix("STATION_ART_GEOMETRY "))
            for line in output.splitlines() if line.startswith("STATION_ART_GEOMETRY ")]
    document = {
        "_documentation": {"owner": "docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups24B/24C",
                           "architecture": "docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
                           "scope": "Compiled read-only native layout snapshot for source authoring; no sprite activation or gameplay authority"},
        "schema_version": 2, "owner": "GetStationTileLayout(StationType::Rail, graphics)",
        "part_order": ["back", "front", "roof"],
        "native_source_hash_policy": "utf8_lf",
        "native_source_sha256": {name: source_digest(ROOT / name) for name in OWNERS},
        "native_proof": {"log": native_log.relative_to(ROOT).as_posix(), "log_sha256": digest(native_log),
                         "binary": binary.relative_to(ROOT).as_posix(), "binary_sha256": digest(binary),
                         "exit_code": 0, "assertions": int(summary[1]), "test_cases": int(summary[2]),
                         "fixture": "Explicit geological fixture floor; actual normal station commands on both rock layers/axes and variable dimensions; no paid TBM bootstrap claim"},
        "layouts": rows,
    }
    validate_geometry(document)
    DESCRIPTOR.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return document


def main():
    """Run explicit native-proof publication or read-only descriptor inspection.

    @return None; invalid/incomplete producer evidence raises before publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-log", type=Path)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    # Both proof inputs are necessary: absent binary provenance cannot turn a
    # plausible log into native production, and inspection performs no writes.
    if (args.native_log is None) != (args.binary is None):
        parser.error("--native-log and --binary must be supplied together")
    document = load_geometry() if args.native_log is None else publish_geometry(args.native_log, args.binary)
    print(f"Conventional station native geometry: {len(document['layouts'])} layouts; four modular roles, two axes, separate elevated roofs")


if __name__ == "__main__":
    main()
