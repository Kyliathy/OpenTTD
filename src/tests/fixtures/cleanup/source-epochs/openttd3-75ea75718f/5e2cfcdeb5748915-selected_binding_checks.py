"""Focused actual-callback and refusal checks for selected tube/first-geothermal review.

@file selected_binding_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Existing EvidenceSet/public/packing owners remain authoritative; all corruption probes are isolated in memory.
"""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation

SELECTION = ROOT / "logs/proofs/mlm/native-art/selected-runtime-20261018/evidence.json"


class SelectedBindingChecks(unittest.TestCase):
    """Verify current real callback chains without publishing assets or running native worlds.

    @see Plan14-84B.2; selected producer logs and sealed native closures are explicit inputs.
    """

    @classmethod
    def setUpClass(cls):
        """Select one named evidence set and retain immutable public/publication identities.

        @param cls Active focused fixture; no latest-run discovery or source output occurs.
        @return None.
        """
        cls.previous = (activation._ACTIVE_EVIDENCE, activation.PACKED, activation.RUNTIME)
        cls.chosen = activation.select_evidence(SELECTION)
        cls.public = json.loads(activation.PUBLIC.read_text(encoding="utf-8"))
        cls.packed = json.loads(activation.PACKED.read_text(encoding="utf-8"))
        cls.scope = json.loads(activation.pack.SCOPE.read_text(encoding="utf-8"))
        cls.public_identity = activation.evidence_identity(activation.PUBLIC)
        cls.published_runtime = cls.chosen.document["retained_published_runtime"]
        cls.published_gallery = cls.chosen.document["retained_published_gallery"]

    @classmethod
    def tearDownClass(cls):
        """Restore the caller's evidence context after all read-only probes.

        @param cls Active focused fixture with the original global evidence context.
        @return None.
        """
        activation._ACTIVE_EVIDENCE, activation.PACKED, activation.RUNTIME = cls.previous

    def tearDown(self):
        """Require every successful or refused probe to preserve published artifacts.

        @param self Active focused fixture after an isolated corruption probe.
        @return None.
        """
        self.assertEqual(activation.evidence_identity(activation.PUBLIC), self.public_identity)
        for identity in (self.published_runtime, self.published_gallery):  # Candidate review must preserve both older published proof products, including their complete bytes.
            self.assertEqual(activation.evidence_identity(ROOT / identity["path"]), identity)
        self.assertFalse(self.chosen.output("runtime").exists())
        self.assertFalse(self.chosen.output("gallery").exists())

    def test_exact_selected_callbacks_and_retained_prefix(self):
        """Require real native implementation for every addition and retain all older ranges.

        @param self Active fixture with actual selected native producer evidence.
        @return None.
        """
        rows = activation.validate_binding_review(self.public, self.packed)
        current = {row["asset_id"] for row in rows if row["native_source_proof"] is not None}
        self.assertEqual(current, activation.SELECTED_RUNTIME_ADDITIONS)
        self.assertEqual(len(rows), 69)
        self.assertEqual(sum(row["sprite_count"] for row in rows), 10557)
        retained = [row for row in rows if row["implementation_review"] == "retained_published_prefix"]
        self.assertEqual(len(retained), 43)
        self.assertEqual(sum(row["sprite_count"] for row in retained), 3495)

    def test_foreign_valid_callback_cannot_be_borrowed(self):
        """A real renderer for a different family cannot authorize tube guideway art.

        @param self Active fixture; candidate metadata is changed only in memory.
        @return None.
        """
        binding = activation.binding_for("tube.straight")
        binding["consumer"] = activation.binding_for("facility.vent-powerhouse")["consumer"]
        with self.assertRaisesRegex(AssertionError, "false selected native consumer/authority"):
            activation.validate_native_binding("tube.straight", binding)

    def test_comments_literals_or_other_function_cannot_prove_consumer(self):
        """The expected call must belong to the actual named native renderer body.

        @param self Active fixture; no native source is edited on disk.
        @return None.
        """
        binding = activation.binding_for("tube.straight")
        target = ROOT / binding["consumer"]["source"]
        original = Path.read_text
        source = '''void DrawTile_Rail(TileInfo *ti) {
            /* GetTubeGuidewayArt(ti->tile); */
            const char *label = "GetTubeGuidewayArt(ti->tile)";
        }
        void UnrelatedRenderer(TileInfo *ti) { GetTubeGuidewayArt(ti->tile); }
        '''

        def read_candidate(path, *args, **kwargs):
            """Offer one isolated consumer mutation while preserving scope/evidence reads.

            @param path Requested source or unchanged evidence/selection path.
            @param args Original Path.read_text positional arguments.
            @param kwargs Original Path.read_text keyword arguments.
            @return Candidate only for the exact consumer; otherwise actual immutable text.
            """
            return source if path == target else original(path, *args, **kwargs)

        with patch.object(Path, "read_text", read_candidate):
            with self.assertRaisesRegex(AssertionError, "native callback edge is absent"):
                activation.validate_native_binding("tube.straight", binding)

    def test_selector_marker_declaration_or_call_cannot_prove_definition(self):
        """An enum marker and call-shaped if block cannot substitute for a selector body.

        @param self Active fixture; genuine consumer files remain unchanged.
        @return None.
        """
        binding = activation.binding_for("facility.vent-powerhouse")
        target = ROOT / binding["selector_source"]
        original = Path.read_text
        source = '''auto marker = MultiLayerSpriteAsset::FacilityVentPowerhouse;
        std::optional<SpriteID> GetGeothermalGeneratorSprite(TileIndex tile);
        void OtherOwner(TileIndex tile) {
            if (GetGeothermalGeneratorSprite(tile)) { return; }
        }
        '''

        def read_candidate(path, *args, **kwargs):
            """Offer a false selector definition without modifying native files.

            @param path Requested native source or unchanged metadata/evidence path.
            @param args Original Path.read_text positional arguments.
            @param kwargs Original Path.read_text keyword arguments.
            @return Isolated false implementation for the exact selector source.
            """
            return source if path == target else original(path, *args, **kwargs)

        with patch.object(Path, "read_text", read_candidate):
            with self.assertRaisesRegex(AssertionError, "native function body is absent"):
                activation.validate_native_binding("facility.vent-powerhouse", binding)

    def test_future_and_forged_selection_cannot_supply_callback(self):
        """Unimplemented models, processors and controls still refuse after scope forgery.

        @param self Active fixture with the real explicit69-family review scope.
        @return None.
        """
        future = ("vehicle.model-05", "purchase.model-08", "facility.battery-works", "control.launch")
        for asset in future:  # Every unimplemented provider category must retain its own refusal despite the larger packed payload.
            with self.subTest(asset=asset):
                with self.assertRaisesRegex(ValueError, "not in delivered runtime scope"):
                    activation.binding_for(asset)
        candidate = copy.deepcopy(self.scope)
        candidate["runtime_asset_ids"].extend(future)
        metadata = Mock()
        metadata.read_text.return_value = json.dumps(candidate)
        with patch.object(activation.pack, "SCOPE", metadata):
            for asset in future:  # Forged metadata cannot turn any unsupported category into an implemented native consumer.
                with self.subTest(forged=asset):
                    with self.assertRaisesRegex(ValueError, "no (delivered|implemented selected)"):
                        activation.binding_for(asset)

    def test_missing_or_duplicate_review_scope_refuses(self):
        """A callback review must cover one unambiguous explicitly selected packed set.

        @param self Active fixture; only copied scope/packing records are corrupted.
        @return None.
        """
        candidate = copy.deepcopy(self.scope)
        candidate["runtime_asset_ids"].append("tube.straight")
        metadata = Mock()
        metadata.read_text.return_value = json.dumps(candidate)
        with patch.object(activation.pack, "SCOPE", metadata):
            with self.assertRaisesRegex(ValueError, "missing or ambiguous"):
                activation.runtime_ranges(self.packed)
        packed = copy.deepcopy(self.packed)
        packed["families"] = [row for row in packed["families"] if row["asset_id"] != "geothermal.conduit"]
        with self.assertRaisesRegex(ValueError, "missing from packed selection"):
            activation.runtime_ranges(packed)

    def test_stale_public_ranges_and_payload_refuse(self):
        """Public offsets, counts and distributed payload must agree before any callback claim.

        @param self Active fixture with immutable actual public and packing owners.
        @return None.
        """
        for key in ("native_offset", "sprite_count", "sha256"):  # Each independent public range/payload dimension protects the exact existing decoded native identities.
            public = copy.deepcopy(self.public)
            row = next(row for row in public["assets"] if row["asset_id"] == "tube.straight")
            row["packed"][key] = "0" * 64 if key == "sha256" else row["packed"][key] + 1
            with self.subTest(field=key):
                with self.assertRaisesRegex(AssertionError, "public runtime (range|payload) is stale"):
                    activation.validate_binding_review(public, self.packed)
        packed = copy.deepcopy(self.packed)
        packed["packed_grf"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(AssertionError, "distributed native payload hash is stale"):
            activation.validate_binding_review(self.public, packed)

    def test_missing_and_changed_native_evidence_refuse(self):
        """Neither missing semantic obligations nor stale selected logs/receipts can pass.

        @param self Active fixture; the selected EvidenceSet is copied rather than written.
        @return None.
        """
        for section, role in (("logs", "tube-guideway-debug"), ("reports", "tube-guideway-acceptance")):  # Native semantics and sealed original pixel ownership are separate required obligations.
            for fault in ("missing", "changed"):  # Both absent obligations and byte-substituted evidence must fail without touching retained artifacts.
                chosen = copy.copy(self.chosen)
                chosen.document = copy.deepcopy(self.chosen.document)
                if fault == "missing":
                    del chosen.document[section][role]
                else:
                    chosen.document[section][role]["sha256"] = "0" * 64
                with self.subTest(section=section, fault=fault), patch.object(activation, "_ACTIVE_EVIDENCE", chosen):
                    with self.assertRaisesRegex((ValueError, AssertionError), "evidence (is absent|changed)"):
                        activation.binding_for("tube.straight")

    def test_premature_runtime_and_collector_publication_refuse(self):
        """Callback review cannot publish additions before current selected gallery acceptance.

        @param self Active fixture; both publisher guards refuse before any output write.
        @return None.
        """
        with self.assertRaisesRegex(AssertionError, "requires the current84C native gallery"):
            activation.require_selected_gallery([{"asset_id": "tube.straight"}], None)
        with self.assertRaisesRegex(ValueError, "collector-only publication cannot promote"):
            activation.collector_integration(None, [], [], publish=True)


if __name__ == "__main__":
    unittest.main()
