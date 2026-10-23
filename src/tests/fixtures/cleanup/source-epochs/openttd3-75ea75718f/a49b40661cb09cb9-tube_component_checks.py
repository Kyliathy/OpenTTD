"""Run focused native tube component edits and retain exact current closure evidence.

@file tube_component_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared floor/materials and loaded packets are mechanical boundary inputs, not useful-route acceptance.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from tube_sprite_report import verify_references

FOCI = {
    "track": "[tube-component-track-edit]",
    "unrelated": "[tube-component-unrelated-edit]",
    "interchange": "[tube-component-interchange-edit]",
    "depot": "[tube-component-depot-edit]",
    "recovery": "[transport-tube-power-recovery]",
    "native-loss": "[tube-geothermal-work-loss]",
    "late-loss": "[tube-geothermal-work-late-loss]",
    "binding-topology": "[tube-geothermal-binding-topology]",
}
SOURCES = (
    "src/transport_tube.h", "src/transport_tube_geothermal.h", "src/transport_tube_topology.cpp",
    "src/transport_tube_power.cpp", "src/transport_tube_geothermal.cpp", "src/transport_tube_runtime.cpp",
    "src/transport_tube_cmd.cpp", "src/utility_power.cpp", "src/rail_cmd.cpp", "src/station_cmd.cpp",
    "src/pathfinder/yapf/yapf_rail.cpp", "src/transport_tube_conduit_sprite.cpp",
    "src/tests/transport_tube_component_edits.cpp", "src/tests/transport_tube_recovery.cpp",
    "src/tests/transport_tube_geothermal_work_loss.cpp", "src/tests/transport_tube_geothermal_binding.cpp",
    "src/tests/geothermal_tube_switch_fixture.h", "src/tests/geothermal_tube_work_fixture.h",
    "src/tests/geothermal_tube_capsule_fixture.h", "src/tests/geothermal_tube_power_fixture.h",
    "src/tests/transport_tube_meter_fixture.h", "src/tests/CMakeLists.txt",
)


def run_one(args):
    """Run one isolated native tag using the invocation-owned runtime and logs.

    @param args Parsed actual configuration and exact focus selection.
    @return None; failed/empty native selection leaves no accepted report.
    """
    assert args.config is not None and args.focus is not None
    context = current_run_context("tubes", "component-edits")
    context.prepare_runtime()
    _, proof = native(context, args.config, FOCI[args.focus], args.focus)
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md14C; actual component edits and original native/external grant/packet recovery; prepared inputs are not useful-route acceptance",
        "configuration": args.config, "focus": args.focus, "native": proof,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-component-{args.config}-{args.focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


def close_reports(args):
    """Verify every exact scoped report and publish original owner/build evidence.

    @param args Parsed sixteen report paths, four build logs and absent output target.
    @return None; missing scopes, stale sources or native failure aborts acceptance.
    """
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    grid, cache, records = set(), {}, []
    # Every layout and prior runtime boundary needs its own isolated native
    # execution. Current source/binary hashes forbid a stale earlier pass from
    # replacing the selected real station/depot and both-source edit checks.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in configurations and focus in FOCI and (cfg, focus) not in grid
        proof = report["native"]
        log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert count is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(count[1]), int(count[2])) == (proof["assertions"], proof["cases"])
        summary[cfg]["cases"] += proof["cases"]
        summary[cfg]["assertions"] += proof["assertions"]
        grid.add((cfg, focus)); records.append({"report": identity(path), **report})
    assert grid == {(cfg, focus) for cfg in configurations for focus in FOCI}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    builds = []
    # Decode the original PowerShell BOM only for success checks; accepted
    # build diagnostics retain their exact original bytes and hashes.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md14C; original physical source components, unused rights and packet custody survive scoped native edits/recovery",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "exact_identities_verified": len(cache), "builds": builds,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "native_version": 434, "packed_art": packed,
        "loaded_cache_receipt": identity(ROOT / "logs/tubes/cache-rebuild/20261018-native-01/verification.json"),
        "ownership_rules": [
            "Actual native track/station/depot notifications invalidate only cached components containing or adjacent to the edited physical cell.",
            "Both original source families retain generated/consumed history and external invoice/native inlet identities; physical changes expire only affected unused rights.",
            "A disconnected paid component and the separate conventional berth never acquire tube electrical reach or revoke an unrelated tube grant.",
            "Native packet17 remains in the original actual capsule; native depot/order and station identities retain their existing lifecycle ownership.",
            "Stable map/set/trackdir traversal derives bounded components only when dirty; repeated lookup and routine allocation never scan map planes or churn native routing.",
        ],
        "fixture_classification": "Prepared floor/materials and native loaded stock remain explicit mechanical inputs; original generator, utilities, leased service, vehicles and physical edits use their real command owners.",
        "remaining": "14.3 ownership,14.4 queued/starved files,15 useful introduction-date routes; runtime gallery and human acceptance remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}))


def main():
    """Dispatch one bounded native scope or verify its explicit complete closure matrix.

    @return None; argument or proof failures leave acceptance absent.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--focus", choices=FOCI)
    parser.add_argument("--reports", nargs=16)
    parser.add_argument("--build-logs", nargs=4)
    parser.add_argument("--output")
    args = parser.parse_args()
    if args.reports is None:
        run_one(args)
    else:
        assert args.output is not None and args.build_logs is not None
        close_reports(args)


if __name__ == "__main__":
    main()
