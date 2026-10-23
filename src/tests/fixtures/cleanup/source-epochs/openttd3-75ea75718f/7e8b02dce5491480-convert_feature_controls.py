"""Convert original tube/geothermal menu glyphs at the shared native20x20 anchor.

@file convert_feature_controls.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group78C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md completed Group78B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native commands own availability; source artwork enables no toolbar or gameplay provider.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_control_art as controls
import convert_tube_art as tube


base = controls.base
RECORD = base.ROOT / "media/multilayer/source/ground/feature-control-generation-record.json"
OUTPUT = tube.REVIEW
ASSETS = ("control.tube", "control.geothermal")
STATES = ("enabled", "disabled", "pressed")


def registration_signature(source):
    """Freeze the checked original identity and measured native glyph registration.

    @param source Immutable candidate with asset/state and full native transform.
    @return SHA256 independent of review/workflow notes.
    """
    fields = {key: source[key] for key in ("sha256", "asset_id", "state", "native_transform")}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode("utf-8")).hexdigest()


def public_family(asset_id, public=None):
    """Resolve exact shared menu geometry without inventing a native widget binding.

    @param asset_id Exact tube or geothermal toolbar source identity.
    @param public Optional isolated shared manifest for refusal tests.
    @return Checked20x20 one-view/three-state native menu contract.
    """
    if public is None:
        public = json.loads(base.PUBLIC.read_text(encoding="utf-8"))
    if asset_id not in ASSETS:
        raise ValueError("unknown related feature control family")
    matches = [row for row in public["assets"] if row["asset_id"] == asset_id]
    if len(matches) != 1:
        raise ValueError("missing or duplicate feature control family")
    spec = matches[0]
    # Menu glyphs communicate actual feature admission and selection. They
    # have no world heading, physical machinery cell or invented animation.
    # Plan7 owns their shared UI contract; Plans17/18 retain command admission.
    if (spec["kind"] != "icon" or spec["directions"] != ["none"] or spec["frames"] != 1
            or spec["states"] != list(STATES) or spec["parts"] != ["body"]
            or spec["native_size"] != [20, 20] or spec["anchor"] != [0, 0]
            or spec["tile_variants"] != 1 or spec["consumer"] != 7):
        raise ValueError("feature control native geometry/state/owner mismatch")
    return spec


def native_original(key, document=None, public=None):
    """Resample one entire retained isolated glyph without painting missing UI states.

    @param key Exact retained original candidate key.
    @param document Optional isolated source record for refusal checks.
    @param public Optional isolated shared source contract.
    @return Indexed20x20 glyph and checked original identity/registration.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    spec = public_family(source["asset_id"], public)
    if source["status"].startswith("rejected") or source["state"] not in STATES:
        raise ValueError("feature control original is rejected or has an unknown state")
    approved = spec.get("geometry_notes", {}).get("approved_control_registrations", {}).get(key)
    if approved is not None and approved != registration_signature(source):
        raise ValueError("feature control changes its reviewed registration")
    transform = source["native_transform"]
    width, height = transform["native_subject_size"]
    x, y = transform["native_position"]
    # Each whole menu glyph stays centered with transparent native margins.
    # A shifted crop cannot turn a UI symbol into another footprint or cursor.
    if (transform["native_size"] != [20, 20] or transform["anchor"] != [0, 0]
            or transform["resampling"] != "BOX" or not 8 <= width <= 18 or not 8 <= height <= 18
            or x != (20 - width) // 2 or y != (20 - height) // 2):
        raise ValueError("feature control native registration leaves centered menu canvas")
    with Image.open(base.repository_path(source["path"])) as image:
        original = image.convert("RGBA")
        bounds = original.getchannel("A").point(lambda value: 255 if value >= 16 else 0).getbbox()
        crop = transform["source_crop"]
        # Isolated originals use ordinary measured rectangles and genuine
        # transparent clearance; clipping a refusal tick cannot be hidden.
        if (bounds is None or min(bounds[0], bounds[1], original.width - bounds[2], original.height - bounds[3]) < 16
                or not 0 <= crop[0] <= bounds[0] < bounds[2] <= crop[2] <= original.width
                or not 0 <= crop[1] <= bounds[1] < bounds[3] <= crop[3] <= original.height):
            raise ValueError("feature control crop clips its original or lacks clearance")
        selected = original.crop(crop).resize((width, height), Image.Resampling.BOX)
    canvas = Image.new("RGBA", (20, 20))
    canvas.paste(selected, (x, y))
    return base.quantize(canvas, {}), source


def build(document=None, public=None):
    """Build the exact six related control poses from six independently authored states.

    @param document Optional isolated provenance for corruption checks.
    @param public Optional isolated shared manifest.
    @return Per-family source reports and exact indexed PNG byte maps.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    identities = [row["asset_id"] for row in document["families"]]
    if len(identities) != 2 or set(identities) != set(ASSETS):
        raise ValueError("incomplete related control family coverage")
    reports, files = {}, {}
    # Source identity and native byte coverage stay explicit for both controls.
    # Other ground icons and any later runtime widget retain their own owners.
    for family in document["families"]:
        asset_id = family["asset_id"]
        spec = public_family(asset_id, public)
        selections = family["state_selections"]
        if set(selections) != set(STATES) or len(set(selections.values())) != 3:
            raise ValueError("missing or duplicate authored feature control state")
        atlas = Image.new("P", (60, 20), 0)
        atlas.putpalette(base.PALETTE_BYTES)
        sprites = []
        # Each state is a real retained source, rather than an algorithmic
        # grey/selection recolour or a relabelled enabled glyph.
        for column, state in enumerate(STATES):
            glyph, source = native_original(selections[state], document, public)
            if (source["asset_id"], source["state"]) != (asset_id, state):
                raise ValueError("feature control substitutes another family/state original")
            if source["key"] not in spec.get("geometry_notes", {}).get("approved_control_registrations", {}):
                raise ValueError("feature control lacks a reviewed native registration")
            atlas.paste(glyph, (column * 20, 0))
            sprites.append({"state": state, "direction": "none", "frame": 0, "part": "body", "tile_variant": 0,
                            "anchor": [0, 0], "box": [column * 20, 0, 20, 20], "source": source["path"],
                            "source_sha256": source["sha256"], "native_transform": source["native_transform"],
                            "registration_sha256": registration_signature(source)})
        path = OUTPUT / (asset_id.replace(".", "-") + "-authored.png")
        report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md78C; completed Plan13 ground78B style; archi-map-layers.md. Source-only, packing/runtime pending.",
                  "schema_version": 1, "asset_id": asset_id, "path": path.relative_to(base.ROOT).as_posix(),
                  "size": [60, 20], "native_cell": [20, 20], "anchor": [0, 0], "pose_count": 3, "sprites": sprites,
                  "source_record": RECORD.relative_to(base.ROOT).as_posix(), "source_coverage_complete": True, "runtime_activated": False}
        reports[asset_id], files[asset_id] = report, tube.indexed_png(atlas)
    return reports, files


def validate(reports, files, document=None, public=None):
    """Check actual decoded glyphs, exact state identities and immutable pixel ancestry.

    @param reports Both candidate family source mappings.
    @param files Exact encoded native atlas bytes keyed by family identity.
    @param document Optional isolated original record for refusal checks.
    @param public Optional isolated shared contract for refusal checks.
    @return None; validation creates no toolbar, packed sprite or gameplay availability.
    """
    if set(reports) != set(ASSETS) or set(files) != set(ASSETS):
        raise ValueError("incomplete feature control publication coverage")
    # Reuse the completed ground palette/state/anchor validator on actual
    # decoded new glyphs, then independently check exact original provenance.
    for asset_id in ASSETS:
        report = reports[asset_id]
        if (report.get("asset_id") != asset_id or report.get("pose_count") != 3
                or report.get("source_coverage_complete") is not True or report.get("runtime_activated") is not False):
            raise ValueError("feature control source falsely claims coverage or runtime")
        with Image.open(io.BytesIO(files[asset_id])) as image:
            if image.size != (60, 20):
                raise ValueError("feature control atlas size mismatch")
            controls.validate_glyphs(image, report)
    expected, original_pixels = build(document, public)
    if reports != expected:
        raise ValueError("feature control changes original state ancestry or anchors")
    if files != original_pixels:
        raise ValueError("feature control changes original native pixels")


def contact_board(reports, files):
    """Compare both source families at native size and exact enlarged pixels.

    @param reports Checked per-family three-state source mappings.
    @param files Actual indexed atlas bytes, decoded independently for review.
    @return Reproducible PNG review; source-only status is explicit.
    """
    page = Image.new("RGB", (1000, 430), (44, 47, 51))
    draw = ImageDraw.Draw(page)
    draw.text((16, 14), "TUBE / GEOTHERMAL CONTROL SOURCE / native20x20 enabled-disabled-pressed / packing and runtime pending", fill="white")
    for row, asset_id in enumerate(ASSETS):
        with Image.open(io.BytesIO(files[asset_id])) as image:
            glyphs = image.convert("RGBA")
        y = 60 + row * 180
        draw.text((16, y), asset_id, fill="white")
        page.paste(glyphs, (16, y + 30), glyphs)
        large = glyphs.resize((360, 120), Image.Resampling.NEAREST)
        page.paste(large, (150, y + 28), large)
        for index, state in enumerate(STATES):
            draw.text((150 + index * 120, y + 8), state, fill="white")
    stream = io.BytesIO()
    page.save(stream, format="PNG")
    return stream.getvalue()


def main():
    """Review one original or publish verified related control source artifacts.

    @return None; flags control only repository-local source/review/shared metadata.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate")
    parser.add_argument("--publish", action="store_true")
    parser.add_argument("--bind", action="store_true")
    options = parser.parse_args()
    if options.candidate:
        glyph, source = native_original(options.candidate)
        (OUTPUT / (options.candidate + "-native.png")).write_bytes(tube.indexed_png(glyph))
        rgba = glyph.convert("RGBA").resize((320, 320), Image.Resampling.NEAREST)
        page = Image.new("RGB", (320, 320), (44, 47, 51))
        page.paste(rgba, (0, 0), rgba)
        page.save(OUTPUT / (options.candidate + "-review.png"))
        print(json.dumps({"key": source["key"], "bounds": glyph.getbbox(), "registration": registration_signature(source)}))
        return
    reports, files = build()
    validate(reports, files)
    if options.publish:
        for asset_id, report in reports.items():
            base.repository_path(report["path"]).write_bytes(files[asset_id])
            (OUTPUT / (asset_id.replace(".", "-") + "-source-manifest.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        (OUTPUT / "feature-controls-contact.png").write_bytes(contact_board(reports, files))
    if options.bind:
        if not options.publish:
            raise ValueError("feature control source binding requires checked publication")
        public = json.loads(base.PUBLIC.read_text(encoding="utf-8"))
        # Only actual checked source coverage is published. Runtime toolbar
        # bindings retain their later feature producer and packed range owners.
        for asset_id, report in reports.items():
            public_family(asset_id, public)["source"].update(status="authored", path=report["path"],
                                                           sha256=hashlib.sha256(files[asset_id]).hexdigest(),
                                                           manifest=(OUTPUT / (asset_id.replace(".", "-") + "-source-manifest.json")).relative_to(base.ROOT).as_posix(),
                                                           source_coverage_complete=True, runtime_activated=False, rights="first_party_original_imagegen")
        base.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    print("Feature controls:2 families/6 authored native20x20 states; packing/runtime pending")


if __name__ == "__main__":
    main()
