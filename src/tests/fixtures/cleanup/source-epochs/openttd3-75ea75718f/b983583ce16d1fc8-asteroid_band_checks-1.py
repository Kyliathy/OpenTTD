"""Verify shared body envelopes and actual static-placement exclusion without a new gameplay owner.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group6
@see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 9 (files this script writes carry the current
version, 424 or later; version 425 added the TBM face wait only)
The current native approach producer consumes an immutable paid418 world;
its historical checkpoint basename does not select the actual save schema.
"""
import argparse
import json
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
NATIVE = runpy.run_path(str(Path(__file__).with_name("space_approach_checks-1.py")))["native"]


def verify(configuration):
    """Check full supported geometry plus a real estimate-command boundary and ordinary reload.

    @param configuration Completed product/test build configuration.
    @return Accepted bounded band-policy proof with native process identities.
    """
    context = current_run_context("space", "asteroid-band-verify")
    focused = context.for_feature("space", "asteroid-band-focused")
    log = focused.logs / "asteroid-band-focused.log"
    with log.open("wb") as sink:
        run_native(focused, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                            "[asteroid-band],[space-construction-contract],[space-approach-contract],[asteroid-motion]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", log.read_text(encoding="utf-8"))
    assert passed, log
    source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    original = identity(source)
    native = context.for_feature("space", "asteroid-band-native")
    saved = NATIVE(native, configuration, "band-check", source)
    evidence = (native.logs / "native-space-approaches.log").read_text(encoding="utf-8")
    assert "band_policy=true estimate_pure=true" in evidence
    assert chunks(saved)[0] >= 424
    retained = context.for_feature("space", "asteroid-band-ordinary")
    ordinary(retained, {"name": "band-policy", "snapshot": identity(saved)}, configuration)
    resaved = retained.saves / "ordinary-resave.sav"
    # Native estimate refusal cannot alter owners; ordinary reload must retain the same world and all unrelated gameplay custody.
    for family in EXACT:
        assert chunks(saved)[1][family] == chunks(resaved)[1][family], family
    assert identity(source) == original
    report = context.reports / "asteroid-band.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group6; shared strip/envelope, actual native estimate refusal and ordinary persistence.",
        "configuration": configuration, "focused_assertions": int(passed.group(1)), "focused_cases": int(passed.group(2)),
        "source": original, "complete": identity(saved), "ordinary": identity(resaved), "focused_log": identity(log),
        "native_log": identity(native.logs / "native-space-approaches.log"), "supported_map_shapes": 49,
        "exact_owner_families": [x.decode("ascii") for x in EXACT],
        "native_reports": [identity(p) for owner in (focused, native, retained) for p in sorted(owner.reports.glob("native-*.json"))]},
        indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse the completed configuration and print its accepted policy report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    print(verify(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
