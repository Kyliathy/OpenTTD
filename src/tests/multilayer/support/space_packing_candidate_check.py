"""Verify selected Space/orbital packing against independent canonical shipping bytes.

@file space_packing_candidate_check.py Uses actual freshly generated source leaves.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17B/17BC/17CC.
Original approval identities remain historical facts; candidates grant no new
art approval, canonical publication, native decoding or runtime activation.
"""

import argparse
import copy
import importlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from PIL import Image

from artifact_paths import RunContext, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_space_sources as source
import pack_space_art as pack
from evidence import identity

RUN_ID = None
COLLECTION = "space"


class SpacePackingTests(unittest.TestCase):
    """Check original selected source/packing producers without historical proofs."""

    @classmethod
    def setUpClass(cls):
        """Select the complete declared source reports for this invocation.

        @return None; missing current inputs fail without searching another run.
        """
        cls.context = RunContext(ROOT, "space", COLLECTION + "-derived-inputs", RUN_ID)
        if not (ROOT / "mlm/runtime/.runs" / RUN_ID).is_dir():
            raise ValueError("packing requires an explicitly reserved source invocation")
        catalogue = json.loads((ROOT / "mlm/review-producers.json").read_text(encoding="utf-8"))
        cls.manifests = {}
        # Existing declarations bind public asset IDs to actual current source
        # owners; no latest-run search can substitute for a missing source leaf.
        for row in catalogue["recipes"]:
            if row["id"].startswith(COLLECTION + "-source-"):
                context = RunContext(ROOT, row["feature"], row["scenario"], RUN_ID)
                cls.manifests[row["original_parameters"]["asset"]] = identity(context.reports / "derived.json")

    def test_actual_packing_repeats_independent_canonical_bytes(self):
        """Derive twice from selected fresh sources and compare canonical outputs.

        @return None; only disposable candidate data and reports are written.
        """
        public = pack.PUBLIC.read_bytes()
        scope_bytes = pack.SCOPE.read_bytes()
        with no_old_proofs(ROOT):
            first = source.produce_packing(COLLECTION, self.context, self.manifests)
            second, files = pack.derive(source_manifests=self.manifests, context=self.context)
        self.assertEqual(public, pack.PUBLIC.read_bytes())
        self.assertEqual(scope_bytes, pack.SCOPE.read_bytes())
        self.assertEqual(json.loads((ROOT / first["manifest"]["path"]).read_text(encoding="utf-8")), second)
        scope = json.loads(scope_bytes)
        if COLLECTION == "launch":
            self.assertEqual(second["historical_source_handoff"], scope["source_handoff"])
        else:
            self.assertEqual(second["historical_source_reviews"], scope["source_reviews"])
        self.assertEqual(second["source_reviews"], self.manifests)
        # Canonical bytes are independent approved inputs, so repeatability
        # cannot silently accept a deterministic change to shipping geometry.
        for role in ("png", "nfo", "header"):
            candidate = self.context.data / Path(scope[role]).name
            self.assertEqual(files[candidate], (ROOT / scope[role]).read_bytes())
            self.assertEqual(candidate.read_bytes(), files[candidate])
        summary = dict(_documentation=__doc__, run_id=RUN_ID, source_manifests=self.manifests,
                       canonical_bytes_exact=True, source_count=len(self.manifests),
                       family_count=len(second["families"]), pose_count=second["pose_count"],
                       native_start=second["native_start"], native_bound=second["native_sprite_bound"],
                       manifest=first["manifest"], artifacts=first["artifacts"], publishes_source=False)
        (ROOT / ("test-cache/cleanup/" + COLLECTION + "-packing-candidate-verification.json")).write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    def test_missing_changed_foreign_sources_and_encoder_refuse(self):
        """Refuse missing/stale source identities, another run and a wrong encoder.

        @return None; intentional faults modify only in-memory selection records.
        """
        changed = copy.deepcopy(self.manifests)
        changed.pop(next(iter(changed)))
        with self.assertRaisesRegex(ValueError, "incomplete"):
            pack.derive(source_manifests=changed, context=self.context)
        changed = copy.deepcopy(self.manifests)
        changed[next(iter(changed))]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "bytes changed"):
            pack.derive(source_manifests=changed, context=self.context)
        foreign = RunContext(ROOT, self.context.feature, self.context.scenario, RUN_ID + "-foreign")
        with self.assertRaisesRegex(ValueError, "foreign invocation"):
            pack.derive(source_manifests=self.manifests, context=foreign)
        with patch.object(pack.PIL, "__version__", "0.0-unit-mismatch"):
            with self.assertRaisesRegex(ValueError, "toolchain"):
                pack.derive(source_manifests=self.manifests, context=self.context)
        with self.assertRaisesRegex(ValueError, "together"):
            pack.derive(context=self.context)
        if COLLECTION in ("orbital", "launch"):
            public = json.loads(pack.PUBLIC.read_text(encoding="utf-8"))
            asset = "module.nucleus" if COLLECTION == "orbital" else "facility.launch-center"
            family = next(row for row in public["assets"] if row["asset_id"] == asset)
            family["anchor"][0] += 1
            with self.assertRaises(ValueError):
                pack.derive(public=public, source_manifests=self.manifests, context=self.context)

    def test_independent_launch_alias_facts_refuse_coherent_wrong_state_pixels(self):
        """Refuse an otherwise valid physical alias that shows working art during construction.

        @return None; altered metadata stays in memory and all source PNG bytes remain exact.
        """
        if COLLECTION != "launch":
            self.skipTest("this case protects the independent launch alias fixture")
        from tools import launch_fleet_handoff as handoff
        import convert_launch_center_source as producer

        reference = self.manifests["facility.launch-center"]
        changed = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        replacement = None
        # Construct a coherent pixel/geometry/alias mutation: the target remains
        # a genuine same-view/cell/part pose, but construction must retain ready art.
        for index, row in enumerate(changed["sprites"]):
            if row["state"] != "construction":
                continue
            for canonical in changed["canonical_parts"]:
                # Genuine working/blocked source pixels cannot become construction
                # merely because a forged alias preserves all physical dimensions.
                if (canonical["state"] in ("working", "blocked") and canonical["direction"] == row["direction"]
                        and canonical["tile_variant"] == row["tile_variant"] and canonical["part"] == row["part"]
                        and canonical["native_sha256"] != row["native_sha256"]):
                    replacement = dict(canonical, state=row["state"], frame=row["frame"],
                                       alias_of={name: canonical[name] for name in handoff.FIELDS})
                    changed["sprites"][index] = replacement
                    break
            if replacement is not None:
                break
        self.assertIsNotNone(replacement)
        context = RunContext(ROOT, "space", "source-facility-launch-center", RUN_ID)
        public = json.loads(pack.PUBLIC.read_text(encoding="utf-8"))
        family = next(row for row in public["assets"] if row["asset_id"] == "facility.launch-center")
        family["authoring"].update(atlas_manifest=reference["path"], atlas_manifest_sha256=reference["sha256"],
                                   native_atlas=changed["native_source"]["path"])
        with no_old_proofs(ROOT):
            with Image.open(ROOT / changed["native_source"]["path"]) as atlas:
                self.assertEqual(len(producer.decode(changed, atlas=atlas, context=context)), 1536)
                with self.assertRaisesRegex(ValueError, "logical aliases differ"):
                    handoff.current_source_poses(family, changed, atlas, None, context)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--collection", choices=("space", "orbital", "launch"), default="space")
    parser.add_argument("--case", choices=("all", "alias", "refusals"), default="all")
    arguments = parser.parse_args()
    RUN_ID, COLLECTION = arguments.run_id, arguments.collection
    pack = importlib.import_module("pack_launch_fleet_art" if COLLECTION == "launch" else "pack_" + COLLECTION + "_art")
    selected = [__file__]
    if arguments.case == "alias":
        selected.append("SpacePackingTests.test_independent_launch_alias_facts_refuse_coherent_wrong_state_pixels")
    elif arguments.case == "refusals":
        selected.append("SpacePackingTests.test_missing_changed_foreign_sources_and_encoder_refuse")
    unittest.main(argv=selected, verbosity=2)
