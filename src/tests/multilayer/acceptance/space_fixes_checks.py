"""Run one bounded native Space characterization in a disposable repository home.

@file space_fixes_checks.py Runtime inputs and logs stay under test-cache/logs.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups1-7.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group14B: retained-file asteroid preview.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group25: saved local legacy departure.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group26: ownerless no-target Return transport.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group28: native retirement from a declared geometry checkpoint.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group28B: native creation from an already funded declared due checkpoint.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group29: actual StateGameLoop lifecycle opportunities.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group30B: actual policy input and original native work/cargo.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups31B/31C: original roles, actual returns and bounded receiver preparation.
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31E: earned lifecycle phases and production scheduler observation.
"""

import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[4]
TEMPLATES = ROOT / "src/tests/fixtures/config/multilayer/retained/build"


def identity(path):
    """Record immutable input bytes so later observations remain attributable.

    @param path Existing input file inside this checkout.
    @return Repository-relative filename and its SHA256 digest.
    """
    return {"path": path.relative_to(ROOT).as_posix(),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def run(args):
    """Load one demo with its own configuration and a hard process deadline.

    @param args Explicit save/configuration/scale and unique output identity.
    @return Actual native process status; evidence is retained on failure.
    """
    source = (ROOT / args.source if args.source else ROOT / "src/tests/fixtures/demos/space" / args.save).resolve()
    # Retained fixtures and explicit current-run checkpoints remain checkout
    # owned; reload comparisons never read or alter a user's personal saves.
    if not source.is_file() or not any(source.is_relative_to(ROOT / owner) for owner in ("src/tests/fixtures", "savegames/retained", "regression", "test-cache/space-fixes")):
        raise ValueError("select an existing repository fixture, retained save or Space fixes checkpoint")
    if not re.fullmatch(r"[a-z0-9-]+", args.run):
        raise ValueError("run identity must be lowercase letters, digits or dashes")
    output = ROOT / "test-cache/space-fixes" / args.run
    output.mkdir(parents=True, exist_ok=False)
    runtime = output / "runtime"
    runtime.mkdir()
    # Private copies prevent configuration, scripts, saves and NewGRF scans from
    # altering either the retained fixture or another running game instance.
    for folder in ("baseset", "newgrf"):
        shutil.copytree(ROOT / "build" / folder, runtime / folder)
    (runtime / "lang").mkdir()
    for language in ("english.lng", "english_US.lng"):
        shutil.copy2(ROOT / "build/lang" / language, runtime / "lang" / language)
    config = runtime / "space-fixes.cfg"
    settings = (TEMPLATES / "play.cfg").read_text(encoding="utf-8-sig")
    settings = re.sub(r"(?m)^gui_scale\s*=.*$", f"gui_scale = {args.scale}", settings)
    settings = re.sub(r"(?m)^medium_size\s*=.*$", f"medium_size = {args.font_size}", settings)
    config.write_text(settings, encoding="utf-8")
    if args.hotkeys != "fresh":
        shutil.copy2(TEMPLATES / "hotkeys.cfg", runtime / "hotkeys.cfg")
    if args.hotkeys == "custom":
        hotkeys = configparser.ConfigParser(interpolation=None)
        hotkeys.read(runtime / "hotkeys.cfg", encoding="utf-8")
        # Customize only copies, with identical names in both native toolbars.
        # An unrelated Debug remapping must survive the default-key migration.
        for section in ("maintoolbar", "scenedit_maintoolbar"):
            hotkeys[section]["map_layer_space"] = "ALT+9"
        hotkeys["global"]["money"] = "CTRL+SHIFT+M"
        with (runtime / "hotkeys.cfg").open("w", encoding="utf-8") as sink:
            hotkeys.write(sink)
    (runtime / "scripts").mkdir()
    (runtime / "scripts/game_start.scr").write_text(
        "# Owner: docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md\n"
        "pause\nspace_fixes_observe\nquit\n", encoding="utf-8")
    executable = ROOT / "build" / args.configuration / "openttd_space_fixes_test.exe"
    environment = dict(os.environ, OTTD_SPACE_FIXES_SOURCE=str(source),
                       OTTD_SPACE_FIXES_CONFIG=str(config), OTTD_SPACE_FIXES_OUTPUT=str(output),
                       OTTD_SPACE_FIXES_RESOLUTION=args.resolution)
    if args.layout:
        environment["OTTD_SPACE_FIXES_LAYOUT"] = "1"
    if args.asteroid_preview:
        environment["OTTD_SPACE_FIXES_ASTEROID_PREVIEW"] = "1"
    if args.cargo_origin:
        environment["OTTD_SPACE_FIXES_CARGO_ORIGIN"] = "1"
    if args.preview_source:
        candidate = (ROOT / args.preview_source).resolve()
        # Independent previews read a separate repository candidate. Full
        # teardown and explicit policy edits belong to their separate modes.
        if not candidate.is_file() or not candidate.is_relative_to(ROOT) or args.cargo_full or args.mining_policy or args.legacy_departure or args.survey_departure or args.asteroid_retirement or args.asteroid_creation:
            raise ValueError("preview-source requires a separate repository file and a read-only observer")
        environment["OTTD_SPACE_FIXES_PREVIEW_CANDIDATE"] = str(candidate)
    if args.mining_policy:
        # The actual command changes one existing Mine only; corrupt-load
        # observers cannot combine a deliberate edit with refusal purity checks.
        if not args.cargo_origin or args.cargo_bad:
            raise ValueError("mining policy observation requires --cargo-origin and a valid candidate")
        environment["OTTD_SPACE_FIXES_MINING_POLICY"] = "1"
    if args.legacy_departure:
        # A declared local departure transition cannot be combined with a
        # corrupt-load purity check or a separate policy-edit component.
        if not args.cargo_origin or args.cargo_bad or args.mining_policy:
            raise ValueError("legacy departure requires --cargo-origin and an unchanged valid source")
        environment["OTTD_SPACE_FIXES_LEGACY_DEPARTURE"] = args.legacy_departure
    if args.survey_departure:
        # Ownerless pre-erasure state is a separate declared transport component.
        # Corrupt-load/isolation observers and other deliberate edits keep their
        # original unchanged-world contracts and cannot share this mode.
        if not args.cargo_origin or args.cargo_bad or args.mining_policy or args.legacy_departure:
            raise ValueError("survey departure requires --cargo-origin and a valid unedited ownerless source")
        environment["OTTD_SPACE_FIXES_SURVEY_DEPARTURE"] = "1"
    if args.asteroid_retirement:
        # The explicit geometry/retirement component has its own original-world
        # comparisons; it cannot share corrupt-load purity or another edit mode.
        if not args.cargo_origin or args.cargo_bad or args.mining_policy or args.legacy_departure or args.survey_departure:
            raise ValueError("asteroid retirement requires --cargo-origin and an unedited valid mining source")
        environment["OTTD_SPACE_FIXES_ASTEROID_RETIREMENT"] = "1"
    if args.asteroid_creation:
        # Actual native publication uses an already funded original field.
        # Its declared due checkpoint cannot share corrupt-load purity checks
        # or another deliberate edit of the same original owner.
        if not args.cargo_origin or args.cargo_bad or args.mining_policy or args.legacy_departure or args.survey_departure or args.asteroid_retirement:
            raise ValueError("asteroid creation requires --cargo-origin and an unchanged funded source")
        environment["OTTD_SPACE_FIXES_ASTEROID_CREATION"] = "1"
    if args.asteroid_lifecycle:
        # Actual ordinary opportunities deliberately advance original owners.
        # Purity, corrupt-load and separate edit observers therefore cannot
        # share this lifecycle mode or its declared geometry/deadline inputs.
        if args.cargo_origin or args.cargo_bad or args.preview_source or args.mining_policy or args.legacy_departure or args.survey_departure or args.asteroid_retirement or args.asteroid_creation:
            raise ValueError("asteroid lifecycle requires a separate unchanged working-miner source")
        environment["OTTD_SPACE_FIXES_ASTEROID_LIFECYCLE"] = "1"
    if args.mining_controls:
        # This mode deliberately edits an existing queue/policy and may offer
        # real pursuit/extraction opportunities; unchanged-world observers and
        # other explicit edits therefore need their own separate processes.
        if args.cargo_origin or args.cargo_bad or args.preview_source or args.mining_policy or args.legacy_departure or args.survey_departure or args.asteroid_retirement or args.asteroid_creation or args.asteroid_lifecycle:
            raise ValueError("mining controls require a separate retained native source")
        environment["OTTD_SPACE_FIXES_MINING_CONTROLS"] = args.mining_controls
    if args.cargo_bad:
        bad = (ROOT / args.cargo_bad).resolve()
        # Only an explicitly selected invocation-owned repository copy may be
        # subjected to rejected loading; neither user files nor fixtures change.
        if not args.cargo_origin or not bad.is_file() or not bad.is_relative_to(ROOT / "test-cache/space-fixes"):
            raise ValueError("cargo refusal requires a test-cache/space-fixes copy and --cargo-origin")
        environment["OTTD_SPACE_FIXES_CARGO_BAD"] = str(bad)
    if args.cargo_full:
        if not args.cargo_bad:
            raise ValueError("--cargo-full requires --cargo-bad")
        environment["OTTD_SPACE_FIXES_CARGO_FULL"] = "1"
    # The classic-world shortcut-only mode uses the same input checks while
    # deliberately omitting the later Space-specific construction clock.
    if args.shortcuts or args.shortcuts_only:
        environment["OTTD_SPACE_FIXES_SHORTCUTS"] = "1"
    if args.shortcuts_only:
        environment["OTTD_SPACE_FIXES_SHORTCUTS_ONLY"] = "1"
    if args.expect_layers is not None:
        environment["OTTD_SPACE_FIXES_EXPECT_LAYERS"] = str(args.expect_layers)
    if args.construction is not None:
        environment["OTTD_SPACE_FIXES_CONSTRUCTION"] = args.construction
    if args.hotkeys == "custom":
        environment["OTTD_SPACE_FIXES_CUSTOM"] = "1"
    log = ROOT / "logs/space-fixes" / (args.run + ".log")
    log.parent.mkdir(parents=True, exist_ok=True)
    report = {"_documentation": "r2sff1-space-fixes-features-1.md focused Space and GUI checks",
              "source": identity(source), "executable": identity(executable),
              "config": identity(config), "scale": args.scale, "hotkeys": args.hotkeys,
              "hotkey_file": identity(runtime / "hotkeys.cfg") if (runtime / "hotkeys.cfg").is_file() else None,
              "layout": args.layout, "shortcuts": args.shortcuts, "shortcuts_only": args.shortcuts_only,
              "resolution": args.resolution, "font_size": args.font_size, "expect_layers": args.expect_layers,
              "construction": args.construction, "asteroid_preview": args.asteroid_preview,
              "cargo_origin": args.cargo_origin, "cargo_bad": identity(bad) if args.cargo_bad else None, "cargo_full": args.cargo_full,
              "mining_policy": args.mining_policy, "legacy_departure": args.legacy_departure,
              "preview_source": identity(candidate) if args.preview_source else None, "survey_departure": args.survey_departure,
              "asteroid_retirement": args.asteroid_retirement, "asteroid_creation": args.asteroid_creation,
              "asteroid_lifecycle": args.asteroid_lifecycle, "mining_controls": args.mining_controls}
    started = time.monotonic()
    with log.open("w", encoding="utf-8") as sink:
        try:
            process = subprocess.run([str(executable), "[.space-fixes-native]"], cwd=runtime,
                                     env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
            report["exit_code"] = process.returncode
        except subprocess.TimeoutExpired:
            report["exit_code"] = 124
            sink.write("\nNative characterization exceeded its 110-second process budget.\n")
    report["elapsed_seconds"] = round(time.monotonic() - started, 3)
    report["log"] = log.relative_to(ROOT).as_posix()
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))
    return report["exit_code"]


def main():
    """Parse a single explicitly selected focused run.

    @return Native process exit code; no implicit full regression is dispatched.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preview-source", help="Independently preview another repository candidate with this source world open")
    parser.add_argument("--mining-policy", action="store_true", help="Exercise actual registered policy edits on one restored Mine before native resave")
    parser.add_argument("--legacy-departure", choices=("running", "stopped"), help="Save a declared local departure on an actual unordered leased miner without moving it")
    parser.add_argument("--survey-departure", action="store_true", help="Save a declared no-target Return on an actual ownerless attachment without erasing its body")
    parser.add_argument("--asteroid-retirement", action="store_true", help="Project body0's declared final-contained geometry and exercise native atomic retirement with original cargo intact")
    parser.add_argument("--asteroid-creation", action="store_true", help="Declare a due checkpoint in an already funded field and exercise one actual replacement publication")
    parser.add_argument("--asteroid-lifecycle", action="store_true", help="Exercise actual ordinary lifecycle opportunities on original stopped miners, including pause/editor/hidden view and one due creation")
    parser.add_argument("--mining-controls", choices=("edit", "edit-light", "unsupported", "continue", "continue-light", "full", "full-light", "surface", "no-mineral", "no-mineral-light", "receiver-inventory", "receiver-prepare", "receiver-fallback", "receiver-remove", "mineral-wake", "mineral-wake-light", "service-pending", "service-shell", "service-parts", "service-module", "resume-service", "resume-service-light", "stop-recall", "stop-recall-light", "lifecycle", "lifecycle-hidden", "lifecycle-speed", "prepare-high-id", "work-retry", "replay", "replay-light"), help="Exercise native policy input, ordinary role/full/Surface handling, named waits, real paid service/recovery, explicit stop/Recall, earned lifecycle/view/speed or high-ID preparation, measured retry work, or saved next-decision replay")
    parser.add_argument("--save", default="space-factory-440.sav")
    parser.add_argument("--source", help="Explicit repository-relative immutable fixture or retained save")
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--scale", type=int, choices=(100, 150, 200, 250), default=200)
    parser.add_argument("--hotkeys", choices=("fresh", "retained", "custom"), default="retained")
    parser.add_argument("--run", required=True)
    parser.add_argument("--layout", action="store_true")
    parser.add_argument("--asteroid-preview", action="store_true", help="Check the retained save through native preview without changing the open world")
    parser.add_argument("--cargo-origin", action="store_true", help="Observe original CAPA migration, native preview and one normal resave")
    parser.add_argument("--cargo-bad", help="Invocation-owned malformed cargo save under test-cache/space-fixes")
    parser.add_argument("--cargo-full", action="store_true", help="Expect native full-load rejection of --cargo-bad in the disposable process")
    parser.add_argument("--shortcuts", action="store_true")
    parser.add_argument("--shortcuts-only", action="store_true")
    parser.add_argument("--expect-layers", type=int, choices=(1, 2, 3, 4))
    replay_cases = tuple(f"{action}-{stage}" for action in ("checkpoint", "reload") for stage in ("queued", "materials", "power", "working"))
    parser.add_argument("--construction", choices=("waiting", "working", "queue", "service", "removed", "delivery", "interruptions", "workers", "presentation") + replay_cases)
    parser.add_argument("--resolution", choices=("1920x1027", "1280x720", "1024x768"), default="1920x1027")
    parser.add_argument("--font-size", type=int, choices=(0, 16, 20), default=0)
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
