"""Shared paused ordinary-game transport for the existing Space/orbital wrappers.

@file ordinary_art_capture.py Actual native screenshot/save commands; no live work or pose grants.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19C.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group31 cold runtime setup.
"""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import uuid

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import contained
from artifact_rebuild import identity
from runner import collect_native_screenshots, console_executable, start_native
from savegame_tables import chunks, read_stream, saved_owner_table


def compare_clocks(before, after, jobs, schedule):
    """Require exact clocks while admitting only the original due-worker load pause.

    @param before Original retained DATE row with normal pause already owned.
    @param after Actual ordinary resave DATE row, including native after-load pause flags.
    @param jobs Unchanged original LGRJ table, indexed by serialized job number.
    @param schedule Unchanged original LGRS row selecting scheduled running jobs.
    @return Explicit original/observed pause bits and their narrow native owner.
    """
    # Loading may restore the existing due-worker pause through
    # AfterLoad_LinkGraphPauseControl (linkgraphschedule.cpp), bit6 in openttd.h.
    # Every clock, scheduler and RNG field still compares exactly; this cannot
    # authorize a gameplay tick, a new job, another pause flag or a changed date.
    if (set(before) != set(after) or before["pause_mode"] != 1
            or after["pause_mode"] not in (1, 65)
            or any(value != after[key] for key, value in before.items() if key != "pause_mode")):
        raise ValueError("paused ordinary capture changed DATE clocks/RNG or pause ownership")
    if after["pause_mode"] == 65:
        if not any(jobs[reference - 1]["join_date"] <= before["economy_date"] for reference in schedule["running"]):
            raise ValueError("ordinary worker pause lacks its unchanged original due linkgraph job")
    return dict(source=before["pause_mode"], output=after["pause_mode"],
                owner="normal pause; optional original due-worker AfterLoad_LinkGraphPauseControl bit6")


def compare_owners(source, output, contract):
    """Compare original exclusive owners and every clock/RNG field after a paused view.

    @param source Exact retained already-earned historical fixture save.
    @param output Complete ordinary-game resave from this capture's actual process.
    @param contract Independent original physical/metadata family scope.
    @return Measured source/output versions and unchanged owner/clock/RNG observations.
    """
    original_version, original_stream = read_stream(source)
    output_version, output_stream = read_stream(output)
    original = {row["name"]: row["raw"] for row in chunks(original_stream)}
    observed = {row["name"]: row["raw"] for row in chunks(output_stream)}
    families = [name.encode("ascii") for name in [*contract["physical_families"], *contract["metadata_families"], "SRND", "LGRJ", "LGRS", "LGRP"]]
    # No stock, body, generation, payment, construction or mission owner may
    # change simply because the actual ordinary application displayed artwork.
    for name in families:
        if name not in original or original[name] != observed.get(name):
            raise ValueError("paused ordinary capture changed original owner family: " + name.decode("ascii"))
    before = saved_owner_table(source, b"DATE")
    after = saved_owner_table(output, b"DATE")
    if set(before) != {0} or set(after) != {0}:
        raise ValueError("ordinary capture requires one original DATE owner")
    pause = compare_clocks(before[0], after[0], saved_owner_table(source, b"LGRJ"), saved_owner_table(source, b"LGRS")[0])
    return dict(source_version=original_version, output_version=output_version,
                unchanged_families=[name.decode("ascii") for name in families],
                date=after[0], pause=pause, gameplay_ticks=0)


def capture(context, configuration, application, source, producer, family, console_commands, contract):
    """Capture one actual paused ordinary Space field and its independent physical resave.

    @param context Fresh explicit configuration-specific output/runtime invocation.
    @param configuration Actual current independently built ordinary application configuration.
    @param application Current original GUI executable, not an archived historical image.
    @param source Hash-verified retained already-earned fixture world.
    @param producer Existing original Space/orbital wrapper actually calling this transport.
    @param family Explicit Space or orbital capture mechanism, used only in output naming.
    @param console_commands Existing owned-window console transport for orderly quit only.
    @param contract Independent retained fixture and original owner-family requirements.
    @return Actual process/frame/save/source identities and complete owner/clock comparison.
    """
    context.prepare_runtime()
    output = context.saves / "ordinary-paused-view.sav"
    name = family + "-paused-field"
    pending = context.runtime_dir / "screenshot" / (name + ".png")
    startup = context.runtime_dir / "scripts/game_start.scr"
    # Cold distributions may contain no scripts directory. This process owns
    # its startup command and must create that private parent before writing it.
    startup.parent.mkdir(parents=True, exist_ok=True)
    # The original ordinary capture commands are retained. A paused historical
    # world supplies every body/job/pose; no unpause, purchase or work command runs.
    startup.write_text("# @see r2fc-file-cleanup3.md19C; paused ordinary native field only.\n"
        + 'pause\nset savegame_format lzma\nspace_view\nzoomto 1\nsave "' + output.with_suffix("").as_posix()
        + '"\nscreenshot giant "' + name + '"\n', encoding="utf-8")
    executable = console_executable(context, application)
    arguments = [str(executable), "-v", "win32", "-b", "8bpp-simple", "-s", "null", "-m", "null",
                 "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)]
    terminal = context.evidence / "ordinary-native-console.txt"
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = subprocess.SW_HIDE
    started = time.monotonic()
    source_before = identity(source, ROOT)
    canonical = identity(ROOT / "media/baseset/openttd.grf", ROOT)
    runtime_payload = context.runtime_dir / "baseset/openttd.grf"
    if identity(runtime_payload, ROOT)["sha256"] != canonical["sha256"]:
        raise ValueError("ordinary capture runtime content differs from current canonical GRF")
    with terminal.open("xb") as sink:
        process = start_native(context, arguments, cwd=ROOT / "build", stdout=sink,
                               stderr=subprocess.STDOUT, startupinfo=info)
        try:
            deadline = time.monotonic() + 40
            # Native save/screenshot writes are asynchronous. Require complete
            # actual containers while the exact owned paused process is alive.
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise ValueError("ordinary capture exited before native save/frame completion")
                if output.is_file() and pending.is_file():
                    try:
                        with Image.open(pending) as image:
                            image.verify()
                        _version, stream = read_stream(output)
                        chunks(stream)
                        break
                    except (OSError, EOFError, ValueError):
                        pass
                time.sleep(.2)
            else:
                raise TimeoutError("ordinary paused capture did not complete native outputs")
            # The existing owned-window transport requests an orderly native
            # quit only after complete screenshot/resave writes. No forced stop
            # can masquerade as a successful current native process.
            console_commands(process, ["quit"])
            process.wait(timeout=10)
        finally:
            # On failure stop only this owned child; never address another game.
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=5)
    if process.returncode != 0:
        raise ValueError("ordinary native capture did not exit successfully: " + str(terminal))
    captures = collect_native_screenshots(context)
    if len(captures) != 1:
        raise ValueError("ordinary paused capture requires one actual native screenshot")
    frame = contained(captures[0], context.images)
    owners = compare_owners(source, output, contract)
    # A view cannot alter its retained original or publish new content. Keep
    # runtime/config/startup and projection identities with the actual process.
    if identity(source, ROOT) != source_before or identity(ROOT / "media/baseset/openttd.grf", ROOT) != canonical:
        raise ValueError("ordinary capture changed its retained source or canonical content")
    with Image.open(frame) as raster:
        if raster.mode != "P":
            raise ValueError("ordinary source-pixel review requires actual indexed native pixels")
        frame_record = dict(identity(frame, ROOT), feature=context.feature, scenario=context.scenario,
                            run_id=context.run_id, dimensions=list(raster.size),
                            indexed_sha256=hashlib.sha256(raster.tobytes()).hexdigest(),
                            rgb_sha256=hashlib.sha256(raster.convert("RGB").tobytes()).hexdigest())
    native = context.write_report("native-" + uuid.uuid4().hex + ".json", producer,
        [output, frame, terminal], [source], configuration, native_writer="ordinary native screenshot/save commands")
    record = json.loads(native.read_text(encoding="utf-8"))
    entrypoint = Path(sys.argv[0]).resolve()
    record.update(argv=arguments, pid=process.pid, exit_status=process.returncode,
        executable=identity(executable, ROOT), original_application=identity(application, ROOT),
        entrypoint=entrypoint.relative_to(ROOT).as_posix() if entrypoint.is_relative_to(ROOT) else "external-caller",
        runtime=context.runtime_dir.relative_to(ROOT).as_posix(), transcripts=[identity(terminal, ROOT)],
        startup=identity(startup, ROOT), termination="orderly native quit after complete paused save/screenshot")
    native.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return dict(native_process=identity(native, ROOT), frames=[frame_record], after=identity(output, ROOT),
                native_terminal=identity(terminal, ROOT), owner_comparison=owners,
                elapsed_seconds=round(time.monotonic() - started, 3), runtime_config=identity(context.config_path, ROOT),
                runtime_payload=identity(runtime_payload, ROOT), startup=identity(startup, ROOT),
                projected_application=identity(executable, ROOT))
