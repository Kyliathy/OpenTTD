"""Verify native managed advice and render actual work/empty-return status.

@file r2tbmf-o-native-advice.py
@see r2tbmf-tbm-fixes.md Group6
@see ../../architecture/gui/archi-gui.md
"""

import hashlib
import os
from pathlib import Path
import subprocess
import time

from PIL import Image

ROOT = Path(__file__).resolve().parents[4]


def digest(path):
    """Identify immutable local input bytes.

    @param path Existing repository input.
    @return SHA256 digest.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """Run one retained native loop with advice enabled and retain its real status pixels.

    @return None; fail on native assertions, missing captions, advice spam or input changes.
    """
    build = ROOT / "build"
    script = build / "scripts/game_start.scr"
    startup = script.read_bytes() if script.exists() else None
    protected = [build / "play.cfg", build / "native-art.cfg",
                 ROOT / "src/tests/fixtures/haulage/tbm-feedback-haul50-393.sav"]
    hashes = {path: digest(path) for path in protected}
    env = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_TBM_FEEDBACK_")}
    env["OTTD_TBM_FEEDBACK_BALANCE"] = "haul50"
    env["OTTD_TBM_FEEDBACK_ADVICE"] = "1"
    log = ROOT / "logs/r2tbmf-o-native-advice.log"
    start = time.monotonic()
    try:
        script.write_text("# @see r2tbmf-tbm-fixes.md Group6\ntbm_feedback\n", encoding="utf-8")
        with log.open("wb") as output:
            result = subprocess.run([str(build / "RelWithDebInfo/openttd_test.exe"), "[.tbm-feedback]"],
                                    cwd=build, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=110)
        assert result.returncode == 0, log
    finally:
        # Restore the interactive launch script even if the bounded process
        # fails, and preserve every original save/configuration byte.
        if startup is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(startup)
        for path, expected in hashes.items():
            assert digest(path) == expected, path
    lines = log.read_text(encoding="utf-8").splitlines()
    assert "ADVICE_RESULT ticks=20000 too_few=0 cutter_capture=true hauler_capture=true" in lines
    reviews = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews"
    # Container conversion retains the native renderer's pixels unchanged.
    for name in ["running", "empty-return"]:
        image = Image.open(reviews / f"tbm-advice-{name}.ppm")
        image.save(reviews / f"tbm-advice-{name}.png")
    print(f"Native advice and both rendered status captions verified in {time.monotonic() - start:.2f}s")
    print("\n".join(line for line in lines if line.startswith(("ADVICE_RESULT ", "BALANCE_FINAL ", "FEEDBACK_COMPLETE "))))


if __name__ == "__main__":
    main()
