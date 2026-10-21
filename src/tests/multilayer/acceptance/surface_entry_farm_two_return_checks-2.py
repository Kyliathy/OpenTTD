"""Bind the genuine loaded farm delivery/empty return and original ordinary reloads.

@file surface_entry_farm_two_return_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVBC.
The real whole prospective wagon invoice remains protected alongside the
unchanged reserve. No unposted fare, supplied stock or work authorizes progress.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_two_return_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore the actual final original or bind only a closed, exact consecutive return sequence."""
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
    names = ("farm-two-return-query",) + tuple(
        f"farm-two-return-work-{window:03d}" for window in range(1, args.windows+1))
    if args.phase == "restore":
        assert args.configuration
        receipt = ROOT / f"logs/surface-entry-{names[-1]}-{args.configuration}.json"
        assert json.loads(receipt.read_text(encoding="utf-8"))["operation"]["return_scope"]["ended"]
        result = restore(args.configuration, receipt, CURRENT["observe"], "farm-two-return-static", __doc__)
    else:
        finals = []
        # Only consecutive actual windows and their first genuine final stop
        # may join this producer; no later scope can silently extend its budget.
        for configuration in ("RelWithDebInfo", "Debug"):
            for window in range(1, args.windows+1):
                receipt = ROOT / f"logs/surface-entry-farm-two-return-work-{window:03d}-{configuration}.json"
                value = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
                assert value["return_scope"]["window"] == window
                assert value["return_scope"]["ended"] == (window == args.windows)
            finals.append(value)
        result = bind(ROOT / "logs/surface-entry-farm-return-verification.json", CURRENT["VERSIONS"],
            names, "farm-two-return-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_farm_two_return_native.py")],
            ROOT / "test-cache/surface-entry/farm-two-return-products" if args.retain else None)
        assert finals[0]["return_scope"] == finals[1]["return_scope"]
        cash = finals[0]["final"]["company"]["cash"]
        assert cash == finals[1]["final"]["company"]["cash"]
        result.update(_documentation=__doc__, return_scope=finals[0]["return_scope"],
            startup_reserve=40000, unpaid_wagons=2226, final_cash=cash,
            original_crops_accepted=finals[0]["cumulative_accepted"], depot_halt=finals[0]["depot_halt"],
            working_gap=max(0, 100000-cash), incomplete_module_base_gap=max(0, 510000-cash),
            module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
