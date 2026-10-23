"""Focused ownership checks for legacy fixture input/output separation.

@file legacy_fixture_paths_check.py No native executable or immutable input is changed.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 5B.
"""

import hashlib
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests"))
import multilayer_state_fixture as owner


class LegacyFixtureOwnership(unittest.TestCase):
    """Preserve old inputs while new and resumed cases select exact output owners.

    @see r2fc-file-cleanup.md tasks 5B.1 and 5B.4.
    """

    def test_absent_reserved_output_does_not_reuse_fixture(self):
        """Require a new stage's actual output even when an old namesake exists.

        @param self Active unittest case.
        @return None; the immutable source identity must remain unchanged.
        """
        name = "r2mlvs14-b-native-new.sav"
        source = owner.save_path(name)
        before = hashlib.sha256(source.read_bytes()).hexdigest()
        case = owner.CONTEXT.for_feature("state", "output-reservation-check")
        output = owner.output_save_path(name, case)
        self.assertEqual(owner.save_path(name, case), output)
        self.assertFalse(output.exists())
        self.assertNotEqual(source, output)
        self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), before)

    def test_same_name_has_separate_feature_owners(self):
        """Prevent imported helpers from redirecting another feature's output.

        @param self Active unittest case.
        @return None; reservations are isolated even inside one invocation.
        """
        left = owner.CONTEXT.for_feature("geology", "reservation-check")
        right = owner.CONTEXT.for_feature("portals", "reservation-check")
        name = "r2mlvs2-g-native.sav"
        a = owner.output_save_path(name, left)
        b = owner.output_save_path(name, right)
        self.assertNotEqual(a, b)
        self.assertEqual(owner.save_path(name, left), a)
        self.assertEqual(owner.save_path(name, right), b)

    def test_prior_selection_refuses_traversal(self):
        """Keep a partial rerun's evidence within its explicitly named case.

        @param self Active unittest case.
        @return None; neither invocation nor basename may escape its owner.
        """
        with self.assertRaises(ValueError):
            owner.prior_artifact(owner.CONTEXT, "../other", "logs", "result.log")
        with self.assertRaises(ValueError):
            owner.prior_artifact(owner.CONTEXT, "historical", "saves", "../source.sav")


if __name__ == "__main__":
    unittest.main()
