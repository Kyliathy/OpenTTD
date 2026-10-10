"""Focused native platform model/component, phase and capsule-clearance checks.

@file tube_terminal_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed records/pixels stay in memory; original source and station/cargo owners are untouched.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_tube_terminals as terminals


class TubeTerminalSourceChecks(unittest.TestCase):
    """Prove256 actual native platform source poses and reject false state/role claims.

    @see Plan13 task48C.3; operating cargo, packing and runtime acceptance remain separate.
    """

    @classmethod
    def setUpClass(cls):
        """Build one immutable baseline for independent malformed report cases.

        @param cls Current test class holding valid decoded metadata and exact PNG bytes.
        @return None; no repository source or public metadata is written.
        """
        cls.baseline, cls.encoded = terminals.build()

    def setUp(self):
        """Isolate retained source metadata and the report for each refusal.

        @param self Current test instance.
        @return None; mutations cannot affect later cases or original images.
        """
        self.document = json.loads(terminals.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == "facility.tube-terminal")
        self.report = copy.deepcopy(self.baseline)

    def test_complete_exact_source_regeneration(self):
        """Verify256 actual roles/states/phases/parts and exact read-only proof.

        @param self Current test instance.
        @return None; all native pixels, contacts and source identities regenerate.
        """
        self.assertEqual(self.report["pose_count"], 256)
        terminals.validate(self.report, self.encoded, self.document)
        terminals.publish(check=True)

    def test_installed_states_keep_the_same_physical_model(self):
        """Verify actual working/power-wait changes are confined to authored glass.

        @param self Current test instance.
        @return None; installed geometry/contacts stay exact outside the nine-pixel fitting.
        """
        with Image.open(io.BytesIO(self.encoded)) as atlas:
            # Every real native axis/role keeps its installed model. No vehicle,
            # cargo buffer, moved booth or recoloured body impersonates activity.
            for direction in terminals.AXES:
                for role in range(4):
                    images = {}
                    for state in ("ready", "working", "blocked"):
                        row = next(s for s in self.report["sprites"] if (s["state"], s["direction"], s["tile_variant"], s["frame"], s["part"]) == (state, direction, role, 0, "back"))
                        x, y, width, height = row["box"]
                        images[state] = atlas.crop((x, y, x + width, y + height))
                    cue = row["indicator"]["target"]
                    cx, cy, width, height = cue
                    allowed = {(x, y) for y in range(cy, cy + height) for x in range(cx, cx + width)}
                    for state in ("working", "blocked"):
                        changed = {(x, y) for y in range(64) for x in range(64) if images["ready"].getpixel((x, y)) != images[state].getpixel((x, y))}
                        self.assertTrue(changed)
                        self.assertTrue(changed <= allowed)

    def test_missing_construction_role_refuses(self):
        """Reject an absent actual Y canopy B construction original.

        @param self Current test instance.
        @return None; a ready model or duplicate role cannot fill the gap.
        """
        self.family["selections"][:] = [row for row in self.family["selections"] if (row["state"], row["axis"], row["role"]) != ("construction", "Y", 3)]
        with self.assertRaisesRegex(ValueError, "model coverage"):
            terminals.build(self.document)

    def test_excluded_moved_booth_refuses(self):
        """Keep the known relocated construction X booth excluded from source.

        @param self Current test instance.
        @return None; correct captions cannot rehabilitate changed model placement.
        """
        row = next(row for row in self.family["selections"] if (row["state"], row["axis"], row["role"]) == ("construction", "X", 1))
        row.update(source="tube-terminal-construction-v1", source_frame=["construction", "X", 1])
        with self.assertRaisesRegex(ValueError, "excluded native platform original frame"):
            terminals.build(self.document)

    def test_wrong_actual_axis_at_same_bounds_refuses(self):
        """Reject true X source pixels relabeled as Y despite matching canvas/bounds.

        @param self Current test instance.
        @return None; independent four-contact geometry detects a false direction.
        """
        original = terminals.tube.retained_candidate("tube-terminal-platform-x-base-v1", self.document)
        whole = terminals.native_platform(original)
        with self.assertRaisesRegex(ValueError, "misses an actual ledge contact"):
            terminals.validate_platform_contacts(whole, "Y", 0)

    def test_changed_original_refuses(self):
        """Reject changed selected source identity before native model conversion.

        @param self Current test instance.
        @return None; only the expected in-memory digest is modified.
        """
        original = next(row for row in self.document["candidates"] if row["key"] == "tube-terminal-platform-x-base-v1")
        original["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            terminals.build(self.document)

    def test_fabricated_model_state_policy_refuses(self):
        """Reject source metadata substituting unfinished construction for working.

        @param self Current test instance.
        @return None; only explicit identical installed-model reuse is authorized.
        """
        self.family["model_state_for"]["working"] = "construction"
        with self.assertRaisesRegex(ValueError, "geometry/model-state contract mismatch"):
            terminals.build(self.document)

    def test_missing_original_indicator_phase_refuses(self):
        """Reject a component missing one actual authored display phase.

        @param self Current test instance.
        @return None; a repeated lamp cannot replace missing original phase artwork.
        """
        parent = json.loads(terminals.facilities.RECORD.read_text(encoding="utf-8"))
        component = next(row for row in parent["components"] if row["key"] == "facility-status")
        component["frames"] = 3
        with self.assertRaisesRegex(ValueError, "indicator coverage mismatch"):
            terminals.facilities.indicator(component)

    def test_indicator_cannot_paint_transparency(self):
        """Reject a cue moved onto transparent background behind the platform.

        @param self Current test instance.
        @return None; component insertion must preserve all original alpha/contact pixels.
        """
        self.family["cue_targets"]["X"][0] = [20, 38, 3, 3]
        with self.assertRaisesRegex(ValueError, "lamp target is not opaque"):
            terminals.build(self.document)

    def test_missing_front_phase_refuses(self):
        """Reject a complete-count report missing the actual final foreground pose.

        @param self Current test instance.
        @return None; each actual axis/role/state/phase needs both aligned parts.
        """
        self.report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "state/axis/role/phase/part coverage"):
            terminals.validate(self.report, self.encoded, self.document)

    def test_false_component_provenance_refuses(self):
        """Reject fabricated authored-glass ancestry behind unchanged atlas pixels.

        @param self Current test instance.
        @return None; physical model reuse cannot conceal a different status source.
        """
        self.report["sprites"][0]["indicator"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "false model/component provenance"):
            terminals.validate(self.report, self.encoded, self.document)

    def test_actual_native_capsule_chord_covered_refuses(self):
        """Reject repainting both parts over actual X capsule coordinate31/49.

        @param self Current test instance.
        @return None; independent native route clearance catches a matching painted subset.
        """
        with Image.open(io.BytesIO(self.encoded)) as atlas:
            broken = atlas.copy()
        colour = next(value for value in broken.get_flattened_data() if value != 0)
        # Equal back/front edits defeat simple recomposition checks. The
        # independent native chord still refuses an invented physical wall.
        for row in self.report["sprites"]:
            if (row["state"], row["direction"], row["tile_variant"], row["frame"]) == ("ready", "NE", 0, 0):
                x, y = row["box"][:2]
                broken.putpixel((x + 31, y + 49), colour)
        with self.assertRaisesRegex(ValueError, "foreground covers actual capsule route"):
            terminals.validate(self.report, terminals.tube.indexed_png(broken), self.document)

    def test_forbidden_palette_refuses(self):
        """Reject a reserved animated colour in the static native source atlas.

        @param self Current test instance.
        @return None; alter only encoded proof pixels in memory.
        """
        with Image.open(io.BytesIO(self.encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((0, 0), min(terminals.tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden native platform palette"):
            terminals.validate(self.report, terminals.tube.indexed_png(broken), self.document)


if __name__ == "__main__":
    unittest.main()
