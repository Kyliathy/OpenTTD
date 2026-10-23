"""Check actual decoded utility originals, contacts and source corruption refusals.

@file utility_conduit_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group60B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3B/4.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source checks create no installed cells, energy rights, packed range or game activation.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_utility_art as art


class UtilityConduitSourceChecks(unittest.TestCase):
    """Exercise retained raster ancestry and native branch geometry independently.

    The native feature boundary separately verifies paid same-domain topology.
    These cases inspect actual atlas crops and deliberately corrupted inputs.
    """

    @classmethod
    def setUpClass(cls):
        """Decode actual publication inputs once for the focused source burst.

        @param cls Test class receiving immutable baseline records and raster bytes.
        @return None; each refusal test mutates an independent copied input only.
        """
        cls.record = json.loads(art.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(art.tube.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.encoded = art.build(cls.record, cls.public)
        with Image.open(io.BytesIO(cls.encoded)) as atlas:
            cls.atlas = atlas.copy()

    def sprite(self, domain, state, direction):
        """Decode an actual stored crop, retaining original indexed palette values.

        @param domain Explicit Heat or Electricity variant.
        @param state Required construction/connected/unconnected presentation.
        @param direction Required physical cardinal branch.
        @return Indexed64x31 crop from the actual generated atlas.
        """
        row = next(r for r in self.report["sprites"] if (r["domain"], r["state"], r["direction"]) == (domain, state, direction))
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def test_exact_complete_publication(self):
        """Require24 unique pose identities and exact reproducible source publication.

        @param self Current isolated test case.
        @return None; missing or duplicate poses cannot hide behind a correct count.
        """
        art.validate(self.report, self.encoded, self.record, self.public)
        identities = {(r["domain"], r["state"], r["direction"], r["tile_variant"]) for r in self.report["sprites"]}
        self.assertEqual(len(identities), 24)
        self.assertEqual(art.ATLAS.read_bytes(), self.encoded)
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(art.public_family(self.public)["packed"]["offset"])

    def test_retained_independent_models_and_construction(self):
        """Inspect12 genuine originals and actual incomplete native hardware differences.

        @param self Current isolated test case.
        @return None; another axis/domain or ready alias cannot replace construction.
        """
        # The shared source record also receives separately owned riser art;
        # only this family's immutable originals establish conduit coverage.
        self.assertEqual(len({r["sha256"] for r in self.record["candidates"] if r["asset_id"] == art.ASSET}), 12)
        # Whole physical views retain all three low fittings and fixed contacts;
        # independently authored incomplete insulation is visible in each axis.
        for domain in art.DOMAINS:
            for axis in ("X", "Y"):
                complete = art.native_original(f"utility-{domain.lower()}-connected-{axis.lower()}-v1", self.record, self.public)[0]
                preview = art.native_original(f"utility-{domain.lower()}-construction-{axis.lower()}-v1", self.record, self.public)[0]
                self.assertNotEqual(complete.tobytes(), preview.tobytes())
                self.assertIsNotNone(preview.getbbox())

    def test_caps_preserve_body_opacity_and_only_change_original_fittings(self):
        """Inspect every stored connected/capped pair outside the actual end interiors.

        @param self Current isolated test case.
        @return None; topology changes cannot move or regenerate installed hardware.
        """
        # Every native branch changes visibly at its own existing end, while
        # all other original indexed pixels and full native opacity stay exact.
        for domain in art.DOMAINS:
            for direction in art.DIRECTIONS:
                connected = self.sprite(domain, "connected", direction)
                capped = self.sprite(domain, "unconnected", direction)
                px, py = art.PORTS[domain][direction]
                changes = []
                for y in range(31):
                    for x in range(64):
                        a, b = connected.getpixel((x, y)), capped.getpixel((x, y))
                        self.assertEqual(a != 0, b != 0)
                        if a != b:
                            changes.append((x, y))
                            self.assertLessEqual(abs(x - px), 2)
                            self.assertLessEqual(abs(y - py), 2)
                self.assertTrue(changes, (domain, direction))

    def test_actual_neighbour_contacts_and_coexisting_domains(self):
        """Meet opposing stored branches at every real32/16 neighbouring cell pitch.

        @param self Current isolated test case.
        @return None; offset domains remain separate and visible at shared floor cells.
        """
        opposites = {"NE": "SW", "SE": "NW", "SW": "NE", "NW": "SE"}
        # Opposing actual raster contacts must overlap without a procedural join
        # pixel. Each domain retains its distinct contact lane through all turns.
        for domain in art.DOMAINS:
            for direction, (dx, dy) in art.PITCH.items():
                own = self.sprite(domain, "connected", direction)
                neighbour = self.sprite(domain, "connected", opposites[direction])
                px, py = art.PORTS[domain][direction]
                overlap = [(x, y) for y in range(py - 1, py + 2) for x in range(px - 1, px + 2)
                           if own.getpixel((x, y)) != 0 and neighbour.getpixel((x - dx, y - dy)) != 0]
                self.assertTrue(overlap, (domain, direction))
        heat = self.sprite("Heat", "connected", "NE")
        electric = self.sprite("Electricity", "connected", "NE")
        self.assertNotEqual(heat.tobytes(), electric.tobytes())
        self.assertTrue(any(a != 0 and b == 0 for a, b in zip(heat.get_flattened_data(), electric.get_flattened_data())))
        self.assertTrue(any(b != 0 and a == 0 for a, b in zip(heat.get_flattened_data(), electric.get_flattened_data())))

    def test_missing_and_duplicate_original_coverage_refuse(self):
        """Remove or duplicate real axis selections without touching immutable originals.

        @param self Current isolated test case.
        @return None; correct total counts do not excuse missing domain/state/axis identity.
        """
        for duplicate in (False, True):
            broken = copy.deepcopy(self.record)
            rows = broken["families"][0]["axis_selections"]
            rows.pop()
            if duplicate:
                rows.append(copy.deepcopy(rows[0]))
            with self.assertRaisesRegex(ValueError, "coverage"):
                art.build(broken, self.public)

    def test_foreign_original_and_rejected_selection_refuse(self):
        """Try a wrong domain, wrong axis and review-excluded physical original.

        @param self Current isolated test case.
        @return None; labels or valid hashes cannot authorize another source identity.
        """
        broken = copy.deepcopy(self.record)
        broken["families"][0]["axis_selections"][0]["source"] = "utility-electricity-connected-x-v1"
        with self.assertRaisesRegex(ValueError, "substitutes"):
            art.build(broken, self.public)
        broken = copy.deepcopy(self.record)
        broken["candidates"][0]["status"] = "rejected_test_only"
        with self.assertRaisesRegex(ValueError, "rejected"):
            art.build(broken, self.public)

    def test_frozen_registration_and_missing_approval_refuse(self):
        """Attempt body drift or an unreviewed native fitting registration.

        @param self Current isolated test case.
        @return None; an unchanged raw image cannot authorize a different placement.
        """
        broken = copy.deepcopy(self.record)
        broken["candidates"][0]["native_transform"]["native_position"][0] += 1
        with self.assertRaisesRegex(ValueError, "reviewed registration"):
            art.build(broken, self.public)
        public = copy.deepcopy(self.public)
        art.public_family(public)["geometry_notes"]["approved_utility_registrations"].pop(self.record["candidates"][0]["key"])
        with self.assertRaisesRegex(ValueError, "lacks a reviewed"):
            art.build(self.record, public)

    def test_domain_merge_and_contact_drift_refuse(self):
        """Corrupt public domain coverage or its actual native coupling position.

        @param self Current isolated test case.
        @return None; shared-floor domain identity and contact geometry stay separate.
        """
        for mutation in ("domain", "contact"):
            public = copy.deepcopy(self.public)
            family = art.public_family(public)
            if mutation == "domain":
                family["native_domains"] = ["Heat", "Heat"]
            else:
                family["geometry_notes"]["port_pixels"]["Electricity"]["NE"][1] -= 4
            with self.assertRaisesRegex(ValueError, "domain/contact"):
                art.build(self.record, public)

    def test_changed_original_reference_hash_refuses(self):
        """Corrupt retained reference identity without modifying the original source file.

        @param self Current isolated test case.
        @return None; source provenance cannot detach an edit from its approved model.
        """
        broken = copy.deepcopy(self.record)
        next(row for row in broken["candidates"] if row["references"])["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            art.build(broken, self.public)

    def test_missing_duplicate_and_false_ancestry_refuse(self):
        """Corrupt atlas mappings while retaining plausible encoded native pixels.

        @param self Current isolated test case.
        @return None; exact identities, anchors and source ancestry are all required.
        """
        for mutation in ("missing", "duplicate", "source", "anchor"):
            report = copy.deepcopy(self.report)
            if mutation == "missing":
                report["sprites"].pop()
            elif mutation == "duplicate":
                report["sprites"][-1] = copy.deepcopy(report["sprites"][0])
            elif mutation == "source":
                report["sprites"][0]["source_sha256"] = "0" * 64
            else:
                report["sprites"][0]["anchor"][1] += 1
            with self.assertRaises(ValueError):
                art.validate(report, self.encoded, self.record, self.public)

    def test_changed_native_pixel_and_palette_refuse(self):
        """Change an actual decoded body pixel and a separately corrupted palette entry.

        @param self Current isolated test case.
        @return None; matching metadata cannot conceal changed shipped source pixels.
        """
        atlas = self.atlas.copy()
        x, y = next((x, y) for y in range(atlas.height) for x in range(atlas.width) if atlas.getpixel((x, y)) != 0)
        atlas.putpixel((x, y), 1 if atlas.getpixel((x, y)) != 1 else 2)
        with self.assertRaisesRegex(ValueError, "branch pixels"):
            art.validate(self.report, art.tube.indexed_png(atlas), self.record, self.public)
        atlas = self.atlas.copy()
        palette = atlas.getpalette()
        palette[3] ^= 1
        atlas.putpalette(palette)
        with self.assertRaisesRegex(ValueError, "palette"):
            art.validate(self.report, art.tube.indexed_png(atlas), self.record, self.public)

    def test_false_runtime_claim_refuses(self):
        """Reject source-only evidence relabelled as a loaded game result.

        @param self Current isolated test case.
        @return None; runtime binding/ordinary rendering need their own later evidence.
        """
        report = copy.deepcopy(self.report)
        report["runtime_activated"] = True
        with self.assertRaisesRegex(ValueError, "runtime activation"):
            art.validate(report, self.encoded, self.record, self.public)


if __name__ == "__main__":
    unittest.main()
