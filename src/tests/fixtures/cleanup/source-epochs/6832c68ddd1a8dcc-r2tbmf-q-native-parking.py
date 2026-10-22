"""Drive resumable native parking and compare three actual saved continuations.

@see r2tbmf-tbm-fixes.md Group7
@see ../../architecture/savegame/archi-savegame.md
"""

import hashlib
import os
from pathlib import Path
import re
import subprocess
import time

from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
KINDS = ("return", "enter", "parked")


def digest(path):
    """@param path Retained file. @return SHA256 of its exact existing bytes."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """Run bounded native journeys, restore startup bytes and require exact frame replay.

    @return None; raise on failed native behavior, replay mismatch or changed inputs.
    """
    build = ROOT / "build"
    fixtures = ROOT / "src/tests/fixtures/haulage"
    protected = [fixtures / "underground-demo-390.sav", fixtures / "tbm-feedback-haul50-393.sav",
                 build / "play.cfg", build / "native-art.cfg"]
    before = {path: digest(path) for path in protected}
    script = build / "scripts/game_start.scr"
    original = script.read_bytes() if script.exists() else None
    env = {key: value for key, value in os.environ.items()
           if not key.startswith(("OTTD_TBM_MOTION_", "OTTD_TBM_PACE_", "OTTD_TBM_PARKING"))}
    env["OTTD_TBM_PARKING"] = "1"
    reference = None
    try:
        script.write_text("# @see r2tbmf-tbm-fixes.md Group7 native parking.\ntbm_motion\n", encoding="utf-8")
        for kind in (None, *KINDS):
            # Each continuation loads an actual complete-frame snapshot. No
            # runner edits saved phase, work, positions, packets or reservations.
            log = ROOT / f"logs/r2tbmf-q-parking-{kind or 'native'}.log"
            if kind is not None:
                snapshot = next(line for line in reference if line.startswith(f"PARK_SNAPSHOT kind={kind} "))
                tick = int(re.search(r"tick=(\d+)", snapshot).group(1))
                env["OTTD_TBM_MOTION_REPLAY"] = kind
                env["OTTD_TBM_MOTION_TICK"] = str(tick)
                retained = fixtures / f"tbm-parking-{kind}-396.sav"
                before[retained] = digest(retained)
            started = time.monotonic()
            with log.open("wb") as output:
                result = subprocess.run([str(build / "RelWithDebInfo/openttd_test.exe"), "[.tbm-motion-reload]"],
                                        cwd=build, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=110)
            print(f"Parking {kind or 'native'}: exit={result.returncode}, seconds={time.monotonic() - started:.2f}, log={log.relative_to(ROOT)}", flush=True)
            result.check_returncode()
            actual = log.read_text(encoding="utf-8").splitlines()
            if kind is None:
                reference = actual
                assert sum(line.startswith("PARK_SNAPSHOT ") for line in actual) == 3
                reviews = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews"
                for name in ("request", "parked"):
                    ppm = reviews / f"tbm-parking-{name}.ppm"
                    with Image.open(ppm) as image:
                        assert image.getbbox() is not None
                        image.save(ppm.with_suffix(".png"))
            else:
                expected = [line for line in reference if line.startswith("MOTION_TRACE ") and int(re.search(r"tick=(\d+)", line).group(1)) >= tick]
                observed = [line for line in actual if line.startswith("MOTION_TRACE ")]
                assert observed == expected, f"Physical continuation mismatch: {kind}"
                assert [line for line in actual if line.startswith("PARK_RESULT ")] == [line for line in reference if line.startswith("PARK_RESULT ")]
                print(f"Exact complete native frames: {kind}={len(observed)}", flush=True)
    finally:
        # Startup/config and supplied saves must survive every assertion or
        # process timeout so the user's next ordinary launch is unaffected.
        if original is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(original)
        for path, expected in before.items():
            assert digest(path) == expected, path


if __name__ == "__main__":
    main()
