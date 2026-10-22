"""Run one bounded actual M1 source family's conversion and refusal checks.

@file m1_source_conversion_check.py Select one stage per invocation; no full regression.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17.
Expected approved pixel hashes come from the existing public contract, independently
of the converter under test. New candidates never update those expectations.
"""

import argparse
import copy
import hashlib
import importlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from artifact_paths import RunContext, create_run_context, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_art_sources as conversion

STAGE = None
RUN_ID = None


class SourceConversionTests(unittest.TestCase):
    """Exercise one actual source producer and its independent approved expectations."""

    @classmethod
    def setUpClass(cls):
        """Select a fresh invocation or explicitly join the requested shared test run.

        @return None; generated candidates remain in their declared review owner.
        """
        if RUN_ID is None:
            cls.context = create_run_context("multilayer-art", "m1-source-" + STAGE)
        else:
            marker = ROOT / "mlm/runtime/.runs" / RUN_ID
            if not marker.is_dir():
                create_run_context("multilayer-art", "m1-source-" + STAGE, run_id=RUN_ID)
            cls.context = RunContext(ROOT, "multilayer-art", "m1-source-" + STAGE, RUN_ID)
        cls.public = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
        cls.dependencies = {}
        if STAGE in ("vehicle", "purchase", "control"):
            predecessor = RunContext(ROOT, "multilayer-art", "m1-source-base", cls.context.run_id)
            manifest = predecessor.reports / "derived.json"
            if RUN_ID is None:
                conversion.produce("base", predecessor)
            cls.dependencies["m1-source-base"] = conversion.identity(manifest)

    def test_actual_source_repeats_exactly_and_preserves_approved_pixels(self):
        """Compare repeat outputs and existing independent authored pixel hashes.

        @return None; retain source candidates for the selected packing closure.
        """
        first = conversion.produce(STAGE, self.context, dependencies=self.dependencies)
        original_report = (self.context.reports / "derived.json").read_bytes()
        original_images = {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in self.context.images.iterdir() if path.is_file()}
        second = conversion.produce(STAGE, self.context, dependencies=self.dependencies)
        self.assertEqual(first["artifacts"], second["artifacts"])
        self.assertEqual(original_report, (self.context.reports / "derived.json").read_bytes())
        self.assertEqual(original_images, {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in self.context.images.iterdir() if path.is_file()})
        aliases = {"base": "atlas-manifest.json", "collector": "collector-source-manifest.json", "hardness": "hardness-manifest.json"}
        alias = aliases.get(STAGE, STAGE + "-details-manifest.json")
        expected = {row["authoring"]["native_atlas"]: row["authoring"]["native_sha256"]
                    for row in self.public["assets"] if Path(row.get("authoring", {}).get("atlas_manifest", "")).name == alias}
        artifacts = {Path(row["path"]).name: row["sha256"] for row in second["artifacts"]}
        self.assertTrue(expected, "selected source has no independent public pixel identities")
        # Every current family from this producer retains its approved atlas;
        # repeat determinism alone cannot legitimize a changed source image.
        for path, digest in expected.items():
            self.assertEqual(artifacts[Path(path).name], digest, path)
        summary = dict(_documentation=__doc__, stage=STAGE, run_id=self.context.run_id,
                       source_manifest=dict(path=(self.context.reports / "derived.json").relative_to(ROOT).as_posix(), sha256=hashlib.sha256(original_report).hexdigest()),
                       approved_atlases=len(expected), generated_images=len(original_images), exact_repeat=True,
                       source_files=first["source_files"], publishes_source=False)
        target = ROOT / "test-cache/cleanup" / ("m1-source-" + STAGE + "-verification.json")
        target.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    def test_changed_encoder_and_source_are_refused(self):
        """Refuse a wrong encoder and a malformed copy of an original source record.

        @return None; no authored source bytes or expected baseline are modified.
        """
        with patch.object(conversion.PIL, "__version__", "0.0-unit-mismatch"):
            with self.assertRaisesRegex(ValueError, "toolchain"):
                conversion.check_tools()
        module = conversion.base if STAGE == "base" else importlib.import_module("convert_geothermal_art" if STAGE == "collector" else "convert_" + STAGE + "_art")
        source = module.ASSETS / "generation-record.json" if STAGE == "base" else module.RECORD
        document = copy.deepcopy(json.loads(source.read_text(encoding="utf-8")))
        pending, mutations = [document], 0
        # Source record schemas differ (locked UI source, controls, facilities,
        # original candidates). Corrupt only copied identities in every schema;
        # whichever real source the producer selects must then be refused.
        while pending:
            value = pending.pop()
            if isinstance(value, dict):
                # Corrupt actual source identities only; arbitrary status text
                # and geometry records are not substitutes for a changed input.
                if "path" in value and "sha256" in value:
                    value["sha256"] = "0" * 64
                    mutations += 1
                pending.extend(value.values())
            elif isinstance(value, list):
                pending.extend(value)
        self.assertGreater(mutations, 0)
        scratch = ROOT / "test-cache/cleanup/m1-source-faults"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as temporary:
            altered = Path(temporary) / "generation-record.json"
            altered.write_text(json.dumps(document), encoding="utf-8")
            attribute, replacement = ("ASSETS", altered.parent) if STAGE == "base" else ("RECORD", altered)
            with patch.object(module, attribute, replacement):
                with self.assertRaises((ValueError, AssertionError)):
                    if STAGE == "base":
                        module.build(cache_only=True, context=self.context)
                    elif STAGE == "collector":
                        module.build_collector()
                    elif STAGE == "facility":
                        # Facility's original input default was bound at import;
                        # exercise the actual malformed document explicitly.
                        module.derive(document=document, source_record=altered, output_dir=self.context.images)
                    else:
                        module.derive()

    def test_actual_source_requires_no_old_proof_files(self):
        """Run the original producer while old log/proof I/O is explicitly refused.

        @return None; selected predecessor images remain sufficient on their own.
        """
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(AssertionError, "old proof/log"):
                (ROOT / "logs/forbidden-source-probe.log").read_bytes()
            conversion.produce(STAGE, self.context, dependencies=self.dependencies)

    def test_foreign_predecessor_is_refused_before_conversion(self):
        """Reject a selected predecessor from a different invocation.

        @return None; source families without a predecessor have no such edge.
        """
        if not self.dependencies:
            return
        dependencies = copy.deepcopy(self.dependencies)
        reference = dependencies["m1-source-base"]
        foreign = RunContext(ROOT, "multilayer-art", "m1-source-base", "foreign-source-run")
        reference["path"] = (foreign.reports / "derived.json").relative_to(ROOT).as_posix()
        with self.assertRaisesRegex(ValueError, "foreign invocation"):
            conversion.produce_source(STAGE, self.context, dependencies)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--stage", required=True, choices=conversion.STAGES[:-1])
    parser.add_argument("--run-id", help="explicit same invocation for later packing closure checks")
    parser.add_argument("--case", choices=("all", "current-inputs", "foreign-input"), default="all")
    arguments = parser.parse_args()
    STAGE, RUN_ID = arguments.stage, arguments.run_id
    selected = {"current-inputs": "test_actual_source_requires_no_old_proof_files", "foreign-input": "test_foreign_predecessor_is_refused_before_conversion"}
    cases = ["SourceConversionTests." + selected[arguments.case]] if arguments.case in selected else []
    unittest.main(argv=[__file__, *cases], verbosity=2)
