"""Run bounded explicit paid-hauler scopes with exact parent and independent refusal membership.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
At most five native checkpoints or two actual refusal processes run in one scope; previous scopes remain explicit immutable inputs.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context

HERE = Path(__file__).parent
SHARED = runpy.run_path(str(HERE / "processor_freight_checks-1.py"))
REFUSALS = runpy.run_path(str(HERE / "processor_freight_checks-2.py"))
GROUPS = {"native": ("inspected", "prepared", "loaded", "travelling", "transferred", "returned"),
          "own": ("prepared", "loaded", "travelling", "transferred", "returned"), "resume": ("loaded", "travelling", "transferred", "returned"),
          "focused": (), "reject-cargo": (), "reject-capacity": (), "reject-quantity": ()}


def execute(configuration, scope, inputs):
    """Run one bounded new scope against explicitly selected predecessor reports.

    @param configuration Matching current native test build.
    @param scope Exact native, own-resave, continuation, component or independent refusal membership.
    @param inputs Explicit earlier native/own scope selectors; latest files cannot supply a parent.
    @return Original newly produced reports and optional malformed candidate provenance.
    """
    parents = {"native": {}, "resave": {}}
    # Only actual original input reports supply parent identities; duplicate or differently configured claims must refuse before any child executes.
    for selector in inputs:
        selected = SHARED["read"](selector); assert selected["configuration"] == configuration and selected["complete"]
        for name in selected["reports"]:
            path = ROOT / name; proof = SHARED["read"](path)
            operation = proof["scope"]; phase = proof["phase"]
            assert proof["complete"] and proof["configuration"] == configuration and operation in parents and phase not in parents[operation]
            assert identity(ROOT / proof["after"]["path"]) == proof["after"]
            parents[operation][phase] = path
    if scope == "focused":
        assert not inputs
        return [SHARED["execute"](configuration, "focused", "inspected", None)], None
    if scope.startswith("reject-"):
        fault = scope.removeprefix("reject-"); parent = parents["native"]["loaded"]; positive = SHARED["read"](parent)
        context = create_run_context("space", "processor-freight-refusal"); context.prepare_runtime()
        candidate = REFUSALS["damage"](context, ROOT / positive["after"]["path"], fault)
        copied = SHARED["execute"](configuration, "preview", "loaded", parent, candidate)
        full = SHARED["execute"](configuration, "full-refusal", "loaded", parent, candidate)
        return [copied, full], dict(fault=fault, source=positive["after"], candidate=identity(candidate), parent=identity(parent))
    produced = []
    # Every checkpoint gets its own isolated child. Own-resave repeats no controls; continuation loads that child's exact saved predecessor before performing the next original action.
    for phase in GROUPS[scope]:
        if scope == "own":
            operation, parent = "resave", parents["native"][phase]
        elif scope == "resume":
            operation, parent = "native", parents["resave"][SHARED["PARENTS"][phase]]
        else:
            operation = "native"; parent = parents["native"][SHARED["PARENTS"][phase]] if phase != "inspected" else None
        report = SHARED["execute"](configuration, operation, phase, parent); produced.append(report)
        if scope == "native":
            assert phase not in parents["native"]; parents["native"][phase] = report
        print(configuration, scope, phase, "passed", flush=True)
    return produced, None


def main():
    """Preserve exact new process membership in one exclusive bounded scope selector."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=tuple(GROUPS), required=True)
    parser.add_argument("--input", type=Path, nargs="*", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); inputs = [p.resolve() for p in args.input]
    reports, refusal = execute(args.configuration, args.scope, inputs)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, scope=args.scope, complete=True,
                                          inputs=[identity(p) for p in inputs], refusal=refusal, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
