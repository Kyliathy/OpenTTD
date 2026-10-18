"""Verify actual paid Surface commands through original native finance owners.

@file surface_supply_checks-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCD
Declared cash/map/stations isolate native command boundaries. Purchased pending
commitments supply no driven construction or final Surface factory acceptance.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

TAGS = "[surface-supply-commands],[surface-supply-dispatch],[surface-supply-lifecycle],[surface-supply-owner],[surface-supply-state],[surface-supply-admission],[surface-supply-order-wire],[surface-supply-custody]"
SOURCES = (
    "src/surface_supply.h", "src/surface_supply.cpp", "src/surface_supply_order.h", "src/surface_supply_order.cpp",
    "src/surface_supply_order_runtime.h", "src/surface_supply_order_runtime.cpp", "src/surface_supply_cmd.h",
    "src/tests/surface_supply_fixture.h", "src/tests/surface_supply_order_owner.cpp",
    "src/tests/surface_supply_order.cpp", "src/tests/surface_supply.cpp",
    "src/command.cpp", "src/command_func.h", "src/command_type.h", "src/network/network_command.cpp",
    "src/company_cmd.cpp", "src/multilayer_bootstrap.h", "src/multilayer_bootstrap.cpp", "CMakeLists.txt",
    "src/station_cmd.cpp", "src/station_supply.h", "src/cargopacket.h", "src/cargopacket.cpp", "src/source_type.h",
    "src/station.cpp", "src/economy.cpp", "src/tests/surface_supply_order_wire.cpp", "src/tests/surface_supply_custody.cpp",
)


def execute(configuration):
    """Run one bounded actual native command burst.

    @param configuration Independently linked original focused engine build.
    @return Exact terminal/source/product/process report identity.
    """
    context = create_run_context("space", "surface-supply-order-execution-components")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "surface-supply-order-execution-components.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), TAGS, "--durations", "yes"], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(errors="replace")
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and matched, log
    # A stale executable may silently ignore a new tag while passing older cases;
    # require the actual new financial case names from this native process too.
    assert "Paid Surface commands charge and refund frozen purchases once through native finance" in terminal, log
    assert "Paid Surface commands recapture actual owners and refuse changed or foreign intents unchanged" in terminal, log
    assert "Paid Surface allowance spans origins and survives day rollover with monotonic retired history" in terminal, log
    assert "Paid Surface pending orders enter original station custody once without another expense" in terminal, log
    assert "Paid Surface dispatch refuses changed station owners layers facilities and editor ticks" in terminal, log
    assert "Native company takeover transfers Surface commitments and over-cap history without duplicate stock or refunds" in terminal, log
    assert "Native company loss retires pending Surface rights before demolition and company slot reuse" in terminal, log
    assert "Native station rating abandonment preserves paid Surface stock and discards ordinary offers" in terminal, log
    target = context.reports / "surface-supply-order-execution-components.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                    assertions=int(matched[1]), cases=int(matched[2]), tags=TAGS,
                                    executable=identity(executable), terminal=identity(log),
                                    sources=[identity(ROOT / name) for name in SOURCES] + [identity(Path(__file__))],
                                    native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                                    native_financial_commands=True, driven_delivery_acceptance=False,
                                    final_surface_factory_acceptance=False), indent=2) + "\n", encoding="utf-8")
    return identity(target)


def main():
    """Run the current focused paid command selection and publish a new repository-owned membership file."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = dict(_documentation=__doc__, report=execute(args.configuration))
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
