"""Append qualified launch-center and rocket originals through existing native packing owners.

@file pack_launch_fleet_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62I.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Source staging preserves all previous sheets/ranges and saved MLVI identities.
Actual GRF decoding, current native compatibility and game review are separate.
"""

import argparse
import copy
import hashlib
import json
import re
import sys

import PIL

import pack_m1_art as packing
from convert_terrain_art import png_bytes
from tools import cargo_rocket_art as art
from tools import launch_fleet_handoff as handoff

ROOT = handoff.ROOT
SCOPE = ROOT/"media/multilayer/launch-pack-scope.json"
PUBLIC = art.PUBLIC
BUILD_PROOF = ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-packing-v1/build-proof.json"


def read_scope():
    """Require exact qualified source owners, append extent and deterministic tools.

    @return Explicit two-family packing scope and checked immutable source handoff.
    @throws ValueError On source/proof, prior sheet/range or pinned toolchain drift.
    """
    scope = art.read_json(SCOPE)
    # The append cannot silently encode with different tools or reinterpret an
    # older reserved/core/Space/orbital slot as available launch-center space.
    if (scope["asset_ids"] != list(handoff.ASSETS) or scope["native_route"] != "native_baseset_launch"
            or scope["native_start"] != 6143 or scope["native_end_exclusive"] != 7791
            or scope["toolchain"]["python"] != sys.version.split()[0] or scope["toolchain"]["pillow"] != PIL.__version__
            or scope["sheet_width"] != 2048 or scope["sprite_gap"] != 2):
        raise ValueError("launch/fleet packing scope or pinned toolchain changed")
    accepted = art.read_json(handoff.bound(scope["source_handoff"]))
    handoff.checked_handoff(accepted)
    proof = art.read_json(handoff.bound(scope["source_qualification"]))
    # A current completed adapter qualification must precede any new packed
    # inputs. Source art flags and a different family's tests are insufficient.
    if (proof["status"] != "passed" or proof["cases"] != 7 or proof["source_handoff"] != scope["source_handoff"]
            or proof["packed"] is not False or proof["runtime"] is not False):
        raise ValueError("launch/fleet completed source adapter qualification is absent")
    for name in ("helper", "tests"):
        handoff.bound(proof[name])
    # Original source sheets, old range declarations and retained predecessor
    # bytes remain unchanged even after the shared current GRF is rebuilt.
    for reference in accepted["protected_outputs"]:
        handoff.bound(reference)
    handoff.bound(accepted["predecessor_payload"])
    return scope, accepted


def format_header(ranges, bound):
    """Reuse the shared native range generator for the separate launch/fleet append.

    @param ranges Exact two-family physical/source domains in stable append order.
    @param bound Exclusive7791native bound after all1648logical positions.
    @return Documented C++ declaration bytes without allocating any saved gameplay ID.
    """
    header = packing.format_header(ranges, bound)
    # One shared declaration owner retains field widths/order and family-name
    # spelling. Only this append's names/plan and explicit starting bound differ.
    substitutions = ((b"multilayer_sprite_assets.h", b"launch_fleet_sprite_assets.h"),
                     (b"MULTILAYER_SPRITE_ASSETS_H", b"LAUNCH_FLEET_SPRITE_ASSETS_H"),
                     (b"MultiLayerSpriteAsset", b"LaunchFleetSpriteAsset"),
                     (b"MultiLayerSpriteRange", b"LaunchFleetSpriteRange"),
                     (b"MULTILAYER_M1_SPRITE_COUNT", b"LAUNCH_FLEET_SPRITE_END"),
                     (b"_multilayer_m1_sprite_ranges", b"_launch_fleet_sprite_ranges"),
                     (b"Generated M1 native graphics ranges", b"Generated launch/fleet native graphics ranges"),
                     (b"r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/81B/79C", b"r2mlvs14-remaining-assets-and-runtime-integration.md Group62I"))
    for original, replacement in substitutions:
        header = header.replace(original, replacement)
    return header.replace(b"static constexpr uint16_t LAUNCH_FLEET_SPRITE_END",
                          b"static constexpr uint16_t LAUNCH_FLEET_SPRITE_START = 6143; ///< Every prior native sprite precedes this append.\n"
                          b"static constexpr uint16_t LAUNCH_FLEET_SPRITE_END")


def derive(public=None):
    """Prepare exact deterministic append bytes from qualified stored native source artwork.

    @param public Optional detached public contract for focused range/source refusal checks.
    @return Complete packing report and exact three owned PNG/NFO/header byte outputs.
    @throws ValueError On source qualification, original geometry or native range drift.
    """
    scope, accepted = read_scope()
    public = art.read_json(PUBLIC) if public is None else public
    selected = [row["asset_id"] for row in public["assets"] if row["asset_id"] in handoff.ASSETS]
    # A missing/duplicated selected public identity must fail before source
    # lookup rather than becoming a dictionary overwrite or a missing-key crash.
    if len(selected) != 2 or set(selected) != set(handoff.ASSETS):
        raise ValueError("launch/fleet selected public family is missing or duplicated")
    ranges, bound = handoff.layout(public, accepted)
    families = {row["asset_id"]: row for row in public["assets"]}
    sprites, entries = [], []
    # Every declared state/frame/view/cell/part retains its own native identity,
    # including direct aliases and explicitly approved empty back/front cuts.
    for native in ranges:
        asset = native["asset_id"]
        family = families[asset]
        poses, image = handoff.source_poses(family, handoff=accepted)
        for index, key in enumerate(packing.required_keys(family)):
            pose = poses[key]
            sprites.append(packing.cropped_pose(image, pose))
            entries.append(dict(asset_id=asset, native_offset=native["native_offset"]+index,
                                state=key[0], frame=key[1], direction=key[2], tile_variant=key[3], part=key[4],
                                source_atlas=family["authoring"]["native_atlas"], source_box=pose["box"], source_anchor=pose["anchor"],
                                source_native_sha256=pose["native_sha256"], structurally_absent=pose["structurally_absent"],
                                alias_of=pose["alias_of"]))
    sheet, positions = packing.shelf_sheet(sprites, scope["sheet_width"], scope["sprite_gap"])
    png = png_bytes(sheet)
    nfo = packing.format_nfo(entries, sprites, positions).replace(b"map_layers_m1", b"launch_final")
    nfo = nfo.replace(b"Generated selected M1 source sprites", b"Generated launch/fleet source sprites")
    nfo = nfo.replace(b"r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C", b"r2mlvs14-remaining-assets-and-runtime-integration.md Group62I")
    header = format_header(ranges, bound)
    # Both full-native source and packed crop geometry remain observable for an
    # independent real-loader comparison of pixel indices, holes and anchors.
    for entry, sprite, position in zip(entries, sprites, positions):
        indices, width, height, x, y = sprite
        entry.update(box=[*position, width, height], anchor=[x, y], pixel_sha256=hashlib.sha256(bytes(indices)).hexdigest())
    files = {ROOT/scope["png"]: png, ROOT/scope["nfo"]: nfo, ROOT/scope["header"]: header}
    report = dict(_documentation=__doc__, schema_version=1, route=scope["native_route"], source_handoff=scope["source_handoff"],
                  source_qualification=scope["source_qualification"], toolchain=scope["toolchain"],
                  native_start=scope["native_start"], native_sprite_bound=bound, pose_count=len(entries), families=ranges,
                  sprites=entries, outputs=[dict(path=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(data).hexdigest())
                                           for path, data in files.items()], packed_grf=None, runtime_activated=False)
    return report, files


def publish(report, files):
    """Stage only current focused-qualified own packing bytes and new native public links.

    @param report Exact current deterministically derived two-family packing report.
    @param files Its exact own PNG/NFO/header byte mapping, with no additional destinations.
    @return None; actual GRF decoding/checksum publication and game review remain open.
    @throws ValueError On stale source/refusal proof or substituted output paths/bytes.
    """
    scope, accepted = read_scope()
    # The stage writer owns exactly these three append artifacts. Source
    # qualification never authorizes an extra or redirected output destination.
    if set(files) != {ROOT/scope[name] for name in ("png", "nfo", "header")}:
        raise ValueError("launch/fleet staging contains an unowned or missing output path")
    verification = art.read_json(ROOT/scope["qualification"])
    # Public staging follows the actual focused packing/refusal/reproducibility
    # checks. It never publishes an unreviewed byte mapping supplied by a caller.
    if (verification["status"] != "passed" or verification["cases"] != 6
            or verification["source_handoff"] != scope["source_handoff"]
            or verification["scope"] != art.source.identity(SCOPE)
            or verification["packer"] != art.source.identity(ROOT/"media/multilayer/pack_launch_fleet_art.py")
            or verification["tests"] != art.source.identity(ROOT/"src/tests/multilayer/launch_fleet_pack_checks.py")):
        raise ValueError("launch/fleet current focused packing qualification is absent")
    actual_report, actual_files = derive()
    if report != actual_report or files != actual_files:
        raise ValueError("launch/fleet staging bytes differ from the qualified deterministic derivation")
    handoff.validate_prepacking_prefix(accepted)
    # Identical own generated headers are not touched, so repeated conversion
    # does not trigger unrelated native rebuilds. Earlier outputs are excluded.
    for path, data in files.items():
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
    destination = ROOT/scope["report"]
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    public = art.read_json(PUBLIC)
    ranges = {row["asset_id"]: row for row in report["families"]}
    # Only the already qualified two families receive staged native artwork
    # references; their saved definition-local namespace/offset stays unchanged.
    for family in public["assets"]:
        if family["asset_id"] not in ranges:
            continue
        row = ranges[family["asset_id"]]
        family["packed"].update(route=scope["native_route"], native_offset=row["native_offset"],
                                sprite_count=row["sprite_count"], source_payload=scope["report"])
    PUBLIC.write_text(json.dumps(public, indent=2)+"\n", encoding="utf-8")


def checked_native_observation(observation, sha):
    """Require complete real-loader byte/anchor evidence for the current shared payload.

    @param observation Actual source-build or generator-disabled native process receipt.
    @param sha Current complete GRF SHA256, independent of any declared success flag.
    @return None after actual native application, data, fixture and terminal checks pass.
    @throws ValueError On missing coverage, stale process/input bytes or false acceptance.
    """
    # Full actual native results, rather than preview/source flags, own packed
    # acceptance. The same exact process covers all prior and newly appended IDs.
    if (observation.get("complete") is not True or observation.get("cases") != 1
            or observation.get("assertions") != 163073 or observation.get("total_sprites") != 7764
            or observation.get("protected_sprites") != 6116 or observation.get("new_sprites") != 1648
            or observation.get("native_bound") != 7791 or observation.get("full_pixel_bytes_exact") is not True
            or observation.get("anchors_exact") is not True or observation.get("paid_world_loaded") is not False):
        raise ValueError("launch/fleet complete actual native byte/anchor acceptance is absent")
    for name in ("application", "terminal", "fixture", "runtime_payload", "copied_payload"):
        handoff.bound(observation[name])
    # Every copy consulted by the real startup must identify this actual GRF;
    # a passed observer of a previous cached payload cannot qualify new data.
    if any(observation[name]["sha256"] != sha for name in ("runtime_payload", "copied_payload")):
        raise ValueError("launch/fleet native acceptance refers to a different payload")
    terminal = (ROOT/observation["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    marker = "LAUNCH_FLEET_PACKED_ART_LOADED sprites=7764 protected=6116 new=1648 families=2 native_start=6143 native_bound=7791 full_pixel_bytes=exact"
    if marker not in terminal or "All tests passed (163073 assertions in 1 test case)" not in terminal:
        raise ValueError("launch/fleet actual full-byte terminal does not prove acceptance")


def prepare_packed_publication(proof=None, public=None):
    """Validate all actual build/load gates and prepare narrowly scoped checksum metadata.

    @param proof Optional detached build evidence for focused refusal checks.
    @param public Optional detached public manifest for protected metadata refusal checks.
    @return Qualified private packed report, proposed public manifest and source record.
    @throws ValueError On absent build/fallback/native evidence or unrelated metadata drift.
    """
    scope, accepted = read_scope()
    proof = art.read_json(BUILD_PROOF) if proof is None else proof
    public = copy.deepcopy(art.read_json(PUBLIC) if public is None else public)
    report, files = derive(public)
    payload = art.source.identity(ROOT/"media/baseset/openttd.grf")
    sha = payload["sha256"]
    checksum = (ROOT/"media/baseset/openttd.grf.hash").read_text(encoding="utf-8").strip()
    reproducible, cached = proof["reproducible"], proof["generator_free"]
    # Follow the established shared packed-binding boundary: two complete clean
    # encodings, actual cached distribution loading and format-specific checksum
    # equality must all precede any new public container identity.
    if (reproducible["sha256"] != [sha, sha] or reproducible["grfid_md5"] != [checksum, checksum]
            or [row["sha256"] for row in reproducible["outputs"]] != [sha, sha]
            or len({row["path"] for row in reproducible["outputs"]}) != 2 or len(reproducible["logs"]) != 2
            or not re.fullmatch(r"[0-9a-f]{32}", checksum)
            or cached["sha256"] != sha or cached["generator_discovery_disabled"] is not True
            or cached["native_loading_verified"] is not True
            or reproducible["source_handoff"] != scope["source_handoff"]):
        raise ValueError("launch/fleet reproducible/cached payload binding evidence is incomplete or stale")
    handoff.bound(proof["producer"])
    handoff.bound(reproducible["encoder"])
    # Local graphics publication cannot replace the accepted financial/game
    # or general-test applications at their original paths.
    for reference in art.read_json(BUILD_PROOF.parent/"preexisting-applications.json")["applications"]:
        handoff.bound(reference)
    for reference in reproducible["outputs"]+reproducible["logs"]+reproducible["source_pngs"]:
        handoff.bound(reference)
    for name in ("configure_log", "copy_log", "restore_log", "copied_payload"):
        handoff.bound(cached[name])
    cache = (ROOT/"build/CMakeCache.txt").read_text(encoding="utf-8")
    setting = re.search(r"^CMAKE_DISABLE_FIND_PACKAGE_Grfcodec:[^=]+=(.*)$", cache, re.MULTILINE)
    restored = setting.group(1) if setting is not None else None
    # Packaging qualification must leave normal project discovery as it was;
    # a proof flag cannot silently keep the user's generator disabled.
    if restored != cached["restored_discovery"]:
        raise ValueError("launch/fleet actual generator discovery configuration was not restored")
    # Exact actual source byte checks preserve the encoded artwork and native
    # declarations, independent of timestamps or a successful older build.
    if reproducible["source_outputs"] != report["outputs"]:
        raise ValueError("launch/fleet clean builds used different source declarations")
    for path, data in files.items():
        if path.read_bytes() != data:
            raise ValueError("launch/fleet staged source changed after actual native loading")
    observations = {item["configuration"]: item for item in cached["native"]}
    if len(cached["native"]) != 2 or len(observations) != 2 or set(observations) != {"Debug", "RelWithDebInfo"}:
        raise ValueError("launch/fleet cached loading is missing or duplicates a configuration")
    native_proofs = {}
    # Both original new-payload qualification and actual generator-disabled
    # loading retain their separate current application/data/input identities.
    for configuration in ("Debug", "RelWithDebInfo"):
        path = BUILD_PROOF.parent/("native-"+configuration+".json")
        original = art.read_json(path)
        checked_native_observation(original, sha)
        checked_native_observation(observations[configuration], sha)
        for name in ("producer", "native_test", "expectations"):
            handoff.bound(original[name])
        if original["application"] != observations[configuration]["application"]:
            raise ValueError("launch/fleet source and cached native application versions differ")
        native_proofs[configuration] = art.source.identity(path)
    # A container-only checksum refresh cannot change source/geometry/authoring,
    # saved definition IDs, prior native ranges, family presence or root schema.
    rows = {row["asset_id"]: row for row in public["assets"]}
    if (len(rows) != len(public["assets"]) or set(rows) != set(accepted["prior_public_rows"]) | set(handoff.ASSETS)
            or art.semantic_digest({name: value for name, value in public.items() if name != "assets"}) != accepted["public_envelope_sha256"]):
        raise ValueError("launch/fleet public schema or unrelated family set changed")
    prefix_updates = []
    for asset, expected in accepted["prior_public_rows"].items():
        row = rows[asset]
        previous = accepted["prior_public_ranges"][asset]
        normalized = dict(row, packed=dict(row["packed"]))
        current_sha = normalized["packed"].get("sha256")
        eligible = previous.get("route") in ("native_action5_1c", "native_baseset_space", "native_baseset_orbital")
        # Only actual already published native artwork in the proven prefix
        # may adopt this shared container's checksum; all other fields stay exact.
        if eligible:
            if current_sha not in (previous["sha256"], sha):
                raise ValueError("prior native artwork names an unqualified container")
            normalized["packed"]["sha256"] = previous["sha256"]
        if art.semantic_digest(normalized) != expected:
            raise ValueError("launch/fleet unrelated source/native binding changed: "+asset)
        if eligible:
            row["packed"]["sha256"] = sha
            prefix_updates.append(asset)
    # The immutable MLVI geometry definition remains a separate owner; adding
    # native art cannot rewrite its public SVG or the completed other source families.
    predecessor = art.read_json(art.FIXTURE)
    for name, expected in predecessor["protected_payload"].items():
        if name.endswith(".svg"):
            if art.source.identity(ROOT/name)["sha256"] != expected:
                raise ValueError("immutable MLVI geometry definition changed")
    document = art.source.read_record()
    for asset, expected in predecessor["completed_source_families"].items():
        if asset not in handoff.ASSETS and art.semantic_digest(document["families"][asset]) != expected:
            raise ValueError("unrelated completed source family changed")
    reference = art.source.identity(BUILD_PROOF)
    report["packed_grf"] = dict(payload, grfid_md5=checksum, proof=reference,
                                 new_sprites=1648, protected_decoded_sprites=6116, native_bound=7791,
                                 native=native_proofs, authorized_prefix_checksum_updates=prefix_updates)
    for asset in handoff.ASSETS:
        row = rows[asset]
        row["packed"].update(payload=payload["path"], sha256=sha)
        document["families"][asset].update(status="packed_runtime_pending", packed=True,
                                             packed_manifest=scope["report"], packed_proof=reference)
    return report, public, document


def bind_packed_payload():
    """Publish qualified own packed metadata and only proven prior-container checksum changes.

    @return Final packed manifest after all actual build/load gates pass.
    @throws ValueError On any failed current native, cached, source or metadata boundary.
    """
    scope, _ = read_scope()
    report, public, document = prepare_packed_publication()
    (ROOT/scope["report"]).write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    PUBLIC.write_text(json.dumps(public, indent=2)+"\n", encoding="utf-8")
    art.source.RECORD.write_text(json.dumps(document, indent=2)+"\n", encoding="utf-8")
    return report


def main():
    """Describe, stage or bind launch/fleet inputs through their separate qualified gates.

    @return Zero after pure conversion or the requested qualified stage/binding operation.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--stage", action="store_true")
    mode.add_argument("--bind-packed", action="store_true")
    args = parser.parse_args()
    if args.bind_packed:
        report = bind_packed_payload()
        print(f"Launch/fleet actual packed handoff: {report['pose_count']} new sprites; native selectors/game review remain pending")
        return 0
    report, files = derive()
    if args.stage:
        publish(report, files)
    print(f"Launch/fleet inputs: {report['pose_count']} parts, native [{report['native_start']},{report['native_sprite_bound']}), decoded/runtime pending")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
