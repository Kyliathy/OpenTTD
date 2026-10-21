"""Focused complete launch-center NE base composition and native cell cuts.

@file launch_facility_base_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61CC.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Run directly to rebuild source-only previews, twenty-four cell parts and exact
verification under logs/proofs/mlm/native-art/space/launch-center-composed-base-v1.
Existing native producer receipts are checked read-only; no game is advanced.
"""

from copy import deepcopy
import hashlib
import json
import unittest

from PIL import Image

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, read_json, verify_native_proof
from tools import launch_facility_base as base

RECIPE = ROOT / "media/multilayer/source/space/launch-center-composed-base-ne-v1.json"
PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-composed-base-v1"


class LaunchBaseChecks(unittest.TestCase):
    """Require complete original ancestry, actual zone sorting and lossless base cuts."""

    @classmethod
    def setUpClass(cls):
        """Compose one bounded fixture from the exact four selected originals.

        @param cls Focused source-composition test class.
        @return None; retain original planes' ownership and stored cell parts.
        """
        cls.recipe = read_json(RECIPE)
        cls.whole, cls.owners, cls.cells, cls.evidence = base.compose(cls.recipe)
        cls.pieces = base.cut_cells(cls.whole, cls.cells)

    def test_native_producers_and_exact_twelve_cell_ground_owner(self):
        """Both compiled producers qualify the actual grid and existing ground owner.

        @param self Active consumer-to-native producer boundary check.
        @return None.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            proof = read_json(NATIVE_PROOFS / f"native-{configuration}.json")
            verify_native_proof(proof)
            self.assertEqual(proof["assertions"], 305)
        self.assertEqual(self.recipe["native_ground"], dict(owner="DrawTile_Station", sprite="SPR_CONCRETE_GROUND", cells=12))
        self.assertEqual(len(base.source.geometry("NE")["world_cells"]), 12)
        self.assertTrue(all(self.evidence["pixels_per_cell"]))
        self.assertFalse(self.recipe["source_coverage"])
        self.assertFalse(self.recipe["packed"])
        self.assertFalse(self.recipe["runtime"])

    def test_composition_reproduces_and_accounts_for_all_original_pixels(self):
        """Every retained source pixel is visible or explicitly occluded by real hardware.

        @param self Active deterministic composition and depth accounting check.
        @return None.
        """
        whole, owners, cells, evidence = base.compose(self.recipe)
        self.assertEqual(whole.tobytes(), self.whole.tobytes())
        self.assertEqual((owners, cells, evidence), (self.owners, self.cells, self.evidence))
        self.assertEqual(sum(evidence["source_pixels"].values()),
                         sum(evidence["visible_pixels"].values()) + sum(evidence["occlusions"].values()))
        self.assertEqual(sum(evidence["visible_pixels"].values()), sum(evidence["pixels_per_cell"]))
        self.assertTrue(all(evidence["visible_pixels"].values()))

    def test_stored_twenty_four_parts_reassemble_exact_original_pixels(self):
        """Stored PNGs reconstruct the visible base without seam drift or duplication.

        @param self Active stored cell-artifact and round-trip check.
        @return None.
        """
        document = read_json(PROOFS / "parts.json")
        stored = []
        # Read the actual output PNGs, not only the in-memory cutter result;
        # this verifies that saved palette/anchors/parts survive serialization.
        for row in document["parts"]:
            self.assertEqual(base.identity(ROOT / row["image"]["path"]), row["image"])
            with Image.open(ROOT / row["image"]["path"]) as image:
                stored.append((row, image.copy()))
        rebuilt = base.reassemble(stored, self.cells, self.evidence["native_sha256"])
        self.assertEqual(rebuilt.tobytes(), self.whole.tobytes())
        self.assertEqual(len(stored), 24)
        self.assertEqual(sum(row["part"] == "front" and row["structurally_absent"] for row, image in stored), 12)

    def test_changed_pose_depth_ground_or_model_identity_refuses(self):
        """A different view, frame, depth order or producer cannot inherit this base.

        @param self Active independent recipe ownership/refusal check.
        @return None.
        """
        mutations = {"directions": ["SE"], "state": "working", "frame": 1,
                     "composition_order": list(reversed(base.ORDER)), "foreground_policy": "all_actor_fronts_approved",
                     "native_ground": dict(owner="art", sprite="synthetic", cells=16),
                     "component_model": dict(self.recipe["component_model"], sha256="0" * 64)}
        # Corrupt one policy at a time while preserving every other valid
        # input, so another failure cannot conceal an omitted producer guard.
        for field, value in mutations.items():
            with self.subTest(field=field):
                broken = deepcopy(self.recipe)
                broken[field] = value
                with self.assertRaises(ValueError):
                    base.compose(broken)

    def test_missing_duplicate_shifted_or_wrong_view_parts_refuse(self):
        """All twenty-four actual source positions retain native pose and anchor.

        @param self Active part coverage and stored metadata refusal check.
        @return None.
        """
        for broken in (self.pieces[:-1], self.pieces[:-1] + [self.pieces[0]]):
            with self.assertRaises(ValueError):
                base.reassemble(broken, self.cells, self.evidence["native_sha256"])
        mutations = dict(direction="SE", native_layout=1, world_cell=[9, 9], anchor=[-30, -33],
                         native_origin=[146, 31], opaque_pixels=0, structurally_absent=True)
        # Each metadata fact must agree with native geometry and the stored
        # pixels; relabeling cannot make a displaced source cut acceptable.
        for field, value in mutations.items():
            with self.subTest(field=field):
                broken = deepcopy(self.pieces)
                broken[0][0][field] = value
                with self.assertRaises(ValueError):
                    base.reassemble(broken, self.cells, self.evidence["native_sha256"])

    def test_rehashed_pixel_drift_and_invented_actor_foreground_refuse(self):
        """A fresh local checksum cannot authorize changed art or a fabricated front.

        @param self Active whole-source and foreground ownership refusal check.
        @return None.
        """
        broken = deepcopy(self.pieces)
        row, image = broken[0]
        point = next((x, y) for y in range(64) for x in range(64) if image.getpixel((x, y)))
        image.putpixel(point, 1 if image.getpixel(point) != 1 else 2)
        row["native_sha256"] = hashlib.sha256(image.tobytes()).hexdigest()
        with self.assertRaises(ValueError):
            base.reassemble(broken, self.cells, self.evidence["native_sha256"])
        broken = deepcopy(self.pieces)
        row, image = broken[1]
        image.putpixel((31, 48), 1)
        row.update(opaque_pixels=1, structurally_absent=False, native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
        with self.assertRaises(ValueError):
            base.reassemble(broken, self.cells, self.evidence["native_sha256"])

    def test_bad_cell_ownership_and_canvas_refuse(self):
        """Invalid or absent cell ownership cannot trim a genuine source pixel.

        @param self Active whole/cell ownership and palette refusal check.
        @return None.
        """
        position = next(i for i, value in enumerate(self.cells) if value)
        for value in (0, 13):
            broken = bytearray(self.cells)
            broken[position] = value
            with self.assertRaises(ValueError):
                base.cut_cells(self.whole, bytes(broken))
        with self.assertRaises(ValueError):
            base.cut_cells(self.whole.convert("RGB"), self.cells)
        with self.assertRaises(ValueError):
            base.cut_cells(self.whole, self.cells[:-1])


def main():
    """Rebuild the bounded complete-base artifacts, then verify stored outputs.

    @return None; exit nonzero on a failed source/native-cell acceptance check.
    """
    recipe = read_json(RECIPE)
    whole, owners, cells, evidence = base.compose(recipe)
    pieces = base.cut_cells(whole, cells)
    PROOFS.mkdir(parents=True, exist_ok=True)
    whole.save(PROOFS / "base-ne-native.png")
    base.geometry_review(whole).resize((1280, 896), Image.Resampling.NEAREST).save(PROOFS / "base-ne-geometry-review.png")
    (PROOFS / "component-ownership.bin").write_bytes(owners)
    (PROOFS / "cell-ownership.bin").write_bytes(cells)
    rows = []
    # Keep each physical cell/part independently observable and reproducible.
    # These ignored artifacts are source proofs, never a packed sprite payload.
    for row, image in pieces:
        path = PROOFS / f"cell-{row['tile_variant']:02d}-{row['part']}.png"
        image.save(path)
        rows.append(dict(row, image=base.identity(path)))
    document = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61CC; ready/empty NE source positions only.",
                    recipe=base.identity(RECIPE), geometry=base.source.geometry("NE"), evidence=evidence, parts=rows)
    (PROOFS / "parts.json").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchBaseChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61CC; complete base source, no full state/view or runtime acceptance.",
                 status="passed", cases=result.testsRun, recipe=base.identity(RECIPE), parts=base.identity(PROOFS / "parts.json"),
                 helper=base.identity(ROOT / "media/multilayer/tools/launch_facility_base.py"),
                 test=base.identity(ROOT / "src/tests/multilayer/launch_facility_base_checks.py"), evidence=evidence,
                 source_coverage=False, packed=False, runtime=False)
    (PROOFS / "verification.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
