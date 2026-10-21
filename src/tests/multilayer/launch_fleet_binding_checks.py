"""Verify final launch/fleet packed publication boundaries and optionally bind the payload.

@file launch_fleet_binding_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62I.3.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Four focused cases exercise actual ready evidence and independent corruptions.
--publish writes metadata only after those current gates pass; no game is run.
"""

import argparse
import copy
import json
from pathlib import Path
import sys
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/"media/multilayer"))
import pack_launch_fleet_art as packing
from tools import cargo_rocket_art as art
from tools import launch_fleet_handoff as handoff


class LaunchFleetBindingChecks(unittest.TestCase):
    """Require complete actual payload evidence and narrowly scoped public/source changes."""

    @classmethod
    def setUpClass(cls):
        """Capture current accepted source/public/native build evidence without changing it.

        @param cls Active final packed-binding boundary suite.
        @return None.
        """
        cls.proof = art.read_json(packing.BUILD_PROOF)
        cls.public = art.read_json(art.PUBLIC)
        cls.record = art.source.read_record()
        cls.accepted = handoff.checked_handoff()

    def test_actual_ready_binding_changes_only_qualified_checksums_and_own_packed_metadata(self):
        """All data loading is proven while saved definition IDs and unrelated source stay exact.

        @param self Active prospective complete actual payload and allowed-delta check.
        @return None.
        """
        snapshot = art.semantic_digest(self.public)
        report, proposed, document = packing.prepare_packed_publication(self.proof, self.public)
        self.assertEqual(art.semantic_digest(self.public), snapshot)
        self.assertEqual(report["pose_count"], 1648)
        self.assertEqual(report["packed_grf"]["native_bound"], 7791)
        self.assertFalse(report["runtime_activated"])
        self.assertEqual(len(report["packed_grf"]["authorized_prefix_checksum_updates"]), 60)
        before = {row["asset_id"]: row for row in self.public["assets"]}
        # Every previously delivered source/geometry/range field remains exact;
        # only the shared native container checksum can change for its60owners.
        for row in proposed["assets"]:
            asset = row["asset_id"]
            if asset in handoff.ASSETS:
                self.assertEqual({name: value for name, value in row.items() if name != "packed"},
                                 {name: value for name, value in before[asset].items() if name != "packed"})
                self.assertEqual(row["packed"]["namespace"], "MLVI")
                self.assertIsNone(row["packed"]["offset"])
                self.assertEqual(row["packed"]["sha256"], report["packed_grf"]["sha256"])
                self.assertTrue(document["families"][asset]["packed"])
                self.assertEqual(document["families"][asset]["status"], "packed_runtime_pending")
                self.assertFalse(row["authoring"]["runtime_activated"])
            else:
                normalized = copy.deepcopy(row)
                normalized["packed"]["sha256"] = before[asset]["packed"]["sha256"]
                self.assertEqual(normalized, before[asset])
        self.assertEqual(document["candidates"], self.record["candidates"])
        self.assertEqual(document["excluded_sources"], self.record["excluded_sources"])

    def test_missing_clean_build_wrong_container_and_grfid_evidence_are_refused(self):
        """A successful older build or one repeated file cannot authorize the final container.

        @param self Active clean-build/source/checksum independent refusal check.
        @return None.
        """
        # Change one evidence dimension at a time while all native processes
        # and approved source artwork remain genuinely successful and unchanged.
        for mutation in ("one_build", "wrong_sha", "grfid", "same_output", "source"):
            proof = copy.deepcopy(self.proof)
            repeated = proof["reproducible"]
            if mutation == "one_build":
                repeated["sha256"].pop()
            elif mutation == "wrong_sha":
                repeated["sha256"][0] = "0"*64
            elif mutation == "grfid":
                repeated["grfid_md5"][0] = "0"*32
            elif mutation == "same_output":
                repeated["outputs"][1] = copy.deepcopy(repeated["outputs"][0])
            else:
                repeated["source_outputs"][0]["sha256"] = "0"*64
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                packing.prepare_packed_publication(proof, self.public)

    def test_uncopied_missing_duplicate_and_false_native_cached_acceptance_are_refused(self):
        """Cached distribution proof needs both actual complete-byte configurations and restore.

        @param self Active real cached-copy/native coverage/restoration refusal check.
        @return None.
        """
        # Real packaging requires both distinct native observations and actual
        # discovery restoration, independently of the shared source success.
        for mutation in ("discovery", "loading", "missing", "duplicate", "pixels", "payload", "restore"):
            proof = copy.deepcopy(self.proof)
            cached = proof["generator_free"]
            if mutation == "discovery":
                cached["generator_discovery_disabled"] = False
            elif mutation == "loading":
                cached["native_loading_verified"] = False
            elif mutation == "missing":
                cached["native"].pop()
            elif mutation == "duplicate":
                cached["native"].append(copy.deepcopy(cached["native"][0]))
            elif mutation == "pixels":
                cached["native"][0]["full_pixel_bytes_exact"] = False
            elif mutation == "payload":
                cached["sha256"] = "0"*64
            else:
                cached["restored_discovery"] = "FORGED"
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                packing.prepare_packed_publication(proof, self.public)

    def test_unrelated_source_definition_namespace_native_range_and_schema_changes_are_refused(self):
        """Valid pixels never authorize modifying another public source or saved identity owner.

        @param self Active protected public contract and immutable native allocation check.
        @return None.
        """
        # These metadata defects can coexist with a perfectly valid GRF, so
        # the publication boundary must reject them independently of pixel tests.
        for mutation in ("source", "namespace", "range", "checksum", "schema"):
            public = copy.deepcopy(self.public)
            row = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-15")
            if mutation == "source":
                row["source"]["status"] = "forged"
            elif mutation == "namespace":
                row["packed"]["namespace"] = "FORGED"
            elif mutation == "range":
                row["packed"]["native_offset"] += 1
            elif mutation == "checksum":
                row["packed"]["sha256"] = "0"*64
            else:
                public["unqualified_extra_schema"] = True
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                packing.prepare_packed_publication(self.proof, public)


def main():
    """Run focused current evidence/refusal checks and optionally publish qualified packed data.

    @return Zero after four current checks and requested actual publication validation pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    started = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2, stream=sys.stdout).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchFleetBindingChecks))
    verification = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                        elapsed_seconds=round(time.monotonic()-started, 3), tests=art.source.identity(Path(__file__)),
                        packer=art.source.identity(Path(packing.__file__)), build_proof=art.source.identity(packing.BUILD_PROOF),
                        published=False, runtime=False)
    destination = packing.BUILD_PROOF.parent/"binding-verification.json"
    destination.write_text(json.dumps(verification, indent=2)+"\n", encoding="utf-8")
    if not result.wasSuccessful():
        return 1
    if args.publish:
        report = packing.bind_packed_payload()
        scope, _ = packing.read_scope()
        candidate, public, document = packing.prepare_packed_publication()
        # Read back all actual published owners after mutation, preserving the
        # distinction between packed loading and the still-pending native draw/game gate.
        if (art.read_json(ROOT/scope["report"]) != report or candidate != report
                or art.read_json(art.PUBLIC) != public or art.source.read_record() != document):
            raise ValueError("launch/fleet actual packed publication differs from its qualified proposed metadata")
        verification.update(published=True, packed_manifest=art.source.identity(ROOT/scope["report"]),
                            public_manifest=art.source.identity(art.PUBLIC), source_record=art.source.identity(art.source.RECORD))
        destination.write_text(json.dumps(verification, indent=2)+"\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
