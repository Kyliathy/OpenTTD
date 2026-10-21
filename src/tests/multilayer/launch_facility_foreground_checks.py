"""Review and verify original occupied launch-center cell/back/front partitions.

@file launch_facility_foreground_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
--review rebuilds diagnostics without source acceptance. Normal execution
requires the immutable reviewed foreground fixture and verifies actual stored
PNGs. Outputs live under logs/proofs/mlm/native-art/space/launch-center-foreground-v1/cuts.
Diagnostic model15 silhouettes are separate source references, never game actors
or cargo; whole facility source pixels and all native owners remain unchanged.
"""

import argparse
from copy import deepcopy
import hashlib
import json
import unittest

from PIL import Image, ImageDraw

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, read_json, verify_native_proof
from launch_facility_state_checks import checked_native, PROOFS as STATE_PROOFS
from tools import launch_facility_foregrounds as foreground

base = foreground.base
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-center-foreground-v1/cuts"
FIXTURE = ROOT / "src/tests/multilayer/launch-center-foreground-fixture.json"
WHOLE_FIXTURE = ROOT / "src/tests/multilayer/launch-center-source-fixture.json"


def write_json(path, document):
    """Save explicit original-source proof metadata with stable readable serialization.

    @param path Owned repository-local output path.
    @param document JSON-compatible source proof.
    @return None.
    """
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def colour_plane(image):
    """Display transparent indexed source art over a diagnostic gray background.

    @param image Original indexed source or source-part canvas.
    @return RGB diagnostic, preserving every nontransparent source pixel colour.
    """
    mask = Image.new("L", image.size)
    mask.putdata([255 if value else 0 for value in image.get_flattened_data()])
    result = Image.new("RGB", image.size, "#29323c")
    result.paste(image.convert("RGB"), (0, 0), mask)
    return result


def part_plane(whole, foreground_map, front):
    """Observe one original part without painting, moving or changing source alpha.

    @param whole Accepted whole state image.
    @param foreground_map Reviewed original per-pixel part ownership.
    @param front Whether to inspect original foreground rather than background pixels.
    @return Same-sized indexed part plane.
    """
    result = base.tiles.indexed_canvas(whole.size)
    # Pixel selection is a lossless partition; every omitted pixel remains in
    # the opposite part and exact whole reassembly is checked independently.
    result.putdata([value if bool(foreground_map[index]) == front else 0
                    for index, value in enumerate(whole.get_flattened_data())])
    return result


def reference_actor(model, direction):
    """Read one exact genuine separate model15 service silhouette for diagnostics.

    @param model Source foreground brief with explicit reference-only ancestry.
    @param direction Actual existing service source heading.
    @return Indexed64x48 reference silhouette, never a facility source component.
    """
    reference = model["diagnostic_actor_reference"]
    report = base.bound_json(reference["source_manifest"])
    atlas = next(row for row in report["atlases"] if row["asset_id"] == reference["asset_id"])
    if {key: atlas[key] for key in ("path", "sha256")} != reference["atlas"]:
        raise ValueError("launch diagnostic reference atlas changed")
    path = ROOT / atlas["path"]
    if base.identity(path) != reference["atlas"]:
        raise ValueError("launch diagnostic reference pixels changed")
    # Select the same genuine existing service pose in this real heading;
    # reference reuse grants no new facility state or synthesized camera view.
    row = next(row for row in atlas["sprites"] if row["state"] == reference["state"]
               and row["frame"] == reference["frame"] and row["direction"] == direction)
    x, y, width, height = row["box"]
    with Image.open(path) as image:
        return image.crop((x, y, x + width, y + height))


def occupied_review(model, direction, fixture, component, actor):
    """Insert a reference silhouette inside one zone's original physical depth order.

    @param model Exact geometry/contact brief under review.
    @param direction Actual physical facility view.
    @param fixture Original whole image, component owners and reviewed part mask.
    @param component Actual pad/workshop/recovery/storage visitor zone.
    @param actor Separate genuine reference-only silhouette.
    @return Diagnostic RGB scene and measured reference pixel visibility/occlusion.
    """
    whole, owners, mask = fixture["images"]["ready"], fixture["owners"], fixture["foreground"]
    contact = model["views"][direction]["contacts"][component]["physical_display_contact"]
    ox, oy = contact[0] - 32, contact[1] - 32
    scene = base.tiles.indexed_canvas(whole.size)
    reference_visible = set()
    total = sum(value != 0 for value in actor.get_flattened_data())
    order = base.DEPTH_ORDERS[direction]
    # Component depth is the already accepted whole-model order. The visitor
    # is inserted only between its own zone's back/front; other near machinery
    # retains ordinary physical occlusion instead of a global front overlay.
    for owner, name in enumerate(order, 1):
        passes = (False, True) if name == component else (None,)
        for front in passes:
            if front is True:
                # The reference is source-only; compositing never registers a
                # native hull, reveals Hidden state or assigns paid inventory.
                for y in range(actor.height):
                    for x in range(actor.width):
                        value = actor.getpixel((x, y))
                        if value:
                            point = (ox + x, oy + y)
                            scene.putpixel(point, value)
                            reference_visible.add(point)
            for index, value in enumerate(whole.get_flattened_data()):
                if owners[index] != owner:
                    continue
                if front is not None and bool(mask[index]) != front:
                    continue
                point = (index % whole.width, index // whole.width)
                scene.putpixel(point, value)
                reference_visible.discard(point)
    return colour_plane(scene), dict(reference_pixels=total, visible_pixels=len(reference_visible),
                                    occluded_pixels=total - len(reference_visible))


def rebuild(model):
    """Rebuild stored occupied parts, original-part contacts and separate-reference diagrams.

    @param model Explicit physical partition/contact brief.
    @return Per-view fixtures for independent stored-source tests.
    """
    OUTPUT.mkdir(parents=True, exist_ok=True)
    layers = Image.new("RGB", (1920, 1920), "#29323c")
    visitors = Image.new("RGB", (1280, 3968), "#29323c")
    layer_labels = ImageDraw.Draw(layers)
    fixtures = {}
    # Each genuine view is recomposed from retained original images. No
    # previous debug PNG or workstation-only clipping mask supplies source art.
    for view_index, direction in enumerate(base.DEPTH_ORDERS):
        images, owners, cells, mask, evidence = foreground.compose_states(model, direction)
        fixture = dict(images=images, owners=owners, cells=cells, foreground=mask, evidence=evidence, documents={})
        directory = OUTPUT / direction
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "foreground.bin").write_bytes(mask)
        (directory / "cell-ownership.bin").write_bytes(cells)
        (directory / "component-ownership.bin").write_bytes(owners)
        for state in ("ready", "working", "blocked"):
            whole = images[state]
            whole.save(directory / f"{state}-whole.png")
            rows = []
            # Actual stored PNGs retain all24 source positions, including
            # explicit front-only or back-only cells and unchanged still poses.
            for row, image in foreground.cut_state(whole, cells, mask, direction, state):
                path = directory / f"{state}-cell-{row['tile_variant']:02d}-{row['part']}.png"
                image.save(path)
                rows.append(dict(row, image=base.identity(path)))
            document = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA; original occupied-source parts only.",
                            model=base.identity(foreground.MODEL), state=state, direction=direction,
                            native_sha256=evidence["state_sha256"][state], parts=rows)
            write_json(directory / f"{state}-parts.json", document)
            fixture["documents"][state] = document
        write_json(directory / "source.json", dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA; actual original ownership and foreground evidence.",
                                                   model=base.identity(foreground.MODEL), evidence=evidence))
        for column, (label, image) in enumerate((("whole", images["ready"]),
                                                ("back", part_plane(images["ready"], mask, False)),
                                                ("front", part_plane(images["ready"], mask, True)))):
            x, y = column * 640, view_index * 480
            layers.paste(colour_plane(image).resize((640, 448), Image.Resampling.NEAREST), (x, y + 24))
            layer_labels.text((x + 8, y + 8), f"{direction} {label}: exact original pixels", fill="white")
        actor = reference_actor(model, direction)
        view_visitors = Image.new("RGB", (1280, 992), "#29323c")
        view_labels = ImageDraw.Draw(view_visitors)
        fixture["reference_reviews"] = {}
        for component_index, component in enumerate(("pad", "workshop", "recovery", "storage")):
            panel, observation = occupied_review(model, direction, fixture, component, actor)
            fixture["reference_reviews"][component] = observation
            x, y = component_index % 2 * 640, component_index // 2 * 496
            view_visitors.paste(panel.resize((640, 448), Image.Resampling.NEAREST), (x, y + 40))
            view_labels.text((x + 6, y + 8), f"{direction} {component}: separate model15 reference only; no native scene", fill="white")
            view_labels.text((x + 6, y + 23), f"reference visible {observation['visible_pixels']} / occluded {observation['occluded_pixels']}", fill="white")
        view_visitors.save(directory / "occupied-reference.png")
        visitors.paste(view_visitors, (0, view_index * 992))
        fixtures[direction] = fixture
    layers.save(OUTPUT / "original-layer-contact.png")
    visitors.save(OUTPUT / "occupied-reference-contact.png")
    write_json(OUTPUT / "review.json", dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA; source-only physical review, never gameplay acceptance.",
                                            model=base.identity(foreground.MODEL),
                                            views={direction: dict(foreground_sha256=fixture["evidence"]["foreground_sha256"],
                                                                   foreground_pixels=fixture["evidence"]["foreground_pixels"],
                                                                   retained_interiors=fixture["evidence"]["retained_interiors"],
                                                                   reference_reviews=fixture["reference_reviews"])
                                                   for direction, fixture in fixtures.items()},
                                            contacts=[base.identity(OUTPUT / name) for name in ("original-layer-contact.png", "occupied-reference-contact.png")]))
    return fixtures


def stored_parts(document):
    """Read exact independently saved source parts after verifying their file identities.

    @param document One state/view source part receipt.
    @return Independent stored indexed metadata/image pairs for isolated checks.
    """
    result = []
    for row in document["parts"]:
        path = ROOT / row["image"]["path"]
        if base.identity(path) != row["image"]:
            raise ValueError("stored launch foreground part changed")
        with Image.open(path) as image:
            result.append((deepcopy(row), image.copy()))
    return result


class LaunchForegroundChecks(unittest.TestCase):
    """Require physical occupied cuts, exact source pixels and original native ownership."""

    fixtures = {}

    @classmethod
    def setUpClass(cls):
        """Load reviewed immutable cut expectations and the earlier whole-pixel fixture.

        @param cls Current bounded occupied-source test class.
        @return None.
        """
        cls.model = read_json(foreground.MODEL)
        cls.expected = read_json(FIXTURE)
        cls.wholes = read_json(WHOLE_FIXTURE)["approved_whole_pixels"]

    def test_actual_native_state_geometry_and_selected_whole_pixels_remain_exact(self):
        """Occupied partitioning consumes current producers and never changes accepted artwork.

        @param self Active native producer and original whole-pixel preservation check.
        @return None.
        """
        mapping = base.bound_json(self.model["state_mapping"])
        for configuration in ("Debug", "RelWithDebInfo"):
            verify_native_proof(read_json(NATIVE_PROOFS / f"native-{configuration}.json"))
            checked_native(read_json(STATE_PROOFS / f"native-{configuration}.json"), mapping)
        # All16 accepted state/view whole hashes remain the pre-cut values.
        # Foreground qualification changes ownership only, never source pixels.
        for direction, fixture in self.fixtures.items():
            self.assertEqual(fixture["evidence"]["state_sha256"], self.wholes[direction])

    def test_reviewed_physical_foregrounds_and_open_interiors_are_exact(self):
        """Every genuine view retains the reviewed structural mask and actual opening roles.

        @param self Active immutable physical cut and source cavity ownership check.
        @return None.
        """
        self.assertEqual(base.identity(foreground.MODEL), self.expected["model"])
        # Expected masks were frozen after source/occupied visual review.
        # Rebuilding artifacts must never rewrite these acceptance values.
        for direction, fixture in self.fixtures.items():
            expected = self.expected["views"][direction]
            evidence = fixture["evidence"]
            self.assertEqual(evidence["foreground_sha256"], expected["foreground_sha256"])
            self.assertEqual(evidence["foreground_pixels"], expected["foreground_pixels"])
            self.assertEqual(evidence["retained_interiors"], expected["retained_interiors"])
            self.assertGreater(evidence["foreground_pixels"]["workshop"], 0)
            self.assertGreater(evidence["foreground_pixels"]["storage"], 0)
            self.assertEqual(evidence["foreground_pixels"]["recovery"], 0)
            if direction in ("NE", "NW"):
                self.assertGreater(evidence["foreground_pixels"]["pad"], 0)
                self.assertGreater(evidence["retained_interiors"]["workshop"], 0)
                self.assertGreater(evidence["retained_interiors"]["storage"], 0)
            else:
                self.assertEqual(evidence["foreground_pixels"]["pad"], 0)
                self.assertEqual(evidence["retained_interiors"]["workshop"], 0)

    def test_all_saved_source_parts_reassemble_unchanged_original_states(self):
        """Saved indexed back/front PNGs reproduce every original state with actual anchors.

        @param self Active independent stored occupied-source round-trip check.
        @return None.
        """
        for direction, fixture in self.fixtures.items():
            for state, document in fixture["documents"].items():
                pieces = stored_parts(document)
                whole = foreground.reassemble_state(pieces, fixture["cells"], fixture["foreground"],
                                                     self.wholes[direction][state], direction, state)
                self.assertEqual(whole.tobytes(), fixture["images"][state].tobytes())
                self.assertEqual(sum(row["opaque_pixels"] for row, image in pieces),
                                 sum(value != 0 for value in whole.get_flattened_data()))

    def test_moved_source_pixel_between_parts_refuses_even_after_local_rehash(self):
        """Lossless whole pixels alone cannot authorize a physically wrong occupied cut.

        @param self Active independent physical part-ownership corruption check.
        @return None.
        """
        fixture = self.fixtures["NE"]
        pieces = stored_parts(fixture["documents"]["ready"])
        row, image = next((row, image) for row, image in pieces if row["part"] == "front" and row["opaque_pixels"])
        point = next((x, y) for y in range(64) for x in range(64) if image.getpixel((x, y)))
        other, back = next((r, i) for r, i in pieces if r["part"] == "back" and r["tile_variant"] == row["tile_variant"])
        back.putpixel(point, image.getpixel(point))
        image.putpixel(point, 0)
        # Retain exactly the same reconstructed whole and rehash both files.
        # Only the independent accepted foreground map exposes the wrong cut.
        for metadata, part in ((row, image), (other, back)):
            count = sum(value != 0 for value in part.get_flattened_data())
            metadata.update(opaque_pixels=count, structurally_absent=count == 0, native_sha256=hashlib.sha256(part.tobytes()).hexdigest())
        with self.assertRaises(ValueError):
            foreground.reassemble_state(pieces, fixture["cells"], fixture["foreground"], self.wholes["NE"]["ready"], "NE", "ready")

    def test_still_aliases_and_extra_rehashed_pixels_keep_original_ownership(self):
        """Still aliases retain physical cuts while extra locally rehashed art refuses.

        @param self Active construction/frame alias and invented source-pixel refusal check.
        @return None.
        """
        fixture = self.fixtures["NE"]
        pieces = foreground.cut_state(fixture["images"]["ready"], fixture["cells"], fixture["foreground"], "NE", "construction", 3)
        whole = foreground.reassemble_state(pieces, fixture["cells"], fixture["foreground"], self.wholes["NE"]["ready"], "NE", "construction", 3)
        self.assertEqual(whole.tobytes(), fixture["images"]["ready"].tobytes())
        actual = stored_parts(fixture["documents"]["ready"])
        self.assertEqual([row["native_sha256"] for row, image in pieces], [row["native_sha256"] for row, image in actual])
        row, image = actual[0]
        point = next((x, y) for y in range(64) for x in range(64) if not image.getpixel((x, y)))
        image.putpixel(point, 1)
        count = sum(value != 0 for value in image.get_flattened_data())
        row.update(opaque_pixels=count, structurally_absent=False, native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
        with self.assertRaises(ValueError):
            foreground.reassemble_state(actual, fixture["cells"], fixture["foreground"], self.wholes["NE"]["ready"], "NE", "ready")

    def test_missing_relabelled_or_reanchored_parts_and_bad_maps_refuse(self):
        """Exact native contacts, zones and source positions outrank plausible part counts.

        @param self Active source coverage/pose/anchor and ownership refusal check.
        @return None.
        """
        fixture = self.fixtures["NE"]
        pieces = stored_parts(fixture["documents"]["ready"])
        for broken in (pieces[:-1], pieces[:-1] + [pieces[0]]):
            with self.assertRaises(ValueError):
                foreground.reassemble_state(broken, fixture["cells"], fixture["foreground"], self.wholes["NE"]["ready"], "NE", "ready")
        # Mutate each native fact separately; another invalid field must not
        # conceal a missing geometry or actual source-ownership guard.
        for field, value in dict(anchor=[0, 0], direction="SE", native_origin=[999, 999], state="working", frame=1).items():
            broken = deepcopy(pieces)
            broken[0][0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                foreground.reassemble_state(broken, fixture["cells"], fixture["foreground"], self.wholes["NE"]["ready"], "NE", "ready")
        mask = bytearray(fixture["foreground"])
        position = next(index for index, value in enumerate(mask) if value)
        mask[position] = 0
        with self.assertRaises(ValueError):
            foreground.reassemble_state(pieces, fixture["cells"], bytes(mask), self.wholes["NE"]["ready"], "NE", "ready")
        cells = bytearray(fixture["cells"])
        cells[position] = 13
        with self.assertRaises(ValueError):
            foreground.reassemble_state(pieces, bytes(cells), fixture["foreground"], self.wholes["NE"]["ready"], "NE", "ready")

    def test_changed_native_mounting_or_false_physical_regions_refuse(self):
        """A source hash cannot legitimize a moved native contact or invented rear doorway.

        @param self Active physical source/model semantic corruption check.
        @return None.
        """
        for mutation in ("custody", "contact", "rear_door", "far_arm"):
            model = deepcopy(self.model)
            if mutation == "custody":
                model["views"]["NE"]["contacts"]["pad"]["native_custody_world_cell"] = [1, 1]
            elif mutation == "contact":
                contact = model["views"]["NE"]["contacts"]["workshop"]
                contact.update(physical_display_contact=[145, 112], display_offset_from_native_custody=[1, 16],
                               declared_actor_canvas_box=[113, 80, 177, 128])
            elif mutation == "rear_door":
                model["views"]["SW"]["foreground_regions"]["workshop"]["interior_polygons"] = [[[1, 1], [2, 1], [2, 2]]]
            else:
                model["views"]["SE"]["foreground_regions"]["pad"]["selection"] = "near_arm"
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                foreground.checked_model(model)


def main():
    """Rebuild current occupied source diagnostics, then run only this group's checks.

    @return None; --review leaves all completion/publication claims unchanged.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", help="Rebuild diagrams without occupied-source acceptance.")
    arguments = parser.parse_args()
    model = read_json(foreground.MODEL)
    fixtures = rebuild(model)
    if arguments.review:
        return
    LaunchForegroundChecks.fixtures = fixtures
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchForegroundChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA; occupied original-source cuts, separate from source closure/packing/game display.",
                 status="passed", cases=result.testsRun, model=base.identity(foreground.MODEL), fixture=base.identity(FIXTURE),
                 test=base.identity(ROOT / "src/tests/multilayer/launch_facility_foreground_checks.py"),
                 helper=base.identity(ROOT / "media/multilayer/tools/launch_facility_foregrounds.py"),
                 review=base.identity(OUTPUT / "review.json"), occupied_foregrounds_complete=True,
                 source_coverage=False, packed=False, runtime=False)
    write_json(OUTPUT / "verification.json", proof)


if __name__ == "__main__":
    main()
