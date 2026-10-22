"""Verify one selected original expectation producer per bounded invocation.

@file sprite_expected_payload_check.py Requires explicit completed packing and prefix inputs.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18.
No historical binary, decoder output or newly generated expected baseline is used.
"""

import argparse
import copy
import hashlib
import importlib
import json
from pathlib import Path
import sys
import unittest

from artifact_paths import RunContext, contained
from artifact_rebuild import identity
from source_input_guard import no_old_proofs
import sprite_expected_payload as source

FAMILY = RUN_ID = RECIPE = None
PREFIXES = {}


class SpriteExpectationTests(unittest.TestCase):
    """Compare actual original payload algorithms with independently preserved pre-edit bytes."""

    @classmethod
    def setUpClass(cls):
        """Bind actual packing outputs from an explicit completed shared-executor result.

        @param cls Active one-family focused source expectation suite.
        @return None; arbitrary files, old cache paths and latest-run lookup cannot supply packing.
        """
        name = source.recipe_family(FAMILY)
        path = source.ROOT / "mlm/reviews/rebuild" / RECIPE / RUN_ID / "reports/rebuild.json"
        state = json.loads(path.read_text(encoding="utf-8"))
        # The chosen root and supplying packing leaf must actually have passed.
        # A stale complete flag or a copied source file cannot replace its witness.
        if state["complete"] is not True or state["status"] != "verified" or state["run_id"] != RUN_ID:
            raise ValueError("expectation checks require the explicit completed packing recipe")
        node = state["nodes"][name + "-derived-inputs"]
        if node["status"] != "verified":
            raise ValueError("selected original packing supplier is incomplete")
        cls.reference = node["outputs"]["manifest"]
        cls.report, cls.image, declaration = source.packing_inputs(FAMILY, cls.reference, cls.context("source-check-inputs"))
        producer = source.ROOT / declaration["producer"]
        sys.path.insert(0, str(producer.parent))
        cls.module = importlib.import_module(producer.stem)
        cls.expected = source.load_facts()[FAMILY]
        cls.prefixes = {}
        # An explicitly selected complete expectation closure supplies its
        # actual typed prefix outputs. Read declared dependency roles, never
        # latest files or guessed filenames. Earlier pure-algorithm checks may
        # instead name independently checked generated binaries explicitly.
        declaration = state["signature"]["declarations"].get(name + "-expected-payload")
        if not PREFIXES and declaration is not None:
            for requirement in declaration["requires"]:
                if requirement["as"].endswith("-prefix"):
                    supplier = requirement["recipe"]
                    family = state["signature"]["declarations"][supplier]["original_parameters"]["family"]
                    reference = state["nodes"][supplier]["outputs"]["manifest"]
                    cls.prefixes[family] = source.checked_payload(family, reference, cls.context("typed-prefix-inputs"))
        # Prefixes supplied to the pure algorithm may come from earlier focused
        # source tests. Their independent bytes are checked here; the actual
        # shared recipe separately enforces same-run typed prefix provenance.
        for family, path in PREFIXES.items():
            actual = contained(path, source.ROOT / "mlm/reviews")
            if identity(actual, source.ROOT)["sha256"] != source.load_facts()[family]["sha256"]:
                raise ValueError("focused source prefix differs from independent original bytes")
            cls.prefixes[family] = actual

    @classmethod
    def context(cls, scenario, run_id=None):
        """Select a bounded current test output without reserving another source invocation.

        @param cls Active one-family test class.
        @param scenario Fixed test scenario under the existing feature owner.
        @param run_id Optional deliberate foreign-run identity for a refusal.
        @return Explicit generated context; current sources retain their own scenarios.
        """
        return RunContext(source.ROOT, "multilayer-art", source.recipe_family(FAMILY) + "-expectations-" + scenario, run_id or RUN_ID)

    def test_actual_original_producer_repeats_exact_pre_edit_payload(self):
        """Execute original source math twice with historical proof/log access refused.

        @param self Active original binary reproduction check.
        @return None; exact old payload identity predates every changed producer.
        """
        paths = []
        canonical = source.ROOT / "media/baseset/openttd.grf"
        before = hashlib.sha256(canonical.read_bytes()).hexdigest()
        # Independent invocations of the pure source algorithm must reproduce
        # every original record and pixel, including all protected prefix slots.
        for scenario in ("first", "repeat"):
            path = self.context(scenario).data / "source-expected.bin"
            with no_old_proofs(source.ROOT):
                count = self.module.produce_selected(self.report, self.image, path, prefixes=self.prefixes)
            self.assertEqual(count, self.expected["record_count"])
            self.assertEqual(path.stat().st_size, self.expected["bytes"])
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), self.expected["sha256"])
            paths.append(path)
        self.assertEqual(paths[0].read_bytes(), paths[1].read_bytes())
        self.assertEqual(hashlib.sha256(canonical.read_bytes()).hexdigest(), before)

    def test_foreign_changed_and_wrong_family_packing_refuse(self):
        """Exercise actual current input roles and semantic family checks with coherent faults.

        @param self Active original source selection refusal check.
        @return None; genuine packing and shipping files remain unchanged.
        """
        changed = dict(self.reference, sha256="0" * 64)
        with no_old_proofs(source.ROOT):
            with self.assertRaisesRegex(ValueError, "identity changed"):
                source.packing_inputs(FAMILY, changed, self.context("changed"))
            with self.assertRaisesRegex(ValueError, "foreign invocation"):
                source.packing_inputs(FAMILY, self.reference, self.context("foreign", RUN_ID + "-foreign"))
            wrong = "space" if FAMILY == "m1" else "m1"
            with self.assertRaisesRegex(ValueError, "family/range/status"):
                source.packing_inputs(wrong, self.reference, self.context("wrong-family"))
        report = copy.deepcopy(self.report)
        report["sprites"][0]["native_offset"] += 1
        destination = self.context("bad-pose").data / "source-expected.bin"
        # A coherent source-report edit cannot become its own expected baseline.
        # Strict original order gates may fail earlier; otherwise independent
        # old binary identity must expose the altered native geometry record.
        try:
            with no_old_proofs(source.ROOT):
                self.module.produce_selected(report, self.image, destination, prefixes=self.prefixes)
        except ValueError:
            pass
        else:
            self.assertNotEqual(hashlib.sha256(destination.read_bytes()).hexdigest(), self.expected["sha256"])

    def test_selected_output_and_source_prefix_roles_are_explicit(self):
        """Refuse wrong generated destinations and missing/foreign source prefix families.

        @param self Active output/prefix boundary check.
        @return None; no old cache or canonical source destination is touched.
        """
        from multilayer_packed_art_fixture import write_selected_payload
        with self.assertRaisesRegex(ValueError, "generated data file"):
            write_selected_payload(self.context("wrong-output").reports / "expected.bin", b"fault")
        with self.assertRaisesRegex(ValueError, "escapes"):
            write_selected_payload(source.ROOT / "media/expected-fault.bin", b"fault")
        with no_old_proofs(source.ROOT):
            with self.assertRaisesRegex(ValueError, "exact declared source prefix families"):
                source.produce(FAMILY, self.context("wrong-prefix"), self.reference, {"foreign": self.reference})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", choices=tuple(source.load_facts()), required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--recipe", help="Completed supplying root; defaults to this family's packing recipe")
    parser.add_argument("--prefix", action="append", default=[], help="Exact family=current-generated-binary path for pure prefix tests")
    arguments = parser.parse_args()
    FAMILY, RUN_ID = arguments.family, arguments.run_id
    RECIPE = arguments.recipe or source.recipe_family(FAMILY) + "-derived-inputs"
    PREFIXES = {key: Path(value).resolve() for key, value in (item.split("=", 1) for item in arguments.prefix)}
    unittest.main(argv=[sys.argv[0]], verbosity=2)
