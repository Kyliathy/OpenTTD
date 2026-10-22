"""Verify native prepared-source scope refusals before any game/build execution.

@file sprite_native_loader_contract_check.py Metadata-only active18C guards, never native proof.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups18C/19.
"""

import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import sprite_native_loader as native


class NativeLoaderContracts(unittest.TestCase):
    """Reject route traversal and attempts to inherit native history as source-byte inputs."""

    def test_malformed_prepared_source_identifiers_refuse_before_selection(self):
        """Prevent explicit input names from traversing source/native output namespaces.

        @param self Active prepared input namespace boundary check.
        @return None; no source file, run marker or native process is created.
        """
        # Both caller-selected namespace components are identifiers. Refuse
        # traversal before even selecting a recipe or reading any source state.
        for run, recipe in (("../foreign", "ground-expected-payload"), ("valid-run", "../foreign")):
            with patch.object(native, "selected_recipe", side_effect=AssertionError("invalid identifiers selected a recipe")):
                with self.assertRaises(ValueError):
                    native.execute_prepared("m1", "RelWithDebInfo", run, recipe, {})

    def test_native_request_never_acquires_paid_or_publication_scope(self):
        """Refuse changed action/configuration scope before reading native build inputs.

        @param self Active native stage authorization boundary check.
        @return None; actual source/native products remain untouched.
        """
        base = dict(stage="verify", configuration="RelWithDebInfo", paid_work_authorized=False, publishes_source=False)
        # Every deliberately changed boundary must fail before selecting any
        # actual image; an existing native result cannot authorize another action.
        for key, value in (("stage", "earn"), ("configuration", "Release"),
                           ("paid_work_authorized", True), ("publishes_source", True)):
            request = dict(base, **{key: value})
            with patch.object(native, "selected_recipe", side_effect=AssertionError("unsafe scope selected a native image")):
                with self.assertRaisesRegex(ValueError, "read-only current recipe scope"):
                    native.checked_request(request, native.ROOT)

    def test_unsafe_attempt_refuses_before_source_or_build_selection(self):
        """Keep repeated native proof names from traversing immutable evidence owners.

        @param self Active native repeat namespace boundary check.
        @return None; no native process or prior result is changed.
        """
        with patch.object(native, "selected_recipe", side_effect=AssertionError("unsafe attempt selected native inputs")):
            with self.assertRaises(ValueError):
                native.execute_prepared("m1", "Debug", "valid-run", "sprite-native-loaders", {}, attempt="../previous")

    def test_prepared_source_cannot_inherit_native_or_cross_configuration_proof(self):
        """Reject complete-looking states that offer native results instead of pure source bytes.

        @param self Active source-only reuse policy check.
        @return None; synthetic negative metadata remains disposable within the review owner.
        """
        owner = native.ROOT / "mlm/reviews/rebuild"
        owner.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="native-source-contract-", dir=owner) as temporary:
            recipe = Path(temporary).name
            run = "native-source-contract-run"
            path = Path(temporary) / run / "reports/rebuild.json"
            path.parent.mkdir(parents=True)
            base = dict(_documentation="Owner: r2fc-file-cleanup3.md18C; deliberate source-contract fault metadata, never a native result.",
                schema_version=1, kind="artifact_rebuild_result", complete=True, status="verified", run_id=run,
                signature=dict(recipe=recipe, declarations={"m1-expected-payload": dict(execution="pure_conversion",
                    configuration_required=False, configuration_reuse="source_bytes_only", comparison="independent_source_bytes")}),
                nodes={"m1-expected-payload": dict(status="verified")})
            # Each altered field breaks the explicit source-byte-only contract.
            # Check this before load_recipe can start any native/build selection.
            for key, value in (("execution", "native_fixture"), ("configuration_required", True),
                               ("configuration_reuse", "forbid"), ("comparison", "current_native_observation")):
                state = copy.deepcopy(base)
                state["signature"]["declarations"]["m1-expected-payload"][key] = value
                path.write_text(json.dumps(state, indent=2) + "\n", encoding="utf-8")
                with patch.object(native, "load_recipe", side_effect=AssertionError("native source history reached build selection")):
                    with self.assertRaisesRegex(ValueError, "pure source-byte prerequisites"):
                        native.execute_prepared("m1", "RelWithDebInfo", run, recipe, {})


if __name__ == "__main__":
    unittest.main(verbosity=2)
