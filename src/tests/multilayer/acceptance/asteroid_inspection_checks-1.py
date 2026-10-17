"""Capture actual company-facing asteroid/probe widgets from exact original paid and completed-scan saves.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14
Original gameplay owners are compared before/after presentation; native preview sprites claim no final art acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from PIL import Image
from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native, save_native_capture

AUDIT = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-3.py")))
ENTRIES = {
    "unknown": ("logs/probe-return-verification.json", "b1c57dee86998772c8dad1b13a091f432656ee6ee39840ef6d6a25e4e2e47953", "native/region"),
    "mining": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/second-partial"),
    "pending": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/first-pending"),
    "service": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/service-partial"),
}
SCOPES = {"native-a": tuple(ENTRIES), "native-b": ("scan", "known", "foreign", "construction"), "own": ("pending", "known", "construction"), "focused": ("focused",)}
TAGS = "[asteroid-inspection],[space-survey-contract],[probe-order-actions]"
MISSION = ("logs/probe-mission-verification.json", "b5c1086261f032fd93a2e8de658ea7020bb2b8b2f116b5f93a9c8fcae20b4936")
CONSTRUCTION = ("logs/space-construction-verification.json", "d2e1f6686e75070feaa9ead8ff44b18d76de133309ca5b38a33a10c9f8198eaa")
WORK = {"path": "mlm/reviews/space/construction-prepare/20261013-053250-b304881ae7/saves/construction-work-415.sav", "sha256": "45f71ef532c131b4485e8c015e1e7fad0519e78a534f8d52aced84b17ab165fb"}
PROCESS = {"path": "mlm/reviews/space/construction-prepare/20261013-053250-b304881ae7/reports/native-b9ec3904622044c2bf27978edfc6f445.json", "sha256": "64372dd7f395a1599676e2211885922c9c5c33cff2c4cafe0996081f5abda079"}


def read(path):
    """Read only explicit retained evidence.

    @param path Exact original report/index/selector path.
    @return Parsed original evidence, without newest-directory discovery.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, phase):
    """Resolve this original current or legacy paid/scan boundary without requalifying historical binaries.

    @param configuration Current executable and matching original producer where both exist.
    @param phase Explicit unknown/mining/pending/service/scan/known/foreign/construction input.
    @return Dated direct bindings, actual source save and its original selected craft identity.
    """
    phase = "unknown" if phase == "focused" else phase
    if phase in ENTRIES:
        relative, sha, checkpoint = ENTRIES[phase]; entry = ROOT / relative; assert identity(entry)["sha256"] == sha
        selected, = [c for c in read(entry)["configurations"] if c["configuration"] == configuration]
        source = selected["checkpoints"][checkpoint]; original = dict(vehicle=16, generation=2); dated = [identity(entry)]
    elif phase in ("scan", "known", "foreign"):
        relative, sha = MISSION; entry = ROOT / relative; assert identity(entry)["sha256"] == sha
        selected, = [c for c in read(entry)["configurations"] if c["configuration"] == configuration]; wanted = "scan-partial" if phase == "scan" else "scan-complete"
        # Only actual local physical reports own these original saved scan states; ordinary continuation rows and network alternatives cannot substitute another paid provenance.
        matches = [(ident, read(ROOT / ident["path"])) for ident in selected["reports"]]
        ident, proof = next((ident, proof) for ident, proof in matches if proof.get("action") == wanted and proof.get("branch") == "local" and proof.get("after"))
        assert identity(ROOT / ident["path"]) == ident; source = proof["after"]; original = proof["original"]; dated = [identity(entry), ident]
    else:
        assert phase == "construction"; relative, sha = CONSTRUCTION; entry = ROOT / relative; assert identity(entry)["sha256"] == sha
        index = read(entry); verified = index["native_verification"]; assert identity(ROOT / verified["path"]) == verified and identity(ROOT / PROCESS["path"]) == PROCESS
        process = read(ROOT / PROCESS["path"]); assert process["exit_status"] == 0 and WORK in process["artifacts"] and read(ROOT / verified["path"])["input"] in process["inputs"]
        source = WORK; original = dict(vehicle=18, generation=4); dated = [identity(entry), verified, PROCESS, *process["inputs"]]
    assert identity(ROOT / source["path"]) == source
    return dated, source, original


def execute(configuration, scope, phase, parent):
    """Capture a native original/ordinary state or run the current focused disclosure callers.

    @param configuration Current native executable configuration.
    @param scope Original native batch, own-resaved capture or focused components.
    @param phase Exact required native disclosure/state view.
    @param parent Exact same-phase original capture report for an own-resave.
    @return Exclusive complete current semantic proof path.
    """
    dated, source, original = predecessor(configuration, phase)
    if parent:
        proof = read(parent); assert proof["complete"] and proof["configuration"] == configuration and proof["phase"] == phase
        source = proof["after"]; assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "asteroid-inspection-" + scope); context.prepare_runtime(); environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    tag = TAGS if scope == "focused" else "[.asteroid-inspection-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("asteroid_inspection_test\n", encoding="utf-8")
        environment.update(OTTD_ASTEROID_INSPECT_SOURCE=str(ROOT / source["path"]), OTTD_ASTEROID_INSPECT_PHASE=phase,
                           OTTD_ASTEROID_INSPECT_VEHICLE=str(original["vehicle"]), OTTD_ASTEROID_INSPECT_GENERATION=str(original["generation"]))
    log = context.logs / "asteroid-inspection.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and passed, log; before = after = marker = None; captures = []
    if scope != "focused":
        marker = re.search(r"ASTEROID_INSPECTION native=true phase=(\S+) known=(true|false) observer=(\d+) added_stock=false final_art=false", terminal)
        assert marker and marker[1] == phase and (marker[2] == "true") == (phase == "known")
        before = identity(context.saves / "inspection-before-438.sav"); after = identity(context.saves / "inspection-after-438.sav")
        assert chunks(ROOT / before["path"])[0] == chunks(ROOT / after["path"])[0] == 438; AUDIT["compare"](before, after)
        if scope == "own":
            AUDIT["compare"](source, before)
        ppm = context.images / ("inspector-" + phase + ".ppm"); png = ppm.with_suffix(".png")
        # Native pixels are encoded unchanged. Container provenance retains the exact native writer and source PPM; this is no second render or image edit.
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, png); pixel_sha = hashlib.sha256(raster.tobytes()).hexdigest(); dimensions = list(raster.size)
        captures.append(dict(native=identity(ppm), png=identity(png), pixel_sha256=pixel_sha, dimensions=dimensions))
    report = context.reports / "asteroid-inspection.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(passed[1]), cases=int(passed[2]), dated_producers=dated, parent=identity(parent) if parent else None,
                                     source=source if scope != "focused" else None, original=original if scope != "focused" else None, before=before, after=after,
                                     observer=int(marker[3]) if marker else None, captures=captures, final_art=False, added_stock=False, terminal=identity(log),
                                     conversions=[identity(path) for path in context.reports.glob("capture-conversions.jsonl")],
                                     native_reports=[identity(path) for path in context.reports.glob("native-*.json")]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most four bounded current children with explicit paid/scan ancestry."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True); parser.add_argument("--parent", type=Path, nargs="+", default=[])
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); parents = {}; reports = []
    # Own-resaves retain the original same-phase current proof; an unrelated native/legacy report cannot silently replace its admitted data or observer.
    for path in args.parent:
        selected = read(path); assert selected["complete"] and selected["configuration"] == args.configuration
        parents.update({read(ROOT / relative)["phase"]: ROOT / relative for relative in selected["reports"]})
    for phase in SCOPES[args.scope]:
        parent = parents.get(phase) if args.scope == "own" else None; assert (parent is not None) == (args.scope == "own")
        report = execute(args.configuration, args.scope, phase, parent); reports.append(report.relative_to(ROOT).as_posix()); print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
