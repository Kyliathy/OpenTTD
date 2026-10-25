"""Verify imported historical/current evidence transport without claiming native gameplay.

@file imported_evidence_roles_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Synthetic reports/images never execute. Fixtures persist under test-cache/merge-evidence;
the actual source inventory, alias resolver, catalogue and role validators are used.
"""
import copy
import json
from pathlib import Path
import unittest
import uuid

from artifact_paths import RunContext
from artifact_rebuild import identity
from native_build import source_epoch
from prior_stage_evidence import (IMPORTED_FAMILIES, generated_evidence_inputs, publish_relocation,
                                  read_imported_evidence, retained_reference)

ROOT = Path(__file__).resolve().parents[4]


class ImportedEvidenceRoles(unittest.TestCase):
    """Reject stale/undeclared dependencies while keeping sealed history byte exact."""

    def write(self, relative, value):
        """Create one explicit synthetic fixture and return its real byte identity.

        @param self Active isolated transport fixture.
        @param relative Path confined to this test's persistent generated root.
        @param value JSON metadata or literal text for an identity-only source/image.
        @return Exact path/hash; no fixture represents actual compiled gameplay.
        """
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value, indent=2) if isinstance(value, dict) else value, encoding="utf-8")
        return identity(path, self.root)

    def setUp(self):
        """Build a minimal explicit source/catalogue/epoch and typed supplier fixture.

        @param self Active role-boundary test case; no recursive cleanup is registered.
        """
        self.root = ROOT / "test-cache/merge-evidence" / uuid.uuid4().hex
        self.context = RunContext(self.root, "ground", "role-fixture", "current-run").for_feature("ground", "role-fixture")
        self.write("CMakeLists.txt", "# @see archi-map-layers.md; transport-only fixture.\nadd_executable(openttd_test fixture.cpp)\n")
        self.write("vcpkg.json", {"_documentation": "@see r2fc-file-cleanup3.md; identity fixture."})
        self.write("source.dat", "@see r2fc-file-cleanup3.md; identity-only archive fixture.\n")
        self.profile = self.write("src/tests/fixtures/profile.json", {"_documentation": "@see r2fc-file-cleanup3.md; identity fixture.",
            "dependencies": {"source_bundle": {"path": "source.dat"}}})
        self.producer = self.write("src/tests/producer.py", '"""@see r2fc-file-cleanup3.md; transport-only producer fixture, never executed."""\n')
        self.write("src/tests/tags.cpp", '/** @see archi-map-layers.md; transport-only selector fixture. */\n// [.role-fixture]\n')
        self.recipe = dict(id="role-fixture", producer=self.producer["path"], feature="ground", scenario="role-fixture",
            execution="native_fixture", enabled=True, publishes_source=False, stages=["inspect", "build", "prepare", "verify"],
            configuration_required=True, configuration_reuse="forbid", comparison="native_current", target="openttd_test",
            test_tags=["[.role-fixture]"], tag_sources=["src/tests/tags.cpp"], native_timeout_seconds=60,
            retained_inputs=[], requires=[], original_parameters={}, outputs=[dict(role="manifest", path="reports/actual.json")])
        self.catalogue = dict(_documentation="@see r2fc-file-cleanup3.md; isolated transport fixture.", recipe_schema_version=1, recipes=[self.recipe])
        self.write("mlm/review-producers.json", self.catalogue)
        self.image = self.write("build/Debug/openttd_test.exe", "identity-only fixture image; never executed")
        self.epoch = self.write("mlm/reviews/build/epoch/current-run/data/epoch.json", source_epoch(self.root, self.profile["path"]))
        result = dict(_documentation="@see r2fc-file-cleanup3.md; synthetic transport metadata, no compiler ran.", kind="native_build_result", complete=True,
                      configuration="Debug", build_directory="build", targets={"openttd_test": self.image},
                      source_epoch=self.epoch, tool_profile=self.profile, steps=[dict(exit_status=0)])
        self.build = dict(result=self.write("mlm/reviews/build/epoch/current-run/reports/build.json", result))
        self.terminal = self.write(self.relative(self.context.evidence / "required.txt"), "Required fixture transcript; no game process ran.\n")
        process = dict(_documentation="@see r2fc-file-cleanup3.md; synthetic role test only.", run_id="current-run", configuration="Debug",
                       exit_status=0, timed_out=False, root_reaped=True, input_integrity_passed=True,
                       executable=self.image, transcripts=[self.terminal])
        self.process = self.write(self.relative(self.context.reports / "process.json"), process)
        self.report = dict(_documentation="@see r2fc-file-cleanup3.md; synthetic role fixture, not acceptance.", run_id="current-run", configuration="Debug",
                           application=self.image, source=self.producer, required=self.terminal,
                           history={"path": "build/archived/old.exe", "sha256": "1" * 64},
                           optional={"path": "logs/optional-missing.txt", "sha256": "2" * 64})
        self.reference = self.write(self.relative(self.context.reports / "actual.json"), self.report)
        self.stage = dict(_documentation="@see r2fc-file-cleanup3.md; synthetic completed transport fixture.", schema_version=1,
                          kind="artifact_recipe_stage_result", complete=True, recipe="role-fixture", stage="verify",
                          run_id="current-run", configuration="Debug", outputs={"manifest": self.reference},
                          sources=[self.producer], inputs={}, facts=dict(checks=["synthetic role contract only; no native case"]))
        request = dict(_documentation="@see r2fc-file-cleanup3.md; synthetic exact transport request.", schema_version=1,
                       kind="artifact_recipe_stage_request", recipe="role-fixture", stage="verify", declaration=self.recipe,
                       run_id="current-run", configuration="Debug", outputs={"manifest": self.reference["path"]},
                       result_path=self.relative(self.context.reports / "verify-result.json"), sources=[self.producer], inputs={},
                       publishes_source=False, paid_work_authorized=False)
        self.stage["request"] = self.write(self.relative(self.context.reports / "request.json"), request)
        self.supplier = dict(recipe="role-fixture", configuration="Debug",
                             result=self.write(self.relative(self.context.reports / "verify-result.json"), self.stage))
        self.bindings = [dict(pointer="/" + key, role=role) for key, role in
                         (("application", "current_executable"), ("source", "current_source"), ("required", "current_output"),
                          ("history", "historical_fact"), ("optional", "diagnostic"))]

    def relative(self, path):
        """Spell an already contained fixture path relative to its own source root.

        @param self Active fixture root owner.
        @param path Concrete generated fixture path selected by this test.
        @return Portable relative pathname.
        """
        return path.relative_to(self.root).as_posix()

    def current(self, **changes):
        """Invoke the actual current role adapter with explicitly selected fixture witnesses.

        @param self Active role test; output is transport only, never gameplay acceptance.
        @param changes Deliberate argument mutations for an individual refusal case.
        @return Original report with separate role/process observations.
        """
        arguments = dict(bindings=self.bindings, suppliers=[self.supplier], configuration="Debug", build=self.build,
                         recipe=self.recipe, native_processes=[self.process])
        arguments.update(changes)
        return read_imported_evidence("normal-start", self.reference, self.context, **arguments)

    def test_all_readers_preserve_sealed_history_and_publish_separate_relocation(self):
        """Each imported family can inspect retained history without following vanished old images.

        @param self Active exact historical relocation/alias test.
        """
        retained = self.write("src/tests/fixtures/original.json", self.report)
        original = dict(retained, path="logs/old/report.json")
        self.write("src/tests/fixtures/multilayer-inputs.json", {"schema_version": 2, "alias_precedence": "retained_before_existing", "inputs": {original["path"]: retained}})
        before = (self.root / retained["path"]).read_bytes()
        for family in IMPORTED_FAMILIES:
            result = read_imported_evidence(family, original, self.context, historical=True)
            self.assertEqual(result["report"], self.report)
            self.assertFalse(result["transport_verified"])
            relocation = publish_relocation(self.context, family + ".json", family, original)
            self.assertEqual(json.loads((self.root / relocation["path"]).read_text())["relocation"]["original"], original)
        self.assertEqual((self.root / retained["path"]).read_bytes(), before)
        with self.assertRaises(ValueError):
            retained_reference(dict(original, sha256="3" * 64), self.context)
        with self.assertRaises(ValueError):
            retained_reference(dict(original, path="logs/wrong/alias.json"), self.context)

    def test_current_roles_allow_missing_diagnostic_but_require_native_transcript(self):
        """Required proof survives explicit roles while an optional absent log stays optional.

        @param self Active synthetic current transport test, with no native acceptance claim.
        """
        result = self.current()
        self.assertTrue(result["transport_verified"])
        self.assertFalse(result["gameplay_acceptance"])
        (self.root / self.terminal["path"]).rename(self.root / (self.terminal["path"] + ".absent"))
        with self.assertRaisesRegex(ValueError, "missing live"):
            self.current()

    def test_previous_run_missing_supplier_and_wrong_recipe_configuration_refuse(self):
        """A matching output hash cannot replace an explicit completed current supplier.

        @param self Active generated prerequisite binding refusal test.
        """
        with self.assertRaisesRegex(ValueError, "unbound generated"):
            self.current(suppliers=[])
        wrong = copy.deepcopy(self.supplier)
        wrong["configuration"] = "RelWithDebInfo"
        with self.assertRaisesRegex(ValueError, "recipe/run/configuration"):
            self.current(suppliers=[wrong])
        stage = dict(self.stage, run_id="previous-run")
        wrong["configuration"] = "Debug"
        wrong["result"] = self.write(self.relative(self.context.reports / "wrong-stage.json"), stage)
        with self.assertRaisesRegex(ValueError, "recipe/run/configuration"):
            self.current(suppliers=[wrong])

    def test_stale_binary_or_source_inventory_refuses_current_transport(self):
        """Actual current-build validation remains enforced rather than dated wholesale.

        @param self Active current executable/source refusal test.
        """
        self.write(self.image["path"], "changed image")
        with self.assertRaisesRegex(ValueError, "input/image changed"):
            self.current()
        self.write(self.image["path"], "identity-only fixture image; never executed")
        self.write("src/new.cpp", "/** @see archi-map-layers.md; changed source membership fixture. */\n")
        with self.assertRaisesRegex(ValueError, "source inventory"):
            self.current()

    def test_undeclared_reference_and_historical_as_current_refuse(self):
        """Every nested dependency needs an explicit role and history never gains current status.

        @param self Active undeclared-dependency/historical-promotion refusal test.
        """
        with self.assertRaisesRegex(ValueError, "undeclared"):
            self.current(bindings=self.bindings[:-1])
        bindings = copy.deepcopy(self.bindings)
        next(row for row in bindings if row["pointer"] == "/history")["role"] = "current_executable"
        with self.assertRaisesRegex(ValueError, "stale archived"):
            self.current(bindings=bindings)
        with self.assertRaisesRegex(ValueError, "historical evidence"):
            read_imported_evidence("normal-start", self.reference, self.context, historical=True, configuration="Debug")

    def test_supplier_current_sources_and_native_status_cannot_be_dated(self):
        """Rehashed output metadata cannot excuse stale supplier code or a failed command.

        @param self Active exact supplier/source/process witness refusal check.
        """
        self.write(self.producer["path"], "changed supplier source")
        with self.assertRaisesRegex(ValueError, "current source/input changed"):
            generated_evidence_inputs(self.context, [self.supplier], "Debug")
        self.write(self.producer["path"], '"""@see r2fc-file-cleanup3.md; transport-only producer fixture, never executed."""\n')
        process = json.loads((self.root / self.process["path"]).read_text())
        process["root_reaped"] = False
        changed = self.write(self.process["path"], process)
        with self.assertRaisesRegex(ValueError, "status is unbound"):
            self.current(native_processes=[changed])

    def test_disabled_provider_cannot_be_enabled_by_a_supplier_receipt(self):
        """A complete-looking verify result never implements a pending native provider.

        @param self Active disabled-interface refusal at the sole catalogue boundary.
        """
        self.catalogue["recipes"][0]["enabled"] = False
        self.catalogue["recipes"][0]["pending_groups"] = [63]
        self.write("mlm/review-producers.json", self.catalogue)
        with self.assertRaisesRegex(ValueError, "recipe/run/configuration"):
            generated_evidence_inputs(self.context, [self.supplier], "Debug")


if __name__ == "__main__":
    unittest.main(verbosity=2)
