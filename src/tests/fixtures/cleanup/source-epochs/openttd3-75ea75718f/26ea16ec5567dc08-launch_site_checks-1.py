"""Verify the actual launch placement geometry producer before generation consumes it.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group3
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
@see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 9 (files this script writes carry the current
version, 424 or later; version 425 added the TBM face wait only)
The native placement fixture uses isolated terrain for boundary checks. This
producer proof does not yet claim generated-world starter accessibility.
"""
import argparse
import json
import re
import struct
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from launch_center_checks import native
from runner import current_run_context, run_native


def generated_body_identity(path):
    """Read unchanged generated composition across the known404 and current424 native layouts.

    @param path Genuine source or actual native placement result.
    @return Generation/dimensions/seed and each body's original identity/class/size/reserve.
    """
    version, data = chunks(path)
    assert version in (404, 424)
    payload = data[b"ASTR"][3:]
    count = int.from_bytes(payload[27:31], "big")
    stride = 36 if version == 404 else 40
    rows = []
    # Paid Surface placement must not spawn or reroll finite bodies, even though legacy lease/input fields migrate when saved.
    for body in range(count):
        start = 51 + body * stride
        identifier, resource, size = struct.unpack_from(">IBB", payload, start)
        initial = struct.unpack_from(">I", payload, start + 24)[0]
        rows.append((identifier, resource, size, initial))
    return payload[:18], rows


def verify(configuration):
    """Check shared geometry, actual paid placement and ordinary persisted station ownership.

    @param configuration Built RelWithDebInfo or Debug configuration to execute.
    @return Accepted producer report with exact executable, input, output and native report identities.
    """
    context = current_run_context("space", "launch-site-verify")
    focused = context.for_feature("space", "launch-site-focused")
    log = focused.logs / "launch-site-focused.log"
    with log.open("wb") as sink:
        run_native(focused, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                             "[launch-site],[launch-center-geometry]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", log.read_text(encoding="utf-8", errors="replace"))
    assert passed, log
    source = ROOT / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav"
    original = identity(source)
    placed = context.for_feature("space", "launch-site-native")
    save = native(placed, configuration)
    assert chunks(save)[0] >= 424
    assert generated_body_identity(source) == generated_body_identity(save)
    replay = context.for_feature("space", "launch-site-ordinary")
    ordinary(replay, {"name": "launch-site", "snapshot": identity(save)}, configuration)
    resaved = replay.saves / "ordinary-resave.sav"
    before, after = chunks(save)[1], chunks(resaved)[1]
    # The real station, facility anchors and joined-freight ownership must survive ordinary loading exactly.
    for family in (b"LCTR", b"LCHB", b"STNN", b"CAPA"):
        assert before[family] == after[family], family
    assert identity(source) == original
    report = context.reports / "launch-site.json"
    report.write_text(json.dumps({
        "_documentation": "Owner: r2mlvs10 Group2 launch-site producer; actual Plan9 geometry and native placement, before generator integration.",
        "configuration": configuration, "source": original,
        "focused_assertions": int(passed.group(1)), "focused_cases": int(passed.group(2)),
        "focused_log": identity(log), "native_save": identity(save), "ordinary_save": identity(resaved),
        "exact_owner_families": [family.decode("ascii") for family in (b"LCTR", b"LCHB", b"STNN", b"CAPA")],
        "native_reports": [identity(path) for owner in (focused, placed, replay) for path in sorted(owner.reports.glob("native-*.json"))],
        "generated_world_accessibility": False,
        "paid_placement_preserves_generated_bodies": True,
    }, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse the completed build configuration and print the accepted producer report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    print(verify(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
