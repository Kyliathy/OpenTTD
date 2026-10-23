"""Focused refusal checks for the selected native gallery's shared evidence owner.

@file selected_gallery_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md84C-84D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md84ED; exact predecessor snapshots never excuse changed producer sources.
Metadata copies exercise refusals; immutable paid worlds and source pixels stay untouched.
"""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation
from tools import selected_gallery as gallery


class SelectedGalleryChecks(unittest.TestCase):
    """Verify bounded contract, stale-artifact and missing/false-consumer refusals.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md84C.1; no native process or full regression.
    """

    @classmethod
    def setUpClass(cls):
        """Load one explicit selected gallery candidate and actual canonical bindings.

        @param cls Unittest class receiving immutable inputs shared by metadata-copy cases.
        @return None; no repository file is changed.
        """
        cls.selection = activation.select_evidence(ROOT / "logs/proofs/mlm/native-art/selected-gallery-20261018/evidence.json")
        cls.public = gallery.read(activation.PUBLIC)
        cls.packed = gallery.read(activation.PACKED)
        cls.bindings = activation.validate_binding_review(cls.public, cls.packed)
        cls.runtime = {"packed_payload": cls.packed["packed_grf"], "bindings": cls.bindings}

    def successor(self):
        """Model an unpublished successor using real exact delivery snapshot identities.

        @param self Current isolated unittest case with the retained named selection.
        @return In-memory EvidenceSet copy; no candidate file or source is written.
        """
        selection = copy.deepcopy(self.selection)
        selection.path = activation.CACHE / "successive-gallery-checks/absent-evidence.json"
        previous = activation.evidence_identity(self.selection.path)
        selection.document["base_evidence"] = previous
        selection.document["selected_gallery_boundary"].update(
            public=activation.evidence_identity(activation.PUBLIC),
            packed=activation.evidence_identity(activation.PACKED), previous_selection=previous)
        return selection

    def test_successive_exact_delivery_snapshots_are_accepted(self):
        """Accept both exact named native manifest identities after protected equivalence.

        @param self Current read-only metadata case.
        @return None; unknown hashes and false byte counts remain refused.
        """
        selection = self.successor()
        gallery.validate_boundary(self.public, self.packed, selection, {})
        boundaries = list(gallery.delivery_boundaries(selection, {}))
        self.assertEqual(len(boundaries), 2)
        public = [row["public"] for row in boundaries]
        packed = [row["packed"] for row in boundaries]
        # Old and fresh native captures name the same public owner through
        # different immutable snapshots, without permitting any unnamed hash.
        for snapshot in public:
            reference = {**snapshot, "path": gallery.PUBLIC}
            gallery.validate_retained(reference, public, packed, {})
        for fault in ("sha256", "bytes"):  # Exact identity includes byte count when a producer supplied it, not just semantic metadata equality.
            reference = {**public[0], "path": gallery.PUBLIC}
            reference[fault] = "0" * 64 if fault == "sha256" else -1
            with self.subTest(fault=fault), self.assertRaises(AssertionError):
                gallery.validate_retained(reference, public, packed, {})

    def test_stale_or_unrelated_predecessor_refuses(self):
        """Refuse changed named ancestry and snapshots borrowed from another base.

        @param self Current isolated metadata case.
        @return None; both failures occur before native proof aliases are admitted.
        """
        for fault in ("stale", "foreign"):  # A valid snapshot is insufficient unless its exact named selection is the declared predecessor of this publication.
            selection = self.successor()
            if fault == "stale":
                selection.document["base_evidence"]["sha256"] = "0" * 64
            else:
                selection.document["base_evidence"] = {"path": "foreign", "sha256": "0" * 64}
            with self.subTest(fault=fault), self.assertRaises(AssertionError):
                gallery.validate_boundary(self.public, self.packed, selection, {})

    def test_ancestor_protected_geometry_refuses(self):
        """Reject an ancestral geometry change even when the newest boundary agrees.

        @param self Current isolated metadata case.
        @return None; corruption touches only an in-memory parsed snapshot.
        """
        original_read = gallery.read
        ancestor = self.selection.document["selected_gallery_boundary"]["public"]["path"]

        def corrupted_snapshot(path):
            """Return one isolated malformed ancestor while retaining actual file checks.

            @param path Exact hash-checked artifact requested by the gallery owner.
            @return Parsed document, with only the selected ancestor's anchor corrupted.
            """
            document = original_read(path)
            if Path(path).resolve() == (ROOT / ancestor).resolve():
                family = next(row for row in document["assets"] if row["asset_id"] == "vehicle.model-09")
                family["anchor"][0] += 1
            return document

        with patch.object(gallery, "read", side_effect=corrupted_snapshot), self.assertRaisesRegex(AssertionError, "protected public"):
            gallery.validate_boundary(self.public, self.packed, self.successor(), {})

    def test_successive_delivery_does_not_hide_changed_english_source(self):
        """Keep changed producer sources strict despite equivalent delivery snapshots.

        @param self Current read-only metadata case.
        @return None; the original stale English identity must still refuse.
        """
        selection = self.successor()
        gallery.validate_boundary(self.public, self.packed, selection, {})
        boundaries = list(gallery.delivery_boundaries(selection, {}))
        proof = gallery.read(self.selection.selected("reports", "selected-purchase"))
        source = next(row for row in gallery.references(proof) if row["path"] == "src/lang/english.txt")
        self.assertNotEqual(source["sha256"], activation.evidence_identity(ROOT / source["path"])["sha256"])
        with self.assertRaisesRegex(AssertionError, "stale selected gallery artifact"):
            gallery.validate_retained(source, [row["public"] for row in boundaries], [row["packed"] for row in boundaries], {})

    def test_protected_source_geometry_and_packed_pose_refuse(self):
        """Refuse protected contract/range/pixel changes before any gallery publication.

        @param self Current isolated unittest case.
        @return None; every copied corruption must raise an assertion.
        """
        for field in ("anchor", "source", "pose", "payload"):  # Each mutation targets a distinct protected owner, preventing delivery metadata aliases from becoming blanket stale-source exemptions.
            public, packed = copy.deepcopy(self.public), copy.deepcopy(self.packed)
            family = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-09")
            if field == "anchor":
                family["anchor"][0] += 1
            elif field == "source":
                family["source"]["status"] = "placeholder"
            elif field == "pose":
                packed["sprites"][0]["native_offset"] = -1
            else:
                packed["packed_grf"]["sha256"] = "0" * 64
            with self.subTest(field=field), self.assertRaises(AssertionError):
                gallery.validate_boundary(public, packed, self.selection, {})

    def test_delivery_alias_is_scoped_to_selected_families(self):
        """Allow selected delivery flags while refusing future/unselected promotion.

        @param self Current isolated unittest case.
        @return None; only the explicitly selected delivery metadata may differ.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-09")
        family["runtime"] = {"status": "bound_verified"}
        family["authoring"]["runtime_activated"] = True
        gallery.validate_boundary(public, self.packed, self.selection, {})
        future = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-05")
        future["runtime"] = {"status": "bound_verified"}
        with self.assertRaises(AssertionError):
            gallery.validate_boundary(public, self.packed, self.selection, {})

    def test_changed_native_png_refuses(self):
        """Refuse a substituted complete native raster without editing its bytes.

        @param self Current isolated unittest case.
        @return None; exact frame identity remains mandatory after PPM cleanup.
        """
        proof = gallery.read(self.selection.selected("reports", "selected-movement"))
        frame = copy.deepcopy(proof["families"][0]["frames"][0])
        frame["sha256"] = "0" * 64
        with self.assertRaises(AssertionError):
            gallery.checked(frame, {})

    def test_missing_and_false_consumer_bindings_refuse(self):
        """Refuse missing selected bodies and foreign otherwise valid consumer chains.

        @param self Current isolated unittest case.
        @return None; both failures occur before loading heavy native closures.
        """
        for fault in ("missing", "foreign"):  # Count equality alone cannot authorize a different actual renderer or omit an implemented family.
            runtime = copy.deepcopy(self.runtime)
            if fault == "missing":
                runtime["bindings"].pop()
            else:
                row = next(row for row in runtime["bindings"] if row["asset_id"] == "vehicle.model-09")
                row["consumer"] = copy.deepcopy(next(row for row in self.bindings if row["asset_id"] == "geothermal.vent")["consumer"])
            with self.subTest(fault=fault), patch.object(activation, "validate_binding_review", return_value=self.bindings), self.assertRaises(AssertionError):
                gallery.validate(self.public, self.packed, runtime, self.selection)

    def test_missing_selected_closure_refuses(self):
        """Refuse absence of one current paid geothermal proof role.

        @param self Current isolated unittest case.
        @return None; the dated prefix cannot supply the missing selected role.
        """
        selection = copy.deepcopy(self.selection)
        selection.document["reports"].pop("selected-geothermal")
        with patch.object(activation, "validate_binding_review", return_value=self.bindings), self.assertRaises(KeyError):
            gallery.validate(self.public, self.packed, self.runtime, selection)

    def test_malformed_or_future_family_mapping_refuses(self):
        """Refuse missing current family coverage and forged future pose mappings.

        @param self Current isolated unittest case.
        @return None; complete native source mappings remain required.
        """
        proofs = [gallery.read(self.selection.selected("reports", role)) for role in gallery.ROLES]
        assets = {row["asset_id"]: row for row in self.public["assets"]}
        for fault in ("missing", "future", "range"):  # Native closure adapters must preserve all delivered IDs and their exact public ranges, independent of overall family counts.
            candidates = copy.deepcopy(proofs)
            if fault == "missing":
                candidates[2]["families"] = [row for row in candidates[2]["families"] if row["asset_id"] != "vehicle.model-13"]
            elif fault == "future":
                candidates[2]["families"][0]["asset_id"] = "vehicle.model-05"
            else:
                candidates[2]["families"][0]["native_offset"] += 1
            with self.subTest(fault=fault), self.assertRaises(AssertionError):
                gallery.current_mappings(candidates, assets, self.packed, {})


if __name__ == "__main__":
    unittest.main()
