"""Audit retained launch-center candidates without approving rejected artwork.

@file launch_facility_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups61C/61CC/61F.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Run normally for focused provenance checks. --restore-predecessor rebuilds
only the missing ignored baseline from an explicit read-only Git object.
Candidate selection and complete source coverage remain separate gates.
"""

import argparse
import json
from pathlib import Path
import subprocess
import unittest

from PIL import Image

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, digest, identity, read_json, verify_native_proof
from tools import launch_facility_base as base


PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-base-v1"
PREDECESSOR = PROOFS / "predecessor.json"
PREDECESSOR_COMMIT = "32cf62eec829f6a57d03f156ce43a76a4f3680ff"
PREDECESSOR_SHA256 = "8afc57b181a34f85c274dac0196a7a80e649dda1506fe5c54d76c8cfd9ed80bc"
RECORD = ROOT / "media/multilayer/source/space/space-generation-record.json"
ASSET = "facility.launch-center"


def restore_predecessor():
    """Rebuild the exact pre-authoring source record without modifying Git.

    @return None; refuse overwrite or a different original object/content.
    """
    result = subprocess.run(["git", "show", PREDECESSOR_COMMIT + ":media/multilayer/source/space/space-generation-record.json"],
                            cwd=ROOT, check=True, capture_output=True)
    # This Windows checkout's observed source bytes use CRLF. Restoring those
    # exact bytes preserves provenance without checkout/reset or a new baseline.
    original = result.stdout.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    import hashlib
    if hashlib.sha256(original).hexdigest() != PREDECESSOR_SHA256:
        raise ValueError("predecessor Git object differs from the captured original")
    PROOFS.mkdir(parents=True, exist_ok=True)
    with PREDECESSOR.open("xb") as sink:
        sink.write(original)


class LaunchFacilitySourceChecks(unittest.TestCase):
    """Require immutable originals, actual native geometry and honest open selection."""

    def setUp(self):
        """Load the exact source record and the pre-authoring producer baseline.

        @param self Active candidate integrity test.
        @return None.
        """
        self.record = read_json(RECORD)
        self.before = read_json(PREDECESSOR)
        self.candidates = [row for row in self.record["candidates"] if row["asset_id"] == ASSET]

    def test_preexisting_families_candidates_and_payload_remain_unchanged(self):
        """Append only the new family/candidates while preserving the complete predecessor.

        @param self Active predecessor preservation test.
        @return None.
        """
        self.assertEqual(digest(PREDECESSOR), PREDECESSOR_SHA256)
        self.assertEqual(len(self.before["families"]), 17)
        self.assertEqual(len(self.before["candidates"]), 230)
        # Every original family/candidate remains byte-equivalent as decoded
        # metadata; the launch source cannot revise an accepted model or range.
        for key, row in self.before["families"].items():
            self.assertEqual(self.record["families"][key], row)
        self.assertEqual(self.record["candidates"][:230], self.before["candidates"])
        self.assertEqual(self.record["excluded_sources"][:len(self.before["excluded_sources"])], self.before["excluded_sources"])
        preserved = read_json(NATIVE_PROOFS / "before.json")["protected"]
        # The generation record may append this explicitly owned family;
        # published GRF/NFO and generic geometry bytes cannot change with it.
        for name, expected in preserved.items():
            if name != RECORD.relative_to(ROOT).as_posix():
                self.assertEqual(digest(ROOT / name), expected)

    def test_every_generated_original_retains_exact_request_and_reference_identity(self):
        """Rejected candidates retain original pixels, complete prompts and actual tool inputs.

        @param self Active original/provenance test.
        @return None.
        """
        self.assertGreaterEqual(len(self.candidates), 4)
        # A rejected draft still needs complete ancestry. Keeping its original
        # prevents later accidental selection or silent repair of failed pixels.
        for candidate in self.candidates:
            receipt_ref = candidate["generation_receipt"]
            self.assertEqual(identity(ROOT / receipt_ref["path"]), receipt_ref)
            receipt = read_json(ROOT / receipt_ref["path"])
            self.assertEqual({key: value for key, value in candidate.items() if key != "generation_receipt"}, receipt)
            self.assertEqual(digest(ROOT / candidate["path"]), candidate["sha256"])
            self.assertEqual(candidate["tool"], "built-in image_gen")
            self.assertTrue(candidate["transparent_background"])
            self.assertIn("Use case:", candidate["prompt"])
            with Image.open(ROOT / candidate["path"]) as original:
                self.assertEqual(original.mode, "RGBA")
                self.assertEqual(list(original.size), candidate["actual_size"])
            for reference in candidate["reference_inputs"]:
                self.assertEqual(digest(ROOT / reference["path"]), reference["sha256"])
                self.assertTrue(reference["role"])

    def test_rejections_stay_excluded_and_only_reviewed_complete_base_is_selected(self):
        """Rejected images stay excluded; a composed base needs exact complete ancestry.

        @param self Active source-selection refusal test.
        @return None.
        """
        family = self.record["families"][ASSET]
        if family["selected_base"] is not None:
            selected = base.bound_json(family["selected_base"])
            self.assertEqual(selected["status"], "selected_base")
            self.assertTrue(selected["base_selected"])
            self.assertEqual(selected["model_review"]["status"], "approved_complete_base")
            model = base.bound_json(selected["component_model"])
            self.assertEqual(set(model["selected_components"]), set(base.ORDER))
            self.assertEqual(selected["native_ground"]["cells"], 12)
            # One selected base cannot authorize missing orientations, state
            # foregrounds, packed loading or the separate ordinary-game gate.
            for field in ("source_coverage", "packed", "runtime"):
                self.assertFalse(selected[field])
        exclusions = {row["key"]: row for row in self.record["excluded_sources"]}
        for candidate in self.candidates:
            if candidate["status"].startswith("rejected_"):
                self.assertEqual(exclusions[candidate["key"]]["status"], candidate["status"])
            else:
                # A reviewed physical component is partial source, not a
                # complete facility base or permission to activate runtime art.
                self.assertEqual(candidate["status"], "selected_component")
                self.assertEqual(candidate["contact_review"]["status"], "approved")
            self.assertFalse(candidate["source_coverage"])
            self.assertFalse(candidate["packed"])
            self.assertFalse(candidate["runtime"])
        alpha_failure = next(row for row in self.candidates if row["key"] == "launch-center-base-ne-v4")
        self.assertEqual(alpha_failure["layout_review"]["minimum_clearance"], 0)
        self.assertEqual(alpha_failure["status"], "rejected_alpha_clearance")
        document = read_json(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json")
        row = next(row for row in document["assets"] if row["asset_id"] == ASSET)
        if family["source_coverage"]:
            # Complete later source coverage needs its exact public handoff;
            # historical component receipts still retain their partial flags.
            report = base.bound_json(family["source_manifest"])
            self.assertTrue(report["coverage_complete"])
            self.assertEqual(report["logical_pose_count"], 1536)
            self.assertEqual(len(report["sprites"]), 1536)
            self.assertFalse(report["packed"])
            self.assertFalse(report["runtime_activated"])
            self.assertEqual(row["source"]["status"], "authored")
            self.assertEqual(identity(ROOT / row["source"]["path"])["sha256"], row["source"]["sha256"])
            self.assertEqual(row["authoring"]["atlas_manifest_sha256"], family["source_manifest"]["sha256"])
        else:
            self.assertEqual(row["source"]["status"], "planned")
            self.assertIsNone(row["source"]["path"])
        self.assertIsNone(row["packed"]["offset"])

    def test_current_native_world_and_viewport_producer_proofs_are_bound(self):
        """Both actual configurations retain the corrected twelve-cell viewport contract.

        @param self Active source consumer producer-boundary test.
        @return None.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            proof = read_json(NATIVE_PROOFS / f"native-{configuration}.json")
            verify_native_proof(proof)
            self.assertEqual(proof["assertions"], 305)


def main():
    """Restore only an explicit baseline or publish the focused candidate audit.

    @return None; fail nonzero without selecting art or changing native state.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--restore-predecessor", action="store_true")
    args = parser.parse_args()
    if args.restore_predecessor:
        restore_predecessor()
        return
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchFacilitySourceChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    selected_base = read_json(RECORD)["families"][ASSET]["selected_base"]
    report = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Groups61C/61CC: source ancestry/preservation; complete coverage/runtime remain separate.",
                  audit_complete=True, focused_cases=result.testsRun, predecessor=identity(PREDECESSOR),
                  record=identity(RECORD), caller=identity(Path(__file__).resolve()),
                  preserved_families=17, preserved_candidates=230,
                  retained_rejected_candidates=sum(row["status"].startswith("rejected_") for row in read_json(RECORD)["candidates"] if row["asset_id"] == ASSET),
                  base_selected=selected_base is not None, selected_base=selected_base,
                  source_coverage=False, packed=False, runtime=False)
    # Preserve the earlier component-audit evidence. Base selection is a later
    # phase with its own exact source-record identity and rebuilt report.
    filename = "base-source-verification.json" if selected_base is not None else "candidate-verification.json"
    (PROOFS / filename).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
