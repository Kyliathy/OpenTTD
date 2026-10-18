"""Verify original one-nozzle motion preserves every actual hauler quantity.

@file space_art_hauler_travel.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group66C.1
The native source proof owns no movement clock, cargo, service or runtime sprite.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_hauler_parts as hauler
import convert_space_mining_parts as parts


class HaulerTravelTests(unittest.TestCase):
    """Protect actual load pixels, one physical outlet and immutable donor ancestry."""

    @classmethod
    def setUpClass(cls):
        """Read the produced48-pose atlas and independently resolve its24static bodies.

        @param cls Fixture sharing only immutable actual native-source inputs.
        """
        cls.record = art.read_record()
        family = cls.record["families"][hauler.ASSET]
        review = family["travel_source_review"]
        assert art.identity(ROOT / review["path"]) == review
        cls.report = json.loads((ROOT / review["path"]).read_text(encoding="utf-8"))
        cls.atlas = Image.open((ROOT / review["path"]).parent / "native.png")
        cls.bodies = {}
        # Each native quantity is independently re-read from its actual original
        # source; the motion report cannot substitute an empty body for freight.
        for direction in family["directions"]:
            cls.bodies["empty", direction] = art.native_view(family["selections"]["free/" + direction], direction, cls.record)
            for quantity in ("partial", "full"):
                cls.bodies[quantity, direction] = hauler.load_pose(quantity, direction, cls.record)

    def setUp(self):
        """Detach metadata before a refused donor/contact/registration change.

        @param self Independent hauler motion fixture.
        """
        self.document = copy.deepcopy(self.record)
        self.family = self.document["families"][hauler.ASSET]

    def pose(self, quantity, direction, phase):
        """Read the exact produced native cell for a quantity/camera/phase.

        @param self Immutable actual motion-atlas fixture.
        @param quantity Original empty/partial/full category.
        @param direction Genuine heading in the existing compass.
        @param phase Actual source gas phase0 or1.
        @return Exact indexed64x48 produced pixels.
        """
        column = self.family["directions"].index(direction)
        row = ("empty", "partial", "full").index(quantity) * 2 + phase
        return self.atlas.crop((column * 64, row * 48, column * 64 + 64, row * 48 + 48))

    def test_all48_poses_preserve_the_actual_quantity_body(self):
        """Adding gas cannot remove cargo, erase opacity or alter any original hardware.

        @param self Complete actual motion/quantity canvas fixture.
        """
        self.assertEqual(self.report["actual_motion_poses"], 48)
        self.assertEqual(len(self.report["poses"]), 48)
        # Inspect every produced pixel against the independently resolved actual
        # quantity, including originally empty air outside the declared gas.
        for item in self.report["poses"]:
            body, _ = self.bodies[item["quantity"], item["direction"]]
            pose = self.pose(item["quantity"], item["direction"], item["phase"])
            gas = {tuple(point) for point in item["gas_pixels"]}
            self.assertEqual(item["quantity_body_sha256"], hashlib.sha256(body.tobytes()).hexdigest())
            for y in range(48):
                for x in range(64):
                    if (x, y) in gas:
                        self.assertEqual(body.getpixel((x, y)), 0)
                        self.assertNotEqual(pose.getpixel((x, y)), 0)
                    else:
                        self.assertEqual(pose.getpixel((x, y)), body.getpixel((x, y)))
        self.assertFalse(self.report["complete_source_coverage"])
        self.assertFalse(self.report["runtime"])

    def test_both_phases_touch_the_same_one_original_nozzle_for_all_loads(self):
        """The three quantity banks use identical actual gas at one opaque rim.

        @param self Real outlet contact and quantity-independent pulse fixture.
        """
        self.assertEqual(self.family["physical_rear_outlet_count"], 1)
        self.assertNotIn("travel_secondary_sources", self.family)
        sources = {}
        # The nozzle is original hardware. Cargo cannot move its mounting point,
        # introduce another engine or replace a missing phase with a floating dot.
        for item in self.report["poses"]:
            direction, phase = item["direction"], item["phase"]
            contacts = self.family["travel_outlet_contacts"][direction]
            empty, _ = self.bodies["empty", direction]
            self.assertEqual(len(contacts), 1)
            self.assertNotEqual(empty.getpixel(tuple(contacts[0])), 0)
            self.assertEqual(min(max(abs(x - contacts[0][0]), abs(y - contacts[0][1])) for x, y in item["gas_pixels"]), 1)
            key = direction, phase
            if key in sources:
                self.assertEqual(item["gas_pixels"], sources[key])
            else:
                sources[key] = item["gas_pixels"]

    def test_real_phases_remain_distinct_in_every_quantity_and_heading(self):
        """Both authored pulses survive native reduction without an invented clock.

        @param self Complete native phase/readability fixture.
        """
        # Recompute the actual atlas differences rather than trusting a metadata
        # animation declaration or a report's claimed count.
        for quantity, direction in self.bodies:
            first = self.pose(quantity, direction, 0)
            second = self.pose(quantity, direction, 1)
            self.assertGreaterEqual(sum(a != b for a, b in zip(first.get_flattened_data(), second.get_flattened_data(), strict=True)), 2)

    def test_public_motion_api_matches_the_reviewed_loaded_pose(self):
        """The direct loaded selector keeps freight instead of returning empty+gas.

        @param self Actual public source-composition consumer fixture.
        """
        pose, evidence = hauler.motion_pose("full", "NE", 0, self.document)
        self.assertEqual(pose.tobytes(), self.pose("full", "NE", 0).tobytes())
        self.assertEqual(evidence["body"]["state"], "full")

    def test_four_registered_instances_keep_exact_approved_original_ancestry(self):
        """Source reuse changes only receiver registration and creates no new artwork.

        @param self Original15 donor and own17 source-instance fixture.
        """
        keys = {key for values in self.family["travel_sources"].values() for key in values.values()}
        self.assertEqual(len(keys), 4)
        # Every physical camera and phase retains its actual donor approval,
        # prompt/receipt/source pixels and scale while owning a distinct17outlet.
        for phase, values in self.family["travel_sources"].items():
            for direction, key in values.items():
                candidate = parts.registered_component_source(key, direction, self.document)
                donor = art.source_candidate(candidate["instance_of"]["candidate"], self.document)
                self.assertEqual(candidate["asset_id"], hauler.ASSET)
                self.assertEqual(candidate["sha256"], donor["sha256"])
                self.assertEqual(candidate["generation_receipt"], donor["generation_receipt"])
                self.assertEqual(candidate["phase"], int(phase))

    def test_changed_donor_geometry_is_refused(self):
        """Native source reuse cannot stretch or rotate the original gas pixels.

        @param self Invalid shared-source scale fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.document)
        candidate["native_transforms"]["N"]["uniform_scale"][0] += 1
        with self.assertRaisesRegex(ValueError, "shared effect changes original source geometry"):
            hauler.motion_pose("empty", "N", 0, self.document)

    def test_changed_donor_approval_is_refused(self):
        """Unchanged raw bytes cannot borrow an unapproved original pulse.

        @param self Invalid donor approval snapshot fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.document)
        candidate["donor_registration"]["N"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "missing or changed original shared effect approval"):
            hauler.motion_pose("empty", "N", 0, self.document)

    def test_fresh_approval_cannot_float_gas_away_from_original_nozzle(self):
        """A signed effect volume still requires contact with its own actual rim.

        @param self Metadata-signed but physically detached pulse fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.document)
        candidate["native_transforms"]["N"]["native_reference"][0] += 8
        self.family["travel_component_bounds"]["N"] = [39, 42, 42, 46]
        self.family["approved_travel_registration"]["0"]["N"] = parts.part_signature(
            candidate, "N", self.family["travel_component_bounds"]["N"],
            self.family["approved_base_registration"]["N"], self.family["travel_outlet_contacts"]["N"])
        with self.assertRaisesRegex(ValueError, "registered pulse does not touch its original rear outlet"):
            hauler.motion_pose("empty", "N", 0, self.document)

    def test_internal_composition_refuses_occupied_cargo_pixels(self):
        """An invalid internal producer cannot overwrite freight with original gas.

        @param self In-memory negative fixture, never an authored or accepted raster.
        """
        empty, _ = self.bodies["empty", "N"]
        body, source_body = self.bodies["partial", "N"]
        moving = self.pose("empty", "N", 0)
        # Select that same real empty N/phase0 pulse; the negative fixture must
        # test a cargo collision without borrowing another camera or phase.
        item = next(item for item in self.report["poses"] if item["quantity"] == "empty" and item["direction"] == "N" and item["phase"] == 0)
        invalid = body.copy()
        invalid.putpixel(tuple(item["gas_pixels"][0]), 1)
        with self.assertRaisesRegex(ValueError, "hauler gas overlaps original cargo"):
            hauler._compose_motion("partial", "N", 0, invalid, empty, moving, source_body, item["plume"])

    def test_service_or_missing_phase_cannot_become_motion(self):
        """Art adds neither native17service admission nor an unprovided pulse phase.

        @param self Unsupported quantity/phase source fixture.
        """
        with self.assertRaisesRegex(ValueError, "unsupported hauler motion quantity"):
            hauler.motion_pose("service", "N", 0, self.document)
        with self.assertRaisesRegex(ValueError, "missing authored mining phase"):
            hauler.motion_pose("empty", "N", 2, self.document)


if __name__ == "__main__":
    unittest.main()
