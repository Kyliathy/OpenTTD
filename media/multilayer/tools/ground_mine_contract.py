"""Consume actual native finite-mine geometry before selected source authoring.

@file ground_mine_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups27B/27-35.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DB, historical source geometry only.
Definitions, geological claims, construction stages and physical Industry output
remain existing owners. This handoff grants no sprite range or mine activation.
"""

import argparse
import json
from pathlib import Path
import re
import sys


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.ground_station_contract import ROOT, digest, source_digest


DESCRIPTOR = ROOT / "media/multilayer/contracts/native-mine-geometry.json"
RESOURCES = ("COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "URAN")
SELECTED = ("COAL", "IORE", "CORE", "SALT", "LITH", "COBT", "RARE")
OWNERS = ("src/layer_industry_definitions.cpp", "src/layer_industry_policy.cpp", "src/layer_industry_mine.cpp",
          "src/multilayer_industry.h", "src/industry_cmd.cpp", "src/industry_map.h", "src/industry_type.h",
          "src/multilayer_sprite.cpp", "src/timer/timer_game_tick.h", "src/tests/layer_mine_definition.cpp",
          "src/tests/layer_mine_presentation.cpp")


def validate_geometry(document, source_only=False):
    """Require complete compiled catalogue geometry and current native state/drawing owners.

    @param document Candidate or checked native mine descriptor.
    @param source_only Reconstruct frozen physical art; grants no current native qualification.
    @return Same descriptor after exact geometry and successful proof validation.
    @throws ValueError On stale, partial, duplicated or foreign native geometry/provenance.
    """
    # Source art depends on the current actual definitions and rendering/state
    # owners. A successful historical fixture cannot authorize changed geometry.
    if (document.get("schema_version") != 1 or document.get("owner") != "InitialiseLayerMineDefinitions/ValidateMineDepositClaim"
            or document.get("native_source_hash_policy") != "utf8_lf"
            or set(document.get("native_source_sha256", {})) != set(OWNERS)):
        raise ValueError("native mine descriptor ownership/schema mismatch")
    if source_only:
        from ground_source_facts import verify_original_descriptor
        verify_original_descriptor(document, DESCRIPTOR)
    else:
        # Current native consumers still require every actual state/drawing
        # owner. Historical source reconstruction alone cannot qualify them.
        for name in OWNERS:
            if document["native_source_sha256"][name] != source_digest(ROOT / name):
                raise ValueError("native mine geometry/presentation owner changed: " + name)
    proof = document.get("native_proof", {})
    # Cold checkouts need not retain ignored executables/logs. Published source
    # geometry must still identify one complete successful native six-case run.
    if (proof.get("exit_code") != 0 or proof.get("test_cases") != 6 or proof.get("assertions", 0) <= 0
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("binary_sha256", ""))
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("log_sha256", ""))):
        raise ValueError("native mine geometry requires complete successful proof")
    rows = document.get("families", [])
    if [row.get("resource") for row in rows] != list(RESOURCES):
        raise ValueError("native mine geometry must cover the complete ordered catalogue exactly once")
    # Runtime identities and coordinate order come from native definitions.
    # A second layout, missing cell or painted loading courtyard cannot replace
    # the real fixed machinery rectangle and separate paid approach floor.
    for index, row in enumerate(rows):
        if (row.get("industry_local_id") != 0x8100 + index or row.get("layout_count") != 1
                or row.get("machinery_count") != 9 or len(row.get("cells", [])) != 9
                or row.get("loading") != [[0, 3], [1, 3], [0, 4], [1, 4]]):
            raise ValueError("native mine layout identity or separate loading floor changed")
        for variant, cell in enumerate(row["cells"]):
            if (cell.get("cell") != [variant % 3, variant // 3]
                    or cell.get("tile_local_id") != 0x8100 + index * 16 + variant
                    or type(cell.get("substitute_graphics")) is not int
                    or not 0 <= cell["substitute_graphics"] < 175):
                raise ValueError("native mine machinery coordinate/TIDS/substitute mismatch")
    return document


def load_geometry(source_only=False):
    """Read the complete checked native producer before dependent source operations.

    @param source_only Explicit frozen-art role; default native freshness remains strict.
    @return Validated ten-resource geometry with its explicitly historical proof descriptor.
    """
    return validate_geometry(json.loads(DESCRIPTOR.read_text(encoding="utf-8")), source_only=source_only)


def mine_contract(resource):
    """Project one dispatched mine's exact fixed native geometry and presentation ownership.

    @param resource Selected content-owned mineral label, never a cargo slot or translated name.
    @return Public source fields for one real nine-cell native mine family.
    @throws ValueError For unselected resources whose source remains separately pending.
    """
    if resource not in SELECTED:
        raise ValueError("mine source contract is outside the selected material scope")
    native = next(row for row in load_geometry()["families"] if row["resource"] == resource)
    return {
        "directions": ["none"], "frames": 4, "states": ["construction", "working", "blocked", "depleted"],
        "footprint": [3, 3], "tile_variants": 9, "parts": ["back", "front"], "geometry": "per_tile",
        "native_size": [64, 64], "anchor": [-31, -33], "bounds": [0, 0, 64, 64],
        "direction_semantics": "fixed_native_layout; none identifies one canonical source view without a user-selectable mine rotation",
        "variant_semantics": "actual_native_machinery_cell_row_major", "native_layout": 0,
        "native_cells": [cell["cell"] for cell in native["cells"]], "loading_cells": native["loading"],
        "geometry_descriptor": DESCRIPTOR.relative_to(ROOT).as_posix(),
        "layout_owner": "InitialiseLayerMineDefinitions/GetLayerMineIndustryType exact IIDS/TIDS; one fixed3x3 machinery layout",
        "approach_owner": "ValidateMineDepositClaim requires separate2x2 excavated loading floor; no machinery or painted source extends onto it",
        "source_part_policy": "Existing DrawTile_Industry selected-family branch draws back at native ground origin and front at origin(0,0,0), extent(16,16,20), offset(0,0,0); native mine binding remains master10.4D",
        "source_height_limit": 20,
        "native_state_policy": {
            "eligibility": "Actual first-party mine capability, physical layer and valid LayerMineState/geological binding; MissingState/foreign definitions grant no source/runtime authority",
            "construction": "Native incomplete machinery; each queried tile GetIndustryConstructionStage selects0..3, completed cells retain final3 until the whole mine completes",
            "working": "GetLayerMineOperationalStatus::Operating reads the sole native mine, geological reserve and Industry output owners",
            "blocked": "Actual Unavailable/Abandoned/Paused/OutputFull/AwaitPickup/Stopped status retains machinery, reserve and all physical output",
            "depleted": "Actual Depleted phase or zero geological remaining_reserve; final stored Industry output stays visible and accounted",
            "world_states": ["construction", "working", "blocked", "depleted"],
            "frame_owner": "construction actual GetIndustryConstructionStage; working existing synchronized TimerGameTick::counter presentation only; blocked/depleted static phase0 aliases",
            "frame_aliases": {"construction": [0, 1, 2, 3], "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0], "depleted": [0, 0, 0, 0]},
        },
    }


def validate_mine_contract(row):
    """Reject draft rotations, missing real cells or art-owned reserve/work/output state.

    @param row Shared selected mine family record.
    @return None after exact native source contract validation.
    """
    resource = row["asset_id"].removeprefix("mine.")
    # Every field affects actual required source coverage or future native
    # selection. A partial topology/state match cannot authorize source art.
    for key, expected in mine_contract(resource).items():
        if row.get(key) != expected:
            raise ValueError("native mine source mismatch: " + key)


def publish_geometry(native_log, binary):
    """Publish actual native layout traces only from a complete successful focused producer.

    @param native_log Exact catalogue/funding/refusal/presentation log with process exit sentinel.
    @param binary Actual executable used by that complete native proof.
    @return Newly checked descriptor; failed/partial/duplicate candidates perform no write.
    """
    native_log, binary = Path(native_log).resolve(), Path(binary).resolve()
    output = native_log.read_text(encoding="utf-8-sig")
    summary = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    # A partial, concatenated or failed run cannot supply missing catalogue
    # cells or prove the real command/loading-floor boundary passed.
    if (summary is None or int(summary[2]) != 6 or output.count("All tests passed") != 1
            or not output.rstrip().endswith("NATIVE_EXIT_CODE=0") or output.count("NATIVE_EXIT_CODE=") != 1):
        raise ValueError("incomplete or failed native mine proof")
    rows = [json.loads(line.removeprefix("MINE_ART_GEOMETRY ")) for line in output.splitlines() if line.startswith("MINE_ART_GEOMETRY ")]
    document = {
        "_documentation": {"owner": "docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group27B",
                           "architecture": "docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
                           "scope": "Actual installed layout/funding/finite production/presentation with explicit fixture excavation; source only, no native sprite activation"},
        "schema_version": 1, "owner": "InitialiseLayerMineDefinitions/ValidateMineDepositClaim", "native_source_hash_policy": "utf8_lf",
        "native_source_sha256": {name: source_digest(ROOT / name) for name in OWNERS},
        "native_proof": {"log": native_log.relative_to(ROOT).as_posix(), "log_sha256": digest(native_log),
                         "binary": binary.relative_to(ROOT).as_posix(), "binary_sha256": digest(binary),
                         "exit_code": 0, "assertions": int(summary[1]), "test_cases": int(summary[2]),
                         "fixture": "Actual native nine-cell funding/finite extraction for all ten definitions;14 selected missing machinery/loading-floor configurations preserve geology/cash/RNG/allocation; existing read-only lifecycle/output/depletion presentation"},
        "families": rows,
    }
    validate_geometry(document)
    DESCRIPTOR.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return document


def main():
    """Publish explicit successful native proof or inspect the current checked descriptor.

    @return None; no source family/range or runtime owner is activated by this command.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-log", type=Path)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    # Publication requires both actual proof inputs. An omitted executable
    # cannot promote plausible source-only JSON into compiled native geometry.
    if (args.native_log is None) != (args.binary is None):
        parser.error("--native-log and --binary must be supplied together")
    result = load_geometry() if args.native_log is None else publish_geometry(args.native_log, args.binary)
    print(f"Native mines: {len(result['families'])} definitions; one fixed3x3 layout and separate2x2 loading floor")


if __name__ == "__main__":
    main()
