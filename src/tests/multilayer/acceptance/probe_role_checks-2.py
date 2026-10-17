"""Exercise genuine typed freight and service actions in independently bounded native children.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
Exact accepted paid checkpoints supply every hull, pod and part; this adapter creates no gameplay stock or poses.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, run_native

PHASES = ("focused", "unload-orbital", "service-supply", "first-pending", "first-return", "mine-again", "second-pending", "service-partial",
          "service-pause", "service-resume", "service-complete", "repeat", "service-center-loss", "service-after-loss")


def service_serial(source, original):
    """Read the original actual service-history baseline without resetting or assuming a fresh hull.

    @param source Exact accepted native source save.
    @param original Original paid hull index/incarnation.
    @return Its actual completed-service serial from the bounded original SPSU row.
    """
    data = chunks(source)[1][b"SPSU"][3:]; cursor = 1; found = None; assert data[0] <= 64
    key = original["vehicle"].to_bytes(4, "big") + original["generation"].to_bytes(8, "big")
    # Every preceding shell/compartment extent belongs to another real hull; walk those complete rows before selecting this exact original and its retained service history.
    for _ in range(data[0]):
        start = cursor; assert cursor + 61 <= len(data); pods = data[cursor + 60]; assert pods <= 8; cursor += 61 + 10 * pods
        for _ in range(3):
            assert cursor < len(data); count = data[cursor]; assert count <= 128; cursor += 1 + 6 * count
        assert cursor <= len(data)
        if data[start:start + 12] == key:
            assert found is None; found = int.from_bytes(data[start + 52:start + 60], "big")
    assert cursor == len(data) and found is not None and found < (1 << 64) - 1
    return found


def entry(configuration, orbital):
    """Resolve one original paid source from explicit immutable accepted producer membership.

    @param configuration Matching current native configuration.
    @param orbital True selects the full paid20 replacement product; false selects the original Earth-launched15 collection completion.
    @return Producer report path, exact source save identity and original actual craft incarnation.
    """
    index = ROOT / ("logs/manufacturing-replacement-verification.json" if orbital else "logs/probe-shell-verification.json")
    expected = "6a6e1bc848c551c9523b2970043af302c8ff5759f1963d6bc347302eaf648878" if orbital else "f27cd42c31421ad8f6e5b7ae2e8d807f6b16d8cbf1a4402c2612abfe61fe1ebd"
    assert identity(index)["sha256"] == expected
    proof = json.loads(index.read_text(encoding="utf-8"))
    accepted = next(row for row in proof["configurations"] if row["configuration"] == configuration)
    if orbital:
        candidates = []
        # Original main completion alone supplies the real paid product; ordinary reloads and edited lifecycle branches cannot silently choose another original or cargo bill.
        for member in accepted["reports"]:
            path = ROOT / member["path"]; assert identity(path) == member; report = json.loads(path.read_text(encoding="utf-8"))
            if report.get("variant") == "main" and report.get("phase") == "complete" and report.get("model") == 20 and "after" in report:
                candidates.append((path, report["after"], report["product"]))
        assert len(candidates) == 1
        path, source, original = candidates[0]
        original = dict(original, service_base=service_serial(ROOT / source["path"], original))
        return path, source, original
    path = ROOT / accepted["report"]["path"]; assert identity(path) == accepted["report"]
    report = json.loads(path.read_text(encoding="utf-8")); source = ROOT / report["complete"]["path"]; assert identity(source) == report["complete"]
    version, owners = chunks(source); assert version == 424
    rows = owners[b"SVST"][3:]; count = rows[0]; assert len(rows) == 1 + count * 49
    bodies = [rows[1 + number * 49:1 + (number + 1) * 49] for number in range(count)]
    miners = [row for row in bodies if int.from_bytes(row[41:43], "big") == 15]
    assert len(miners) == 1; original = miners[0]
    original = dict(vehicle=int.from_bytes(original[:4], "big"), generation=int.from_bytes(original[4:12], "big"))
    original["service_base"] = service_serial(source, original)
    return path, report["complete"], original


def native(context, configuration, phase, source, original, corrupt=None, reject=False):
    """Run one actual native observer with strict original identity and independent process/time bounds.

    @param context Unique current report/log/runtime owner.
    @param configuration Matching current completed test/game configuration.
    @param phase Actual requested typed navigation/freight/return/service action.
    @param source Exact independently accepted native predecessor save.
    @param original Original paid hull incarnation, never a newest/first-hull heuristic.
    @param corrupt Optional malformed file for independent copied-preview refusal while source remains open.
    @param reject True requires startup full-load refusal before the observer.
    @return Successful assertion count and actual original/progress metadata, or None after an intended full-load refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("probe_order_actions_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_PROBE_ORDER_SOURCE=str(source), OTTD_PROBE_ORDER_ACTION=phase,
                       OTTD_PROBE_ORDER_VEHICLE=str(original["vehicle"]), OTTD_PROBE_ORDER_GENERATION=str(original["generation"]),
                       OTTD_PROBE_ORDER_SERVICE_BASE=str(original["service_base"]),
                       OTTD_TEST_CONFIGURATION=configuration)
    environment.pop("OTTD_PROBE_ORDER_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_PROBE_ORDER_CORRUPT"] = str(corrupt)
    log = context.logs / "probe-order-actions.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.probe-order-actions-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode != 0 and passed is None and "Broken savegame" in evidence, log
        assert any(owner in evidence.lower() for owner in ("space order", "space fleet", "service", "manifest", "launch", "station")), log
        return None
    marker = re.search(r"PROBE_ORDERS native=true action=(\S+) vehicle=(\d+) generation=(\d+) cursor=(\d+) work=(\d+) returns=(\d+) pending=(\d+) services=(\d+) service_ticks=(\d+) cargo=(\d+)", evidence)
    # A successful unrelated case or a startup that never reached the selected observer cannot supply native acceptance.
    assert result.returncode == 0 and passed and int(passed[2]) == 1 and marker and marker[1] == phase, log
    assert dict(vehicle=int(marker[2]), generation=int(marker[3])) == {key: original[key] for key in ("vehicle", "generation")}, log
    if corrupt is not None:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    keys = ("cursor", "work", "returns", "pending", "services", "service_ticks", "cargo")
    return int(passed[1]), dict(zip(keys, (int(marker[index]) for index in range(4, 11))))


def verify(configuration, phase, predecessor=None, branch="main"):
    """Bind one current focused/native action to its exact physical predecessor and issued original.

    @param configuration Matching current native build.
    @param phase One explicit independently bounded acceptance operation.
    @param predecessor Exact accepted current report, required after original producer entry.
    @param branch Explicit independent main or paused-service continuation owner.
    @return Current immutable accepted report path.
    """
    assert phase in PHASES and branch in ("main", "paused", "lost")
    context = current_run_context("space", f"probe-orders-{branch}-{phase}")
    target = context.reports / "probe-order-actions.json"; assert not target.exists()
    if phase == "focused":
        assert predecessor is None; log = context.logs / "probe-order-actions-focused.log"
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[probe-order-actions]"],
                       cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed and int(passed[2]) == 4
        result = dict(assertions=int(passed[1]), cases=int(passed[2]), log=identity(log))
    else:
        if phase in ("unload-orbital", "service-supply"):
            assert predecessor is None; previous, selected, original = entry(configuration, phase == "unload-orbital")
        else:
            assert predecessor is not None; previous = predecessor
            proof = json.loads(previous.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["save_version"] == 430
            selected, original = proof["after"], proof["original"]
        source = ROOT / selected["path"]; assert identity(source) == selected
        assertions, metadata = native(context, configuration, phase, source, original)
        after = context.saves / f"probe-orders-{phase}-430.sav"; assert chunks(after)[0] == 430 and identity(source) == selected
        result = dict(assertions=assertions, predecessor=identity(previous), source=selected, after=identity(after), original=original, **metadata)
    result.update(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md Group8B; actual typed native freight and paid service continuation. Original accepted checkpoints supply all hulls, cargo, parts and paid pods; no stock/pose/allowance injection or procurement claim.",
                  configuration=configuration, phase=phase, branch=branch, save_version=430, procurement_acceptance=False,
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run exactly one current focused/native boundary with an optional explicit accepted predecessor."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--predecessor", type=Path)
    parser.add_argument("--branch", choices=("main", "paused", "lost"), default="main")
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.predecessor.resolve() if args.predecessor else None, args.branch).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
