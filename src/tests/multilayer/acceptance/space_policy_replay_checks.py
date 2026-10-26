#!/usr/bin/env python3
"""Replay actual mining phase saves and independently audit native no-op resaves.

Owner: docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups30B/31B/31C/31D/31E.
Each application has the existing110-second harness deadline. Artifacts belong
in test-cache/space-fixes and driver/native logs in logs/space-fixes. Elapsed
gameplay is compared only with its recorded next-opportunity oracle; complete
packet rows are compared only across a no-op native load/preview/save.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

from asteroid_cargo_roundtrip_checks import compare as compare_packets
from asteroid_field_wire_checks import compare as compare_field
from space_mining_wire_checks import compare as compare_orders
from space_preview_checks import chunks as native_chunks

ROOT = Path(__file__).resolve().parents[4]
PHASES = ("before-departure", "after-departure", "retarget", "return-travel", "after-unload")
LIFECYCLE_PHASES = ("lifecycle-before-detachment", "lifecycle-after-departure", "lifecycle-after-credit", "lifecycle-before-expiry", "lifecycle-after-birth")


def invoke(source, configuration, run, mode, role):
    """Run one isolated original native application with its own log/runtime.

    @param source Immutable repository-owned actual phase save.
    @param configuration Completed native build configuration.
    @param run Unique existing-harness-compatible artifact identity.
    @param mode Replay the recorded next step or preserve the unchanged save.
    @param role Original mining role whose restored status the native observer captures.
    @return Native no-op resave path when requested, otherwise None.
    """
    command = [sys.executable, "-B", str(Path(__file__).with_name("space_fixes_checks.py")),
               "--source", source.relative_to(ROOT).as_posix(),
               "--configuration", configuration, "--run", run]
    if mode == "replay":
        command.extend(("--mining-controls", "replay-light" if role == 15 else "replay"))
    else:
        command.append("--cargo-origin")
    with (ROOT / "logs/space-fixes" / (run + "-driver.log")).open("w", encoding="utf-8") as log:
        subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    if mode == "preserve":
        return ROOT / "test-cache/space-fixes" / run / "cargo-origins-current.sav"
    return None


def compare_service_utility(source, resaved):
    """Preserve complete native common-power ledgers and lineage across no-op service transport.

    @param source Actual current service phase save, with its original native power owners.
    @param resaved Independent current native no-op save of that same phase.
    @return Audited byte lengths for the four original utility ledger/lineage families.
    """
    before_version, before = native_chunks(source)
    after_version, after = native_chunks(resaved)
    assert before_version == after_version
    families = (b"UPLH", b"UPLD", b"UPPH", b"UPPD")
    # Compare saved authority directly: settled sources, exact grants and their
    # lineage cannot be replaced by a clock-only or work-count observation.
    for family in families:
        assert family in before and family in after, family
        assert before[family] == after[family], family
    return {family.decode("ascii"): len(before[family]) for family in families}


def compare_lifecycle_views(source, paired):
    """Compare identical real opportunity streams and saved owners with Space shown or hidden.

    @param source Actual visible native lifecycle run directory.
    @param paired Actual hidden run from the same original source and commands.
    @return Complete phase owner comparisons and the exact compared opportunity count.
    """
    for name in ("policy-next.tsv", "lifecycle-opportunities.tsv"):
        assert (source / name).read_bytes() == (paired / name).read_bytes(), name
    phases = []
    # Camera selection may change presentation bytes, but must preserve every
    # actual packet, mission, typed queue and complete persistent field owner.
    for phase in LIFECYCLE_PHASES:
        visible = source / ("policy-" + phase + ".sav")
        hidden = paired / ("policy-" + phase + ".sav")
        phases.append({"phase": phase, "packets": compare_packets(visible, hidden),
                       "orders": compare_orders(visible, hidden, False),
                       "field": compare_field(visible, hidden)})
    return {"opportunities": len((source / "lifecycle-opportunities.tsv").read_text().splitlines()),
            "all_thirteen_owners_equal_each_opportunity": True, "phases": phases}


def main():
    """Validate the selected actual native phases without dispatching a full regression.

    @return Zero after independent next-decision and complete transport checks.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, help="Actual continuation run directory with policy-next.tsv")
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    parser.add_argument("--role", type=int, choices=(15, 21), default=21)
    parser.add_argument("--full-hold", action="store_true", help="Replay the actual retarget/full/return/unload phases instead of the partial-edge phases")
    parser.add_argument("--surface", action="store_true", help="Replay the original15's actual departure/release/descent/settlement phases")
    parser.add_argument("--no-mineral", action="store_true", help="Replay the actual saved NoCompatibleAsteroid wait")
    parser.add_argument("--no-receiver", action="store_true", help="Replay the actual saved full-compatible-receiver refusal")
    parser.add_argument("--receiver-fallback", action="store_true", help="Replay paid receiver activation, actual farther travel and original unload")
    parser.add_argument("--receiver-removed", action="store_true", help="Replay checked real receiver removal and the resulting native wait")
    parser.add_argument("--mineral-wake", action="store_true", help="Replay actual unrelated-mineral exit and matching-mineral native retry wake")
    parser.add_argument("--service-wait", choices=("pending", "shell", "parts", "module"), help="Replay one actual paid pending/shell/onboard-parts/module-parts wait")
    parser.add_argument("--service-resume", action="store_true", help="Replay genuine supplied/started/working/completed/resumed service phases")
    parser.add_argument("--stop-recall", action="store_true", help="Replay actual stopped safety release, resumed movement and checked Recall")
    parser.add_argument("--lifecycle", action="store_true", help="Replay actual detachment/departure/credit/expiry/birth phases with naturally earned timing")
    parser.add_argument("--paired-view", help="Compare every actual opportunity and saved owner with an independently run hidden lifecycle")
    args = parser.parse_args()
    if sum((args.full_hold, args.surface, args.no_mineral, args.no_receiver, args.receiver_fallback, args.receiver_removed, args.mineral_wake, args.service_wait is not None, args.service_resume, args.stop_recall, args.lifecycle)) > 1:
        raise ValueError("select exactly one saved phase family per replay invocation")
    # Surface ownership and full module handling have distinct saved phases;
    # one invocation must not confuse its original role or transport oracle.
    if args.surface and (args.full_hold or args.role != 15):
        raise ValueError("Surface replay requires --role 15 and its own phase directory")
    # Service phases retain distinct original actors: only the module shortage
    # belongs to21; pending/shell/incoming-resupply checkpoints belong to15.
    if args.service_wait and args.role != (21 if args.service_wait == "module" else 15):
        raise ValueError("service wait role must match its actual original phase")
    if args.paired_view and not args.lifecycle:
        raise ValueError("paired view comparison requires the actual lifecycle phase family")
    source = (ROOT / args.source).resolve()
    # Restrict every generated application/artifact to the established harness
    # domain. The phase files and oracle remain immutable actual-run evidence.
    if not source.is_relative_to(ROOT / "test-cache/space-fixes") or not (source / "policy-next.tsv").is_file() or not re.fullmatch(r"[a-z0-9-]+", args.run):
        raise ValueError("select a native continuation run and a unique lowercase run identity")
    report = {"_documentation": "r2sff1-space-fixes-features-1.md Groups30B/31B/31C/31D/31E; actual saved next decisions and complete no-op transport", "phases": []}
    if args.paired_view:
        paired = (ROOT / args.paired_view).resolve()
        if not paired.is_relative_to(ROOT / "test-cache/space-fixes"):
            raise ValueError("paired lifecycle must be a repository-owned native run")
        report["view_comparison"] = compare_lifecycle_views(source, paired)
    # Each checkpoint gets a separate actual application for its next decision
    # and another for no-op transport, preventing elapsed packet accounting
    # from being confused with a save/load preservation failure.
    phases = ("retarget", "full-hold", "return-travel", "after-unload") if args.full_hold else PHASES
    if args.surface:
        phases = ("surface-before-departure", "surface-before-release", "surface-descent", "surface-settled")
    if args.no_mineral:
        phases = ("no-mineral",)
    if args.no_receiver:
        phases = ("no-receiver",)
    if args.receiver_fallback:
        phases = ("receiver-activated", "receiver-travel", "receiver-unloaded")
    if args.receiver_removed:
        phases = ("receiver-removed", "receiver-removed-wait")
    if args.mineral_wake:
        phases = ("mineral-unrelated-exit", "mineral-before-wake", "mineral-after-wake")
    if args.service_wait:
        phases = ("service-" + args.service_wait,)
    if args.service_resume:
        phases = ("service-supplied", "service-started", "service-working", "service-completed", "service-resumed")
    if args.stop_recall:
        phases = ("stopped-edge", "resumed-edge", "recall-requested", "recall-stable")
    if args.lifecycle:
        phases = LIFECYCLE_PHASES
    for phase in phases:
        saved = source / ("policy-" + phase + ".sav")
        invoke(saved, args.configuration, args.run + "-" + phase, "replay", args.role)
        resaved = invoke(saved, args.configuration, args.run + "-" + phase + "-wire", "preserve", args.role)
        report["phases"].append({"phase": phase, "packets": compare_packets(saved, resaved),
                                 "orders": compare_orders(saved, resaved, False),
                                 "field": compare_field(saved, resaved)})
        if args.service_resume:
            report["phases"][-1]["utility"] = compare_service_utility(saved, resaved)
    (ROOT / "logs/space-fixes" / (args.run + "-transport.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
