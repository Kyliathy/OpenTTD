"""Bind the closed loaded arrival and its exact original ordinary reloads.

@file surface_entry_arrival_settlement_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZS.
Actual packet ownership, posted company money and the explicitly approved bounded reserve remain
authoritative; every predecessor and original application is preserved.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_arrival_settlement_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore one exact endpoint or bind the complete finite arrival in both builds."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    names = tuple(f"arrival-settlement-{window:03d}" for window in range(1, args.windows+1))
    if args.phase == "restore":
        assert args.configuration
        receipt = ROOT / f"logs/surface-entry-{names[-1]}-{args.configuration}.json"
        final = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
        assert final["settlement_scope"]["ended"]
        result = restore(args.configuration, receipt, CURRENT["observe"], "arrival-settlement-static", __doc__)
    else:
        finals = []
        # Every actual interval must consume its immediate paid predecessor,
        # and only the final real stop may close the bounded arrival sequence.
        for configuration in ("RelWithDebInfo", "Debug"):
            for window, name in enumerate(names, 1):
                value = json.loads((ROOT / f"logs/surface-entry-{name}-{configuration}.json").read_text(
                    encoding="utf-8"))["operation"]
                scope = value["settlement_scope"]
                assert scope["kind"] == CURRENT["SCOPE"] and scope["window"] == window
                assert scope["ended"] == (window == args.windows)
            finals.append(value)
        result = bind(ROOT / "logs/surface-entry-loaded-arrival-verification.json", [], names,
            "arrival-settlement-static", CURRENT["observe"], [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/arrival-settlement-products" if args.retain else None)
        assert finals[0]["settlement_scope"] == finals[1]["settlement_scope"]
        cash = finals[0]["final"]["company"]["cash"]
        assert cash == finals[1]["final"]["company"]["cash"]
        result.update(_documentation=__doc__, settlement_scope=finals[0]["settlement_scope"],
            startup_reserve=30000, final_cash=cash, working_gap=max(0, 100000-cash),
            incomplete_module_base_gap=max(0, 510000-cash), module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
