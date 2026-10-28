#!/usr/bin/env python3
"""Audit the Space fixes plan and publish current scoped implementation evidence.

@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group34.
Reads Git only. Generated audit/index JSON belongs in logs/space-fixes; no source,
save, configuration or historical receipt is rewritten by this tool.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote

from space_fixes_checks import ROOT, identity

DOCS = ROOT / "docs/zz-reach2/upgrades/2026-10"
PLAN = DOCS / "r2sff1-space-fixes-features-1.md"
SCOPED_EXISTING = (
    "docs/zz-reach2/architecture/gui/archi-gui.md",
    "docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
    "docs/zz-reach2/architecture/savegame/archi-savegame.md",
    "docs/zz-reach2/architecture/desync/archi-desync.md",
    "docs/zz-reach2/architecture/vehicles/archi-vehicles.md",
    "docs/zz-reach2/architecture/game-loop/archi-game-loop.md",
    "docs/zz-reach2/upgrades/2026-10/d2sa-space-adventures.md",
    "docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md",
    "docs/zz-reach2/upgrades/2026-10/r2df-demo-fixing-oct-15.md",
)
CRITICAL = {
    "src/asteroid.h": ("AsteroidBody", "AsteroidField", "AsteroidKnowledgeRecord", "InspectAsteroidStorage"),
    "src/asteroid.cpp": ("TickAsteroidField", "RetireAsteroidBatch", "CommitAsteroidRenewal", "AdvanceAsteroidRenewalTimer", "FinishAsteroidRenewalAttempt"),
    "src/space.h": ("SpaceSpatialIndex", "InspectStorage"),
    "src/space_orders.h": ("SpaceOrderQueue", "SpaceMiningPolicy", "SpaceApproachRequest"),
    "src/space_vehicle_cmd.h": ("CmdSetSpaceMiningPolicy", "CmdSetSpaceMissionChecked"),
    "src/space_structure.cpp": ("CmdRemoveOrbitalModule",),
    "src/tests/space_lifecycle_peer_native.cpp": ("ObserveLifecyclePeer", "SaveLifecyclePeer"),
    "src/tests/asteroid_lifecycle_work.cpp": ("MeasureInsufficientLifecycleIntervals", "LifecycleStorageRecord"),
}


def group_key(value):
    """Order original and suffixed groups without confusing them with task decimals.

    @param value Published group identifier, such as31 or32B.
    @return Numeric group and appended uppercase subdivision.
    """
    number, suffix = re.fullmatch(r"(\d+)([A-Z]*)", value).groups()
    return int(number), suffix


def scope_contains(scope, group):
    """Check the nearest explicit mandatory-reading scope, including numeric ranges.

    @param scope Actual Applies-to paragraph, without following other documents' reading lists.
    @param group Current original/suffixed task group.
    @return Whether the declared single/range scope includes this exact group.
    """
    # Only the first sentence declares coverage; later sentences may direct
    # another group to its own narrower reading rather than extend this scope.
    declared = " ".join(scope.split()).split(". ", 1)[0]
    for first, last in re.findall(r"(\d+[A-Z]*)(?:\s*-\s*(\d+[A-Z]*))?", declared.replace("–", "-")):
        if group_key(first) <= group_key(group) <= group_key(last or first):
            return True
    return False


def audit_plan(require_complete):
    """Audit visible task granularity, difficulty, numbering and nearest mandatory reading.

    @param require_complete Whether every task must now have its requested uppercase done marker.
    @return Complete group/task accounting plus incomplete task identities.
    """
    source = PLAN.read_text(encoding="utf-8")
    assert source.index("## Global Mandatory Reading") < source.index("## Implementation tasks")
    headers = list(re.finditer(r"^### Group\s*(\d+[A-Z]*) - (.+)$", source, re.M))
    readings = list(re.finditer(r"^### Area Mandatory Reading[^\n]*\n[\s\S]*?^_Applies to ([\s\S]*?)_", source, re.M))
    groups, seen, pending = [], set(), []
    # Validate each published batch independently so a subdivision cannot hide
    # an oversized, unnumbered or uncovered batch elsewhere in the plan.
    for index, header in enumerate(headers):
        group = header[1]
        assert group not in seen
        seen.add(group)
        body = source[header.end():headers[index + 1].start() if index + 1 < len(headers) else source.index("## Consult on Demand")]
        tasks = re.findall(r"^- \[([ X])\] (\d+[A-Z]*\.\d+)\s", body, re.M)
        assert 1 <= len(tasks) <= 8, (group, len(tasks))
        assert [int(task.rsplit(".", 1)[1]) for _, task in tasks] == list(range(1, len(tasks) + 1)), group
        assert all(task.rsplit(".", 1)[0] == group for _, task in tasks), group
        difficulty = re.findall(r"^\{\{(\w+)\}\}$", body, re.M)
        assert len(difficulty) == 1 and difficulty[0] in ("SIMPLE", "MEDIUM", "HARD"), (group, difficulty)
        local = [row for row in readings if row.start() < header.start()][-1]
        assert scope_contains(local[1], group), (group, local[1])
        open_tasks = [task for state, task in tasks if state != "X"]
        pending.extend(open_tasks)
        groups.append({"group": group, "difficulty": difficulty[0], "tasks": len(tasks), "complete": len(tasks) - len(open_tasks), "nearest_reading": local[1]})
    assert [group_key(row["group"]) for row in groups] == sorted(group_key(row["group"]) for row in groups)
    assert {group_key(row["group"])[0] for row in groups} == set(range(1, 35))
    if require_complete:
        assert not pending, pending
    return {"groups": groups, "tasks": sum(row["tasks"] for row in groups), "pending": pending, "maximum_tasks_per_group": max(row["tasks"] for row in groups)}


def heading_anchors(path):
    """Compute local Markdown heading anchors and retain explicit HTML anchors.

    @param path Actual local Markdown target.
    @return Defined anchors, including repeated-heading suffixes.
    """
    source = path.read_text(encoding="utf-8-sig")
    anchors = set(re.findall(r'<a\s+(?:id|name)="([^"]+)"', source))
    duplicates = Counter()
    # Duplicate heading suffixes must match Markdown navigation, otherwise a
    # valid earlier heading could mask a broken link to a later occurrence.
    for heading in re.findall(r"^#{1,6}\s+(.+?)\s*#*\s*$", source, re.M):
        slug = re.sub(r"<[^>]+>", "", heading).lower()
        slug = re.sub(r"[^\w\- ]", "", slug).replace(" ", "-")
        number = duplicates[slug]
        duplicates[slug] += 1
        anchors.add(slug + ("-" + str(number) if number else ""))
    return anchors


def audit_links(generated_output):
    """Check all follow-up note links and links on changed lines of existing scoped docs.

    @param generated_output Exact report path being published after the evidence checks.
    @return Validated local-link count, external source availability and scope labels.
    Historical unrelated document links retain their prior scope; no broad cleanup is inferred.
    """
    texts = [(path, path.read_text(encoding="utf-8")) for path in sorted(DOCS.glob("r2sff1-*.md"))]
    # Existing documents contain unrelated historical work: audit only the
    # changed lines there while checking this follow-up's own notes in full.
    for relative in SCOPED_EXISTING:
        diff = subprocess.run(["git", "diff", "--unified=0", "--", relative], cwd=ROOT, capture_output=True, text=True, check=True).stdout
        additions = "\n".join(line[1:] for line in diff.splitlines() if line.startswith("+") and not line.startswith("+++"))
        texts.append((ROOT / relative, additions))
    checked, outside, failures = 0, [], []
    # Resolve each link relative to its owning document; repository evidence
    # must exist, while user-owned external attachments are reported read-only.
    for path, source in texts:
        source = re.sub(r"^```[^\n]*\n[\s\S]*?^```\s*$", "", source, flags=re.M)
        for target in re.findall(r"\[[^\]]*\]\(([^)\n]+)\)", source):
            target = unquote(target.strip().strip("<>"))
            if re.match(r"(?:https?|mailto):", target):
                continue
            filename, _, anchor = target.partition("#")
            actual = (path.parent / filename).resolve() if filename else path
            # The index can link to itself before its first successful creation;
            # only this explicitly contained output receives that exception.
            if actual == generated_output:
                checked += 1
                continue
            if not actual.is_relative_to(ROOT):
                outside.append({"from": path.relative_to(ROOT).as_posix(), "target": target, "available": actual.is_file()})
                continue
            if not actual.is_file():
                failures.append({"from": path.relative_to(ROOT).as_posix(), "target": target, "error": "missing local target"})
            # A present Markdown file is insufficient when a required reading
            # or evidence link promises a specific, now-missing subsection.
            elif anchor and actual.suffix == ".md" and anchor not in heading_anchors(actual):
                failures.append({"from": path.relative_to(ROOT).as_posix(), "target": target, "error": "missing heading anchor"})
            else:
                checked += 1
    return {"checked_local_links": checked, "failures": failures, "external_references": outside,
            "scope": "complete r2sff1 notes/plan; added or changed lines in existing architecture, guide, master and earlier plan"}


def read_json_receipt(path):
    """Decode native UTF-8 receipts and PowerShell's BOM-marked redirected output.

    @param path Existing receipt or redirected single-JSON driver log.
    @return Parsed receipt without rewriting its original evidence bytes.
    """
    payload = path.read_bytes()
    # Windows PowerShell redirects as UTF-16, while native Python receipts are
    # UTF-8. Honor the BOM so the evidence inventory can preserve both formats.
    encoding = "utf-16" if payload.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    return json.loads(payload.decode(encoding))


def main():
    """Publish a reproducible scoped audit and, when complete, current binary/evidence identities.

    @return Zero on successful audit; missing local evidence remains an explicit assertion failure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-complete", action="store_true")
    parser.add_argument("--output", default="logs/space-fixes/implementation-evidence.json")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT / "logs/space-fixes"):
        raise ValueError("evidence output must remain under repository logs/space-fixes")
    report = {"_documentation": __doc__, "plan": identity(PLAN), "plan_audit": audit_plan(args.require_complete), "links": audit_links(output), "critical_symbols": []}
    # Public ownership/navigation contracts must still point at actual exports;
    # proposed interfaces in the original plan are not implementation evidence.
    for relative, symbols in CRITICAL.items():
        path = ROOT / relative
        text = path.read_text(encoding="utf-8")
        assert all(re.search(r"\b" + re.escape(symbol) + r"\b", text) for symbol in symbols), relative
        report["critical_symbols"].append({"source": identity(path), "symbols": symbols})
    report["binaries"] = [identity(ROOT / "build" / configuration / executable) for configuration in ("RelWithDebInfo", "Debug")
                          for executable in ("openttd.exe", "openttd_test.exe", "openttd_space_fixes_test.exe")]
    report["notes"] = [identity(path) for path in sorted(DOCS.glob("r2sff1-*.md"))]
    receipts = ["lifecycle-phase-replay-final-release-transport.json", "lifecycle-phase-replay-debug-transport.json"]
    # Retain original scoped receipts alongside final executable rechecks;
    # historical fingerprints are evidence of what ran and are never rewritten.
    # Keep both independently checked configurations and every bounded workload
    # discoverable without rerunning unrelated regression suites.
    for suffix in ("release", "debug"):
        receipts.extend(("legacy-compatibility-" + suffix + "-compatibility.json", "lifecycle-peers-minimal-" + suffix + "-peers.json",
                         "high-id-lifecycle-peers-" + suffix + "-peers.json", "high-id-source-wire-" + suffix + "-transport.json"))
        final_peer = ROOT / "logs/space-fixes" / ("final-high-id-peers-" + suffix + "-peers.json")
        if args.require_complete or final_peer.is_file():
            receipts.append(final_peer.name)
        for case in ("small", "normal", "largest-normal", "largest-dense", "clustered", "narrow-tall", "narrow-wide"):
            stem = "lifecycle-work-pool-small-release" if (case, suffix) == ("small", "release") else "lifecycle-work-" + case + "-" + suffix
            receipts.append(stem + "-work.json")
    report["acceptance_receipts"] = [identity(ROOT / "logs/space-fixes" / name) for name in receipts]
    report["retry_samples"] = [identity(ROOT / "test-cache/space-fixes" / ("mining-retry-work-" + suffix) / "native-retry-work.json") for suffix in ("release", "debug")]
    report["documentation"] = [identity(ROOT / relative) for relative in SCOPED_EXISTING]
    report["current_rechecks"] = []
    # These final peer and expiry decisions bind the last compiled observers;
    # earlier scoped receipts keep their actual historical executable hashes.
    for suffix, configuration in (("release", "RelWithDebInfo"), ("debug", "Debug")):
        peer_path = ROOT / "logs/space-fixes" / ("final-high-id-peers-" + suffix + "-peers.json")
        peer = read_json_receipt(peer_path)
        assert peer["completed"] and peer["actual_socket_multiplayer"], peer_path
        expected = identity(ROOT / "build" / configuration / "openttd_test.exe")
        assert all(child["executable"] == expected and child["exit_code"] == 0 for child in peer["children"].values()), peer_path
        replay_path = ROOT / "logs/space-fixes" / ("final-expiry-replay-" + suffix + "-driver.log")
        replay = read_json_receipt(replay_path)
        assert replay["executable"] == identity(ROOT / "build" / configuration / "openttd_space_fixes_test.exe")
        assert replay["exit_code"] == 0
        ui_path = ROOT / "logs/space-fixes" / ("final-report-ui-" + suffix + "-driver.log")
        ui = read_json_receipt(ui_path)
        assert ui["executable"] == replay["executable"]
        assert ui["exit_code"] == 0
        report["current_rechecks"].append({"configuration": configuration, "peer_receipt": identity(peer_path),
                                            "expiry_receipt": identity(replay_path), "expiry_input": replay["source"],
                                            "ui_receipt": identity(ui_path), "ui_input": ui["source"]})
    fixture_names = ("space-factory-440.sav", "space-module-work-418.sav", "space-heavy-440.sav",
                     "space-mining-working-405.sav", "space-mining-working-1990-405.sav")
    report["immutable_demo_inputs"] = [identity(ROOT / "src/tests/fixtures/demos/space" / name) for name in fixture_names]
    report["immutable_demo_inputs"].append(identity(ROOT / "src/tests/fixtures/geology/r2mlvs2-z-classic-current.sav"))
    report["final_build_logs"] = [identity(ROOT / "logs/space-fixes" / name) for name in (
        "build-lifecycle-work-debug.log", "build-lifecycle-work-includes-release.log",
        "build-lifecycle-work-pool-release.log", "build-final-ordinary-tests-release.log")]
    # Record the actual uncommitted source snapshot, including untracked test
    # tools. This is an identity inventory, not a claim to own unrelated edits.
    changed = subprocess.run(["git", "ls-files", "--modified", "--others", "--exclude-standard", "--", "src", "CMakeLists.txt"],
                             cwd=ROOT, capture_output=True, text=True, check=True).stdout.splitlines()
    report["working_source_snapshot"] = [identity(ROOT / name) for name in sorted(set(changed))]
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    assert not report["links"]["failures"], report["links"]["failures"]
    print("SPACE_FIXES_AUDIT", "tasks=" + str(report["plan_audit"]["tasks"]), "pending=" + str(len(report["plan_audit"]["pending"])),
          "links=" + str(report["links"]["checked_local_links"]), "six_current_binaries=true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
