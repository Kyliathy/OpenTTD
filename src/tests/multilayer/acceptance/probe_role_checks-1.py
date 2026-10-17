"""Verify current probe-role admission against the actual paid manufacturing handoff.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
The dated producer remains immutable; current executable/source identities must still match before its paid runtime evidence is reused.
"""
import argparse
import json
import os
import re
import subprocess

from space_preview_checks import ROOT, identity
from runner import current_run_context, run_native

PRODUCER = ROOT / "logs/manufacturing-replacement-verification.json"
PRODUCER_SHA256 = "6a6e1bc848c551c9523b2970043af302c8ff5759f1963d6bc347302eaf648878"


def commissioning(configuration):
    """Revalidate role recipes and admission without replaying the completed producer's full acceptance.

    @param configuration Matching current native game/test configuration.
    @return New hash-qualified focused consumer report bound to exact paid19/20 producer milestones.
    """
    assert identity(PRODUCER)["sha256"] == PRODUCER_SHA256
    producer = json.loads(PRODUCER.read_text(encoding="utf-8"))
    # Earlier producer declarations alone cannot authorize this consumer: unchanged current implementation and native products must match their accepted physical work/receipt boundary.
    for source in producer["current_sources"]:
        assert identity(ROOT / source["path"]) == source, source["path"]
    selected = next(row for row in producer["configurations"] if row["configuration"] == configuration)
    paid = []
    # Select explicit original paid phases for both newly appended roles; edited/lost/reloaded branches cannot stand in for an ordinary full-bill product.
    for member in selected["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member
        report = json.loads(path.read_text(encoding="utf-8"))
        if report.get("variant") == "main" and report.get("model") in (19, 20) and report.get("phase") in ("queued", "reserved", "before", "complete") and "after" in report:
            assert report["save_version"] == 429 and identity(ROOT / report["after"]["path"]) == report["after"]
            paid.append(member)
    assert len(paid) == 8
    context = current_run_context("space", "probe-role-commissioning")
    log = context.logs / "probe-role-commissioning.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                             "[space-content],[launch-assembly-contract],[manufacturing-purchase]"],
                   cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                   stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
    assert passed and int(passed[2]) == 8, "All current content, Earth receipt and ordinary orbital admission contracts must be compiled."
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.1; current content2/save429 consumer of actual full-bill paid19/20 manufacturing. Earth commissioning remains the original launch-center producer; no new fixture stock or hull is granted.",
                  configuration=configuration, completed_task="8B.1", save_version=429, space_content_version=2,
                  assertions=int(passed[1]), cases=int(passed[2]), log=identity(log), producer=identity(PRODUCER), paid_milestones=paid,
                  current_sources=producer["current_sources"],
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    target = context.reports / "probe-role-commissioning.json"; assert not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run one explicit current commissioning configuration and print its invocation-owned proof path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    args = parser.parse_args()
    print(commissioning(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
