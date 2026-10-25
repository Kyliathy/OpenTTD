"""Verify actual conventional native callback chains without historical log dependencies.

@file merge_renderer_binding_checks.py
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Existing dispatch/public owners supply family identities. This read-only test
never publishes bindings, art or approval and never reads old producer receipts.
"""
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "media/multilayer"), str(ROOT / "src/tests/multilayer/support")]
from activate_m1_art import cpp_function, cpp_code
from artifact_paths import contained


def verify_imported(row, original, root=ROOT):
    """Check admitted source dimensions and the actual imported callback chain.

    @param row Current family in the existing dispatch owner; no parallel catalogue.
    @param original Independent original-derived family facts frozen before integration.
    @param root Explicit checkout containing the current production callback bodies.
    @return Current native body identities and complete original pose coverage.
    """
    chain = row["native_chain"]
    result = []
    marker_found = False
    # Each real function must reach the next owner; comments and calls in an
    # unrelated body cannot qualify a disconnected renderer or preview fallback.
    for index, edge in enumerate(chain):
        code, identity = body(edge["source"], edge["symbol"], root)
        source = cpp_code((root / edge["source"]).read_text(encoding="utf-8"))
        marker_found |= row["asset_marker"] in source
        if index + 1 < len(chain):
            target = chain[index + 1]["symbol"].rsplit("::", 1)[-1]
            assert re.search(r"\b" + re.escape(target) + r"\s*\(", code), (row["asset_id"], edge, target)
        result.append(identity)
    assert marker_found, row["asset_id"]
    assert row["dispatch"] == "current_bound"
    assert row["native_offset"] == original["A_binding"]["native_offset"]
    assert row["sprite_count"] == original["A_binding"]["sprite_count"]
    count = len(row["states"]) * row["frames"] * len(row["directions"]) * row["tile_variants"] * len(row["parts"])
    assert count == row["sprite_count"], row["asset_id"]
    return {"asset_id": row["asset_id"], "actual_chain": result, "complete_source_pose_count": count}


def body(path, symbol, root=ROOT):
    """Read a real native definition through the existing token/body verifier.

    @param path Repository-relative implementation, never a report or cached source.
    @param symbol Exact native function definition required by the callback chain.
    @param root Explicit checkout; retained copies and test implementations cannot qualify production drawing.
    @return Complete current code body and its content identity.
    """
    source = contained(root / path, root / "src")
    # A callback found in a fixture, test double or archived copy cannot prove
    # that the actual game renderer reaches the selected source family.
    if source.is_relative_to((root / "src/tests").resolve()) or source.suffix not in (".cpp", ".h"):
        raise ValueError("renderer callback must belong to production native source")
    code = cpp_function(source.read_text(encoding="utf-8"), symbol)
    return code, {"path": path, "symbol": symbol, "body_sha256": hashlib.sha256(code.encode()).hexdigest()}


def verify(row, root=ROOT):
    """Follow one existing dispatch row to its sole typed Ground source selector.

    @param row Existing conventional family dispatch; no new runtime ID is allocated.
    @param root Explicit checkout containing the actual current drawing and binding owners.
    @return Actual code identities of every connected callback/selector edge.
    """
    binding = row["binding_symbol"]
    chain = [(row["draw_owner"], row["draw_symbol"])]
    selector = row.get("selector")
    if selector:
        chain.append(("src/multilayer_sprite.cpp", selector))
    # Purchase dispatch uses the same native engine admission and generic
    # purchase bridge before choosing its existing Ground body family.
    if selector == "GetMultiLayerEnginePurchaseSprite":
        chain.append(("src/multilayer_sprite.cpp", "GetMultiLayerPurchaseSprite"))
    chain.append((row["binding_header"].replace(".h", ".cpp"), binding))
    if binding == "GetGroundStationTileLayout":
        chain.append(("src/ground_station_sprite.cpp", "GetGroundStationArtLayout"))
    chain.append(("src/ground_sprite.h", "GetGroundSprite"))
    result = []
    # Each named real body must call the next actual owner. A matching call in
    # another function, a comment, quoted source or old receipt cannot prove it.
    for index, (path, symbol) in enumerate(chain):
        code, identity = body(path, symbol, root)
        if index + 1 < len(chain):
            target = chain[index + 1][1]
            assert re.search(r"\b" + re.escape(target) + r"\s*\(", code), (row["asset_id"], symbol, target)
        result.append(identity)
    return {"asset_id": row["asset_id"], "actual_chain": result}


def main():
    """Verify all independent conventional bindings and reject comment/literal impostors."""
    dispatch = json.loads((ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json").read_text())
    frozen = json.loads((ROOT / "src/tests/fixtures/graphics/merge-asset-families.json").read_text())
    expected = {row["asset_id"]: row["destination_draw_contract"] for row in frozen["families"]
                if row["destination_draw_contract"].get("binding_header", "").startswith("src/ground_")}
    rows = [row for row in dispatch["families"] if row["asset_id"] in expected]
    assert {row["asset_id"] for row in rows} == set(expected)
    # Keep the original draw/binding ownership independent of current bodies;
    # rewritten dispatch metadata cannot self-certify a newly invented consumer.
    for row in rows:
        for key, value in expected[row["asset_id"]].items():
            assert row.get(key) == value, (row["asset_id"], key)
    results = [verify(row) for row in rows]
    originals = {row["asset_id"]: row for row in frozen["families"]}
    imported = [verify_imported(row, originals[row["asset_id"]]) for row in dispatch["families"] if "native_chain" in row]
    assert len(imported) == 26
    assert not any(row["dispatch"] == "later_owner" for row in dispatch["families"])
    # The native Action5 admission and NewGRF base must use the complete A
    # bound. Expanded donor M1 counts must never move the shipped Space preview.
    sprites = cpp_code((ROOT / "src/table/sprites.h").read_text())
    assert "MAP_LAYER_SPRITE_COUNT = GROUND_SPRITE_END" in sprites
    assert "SPR_SPACE_PREVIEW_ASTEROID = SPR_MAP_LAYER_BASE + MULTILAYER_M1_SPRITE_COUNT" in sprites
    assert "SPR_SPACE_PREVIEW_PROBE = SPR_SPACE_PREVIEW_ASTEROID + 9" in sprites
    assert "SPR_NEWGRFS_BASE = SPR_MAP_LAYER_BASE + MAP_LAYER_SPRITE_COUNT" in sprites
    admission = cpp_code((ROOT / "src/newgrf/newgrf_act5.cpp").read_text())
    assert re.search(r"SPR_MAP_LAYER_BASE,\s*1,\s*MAP_LAYER_SPRITE_COUNT", admission)
    fake = 'void Draw() { /* Selector(); */ const char *text = "Selector(); }"; }'
    assert not re.search(r"\bSelector\s*\(", cpp_function(fake, "Draw"))
    try:
        cpp_function("void Draw(); // { Selector(); }", "Draw")
    except AssertionError:
        pass
    else:
        raise AssertionError("a declaration/comment masqueraded as a native renderer")
    output = ROOT / "test-cache/merge-assets/group50-renderer-bindings.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps({"_documentation": __doc__, "complete": True, "families": results,
                                  "imported_families": imported, "native_admission_and_preview_bounds": True,
                                  "comment_literal_declaration_controls": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {len(results)} conventional and {len(imported)} imported families through actual native callback bodies.")


if __name__ == "__main__":
    main()
