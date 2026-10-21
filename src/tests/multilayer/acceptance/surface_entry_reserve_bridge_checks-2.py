"""Bind the authorized one-window bridge and ordinary paused save reconstruction.

@file surface_entry_reserve_bridge_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZQ.
Both original financial binaries and closed prior receipts remain immutable.
Dated renderer/build inputs retain their exact previously archived source bytes.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_reserve_bridge_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
FOUNDATION = ROOT / "logs/surface-entry-joint-cycle-reserve-verification.json"


def renderer_versions():
    """Retain exact historical bytes behind the now-extended graphics/build paths.

    @return Explicit original-to-retained source manifest for the existing generic binder.
    @throws AssertionError On unrelated financial drift or missing historical source copies.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    manifest = foundation["products"]
    assert identity(ROOT / manifest["path"]) == manifest
    prior = json.loads((ROOT / manifest["path"]).read_text(encoding="utf-8"))
    products = []
    allowed = {"src/tests/CMakeLists.txt", "CMakeLists.txt", "src/station_cmd.cpp",
               "src/space_vehicle.cpp", "src/vehicle.cpp"}
    # Current source changed only through the separately qualified graphics
    # append/isolated target registration. The running financial executable stays
    # exact; old source goldens remain actual archived bytes rather than rebased hashes.
    for row in prior["products"]:
        reference = row["source"]
        if identity(ROOT / reference["path"]) == reference:
            continue
        assert reference["path"] in allowed
        retained = row["retained"]
        assert identity(ROOT / retained["path"]) == retained
        assert retained["sha256"] == reference["sha256"]
        products.append(dict(source=reference["path"], path=retained["path"], sha256=reference["sha256"]))
    assert {row["source"] for row in products} == allowed
    target = ROOT / "logs/surface-entry-reserve-bridge-renderer-versions.json"
    result = dict(_documentation=__doc__, products=products,
        foundation=identity(FOUNDATION),
        qualified_graphics=identity(ROOT / "logs/proofs/mlm/native-art/space/launch-presentation-v1/verification.json"))
    encoded = json.dumps(result, indent=2) + "\n"
    if target.exists():
        assert target.read_text(encoding="utf-8") == encoded
    else:
        target.write_text(encoded, encoding="utf-8")
    return target


def qualify(retain=False):
    """Verify the one native attempt and ordinary reload in both configurations.

    @param retain Whether to keep the new exact authorities under this group's test-cache directory.
    @return Bound native/static/cash result with no second-window permission.
    @throws AssertionError On inconsistent scope, paid books, native identity or duplicated progress.
    """
    final = []
    # The user's single approval ends on this window even without payment.
    # Neither a loss nor a pending positive fare can authorize another interval.
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / ("logs/surface-entry-reserve-bridge-" + configuration + ".json")
        receipt = json.loads(path.read_text(encoding="utf-8"))
        operation = receipt["operation"]
        scope = operation["bridge_scope"]
        assert receipt["complete"] and operation["startup_reserve"] == 30000
        assert scope["ended"] and scope["maximum_windows"] == scope["window"] == 1
        assert 0 < scope["earned_ticks"] <= scope["tick_cap"] == 2000
        assert not scope["another_window_authorized"]
        final.append(operation)
    assert final[0]["bridge_scope"] == final[1]["bridge_scope"]
    assert final[0]["measured_cash_flow"] == final[1]["measured_cash_flow"]
    versions = renderer_versions()
    result = bind(FOUNDATION, [versions], ("reserve-bridge",), "reserve-bridge-static",
        CURRENT["observe"], [Path(__file__), CALLER, CURRENT["AUTHORIZATION"]],
        ROOT / "test-cache/surface-entry/reserve-bridge-products" if retain else None)
    result.update(_documentation=__doc__, authorization=identity(CURRENT["AUTHORIZATION"]),
        bridge_scope=final[0]["bridge_scope"], startup_reserve=30000,
        default_reserve_unchanged=40000, measured_cash_flow=final[0]["measured_cash_flow"],
        final_cash=final[0]["final"]["company"]["cash"],
        working_gap=max(0, 100000-final[0]["final"]["company"]["cash"]),
        incomplete_module_base_gap=max(0, 510000-final[0]["final"]["company"]["cash"]),
        actual_reserve_restored=all(value["final"]["company"]["cash"] >= 100000 for value in final),
        module_capital_complete=False, another_window_authorized=False)
    return result


def main():
    """Restore one original without work or bind the complete one-window evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration
        receipt = ROOT / ("logs/surface-entry-reserve-bridge-" + args.configuration + ".json")
        result = restore(args.configuration, receipt, CURRENT["observe"], "reserve-bridge-static", __doc__)
    else:
        result = qualify(args.retain)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
