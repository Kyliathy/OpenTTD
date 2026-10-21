"""Verify genuine launch-center base views and their stored native cell cuts.

@file launch_facility_view_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61DE.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Run directly to rebuild SE/SW/NW source proofs and the four-view review contact
under logs/proofs/mlm/native-art/space/launch-center-views-v1. The retained NE
fixture is immutable; this runner never replaces expected preservation hashes.
These source checks neither advance a game nor qualify actor/state foregrounds.
"""

from copy import deepcopy
import hashlib
import json
import unittest
from unittest.mock import patch

from PIL import Image, ImageDraw

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, read_json, verify_native_proof
from tools import launch_facility_base as base

PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-views-v1"
SOURCE = ROOT / "media/multilayer/source/space"
FIXTURE = ROOT / "src/tests/multilayer/launch-center-ne-base-fixture.json"
DIRECTIONS = ("SE", "SW", "NW")


def recipe_path(direction):
    """Select an explicit curated view recipe without latest-file discovery.

    @param direction Actual native quarter-turn name.
    @return Repository-local version-one composition recipe.
    """
    return SOURCE / f"launch-center-composed-base-{direction.lower()}-v1.json"


def write_json(path, document):
    """Persist reproducible evidence with stable readable serialization.

    @param path Explicit repository-local proof destination.
    @param document JSON-compatible source proof or metadata.
    @return None.
    """
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def rebuild_view(direction):
    """Rebuild original pixels, ownership planes and all twenty-four stored parts.

    @param direction Genuine selected source direction to compose.
    @return Complete parts document whose image identities bind stored PNGs.
    """
    recipe = read_json(recipe_path(direction))
    whole, owners, cells, evidence = base.compose(recipe)
    pieces = base.cut_cells(whole, cells, direction)
    directory = PROOFS / direction
    directory.mkdir(parents=True, exist_ok=True)
    whole.save(directory / "base-native.png")
    base.geometry_review(whole, direction).resize((1280, 896), Image.Resampling.NEAREST).save(directory / "base-geometry-review.png")
    (directory / "component-ownership.bin").write_bytes(owners)
    (directory / "cell-ownership.bin").write_bytes(cells)
    rows = []
    # Every actual cell retains both declared parts, including absent empty-
    # base fronts. Stored source proofs cannot silently lose logical positions.
    for row, image in pieces:
        path = directory / f"cell-{row['tile_variant']:02d}-{row['part']}.png"
        image.save(path)
        rows.append(dict(row, image=base.identity(path)))
    document = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61DE; genuine ready/empty source view only.",
                    recipe=base.identity(recipe_path(direction)), geometry=base.source.geometry(direction),
                    evidence=evidence, parts=rows,
                    whole=base.identity(directory / "base-native.png"),
                    component_ownership=base.identity(directory / "component-ownership.bin"),
                    cell_ownership=base.identity(directory / "cell-ownership.bin"),
                    review=base.identity(directory / "base-geometry-review.png"))
    write_json(directory / "parts.json", document)
    return document


def stored_pieces(document):
    """Read exact stored indexed PNGs after independently checking file identities.

    @param document Parts metadata rebuilt from one explicit source recipe.
    @return Metadata/image pairs suitable for independent cell reassembly.
    @throws ValueError When a stored artifact no longer matches its receipt.
    """
    result = []
    # Opening saved images checks palette and serialization, rather than only
    # comparing a cutter with the same in-memory objects it just produced.
    for row in document["parts"]:
        path = ROOT / row["image"]["path"]
        if base.identity(path) != row["image"]:
            raise ValueError("stored launch view part changed")
        with Image.open(path) as image:
            result.append((row, image.copy()))
    return result


def rebuild_contact():
    """Show the four genuine views together without authoring new source pixels.

    @return Exact identity of the labelled diagnostic contact PNG.
    """
    sheet = Image.new("RGB", (1280, 960), "#29323c")
    draw = ImageDraw.Draw(sheet)
    # Nearest-neighbor display preserves native readability. Floor outlines
    # and view labels are diagnostic only; native concrete remains game-owned.
    for index, direction in enumerate(("NE",) + DIRECTIONS):
        whole, owners, cells, evidence = base.compose(read_json(recipe_path(direction)))
        panel = base.geometry_review(whole, direction).resize((640, 448), Image.Resampling.NEAREST)
        x, y = index % 2 * 640, index // 2 * 480
        sheet.paste(panel, (x, y + 24))
        draw.text((x + 16, y + 8), f"{direction}: genuine source; diagnostic floor outlines", fill="white")
    path = PROOFS / "four-view-contact.png"
    sheet.save(path)
    return base.identity(path)


class LaunchViewChecks(unittest.TestCase):
    """Require genuine quarter turns, exact stored cuts and protected accepted NE art."""

    @classmethod
    def setUpClass(cls):
        """Load the bounded three-view fixture and immutable NE preservation contract.

        @param cls Current focused launch-view test class.
        @return None; retain independently saved parts for acceptance and refusals.
        """
        cls.fixture = read_json(FIXTURE)
        cls.views = {}
        # Each direction uses its own originals and native rectangular layout.
        # A transformed NE raster cannot satisfy these independent fixtures.
        for direction in DIRECTIONS:
            document = read_json(PROOFS / direction / "parts.json")
            cls.views[direction] = dict(document=document, recipe=read_json(recipe_path(direction)),
                                        pieces=stored_pieces(document),
                                        cells=(ROOT / document["cell_ownership"]["path"]).read_bytes())

    def test_actual_producers_and_twelve_cells_per_genuine_view(self):
        """Current compiled geometry qualifies all physical views and native ground.

        @param self Active source-to-native producer check.
        @return None.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            proof = read_json(NATIVE_PROOFS / f"native-{configuration}.json")
            verify_native_proof(proof)
            self.assertEqual(proof["assertions"], 305)
        # Every actual orientation needs twelve distinct physical cells and
        # twenty-four source positions, including both non-square layouts.
        for direction, view in self.views.items():
            native = base.source.geometry(direction)
            self.assertEqual(len(native["world_cells"]), 12)
            self.assertEqual(len({tuple(cell) for cell in native["world_cells"]}), 12)
            self.assertEqual(len(view["pieces"]), 24)
            self.assertTrue(all(view["document"]["evidence"]["pixels_per_cell"]))
            self.assertEqual(view["recipe"]["native_ground"], dict(owner="DrawTile_Station", sprite="SPR_CONCRETE_GROUND", cells=12))
            for field in ("source_coverage", "packed", "runtime"):
                self.assertFalse(view["recipe"][field])

    def test_deterministic_original_pixel_ancestry_and_normal_occlusion(self):
        """Every original pixel remains visible or counted in physical depth overlap.

        @param self Active deterministic composition and original ownership check.
        @return None.
        """
        # Recompose from immutable receipts, independently of saved artifacts,
        # so a changed registered original or depth order cannot inherit proof.
        for direction, view in self.views.items():
            whole, owners, cells, evidence = base.compose(view["recipe"])
            document = view["document"]
            self.assertEqual(evidence, document["evidence"])
            self.assertEqual(owners, (ROOT / document["component_ownership"]["path"]).read_bytes())
            self.assertEqual(cells, view["cells"])
            self.assertEqual(sum(evidence["source_pixels"].values()),
                             sum(evidence["visible_pixels"].values()) + sum(evidence["occlusions"].values()))
            self.assertEqual(sum(evidence["pixels_per_cell"]), sum(evidence["visible_pixels"].values()))
            self.assertTrue(all(evidence["visible_pixels"].values()))
            with Image.open(ROOT / document["whole"]["path"]) as stored:
                self.assertEqual(stored.tobytes(), whole.tobytes())
            for field in ("whole", "component_ownership", "cell_ownership", "review"):
                self.assertEqual(base.identity(ROOT / document[field]["path"]), document[field])

    def test_stored_parts_reassemble_exact_original_view_pixels(self):
        """Stored indexed parts retain actual view, layout, anchor and all source pixels.

        @param self Active serialized part reassembly check.
        @return None.
        """
        # Independent serialized views must each reconstruct their own source;
        # a successful NE round trip cannot stand in for another orientation.
        for direction, view in self.views.items():
            evidence = view["document"]["evidence"]
            whole = base.reassemble(view["pieces"], view["cells"], evidence["native_sha256"], direction)
            self.assertEqual(hashlib.sha256(whole.tobytes()).hexdigest(), evidence["native_sha256"])
            self.assertEqual(sum(row["structurally_absent"] for row, image in view["pieces"]), 12)

    def test_wrong_view_depth_ground_or_model_receipts_refuse(self):
        """Relabelled pixels and changed source authority cannot qualify a genuine view.

        @param self Active single-policy recipe corruption check.
        @return None.
        """
        # One mutation at a time exposes missing guards instead of relying on
        # a second corrupt field to make an invalid source recipe fail.
        for direction, view in self.views.items():
            mutations = dict(directions=["NE"], state="working", frame=1,
                             composition_order=list(reversed(base.DEPTH_ORDERS[direction])),
                             foreground_policy="all_actor_fronts_approved",
                             native_ground=dict(owner="art", sprite="synthetic", cells=16),
                             component_model=dict(view["recipe"]["component_model"], sha256="0" * 64))
            for field, value in mutations.items():
                with self.subTest(direction=direction, field=field):
                    broken = deepcopy(view["recipe"])
                    broken[field] = value
                    with self.assertRaises(ValueError):
                        base.compose(broken)

    def test_replaced_component_version_or_physical_turn_refuses(self):
        """A valid original from another selection cannot masquerade as the frozen model.

        @param self Active selected receipt/version and physical turn refusal check.
        @return None.
        """
        original_reader = base.bound_json
        # Intercept one already hash-qualified document to test downstream
        # semantic guards independently of the earlier SHA256 refusal.
        for direction, view in self.views.items():
            model = original_reader(view["recipe"]["component_model"])
            selected = model["selected_components"][direction][base.DEPTH_ORDERS[direction][0]]
            for field in ("key", "physical_view"):
                candidate = deepcopy(original_reader(selected["generation_receipt"]))
                if field == "key":
                    candidate[field] += "-unselected"
                else:
                    candidate[field]["native_rotation"] = 0

                def replace_receipt(reference):
                    """Expose one semantically corrupt receipt behind a valid outer identity.

                    @param reference Explicit bound source-document identity.
                    @return Mutated selected receipt or the untouched real input.
                    """
                    if reference == selected["generation_receipt"]:
                        return candidate
                    return original_reader(reference)

                with self.subTest(direction=direction, field=field), patch.object(base, "bound_json", side_effect=replace_receipt):
                    with self.assertRaises(ValueError):
                        base.compose(view["recipe"])

    def test_missing_duplicate_or_relabelled_stored_parts_refuse(self):
        """Coverage and native geometry must agree for every actual cell part.

        @param self Active stored-position and native anchor corruption check.
        @return None.
        """
        # Coverage and native placement are required independently in each
        # rectangular layout, including views with similar-looking hardware.
        for direction, view in self.views.items():
            pieces, cells = view["pieces"], view["cells"]
            expected = view["document"]["evidence"]["native_sha256"]
            for broken in (pieces[:-1], pieces[:-1] + [pieces[0]]):
                with self.assertRaises(ValueError):
                    base.reassemble(broken, cells, expected, direction)
            mutations = dict(direction="NE", native_layout=0, world_cell=[9, 9],
                             anchor=[-30, -33], native_origin=[999, 999], state="working", frame=1)
            # Relabelling a PNG cannot move native seams, change the frame or
            # turn another view's cell into a valid source position.
            for field, value in mutations.items():
                broken = deepcopy(pieces)
                broken[0][0][field] = value
                with self.subTest(direction=direction, field=field), self.assertRaises(ValueError):
                    base.reassemble(broken, cells, expected, direction)

    def test_rehashed_pixels_false_fronts_and_changed_ownership_refuse(self):
        """Fresh checksums cannot bless altered machinery or fabricated actor foregrounds.

        @param self Active pixel, palette and actual-zone ownership refusal check.
        @return None.
        """
        # A locally rehashed corruption must fail for every genuine view;
        # palette, original pixel identity and actual cell sorting stay coupled.
        for direction, view in self.views.items():
            expected = view["document"]["evidence"]["native_sha256"]
            broken = deepcopy(view["pieces"])
            row, image = broken[0]
            point = next((x, y) for y in range(64) for x in range(64) if image.getpixel((x, y)))
            image.putpixel(point, 1 if image.getpixel(point) != 1 else 2)
            row["native_sha256"] = hashlib.sha256(image.tobytes()).hexdigest()
            with self.assertRaises(ValueError):
                base.reassemble(broken, view["cells"], expected, direction)
            broken = deepcopy(view["pieces"])
            row, image = broken[1]
            image.putpixel((31, 48), 1)
            row.update(opaque_pixels=1, structurally_absent=False, native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
            with self.assertRaises(ValueError):
                base.reassemble(broken, view["cells"], expected, direction)
            broken = deepcopy(view["pieces"])
            palette = broken[0][1].getpalette()
            palette[0] ^= 1
            broken[0][1].putpalette(palette)
            with self.assertRaises(ValueError):
                base.reassemble(broken, view["cells"], expected, direction)
            cells = bytearray(view["cells"])
            position = next(index for index, value in enumerate(cells) if value)
            cells[position] = 13
            with self.assertRaises(ValueError):
                base.reassemble(view["pieces"], bytes(cells), expected, direction)

    def test_accepted_ne_recipe_pixels_parts_and_models_remain_exact(self):
        """Shared converter changes preserve the immutable accepted NE fixture exactly.

        @param self Active earlier-view preservation check.
        @return None.
        """
        fixture = self.fixture
        for field in ("recipe", "component_model", "rotation_model"):
            self.assertEqual(base.identity(ROOT / fixture[field]["path"]), fixture[field])
        # The expected identities were captured before converter extension.
        # Rebuilding output never changes the preservation contract itself.
        for artifact in fixture["artifacts"]:
            self.assertEqual(base.identity(ROOT / artifact["path"]), artifact)
        whole, owners, cells, evidence = base.compose(base.bound_json(fixture["recipe"]))
        self.assertEqual(evidence, fixture["evidence"])
        pieces = base.cut_cells(whole, cells)
        rebuilt = base.reassemble(pieces, cells, evidence["native_sha256"])
        self.assertEqual(rebuilt.tobytes(), whole.tobytes())

    def test_reviewed_view_selection_binds_real_sources_without_state_claims(self):
        """Four reviewed bases cannot authorize unmade state or actor foreground art.

        @param self Active source-family handoff and selection boundary check.
        @return None.
        """
        selection_path = SOURCE / "launch-center-base-views-v1.json"
        selection = read_json(selection_path)
        family = read_json(SOURCE / "space-generation-record.json")["families"][base.source.ASSET]
        self.assertEqual(family["selected_views"], base.identity(selection_path))
        self.assertEqual(family["selected_base"], self.fixture["recipe"])
        self.assertEqual(selection["status"], "base_views_selected_states_pending")
        self.assertEqual(selection["model_review"]["status"], "approved_neighboring_views")
        self.assertTrue(selection["base_views_complete"])
        self.assertEqual(set(selection["selected_views"]), {"NE", "SE", "SW", "NW"})
        # Every frozen source pose needs its own exact complete-base review.
        # Partial state coverage remains explicit despite all four ready views.
        for direction, reference in selection["selected_views"].items():
            recipe = base.bound_json(reference)
            self.assertEqual(reference, base.identity(recipe_path(direction)))
            self.assertEqual(recipe["directions"], [direction])
            self.assertEqual(recipe["status"], "selected_base")
            self.assertTrue(recipe["base_selected"])
            self.assertEqual(recipe["model_review"]["status"], "approved_complete_base")
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(selection[field])
        public = read_json(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json")
        row = next(row for row in public["assets"] if row["asset_id"] == base.source.ASSET)
        if family["source_coverage"]:
            # A later complete-source handoff may advance this same family;
            # it never changes the earlier four-view selection checkpoint.
            complete = base.bound_json(family["source_manifest"])
            self.assertTrue(complete["coverage_complete"])
            self.assertEqual(complete["logical_pose_count"], 1536)
            self.assertEqual(row["source"]["status"], "authored")
        else:
            self.assertEqual(row["source"]["status"], "planned")
        self.assertIsNone(row["packed"]["offset"])


def main():
    """Rebuild bounded current-group proofs, then verify actual saved artifacts.

    @return None; exit nonzero without publishing a passing proof on failure.
    """
    PROOFS.mkdir(parents=True, exist_ok=True)
    # These three genuine views are the active task group; the accepted NE
    # output is read-only and checked against its curated preservation fixture.
    for direction in DIRECTIONS:
        rebuild_view(direction)
    contact = rebuild_contact()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchViewChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61DE; base views only, no actual state/actor or runtime acceptance.",
                 status="passed", cases=result.testsRun, contact=contact,
                 views={direction: base.identity(PROOFS / direction / "parts.json") for direction in DIRECTIONS},
                 recipes={direction: base.identity(recipe_path(direction)) for direction in ("NE",) + DIRECTIONS},
                 ne_fixture=base.identity(FIXTURE),
                 selection=base.identity(SOURCE / "launch-center-base-views-v1.json"),
                 native_producers={configuration: base.identity(NATIVE_PROOFS / f"native-{configuration}.json")
                                   for configuration in ("RelWithDebInfo", "Debug")},
                 helper=base.identity(ROOT / "media/multilayer/tools/launch_facility_base.py"),
                 test=base.identity(ROOT / "src/tests/multilayer/launch_facility_view_checks.py"),
                 source_coverage=False, packed=False, runtime=False)
    write_json(PROOFS / "verification.json", proof)


if __name__ == "__main__":
    main()
