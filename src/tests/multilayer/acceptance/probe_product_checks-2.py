"""Run bounded local/network pairs for actual paid processor product commands.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8C.4
Original accepted paid-world checkpoints supply every material and interrupted job.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

HERE = Path(__file__).parent
AUDIT = runpy.run_path(str(HERE / "orbital_runtime_checks-3.py"))
ENTRY = ROOT / "logs/processor-returns-verification.json"
ENTRY_SHA = "0a2006417589f3b28cd5149ee1cbad417aa0a15f2104cb95d48c77bf5f45c6d2"
ACTIONS = {"processing": "ore-delivered", "freight": "products-loaded", "release": "historical-cleared", "handling": "recovered"}
SCOPES = {"native-a": ("processing", "freight"), "native-b": ("release", "handling"), "focused": ()}
TAGS = "[space-content],[launch-assembly-contract],[manufacturing-purchase],[manufacturing-reservation],[orbital-recipe-inventory],[processor-freight]"


def execute(configuration, action, network):
    """Run one exact paid-world command twin or directly related component burst.

    @param configuration Current native test build.
    @param action Original product command family, or focused component boundary.
    @param network Whether actual original SendNet dispatch supplies the command.
    @return Exclusive semantic proof path and original physical output identity.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in json.loads(ENTRY.read_text(encoding="utf-8"))["configurations"] if c["configuration"] == configuration]
    source = None if action == "focused" else selected["checkpoints"]["native/" + ACTIONS[action]]
    if source:
        assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-product-replay"); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    tag = TAGS if action == "focused" else "[.probe-product-replay-native]"
    if source:
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_product_replay_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_PRODUCT_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_PRODUCT_ACTION=action)
        if network:
            env["OTTD_PROBE_PRODUCT_NETWORK"] = "1"
        else:
            env.pop("OTTD_PROBE_PRODUCT_NETWORK", None)
    log = context.logs / "probe-product.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log
    after = None
    if source:
        assert f"PROBE_PRODUCT replay=true action={action} network={str(network).lower()} added_stock=false" in terminal
        after = identity(context.saves / ("probe-product-" + action + "-438.sav"))
    report = context.reports / "probe-product.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, action=action, network=network,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=identity(ENTRY), source=source, after=after,
                                     added_stock=False, terminal=identity(log), native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report, after


def main():
    """Run at most four original native children; each pair must preserve all thirty physical families and metadata."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); reports = []
    if args.scope == "focused":
        report, _ = execute(args.configuration, "focused", False); reports.append(report)
    # Each local/network twin starts from the same exact original paid input, never from its sibling's already consumed output. Pair comparison covers funds, physical work, source provenance and every unique cargo owner.
    for action in SCOPES[args.scope]:
        local, before = execute(args.configuration, action, False); remote, after = execute(args.configuration, action, True)
        AUDIT["compare"](before, after); reports.extend((local, remote)); print(f"passed {args.configuration} {action} local/network", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope,
                                          reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
