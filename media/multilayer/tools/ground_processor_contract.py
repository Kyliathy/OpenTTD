"""Consume compiled Surface processor geometry/state before selected original art.

@file ground_processor_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B/39-42.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group41B current native qualification.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Industry owns physical stock; existing native recipe/control/work own production.
This contract allocates neither a sprite range nor runtime rendering authority.
"""

import argparse
import json
from pathlib import Path
import re
import sys


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.ground_station_contract import ROOT, digest, source_digest


DESCRIPTOR = ROOT / "media/multilayer/contracts/native-processor-geometry.json"
SELECTED = ("facility.metal-refinery", "facility.chemical-works", "facility.battery-works", "facility.electronics-works")
PRIMARY = ("CFMT", "CHEM", "BATT", "ELEC")
OWNERS = ("src/layer_industry_definitions.cpp", "src/layer_industry_policy.cpp", "src/layer_industry_processor.cpp",
          "src/layer_industry_processor.h", "src/layer_processor_presentation.cpp", "src/layer_processor_presentation.h",
          "src/multilayer_recipe.cpp", "src/multilayer_recipe.h", "src/multilayer_industry.h", "src/industry_map.h",
          "src/industry_cmd.cpp", "src/multilayer_sprite.cpp", "src/CMakeLists.txt", "src/tests/CMakeLists.txt",
          "src/tests/layer_processor_definition.cpp", "src/tests/layer_processor_presentation.cpp", "src/tests/layer_processor.cpp")


def validate_geometry(document):
    """Require exact compiled selected native families and current physical/state owners.

    @param document Candidate or published processor geometry and successful native proof.
    @return Same checked document; stale owners, false proof and partial geometry refuse.
    @throws ValueError When native provenance or any ordered machinery identity differs.
    """
    # Actual compiled owners authorize source layout/state. Plausible JSON,
    # older test success or another facility's geometry cannot replace them.
    if (document.get("schema_version") != 1 or document.get("owner") != "InstallLayerProcessorDefinition/GetLayerProcessorOperationalStatus"
            or document.get("native_source_hash_policy") != "utf8_lf"
            or set(document.get("native_source_sha256", {})) != set(OWNERS)):
        raise ValueError("native processor ownership/schema mismatch")
    changed = [name for name in OWNERS if document["native_source_sha256"][name] != source_digest(ROOT / name)]
    if changed:
        # Later shared-code changes need a separate current compiled proof of
        # this exact original geometry. Keep all historical source hashes and
        # completed artwork intact rather than rebasing their old descriptor.
        from tools.ground_processor_qualification import validate_qualification
        validate_qualification(document, OWNERS)
    proof = document.get("native_proof", {})
    # Cold source checkouts may omit ignored native artifacts, but the public
    # handoff must still identify one complete seven-case compiled success.
    if (proof.get("exit_code") != 0 or proof.get("test_cases") != 7 or proof.get("assertions", 0) <= 0
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("binary_sha256", ""))
            or not re.fullmatch(r"[a-f0-9]{64}", proof.get("log_sha256", ""))):
        raise ValueError("native processor geometry requires successful complete proof")
    rows = document.get("families", [])
    if [row.get("asset_id") for row in rows] != list(SELECTED):
        raise ValueError("native processor geometry requires all four selected families exactly once")
    # Native IIDS/TIDS and actual row-major cells define one4x4 layout. There
    # is no art rotation or extra loading courtyard hidden in the source count.
    for index, row in enumerate(rows):
        if (row.get("primary_recipe") != PRIMARY[index] or row.get("industry_local_id") != 0x8200 + index
                or row.get("layout_count") != 1 or row.get("machinery_count") != 16 or len(row.get("cells", [])) != 16):
            raise ValueError("native processor layout or primary identity changed")
        for variant, cell in enumerate(row["cells"]):
            if (cell.get("cell") != [variant % 4, variant // 4] or cell.get("tile_local_id") != 0x8200 + index * 16 + variant
                    or type(cell.get("substitute_graphics")) is not int or not 0 <= cell["substitute_graphics"] < 175):
                raise ValueError("native processor machinery coordinate/TIDS/substitute mismatch")
    return document


def load_geometry():
    """Read checked compiled processor geometry before dependent source operations.

    @return Validated four-family native layout/state descriptor.
    """
    return validate_geometry(json.loads(DESCRIPTOR.read_text(encoding="utf-8")))


def processor_contract(asset_id):
    """Project one selected processor's actual native cell and derived-state source requirements.

    @param asset_id Existing capability-owned selected Surface family, never an industry display name.
    @return Exact one-layout/sixteen-cell source fields with native quantity/control ownership.
    @throws ValueError For undispatched processors whose source remains separately pending.
    """
    if asset_id not in SELECTED:
        raise ValueError("processor source contract is outside selected scope")
    native = next(row for row in load_geometry()["families"] if row["asset_id"] == asset_id)
    return {"directions": ["none"], "frames": 4, "states": ["construction", "ready", "working", "blocked"],
            "footprint": [4, 4], "tile_variants": 16, "parts": ["back", "front"], "geometry": "per_tile",
            "native_size": [64, 64], "anchor": [-31, -33], "bounds": [0, 0, 64, 64],
            "direction_semantics": "fixed_native_layout; none identifies the installed canonical source view without a user-selectable processor rotation",
            "variant_semantics": "actual_native_machinery_cell_row_major", "native_layout": 0,
            "native_cells": [cell["cell"] for cell in native["cells"]],
            "geometry_descriptor": DESCRIPTOR.relative_to(ROOT).as_posix(),
            "layout_owner": "InstallLayerProcessorDefinition/GetLayerProcessorIndustryType exact IIDS/TIDS; one fixed4x4 layout",
            "source_part_policy": "Existing DrawTile_Industry selected branch: original back at ground origin; front origin(0,0,0), extent(16,16,20), offset(0,0,0); native processor binding remains master10.4D",
            "source_height_limit": 20,
            "native_state_policy": {
                "eligibility": "Actual selected first-party processor, valid native LayerProcessorState and supported recipe; MissingState/InvalidState grant no authored pose authority",
                "construction": "Each actual owned tile GetIndustryConstructionStage selects0..3; already complete cells stay3 while IsLayerIndustryComplete waits for the whole industry",
                "ready": "GetLayerProcessorOperationalStatus::Idle: actual required input cannot supply a complete batch, independently of banked work; final output remains physical",
                "working": "GetLayerProcessorOperationalStatus::Working: shared CaptureLayerProcessorInventory/common PlanMultiLayerRecipeBatches admit one batch's actual all-input/output prerequisites; presentation earns no work",
                "blocked": "Actual OutputFull/Paused/Stopped/Unavailable; Industry input/output and existing recipe/work/history remain unchanged",
                "quantity_owner": "Existing Industry accepted/produced slots and common native atomic recipe commit; no source-state cargo inventory",
                "world_states": ["construction", "ready", "working", "blocked"],
                "frame_owner": "construction actual GetIndustryConstructionStage; working existing synchronized TimerGameTick::counter presentation only; ready/blocked static phase0 aliases",
                "frame_aliases": {"construction": [0, 1, 2, 3], "ready": [0, 0, 0, 0], "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0]}}}


def validate_processor_contract(row):
    """Reject draft rotations, shifted cells or art-owned physical processing state.

    @param row Existing shared selected Surface processor family row.
    @return None after every native source geometry/state field agrees.
    """
    # Each field affects actual coverage, placement or future native selection;
    # a partial match cannot authorize a guessed source matrix.
    for key, expected in processor_contract(row["asset_id"]).items():
        if row.get(key) != expected:
            raise ValueError("native processor source mismatch: " + key)


def publish_geometry(native_log, binary):
    """Publish native trace cells only after one complete successful focused executable run.

    @param native_log Explicit native seven-case log ending with one successful process sentinel.
    @param binary Actual isolated executable used to produce that run.
    @return Checked descriptor; failed/partial/duplicate or foreign geometry performs no write.
    """
    native_log, binary = Path(native_log).resolve(), Path(binary).resolve()
    output = native_log.read_text(encoding="utf-8-sig")
    summary = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    # Concatenated runs, partial filters and a failed native process never
    # promote source-only JSON into a compiled physical consumer handoff.
    if (summary is None or int(summary[2]) != 7 or output.count("All tests passed") != 1
            or not output.rstrip().endswith("NATIVE_EXIT_CODE=0") or output.count("NATIVE_EXIT_CODE=") != 1):
        raise ValueError("incomplete or failed native processor proof")
    rows = [json.loads(line.removeprefix("PROCESSOR_ART_GEOMETRY ")) for line in output.splitlines()
            if line.startswith("PROCESSOR_ART_GEOMETRY ")]
    document = {"_documentation": {
        "owner": "docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B",
        "architecture": "docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
        "scope": "Actual native command-body quoted funding/complete4x4 construction and pure stock/work/calendar status; injected full/history/malformed boundary probes are not a driven game loop"},
        "schema_version": 1, "owner": "InstallLayerProcessorDefinition/GetLayerProcessorOperationalStatus",
        "native_source_hash_policy": "utf8_lf", "native_source_sha256": {name: source_digest(ROOT / name) for name in OWNERS},
        "native_proof": {"log": native_log.relative_to(ROOT).as_posix(), "log_sha256": digest(native_log),
                         "binary": binary.relative_to(ROOT).as_posix(), "binary_sha256": digest(binary),
                         "exit_code": 0, "assertions": int(summary[1]), "test_cases": int(summary[2]),
                         "fixture": "Four actual fixed16-cell native families; pure all-input/work/output/history/pause/construction/date/foreign/missing-state status; existing atomic processor batch cases"},
        "families": rows}
    validate_geometry(document)
    DESCRIPTOR.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return document


def main():
    """Inspect current geometry or publish an explicitly successful compiled native handoff.

    @return None; this command neither creates art nor assigns packed/native sprite ranges.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-log", type=Path)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    # Both actual proof inputs are mandatory for publication; absent ignored
    # native artifacts cannot be replaced by fabricated layout declarations.
    if (args.native_log is None) != (args.binary is None):
        parser.error("--native-log and --binary must be supplied together")
    result = load_geometry() if args.native_log is None else publish_geometry(args.native_log, args.binary)
    print(f"Native selected processors: {len(result['families'])}; one fixed4x4 layout; native stock/work status")


if __name__ == "__main__":
    main()
