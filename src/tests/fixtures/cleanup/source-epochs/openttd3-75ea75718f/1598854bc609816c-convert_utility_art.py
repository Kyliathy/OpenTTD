"""Select original Heat/electrical floor branches at actual native contacts.

@file convert_utility_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group60B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3B/4.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Installed shared links own domain/company/layer topology. Art creates no path or energy.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube


RECORD = tube.ROOT / "media/multilayer/source/ground/utility-generation-record.json"
REVIEW = tube.REVIEW
GUIDES = tube.ROOT / "media/multilayer/guides/ground"
ASSET = "geothermal.conduit"
DOMAINS = ("Heat", "Electricity")
STATES = ("construction", "connected", "unconnected")
DIRECTIONS = ("NE", "SE", "SW", "NW")
AXES = {"NE": "X", "SE": "Y", "SW": "X", "NW": "Y"}
PITCH = {"NE": (32, -16), "SE": (32, 16), "SW": (-32, 16), "NW": (-32, -16)}
CENTERS = {"Heat": (31, 14), "Electricity": (31, 18)}
PORTS = {domain: {direction: (center[0] + dx // 2, center[1] + dy // 2)
                  for direction, (dx, dy) in PITCH.items()} for domain, center in CENTERS.items()}
ATLAS = REVIEW / "utility-conduit-authored.png"
MANIFEST = REVIEW / "utility-conduit-source-manifest.json"


def registration_signature(candidate):
    """Bind the complete reviewed source registration independently of workflow notes.

    @param candidate Immutable source identity with its measured native transform.
    @return SHA256 over actual source bytes, domain, axis, state and registration.
    """
    fields = {key: candidate[key] for key in ("sha256", "domain", "axis", "state", "native_transform")}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode("utf-8")).hexdigest()


def public_family(public=None):
    """Resolve the one shared domain/branch contract without making a new inventory.

    @param public Optional isolated decoded public manifest for refusal checks.
    @return Checked geothermal conduit family; no installed link is inferred.
    """
    if public is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    matches = [row for row in public["assets"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate utility conduit public family")
    family = matches[0]
    notes = family.get("geometry_notes", {})
    # The two coexisting domains retain different floor contacts. A source
    # cannot merge their identity, infer flow from adjacency or become a wall.
    if (family["consumer"] != 18 or family["directions"] != list(DIRECTIONS)
            or family["states"] != list(STATES) or family["frames"] != 1
            or family["parts"] != ["body"] or family["tile_variants"] != 2
            or family.get("native_domains") != list(DOMAINS)
            or family["native_size"] != [64, 31] or family["anchor"] != [-31, 0]
            or notes.get("center_pixels") != {key: list(point) for key, point in CENTERS.items()}
            or notes.get("port_pixels") != {key: {d: list(p) for d, p in ports.items()} for key, ports in PORTS.items()}):
        raise ValueError("utility conduit public domain/contact contract mismatch")
    return family


def guides():
    """Write labelled geometry-only references before selecting original art.

    @return Four guide paths; blue reference lines never enter a source atlas.
    """
    paths = []
    # Each real domain/axis gets its own full native canvas. Endpoint labels
    # explain the fixed32/16 neighbour pitch and distinct coexisting lanes.
    for domain in DOMAINS:
        for axis in ("X", "Y"):
            page = Image.new("RGB", (1024, 640), (238, 244, 250))
            draw = ImageDraw.Draw(page)
            draw.text((24, 18), f"{domain} / {axis} / FLOOR GEOMETRY ONLY / native64x31 anchor(-31,0)", fill=(20, 50, 85))
            floor = [(31, 0), (63, 16), (31, 30), (0, 16)]
            draw.polygon([(x * 16, y * 16 + 80) for x, y in floor], outline=(130, 160, 190), width=2)
            directions = ("NE", "SW") if axis == "X" else ("NW", "SE")
            points = [PORTS[domain][direction] for direction in directions]
            draw.line([(x * 16, y * 16 + 80) for x, y in points], fill=(25, 100, 180), width=32)
            # Complete matching couplings and a compact central sleeve remain
            # visible after native reduction; the diagram prescribes contacts.
            for label, (x, y) in [(d, PORTS[domain][d]) for d in directions] + [("CENTER", CENTERS[domain])]:
                draw.ellipse((x * 16 - 24, y * 16 + 56, x * 16 + 24, y * 16 + 104), outline=(10, 65, 125), width=3)
                draw.text((x * 16 + 30, y * 16 + 68), label, fill=(20, 50, 85))
            draw.text((24, 600), "One straight original; no ground slab, track, tall hub, text or guide pixels in generated art.", fill=(20, 50, 85))
            path = GUIDES / f"utility-{domain.lower()}-{axis.lower()}-geometry-v1.png"
            page.save(path)
            paths.append(path)
    return paths


def validate_axis(whole, domain, axis):
    """Reject missing contacts or a tall/off-axis floor installation.

    @param whole Indexed64x31 complete immutable original at its registered anchor.
    @param domain Actual separate Heat or Electricity contact lane.
    @param axis Independently authored X or Y physical straight geometry.
    @return None; invalid placement cannot be repaired by drawing replacement pixels.
    """
    if domain not in DOMAINS or axis not in ("X", "Y"):
        raise ValueError("unknown utility domain/axis")
    bounds = whole.getbbox()
    if bounds is None:
        raise ValueError("empty native utility original")
    left, top, right, bottom = bounds
    if left < 11 or right > 52 or top < 2 or bottom > 31:
        raise ValueError("utility original leaves floor contact envelope")
    if right - left < 30 or bottom - top < 14:
        raise ValueError("utility original loses full opposing contacts")
    cx, cy = CENTERS[domain]
    # The original stays near the physical centerline. Large machines, slab
    # backgrounds and wrong isometric views cannot pass on a filename alone.
    for offset, value in enumerate(whole.get_flattened_data()):
        x, y = offset % 64, offset // 64
        residual = abs(2 * (y - cy) + (x - cx)) if axis == "X" else abs(2 * (y - cy) - (x - cx))
        if value != 0 and residual > 8:
            raise ValueError(f"utility original leaves its {domain}/{axis} floor axis: {x},{y}")
    directions = ("NE", "SW") if axis == "X" else ("NW", "SE")
    # Connected, capped and preview couplings all retain the same footprint;
    # state changes must never shift the floor ports or erase the whole cell.
    for px, py in [CENTERS[domain]] + [PORTS[domain][d] for d in directions]:
        if not any(whole.getpixel((x, y)) != 0 for y in range(py - 1, py + 2) for x in range(px - 1, px + 2)):
            raise ValueError("utility original has a missing native contact")


def native_original(key, document=None, public=None):
    """Sample one retained original and enforce its reviewed registration.

    @param key Exact immutable candidate key, not a runtime utility identity.
    @param document Optional isolated provenance for refusal checks.
    @param public Optional isolated shared manifest from the same operation.
    @return Indexed native whole-axis model and complete checked source metadata.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    family = public_family(public)
    # Rejected artwork cannot return through a valid raw hash. A foreign
    # family/domain/state cannot substitute for a missing physical original.
    if (source["asset_id"] != ASSET or source["status"].startswith("rejected")
            or source["domain"] not in DOMAINS or source["state"] not in STATES
            or source["axis"] not in ("X", "Y")):
        raise ValueError("utility original is rejected or has a foreign identity")
    transform = source["native_transform"]
    expected = family["geometry_notes"].get("approved_utility_registrations", {}).get(key)
    if expected is not None and expected != registration_signature(source):
        raise ValueError("utility original changes its reviewed registration")
    width, height = transform["native_subject_size"]
    px, py = transform["native_position"]
    # Registration may be measured before approval, then becomes immutable.
    # Only small floor-sized originals can reach the shared native canvas.
    if (transform["native_size"] != [64, 31] or transform["anchor"] != [-31, 0]
            or transform["resampling"] != "BOX" or not 32 <= width <= 40
            or not 18 <= height <= 24 or not 11 <= px <= 15
            or not 2 <= py <= 10 or px + width > 64 or py + height > 31):
        raise ValueError("unsupported utility native registration")
    with Image.open(tube.base.repository_path(source["path"])) as image:
        original = image.convert("RGBA")
        crop = transform["source_crop"]
        alpha = original.getchannel("A").point(lambda value: 255 if value >= 16 else 0)
        # An isolated source must have genuine transparent clearance on all
        # sides, and its ordinary rectangle must contain every visible pixel.
        bounds = alpha.getbbox()
        if (bounds is None or bounds[0] < 16 or bounds[1] < 16
                or original.width - bounds[2] < 16 or original.height - bounds[3] < 16
                or not 0 <= crop[0] <= bounds[0] < bounds[2] <= crop[2] <= original.width
                or not 0 <= crop[1] <= bounds[1] < bounds[3] <= crop[3] <= original.height):
            raise ValueError("utility source crop clips its original or lacks clearance")
        selected = original.crop(crop).resize((width, height), Image.Resampling.BOX)
    canvas = Image.new("RGBA", (64, 31))
    canvas.paste(selected, (px, py))
    whole = tube.base.quantize(canvas, {})
    validate_axis(whole, source["domain"], source["axis"])
    return whole, source


def branch(whole, domain, direction):
    """Select an exact original half and retain its shared compact central sleeve.

    @param whole Complete checked indexed original on the correct physical axis.
    @param domain Actual separately installed domain at its own contact lane.
    @param direction Real cardinal neighbour edge in the native projection.
    @return Unchanged subset; no raster rotation, mirroring or new fitting pixels.
    """
    cx, cy = CENTERS[domain]
    result = whole.copy()
    pixels = []
    # All four branch states can compose an installed isolated/junction cell.
    # Only the selected arm survives; the small sleeve overlaps at one center.
    for offset, value in enumerate(whole.get_flattened_data()):
        x, y = offset % 64, offset // 64
        center = cx - 2 <= x <= cx + 2 and cy - 2 <= y <= cy + 2
        selected = x >= cx if direction in ("NE", "SE") else x <= cx
        pixels.append(value if center or selected else 0)
    result.putdata(pixels)
    return result


def capped_original(connected, capped, domain, axis):
    """Select only original opaque end-cap fittings onto unchanged installed hardware.

    @param connected Approved connected whole-axis indexed model and native opacity.
    @param capped Independently authored capped original at its frozen registration.
    @param domain Actual Heat or Electricity lane, never a recolour selector.
    @param axis Actual independently authored X or Y physical view.
    @return Exact connected body with selected original cap pixels, plus their coordinates.
    """
    result = connected.copy()
    selected = []
    directions = ("NE", "SW") if axis == "X" else ("NW", "SE")
    # Topology changes only each existing end fitting. Sampling capped source
    # interiors preserves every other indexed body pixel and native opacity;
    # whole-model regeneration drift cannot move a paid installed conduit.
    for direction in directions:
        px, py = PORTS[domain][direction]
        changed = 0
        for y in range(py - 2, py + 3):
            for x in range(px - 2, px + 3):
                original, replacement = connected.getpixel((x, y)), capped.getpixel((x, y))
                if original == 0 or replacement == 0:
                    continue
                result.putpixel((x, y), replacement)
                selected.append([x, y])
                changed += original != replacement
        if changed == 0:
            raise ValueError("utility capped fitting has no distinct original native pixels")
    validate_axis(result, domain, axis)
    return result, selected


def build(document=None, public=None):
    """Compose every domain/state/physical branch from its retained original.

    @param document Optional complete isolated source record for refusal checks.
    @param public Optional isolated shared Sprite Contract.
    @return Complete24-pose source manifest and reproducible indexed PNG bytes.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    spec = public_family(public)
    matches = [row for row in document["families"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate utility source family")
    family = matches[0]
    required = {(domain, state, axis) for domain in DOMAINS for state in STATES for axis in ("X", "Y")}
    identities = [(row["domain"], row["state"], row["axis"]) for row in family["axis_selections"]]
    if len(identities) != len(required) or set(identities) != required:
        raise ValueError("incomplete utility domain/state/axis coverage")
    originals = {}
    # Independently authored axes and state selections must retain the same
    # identity as their checked source; relabelling another pose is refused.
    for row in family["axis_selections"]:
        whole, source = native_original(row["source"], document, public)
        identity = row["domain"], row["state"], row["axis"]
        if (source["domain"], source["state"], source["axis"]) != identity:
            raise ValueError("utility original substitutes a domain/state/axis")
        if row["source"] not in spec["geometry_notes"].get("approved_utility_registrations", {}):
            raise ValueError("utility original lacks a reviewed registration")
        originals[identity] = whole, source
    atlas = Image.new("P", (256, 186), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Stable domain/state/branch ordering adds no runtime range. Each variant
    # stays in its own domain and copies exact original indexed pixels only.
    for variant, domain in enumerate(DOMAINS):
        for state_index, state in enumerate(STATES):
            for column, direction in enumerate(DIRECTIONS):
                whole, source = originals[domain, state, AXES[direction]]
                ancestry = {"selection_policy": "unchanged_original_whole_axis_half"}
                if state == "unconnected":
                    connected, body = originals[domain, "connected", AXES[direction]]
                    whole, fitting_pixels = capped_original(connected, whole, domain, AXES[direction])
                    ancestry = {"selection_policy": "unchanged_connected_body_with_original_opaque_end_cap_fittings",
                                "body_source": body["path"], "body_sha256": body["sha256"],
                                "body_registration_sha256": registration_signature(body), "cap_pixels": fitting_pixels}
                x, y = column * 64, (variant * 3 + state_index) * 31
                atlas.paste(branch(whole, domain, direction), (x, y))
                sprites.append({"direction": direction, "state": state, "part": "body", "frame": 0,
                                "tile_variant": variant, "domain": domain, "box": [x, y, 64, 31],
                                "anchor": [-31, 0], "source": source["path"], "source_sha256": source["sha256"],
                                "registration_sha256": registration_signature(source),
                                "native_transform": source["native_transform"], **ancestry})
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md60B; Plan18 installed utility paths; archi-map-layers.md. Original source only, packing/runtime pending.",
              "schema_version": 1, "asset_id": ASSET, "path": ATLAS.relative_to(tube.ROOT).as_posix(),
              "size": [256, 186], "native_cell": [64, 31], "anchor": [-31, 0], "pose_count": 24,
              "source_record": RECORD.relative_to(tube.ROOT).as_posix(), "sprites": sprites,
              "source_coverage_complete": True, "runtime_activated": False,
              "draw_order": "Heat then Electricity floor body below vehicles/buildings; all four installed-cell branches reflect actual same-owner/domain cardinal adjacency"}
    return report, tube.indexed_png(atlas)


def validate(report, encoded, document=None, public=None):
    """Reject incomplete coverage, changed pixels, provenance or false activation.

    @param report Complete source mappings supplied for source publication.
    @param encoded Actual indexed PNG atlas bytes to decode.
    @param document Optional isolated provenance for corruption checks.
    @param public Optional isolated shared contract for corruption checks.
    @return None; successful source validation installs no link or packed sprite.
    """
    required = {(domain, state, direction, "body", 0, variant) for variant, domain in enumerate(DOMAINS)
                for state in STATES for direction in DIRECTIONS}
    identities = [(r["domain"], r["state"], r["direction"], r["part"], r["frame"], r["tile_variant"]) for r in report["sprites"]]
    if len(identities) != len(required) or set(identities) != required:
        raise ValueError("incomplete utility source pose identities")
    if report.get("runtime_activated") is not False:
        raise ValueError("utility source falsely claims runtime activation")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Floor overlays use the shared nonanimated native palette; altered
        # transparency or reserved animation colours can hide or imply flow.
        if (atlas.mode != "P" or atlas.size != (256, 186) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES
                or set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0})):
            raise ValueError("utility source palette/canvas mismatch")
    expected, pixels = build(document, public)
    if report["sprites"] != expected["sprites"]:
        raise ValueError("utility source changes original ancestry or anchors")
    if encoded != pixels:
        raise ValueError("utility source changes original branch pixels")


def contact_board(report, encoded):
    """Review every real adjacency mask, neighbouring join and domain coexistence.

    @param report Checked24-pose source manifest with actual source mappings.
    @param encoded Exact indexed source atlas bytes.
    @return PNG contact board; layouts are illustrative source assemblies only.
    """
    page = Image.new("RGB", (1536, 1460), (46, 50, 57))
    draw = ImageDraw.Draw(page)
    draw.text((18, 15), "INSTALLED UTILITY SOURCE / actual four cardinal branches / no flow inference / packing and runtime pending", fill="white")
    sprites = {}
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Decode stored crops independently so review covers the actual atlas,
        # including both domain lanes and each capped/connected branch state.
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            sprites[row["domain"], row["state"], row["direction"]] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
    # All16 neighbour masks are legal per native cell; zero is a paid isolated
    # capped installation, not absent infrastructure. Both domains are shown.
    for column, domains in enumerate((("Heat",), ("Electricity",), DOMAINS)):
        draw.text((column * 512 + 18, 44), "+".join(domains) + ": all16 native adjacency masks, enlarged4x", fill="white")
        for mask in range(16):
            scene = Image.new("RGBA", (64, 31))
            for domain in domains:
                for bit, direction in enumerate(DIRECTIONS):
                    state = "connected" if mask & (1 << bit) else "unconnected"
                    scene.alpha_composite(sprites[domain, state, direction])
            scene = scene.resize((256, 124), Image.Resampling.NEAREST)
            left, top = column * 512 + mask % 2 * 256, 82 + mask // 2 * 148
            page.paste(scene, (left, top), scene)
            draw.text((left + 12, top - 16), f"mask{mask:02d}", fill="white")
    draw.text((18, 1280), "Connected/capped ends and noninstalled construction preview4x; actual-neighbour joins in companion geometry contact", fill="white")
    for column, domain in enumerate(DOMAINS):
        for index, state in enumerate(STATES):
            scene = Image.new("RGBA", (64, 31))
            for direction in DIRECTIONS:
                scene.alpha_composite(sprites[domain, state, direction])
            scene = scene.resize((256, 124), Image.Resampling.NEAREST)
            left, top = column * 768 + index * 256, 1320
            page.paste(scene, (left, top), scene)
            draw.text((left + 12, top - 18), domain + " " + state, fill="white")
    stream = io.BytesIO()
    page.save(stream, format="PNG")
    return stream.getvalue()


def original_board(document=None, public=None):
    """Compare all original hardware states with the restricted cap compositions.

    @param document Optional retained original record from the same publication.
    @param public Optional shared contract with approved source registrations.
    @return Reproducible PNG showing12 originals and four unchanged-body compositions.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == ASSET)
    selections = {(row["domain"], row["state"], row["axis"]): row["source"] for row in family["axis_selections"]}
    page = Image.new("RGB", (1280, 900), (46, 50, 57))
    draw = ImageDraw.Draw(page)
    # Whole-source comparisons expose equipment drift before restricted end
    # selection; the last row then verifies the actual installed-body result.
    for column, (domain, axis) in enumerate((d, a) for d in DOMAINS for a in ("X", "Y")):
        models = {}
        for row, state in enumerate(STATES):
            whole, source = native_original(selections[domain, state, axis], document, public)
            models[state] = whole
            sprite = whole.convert("RGBA").resize((320, 155), Image.Resampling.NEAREST)
            page.paste(sprite, (column * 320, 40 + row * 200), sprite)
            draw.text((column * 320 + 8, 20 + row * 200), f"{domain}/{axis}/{state}: whole original", fill="white")
        composed, pixels = capped_original(models["connected"], models["unconnected"], domain, axis)
        sprite = composed.convert("RGBA").resize((320, 155), Image.Resampling.NEAREST)
        page.paste(sprite, (column * 320, 690), sprite)
        draw.text((column * 320 + 8, 670), f"{domain}/{axis}: cap selection only", fill="white")
    stream = io.BytesIO()
    page.save(stream, format="PNG")
    return stream.getvalue()


def geometry_board(report, encoded):
    """Show stored connected axes, turns and actual adjacent-cell coupling joins.

    @param report Complete checked source mapping with both physical domains.
    @param encoded Indexed atlas to decode; no reference diagrams supply pixels.
    @return Reproducible PNG with native contact and coexistence comparisons.
    """
    page = Image.new("RGB", (1536, 1330), (46, 50, 57))
    draw = ImageDraw.Draw(page)
    sprites = {}
    with Image.open(io.BytesIO(encoded)) as atlas:
        for row in report["sprites"]:
            if row["state"] == "connected":
                x, y, width, height = row["box"]
                sprites[row["domain"], row["direction"]] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
    # Separate domains and their combined presentation share the same real
    # branch pixels; changing layout never repaints or synthesizes geometry.
    for column, domains in enumerate((("Heat",), ("Electricity",), DOMAINS)):
        draw.text((column * 512 + 12, 12), "+".join(domains) + " connected original geometry / source only", fill="white")
        for row, directions in enumerate((("NE", "SW"), ("NW", "SE"), ("NE", "SE"), DIRECTIONS)):
            scene = Image.new("RGBA", (64, 31))
            for domain in domains:
                for direction in directions:
                    scene.alpha_composite(sprites[domain, direction])
            scene = scene.resize((512, 248), Image.Resampling.NEAREST)
            page.paste(scene, (column * 512, 50 + row * 260), scene)
            draw.text((column * 512 + 12, 34 + row * 260), "+".join(directions), fill="white")
        scene = Image.new("RGBA", (128, 64))
        # Opposing half-branches from two actual neighbouring cell anchors
        # meet at the same shared coupling, without an invented joining pixel.
        for domain in domains:
            for direction, origin in (("NE", (16, 24)), ("SW", (48, 8))):
                scene.alpha_composite(sprites[domain, direction], origin)
        scene = scene.resize((512, 256), Image.Resampling.NEAREST)
        page.paste(scene, (column * 512, 1080), scene)
        draw.text((column * 512 + 12, 1064), "actual32/-16 neighbour join", fill="white")
    stream = io.BytesIO()
    page.save(stream, format="PNG")
    return stream.getvalue()


def main():
    """Generate geometry guides, review an original or publish checked source art.

    @return None; command flags control repository-local reproducible artifacts only.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--guides", action="store_true")
    parser.add_argument("--candidate")
    parser.add_argument("--publish", action="store_true")
    parser.add_argument("--bind", action="store_true")
    options = parser.parse_args()
    if options.guides:
        for path in guides():
            print(path.relative_to(tube.ROOT).as_posix())
        return
    if options.candidate:
        whole, source = native_original(options.candidate)
        path = REVIEW / (options.candidate + "-native.png")
        path.write_bytes(tube.indexed_png(whole))
        whole.resize((768, 372), Image.Resampling.NEAREST).save(REVIEW / (options.candidate + "-review.png"), transparency=0)
        print(json.dumps({"path": path.as_posix(), "registration": registration_signature(source), "bounds": whole.getbbox()}))
        return
    report, encoded = build()
    validate(report, encoded)
    if options.publish:
        ATLAS.write_bytes(encoded)
        MANIFEST.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        (REVIEW / "utility-conduit-contact.png").write_bytes(contact_board(report, encoded))
        (REVIEW / "utility-conduit-originals-contact.png").write_bytes(original_board())
        (REVIEW / "utility-conduit-geometry-contact.png").write_bytes(geometry_board(report, encoded))
    if options.bind:
        # Shared source binding is allowed only after the complete actual atlas
        # has been published; it assigns no packed range or gameplay consumer.
        if not options.publish or ATLAS.read_bytes() != encoded:
            raise ValueError("utility source binding requires checked publication")
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        family = public_family(public)
        family["source"].update(status="authored", path=report["path"], sha256=hashlib.sha256(encoded).hexdigest(),
                                manifest=MANIFEST.relative_to(tube.ROOT).as_posix(), source_coverage_complete=True,
                                runtime_activated=False, rights="first_party_original_imagegen")
        tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    print("Utility conduit source:24 exact poses; Heat/Electricity separate; packing/runtime pending")


if __name__ == "__main__":
    main()
