"""Exercise candidate joining refusals without encoding or publishing canonical art.

@file merged_art_candidates_check.py
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Real complete source/packing repetition belongs to the selected recipe run;
these cases reject incomplete or foreign inputs before an encoder can start.
"""
import json
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from PIL import Image

from artifact_paths import RunContext, repository_root

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_art_sources as source
import pack_ground_art as ground


class MergedCandidateRefusals(unittest.TestCase):
    """Reject missing suppliers, foreign runs and missing or unpinned encoder tools."""

    def test_missing_family_never_starts_encoder(self):
        """Reject an incomplete family closure before discovering or running tools.

        @param self Active candidate input coverage check.
        """
        context = RunContext(ROOT, "multilayer-art", "candidate-refusals", "merge52-faults")
        with patch.object(source, "candidate_encoder_tools") as tools:
            with self.assertRaisesRegex(ValueError, "exactly five"):
                source.produce_candidate_grf(context, {})
            tools.assert_not_called()

    def test_foreign_run_cannot_borrow_valid_family_name(self):
        """Reject a foreign run even when all five semantic family roles are named.

        @param self Active invocation ownership check; no fault file is written.
        """
        context = RunContext(ROOT, "multilayer-art", "candidate-refusals", "merge52-faults")
        manifests = {name: {"path": f"mlm/reviews/multilayer-art/{name}-derived-inputs/foreign/reports/derived.json",
                            "sha256": "0" * 64} for name in ("m1", "space", "orbital", "launch", "ground")}
        with patch.object(source, "candidate_encoder_tools", return_value={}), patch.object(source, "source_snapshot", return_value={}):
            with self.assertRaisesRegex(ValueError, "foreign family/run/role"):
                source.produce_candidate_grf(context, manifests)

    def test_absent_encoder_names_the_supplier(self):
        """Report the exact unavailable tool instead of borrowing a cached binary.

        @param self Active tool discovery refusal check.
        """
        with patch.object(source.shutil, "which", return_value=None):
            with self.assertRaisesRegex(ValueError, "PATH tool: cmake"):
                source.candidate_encoder_tools()

    def test_changed_encoder_version_refuses_before_encoding(self):
        """Keep the admitted original encoder version independent of output bytes.

        @param self Active graphics tool pin check.
        """
        probe = SimpleNamespace(stdout="foreign encoder 99.0", stderr="")
        with patch.object(source.shutil, "which", return_value=sys.executable), patch.object(source.subprocess, "run", return_value=probe):
            with self.assertRaisesRegex(ValueError, "pinned version: grfcodec"):
                source.candidate_encoder_tools()

    def test_foreign_palette_and_animation_indices_cannot_enter_native_sources(self):
        """Refuse altered color tables and reserved animation in actual selected families.

        @param self Active palette contract check; no original or candidate file is written.
        """
        public = json.loads(ground.PUBLIC.read_text(encoding="utf-8"))
        families = {row["asset_id"]: row for row in public["assets"]}
        # Ordinary processor paint and explicit capsule company masks use their
        # original separate policies. Neither may borrow a foreign palette or
        # animate infrastructure by introducing reserved material indices.
        for asset in ("facility.battery-works", "vehicle.model-09"):
            family = families[asset]
            allowed = set(range(1, 256)) - ground.packing.base.FORBIDDEN
            with Image.new("P", (1, 1), min(allowed)) as image:
                image.putpalette(ground.packing.base.PALETTE_BYTES)
                image.info["transparency"] = 0
                ground.handoff.validate_native_palette(image, family)
                palette = image.getpalette()
                palette[0] ^= 1
                image.putpalette(palette)
                with self.assertRaises(ValueError):
                    ground.handoff.validate_native_palette(image, family)
                image.putpalette(ground.packing.base.PALETTE_BYTES)
                forbidden = ground.packing.base.FORBIDDEN - {0} - ground.handoff.COMPANY_INDICES
                image.putpixel((0, 0), min(forbidden))
                with self.assertRaises(ValueError):
                    ground.handoff.validate_native_palette(image, family)


if __name__ == "__main__":
    unittest.main(verbosity=2)
