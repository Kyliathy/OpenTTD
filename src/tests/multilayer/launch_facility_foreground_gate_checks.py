"""Prevent empty-front launch source matrices from claiming occupied source completion.

@file launch_facility_foreground_gate_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
The retained status-only recipe is an explicit negative fixture. These checks
perform no art generation, public-source writes, native process or world tick.
"""

from copy import deepcopy
import json
import unittest
from unittest.mock import patch

from launch_facility_art_contract_checks import ROOT, read_json
import convert_launch_center_source as source

STATUS_ONLY = ROOT / "media/multilayer/source/space/launch-center-state-art-v1.json"
PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-foreground-v1"


class LaunchForegroundGateChecks(unittest.TestCase):
    """Require occupied-source qualification before derivation or completion publication."""

    def test_status_only_derivation_refuses_before_any_source_output(self):
        """A complete static matrix cannot bypass the original occupied-art requirement.

        @param self Active missing-foreground source prerequisite refusal check.
        @return None.
        """
        # If the guard disappears, abort at the first downstream boundary;
        # the regression test must never rebuild or publish unqualified art.
        with patch.object(source, "POLICY", STATUS_ONLY), patch.object(source.states, "checked_policy", side_effect=AssertionError("unqualified source reached conversion")):
            with self.assertRaisesRegex(ValueError, "occupied foreground source"):
                source.derive()

    def test_status_only_publication_refuses_before_metadata_writes(self):
        """Rehashing a retained partial report cannot turn absent fronts into reviewed cuts.

        @param self Active independent public-source prerequisite refusal check.
        @return None.
        """
        report = deepcopy(read_json(source.OUTPUT / "source-manifest.json"))
        report["policy"] = source.base.identity(STATUS_ONLY)
        # These forged completion booleans deliberately leave the real source
        # recipe unqualified. Its prerequisite outranks count-only assertions.
        report.update(coverage_complete=True, occupied_foregrounds_complete=True)
        with patch.object(source, "write_json", side_effect=AssertionError("unqualified source reached publication")):
            with self.assertRaisesRegex(ValueError, "occupied launch-center foreground review"):
                source.publish(report)


def main():
    """Verify only the active occupied-source prerequisite and retain explicit evidence.

    @return None; exit nonzero on a missing source/publication guard.
    """
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchForegroundGateChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    PROOFS.mkdir(parents=True, exist_ok=True)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA; refusal proof only, physical foreground cuts remain pending.",
                 status="passed", cases=result.testsRun, negative_source_recipe=source.base.identity(STATUS_ONLY),
                 converter=source.base.identity(ROOT / "media/multilayer/convert_launch_center_source.py"),
                 test=source.base.identity(ROOT / "src/tests/multilayer/launch_facility_foreground_gate_checks.py"),
                 occupied_foregrounds_complete=False, source_coverage=False, packed=False, runtime=False)
    (PROOFS / "gate-verification.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
