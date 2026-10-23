"""Verify and publish the selected native M1 runtime-consumer handoff.

@file activate_m1_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group83B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D; packed and delivered runtime scopes are explicit.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84B-84D; current selected native bindings precede gallery/public activation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
Binding metadata never substitutes for the later crossing/loop/sound gallery or
the user's deferred personal review. Source art and gameplay definitions stay immutable.
"""

import argparse
import copy
import hashlib
import json
import re
from pathlib import Path

import pack_m1_art as pack
from evidence import CACHE, DEFAULT_MANIFEST, EvidenceSet

ROOT = Path(__file__).resolve().parents[2]
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
PACKED = CACHE / "derived/packed-manifest.json"
RUNTIME = CACHE / "runtime-manifest.json"
_ACTIVE_EVIDENCE = None
# This is the implemented callback support boundary, not another sprite/range
# registry. The existing scope/public contract alone selects identities and
# ranges; a forged selection still cannot grant an unimplemented consumer.
SELECTED_RUNTIME_ADDITIONS = frozenset(
    [f"{prefix}.model-{model:02}" for prefix in ("vehicle", "purchase") for model in range(9, 14)] +
    ["facility.tube-terminal", "facility.tube-service", "facility.tube-interchange", "facility.vent-powerhouse", "facility.magma-powerhouse", "facility.utility-riser",
     "tube.straight", "tube.bend45", "tube.junction", "tube.end", "tube.crossing", "tube.conduit", "geothermal.vent", "geothermal.conduit", "control.tube", "control.geothermal"])


def evidence():
    """Obtain the explicitly selected proof set, requiring real generated evidence.

    @return Content-bound selection; a missing cache fails with regeneration guidance.
    """
    global _ACTIVE_EVIDENCE
    if _ACTIVE_EVIDENCE is None:
        select_evidence(DEFAULT_MANIFEST)
    return _ACTIVE_EVIDENCE


def select_evidence(path):
    """Select one named cache or review-run manifest for this validation process.

    @param path Exact evidence manifest; no historical/latest discovery is performed.
    @return Selected EvidenceSet with publication paths restricted to generated storage.
    """
    global _ACTIVE_EVIDENCE, PACKED, RUNTIME
    _ACTIVE_EVIDENCE = EvidenceSet(path)
    PACKED = _ACTIVE_EVIDENCE.output("packed")
    RUNTIME = _ACTIVE_EVIDENCE.output("runtime")
    return _ACTIVE_EVIDENCE


def digest(path):
    """Hash an actual repository artifact without changing it.

    @param path Existing artifact whose identity is being bound.
    @return Lowercase SHA256 of its exact bytes.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def runtime_ranges(packed):
    """Select only delivered consumer/retained-legacy families for current runtime proof.

    @param packed Complete current packing report, which may include source-only future consumers.
    @return Ordered ranges in the explicit delivered runtime review subset.
    @throws ValueError If the subset is absent, ambiguous or outside the packed selection.
    """
    scope = json.loads(pack.SCOPE.read_text(encoding="utf-8"))
    selected = scope.get("runtime_asset_ids", [])
    # A larger packed payload cannot grant a missing consumer. Runtime scope
    # is deliberately selected through the same art owner, never inferred from
    # image availability, a broad vehicle prefix or a convenient sprite count.
    if not selected or len(selected) != len(set(selected)) or not set(selected).issubset(scope["asset_ids"]):
        raise ValueError("delivered runtime scope is missing or ambiguous")
    rows = [row for row in packed["families"] if row["asset_id"] in selected]
    if len(rows) != len(selected) or {row["asset_id"] for row in rows} != set(selected):
        raise ValueError("delivered runtime family is missing from packed selection")
    return rows


def cpp_ignored(match):
    """Blank an ignored C++ token without moving subsequent declaration lines.

    @param match Comment or literal matched by the callback verifier's tokenizer.
    @return Whitespace retaining the original line boundaries for declaration matching.
    """
    return re.sub(r"[^\r\n]", " ", match.group())


def cpp_code(source):
    """Exclude comments and literals from actual native callback verification.

    @param source Actual C++ source text, never an enum/header/test declaration offered as a renderer.
    @return Code tokens with comments/string/character literals replaced by whitespace, preventing a commented-out or quoted call from proving a consumer.
    """
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', cpp_ignored, source)


def cpp_function(source, symbol):
    """Locate one real native function body with nested blocks and no comment/literal impostors.

    @param source Actual implementation text, independent of the public binding description.
    @param symbol Canonical native function, optionally qualified for an out-of-class definition.
    @return Normalized complete body; declarations, call sites and missing/unbalanced definitions refuse.
    """
    code = cpp_code(source)
    names = (symbol, symbol.rsplit("::", 1)[-1])
    for name in names:  # In-class widget methods omit the qualifier; ordinary out-of-class native consumers retain it.
        # These native owners declare return types on the signature line.
        # Requiring that declaration prevents an if/call block from standing
        # in for a removed selector implementation elsewhere in the file.
        declaration = r"(?m)^[ \t]*(?!(?:if|while|for|switch|return)\b)[A-Za-z_][\w:<>,*& \t]*[ \t]+"
        match = re.search(declaration + re.escape(name) + r"\s*\([^;{}]*\)\s*(?:(?:const|noexcept|override|final)\s*)*\{", code)
        if match is None:
            continue
        start = match.end() - 1; depth = 0
        for position in range(start, len(code)):  # Only real code braces define the body; quoted artwork/status strings cannot hide or invent a call edge.
            depth += (code[position] == "{") - (code[position] == "}")
            if depth == 0:
                return code[start:position + 1]
    raise AssertionError("native function body is absent: " + symbol)


def selected_binding_for(asset):
    """Describe the delivered tube/first-geothermal addition through its actual native callback chain.

    @param asset One of the twenty-six implemented additions explicitly selected by the existing scope owner.
    @return Exact selector/consumer/bridge/state-owner contract and hash-selected dated producer evidence; gallery acceptance remains pending.
    @throws ValueError If the family or selected native evidence is absent; packed pixels never provide a fallback consumer.
    """
    if asset not in SELECTED_RUNTIME_ADDITIONS:
        raise ValueError("no implemented selected addition: " + asset)
    if asset not in json.loads(pack.SCOPE.read_text(encoding="utf-8"))["runtime_asset_ids"]:
        raise ValueError("asset is not in delivered runtime scope: " + asset)
    bridges = []
    if asset.startswith("vehicle."):
        selector, selector_source = "GetTubeTrainSprite", "src/transport_tube_vehicle_sprite.cpp"
        consumer = {"source": "src/train_cmd.cpp", "symbol": "Train::GetImage", "call": "GetMultiLayerTrainSprite"}
        bridges = [{"source": "src/multilayer_sprite.cpp", "symbol": "GetMultiLayerTrainSprite", "call": selector}]
        owners, role, receipt = ["actual native capsule/consist model and direction", "real cargo hold/capacity", "existing movement/service/source wait"], "tube-body", "tube-capsule-acceptance"
    elif asset.startswith("purchase."):
        selector, selector_source = "GetTubeEnginePurchaseSprite", "src/transport_tube_vehicle_sprite.cpp"
        consumer = {"source": "src/train_cmd.cpp", "symbol": "GetRailIcon", "call": "GetMultiLayerEnginePurchaseSprite"}
        bridges = [{"source": "src/multilayer_sprite.cpp", "symbol": "GetMultiLayerEnginePurchaseSprite", "call": selector}]
        owners, role, receipt = ["actual persisted native capsule engine mapping", "native model/company/calendar purchase availability"], "tube-purchase", "tube-capsule-acceptance"
    elif asset.startswith("facility.tube-"):
        selector, selector_source = "GetTubeFacilityArt", "src/transport_tube_facility_sprite.cpp"
        consumer = {"source": "src/station_cmd.cpp", "symbol": "DrawTile_Station", "call": selector}
        if asset == "facility.tube-service":
            consumer = {"source": "src/rail_cmd.cpp", "symbol": "DrawTile_Rail", "call": "GetMultiLayerRailDepotSprite"}
            bridges = [{"source": "src/multilayer_sprite.cpp", "symbol": "GetMultiLayerRailDepotSprite", "call": selector}]
        owners, role, receipt = ["actual owned tube depot/platform geometry and joins", "original native packet queue/service state", "existing body transparency/sorting"], "tube-facility", "tube-facility-acceptance"
    elif asset in ("facility.vent-powerhouse", "facility.magma-powerhouse", "facility.utility-riser"):
        riser = asset == "facility.utility-riser"
        selector, selector_source = ("GetGeothermalRiserSprite", "src/geothermal_riser_sprite.cpp") if riser else ("GetGeothermalGeneratorSprite", "src/geothermal_sprite.cpp")
        consumer = {"source": "src/industry_cmd.cpp", "symbol": "DrawTile_Industry", "call": "GetMultiLayerIndustrySprite"}
        bridges = [{"source": "src/multilayer_sprite.cpp", "symbol": "GetMultiLayerIndustrySprite", "call": selector}]
        owners, role, receipt = ["actual consumed site/native funded footprint and construction quarters", "original paid work/material/service owners", "original shared Heat/electrical interval,ports and flow"], "machinery", "machinery-acceptance"
    elif asset in ("tube.conduit", "geothermal.conduit"):
        geothermal = asset == "geothermal.conduit"; prefix = "Geothermal" if geothermal else "Tube"
        selector, selector_source = f"Get{prefix}ConduitArt", "src/geothermal_conduit_sprite.cpp" if geothermal else "src/transport_tube_conduit_sprite.cpp"
        draw = f"Draw{prefix}ConduitGround"; consumer = {"source": "src/viewport.cpp", "symbol": "ViewportAddLandscape", "call": draw}
        bridges = [{"source": selector_source, "symbol": draw, "call": selector}]
        owners, role, receipt = ["actual installed original paid domain/path cells and native cardinal adjacency", "same owner/rock plane,physical removal and caps", "native ground-before-body rendering,no energy/route owner"], "geothermal-conduit" if geothermal else "tube-conduit", "geothermal-conduit-acceptance" if geothermal else "tube-conduit-acceptance"
    elif asset.startswith("tube."):
        selector, selector_source = "GetTubeGuidewayArt", "src/transport_tube_sprite.cpp"
        consumer = {"source": "src/rail_cmd.cpp", "symbol": "DrawTile_Rail", "call": selector}
        owners, role, receipt = ["actual installed native tube rail bits", "original endpoint/branch adjacency and source publication", "native reservation cues and foreground depth"], "tube-guideway", "tube-guideway-acceptance"
    elif asset == "geothermal.vent":
        selector, selector_source = "UndergroundGeothermalControls::GetSurveySprite", "src/geothermal_gui.cpp"
        consumer = {"source": "src/viewport.cpp", "symbol": "DrawTileSelection", "call": "GetUndergroundGeothermalSurveyOverlaySprite"}
        bridges = [{"source": "src/underground_gui.cpp", "symbol": "GetUndergroundGeothermalSurveyOverlaySprite", "call": "GetSurveySprite"}]
        owners, role, receipt = ["actual active geothermal survey cursor and matching viewed plane", "original natural vent/geology information", "survey grants no install,claim or heat"], "natural-vent", "natural-vent-acceptance"
    else:
        selector, selector_source = "GetMultiLayerControlSprite", "src/multilayer_sprite.cpp"
        consumer = {"source": "src/underground_gui.cpp", "symbol": "UndergroundConstructionWindow::GetWidgetIcon", "call": selector}
        owners, role, receipt = ["actual native tube/geothermal menu and primary action widgets", "existing disabled/lowered/date/company policy", "native caption/layout and disabled checkerboard"], "selected-controls", "selected-controls-acceptance"
    try:
        logs = [evidence().log(role + "-" + configuration) for configuration in ("debug", "optimized")]
        proof = evidence().selected("reports", receipt)
    except KeyError as error:
        raise ValueError("selected native evidence is absent: " + str(error)) from error
    return {"retained_only": False, "superseded_by": None, "selector": selector, "selector_source": selector_source, "consumer": consumer, "bridges": bridges,
            "asset_marker": "MultiLayerSpriteAsset::" + pack.cpp_name(asset), "state_authorities": owners, "evidence": logs,
            "producer_receipts": [evidence_identity(proof)], "simulation_independent": True, "gallery_status": "selected_native_gallery_pending", "user_review": "deferred_until_integration"}


def evidence_identity(path):
    """Bind an exact dated producer receipt without reinterpreting its removed historical transient files as current evidence.

    @param path Existing hash-selected native producer closure below the repository evidence owners.
    @return Relative path, byte count and SHA256 of the complete sealed artifact.
    """
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size, "sha256": digest(path)}


def validate_native_binding(asset, binding):
    """Reject false consumers and verify each selected addition's actual implementation/callback chain.

    @param asset Exact existing selected addition whose public/native range is verified separately.
    @param binding Candidate consumer description; different callbacks cannot borrow another family's valid code or evidence.
    @return Fresh exact native source identities and function-body hashes; this establishes implementation, not current pixels or user acceptance.
    """
    canonical = selected_binding_for(asset)
    for field in ("selector", "selector_source", "consumer", "bridges", "asset_marker", "state_authorities", "evidence", "producer_receipts"):
        assert binding.get(field) == canonical[field], "false selected native consumer/authority: " + asset + "/" + field
    source = (ROOT / binding["selector_source"]).read_text(encoding="utf-8")
    assert binding["asset_marker"] in cpp_code(source), "selected native family marker is absent: " + asset
    cpp_function(source, binding["selector"])
    records = []
    chain = [binding["consumer"], *binding["bridges"]]
    for index, edge in enumerate(chain):  # Every edge belongs to its named real function; a call elsewhere, comment or header cannot establish the world renderer.
        path = ROOT / edge["source"]; body = cpp_function(path.read_text(encoding="utf-8"), edge["symbol"])
        assert re.search(r"\b" + re.escape(edge["call"]) + r"\s*\(", body), "native callback edge is absent: " + asset + "/" + edge["symbol"]
        destination = chain[index + 1]["symbol"] if index + 1 < len(chain) else binding["selector"]
        assert edge["call"].rsplit("::", 1)[-1] == destination.rsplit("::", 1)[-1], "native callback chain is disconnected: " + asset
        records.append({"source": evidence_identity(path), "symbol": edge["symbol"], "call": edge["call"], "body_sha256": hashlib.sha256(body.encode("utf-8")).hexdigest()})
    for name in binding["evidence"]:
        assert "All tests passed" in read_evidence(ROOT / name), "selected native producer evidence failed: " + name
    return {"selector_source": evidence_identity(ROOT / binding["selector_source"]), "callback_chain": records}


def validate_binding_review(public, packed):
    """Validate the existing delivered review selection without publishing or claiming a new native gallery.

    @param public Existing sole public sprite-family contract or an isolated refusal-test candidate.
    @param packed Exact distributed packing report; candidate review cannot change ranges, pixels or the published payload.
    @return All selected binding descriptions, distinguishing current addition callback proof from the retained published prefix.
    """
    families = {family["asset_id"]: family for family in public["assets"]}; delivered = runtime_ranges(packed)
    assert len(families) == len(public["assets"]), "public sprite identities are ambiguous"
    payload = packed["packed_grf"]; assert digest(ROOT / payload["path"]) == payload["sha256"], "distributed native payload hash is stale"
    result = []
    for row in delivered:  # Existing selection alone supplies native extents; an authored image or broader prefix cannot create a binding or promote public runtime state.
        asset = row["asset_id"]; family = families[asset]
        for key in ("native_offset", "sprite_count"):
            assert family["packed"][key] == row[key], "public runtime range is stale: " + asset + "/" + key
        assert family["packed"]["sha256"] == payload["sha256"], "public runtime payload is stale: " + asset
        binding = binding_for(asset)
        if asset in SELECTED_RUNTIME_ADDITIONS:
            proof = validate_native_binding(asset, binding)
        else:
            # Earlier43 families retain their exact dated delivery contract;
            # this scoped review does not relabel their historical raw captures
            # as current files after cleanup or silently redo that full gallery.
            assert binding["selector"] + "(" in (ROOT / binding["selector_source"]).read_text(encoding="utf-8"), "retained native selector is absent"
            assert binding["consumer"]["call"] + "(" in (ROOT / binding["consumer"]["source"]).read_text(encoding="utf-8"), "retained native consumer is absent"
            for name in binding["evidence"]:
                assert "All tests passed" in read_evidence(ROOT / name), "retained native evidence failed: " + name
            proof = None
        result.append({"asset_id": asset, "native_offset": row["native_offset"], "sprite_count": row["sprite_count"], **binding,
                       "native_source_proof": proof, "implementation_review": "selected_addition_verified" if proof is not None else "retained_published_prefix"})
    return result


def binding_for(asset):
    """Resolve the selected asset's explicit implemented consumer and authoritative state queries.

    @param asset Public selected family identity; unknown/unselected identities refuse.
    @return Selector, actual consumer source/symbol, owner descriptions and focused evidence.
    @throws ValueError If no implemented runtime consumer is declared for that family.
    """
    scope = json.loads(pack.SCOPE.read_text(encoding="utf-8"))
    if asset not in scope["runtime_asset_ids"]:
        raise ValueError("asset is not in delivered runtime scope: " + asset)
    if asset in SELECTED_RUNTIME_ADDITIONS:
        return selected_binding_for(asset)
    # The implemented train/purchase selectors cover actual models00..04.
    # The delivered capsules use their own exact branch above. Ground05..08
    # and future models cannot borrow the old train/purchase callbacks merely
    # because their source was forged into an otherwise valid selection.
    if asset.startswith(("vehicle.", "purchase.")) and asset.rsplit("-", 1)[-1] not in ("00", "01", "02", "03", "04"):
        raise ValueError("no delivered model-specific native consumer: " + asset)
    # The two actual selected tool families use the explicit branch above;
    # other future controls cannot borrow the seven original widget bindings.
    if asset.startswith("control.") and asset not in {"control.survey", "control.excavation", "control.tbm-area",
                                                     "control.tbm-turn45", "control.exchange", "control.deep-lock", "control.future-lock"}:
        raise ValueError("no delivered control-specific native consumer: " + asset)
    terrain = [evidence().log("terrain-checks"), evidence().log("terrain-capture")]
    controls = [evidence().log("control-checks"), evidence().log("control-capture")]
    trains = [evidence().log("vehicle-checks")]
    facilities = [evidence().log("facility-checks")]
    if asset in ("terrain.hardness", "terrain.hardness-edges"):
        selector = "GetMultiLayerTerrainEdgeSprites" if asset.endswith("edges") else "GetMultiLayerTerrainSprite"
        values = (selector, "src/clear_cmd.cpp", "DrawTile_Clear", selector,
                  ["GetGeologyWorldState.terrain", "GetExcavationCellHardness", "adjacent rock owns exposed walls"],
                  [evidence().log("geology-checks"), evidence().log("geology-capture")])
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
        values = ("GetMultiLayerRailDepotSprite", "src/rail_cmd.cpp", "DrawTile_Rail", "GetMultiLayerRailDepotSprite",
                  ["GetTBMProjectForExchangeBay", "GetTBMExchangeAssignment", "real train depot occupancy", "custom railtype resolver/transparency"], trains)
    elif asset == "facility.road-service":
        values = ("GetMultiLayerRoadDepotSprite", "src/road_cmd.cpp", "DrawTile_Road", "GetMultiLayerRoadDepotSprite",
                  ["actual road depot/vehicle occupancy", "custom roadtype resolver/transparency"], facilities)
    elif asset in ("facility.earth-receiver", "facility.rock-receiver"):
        values = ("GetMultiLayerReceiverSprite", "src/industry_cmd.cpp", "DrawTile_Industry", "GetMultiLayerIndustrySprite",
                  ["actual Industry cells/construction", "GetSpoilReceiver/stock/room", "existing interval/aggregate bounds", "foreign resolver/transparency"], facilities)
    elif asset == "facility.vent-collector":
        values = ("GetMultiLayerCollectorSprite", "src/industry_cmd.cpp", "DrawTile_Industry", "GetMultiLayerIndustrySprite",
                  ["actual consumed site and native cells", "GetIndustryConstructionStage/IsIndustryCompleted", "IsGeothermalCollectorOperational",
                   "current shared Heat producer/interval; no graphics-owned energy"],
                  [evidence().log("collector-checks"), evidence().log("collector-capture")])
    elif asset.startswith("control."):
        values = ("GetMultiLayerControlSprite", "src/underground_gui.cpp", "UndergroundConstructionWindow::GetWidgetIcon", "GetMultiLayerControlSprite",
                  ["actual disabled/lowered widgets", "CheckDeepLayerAvailability/IsTBMCutterAvailable", "canonical area_quote/turn_envelope"], controls)
    elif asset in ("cargo.ERTH", "cargo.ROCK"):
        values = ("GetMultiLayerCargoSprite", "src/cargotype.cpp", "CargoSpec::GetCargoIcon", "GetMultiLayerCargoSprite",
                  ["actual CargoSpec label and GRFFile owner", "immutable first-party definition package"], controls)
    else:
        raise ValueError("no implemented selected consumer: " + asset)
    selector, source, symbol, call, owners, log_paths = values
    retained = asset in ("terrain.underground", "terrain.deep", "terrain.edges.underground", "terrain.edges.deep")
    return {"retained_only": retained, "superseded_by": "terrain.hardness/terrain.hardness-edges" if retained else None,
            "selector": selector, "selector_source": "src/multilayer_sprite.cpp",
            "consumer": {"source": source, "symbol": symbol, "call": call}, "state_authorities": owners,
            "evidence": log_paths, "simulation_independent": True,
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


def collector_integration(native_path, normal_paths, check_paths, publish=False):
    """Append the proven collector binding while preserving delivered runtime families.

    @param native_path Explicit current native fourteen-frame observer report.
    @param normal_paths Explicit ordinary-game construction/completion reports from that observer's checkpoints.
    @param check_paths Focused successful selector/collector/Heat check logs from both configurations.
    @param publish Publish the verified public binding and generated runtime metadata when true.
    @return None; fail before publication on stale pixels, sources, saves, builds or ownership evidence.
    """
    # A collector-only publisher cannot promote the larger selected scope.
    # Refuse before resolving captures or deriving any output; current shared
    # gallery/publication belongs to84C/84D, even when old collector proof passes.
    if publish and SELECTED_RUNTIME_ADDITIONS.intersection(json.loads(pack.SCOPE.read_text(encoding="utf-8"))["runtime_asset_ids"]):
        raise ValueError("collector-only publication cannot promote selected additions; complete84C/84D through the shared runtime owner")
    native_path = pack.base.repository_path(str(native_path))
    normal_paths = [pack.base.repository_path(str(path)) for path in normal_paths]
    check_paths = [pack.base.repository_path(str(path)) for path in check_paths]
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    packed = json.loads(PACKED.read_text(encoding="utf-8"))
    expected, outputs = pack.derive()
    assert packed["families"] == expected["families"] and packed["sprites"] == expected["sprites"]
    for path, encoded in outputs.items():
        assert path.read_bytes() == encoded, "collector runtime source differs from packed inputs"
    payload = packed["packed_grf"]
    assert digest(ROOT / payload["path"]) == payload["sha256"], "collector runtime payload changed"
    loader = read_evidence(ROOT / payload["loader_log"])
    marker = pack.loader_marker(packed)
    assert marker in loader and "All tests passed" in loader, "collector pixels were not actually loaded"
    native = json.loads(native_path.read_text(encoding="utf-8"))
    normals = [json.loads(path.read_text(encoding="utf-8")) for path in normal_paths]
    assert len(native["frames"]) == 14 and native["simulation_owners_unchanged_during_render"]
    native_log = read_evidence(ROOT / native["log"])
    assert "All tests passed" in native_log and "COLLECTOR_ART_RUNTIME frames=14 construction_stages=4 completed=true current_heat=true" in native_log
    saves = {row["path"]: row["sha256"] for row in native["saves"]}
    # Ordinary captures must load this observer's actual checkpoints, including
    # unfinished and completed machinery. A similar-looking external scene or
    # source contact sheet cannot establish native collector display.
    assert any("construction-" in row["source"] for row in normals)
    assert any("collector-ready.sav" in row["source"] for row in normals)
    for report in [native, *normals]:
        assert report["fixture_floor"] and report["fixture_material_delivery"] and not report["transported_bootstrap"]
        assert digest(ROOT / report["source"]) == report["source_sha256"], "collector capture source changed"
        executable = report.get("test_executable", report.get("game_executable"))
        assert digest(ROOT / executable["path"]) == executable["sha256"], "collector executable changed after capture"
        for frame in report["frames"]:
            path = pack.base.repository_path(frame["path"])
            assert path.is_relative_to(ROOT / "mlm/reviews/geothermal/collector-art"), "collector frame has another owner"
            assert digest(path) == frame["sha256"], "collector native pixels changed"
    for report in normals:
        assert saves.get(report["source"]) == report["source_sha256"], "ordinary scene is not this native checkpoint"
        assert report["native_owner_chunks_exact"], "ordinary capture changed native construction/stock/Heat owners"
        assert report["native_hardware"]["pixels_exact"], "ordinary frame lacks exact native machinery pixels"
        hardware = report["native_hardware"]["native_frame"]
        assert digest(ROOT / hardware["path"]) == hardware["sha256"], "native machinery comparison frame changed"
        assert digest(ROOT / report["resave"]["path"]) == report["resave"]["sha256"]
    assert {"focused-Debug.log", "focused-RelWithDebInfo.log"}.issubset({path.name for path in check_paths})
    for path in check_paths:
        assert "All tests passed" in read_evidence(path), "collector focused checks failed"
    selector = (ROOT / "src/multilayer_sprite.cpp").read_text(encoding="utf-8")
    consumer = (ROOT / "src/industry_cmd.cpp").read_text(encoding="utf-8")
    assert "GetMultiLayerCollectorSprite(" in selector and "GetMultiLayerIndustrySprite(" in consumer
    runtime = json.loads(RUNTIME.read_text(encoding="utf-8"))
    ranges = {row["asset_id"]: row for row in packed["families"]}
    # The existing delivered prefix keeps its original per-family evidence and
    # source geometry. The new loader proves the complete appended payload;
    # collector evidence adds one consumer rather than reclassifying old art.
    for row in runtime["bindings"]:
        assert row["native_offset"] == ranges[row["asset_id"]]["native_offset"]
        assert row["sprite_count"] == ranges[row["asset_id"]]["sprite_count"]
    asset = "facility.vent-collector"
    family = next(row for row in public["assets"] if row["asset_id"] == asset)
    assert family["packed"]["sha256"] == payload["sha256"]
    binding = {"asset_id": asset, "native_offset": ranges[asset]["native_offset"], "sprite_count": ranges[asset]["sprite_count"],
               "retained_only": False, "selector": "GetMultiLayerCollectorSprite", "selector_source": "src/multilayer_sprite.cpp",
               "consumer": {"source": "src/industry_cmd.cpp", "symbol": "DrawTile_Industry", "call": "GetMultiLayerIndustrySprite"},
               "state_authorities": ["GetGeothermalCollectorState", "GetGeothermalSiteClaim.site.access", "GetIndustryConstructionStage/IsIndustryCompleted",
                                     "IsGeothermalCollectorOperational", "GetUtilityPowerProducer/GetUtilityPowerInterval"],
               "evidence": [path.relative_to(ROOT).as_posix() for path in [*check_paths, native_path, *normal_paths]],
               "simulation_independent": True, "gallery_status": "scoped_native_and_ordinary_display_verified", "user_review": "deferred_until_integration"}
    runtime["bindings"] = [row for row in runtime["bindings"] if row["asset_id"] != asset] + [binding]
    delivered = runtime_ranges(packed)
    assert {row["asset_id"] for row in delivered} == {row["asset_id"] for row in runtime["bindings"]}, "runtime family coverage differs from delivered selection"
    runtime.update(packed_payload=payload, selected_families=len(runtime["bindings"]),
                   selected_sprites=sum(row["sprite_count"] for row in delivered))
    runtime["_documentation"] = "Owner: r2mlvs13-graphics-and-asset-pipeline.md83B and r2mlvs14-remaining-assets-and-runtime-integration.md79C; retained M1 bindings plus scoped collector native/ordinary display; user feedback pending"
    # Receiver state remains with its existing selector. The shared industry
    # dispatch now selects that producer and the collector through one draw call.
    receiver_ids = {"facility.earth-receiver", "facility.rock-receiver"}
    for row in runtime["bindings"]:
        if row["asset_id"] in receiver_ids:
            row["consumer"]["call"] = "GetMultiLayerIndustrySprite"
    family["authoring"].update(runtime_activated=True, status="source_coverage_verified_runtime_bound")
    family["runtime"] = {"status": "bound_verified", "consumer": binding["consumer"], "binding_manifest": RUNTIME.relative_to(ROOT).as_posix(),
                         "scoped_evidence": binding["evidence"], "user_review": "deferred_until_integration"}
    for row in public["assets"]:
        if row["asset_id"] in receiver_ids:
            row["runtime"]["consumer"]["call"] = "GetMultiLayerIndustrySprite"
    packed.update(runtime_activated=len(delivered) == len(packed["families"]),
                  runtime_asset_ids=[row["asset_id"] for row in delivered],
                  runtime_binding_manifest=RUNTIME.relative_to(ROOT).as_posix())
    if publish:
        RUNTIME.write_text(json.dumps(runtime, indent=2) + "\n", encoding="utf-8")
        PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
        PACKED.write_text(json.dumps(packed, indent=2) + "\n", encoding="utf-8")
    print(f"Collector runtime binding: {len(runtime['bindings'])} preserved/appended families, exact loaded pixels, fourteen native frames and {len(normals)} ordinary scenes; transported bootstrap and user feedback pending")


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
    assert identity["loader_log"] == evidence().log("packed-loader"), "loader log differs from selected proof"
    loaded = read_evidence(ROOT / identity["loader_log"])
    marker = pack.loader_marker(expected)
    assert marker in loaded, "actual loader coverage differs from selected runtime families"
    assert "All tests passed" in loaded
    # Native PNG and immutable save identities tie the real rendered controls
    # to their actual loaded world. A fabricated contact sheet cannot pass here.
    for raster in rasters:
        evidence().validate_capture(raster)
        assert raster["simulation_owners_unchanged_during_render"]
        assert digest(ROOT / raster["source"]) == raster["source_sha256"], "runtime source save is stale"
        log = read_evidence(ROOT / raster["log"])
        assert "All tests passed" in log
        for frame in raster["frames"]:
            assert digest(ROOT / frame["path"]) == frame["sha256"], "runtime raster hash is stale"
    assert "M1_CONTROL_RUNTIME frames=6 actual_widgets=true engines=5 cargo_icons=2" in read_evidence(ROOT / rasters[1]["log"])
    families = {family["asset_id"]: family for family in public["assets"]}
    result = []
    # Bind only the explicit delivered review subset. The complete payload's
    # other source families retain their later consumer integration gates.
    delivered = runtime_ranges(packed)
    for row in delivered:
        asset = row["asset_id"]
        family = families[asset]
        assert family["packed"]["native_offset"] == row["native_offset"], "public runtime range is stale"
        assert family["packed"]["sprite_count"] == row["sprite_count"], "public runtime count is stale"
        assert family["packed"]["sha256"] == identity["sha256"], "public runtime payload is stale"
        binding = binding_for(asset)
        if asset in SELECTED_RUNTIME_ADDITIONS:
            binding["native_source_proof"] = validate_native_binding(asset, binding)  # New families need their actual function chain; publication cannot borrow a marker/comment elsewhere in a file.
        selector = (ROOT / binding["selector_source"]).read_text(encoding="utf-8")
        consumer = (ROOT / binding["consumer"]["source"]).read_text(encoding="utf-8")
        assert binding["selector"] + "(" in selector, "declared selector is absent"
        assert binding["consumer"]["call"] + "(" in consumer, "declared native consumer is absent"
        for log_path in binding["evidence"]:
            assert "All tests passed" in read_evidence(ROOT / log_path), "consumer evidence failed: " + log_path
        result.append({"asset_id": asset, "native_offset": row["native_offset"], "sprite_count": row["sprite_count"], **binding})
    assert len(result) == len(delivered) and sum(row["sprite_count"] for row in result) == sum(row["sprite_count"] for row in delivered), "delivered runtime coverage is incomplete"
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
    import review_m1_art as gallery_owner
    # Running this file as a CLI gives it the __main__ module identity. The
    # imported gallery still uses activate_m1_art by name; explicitly select
    # the same evidence on that owner so it cannot fall back to the old default.
    gallery_owner.activation.select_evidence(evidence().path)
    gallery_owner.configure(evidence())
    from review_m1_art import GALLERY, REPORTS, read_json, validate as validate_gallery
    if not GALLERY.exists():
        return None
    try:
        record = read_json(GALLERY)
        candidate = {"packed_payload": packed["packed_grf"], "bindings": bindings}
        selected = "selected_gallery_boundary" in evidence().document
        reports = None if selected else [evidence().report(name) for name in REPORTS]
        rows, frames = validate_gallery(public, packed, candidate, reports)
        assert record["technical_acceptance"] == "verified"
        assert record["packed_payload"] == packed["packed_grf"]
        assert record["families"] == rows and record["frames"] == frames
        for artifact in (record["contact_sheet"], record["html"]):
            assert digest(ROOT / artifact["path"]) == artifact["sha256"]
        if selected:
            # The selected receipt adapter and gallery owner are part of the
            # acceptance boundary. A changed validator must regenerate its
            # named gallery rather than borrowing an earlier success flag.
            for role in ("gallery_owner", "selected_adapter"):
                assert evidence_identity(ROOT / record[role]["path"]) == record[role]
            assert record["simulation_owners_clock_rng_unchanged"] and record["active_consumers"] == 65
        return GALLERY.relative_to(ROOT).as_posix()
    except (AssertionError, OSError, KeyError, ValueError):
        return None


def require_selected_gallery(bindings, gallery):
    """Keep reviewed callback additions unpublished until current native gallery acceptance.

    @param bindings Validated runtime rows from the existing explicit selection owner.
    @param gallery Current gallery path returned by verified_gallery, or None when missing/stale.
    @return None; refusal precedes generated runtime or public metadata writes.
    """
    # Implementation review grants callback authority only. Both generated
    # and public runtime activation of new families require their actual
    # selected gallery rather than borrowing the retained43-family review.
    if any(row["asset_id"] in SELECTED_RUNTIME_ADDITIONS for row in bindings) and gallery is None:
        raise AssertionError("selected runtime publication requires the current84C native gallery before84D")


def publish_selected_runtime(public, packed, bindings, publish=False):
    """Publish the selected delivery only after exact current gallery revalidation.

    @param public Existing public family contract; protected fields remain unchanged.
    @param packed Complete ninety-family payload report; bytes/ranges remain immutable.
    @param bindings Canonical sixty-nine-family review from the existing activation owner.
    @param publish Whether to update public metadata and the default EvidenceSet; false writes candidate runtime/packing metadata only.
    @return Runtime path; human feedback, future consumers and whole-profile activation stay pending.
    """
    from tools import selected_gallery
    gallery = verified_gallery(public, packed, bindings)
    require_selected_gallery(bindings, gallery)
    selected = {row["asset_id"]: row for row in bindings}
    candidate = copy.deepcopy(public)
    for family in candidate["assets"]:  # Explicit delivered scope alone controls activation; unsupported ground/factory/Space families retain every prior field.
        asset = family["asset_id"]
        if asset not in selected:
            continue
        binding = selected[asset]
        active = not binding["retained_only"]
        if "authoring" in family:
            family["authoring"].update(runtime_activated=active, status="source_coverage_verified_runtime_bound" if active else "retained_source_and_packed_range")
        if "runtime_activated" in family["source"]:
            family["source"]["runtime_activated"] = active
        family["runtime"] = {"status": "bound_verified" if active else "superseded_retained", "binding_manifest": RUNTIME.relative_to(ROOT).as_posix(),
                             "gallery": {"manifest": gallery, "technical_acceptance": "verified", "user_feedback": "deferred"}, "user_review": "deferred"}
        if active:
            family["runtime"]["consumer"] = binding["consumer"]
        else:
            family["runtime"]["superseded_by"] = binding["superseded_by"]
    delivered_packed = copy.deepcopy(packed)
    delivered_packed.update(runtime_activated=False, runtime_asset_ids=list(selected), runtime_binding_manifest=RUNTIME.relative_to(ROOT).as_posix(), gallery_manifest=gallery,
                            _documentation="r2mlvs14-remaining-assets-and-runtime-integration.md84D;90packed families,69review/65active consumers,selected native technical gallery;human feedback/future producers pending")
    selected_gallery.validate_boundary(candidate, delivered_packed, evidence(), {})
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84D;selected actual native consumers/technical gallery;retained dated43-family prefix;human feedback deferred",
              "schema_version": 2, "packed_payload": packed["packed_grf"], "selected_families": len(bindings), "selected_sprites": sum(row["sprite_count"] for row in bindings),
              "active_consumers": sum(not row["retained_only"] for row in bindings), "bindings": copy.deepcopy(bindings), "gallery_manifest": gallery,
              "evidence_manifest": evidence().path.relative_to(ROOT).as_posix(), "selected_native_closures": {role: evidence().document["reports"][role] for role in selected_gallery.ROLES},
              "user_review": "deferred", "whole_profile_activated": False}
    for row in report["bindings"]:  # Every selected row carries technical review without upgrading the user's separate personal acceptance gate.
        row.update(gallery_status="technical_review_verified_user_feedback_deferred", user_review="deferred")
    RUNTIME.parent.mkdir(parents=True, exist_ok=True)
    # Validate every candidate before the first delivery write. Originals and
    # the previous default selection remain retained under exact named paths.
    previous_default = evidence().path.parent / "evidence-before-delivery.json"
    if publish:
        if not previous_default.exists():
            previous_default.write_bytes(DEFAULT_MANIFEST.read_bytes())
        PUBLIC.write_text(json.dumps(candidate, indent=2) + "\n", encoding="utf-8")
    RUNTIME.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    PACKED.write_text(json.dumps(delivered_packed, indent=2) + "\n", encoding="utf-8")
    if publish:
        DEFAULT_MANIFEST.write_bytes(evidence().path.read_bytes())
    return RUNTIME


def main():
    """Validate retained native bindings, exercise bounded corruption refusals, and optionally publish.

    @return None; write only verified selected metadata when --publish is explicitly requested.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--publish", action="store_true")
    parser.add_argument("--cache-only", action="store_true", help="publish generated metadata while preserving the public source contract")
    parser.add_argument("--evidence-manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--collector-native", type=Path, help="explicit scoped native collector report")
    parser.add_argument("--collector-normal", type=Path, action="append", default=[], help="explicit ordinary collector report; repeat per scene")
    parser.add_argument("--collector-checks", type=Path, action="append", default=[], help="focused collector check log; supply both configurations")
    args = parser.parse_args()
    if args.collector_native is not None:
        collector_integration(args.collector_native, args.collector_normal, args.collector_checks, args.publish)
        return
    select_evidence(args.evidence_manifest)
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    packed = json.loads(PACKED.read_text(encoding="utf-8"))
    if "selected_gallery_boundary" in evidence().document:
        bindings = validate_binding_review(public, packed)
        gallery = verified_gallery(public, packed, bindings)
        require_selected_gallery(bindings, gallery)
        # Both cached and public delivery create reviewed runtime metadata;
        # neither mode can bypass actual selected native gallery acceptance.
        if args.publish or args.cache_only:
            publish_selected_runtime(public, packed, bindings, args.publish)
        print(json.dumps({"families": len(bindings), "sprites": sum(row["sprite_count"] for row in bindings), "gallery": gallery, "public_promoted": args.publish, "user_feedback": "deferred"}))
        return
    rasters = [evidence().report(name) for name in ("geology", "control")]
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
    if args.publish or args.cache_only:
        gallery = verified_gallery(public, packed, bindings)
        require_selected_gallery(bindings, gallery)
        if gallery is not None:
            for binding in bindings:
                binding["gallery_status"] = "technical_review_verified_user_feedback_deferred"
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Group83B; selected actual native consumers; final gallery/user review pending; sound proof in sound-runtime.json",
                  "schema_version": 1, "packed_payload": packed["packed_grf"], "selected_families": len(bindings),
                  "selected_sprites": sum(row["sprite_count"] for row in bindings), "bindings": bindings, "rasters": [evidence().selected("reports", name).relative_to(ROOT).as_posix() for name in ("geology", "control")],
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
        if not args.cache_only:
            PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
        # Technical review covers the delivered subset, not all newly packed
        # families. Whole-payload activation stays false until every real
        # consumer has its own native evidence and staged integration.
        packed["runtime_activated"] = len(bindings) == len(packed["families"])
        packed["runtime_asset_ids"] = [row["asset_id"] for row in bindings]
        packed["runtime_binding_manifest"] = RUNTIME.relative_to(ROOT).as_posix()
        packed["_documentation"] = "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/83B; selected native bytes and actual runtime bindings; final gallery/user review pending; sound proof in sound-runtime.json"
        if gallery is not None:
            packed["gallery_manifest"] = gallery
            packed["_documentation"] = "Owner: r2mlvs13-graphics-and-asset-pipeline.md79B/80B/83B/85B; selected native packing, consumer and technical gallery verified; user feedback deferred"
        else:
            packed.pop("gallery_manifest", None)
        PACKED.write_text(json.dumps(packed, indent=2) + "\n", encoding="utf-8")
    print(f"Selected M1 runtime bindings: {len(bindings)} families / {sum(row['sprite_count'] for row in bindings)} sprites of {len(packed['families'])} packed families; actual consumer/range/payload/raster evidence and three corruption refusals pass; final gallery/user review pending; sound proof retained")


if __name__ == "__main__":
    main()
