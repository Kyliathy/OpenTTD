"""Audit actual selected tube/first-geothermal native consumers without promoting missing artwork.

@file selected_runtime_audit.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84B.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The existing public manifest/packing owner supplies every identity; this report creates no sprite or gameplay registry.
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
from local_electrical_feed_checks import identity, native
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native

CASES = ("[tube-guideway-art-geometry]", "[tube-capsule-art-body]", "[tube-capsule-art-purchase]",
         "[tube-facility-art-service]", "[tube-conduit-art-native]", "[geothermal-control-inspection]",
         "[geothermal-generator-ports]", "[m1-sprite-selection]")
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
SCOPE = ROOT / "media/multilayer/m1-pack-scope.json"
HISTORY = ("logs/tubes/guideway-art/20261017-native-01/verification.json",
           "logs/tubes/capsule-art/20261018-native-01/verification.json",
           "logs/tubes/facility-art/20261018-native-01/verification.json",
           "logs/tubes/forge-files/20261018-native-01/five-model-verification.json",
           "logs/geothermal/first-plant/20261017-native-01/window-raster-verification.json",
           "logs/geothermal/lifecycle-files/20261018-native-01/verification.json")


def focused(configuration):
    """Verify the current real selected producer boundaries in isolated native processes.

    @param configuration Actual current Debug or optimized test executable.
    @return None; print a report only after every focused original selector/owner boundary passes.
    """
    context = current_run_context("graphics", "selected-runtime-audit"); proofs = []
    # Each native fixture runs in its own process, so pooled source/cargo/
    # service state cannot leak from a prior case into an apparent art pass.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"producer-{index}"); proofs.append({"case": case, **proof})
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84B.1; existing actual native producer audit, not missing-art activation",
              "configuration": configuration, "native": proofs, "controller": identity(__file__),
              "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "public": identity(PUBLIC), "scope": identity(SCOPE)}
    path = context.output("reports", f"selected-runtime-audit-{configuration}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def selected_assets(public):
    """Select only the master-dispatched existing tube/first-geothermal families.

    @param public Existing canonical public sprite contract; no identities are generated here.
    @return Twenty-seven exact existing asset rows, including the already published collector.
    """
    families = {"facility.tube-terminal", "facility.tube-service", "facility.tube-interchange", "facility.vent-collector",
                "facility.vent-powerhouse", "facility.magma-powerhouse", "facility.utility-riser", "control.tube", "control.geothermal"}
    # Native models09..13 already own the five capsule/purchase meanings.
    # Packed conventional05..08 and later facilities stay outside this dispatch.
    families.update(f"{prefix}.model-{model:02}" for prefix in ("vehicle", "purchase") for model in range(9, 14))
    rows = [row for row in public["assets"] if row["asset_id"] in families or row["asset_id"].startswith(("tube.", "geothermal."))]
    assert len(rows) == 27 and len({row["asset_id"] for row in rows}) == 27
    return rows


def consumer(asset):
    """Describe the actual callback or exact missing native marker for one selected family.

    @param asset Existing selected public identity.
    @return Selector/call-site source and presence evidence; absence remains an implementation task rather than a metadata binding.
    """
    if asset.startswith("vehicle."):
        result = ("src/transport_tube_vehicle_sprite.cpp", "GetTubeTrainSprite(", "src/multilayer_sprite.cpp", "GetTubeTrainSprite(")
    elif asset.startswith("purchase."):
        result = ("src/transport_tube_vehicle_sprite.cpp", "GetTubeEnginePurchaseSprite(", "src/multilayer_sprite.cpp", "GetTubeEnginePurchaseSprite(")
    elif asset == "tube.conduit":
        result = ("src/transport_tube_conduit_sprite.cpp", "GetTubeConduitArt(", "src/viewport.cpp", "DrawTubeConduitGround(")
    elif asset.startswith("tube."):
        result = ("src/transport_tube_sprite.cpp", "GetTubeGuidewayArt(", "src/rail_cmd.cpp", "GetTubeGuidewayArt(")
    elif asset.startswith("facility.tube-"):
        result = ("src/transport_tube_facility_sprite.cpp", "GetTubeFacilityArt(", "src/station_cmd.cpp", "GetTubeFacilityArt(")
        if asset == "facility.tube-service":  # Actual one-tile depot uses the native depot wrapper, unlike a station tile.
            result = result[:2] + ("src/multilayer_sprite.cpp", "GetTubeFacilityArt(")
    elif asset == "facility.vent-collector":
        result = ("src/multilayer_sprite.cpp", "GetMultiLayerCollectorSprite(", "src/industry_cmd.cpp", "GetMultiLayerIndustrySprite(")
    elif asset == "geothermal.vent":
        result = ("src/geothermal_gui.cpp", "UndergroundGeothermalControls::GetSurveySprite(", "src/viewport.cpp", "GetUndergroundGeothermalSurveyOverlaySprite(")
    else:
        markers = {"facility.vent-powerhouse": "MultiLayerSpriteAsset::FacilityVentPowerhouse", "facility.magma-powerhouse": "MultiLayerSpriteAsset::FacilityMagmaPowerhouse",
                   "facility.utility-riser": "MultiLayerSpriteAsset::FacilityUtilityRiser", "geothermal.conduit": "MultiLayerSpriteAsset::GeothermalConduit",
                   "control.tube": "MultiLayerSpriteAsset::ControlTube", "control.geothermal": "MultiLayerSpriteAsset::ControlGeothermal"}
        assert asset in markers
        # Search only real production CPP owners. An enum declaration, source
        # packing table or test reference cannot supply a missing native draw.
        matches = [path for path in (ROOT / "src").rglob("*.cpp") if "tests" not in path.relative_to(ROOT / "src").parts and markers[asset] in path.read_text(encoding="utf-8")]
        return {"status": "native_marker_present_requires_actual_render_acceptance" if matches else "missing_native_binding",
                "marker": markers[asset], "sources": [identity(path) for path in matches], "next_group": "84BD" if asset.startswith("control.") else "84BC" if asset == "geothermal.conduit" else "84BB"}
    selector, symbol, callback, call = result
    assert symbol in (ROOT / selector).read_text(encoding="utf-8"), (asset, symbol)
    assert call in (ROOT / callback).read_text(encoding="utf-8"), (asset, call)
    return {"status": "existing_native_consumer", "selector": symbol, "selector_source": identity(ROOT / selector), "callback": call, "callback_source": identity(ROOT / callback)}


def audit(reports, output):
    """Bind exact selected packing/ranges and actual present/missing native consumers before implementation/publication.

    @param reports Current isolated producer reports for both native configurations.
    @param output Absent immutable audit under repository logs.
    @return None; missing public fields/ranges/payload/history or failed current native producers stop the handoff.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in reports:  # Current native binaries/logs/public inputs are verified freshly; no old nested binary is treated as current.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in records
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == summary.keys() and summary["Debug"] == summary["RelWithDebInfo"]
    public, scope = json.loads(PUBLIC.read_text(encoding="utf-8")), json.loads(SCOPE.read_text(encoding="utf-8"))
    packing_path = ROOT / scope["report"]; packing = json.loads(packing_path.read_text(encoding="utf-8")); ranges = {row["asset_id"]: row for row in packing["families"]}
    assert len(ranges) == len(scope["asset_ids"]) == 90 and packing["pose_count"] == 17749
    assert identity(ROOT / packing["packed_grf"]["path"])["sha256"] == packing["packed_grf"]["sha256"]
    rows = []
    # Public meanings/native ranges remain with their existing owners. This
    # audit never flips a runtime flag or substitutes a source image for pixels.
    for family in selected_assets(public):
        assert {"asset_id", "consumer", "directions", "frames", "anchor", "source"} <= family.keys()
        asset = family["asset_id"]; packed = ranges[asset]
        assert family["packed"]["native_offset"] == packed["native_offset"] and family["packed"]["sprite_count"] == packed["sprite_count"]
        assert family["packed"]["sha256"] == packing["packed_grf"]["sha256"]
        rows.append({"asset_id": asset, "native_offset": packed["native_offset"], "sprite_count": packed["sprite_count"], "current_runtime_selection": asset in scope["runtime_asset_ids"], **consumer(asset)})
    selected = {row["asset_id"] for row in rows}; assert len(selected - set(scope["runtime_asset_ids"])) == 26
    assert len(selected | set(scope["runtime_asset_ids"])) == 69
    history = [identity(ROOT / path) for path in HISTORY]  # Exact sealed old receipts stay opaque; their later-stale binaries/sources are not silently revalidated or replaced.
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84B.1; original selected bindings audit, no runtime activation/gallery/human acceptance",
              "controller": identity(__file__), "public": identity(PUBLIC), "scope": identity(SCOPE), "packing": identity(packing_path), "packed_grf": identity(ROOT / packing["packed_grf"]["path"]),
              "records": list(records.values()), "summary": summary, "families": rows, "sealed_producer_receipts": history,
              "selected_families": len(rows), "new_families": 26, "intended_total_runtime_families": 69,
              "existing_native_consumers": sum(row["status"] == "existing_native_consumer" for row in rows), "missing_native_bindings": [row["asset_id"] for row in rows if row["status"] == "missing_native_binding"],
              "native_version": 434, "remaining": "84BB native generator/riser art,84BC geothermal conduit art,84BD control icons,then84B.2 explicit runtime validation/84C actual gallery/84D publication. Whole84.3 future factories and human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"audit": identity(output), "summary": summary, "selected": len(rows), "existing": result["existing_native_consumers"], "missing": result["missing_native_bindings"]}), flush=True)


def main():
    """Select one current focused producer burst or the exact two-build selected-consumer audit.

    @return None; incomplete arguments or absent real native/packing owners prevent audit publication.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("native", "audit"), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--reports", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "native":
        assert args.config is not None; focused(args.config)
    else:
        assert args.reports is not None and args.output is not None; audit(args.reports, args.output)


if __name__ == "__main__":
    main()
