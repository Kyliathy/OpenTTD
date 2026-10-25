"""Focused source-review scope, independent-policy and upstream process refusal tests.

@file space_source_review_check.py Synthetic negative metadata never certifies native execution.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19.
"""

import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import space_source_review as review


class SourceReviewChecks(unittest.TestCase):
    """Keep current source review separate from native decoding, paid work and old proof chains."""

    def test_upstream_decoder_scope_refuses_unrelated_or_source_only_flags(self):
        """Reject complete-looking outcomes that do not represent this actual decoder scope.

        @param self Active upstream decoder metadata boundary check.
        @return None; all records are detached negative metadata, never execution evidence.
        """
        base = dict(kind="native_sprite_loader_result", complete=True, native_loading_verified=True,
                    family="space", recipe="space-native-loader", configuration="Debug", run_id="source-check",
                    paid_world_loaded=False, publishes_source=False)
        # Each independent scope change must fail before any real image or
        # process is selected; a plausible complete flag grants no acceptance.
        for key, value in (("kind", "selected_sprite_expectations"), ("native_loading_verified", False),
                           ("family", "orbital"), ("recipe", "orbital-native-loader"),
                           ("configuration", "RelWithDebInfo"), ("run_id", "other-run"),
                           ("paid_world_loaded", True), ("publishes_source", True)):
            with self.subTest(field=key), self.assertRaisesRegex(ValueError, "exact actual current native"):
                review.validate_native_manifest(dict(base, **{key: value}), "space", "Debug", "source-check")

    def test_upstream_process_refuses_stale_or_unbound_execution(self):
        """Require actual source binding, case selection and successful terminal to agree.

        @param self Active physical-process metadata consistency check.
        @return None; no synthetic record is passed to native execution or published as proof.
        """
        payload = dict(path="generated.bin", sha256="0" * 64)
        manifest = dict(path="source.json", sha256="1" * 64)
        candidate = dict(path="candidate.grf", sha256="3" * 64)
        checksum = dict(path="candidate.grf.hash", sha256="4" * 64)
        native = dict(family="space", configuration="Debug", run_id="source-check", expected_payload=payload,
                      expected_manifest=manifest, application=dict(path="Debug/test.exe", sha256="2" * 64),
                      actual_candidate=candidate, candidate_checksum=checksum,
                      tag="[.space-packed-art-load]", assertions=2, marker="original decoder marker")
        binding = dict(role="expected_sprites", family="space", configuration="Debug", **payload, source_manifest=manifest)
        process = dict(exit_status=0, input_integrity_passed=True, configuration="Debug", run_id="source-check",
                       executable=native["application"], argv=["test.exe", native["tag"], "--reporter", "console", "--durations", "yes"],
                       inputs=[payload, candidate, checksum], bound_inputs=[binding])
        text = "All tests passed (2 assertions in 1 test case)\noriginal decoder marker\n"
        # This detached positive control checks only the exact process-input
        # layout; the native loader owner separately verifies real opened bytes.
        review.validate_native_process(native, process, text)
        # Faults are independent: failed exit, changed image/configuration,
        # unrelated case, missing source input and broken closing integrity.
        for key, value in (("exit_status", 1), ("configuration", "RelWithDebInfo"),
                           ("executable", {}), ("argv", ["test.exe", "[space-art]"]),
                           ("inputs", []), ("inputs", [payload]), ("inputs", [payload, candidate]),
                           ("inputs", [payload, checksum]), ("inputs", [payload, candidate, checksum, manifest]),
                           ("bound_inputs", []), ("input_integrity_passed", False)):
            damaged = copy.deepcopy(process)
            damaged[key] = value
            with self.subTest(field=key), self.assertRaisesRegex(ValueError, "process/source/terminal"):
                review.validate_native_process(native, damaged, text)
        with self.assertRaisesRegex(ValueError, "process/source/terminal"):
            review.validate_native_process(native, process, "original decoder marker only")

    def test_original_policy_does_not_dereference_historical_proof_chains(self):
        """Read the original independent policy while denying all dated executable/log reads.

        @param self Active independent source-policy access boundary check.
        @return None; the actual immutable policy is read without changing its bytes.
        """
        recipe = review.selected_recipe("launch")
        inputs = review.verify_retained_inputs(recipe, review.ROOT)
        allowed = {review.ROOT / inputs[role]["path"] for role in ("launch-owner-contract", "launch-policy")}
        original = Path.read_text

        def guarded(path, *args, **kwargs):
            """Reject undeclared proof-chain reads during independent policy selection.

            @param path Actual requested repository path.
            @param args Original positional text-reading options.
            @param kwargs Original named decoding options.
            @return Actual original independent policy/fixture bytes as decoded text.
            """
            if path not in allowed:
                raise AssertionError("unexpected source-policy dependency: " + str(path))
            return original(path, *args, **kwargs)

        with patch.object(Path, "read_text", guarded):
            policy = review.launch_policy(dict(inputs=inputs))
        self.assertEqual(policy["native_budget"]["flight_ticks"], 222)
        self.assertEqual(policy["frame_rule"]["opportunities_per_frame"], 8)

    def test_named_selector_attempt_cannot_escape_prior_evidence(self):
        """Reject a repeat name before reading or selecting native/source prerequisites.

        @param self Active explicit repeat namespace boundary check.
        @return None; no source, runtime or earlier observation is changed.
        """
        with patch.object(review, "selected_recipe", side_effect=AssertionError("unsafe repeat selected sources")):
            with self.assertRaises(ValueError):
                review.execute_prepared("launch", "Debug", {}, {}, attempt="../old-proof")

    def test_viewport_prerequisite_rejects_wrong_scope_before_build_selection(self):
        """Keep source-only, other-family or paid outcomes out of native viewport preparation.

        @param self Active current-selector consumer metadata boundary check.
        @return None; temporary records are negative metadata, never native proof.
        """
        cache = review.ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        # Persistent owned refusal fixtures avoid recursive cleanup of any repository path.
        root = Path(tempfile.mkdtemp(prefix="selector-consumer-", dir=cache))
        path = root / "mlm/reviews/negative.json"
        path.parent.mkdir(parents=True)
        base = dict(_documentation="Owner: r2fc-file-cleanup3.md19B; synthetic refusal metadata only.",
                    kind="native_source_selector_result", complete=True, source_selectors_verified=True,
                    family="launch", configuration="Debug", paid_world_loaded=False,
                    viewport_captured=False, publishes_source=False)
        # Every independent altered scope must refuse before selecting an
        # actual build or loading a historical saved world for its viewport.
        for key, value in (("kind", "selected_space_source"), ("complete", False),
                           ("source_selectors_verified", False), ("family", "orbital"),
                           ("configuration", "RelWithDebInfo"), ("paid_world_loaded", True),
                           ("viewport_captured", True), ("publishes_source", True)):
            path.write_text(json.dumps(dict(base, **{key: value})), encoding="utf-8")
            reference = dict(path=path.relative_to(root).as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            with patch.object(review, "selected_recipe", side_effect=AssertionError("wrong scope selected current build")):
                with self.subTest(field=key), self.assertRaisesRegex(ValueError, "exact current source-selector"):
                    review.checked_result(reference, "launch", "Debug", root)


if __name__ == "__main__":
    unittest.main()
