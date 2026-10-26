"""Compare one native construction control with a separately loaded checkpoint.

@file construction_reload_checks.py Bounded paired processes and saved-owner evidence.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group7.
"""

import argparse
import json
from space_fixes_checks import ROOT, run


def observations(report):
    """Read only explicit authoritative checkpoint/final comparison records.

    @param report Completed native run report, including its repository log path.
    @return Phase-keyed scalar fields; missing or duplicate phases are errors.
    """
    result = {}
    # Ignore human-readable boundary traces. Each canonical comparison record
    # covers full owner digests as well as directly reviewable work/accounting.
    for line in (ROOT / report["log"]).read_text(encoding="utf-8").splitlines():
        if not line.startswith("CONSTRUCTION_REPLAY "):
            continue
        values = dict(item.split("=", 1) for item in line.split()[1:])
        phase = values.pop("phase")
        if phase in result:
            raise AssertionError(f"duplicate observation phase: {phase}")
        result[phase] = values
    if set(result) != {"checkpoint", "final"}:
        raise AssertionError("native run did not report both comparison boundaries")
    return result


def check(args):
    """Run an uninterrupted control, then reload its exact saved checkpoint.

    @param args One stage and unique output prefix; no implicit regression matrix.
    @return Zero only when both native checks and every saved-owner comparison pass.
    """
    source = "space-module-cancelled-418.sav" if args.stage in ("queued", "materials") else "space-module-power-418.sav"
    reports = []
    # The second process depends on the first process's real native save. Run
    # sequentially, retaining per-process hard deadlines in the shared harness.
    for action in ("checkpoint", "reload"):
        name = f"{args.run}-{action}"
        settings = argparse.Namespace(save=source, source=None, configuration=args.configuration,
                                      scale=200, hotkeys="retained", run=name, layout=False,
                                      shortcuts=False, shortcuts_only=False, expect_layers=None,
                                      construction=f"{action}-{args.stage}", resolution="1920x1027", font_size=0,
                                      asteroid_preview=False, cargo_origin=False, cargo_bad=None, cargo_full=False,
                                      preview_source=None, mining_policy=False, legacy_departure=None,
                                      survey_departure=False, asteroid_retirement=False, asteroid_creation=False,
                                      asteroid_lifecycle=False, mining_controls=None)
        if action == "reload":
            settings.source = f"test-cache/space-fixes/{args.run}-checkpoint/checkpoint.sav"
        status = run(settings)
        reports.append(json.loads((ROOT / "test-cache/space-fixes" / name / "report.json").read_text(encoding="utf-8")))
        if status != 0:
            return status
    control, reloaded = (observations(report) for report in reports)
    differences = {}
    # Compare both the paused saved world and the identical continuation. This
    # catches lost packets/queues immediately and divergent work/upkeep later.
    for phase in ("checkpoint", "final"):
        for key in control[phase].keys() | reloaded[phase].keys():
            left, right = control[phase].get(key), reloaded[phase].get(key)
            if left != right:
                differences[f"{phase}.{key}"] = {"control": left, "reload": right}
    result = {"_documentation": "r2sff1-space-fixes-features-1.md Group7",
              "stage": args.stage, "control": control, "reload": reloaded,
              "differences": differences, "passed": not differences}
    target = ROOT / "test-cache/space-fixes" / f"{args.run}-reload" / "comparison.json"
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"comparison": target.relative_to(ROOT).as_posix(), "passed": not differences, "differences": differences}))
    return 1 if differences else 0


def main():
    """Select one focused comparison so cases remain short and independently repeatable.

    @return Paired native/comparison exit status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("queued", "materials", "power", "working"), required=True)
    parser.add_argument("--run", required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    return check(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
