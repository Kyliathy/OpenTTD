"""Verify and publish the selected native M1 runtime-consumer handoff.

@file activate_m1_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group83B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
Binding metadata never substitutes for the later crossing/loop/sound gallery or
the user's deferred personal review. Source art and gameplay definitions stay immutable.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path

import pack_m1_art as pack

ROOT = Path(__file__).resolve().parents[2]
PUBLIC = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/multilayer_sprite_manifest.json"
PACKED = ROOT / "media/multilayer/sprites/m1/packed-manifest.json"
RUNTIME = ROOT / "media/multilayer/sprites/m1/runtime-manifest.json"
CONTROL = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews/m1-control-runtime.json"
TERRAIN = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-geology-runtime.json"


def digest(path):
    """Hash an actual repository artifact without changing it.

    @param path Existing artifact whose identity is being bound.
    @return Lowercase SHA256 of its exact bytes.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def binding_for(asset):
    """Resolve the selected asset's explicit implemented consumer and authoritative state queries.

    @param asset Public selected family identity; unknown/unselected identities refuse.
    @return Selector, actual consumer source/symbol, owner descriptions and focused evidence.
    @throws ValueError If no implemented runtime consumer is declared for that family.
    """
    terrain = ["logs/r2mlvs13-be-terrain-portal-checks.log", "logs/r2mlvs13-bh-native-terrain-capture.log"]
    controls = ["logs/r2mlvs13-cj-native-control-art-checks.log", "logs/r2mlvs13-cl-native-control-capture.log"]
    trains = ["logs/r2mlvs13-bw-native-image-exchange-checks.log"]
    facilities = ["logs/r2mlvs13-ce-native-facility-art-checks.log"]
    if asset in ("terrain.hardness", "terrain.hardness-edges"):
        selector = "GetMultiLayerTerrainEdgeSprites" if asset.endswith("edges") else "GetMultiLayerTerrainSprite"
        values = (selector, "src/clear_cmd.cpp", "DrawTile_Clear", selector,
                  ["GetGeologyWorldState.terrain", "GetExcavationCellHardness", "adjacent rock owns exposed walls"],
                  ["logs/r2tbmf-z-focused-1.log", "logs/r2tbmf-z-native-1.log"])
    elif asset.startswith("terrain."):
        values = ("GetMultiLayerTerrainSprite", "src/clear_cmd.cpp", "DrawTile_Clear", "GetMultiLayerTerrainSprite",
                  ["GetGeologyWorldState.terrain", "layer-bounded TileAddWrap", "retained legacy family; superseded by terrain.hardness"], terrain)
    elif asset.startswith("survey."):
        values = ("GetMultiLayerSurveySprite", "src/viewport.cpp", "DrawTileSelection", "GetUndergroundSurveyOverlaySprite",
                  ["actual active survey cursor", "GeologyDeposit.resource", "GeologyDeposit.remaining_reserve", "existing Native public/free information"], controls)
    elif asset.startswith("vehicle."):
        values = ("GetMultiLayerTrainSprite", "src/train_cmd.cpp", "Train::GetImage", "GetMultiLayerTrainSprite",
                  ["actual EIDS/physical part direction", "GetTBMProjectForCutter/GetTBMConsistRole", "real cargo.StoredCount/cargo_cap", "IsDrivingBackwards/IsInDepot"], trains)
    elif asset.startswith("purchase."):
        values = ("GetMultiLayerEnginePurchaseSprite", "src/train_cmd.cpp", "GetRailIcon", "GetMultiLayerEnginePurchaseSprite",
                  ["actual persisted engine mapping", "IsEngineBuildable", "portal button's own can_commission"], controls)
    elif asset in ("facility.portal.sealed", "facility.portal.deep-rail"):
        values = ("GetMultiLayerPortalSprite", "src/layer_portal_cmd.cpp", "DrawTile_LayerPortal", "GetMultiLayerPortalSprite",
                  ["actual reciprocal pair", "GetLayerPortalWorkState", "existing rail/transparency/sorting"], terrain)
    elif asset in ("facility.exchange", "facility.rail-service"):
        values = ("GetMultiLayerRailDepotSprite", "src/rail_cmd.cpp", "DrawTile_Track", "GetMultiLayerRailDepotSprite",
                  ["GetTBMProjectForExchangeBay", "GetTBMExchangeAssignment", "real train depot occupancy", "custom railtype resolver/transparency"], trains)
    elif asset == "facility.road-service":
        values = ("GetMultiLayerRoadDepotSprite", "src/road_cmd.cpp", "DrawTile_Road", "GetMultiLayerRoadDepotSprite",
                  ["actual road depot/vehicle occupancy", "custom roadtype resolver/transparency"], facilities)
    elif asset in ("facility.earth-receiver", "facility.rock-receiver"):
        values = ("GetMultiLayerReceiverSprite", "src/industry_cmd.cpp", "DrawTile_Industry", "GetMultiLayerReceiverSprite",
                  ["actual Industry cells/construction", "GetSpoilReceiver/stock/room", "existing interval/aggregate bounds", "foreign resolver/transparency"], facilities)
    elif asset.startswith("control."):
        values = ("GetMultiLayerControlSprite", "src/underground_gui.cpp", "UndergroundConstructionWindow::GetWidgetIcon", "GetMultiLayerControlSprite",
                  ["actual disabled/lowered widgets", "CheckDeepLayerAvailability/IsTBMCutterAvailable", "canonical area_quote/turn_envelope"], controls)
    elif asset in ("cargo.ERTH", "cargo.ROCK"):
        values = ("GetMultiLayerCargoSprite", "src/cargotype.cpp", "CargoSpec::GetCargoIcon", "GetMultiLayerCargoSprite",
                  ["actual CargoSpec label and GRFFile owner", "immutable first-party definition package"], controls)
    else:
        raise ValueError("no implemented selected consumer: " + asset)
    selector, source, symbol, call, owners, evidence = values
    retained = asset in ("terrain.underground", "terrain.deep", "terrain.edges.underground", "terrain.edges.deep")
    return {"retained_only": retained, "superseded_by": "terrain.hardness/terrain.hardness-edges" if retained else None,
            "selector": selector, "selector_source": "src/multilayer_sprite.cpp",
            "consumer": {"source": source, "symbol": symbol, "call": call}, "state_authorities": owners,
            "evidence": evidence, "simulation_independent": True,
            "gallery_status": "final_m1_review_pending", "user_review": "deferred_until_integration"}


def read_evidence(path):
    """Read retained native logs from either PowerShell or the binary application runner.

    @param path Existing repository evidence file, including an optional UTF16 BOM.
    @return Decoded actual log text without changing its retained bytes.
    """
    encoded = path.read_bytes()
    # Windows PowerShell redirects native output as UTF16, while the capture
    # runner preserves UTF8 process bytes. Both are real evidence containers.
    encoding = "utf-16" if encoded.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    return encoded.decode(encoding, errors="replace")


def validate(public, packed, rasters):
    """Verify exact native ranges, actual loader identity and retained consumer/raster evidence.

    @param public Public family manifest candidate; unrelated later families remain inactive.
    @param packed Actual selected packing report bound to the distributed GRF.
    @param rasters Actual terrain/control raster reports, including exact PNG/source/log identities.
    @return Fully verified per-family binding metadata; no repository files are written.
    @throws AssertionError If a native range/hash, consumer or retained actual evidence is stale/missing.
    """
    expected, outputs = pack.derive()
    assert packed["families"] == expected["families"], "selected runtime ranges differ from emitted source"
    assert packed["sprites"] == expected["sprites"], "selected runtime poses differ from emitted source"
    for path, encoded in outputs.items():
        assert path.read_bytes() == encoded, "actual packed source changed after runtime evidence"
    identity = packed["packed_grf"]
    assert identity["sha256"] == digest(ROOT / identity["path"]), "distributed native payload hash is stale"
    loaded = read_evidence(ROOT / identity["loader_log"])
    assert "M1_PACKED_ART_LOADED sprites=2409 native_bound=2436 protected=66 selected=2343 pixel_hashes=exact" in loaded
    assert "All tests passed" in loaded
    # Native PNG and immutable save identities tie the real rendered controls
    # to their actual loaded world. A fabricated contact sheet cannot pass here.
    for raster in rasters:
        assert raster["simulation_owners_unchanged_during_render"]
        assert digest(ROOT / raster["source"]) == raster["source_sha256"], "runtime source save is stale"
        log = read_evidence(ROOT / raster["log"])
        assert "All tests passed" in log
        for frame in raster["frames"]:
            assert digest(ROOT / frame["path"]) == frame["sha256"], "runtime raster hash is stale"
    assert "M1_CONTROL_RUNTIME frames=6 actual_widgets=true engines=5 cargo_icons=2" in read_evidence(ROOT / rasters[1]["log"])
    families = {family["asset_id"]: family for family in public["assets"]}
    result = []
    # Bind every selected family to its implemented call site and exact emitted
    # range. Never activate later orbital/factory families from a blanket flag.
    for row in packed["families"]:
        asset = row["asset_id"]
        family = families[asset]
        assert family["packed"]["native_offset"] == row["native_offset"], "public runtime range is stale"
        assert family["packed"]["sprite_count"] == row["sprite_count"], "public runtime count is stale"
        assert family["packed"]["sha256"] == identity["sha256"], "public runtime payload is stale"
        binding = binding_for(asset)
        selector = (ROOT / binding["selector_source"]).read_text(encoding="utf-8")
        consumer = (ROOT / binding["consumer"]["source"]).read_text(encoding="utf-8")
        assert binding["selector"] + "(" in selector, "declared selector is absent"
        assert binding["consumer"]["call"] + "(" in consumer, "declared native consumer is absent"
        for evidence in binding["evidence"]:
            assert "All tests passed" in read_evidence(ROOT / evidence), "consumer evidence failed: " + evidence
        result.append({"asset_id": asset, "native_offset": row["native_offset"], "sprite_count": row["sprite_count"], **binding})
    assert len(result) == 42 and sum(row["sprite_count"] for row in result) == 2343, "selected runtime coverage is incomplete"
    return result


def verified_gallery(public, packed, bindings):
    """Preserve completed review only when its entire current evidence still verifies.

    @param public Current public selected family metadata.
    @param packed Actual current native packing report.
    @param bindings Newly validated actual consumer bindings.
    @return Repository-relative gallery path, or None when review is absent/stale.
    """
    # A later binding refresh must not silently discard valid technical review,
    # or carry it across changed frames/payloads. The gallery owner revalidates
    # actual phases, source saves, distribution and all selected family links.
    from review_m1_art import GALLERY, REPORTS, REVIEWS, read_json, validate as validate_gallery
    if not GALLERY.exists():
        return None
    try:
        record = read_json(GALLERY)
        candidate = {"packed_payload": packed["packed_grf"], "bindings": bindings}
        rows, frames = validate_gallery(public, packed, candidate, [read_json(REVIEWS / name) for name in REPORTS])
        assert record["technical_acceptance"] == "verified"
        assert record["packed_payload"] == packed["packed_grf"]
        assert record["families"] == rows and record["frames"] == frames
        for artifact in (record["contact_sheet"], record["html"]):
            assert digest(ROOT / artifact["path"]) == artifact["sha256"]
        return GALLERY.relative_to(ROOT).as_posix()
    except (AssertionError, OSError, KeyError, ValueError):
        return None


def main():
    """Validate retained native bindings, exercise bounded corruption refusals, and optionally publish.

    @return None; write only verified selected metadata when --publish is explicitly requested.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    packed = json.loads(PACKED.read_text(encoding="utf-8"))
    rasters = [json.loads(path.read_text(encoding="utf-8")) for path in (TERRAIN, CONTROL)]
    bindings = validate(public, packed, rasters)
    # Three bounded metadata corruptions must refuse before any publication:
    # stale public ranges, a substituted payload and altered actual PNG evidence.
    for fault in ("range", "payload", "raster"):
        candidate = copy.deepcopy(public)
        images = copy.deepcopy(rasters)
        if fault == "range":
            candidate["assets"][0]["packed"]["native_offset"] += 1
        elif fault == "payload":
            candidate["assets"][0]["packed"]["sha256"] = "0" * 64
        else:
            images[1]["frames"][0]["sha256"] = "0" * 64
        try:
            validate(candidate, packed, images)
        except AssertionError:
            continue
        raise AssertionError("runtime metadata corruption was accepted: " + fault)
    if args.publish:
        gallery = verified_gallery(public, packed, bindings)
        if gallery is not None:
            for binding in bindings:
                binding["gallery_status"] = "technical_review_verified_user_feedback_deferred"
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Group83B; selected actual native consumers; final gallery/user review pending; sound proof in sound-runtime.json",
                  "schema_version": 1, "packed_payload": packed["packed_grf"], "selected_families": len(bindings),
                  "selected_sprites": 2343, "bindings": bindings, "rasters": [TERRAIN.relative_to(ROOT).as_posix(), CONTROL.relative_to(ROOT).as_posix()],
                  "user_review": "deferred_until_integration"}
        if gallery is not None:
            report["gallery_manifest"] = gallery
            report["_documentation"] = "Owner: r2mlvs13-graphics-and-asset-pipeline.md83B/85B; selected native consumers and technical gallery verified; user feedback deferred"
        RUNTIME.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        selected = {row["asset_id"]: row for row in bindings}
        for family in public["assets"]:
            if family["asset_id"] not in selected:
                continue
            if selected[family["asset_id"]]["retained_only"]:
                family["authoring"].update(runtime_activated=False, status="retained_source_and_packed_range")
                family["runtime"] = {"status": "superseded_retained", "superseded_by": ["terrain.hardness", "terrain.hardness-edges"],
                                     "binding_manifest": RUNTIME.relative_to(ROOT).as_posix()}
                continue
            family["authoring"].update(runtime_activated=True, status="source_coverage_verified_runtime_bound")
            family["runtime"] = {"status": "bound_verified", "consumer": selected[family["asset_id"]]["consumer"],
                                 "binding_manifest": RUNTIME.relative_to(ROOT).as_posix(), "user_review": "deferred_until_integration"}
            if gallery is not None:
                family["runtime"]["gallery"] = {"manifest": gallery, "technical_acceptance": "verified", "user_feedback": "deferred"}
        PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
        packed["runtime_activated"] = True
        packed["runtime_binding_manifest"] = RUNTIME.relative_to(ROOT).as_posix()
        packed["_documentation"] = "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/83B; selected native bytes and actual runtime bindings; final gallery/user review pending; sound proof in sound-runtime.json"
        if gallery is not None:
            packed["gallery_manifest"] = gallery
            packed["_documentation"] = "Owner: r2mlvs13-graphics-and-asset-pipeline.md79B/80B/83B/85B; selected native packing, consumer and technical gallery verified; user feedback deferred"
        else:
            packed.pop("gallery_manifest", None)
        PACKED.write_text(json.dumps(packed, indent=2) + "\n", encoding="utf-8")
    print("Selected M1 runtime bindings: 42 families / 2343 sprites; actual consumer/range/payload/raster evidence and three corruption refusals pass; final gallery/user review pending; sound proof retained")


if __name__ == "__main__":
    main()
