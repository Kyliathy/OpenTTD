"""Compare actual native facility, glass and vehicle foreground compositions with accepted sources.

@file ground_station_pixel_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native invisible captures supply unchanged world background. Independent source
pixels, actual physical boxes and native destination remaps predict visible composition.
"""

import argparse
import json

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference


def expected_matrix():
    """Declare the exact required native role, state, phase and appearance matrix.

    @return All210 distinct actual station/terminal scene identities.
    """
    return {(asset, graphics, state, phase, appearance)
            for asset, count in (("facility.rail-station", 8), ("facility.road-terminal", 6))
            for graphics in range(count) for state in ("ready", "working")
            for phase in (range(4) if state == "working" else (0,)) for appearance in range(3)}


def source_pixels(pose, atlas, baseline):
    """Place original opaque canvas pixels at an independently declared screen reference.

    @param pose Accepted indexed source cell with its original canvas box.
    @param atlas Immutable indexed source atlas.
    @param baseline Screen position of the original full source canvas's top-left corner.
    @return Exact source-owned screen coordinates and palette indices.
    """
    left, top, width, height = pose["box"]
    pixels = atlas.crop((left, top, left + width, top + height)).tobytes()
    return {(baseline[0] + index % width, baseline[1] + index // width): value
            for index, value in enumerate(pixels) if value != 0}


def depth_order(nodes):
    """Order independently sorted native physical boxes by unambiguous world-axis separation.

    @param nodes Actual parent/vehicle origins and extents, with attached children excluded from sorting.
    @return Draw order and count of ambiguous intersecting screen masks.
    @throws ValueError On contradictory physical ordering or an overlapping source pair without a unique depth relation.
    """
    successors = {index: set() for index in range(len(nodes))}
    indegree = {index: 0 for index in range(len(nodes))}
    ambiguous = 0
    # A complete separation along one native world axis constrains drawing.
    # Conflicting diagonal separations are harmless only where source masks
    # do not overlap; a guessed sequence cannot authorize foreground pixels.
    for first in range(len(nodes)):
        for second in range(first + 1, len(nodes)):
            a, b = nodes[first], nodes[second]
            before = any(a["origin"][axis] + a["extent"][axis] <= b["origin"][axis] for axis in range(3))
            after = any(b["origin"][axis] + b["extent"][axis] <= a["origin"][axis] for axis in range(3))
            overlap = set(a["pixels"]) & set(b["pixels"])
            if before == after:
                if overlap:
                    ambiguous += len(overlap)
                    raise ValueError("overlapping source masks lack unique physical depth: {} / {}".format(a["name"], b["name"]))
                continue
            source, target = (first, second) if before else (second, first)
            successors[source].add(target)
            indegree[target] += 1
    order = []
    # Stable unrelated nodes have disjoint source masks, so choosing their
    # original sequence has no pixel effect. Parent children remain attached.
    while len(order) != len(nodes):
        ready = [index for index in range(len(nodes)) if indegree[index] == 0 and index not in order]
        if not ready:
            raise ValueError("native physical boxes contain a foreground ordering cycle")
        index = ready[0]
        order.append(index)
        for target in successors[index]:
            indegree[target] -= 1
    return order, ambiguous


def compose(base, nodes, order, appearance, transparent_remap):
    """Apply accepted source masks and native destination filters to actual unchanged world background.

    @param base Exact native invisible world/vehicle capture before facility overlays.
    @param nodes Source-owned parent/vehicle pixels and attached relative glass masks.
    @param order Independently constrained native world depth order.
    @param appearance Native opaque0, transparent1 or invisible2 preference.
    @param transparent_remap Actual native palette destination recolour table.
    @return Expected source-owned pixels, final owners, glass-over-vehicle count and foreground overwrite count.
    """
    pixels, owners, glass_vehicle, foreground = {}, {}, 0, 0
    # Repainting the actual body after any rear parent preserves independent
    # vehicle sorting. Building transparency changes destination pixels; its
    # glass child never becomes a separately sorted world object.
    for index in order:
        node = nodes[index]
        if appearance == 2 and node["name"] != "body":
            continue
        for layer in [node, *node.get("children", [])]:
            filtered = layer.get("transparent", False) or (appearance == 1 and node["name"] != "body")
            for point, value in layer["pixels"].items():
                if owners.get(point) == "body" and node["name"] != "body":
                    if layer["name"] == "glass":
                        glass_vehicle += 1
                    else:
                        foreground += 1
                if filtered:
                    previous = pixels.get(point, base[point[1] * 640 + point[0]])
                    pixels[point] = transparent_remap[previous]
                else:
                    pixels[point] = value
                    owners[point] = layer["name"]
    return pixels, owners, glass_vehicle, foreground


def verify(execution):
    """Require exact source/native composition for every current default facility view.

    @param execution Successful actual native station execution report beneath repository logs.
    @return Hash-bound source, body, depth, glass and appearance evidence for the exact native process.
    """
    proof = handoff.read_json(execution)
    observed = proof["observation"]
    # Only a complete actual process can supply a native world background and
    # owner observations; plausible standalone source canvases are insufficient.
    if proof.get("passed") is not True or observed["exit_code"] != 0:
        raise ValueError("station pixels require a successful actual native process")
    artifacts = {row["path"]: checked_reference(row) for row in observed["artifacts"]}
    metadata = [path for path in artifacts.values() if path.name == "station-modules.json"]
    if len(metadata) != 1:
        raise ValueError("station pixels lack unique actual native metadata")
    scene = handoff.read_json(metadata[0].relative_to(ROOT).as_posix())
    frames = {(row["asset_id"], row["graphics"], row["state"], row["frame"], row["appearance"]): row for row in scene["frames"]}
    required = expected_matrix()
    if len(frames) != len(scene["frames"]) or set(frames) != required:
        raise ValueError("native station source matrix is incomplete or duplicated")
    scope, before, public = handoff.read_scope()
    families = {row["asset_id"]: row for row in public["assets"]}
    sources, documents, images = {}, {}, {}
    report_path = "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1/packed-ground-manifest.json"
    packed = handoff.read_json(report_path)
    offsets = {(row["asset_id"], row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]): row["native_offset"]
               for row in packed["sprites"]}
    results, buffers = [], {}
    # Each scene retains exact source identities and actual native raw raster
    # provenance. The invisible counterpart supplies only its own world pixels.
    for identity, frame in frames.items():
        asset, graphics, state, phase, appearance = identity
        if asset not in sources:
            sources[asset] = handoff.source_poses(families[asset], scope, before, documents, images)
        poses, atlas = sources[asset]
        family = families[asset]
        rail = asset == "facility.rail-station"
        direction = family["directions"][graphics % 2 if rail else graphics]
        variant = graphics // 2 if rail else 0
        indices = metadata[0].parent.parent / "images" / (frame["basename"] + ".idx")
        base_frame = frames[asset, graphics, state, phase, 2]
        base_path = indices.with_name(base_frame["basename"] + ".idx")
        for path in (indices, base_path):
            relative = path.relative_to(ROOT).as_posix()
            if relative not in artifacts:
                raise ValueError("station indexed frame lacks actual native provenance")
            if relative not in buffers:
                buffers[relative] = path.read_bytes()
            if len(buffers[relative]) != 640 * 360:
                raise ValueError("station native indexed frame has unexpected dimensions")
        base = buffers[base_path.relative_to(ROOT).as_posix()]
        actual = buffers[indices.relative_to(ROOT).as_posix()]
        nodes = []
        required_parts = {index for index, part in enumerate(family["parts"])
                          if not poses[(state, phase, direction, variant, part)]["structurally_absent"]}
        # Each installed native sequence must retain every declared nonempty
        # parent/child exactly once; disjoint masks cannot expose an omitted
        # platform merely by comparing the other surviving source components.
        if len(frame["parts"]) != len(required_parts) or {part["index"] for part in frame["parts"]} != required_parts:
            raise ValueError("native facility sequence omits or duplicates a required source part")
        for native in frame["parts"]:
            part = family["parts"][native["index"]]
            key = (state, phase, direction, variant, part)
            pose = poses[key]
            # Installed native sequences must use exactly their declared source
            # slots. Structural absence cannot acquire a synthetic world parent.
            if pose["structurally_absent"] or native["native_sprite_offset"] != offsets[(asset, *key)] or native["opaque"]:
                raise ValueError("native facility part disagrees with its accepted source/absence/appearance contract")
            layer = {"name": part, "pixels": source_pixels(pose, atlas, (289, 131)),
                     "transparent": native["transparent"]}
            if native["parent"] == (part == "glass"):
                raise ValueError("native source changed independent-parent or relative-glass ownership")
            if native["parent"]:
                nodes.append({**layer, "origin": native["origin"], "extent": native["extent"], "children": []})
            else:
                # Only the accepted glass child can borrow its immediate roof;
                # it has no independent world box or replacement sort position.
                if part != "glass" or not nodes or nodes[-1]["name"] != "roof":
                    raise ValueError("native glass is detached from its roof parent")
                nodes[-1]["children"].append(layer)
        body_count = 0
        if frame["vehicle_visible"]:
            body = frame["body"]
            body_asset = body["asset_id"]
            if body_asset not in sources:
                sources[body_asset] = handoff.source_poses(families[body_asset], scope, before, documents, images)
            body_poses, body_atlas = sources[body_asset]
            if body["cargo_label"] != "COAL" or body["load"] != 1:
                raise ValueError("station foreground lacks its actual packet-backed cargo boundary")
            body_key = ("partial", 0, families[body_asset]["directions"][body["direction"]], 0, "body")
            pose = body_poses[body_key]
            x, y = [a + b for a, b in zip(body["baseline"], pose["anchor"])]
            pixels = {point: body["remap"][value] for point, value in source_pixels(pose, body_atlas, (x, y)).items()}
            if any(base[point[1] * 640 + point[0]] != value for point, value in pixels.items()):
                raise ValueError("native invisible foreground body differs from accepted source pixels")
            body_count = len(pixels)
            origin = [body["local_position"][0] + body["origin"][0], body["local_position"][1] + body["origin"][1], body["origin"][2]]
            nodes.append({"name": "body", "pixels": pixels, "origin": origin, "extent": body["extent"]})
        order, ambiguous = depth_order(nodes)
        expected, owners, glass_vehicle, foreground = compose(base, nodes, order, appearance, scene["transparent_remap"])
        wrong, _, _, _ = compose(base, nodes, list(reversed(order)), appearance, scene["transparent_remap"])
        mismatches = [(point, value, actual[point[1] * 640 + point[0]]) for point, value in expected.items()
                      if actual[point[1] * 640 + point[0]] != value]
        if mismatches:
            raise ValueError("native station composition differs {} matched={} missing={} first={}".format(frame["basename"], len(expected) - len(mismatches), len(mismatches), mismatches[:4]))
        results.append({"asset_id": asset, "graphics": graphics, "state": state, "frame": phase, "appearance": appearance,
                        "native_indices": reference(indices), "matched_source_composition_pixels": len(expected),
                        "matched_body_pixels_in_invisible_background": body_count, "glass_vehicle_pixels": glass_vehicle,
                        "foreground_vehicle_pixels": foreground, "ambiguous_depth_pixels": ambiguous,
                        "reversed_depth_different_pixels": sum(value != wrong.get(point) for point, value in expected.items()),
                        "source_atlas": reference(handoff.bound(family["authoring"]["native_atlas"]))})
    return {"_documentation": __doc__, "passed": True, "execution": reference(handoff.bound(execution)),
            "frames": results, "native_scenes": len(results),
            "matched_source_composition_pixels": sum(row["matched_source_composition_pixels"] for row in results),
            "glass_vehicle_pixels": sum(row["glass_vehicle_pixels"] for row in results),
            "foreground_vehicle_pixels": sum(row["foreground_vehicle_pixels"] for row in results),
            "reversed_depth_different_pixels": sum(row["reversed_depth_different_pixels"] for row in results),
            "human_acceptance": "pending", "transported_bootstrap_claimed": False}


def main():
    """Verify one exact current station run and retain its source/native foreground result.

    @return None; any source, depth, glass or appearance mismatch fails before successful publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execution", required=True)
    parser.add_argument("--report", required=True)
    args = parser.parse_args()
    output = handoff.bound(args.report)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("station pixel evidence belongs beneath repository logs")
    result = verify(args.execution)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: result[key] for key in ("passed", "native_scenes", "matched_source_composition_pixels", "glass_vehicle_pixels", "foreground_vehicle_pixels", "reversed_depth_different_pixels")}))


if __name__ == "__main__":
    main()
