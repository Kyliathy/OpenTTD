"""Verify selected M1 packing from actual regenerated source inputs.

@file m1_packing_candidate_check.py Run after explicit bounded source-family checks.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17.
Canonical shipping bytes and original source identities are independent expected
inputs. Tests neither publish candidates nor update a source approval baseline.
"""

import argparse
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from artifact_paths import RunContext, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_art_sources as conversion
import pack_m1_art as pack
from evidence import identity

RUN_ID = None


class PackingCandidateTests(unittest.TestCase):
    """Check current selected sources against immutable shipping/approval expectations."""

    @classmethod
    def setUpClass(cls):
        """Select nine actual source manifests from the explicitly requested invocation.

        @return None; missing prerequisites fail without searching old proof trees.
        """
        cls.context = RunContext(ROOT, "multilayer-art", "m1-derived-inputs", RUN_ID)
        if not (ROOT / "mlm/runtime/.runs" / RUN_ID).is_dir():
            raise ValueError("packing checks require an explicitly reserved source invocation")
        data = json.loads((ROOT / "mlm/review-producers.json").read_text(encoding="utf-8"))
        cls.manifests = {}
        # Declarations identify each original producer alias and exact selected
        # source owner; no basename/latest-run search can satisfy a missing leaf.
        for row in data["recipes"]:
            if row["id"].startswith("m1-source-"):
                context = RunContext(ROOT, row["feature"], row["scenario"], RUN_ID)
                cls.manifests[row["original_parameters"]["manifest_alias"]] = identity(context.reports / "derived.json")

    def test_actual_packing_is_repeatable_without_old_cache_or_publication(self):
        """Compare current candidates twice to existing independent shipping bytes.

        @return None; only selected generated data/reports are written.
        """
        public = (ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_bytes()
        with no_old_proofs(ROOT):
            first = conversion.produce("packing", self.context, self.manifests)
            second, files = pack.derive(source_manifests=self.manifests, context=self.context)
        self.assertEqual(first["report"], second)
        self.assertEqual(public, (ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_bytes())
        scope = json.loads(pack.SCOPE.read_text(encoding="utf-8"))
        # Independent canonical payload inputs protect all existing byte-level
        # sprites/ranges; deterministic regeneration alone cannot approve drift.
        for role in ("png", "nfo", "header"):
            candidate = self.context.data / Path(scope[role]).name
            self.assertEqual(files[candidate], (ROOT / scope[role]).read_bytes())
            self.assertEqual(candidate.read_bytes(), files[candidate])
        summary = dict(_documentation=__doc__, run_id=RUN_ID, source_manifests=self.manifests,
                       canonical_bytes_exact=True, source_count=len(self.manifests),
                       family_count=len(second["families"]), pose_count=second["pose_count"],
                       native_bound=second["native_sprite_bound"], artifacts=first["artifacts"], publishes_source=False)
        (ROOT / "test-cache/cleanup/m1-packing-candidate-verification.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    def test_missing_changed_foreign_sources_and_encoder_refuse(self):
        """Refuse incomplete sources, a stale identity, another run and wrong encoder.

        @return None; intentional faults change only in-memory selection records.
        """
        changed = copy.deepcopy(self.manifests)
        changed.pop(next(iter(changed)))
        with self.assertRaisesRegex(ValueError, "incomplete"):
            pack.derive(source_manifests=changed, context=self.context)
        changed = copy.deepcopy(self.manifests)
        changed[next(iter(changed))]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "manifest changed"):
            pack.derive(source_manifests=changed, context=self.context)
        foreign = RunContext(ROOT, self.context.feature, self.context.scenario, "foreign-packing-run")
        with self.assertRaisesRegex(ValueError, "foreign invocation"):
            pack.derive(source_manifests=self.manifests, context=foreign)
        with patch.object(pack.PIL, "__version__", "0.0-unit-mismatch"):
            with self.assertRaisesRegex(ValueError, "toolchain"):
                pack.derive(source_manifests=self.manifests, context=self.context)
        with self.assertRaisesRegex(ValueError, "both a context"):
            pack.derive(context=self.context)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run-id", required=True)
    RUN_ID = parser.parse_args().run_id
    unittest.main(argv=[__file__], verbosity=2)
