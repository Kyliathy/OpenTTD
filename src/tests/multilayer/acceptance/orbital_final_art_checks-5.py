"""Bind reviewed ordinary orbital presentation without replaying dated producer graphs.

@file orbital_final_art_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L.1
Direct native processes, exact source parts, original owners and explicit visual
review qualify this presentation boundary. Later refusal/save/network gates remain separate.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity

CAPTURE = runpy.run_path(str(Path(__file__).with_name("orbital_final_art_checks-1.py")))
FIELD_CASES = ("ice", "steel-live", "hauler-live", "heavy-live", "assembly-live", "dock-live", "heavy-work", "factory-work",
               "power-blueprint", "power-partial", "power-active", "power-blocked", "fuel-blueprint", "fuel-partial", "fuel-frame")
INSPECTORS = ("steel-outage-2", "power-partial-1", "fuel-frame-2", "factory-work-1", "goods-input-2", "ice-history-1")
PAIRED_FIELDS = FIELD_CASES[8:]


def read(path):
    """Read one explicitly selected repository-owned evidence document.

    @param path Exact receipt or manifest path. @return Parsed JSON without recursive historical graph traversal.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def genuine_frame(matches, asset, state, frame):
    """Require a visible exact part unique to an actual working or moving source phase.

    @param matches Exact substantial parts from one ordinary capture qualification.
    @param asset Actual canonical source family. @param state Required original state.
    @param frame Required genuine authored phase, excluding ambiguous static aliases.
    @return Capture indices with at least one unambiguous required native part.
    """
    # Shared floors and identical still aliases cannot certify earned animation.
    return [index for index, capture in enumerate(matches["captures"])
            if any(row["asset_id"] == asset and {(alias["state"], alias["frame"]) for alias in row["logical_aliases"]} == {(state, frame)}
                   for row in capture["exact_part_matches"])]


def qualify(short, phase, inspector, approved):
    """Qualify one exact native capture and its direct paid predecessor boundary.

    @param short Release/debug membership prefix. @param phase Explicit approved case name.
    @param inspector Require post-GUI owner purity instead of field part matching.
    @param approved Accepted current and dated application identities by configuration.
    @return Bound current capture, process, physical comparisons and exact part evidence.
    """
    selector = ROOT / "logs" / ("orbital-art-ordinary-" + short + "-" + ("inspector-" if inspector else "") + phase + ".json")
    reference = read(selector)["report"]
    path = ROOT / reference["path"]
    assert identity(path) == reference
    record = read(path)
    # A second membership filename cannot substitute the other configuration's
    # favorable process; each actual game must independently supply its view.
    expected_configuration = {"release": "RelWithDebInfo", "debug": "Debug"}[short]
    assert record["configuration"] == expected_configuration
    assert record["complete"] and not any(record[field] for field in ("added_stock", "granted_work", "granted_pose"))
    assert (record["dated_producer"], record["source"]) == CAPTURE["predecessor"](record["configuration"], record["owner"], record["checkpoint"])
    comparison = CAPTURE["compare_ordinary_owners"](record["source"], record["after"], record["configuration"])
    native_path = ROOT / record["native_process"]["path"]
    assert identity(native_path) == record["native_process"]
    native = read(native_path)
    assert native["configuration"] == expected_configuration
    executable = ROOT / native["executable"]["path"]
    assert identity(executable) == native["executable"]
    assert native["original_application"] in approved[record["configuration"]]
    # A dated original build path may now contain the corrected application.
    # Reconstruct its exact GUI PE identity from this process's retained console
    # projection, rather than demanding a historical executable at today's path.
    encoded = bytearray(executable.read_bytes())
    header = struct.unpack_from("<I", encoded, 0x3C)[0]
    assert encoded[header:header + 4] == b"PE\0\0"
    encoded[header + 92:header + 94] = (2).to_bytes(2, "little")
    assert hashlib.sha256(encoded).hexdigest() == native["original_application"]["sha256"]
    for field in ("startup", "terminal"):
        assert identity(ROOT / native[field]["path"]) == native[field]
    for capture in record["captures"]:
        assert identity(ROOT / capture["path"])["sha256"] == capture["sha256"]
    result = dict(selector=identity(selector), report=reference, **record, verified_owner_comparison=comparison)
    if inspector:
        assert record["view"] == "inspector" and record["inspected_after"]
        result["verified_inspected_comparison"] = CAPTURE["compare_ordinary_owners"](record["after"], record["inspected_after"], record["configuration"])
    else:
        matches_path = path.with_name("ordinary-orbital-pixel-matches.json")
        matches = read(matches_path)
        assert matches["ordinary_capture"] == reference
        assert identity(ROOT / matches["manifest"]["path"]) == matches["manifest"]
        assert [row["source"] for row in matches["captures"]] == record["captures"]
        result["part_qualification"] = identity(matches_path)
        result["visible_assets"] = sorted({row["asset_id"] for capture in matches["captures"] for row in capture["exact_part_matches"]})
        if phase in ("steel-live-1", "hauler-live-1", "heavy-work-1", "factory-work-1"):
            # Only actual earned live phases close animation review; idle
            # freight Assembly hardware and initial heavy pursuit stay distinct.
            asset, state, frame = {"steel-live-1": ("module.processor", "complete", 1),
                                   "hauler-live-1": ("vehicle.model-17", "moving-partial", 1),
                                   "heavy-work-1": ("vehicle.model-21", "mining", 1),
                                   "factory-work-1": ("module.workshop", "complete", 1)}[phase]
            result["genuine_activity_captures"] = genuine_frame(matches, asset, state, frame)
            assert result["genuine_activity_captures"]
        if phase == "dock-live-1":
            composition_path = path.with_name("ordinary-dock-composition.json")
            composition = read(composition_path)
            assert composition["ordinary_capture"] == reference and composition["matches"] == identity(matches_path)
            assert all(row["foreground_pixels"] == 18 for row in composition["captures"])
            assert all(item["unexplained_pixels"] == 0 for row in composition["captures"] for item in row["explained_compositions"])
            result["registered_composition"] = identity(composition_path)
    return result


def bind(review_path, output, publish):
    """Bind both ordinary configurations and optionally publish only their runtime metadata.

    @param review_path Explicit completed visual inspection/equivalence receipt.
    @param output New final ordinary verification file inside the repository logs.
    @param publish Update only the ten accepted families' authoring/runtime publication fields.
    @return Final evidence identity; no gameplay/source pixels, packed IDs or old proof is modified.
    """
    review = read(review_path)
    assert review["complete"]
    packed_path = ROOT / "logs/orbital-art-final-verification.json"
    binary_path = ROOT / "logs/orbital-art-ordinary-binary-verification.json"
    packed, binary = read(packed_path), read(binary_path)
    assert binary["complete"] and identity(ROOT / binary["grf"]["path"]) == binary["grf"]
    approved = {cfg: [packed["builds"][cfg]["application"], binary["builds"][cfg]["application"]] for cfg in ("RelWithDebInfo", "Debug")}
    fields, inspectors = {}, {}
    # Each independently built ordinary game must supply every exact requested
    # field and inspector; no favorable other-world/build snapshot fills a gap.
    for short in ("release", "debug"):
        fields[short] = {phase: qualify(short, phase + "-1", False, approved) for phase in FIELD_CASES}
        inspectors[short] = {phase: qualify(short, phase, True, approved) for phase in INSPECTORS}
    selected = [row for group in (fields, inspectors) for cases in group.values() for row in cases.values()]
    assert {row["selector"]["path"]: row["selector"] for row in selected} == {row["path"]: row for row in review["reviewed_selectors"]}
    pairs = []
    # Entire original indexed and RGB equality transfers an already inspected
    # view to its independent counterpart without repeating image editing/review.
    for group, phases in ((fields, PAIRED_FIELDS), (inspectors, INSPECTORS)):
        for phase in phases:
            left, right = group["release"][phase], group["debug"][phase]
            assert len(left["captures"]) == len(right["captures"]) == 1
            assert left["captures"][0]["indexed_sha256"] == right["captures"][0]["indexed_sha256"]
            assert left["captures"][0]["rgb_sha256"] == right["captures"][0]["rgb_sha256"]
            pairs.append(dict(case=phase, release=left["report"], debug=right["report"]))
    manifest_path = ROOT / "logs/proofs/mlm/native-art/space/orbital-packed-manifest.json"
    native = read(manifest_path)
    selected_assets = {row["asset_id"] for row in native["families"]}
    assert len(selected_assets) == 10 and len(native["sprites"]) == 976
    assert selected_assets <= {asset for cases in fields.values() for row in cases.values() for asset in row["visible_assets"]}
    public_path = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
    public = read(public_path)
    protected = [copy.deepcopy(row) for row in public["assets"] if row["asset_id"] not in selected_assets]
    if publish:
        # Runtime publication changes no original MLVI identity, paid behavior,
        # source provenance or protected family; only reviewed consumers bind.
        for row in public["assets"]:
            if row["asset_id"] not in selected_assets:
                continue
            row["authoring"].update(status="source_packed_runtime_verified", runtime_activated=True)
            vehicle = row["asset_id"].startswith("vehicle.")
            row["runtime"] = dict(status="bound_verified", consumer=dict(source="src/space_vehicle.cpp" if vehicle else "src/asteroid_gui.cpp",
                                   symbol="SpaceVehicle::GetImage" if vehicle else "ViewportAddSpaceEntities"), binding_manifest=output.relative_to(ROOT).as_posix())
        assert protected == [row for row in public["assets"] if row["asset_id"] not in selected_assets]
        public_path.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    proof = dict(_documentation=__doc__, complete=True, visual_review=identity(review_path), packed_acceptance=identity(packed_path),
                 binary_acceptance=identity(binary_path), native_manifest=identity(manifest_path), public_manifest=identity(public_path),
                 fields=fields, inspectors=inspectors, exact_counterpart_pairs=pairs, processes=len(selected),
                 screenshots=sum(len(row["captures"]) for row in selected), protected_public_rows=len(protected), published=publish,
                 unsupported_native_behaviour={"vehicle.model-16": ["construction", "service"], "vehicle.model-17": ["service"],
                                               "module.fuel-store": ["activation", "inventory", "power", "service"]},
                 scope="Original13.1 presentation only; original13.2 and14-16 remain separate open acceptance gates.",
                 sources=[identity(Path(__file__)), identity(Path(__file__).with_name("orbital_final_art_checks-1.py")),
                          identity(Path(__file__).with_name("orbital_final_art_checks-2.py")), identity(Path(__file__).with_name("orbital_final_art_checks-4.py"))])
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    return identity(output)


def main():
    """Bind an explicit visual receipt into a new repository-owned final verification."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reviewed", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    print(bind(ROOT / args.reviewed, output, args.publish), flush=True)


if __name__ == "__main__":
    main()
