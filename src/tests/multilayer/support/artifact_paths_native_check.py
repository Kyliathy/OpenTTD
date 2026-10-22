"""Compare the actual Python/native path owners without building the game.

@file artifact_paths_native_check.py Explicit native probe, isolated source roots.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group10.
Compile artifact_paths_probe.cpp with the actual src/tests/test_artifact_paths.cpp;
keep the executable/intermediates and this test's scratch under test-cache/.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from artifact_paths import create_run_context, repository_root, resolve_input_path

APPLICATION = None


class NativePathChecks(unittest.TestCase):
    """Exercise real native input/routing APIs against independent Python results.

    @param self Active isolated fixture test, with no gameplay/native world code.
    """

    def setUp(self):
        """Create one explicit source/invocation shape inside disposable scratch.

        @param self Active test instance.
        @return None; user fixtures and original native applications stay untouched.
        """
        cache = repository_root() / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="native-routing-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "src/tests/fixtures").mkdir(parents=True)
        (self.root / "CMakeLists.txt").write_text("# @see r2fc-file-cleanup3.md; routing-only fixture.\n", encoding="utf-8")
        environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
        self.environment = patch.dict(os.environ, environment, clear=True)
        self.environment.start()
        self.addCleanup(self.environment.stop)
        self.context = create_run_context("tbm", "routing-check", root=self.root)

    def invoke(self, *arguments, context=None):
        """Invoke the explicitly built real owner with one isolated process identity.

        @param self Active isolated test.
        @param arguments Structured probe mode/values, never shell command text.
        @param context Optional host/client feature view; defaults to the main view.
        @return Completed process with bounded runtime and captured diagnostics.
        """
        selected = context or self.context
        return subprocess.run([str(APPLICATION), *map(str, arguments)], env=selected.environment(),
                              capture_output=True, text=True, timeout=15)

    def test_all_roles_and_mixed_features_match_python(self):
        """Every actual native directory agrees with its independent Python owner.

        @param self Active isolated test.
        @return None; mixed features share a run/runtime but own separate outputs.
        """
        result = self.invoke("paths")
        self.assertEqual(result.returncode, 0, result.stderr)
        document = json.loads(result.stdout)
        # Compare every generated role, including newly materialized directories,
        # rather than checking only field presence or equal filename suffixes.
        for feature in ("tbm", "geology"):
            context = self.context.for_feature(feature, "routing-check")
            for role in ("images", "saves", "data", "reports", "evidence", "scratch", "logs"):
                expected = context.output(role, "sample.bin").parent
                self.assertEqual(Path(document[feature][role]), expected)
                self.assertTrue(expected.is_dir())
        self.assertEqual(document["tbm"]["runtime_dir"], document["geology"]["runtime_dir"])
        self.assertNotEqual(document["tbm"]["data"], document["geology"]["data"])

    def test_host_client_and_wrong_runtime_identity(self):
        """Native peers inherit process isolation and reject a foreign runtime home.

        @param self Active isolated test.
        @return None; one process cannot claim another process's writable state.
        """
        host = self.context.for_process("host")
        client = self.context.for_process("client")
        first, second = self.invoke("paths", context=host), self.invoke("paths", context=client)
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(second.returncode, 0, second.stderr)
        self.assertEqual(Path(json.loads(first.stdout)["tbm"]["runtime_dir"]), host.runtime_dir)
        self.assertEqual(Path(json.loads(second.stdout)["tbm"]["runtime_dir"]), client.runtime_dir)
        self.assertNotEqual(host.runtime_dir, client.runtime_dir)
        with patch.dict(os.environ, {"OTTD_TEST_RUNTIME_DIR": str(client.runtime_dir)}):
            environment = host.environment()
            environment["OTTD_TEST_RUNTIME_DIR"] = str(client.runtime_dir)
            refused = subprocess.run([str(APPLICATION), "paths"], env=environment, capture_output=True, text=True, timeout=15)
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("runtime directory disagrees", refused.stderr)

    def test_alias_precedence_hash_tamper_and_missing_retained(self):
        """Python and native choose retained bytes even with a stale old file present.

        @param self Active isolated test.
        @return None; both readers fail on missing/tampered retained copies.
        """
        old = self.context.output("saves", "original.sav")
        old.write_bytes(b"stale original")
        retained = self.root / "savegames/retained/original.sav"
        retained.parent.mkdir(parents=True)
        retained.write_bytes(b"retained independent original")
        key = old.relative_to(self.root).as_posix()
        manifest = dict(schema_version=2, alias_precedence="retained_before_existing", inputs={key:
            dict(path=retained.relative_to(self.root).as_posix(), sha256=hashlib.sha256(retained.read_bytes()).hexdigest())})
        (self.root / "src/tests/fixtures/multilayer-inputs.json").write_text(json.dumps(manifest), encoding="utf-8")
        accepted = self.invoke("input", key)
        self.assertEqual(accepted.returncode, 0, accepted.stderr)
        self.assertEqual(Path(json.loads(accepted.stdout)["path"]), resolve_input_path(key, self.root))
        retained.write_bytes(b"tampered retained")
        self.assertNotEqual(self.invoke("input", key).returncode, 0)
        with self.assertRaisesRegex(ValueError, "hash changed"):
            resolve_input_path(key, self.root)
        retained.unlink()
        self.assertNotEqual(self.invoke("input", key).returncode, 0)

    def test_prior_run_traversal_and_capture_role_guards(self):
        """Explicit data remains readable; frame provenance still requires images/run.

        @param self Active isolated test.
        @return None; new roles never become a fixture write or frame-owner bypass.
        """
        source = self.context.output("data", "selected.bin")
        source.write_bytes(b"explicit generated prerequisite")
        accepted = self.invoke("input", source)
        self.assertEqual(accepted.returncode, 0, accepted.stderr)
        self.assertEqual(Path(json.loads(accepted.stdout)["path"]), source)
        self.assertNotEqual(self.invoke("output", "data", "../fixture.bin").returncode, 0)
        self.assertNotEqual(self.invoke("output", "fixtures", "input.sav").returncode, 0)
        image = self.context.output("images", "routing-only.png")
        image.write_bytes(b"routing-only fixture; no pixels claimed")
        self.assertEqual(self.invoke("record", image).returncode, 0)
        self.assertNotEqual(self.invoke("record", source).returncode, 0)
        other = create_run_context("tbm", "routing-check", root=self.root)
        foreign = other.output("images", "routing-only.png")
        foreign.write_bytes(b"other invocation")
        self.assertNotEqual(self.invoke("record", foreign).returncode, 0)


def main():
    """Require an explicitly built native owner probe before the focused cases run.

    @return None; unittest reports failures with no game build or simulation claim.
    """
    global APPLICATION
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--application", type=Path, required=True)
    args, remaining = parser.parse_known_args()
    APPLICATION = args.application.resolve()
    unittest.main(argv=[sys.argv[0], *remaining], verbosity=2)


if __name__ == "__main__":
    main()
