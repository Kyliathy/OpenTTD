"""Focused original Ground viewport source/world checks with explicit prepared inputs.

@file ground_review_sources_check.py Existing independent sources only; no simulated native pass.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20B.
"""
import argparse
import copy
import json
from pathlib import Path
import unittest
from unittest.mock import patch

from ground_review_sources import GroundSourceViews, ROOT, retained_world


class GroundReviewSourceChecks(unittest.TestCase):
    """Keep complete independent source poses, original saves and cache-free selection binding."""

    def test_complete_original_views_without_historical_cache_reads(self):
        """Read all15488source poses while denying old logs, test-cache and executable reads.

        @param self Focused source-boundary test owner.
        """
        read_text, read_bytes = Path.read_text, Path.read_bytes

        def guarded(path, reader, args, kwargs):
            """Allow only current source/fixture/selected-review reads during actual normalization.

            @param path Actual requested file path.
            @param reader Original Path text/byte reader.
            @param args Original positional reader arguments.
            @param kwargs Original keyword reader arguments.
            @return Unchanged original bytes/text from an admitted input owner.
            """
            resolved = path.resolve()
            # Historical proof caches and old executing images cannot supply
            # current source semantics, even while those directories still exist.
            if (resolved.is_relative_to(ROOT / "logs") or resolved.is_relative_to(ROOT / "test-cache")
                    or resolved.suffix.lower() == ".exe"):
                raise AssertionError("historical cache read during current source selection: " + str(path))
            return reader(path, *args, **kwargs)

        def text(path, *args, **kwargs):
            """Route actual text reads through the historical-cache refusal.

            @param path Actual requested JSON/source contract.
            @param args Original positional decoding options.
            @param kwargs Original named decoding options.
            @return Original admitted text, without changing its contents.
            """
            return guarded(path, read_text, args, kwargs)

        def binary(path, *args, **kwargs):
            """Route actual byte reads through the same historical-cache refusal.

            @param path Actual requested immutable image/contract.
            @param args Original positional byte-reader options.
            @param kwargs Original named byte-reader options.
            @return Original admitted bytes, without generating new expectations.
            """
            return guarded(path, read_bytes, args, kwargs)

        with patch.object(Path, "read_text", text), patch.object(Path, "read_bytes", binary):
            with GroundSourceViews(ROOT, self.source_run, self.inputs) as view:
                self.assertEqual(len(view.allowed), 50)
                self.assertEqual(sum(len(view.poses(asset)[0]) for asset in view.allowed), 15488)
                view.verify_current()
                with self.assertRaisesRegex(ValueError, "unselected"):
                    view.poses("asteroid.nickel")

    def test_another_m1_source_role_cannot_borrow_an_approved_report(self):
        """Refuse a legitimate current purchase report substituted for the vehicle producer.

        @param self Focused exact-producer selection test owner.
        """
        changed = copy.deepcopy(self.inputs)
        changed["m1-source-vehicle"] = changed["m1-source-purchase"]
        with self.assertRaisesRegex(ValueError, "another M1 producer"):
            GroundSourceViews(ROOT, self.source_run, changed)

    def test_retained_worlds_and_original_raster_domains_are_explicit(self):
        """Keep exact original worlds and complete independent raster fingerprints for all bursts.

        @param self Focused retained-fixture selection test owner.
        """
        required = {"collector": 2, "industry-states": 222, "purchases": 10, "rail": 531, "road": 390, "stations": 420}
        # Fingerprints preserve original complete indexed/RGB containers without
        # retaining a screenshot/EXE archive or granting current native success.
        for mode, count in required.items():
            with self.subTest(mode=mode):
                row, source = retained_world(mode)
                self.assertTrue(source.is_file())
                self.assertEqual(len(row["expected_captures"]), count)
        with self.assertRaisesRegex(ValueError, "unknown"):
            retained_world("paid-tube-models")


def main():
    """Verify only explicit completed pure source bytes, without reusing old native outcomes."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source-result", type=Path, required=True)
    args = parser.parse_args()
    state = json.loads(args.source_result.read_text(encoding="utf-8"))
    # A named completed pure supplier is mandatory; no latest-run discovery or
    # historical native status can manufacture the original source input set.
    if (state.get("kind") != "artifact_rebuild_result" or state.get("complete") is not True
            or state.get("status") != "verified"):
        raise ValueError("select a completed explicit pure source supplier")
    GroundReviewSourceChecks.source_run = state["run_id"]
    inputs = {"ground-packing": state["nodes"]["ground-derived-inputs"]["outputs"]["manifest"]}
    for role in ("m1-source-vehicle", "m1-source-purchase", "m1-source-collector"):
        inputs[role] = state["nodes"][role]["outputs"]["manifest"]
    GroundReviewSourceChecks.inputs = inputs
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(GroundReviewSourceChecks))
    raise SystemExit(0 if result.wasSuccessful() else 1)


if __name__ == "__main__":
    main()
