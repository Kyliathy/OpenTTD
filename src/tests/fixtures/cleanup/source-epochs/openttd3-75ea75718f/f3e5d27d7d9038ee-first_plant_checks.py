"""Run one bounded actual paid first-plant native stage with explicit evidence ownership.

@file first_plant_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group13B selected entry-point boundaries.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group14B selected recovery boundaries.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15B native control providers.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBB atomic town connections.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBC retained-service repair and source replacement.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CB actual site-window actions.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15CC/15CDB actual physical actions/live information/overlay.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15D selected native calendar/policy boundaries.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No helper creates excavation, construction stock, energy or a replacement world.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native
sys.path.insert(0, str(ROOT / "src/tests"))
from save_tables import saved_table, read_stream, chunks, geology_snapshot

CONTEXT = current_run_context("geothermal", "first-plant")

FOCUSED = {
    "origin-retry": "[spoil-receiver-origin-retry]",
    "spoil-delivery": "[spoil-receiver-delivery]",
    "magazine": "[tbm-hauler-magazine-transition]",
    "clearance": "[tbm-hauler-cutter-clearance]",
    "face-access": "[demo-fix-face-access]",
    "face-steering": "[demo-fix-face-steering]",
    "reserved-loop": "[tbm-reserved-loop]",
    "controller-edges": r"Every edge of the face wait\, including its fallback to the bay",
    "controller-wait": "The cutter holds still and needs a hauler while it waits at the face",
    "controller-transfer": "The hauler takes the spoil at the face through its own controller",
    "controller-loop": "A Standard loop serves every breakpoint at the face",
    "controller-layers": "Standard and Deep chamber loops serve the face whenever it has room",
    "tight-chamber": "[tbm-face-tight-chamber]",
    "workflow-excavation": "[underground-workflow-excavation]",
    "depot-cycle": "[tbm-depot-cycle]",
    "conventional-engine": "[underground-engine]",
    "conventional-startup": "[underground-engine-startup]",
    "utility-build": "[geothermal-utility-build]",
    "utility-coexist": "[geothermal-utility-coexist]",
    "utility-path": "[geothermal-utility-path]",
    "utility-port": "[geothermal-utility-port]",
    "riser-link": "[geothermal-riser-link]",
    "town-effects": "[utility-power-effects]",
    "utility-company": "[utility-power-company]",
    "generator-waits": "[geothermal-generator-operating-waits]",
    "generator-zero": "[geothermal-generator-zero-heat]",
    "generator-service": "[geothermal-generator-service]",
    "supply-rating": "[surface-supply-rating]",
    "supply-purpose": "[surface-supply-purpose]",
    "supply-ordinary": "[surface-supply-payment-control]",
    "supply-cancel": "[surface-supply-cancel]",
    "supply-dispatch": "[surface-supply-dispatch]",
    "supply-station-loss": "[surface-supply-station-loss]",
    "generator-funding": "[geothermal-generator-funding]",
    "generator-refusal": "[geothermal-generator-refusal]",
    "magma-construction": "[geothermal-magma-construction]",
    "magma-locks": "[geothermal-magma-locks]",
    "riser-admission": "[geothermal-riser]",
    "riser-construction": "[geothermal-riser-construction]",
    "generator-generic": "[geothermal-generator-generic]",
    "riser-generic": "[geothermal-riser-generic]",
    "generator-company": "[geothermal-generator-operating-company]",
    "generator-removal": "[geothermal-generator-operating-removal]",
    "generator-takeover": "[geothermal-generator-takeover]",
    "generator-abandonment": "[geothermal-generator-abandonment]",
    "generator-reuse": "[geothermal-generator-reuse]",
    "magma-lifecycle": "[geothermal-magma-lifecycle]",
    "magma-contention-loss": "[geothermal-magma-contention-loss]",
    "riser-lifecycle": "[geothermal-riser-lifecycle]",
    "site-commands": "[geothermal-site-commands]",
    "site-funding": "[geothermal-site-command-funding]",
    "site-refusal": "[geothermal-site-command-refusal]",
    "control-inspection": "[geothermal-control-inspection]",
    "control-availability": "[geothermal-control-availability]",
    "site-serialization": "[geothermal-site-command-serialization]",
    "site-ownership": "[geothermal-site-command-ownership]",
    "service-serialization": "[geothermal-service-serialization]",
    "service-connection": "[geothermal-service-connection]",
    "service-refusal": "[geothermal-service-refusal]",
    "service-capacity": "[geothermal-service-capacity]",
    "service-terminal-repair": "[geothermal-service-terminal-repair]",
    "service-riser-repair": "[geothermal-service-riser-repair]",
    "service-source-replacement": "[geothermal-service-source-replacement]",
    "service-recovery-refusal": "[geothermal-service-recovery-refusal]",
    "window-plan": "[geothermal-window-plan]",
    "window-collector": "[geothermal-window-collector]",
    "window-placement": "[geothermal-window-placement]",
    "window-policy": "[geothermal-window-policy]",
    "window-information-stock": "[geothermal-window-information-stock]",
    "window-information-energy": "[geothermal-window-information-energy]",
    "window-information-town": "[geothermal-window-information-town]",
    "window-survey-overlay": "[geothermal-window-survey-overlay]",
    "window-utilities": "[geothermal-window-utilities]",
    "window-riser": "[geothermal-window-riser]",
    "window-physical-refusal": "[geothermal-window-physical-refusal]",
    "window-export": "[geothermal-window-export]",
    "window-export-repair": "[geothermal-window-export-repair]",
    "window-export-refusal": "[geothermal-window-export-refusal]",
    "window-existing-survey": "Underground tools open each rock layer and survey without mutation",
    "window-existing-date": "Deep underground tools explain the canonical calendar unlock",
    "window-existing-placement": "[underground-placement]",
    "window-existing-command": "[underground-viewport-command]",
    "window-existing-escape": "Escape aborts placement before it closes a window",
    "window-tube-controls": "[transport-tube-controls]",
    "window-tube-construction": "[transport-tube-controls-construction]",
    "window-tube-fleet": "[transport-tube-controls-fleet]",
    "window-tube-stale": "[transport-tube-controls-stale]",
    "calendar-planning": "[geothermal-calendar-planning]",
    "calendar-funding": "[geothermal-calendar-funding]",
    "calendar-construction": "[geothermal-calendar-construction]",
    "calendar-operation": "[geothermal-calendar-operation]",
    "calendar-window": "[geothermal-calendar-window]",
    "calendar-editor-funding": "[geothermal-calendar-editor-funding]",
}  # Each invocation gets its own actual native process, diagnostic and count proof.

OWNERS = frozenset(name.encode() for name in """MAPS MAPT MAPH MAPO MAP2 M3LO M3HI MAP5 MAPE MAP7 MAP8 DATE MLMD SDEF
GEOS GDEP GSPN GVNT EXJS XJOB XJRT XJTL XPAS LPWH LPWK TBMH TBMP TBMS SPRH SPRR
TBXH TBXA TBXS TBXW LMNH LMNR LPCH LPCR LPCC UPLH UPLD UPPH UPPD UCVH UCVD TUPH TUPD TUVH TUVD
SSOH SSOR GCSH GCSD GCBH GCBD GPGH GPGD GRSH GRSD GHTH GHTD
VEHS DEPT BKOR ORDL INDY IIDS TIDS EIDS ENGN STNN PLYR CAPA CAPY ECMY RAIL ROTT LGRP LGRJ LGRS""".split())


def identity(path):
    """Identify one exact native producer or retained artifact.

    @param path Existing repository-owned file.
    @return Relative path, byte length and SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def owned(path):
    """Read exact complete native first-plant quantity, geometry and transport authorities.

    @param path Genuine ordinary430 writer original or application resave.
    @return Hashes of actual native owner chunks, independent of test scalars.
    """
    version, stream = read_stream(Path(path))
    assert version in (430, 431), (path, version)
    rows = {row["name"]: row["raw"] for row in chunks(stream)}
    assert OWNERS <= rows.keys(), (path, OWNERS - rows.keys())
    return {name.decode(): hashlib.sha256(rows[name]).hexdigest() for name in sorted(OWNERS)}


def verify_original(source, restored, startup_linkgraph=False):
    """Compare complete paid owners, retaining explicit native manager and no-human-player initialization.

    @param source Exact original before actual application afterload.
    @param restored Its untouched ordinary native resave.
    @param startup_linkgraph Allow only the native startup LinkGraph pause bit before the first complete frame; all other clock fields remain exact.
    @return Exact owner hashes and explicitly observed original-manager initialization; new human creation is separately recorded.
    """
    before, after = owned(source), owned(restored)
    #431 adds reference-only physical town-service columns. A genuine430
    # source retains every original field; only the three explicit absent
    # defaults may be appended by the ordinary current writer.
    original_version, _ = read_stream(Path(source))
    loaded_version, _ = read_stream(Path(restored))
    migrated = original_version == 430 and loaded_version == 431
    if migrated:
        original_rows, loaded_rows = saved_table(source, b"UPLD"), saved_table(restored, b"UPLD")
        assert original_rows.keys() == loaded_rows.keys(), "Service migration changed original utility identities."
        for identity, fields in original_rows.items():
            loaded = loaded_rows[identity]
            assert all(loaded[name] == value for name, value in fields.items()), (identity, "Original utility field changed.")
            defaults = {"town_terminal": 65535, "town_supply": 4294967295, "town_source_port": 4294967295} if fields["type"] == 1 else dict.fromkeys(("town_terminal", "town_supply", "town_source_port"), 0)
            assert set(loaded) - set(fields) == set(defaults)
            assert all(loaded[name] == value for name, value in defaults.items()), (identity, "Older absence acquired service.")
    differences = [name for name in before if before[name] != after[name] and name not in ({"DATE", "PLYR", "UPLD"} if migrated else {"DATE", "PLYR"})]
    assert not differences, differences
    companies, loaded_companies = saved_table(source, b"PLYR"), saved_table(restored, b"PLYR")
    created = set(loaded_companies) - set(companies)
    assert set(companies) <= set(loaded_companies), "Loading removed an original company."
    # Ordinary single-player loading creates one fresh human company if
    # liquidation left only AI companies. Existing identities, utility
    # tombstones and all original quantities remain exact; this is no refund.
    if created:
        expected = next(index for index in range(15) if index not in companies)
        assert created == {expected} and companies and all(row["is_ai"] == 1 for row in companies.values()), "Unexpected new player during ordinary load."
        assert loaded_companies[expected]["is_ai"] == 0
        assert loaded_companies[expected]["money"] == loaded_companies[expected]["current_loan"] == 0, "The declared zero-loan liquidation world gained money."
    initialized = []
    # An unset reduced-scenario manager gets the ordinary application's face
    # once. Every money, allowance, expense and other company field stays exact.
    for identity, fields in companies.items():
        unset = fields["face_style"] == "" and fields["face"] == 0
        if unset:
            initialized.append(identity)
        for name, value in fields.items():
            if unset and name in {"face", "face_style"}:
                continue
            assert value == loaded_companies[identity][name], (identity, name)
    clocks, loaded_clocks = saved_table(source, b"DATE"), saved_table(restored, b"DATE")
    assert clocks.keys() == loaded_clocks.keys(), "Original clock identities changed."
    # Face initialization consumes its normal RNG draws. No game/economy clock,
    # tick counter or unrelated DATE field can change during the paused load.
    for identity, fields in clocks.items():
        for name, value in fields.items():
            if (initialized or created) and name in {"random_state[0]", "random_state[1]"}:
                continue
            # A production game-start script runs before the first complete
            # frame can clear AfterLoad_LinkGraphPauseControl's temporary wait.
            # This permits only that added flag, with unchanged saved jobs;
            # neither elapsed simulation nor another pause reason is admitted.
            if startup_linkgraph and name == "pause_mode" and loaded_clocks[identity][name] == (value | 64):
                assert saved_table(source, b"LGRJ") == saved_table(restored, b"LGRJ")
                continue
            assert value == loaded_clocks[identity][name], (identity, name)
    return before, after, initialized


def native_receipts(output):
    """Retain native quotation, custody and invoice markers without supplying values.

    @param output Exact successful native diagnostic text.
    @return Ordered actual receipts grouped by their emitted marker.
    """
    receipts = {}
    # Each line is emitted by the actual owner inspection at a retained stage;
    # multiple extensions/receivers must remain separate, ordered observations.
    for marker, body in re.findall(r"^FIRST_PLANT_(QUOTE|CASH|SPOIL|RECEIVER|EXTENSION|CONTINUED|INVOICE|RELEASE|BUILD_INVOICE|BUILD_READY|DELIVERY_MATERIAL|DELIVERY_PURCHASE|DELIVERY_READY|UTILITY_CELL|UTILITY_READY|GENERATION_DESTINATION|GENERATION_CONNECTION|GENERATION_READY|MAGMA_DATE|MAGMA_INVOICE|MAGMA_MATERIAL|MAGMA_ENERGY|MAGMA_READY) (.*)$", output, re.MULTILINE):
        fields = {}
        for name, value in re.findall(r"(\w+)=([^\s]+)", body):
            fields[name] = int(value) if re.fullmatch(r"-?\d+", value) else value
        receipts.setdefault(marker.lower(), []).append(fields)
    return receipts


def main():
    """Execute one actual native stage; an empty tag or partial/failing run refuses proof.

    @return None; successful native sources, saves and diagnostics get exact identities.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--stage", choices=("baseline", "access", "deep", "reload", "construction", "delivery", "utilities", "generation", "magma", "focused"), default="baseline")
    parser.add_argument("--focus", choices=tuple(FOCUSED), help="One affected owner boundary, isolated from other tests.")
    parser.add_argument("--input", type=Path, help="Exact retained ordinary original for the reload stage.")
    parser.add_argument("--action", choices=("inspect", "continue", "release", "prepare", "shortage", "deliver", "park", "install", "operate", "boundary", "repair", "upgrade", "wire", "view", "retry"), default="inspect")
    parser.add_argument("--boundary", choices=("path", "sealed", "terminal", "source", "all-sources", "company", "town", "custom", "date"), help="One actual native physical/owner/calendar interruption.")
    parser.add_argument("--town-command", action="store_true", help="Use the synchronized whole connection for actual first-plant town creation, repair or source replacement.")
    args = parser.parse_args()
    # The command must be proved with actual retained physical participants
    # at the precise initial connection or recovery action. An inspection flag
    # cannot fabricate a successful command marker or a source replacement.
    if args.town_command:
        assert (args.stage, args.action) in {("generation", "prepare"), ("generation", "repair"), ("magma", "wire")}, "Town command acceptance requires actual first connection or retained-service repair/source replacement."
    binary = ROOT / f"build/{args.config}/openttd_test.exe"
    tag = f"[geothermal-first-plant-{args.stage}]"
    label = args.stage
    if args.stage == "focused":
        assert args.focus is not None, "Focused validation requires one explicit owner boundary."
        tag = FOCUSED[args.focus]
        label = args.focus
    environment = os.environ.copy()
    environment["OTTD_FIRST_PLANT_TOWN_COMMAND"] = "1" if args.town_command else "0"
    if args.boundary:
        environment["OTTD_FIRST_PLANT_BOUNDARY"] = args.boundary
    # Ordinary loading consumes the explicitly retained input; it cannot
    # construct another fixture world and call that a continuation.
    file_modes = {
        "reload": ("[.geothermal-first-plant-native-file]", "first_plant_check"),
        "construction": ("[.geothermal-first-plant-construction-file]", "first_plant_build"),
        "delivery": ("[.geothermal-first-plant-delivery-file]", "first_plant_deliver"),
        "utilities": ("[.geothermal-first-plant-utility-file]", "first_plant_wire"),
        "generation": ("[.geothermal-first-plant-generation-file]", "first_plant_generate"),
        "magma": ("[.geothermal-first-plant-magma-file]", "first_plant_magma"),
    }  # Exact native file owners select their startup command; a label never substitutes for a genuine input.
    if args.stage in file_modes:
        assert args.input is not None and args.input.is_file(), "Reload requires an exact existing original."
        tag, command = file_modes[args.stage]
        startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
        startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text(f"# @see r2mlvs18-geothermal-energy-and-industry.md8FA/8FBA/8FBB/8FBC.\n{command}\n", encoding="utf-8")
        environment["OTTD_FIRST_PLANT_INPUT"] = str(args.input.resolve())
        environment["OTTD_FIRST_PLANT_ACTION"] = args.action
    log = CONTEXT.logs / f"{args.config}-{label}.log"
    try:
        result = run_native(CONTEXT, [binary, tag], cwd=ROOT / "build", env=environment, capture_output=True, timeout=110, check=False)
    except subprocess.TimeoutExpired as error:
        # A timed-out native burst is failed evidence. Preserve its actual
        # partial diagnostics so a slow run cannot erase the last paid state.
        log.write_bytes((error.stdout or b"") + (error.stderr or b"") + b"\nNATIVE_TIMEOUT=110\n")
        raise AssertionError(log) from error
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts, log
    if args.town_command:
        assert "FIRST_PLANT_GENERATION_CONNECTION command=1" in output, "The retained world did not execute the new connection command."
    saves = {stage: dict(identity(path), cash=int(cash), geology_digest=geology)
             for stage, cash, geology, path in re.findall(
                 r"FIRST_PLANT_SAVE stage=([\w-]+) cash=(\d+) geology=([0-9A-F]+) path=([^\r\n]+)", output)}
    if args.stage in {"delivery", "utilities", "generation", "magma"}:
        family = {"utilities": "UTILITY", "delivery": "DELIVERY", "generation": "GENERATION", "magma": "MAGMA"}[args.stage]
        saves.update({stage: dict(identity(path), cash=int(cash), owners=owned(path)) for stage, cash, path in re.findall(
            rf"FIRST_PLANT_{family}_SAVE stage=([\w-]+) cash=(\d+) path=([^\r\n]+)", output)})
    input_proof = None
    if args.stage in file_modes:
        match = re.search(r"FIRST_PLANT_READ action=inspect cash=(\d+) path=([^\r\n]+)", output)
        assert match, log
        saved = Path(match[2]); before, after, initialized = verify_original(args.input, saved)
        saves["inspect"] = dict(identity(saved), cash=int(match[1]), owners=after)
        input_proof = dict(identity(args.input), owners=before, native_manager_initialized=initialized,
                           native_new_human_company=sorted(set(saved_table(saved, b"PLYR")) - set(saved_table(args.input, b"PLYR"))))
        if args.stage == "magma":
            # Upgrading adds only paid machinery, packets and domain cells.
            # Every saved stage must retain all already-earned geology and
            # the exact original natural source; the date cheat opens no floor.
            for saved in saves.values():
                for owner in ("GEOS", "GVNT", "GCBD", "GRSD"):
                    assert saved["owners"][owner] == before[owner], f"Magma stage changed retained original owner {owner}."
        if args.stage in {"delivery", "utilities"}:
            # Delivery and ordinary construction may change native stock/work,
            # never the already-paid floor or original natural source.
            for saved in saves.values():
                assert saved["owners"]["GEOS"] == before["GEOS"], "Delivery changed paid geology."
                assert saved["owners"]["GVNT"] == before["GVNT"], "Delivery changed the original source."
                if args.stage == "utilities":
                    for owner in ("SSOR", "GCBD", "GPGD", "GRSD"):
                        assert saved["owners"][owner] == before[owner], f"Physical installation changed original paid bill/history owner {owner}."
        if args.stage == "construction" and args.action == "prepare":
            prepared = re.search(r"FIRST_PLANT_BUILD action=prepare cash=(\d+) path=([^\r\n]+)", output)
            assert prepared, log
            saves["prepare"] = dict(identity(prepared[2]), cash=int(prepared[1]), owners=owned(prepared[2]))
            assert before["GVNT"] == saves["prepare"]["owners"]["GVNT"], "Funding changed the original natural source."
            assert before["GEOS"] == saves["prepare"]["owners"]["GEOS"], "Funding changed already-earned geology."
        if args.action in {"continue", "release"}:
            continued = re.search(rf"FIRST_PLANT_READ action={args.action} cash=(\d+) path=([^\r\n]+)", output)
            assert continued, log
            saves[args.action] = dict(identity(continued[2]), cash=int(continued[1]), owners=owned(continued[2]))
            assert before["GVNT"] == saves[args.action]["owners"]["GVNT"], "Actual work changed its original natural thermal source."
            assert geology_snapshot(args.input)["header"] == geology_snapshot(continued[2])["header"], "Work changed immutable geological generation/hardness metadata."
    if args.stage != "focused":
        assert saves, "A native stage must retain its actual world, not only a passing scalar assertion."
    baseline = None
    if "baseline" in saves:
        baseline = geology_snapshot(ROOT / saves["baseline"]["path"])
        cells = baseline["width"] * baseline["height"]
        assert all(layer["solid"] == cells for layer in baseline["layers"].values()), "A declared original baseline already contains excavated/claimed rock."
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md Groups8F/13B/14B/15B/15BBB/15BBC/15CB/15CC/15CDB/15D; actual paid native stages and separately scoped isolated producer boundaries",
              "configuration": args.config, "stage": args.stage, "focus": args.focus, "binary": identity(binary), "log": identity(log),
              "action": args.action, "boundary": args.boundary,
              "town_command": args.town_command,
              "case_runs": int(counts[2]), "assertions": int(counts[1]), "saves": saves,
              "native_receipts": native_receipts(output),
              "solid_baseline": baseline,
              "input": input_proof,
              "sources": [identity(ROOT / name) for name in (
                  "src/tests/geothermal_first_plant.cpp", "src/tests/geothermal_first_plant_fixture.h", "src/tests/geothermal_first_plant_load.cpp",
                  "src/tests/geothermal_first_plant_release.cpp",
                  "src/tests/geothermal_first_plant_construction.cpp",
                  "src/tests/geothermal_first_plant_delivery.cpp",
                  "src/tests/geothermal_first_plant_generation.cpp", "src/utility_power.cpp", "src/utility_power.h",
                  "src/tests/geothermal_first_plant_magma.cpp", "src/cheat_gui.cpp", "src/cheat_func.h",
                  "src/tests/geothermal_entry_points.cpp",
                  "src/tests/geothermal_generator_magma.cpp",
                  "src/tests/geothermal_controls.cpp", "src/geothermal_controls.cpp", "src/geothermal_controls.h",
                  "src/geothermal_site_cmd.cpp", "src/geothermal_site_cmd.h", "src/command.cpp", "src/command_type.h",
                  "src/geothermal_service_cmd.cpp", "src/geothermal_service_cmd.h",
                  "src/tests/geothermal_town_service.cpp", "src/tests/geothermal_town_service_fixture.h",
                  "src/tests/geothermal_town_recovery.cpp",
                  "src/geothermal_generator.h", "src/geothermal_generator.cpp", "src/underground_gui.h",
                  "src/geothermal_gui.h", "src/geothermal_gui.cpp", "src/geothermal_utility_cmd.h",
                  "src/geothermal_service_cmd.h", "src/tests/geothermal_riser_fixture.h", "src/tests/geothermal_town_service_fixture.h", "src/underground_gui.cpp", "src/widgets/underground_widget.h",
                  "src/tests/geothermal_window_sites.cpp", "src/tests/geothermal_window_utilities.cpp",
                  "src/tests/geothermal_window_export.cpp", "src/tests/geothermal_window_information.cpp",
                  "src/tests/geothermal_calendar.cpp", "src/industry_cmd.cpp", "src/tests/geothermal_collector_fixture.h",
                  "src/timer/timer_game_calendar.h", "src/timer/timer_game_economy.h",
                  "src/geothermal_site.cpp", "src/multilayer_content.cpp", "src/multilayer_content.h",
                  "src/viewport.cpp", "src/multilayer_sprite.h", "src/multilayer_sprite.cpp", "src/lang/english.txt",
                  "src/tests/underground_window.cpp", "src/tests/transport_tube_controls.cpp", "src/tests/window_hotkey_toggle.cpp",
                  "src/tests/underground_view_fixture.h", "src/tests/layer_window_fixture.h",
                  "src/network/network_command.cpp", "src/CMakeLists.txt",
                  "src/cargopacket.cpp", "src/cargopacket.h", "src/station_cmd.cpp", "src/tests/surface_supply.cpp",
                  "src/saveload/utility_power_sl.cpp", "src/saveload/saveload.h", "src/station.cpp", "src/transport_tube_runtime.cpp",
                  "src/tests/geothermal_first_plant_utilities.cpp", "src/geothermal_utility.cpp", "src/geothermal_utility.h",
                  "src/geothermal_utility_cmd.h",
                  "src/underground_vehicle.cpp", "src/underground_vehicle.h", "src/multilayer_vehicle_catalogue.cpp",
                  "src/tests/multilayer/construction_supply_file_checks.py",
                  "src/tests/multilayer/utilities/first_plant_service_checks.py",
                  "src/tests/multilayer/utilities/first_plant_town_recovery_checks.py",
                  "src/tests/multilayer/support/save_tables.py",
                  "mlm/tools/savegames/savegame_plane_digest.py",
                  "src/tests/CMakeLists.txt", "src/tests/spoil_receiver_delivery.cpp", "src/tests/tbm_face_access.cpp", "src/tests/tbm_face_controller.cpp",
                  "src/tests/tbm_face_steering.cpp", "src/tests/tbm_reserved_loop.cpp",
                  "src/tests/tbm_face_controller_loop.cpp",
                  "src/tests/tbm_face_controller_layers.cpp", "src/tests/tbm_depot_workflow.cpp",
                  "src/pathfinder/yapf/yapf_destrail.hpp", "src/pathfinder/yapf/yapf_costrail.hpp", "src/tbm_exchange.h", "src/tbm_exchange.cpp",
                  "src/pbs.cpp",
                  "src/vehicle.cpp", "src/layer_portal_work.cpp", "src/train_cmd.cpp", "src/train_cmd.h", "src/tbm_support.cpp", "src/tbm_project.cpp",
                  "src/tests/underground_workflow_fixture.h", "src/tests/underground_workflow.cpp", str(Path(__file__).resolve().relative_to(ROOT)))]}
    path = CONTEXT.output("reports", f"first-plant-{args.config}-{label}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"{args.config}-{label}: {counts[1]} assertions / {counts[2]} cases\n{path}", flush=True)


if __name__ == "__main__":
    main()
