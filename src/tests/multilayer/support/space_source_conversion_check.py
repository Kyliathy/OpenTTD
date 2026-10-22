"""Verify one actual Space source family with current generated storage.

@file space_source_conversion_check.py Each explicit family remains a bounded burst.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17B/17BB/17BC/17CB.
Existing canonical native pixel hashes and source policies supply independent
expectations; neither a new source report nor its gallery grants new approval.
"""

import argparse
import copy
import importlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

import PIL
from PIL import Image
from artifact_paths import RunContext, create_run_context, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_space_sources as source
from evidence import identity

ASSET = None
RUN_ID = None
COLLECTION = "space"


def context_for(run_id):
    """Reserve or explicitly join one shared selected source-check invocation.

    @param run_id Caller-selected exact primary/repeat/fault invocation identity.
    @return Current source family's explicit RunContext, with no latest-run search.
    """
    scenario = "source-" + ASSET.replace(".", "-")
    if not (ROOT / "mlm/runtime/.runs" / run_id).is_dir():
        return create_run_context("space", scenario, run_id=run_id)
    return RunContext(ROOT, "space", scenario, run_id)


class SpaceSourceTests(unittest.TestCase):
    """Exercise real source conversion independently of native game execution."""

    @classmethod
    def setUpClass(cls):
        """Select the explicit source context and existing original approval identity.

        @return None; actual source images stay in their disposable selected run.
        """
        cls.context = context_for(RUN_ID)
        public = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
        cls.family = next(row for row in public["assets"] if row["asset_id"] == ASSET)

    def test_actual_source_repeats_original_pixels_without_old_proofs(self):
        """Generate actual native source/contacts twice in separate fresh invocations.

        @return None; compare original approved pixels and all generated review images.
        """
        repeat = context_for(RUN_ID + "-repeat")
        with no_old_proofs(ROOT):
            first = source.produce_source(ASSET, COLLECTION, self.context)
            second = source.produce_source(ASSET, COLLECTION, repeat)
        self.assertEqual(first["native_source"]["sha256"], self.family["authoring"]["native_sha256"])
        self.assertEqual(first["native_source"]["sha256"], second["native_source"]["sha256"])
        original_images = {path.name: identity(path)["sha256"] for path in self.context.images.iterdir()}
        repeat_images = {path.name: identity(path)["sha256"] for path in repeat.images.iterdir()}
        self.assertEqual(original_images, repeat_images)
        report = json.loads((ROOT / first["manifest"]["path"]).read_text(encoding="utf-8"))
        self.assertEqual(report["historical_approval"], source.read_scope(COLLECTION)["source_reviews"][ASSET])
        self.assertFalse(report["runtime_activated"] if "runtime_activated" in report else report["runtime"])
        summary = dict(_documentation=__doc__, asset_id=ASSET, run_id=RUN_ID,
                       manifest=first["manifest"], exact_native_pixels=True, exact_repeat=True,
                       old_proof_io_refused=True, generated_images=len(original_images), source_files=first["source_files"])
        destination = ROOT / "test-cache/cleanup" / (COLLECTION + "-source-" + ASSET.replace(".", "-") + "-verification.json")
        destination.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
        foreign = RunContext(ROOT, self.context.feature, self.context.scenario, RUN_ID + "-foreign")
        with self.assertRaisesRegex(ValueError, "foreign invocation"):
            source.selected_path(first["manifest"], foreign, "reports")

    def test_encoder_and_changed_original_are_refused(self):
        """Refuse a wrong encoder and copied malformed selected source identities.

        @return None; no actual original/approval input or expected baseline is edited.
        """
        with patch.object(PIL, "__version__", "0.0-unit-mismatch"):
            with self.assertRaisesRegex(ValueError, "toolchain"):
                source.read_scope(COLLECTION)
        document = copy.deepcopy(source.art.read_record())
        mutations = 0
        # Mutate only isolated candidate identities for this actual family;
        # the real original pixels and frozen policy hashes stay untouched.
        for candidate in document["candidates"]:
            if candidate["asset_id"] == ASSET:
                candidate["sha256"] = "0" * 64
                mutations += 1
        self.assertGreater(mutations, 0)
        context = context_for(RUN_ID + "-fault")
        with patch.object(source.art, "read_record", return_value=document):
            with self.assertRaises(ValueError):
                source.produce_source(ASSET, COLLECTION, context)
        self.assertFalse((context.reports / "derived.json").exists())

    def test_selected_source_decoder_rejects_pixels_coverage_and_aliases(self):
        """Exercise real selected launch/rocket decoder refusals against retained fixtures.

        @return None; all pixel/metadata faults are detached in-memory copies.
        """
        if COLLECTION != "launch":
            self.skipTest("this case covers the two launch/rocket source decoders")
        report = json.loads((self.context.reports / "derived.json").read_text(encoding="utf-8"))
        producer = importlib.import_module(Path(report["source_producer"]["path"]).stem)
        with no_old_proofs(ROOT):
            with Image.open(ROOT / report["native_source"]["path"]) as atlas:
                changed = copy.deepcopy(report)
                changed["sprites"].pop()
                with self.assertRaises(ValueError):
                    producer.decode(changed, atlas=atlas, context=self.context)
                changed = copy.deepcopy(report)
                row = next(row for row in changed["sprites"] if row["alias_of"] is not None)
                row["alias_of"]["direction"] = "foreign-heading"
                with self.assertRaises(ValueError):
                    producer.decode(changed, atlas=atlas, context=self.context)
                altered = atlas.copy()
                point = next((x, y) for y in range(atlas.height) for x in range(atlas.width) if atlas.getpixel((x, y)))
                altered.putpixel(point, (altered.getpixel(point) + 1) % 256)
                with self.assertRaises(ValueError):
                    producer.decode(report, atlas=altered, context=self.context)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--asset", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--collection", choices=("space", "orbital", "launch"), default="space")
    parser.add_argument("--case", choices=("all", "decoder"), default="all")
    arguments = parser.parse_args()
    ASSET, RUN_ID = arguments.asset, arguments.run_id
    COLLECTION = arguments.collection
    selected = [__file__]
    if arguments.case == "decoder":
        selected.append("SpaceSourceTests.test_selected_source_decoder_rejects_pixels_coverage_and_aliases")
    unittest.main(argv=selected, verbosity=2)
