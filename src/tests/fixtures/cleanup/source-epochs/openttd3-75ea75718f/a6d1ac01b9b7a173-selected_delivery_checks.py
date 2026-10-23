"""Focused selected native gallery publication and delivery-metadata boundaries.

@file selected_delivery_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md84D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md84ED; explicit successive named evidence remains reviewable before changing the default.
All corruptions use isolated metadata copies; native/source/gallery bytes stay intact.
"""

import argparse
import copy
import subprocess
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation
import review_m1_art as review
from tools import selected_gallery as gallery

EVIDENCE_MANIFEST = activation.DEFAULT_MANIFEST


class SelectedDeliveryPreflight(unittest.TestCase):
    """Exercise stale/missing gallery and premature delivery refusals.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md84D.1; read-only publication preflight.
    """

    @classmethod
    def setUpClass(cls):
        """Load the explicit accepted selected gallery and canonical callbacks once.

        @param cls Active unittest class receiving shared immutable inputs.
        @return None; the default evidence/public metadata remains unchanged.
        """
        cls.selection = activation.select_evidence(EVIDENCE_MANIFEST)
        review.configure(cls.selection)
        cls.public, cls.packed = gallery.read(activation.PUBLIC), gallery.read(activation.PACKED)
        cls.bindings = activation.validate_binding_review(cls.public, cls.packed)
        cls.record = gallery.read(review.GALLERY)

    def test_current_exact_gallery_is_accepted(self):
        """Accept all exact current frames, ranges, callbacks and sealed native worlds.

        @param self Current read-only unittest case.
        @return None; actual accepted gallery must be selected, not a legacy fallback.
        """
        self.assertEqual(activation.verified_gallery(self.public, self.packed, self.bindings), self.selection.output("gallery").relative_to(ROOT).as_posix())

    def test_missing_gallery_cannot_borrow_dated_prefix(self):
        """Refuse a missing selected gallery despite the retained earlier review.

        @param self Current read-only unittest case.
        @return None; no artifact is created for this missing-file probe.
        """
        missing = self.selection.output("gallery").parent / "absent-gallery.json"
        self.assertFalse(missing.exists())
        with patch.object(review, "configure"), patch.object(review, "GALLERY", missing):
            self.assertIsNone(activation.verified_gallery(self.public, self.packed, self.bindings))

    def test_stale_frame_contact_and_validator_refuse(self):
        """Refuse changed native frames, contact pixels and the validation owner itself.

        @param self Current isolated unittest case.
        @return None; each refusal preserves actual sealed gallery/source files.
        """
        for fault in ("frame", "contact", "owner"):  # A valid payload or native log cannot carry review across a substituted raster, damaged overview or changed gallery-validation policy.
            candidate = copy.deepcopy(self.record)
            if fault == "frame":
                candidate["frames"][0]["sha256"] = "0" * 64
            elif fault == "contact":
                candidate["contact_sheet"]["sha256"] = "0" * 64
            else:
                candidate["selected_adapter"]["sha256"] = "0" * 64
            with self.subTest(fault=fault), patch.object(review, "read_json", return_value=candidate):
                self.assertIsNone(activation.verified_gallery(self.public, self.packed, self.bindings))

    def test_failed_preflight_writes_nothing(self):
        """Refuse selected publication before changing any public/default/generated owner.

        @param self Current isolated unittest case.
        @return None; every existing output identity remains exact on failure.
        """
        paths = [activation.PUBLIC, activation.PACKED, activation.DEFAULT_MANIFEST, activation.RUNTIME]
        before = {path: activation.evidence_identity(path) if path.exists() else None for path in paths}
        with patch.object(activation, "verified_gallery", return_value=None), self.assertRaisesRegex(AssertionError, "current84C"):
            activation.publish_selected_runtime(self.public, self.packed, self.bindings, True)
        for path, previous in before.items():  # Failed acceptance must not leave partial promotion or a new runtime file behind.
            self.assertEqual(activation.evidence_identity(path) if path.exists() else None, previous)

    def test_cli_uses_the_same_named_evidence(self):
        """Keep CLI and imported gallery activation owners on the same explicit selection.

        @param self Current read-only unittest case.
        @return None; the command validates without publishing or using the dated default.
        """
        result = subprocess.run([sys.executable, "-B", str(ROOT / "media/multilayer/activate_m1_art.py"), "--evidence-manifest", str(self.selection.path)],
                                cwd=ROOT, capture_output=True, text=True, timeout=110)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('"families": 69', result.stdout)
        self.assertIn('"public_promoted": false', result.stdout)


class SelectedPublishedDelivery(unittest.TestCase):
    """Verify the actual selected public/default/runtime handoff after publication.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md84D.1; no future or personal acceptance.
    """

    def test_exact_public_runtime_and_default_selection(self):
        """Verify selected-only activation with all protected fields/source identities intact.

        @param self Current read-only postpublication unittest case.
        @return None; sixty-five active and four retained consumers must remain explicit.
        """
        selection = activation.select_evidence(activation.DEFAULT_MANIFEST)
        self.assertIn("selected_gallery_boundary", selection.document)
        public, packed = gallery.read(activation.PUBLIC), gallery.read(activation.PACKED)
        frozen, _ = gallery.validate_boundary(public, packed, selection, {})
        previous = {row["asset_id"]: row for row in frozen["assets"]}
        runtime = gallery.read(activation.RUNTIME)
        bindings = activation.validate_binding_review(public, packed)
        expected = copy.deepcopy(bindings)
        for row in expected:  # Published metadata may add completed technical-review flags only; the actual callback/pose/source proof remains the canonical activation owner's result.
            row.update(gallery_status="technical_review_verified_user_feedback_deferred", user_review="deferred")
        self.assertEqual(runtime["bindings"], expected)
        self.assertEqual((runtime["selected_families"], runtime["selected_sprites"], runtime["active_consumers"]), (69, 10557, 65))
        selected = {row["asset_id"]: row for row in bindings}
        for family in public["assets"]:  # Unselected ground/future factories/Space remain byte-equivalent; retained terrain is reviewed source, never reactivated native body art.
            asset = family["asset_id"]
            if asset not in selected:
                self.assertEqual(family, previous[asset])
                continue
            active = not selected[asset]["retained_only"]
            self.assertEqual(family["runtime"]["status"], "bound_verified" if active else "superseded_retained")
            self.assertEqual(family["runtime"]["gallery"]["user_feedback"], "deferred")
            self.assertEqual(family["runtime"]["binding_manifest"], activation.RUNTIME.relative_to(ROOT).as_posix())
            if "authoring" in family:
                self.assertEqual(family["authoring"]["runtime_activated"], active)
            if "runtime_activated" in family["source"]:
                self.assertEqual(family["source"]["runtime_activated"], active)
        self.assertEqual(packed["runtime_asset_ids"], list(selected))
        self.assertFalse(packed["runtime_activated"])
        self.assertFalse(runtime["whole_profile_activated"])
        self.assertEqual(activation.verified_gallery(public, packed, bindings), runtime["gallery_manifest"])


if __name__ == "__main__":
    # Preflight can select an immutable candidate before publication; ordinary
    # postpublication checks use the existing default evidence owner.
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--evidence-manifest", type=Path, default=activation.DEFAULT_MANIFEST)
    options, test_arguments = parser.parse_known_args()
    EVIDENCE_MANIFEST = options.evidence_manifest
    unittest.main(argv=[sys.argv[0], *test_arguments])
