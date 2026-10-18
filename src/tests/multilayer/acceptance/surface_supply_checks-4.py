"""Verify the native order owner, codec, copied joins and original packet retention.

@file surface_supply_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC
Declared records and stock exercise component boundaries. Mutable supply commands
remain disabled; actual native-file transport is checked separately by checks-5.
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

TAGS = "[surface-supply-owner],[surface-supply-order-wire],[surface-supply-state],[surface-supply-admission],[surface-supply-custody],[multilayer-bootstrap]"
SOURCES = (
    "src/surface_supply.h", "src/surface_supply.cpp", "src/surface_supply_order.h", "src/surface_supply_order.cpp",
    "src/multilayer_bootstrap.h", "src/multilayer_bootstrap.cpp", "src/tests/multilayer_bootstrap.cpp",
    "src/surface_supply_order_runtime.h", "src/surface_supply_order_runtime.cpp", "src/surface_supply_cmd.h",
    "src/surface_supply_order_wire.h", "src/surface_supply_order_wire.cpp", "src/cargopacket.h", "src/cargopacket.cpp",
    "src/map_func.h", "src/map.cpp", "src/map_layer.h", "src/map_layer.cpp", "src/fios.h", "src/fios_gui.cpp",
    "src/multilayer_state.cpp", "src/command_type.h", "src/command.cpp", "src/network/network_command.cpp", "src/CMakeLists.txt", "CMakeLists.txt",
    "src/saveload/surface_supply_order_sl.cpp", "src/saveload/saveload.h", "src/saveload/saveload.cpp",
    "src/saveload/afterload.cpp", "src/saveload/misc_sl.cpp", "src/saveload/map_sl.cpp", "src/saveload/station_sl.cpp",
    "src/saveload/CMakeLists.txt", "src/tests/CMakeLists.txt", "src/tests/surface_supply_fixture.h",
    "src/tests/surface_supply.cpp", "src/tests/surface_supply_order.cpp", "src/tests/surface_supply_custody.cpp",
    "src/tests/surface_supply_order_owner.cpp", "src/tests/surface_supply_order_wire.cpp", "src/tests/surface_supply_order_native.cpp",
)


def read(path):
    """@param path Explicit repository-owned JSON. @return Its original parsed content."""
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration):
    """Run only current owner/transport components and directly affected admission/custody.

    @param configuration Independently linked native build.
    @return Original report identity; no declared metadata becomes paid supplies.
    """
    context = create_run_context("space", "surface-supply-order-persistence-components")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "surface-supply-order-components.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), TAGS], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(errors="replace"))
    assert result.returncode == 0 and matched, log
    target = context.reports / "surface-supply-order-components.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                    tags=TAGS, assertions=int(matched[1]), cases=int(matched[2]),
                                    executable=identity(executable), terminal=identity(log),
                                    sources=[identity(ROOT / name) for name in SOURCES] + [identity(Path(__file__))],
                                    native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                                    component_only=True, mutable_supply_commands_enabled=False,
                                    runtime_procurement_acceptance=False), indent=2) + "\n", encoding="utf-8")
    return identity(target)


def bind(members):
    """Bind the two actual component passes to original current sources and products.

    @param members Explicit native-build membership files.
    @return Complete component handoff, with native-file and procurement acceptance separate.
    """
    configurations = []
    # Each independently linked executable must pass its own original cases;
    # copied metadata or a dated product cannot substitute for either build.
    for member in members:
        reference = read(member)["report"]
        assert identity(ROOT / reference["path"]) == reference
        proof = read(ROOT / reference["path"])
        assert proof["complete"] and proof["component_only"] and not proof["mutable_supply_commands_enabled"]
        # Exact tested source/product/process bytes define this component result.
        for original in [proof["executable"], proof["terminal"], *proof["sources"], *proof["native_processes"]]:
            assert identity(ROOT / original["path"]) == original
        configurations.append(dict(report=reference, **proof))
    assert len(configurations) == 2 and {row["configuration"] for row in configurations} == {"RelWithDebInfo", "Debug"}
    return dict(_documentation=__doc__, complete=True, configurations=configurations,
                native_order_owner_implemented=True, native_file_acceptance=False,
                runtime_procurement_acceptance=False)


def main():
    """Run one bounded native component burst or bind explicit both-build reports."""
    parser = argparse.ArgumentParser(description=__doc__)
    scope = parser.add_mutually_exclusive_group(required=True)
    scope.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    scope.add_argument("--bind", nargs=2, type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = bind([ROOT / path for path in args.bind]) if args.bind else dict(_documentation=__doc__, report=execute(args.configuration))
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
