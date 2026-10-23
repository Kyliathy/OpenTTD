"""Prove the face exchange's saves with real files written and read by the ordinary saver and loader.

Face-loop runs of the native demo observer (demo_feedback.py --scenario face-loop) save checkpoints through the
ordinary saver. The checkpoints are:
- the hauler travelling to its stand, held there, the cutter mid-dwell and the loaded hauler leaving through the pair;
- a Deep cutter returning to its bay;
- the results of the player's stop, park and release.
Each writer logs every checkpoint's owners and its next transition. The native reader (src/tests/tbm_face_exchange_save.cpp,
[.demo-fix-face-save-file]) loads each file through the ordinary loader. It must log the same owners, then reach the
same transition at the same offset with the same owners. Files of the R2DF build (version 424) follow their next
exchanges under the bay and face rules, and a version-424 copy of the face wait is refused.

@file face_save.py
@see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 9
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

# Repository markers identify the checkout independently of the caller's working directory.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native  # noqa: E402

DEMO = ROOT / "src/tests/multilayer/tbm/demo_feedback.py"
DEEP = "src/tests/fixtures/haulage/tbm-fresh-deep-tunnel-393.sav"

#: Face-loop writers: name and launcher arguments. Lifecycle runs end once their checkpoint's transition is logged.
WRITERS = (
    ("loop", ["--ticks", "60000", "--saves", "travelling,held,dwell,leaving"]),
    ("deep", ["--input", DEEP, "--bend", "--ticks", "80000", "--saves", "bay-return"]),
    ("stop", ["--ticks", "30000", "--lifecycle", "stop", "--face-transfers", "0"]),
    ("park", ["--ticks", "30000", "--lifecycle", "park", "--face-transfers", "0"]),
    ("release", ["--ticks", "30000", "--lifecycle", "release", "--face-transfers", "0"]),
)

#: Files written by the R2DF build (version 424) and the meeting point of each exchange that must follow them.
LEGACY = (
    ("mlm/reviews/demo-fixes/hauler-depot/20261015-220425-298fa44a13/saves/stored-consist.sav", ["bay", "face"]),
    ("mlm/reviews/demo-fixes/hauler-depot/20261015-220425-298fa44a13/saves/hauler-departing.sav", ["face"]),
)

#: Version before the face wait existed; a copy patched to it stands for a file written before version 425.
LEGACY_VERSION = 424


def digest(path):
    """Hash a file without changing it.

    @param path Existing file.
    @return SHA256 hexadecimal digest of its exact bytes.
    """
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rows(text, tag):
    """Collect the rows of one tag.

    @param text Native log text.
    @param tag Row tag, such as DEMO_FACE_NEXT.
    @return Dictionary from each row's name field to the rest of the row after it.
    """
    found = {}
    for line in text.splitlines():
        if line.startswith(tag + " name="):
            name, _, rest = line[len(tag) + 6:].partition(" ")
            found[name] = rest
    return found


def owners(row):
    """Cut a row down to its owner fields, which start at the tick clock.

    @param row Row text after its name.
    @return The owner words.
    """
    return row[row.index("counter="):]


def patch_version(source, target, version):
    """Copy a save with another version in its header; the chunks stay byte for byte.

    @param source Save written by the current build.
    @param target New file.
    @param version Version written into the header.
    @return The source's own version.
    """
    data = bytearray(Path(source).read_bytes())
    assert data[:3] == b"OTT", f"not an OpenTTD save: {source}"
    # The header holds the format tag, then the version as a big-endian 16-bit number.
    old = int.from_bytes(data[4:6], "big")
    data[4:6] = version.to_bytes(2, "big")
    Path(target).write_bytes(bytes(data))
    return old


def write_checkpoints(name, arguments):
    """Run one face-loop writer.

    @param name Writer name, for the report.
    @param arguments Launcher arguments after the scenario.
    @return Dictionary with the log path, the saved files and the owner and next-transition rows by checkpoint.
    """
    command = [sys.executable, str(DEMO), "--scenario", "face-loop", "--timeout", "580", *arguments]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    out = result.stdout + result.stderr
    match = re.search(r'"log": "([^"]+)"', out)
    assert result.returncode == 0 and match, f"writer {name} failed:\n{out[-3000:]}"
    log = ROOT / match.group(1).replace("\\\\", "/")
    text = log.read_text(encoding="utf-8", errors="replace")
    saves = {key: row.split("path=")[1] for key, row in rows(text, "DEMO_SAVE").items()}
    return {"log": str(log.relative_to(ROOT)), "saves": saves, "checkpoints": rows(text, "DEMO_FACE_CHECKPOINT"), "next": rows(text, "DEMO_FACE_NEXT")}


class Reader:
    """Native reader runs inside one invocation, with the startup script that starts the reader."""

    def __init__(self):
        """Prepare the invocation's runtime home and locate the test executable."""
        self.context = current_run_context("demo-fixes", "face-save")
        self.context.prepare_runtime()
        self.build = ROOT / "build"
        self.executable = self.build / "RelWithDebInfo" / "openttd_test.exe"
        self.count = 0

    def run(self, file, mode, extra=None):
        """Load one file natively in one reader mode.

        @param file Save to load.
        @param mode Reader mode: next, exchanges or refuse.
        @param extra Further environment for the reader.
        @return Process exit code and the log text.
        """
        self.count += 1
        script = self.context.runtime_dir / "scripts/game_start.scr"
        original = script.read_bytes() if script.exists() else None
        log = self.context.logs / f"reader-{self.count:02d}-{mode}-{Path(file).stem}.log"
        environment = dict(os.environ)
        environment.update({"OTTD_FACE_SAVE_INPUT": str(file), "OTTD_FACE_SAVE_MODE": mode, **(extra or {})})
        before = digest(file)
        try:
            script.parent.mkdir(parents=True, exist_ok=True)
            script.write_text("# @see r2df2-face-exchange-and-demo-acceptance.md Group 9 face-loop file reader.\nface_save_check\n", encoding="utf-8")
            with log.open("wb") as output:
                result = run_native(self.context, [str(self.executable), "[.demo-fix-face-save-file]"], cwd=self.build, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=300, env=environment)
        finally:
            # The next interactive launch must never run the test-only reader.
            if original is None:
                script.unlink(missing_ok=True)
            else:
                script.write_bytes(original)
        assert digest(file) == before, f"the reader changed its input: {file}"
        return result.returncode, log.read_text(encoding="utf-8", errors="replace")


def main():
    """Write the checkpoints, reload each one, follow the legacy files and refuse the malformed copy.

    @return None; exits non-zero when any comparison or native check fails.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--writers", default=",".join(name for name, _ in WRITERS), help="comma-separated writers to run")
    args = parser.parse_args()
    selected = set(args.writers.split(","))
    reader = Reader()
    report = {"writers": {}, "reloads": [], "legacy": [], "refusal": None}
    failures = []
    files = {}
    for name, arguments in WRITERS:
        if name not in selected:
            continue
        written = write_checkpoints(name, arguments)
        report["writers"][name] = written
        for checkpoint, path in written["saves"].items():
            files[checkpoint] = (Path(path), written)
    # Every checkpoint reloads with the owners it was saved with and reaches the same next transition.
    for checkpoint, (path, written) in files.items():
        code, text = reader.run(path, "next")
        loaded = rows(text, "FACE_SAVE_LOADED").get(checkpoint, "")
        following = rows(text, "FACE_SAVE_NEXT").get(checkpoint, "")
        entry = {"checkpoint": checkpoint, "file": str(path), "exit": code, "saved_owners": owners(written["checkpoints"][checkpoint]),
                 "loaded": loaded, "writer_next": written["next"].get(checkpoint), "reader_next": following}
        entry["same_owners"] = bool(loaded) and owners(loaded) == entry["saved_owners"]
        entry["same_next"] = bool(following) and following == entry["writer_next"]
        report["reloads"].append(entry)
        if code != 0 or not entry["same_owners"] or not entry["same_next"]:
            failures.append(f"reload {checkpoint}")
        # A Deep return to the bay, in a copy carrying the version before the face wait, reaches the same transition.
        if checkpoint == "face-bay-return":
            copy = reader.context.saves / f"{checkpoint}-{LEGACY_VERSION}.sav"
            copy.parent.mkdir(parents=True, exist_ok=True)
            source_version = patch_version(path, copy, LEGACY_VERSION)
            code, text = reader.run(copy, "next")
            following = rows(text, "FACE_SAVE_NEXT").get(copy.stem, "")
            same = bool(following) and following == entry["writer_next"]
            report["legacy"].append({"file": str(copy.relative_to(ROOT)), "source_version": source_version, "exit": code, "reader_next": following, "same_next": same})
            if code != 0 or not same:
                failures.append(f"legacy {copy.name}")
    # Files of the R2DF build finish their bay exchange under the bay rules; later breakpoints meet at the face.
    for path, meetings in LEGACY:
        code, text = reader.run(ROOT / path, "exchanges", {"OTTD_FACE_SAVE_EXCHANGES": str(len(meetings))})
        found = [row.split("meeting=")[1].split(" ")[0] for row in text.splitlines() if row.startswith("FACE_SAVE_EXCHANGE ")]
        loaded = rows(text, "FACE_SAVE_LOADED").get(Path(path).stem, "")
        report["legacy"].append({"file": path, "exit": code, "loaded": loaded, "meetings": found, "expected": meetings,
                                 "rows": [row for row in text.splitlines() if row.startswith("FACE_SAVE_EXCHANGE ")]})
        if code != 0 or found != meetings:
            failures.append(f"legacy {path}")
    # A pre-425 file that holds the face wait is refused; the same version with a phase it knew loads.
    if "face-dwell" in files and "face-held" in files:
        corrupt = reader.context.saves / f"face-dwell-{LEGACY_VERSION}.sav"
        control = reader.context.saves / f"face-held-{LEGACY_VERSION}.sav"
        corrupt.parent.mkdir(parents=True, exist_ok=True)
        patch_version(files["face-dwell"][0], corrupt, LEGACY_VERSION)
        patch_version(files["face-held"][0], control, LEGACY_VERSION)
        code, text = reader.run(files["face-dwell"][0], "refuse", {"OTTD_FACE_SAVE_CORRUPT": str(corrupt), "OTTD_FACE_SAVE_CONTROL": str(control)})
        refused = rows(text, "FACE_SAVE_REFUSED").get("face-dwell")
        report["refusal"] = {"corrupt": str(corrupt.relative_to(ROOT)), "control": str(control.relative_to(ROOT)), "exit": code, "row": refused}
        if code != 0 or refused is None:
            failures.append("refusal")
    output = reader.context.reports / "face-save-report.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    report["failures"] = failures
    output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps({"report": str(output.relative_to(ROOT)), "reloads": len(report["reloads"]), "legacy": len(report["legacy"]), "failures": failures}, indent=2))
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
