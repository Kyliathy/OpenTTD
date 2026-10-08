"""Focused native migration of retained publication, settlement and physical transfer formats.

@file utility_power_save_fixture-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 9
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md older-save policy
"""

import hashlib
import importlib.util
import re
import os
import subprocess

from utility_power_save_fixture import ROOT, DIRECTORY, run_case
from geology_generation_fixture import read_stream


def main():
    """Load one actual retained world per older utility format through the current native reader.

    @return None; preserve all original inputs/startup bytes and prove no invented consumption, effect or route.
    """
    startup = ROOT / "build/scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    inputs = {381: DIRECTORY / "utility-v381-granted.sav",
              382: DIRECTORY / "utility-v382-released.sav",
              383: DIRECTORY / "utility-v383-transfer-rebuilt.sav"}
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs.values()}
    effects = DIRECTORY / "utility-town-applied.sav"
    assert read_stream(effects)[0] == 384, "retain actual original native effect-format bytes"
    hashes[effects] = hashlib.sha256(effects.read_bytes()).hexdigest()
    issuer = (ROOT / "logs/r2mlvs3mp-v-power-file-settle.log").read_text(encoding="utf-8", errors="replace")
    ticket = re.search(r"UTILITY_TICKET sequence=(\d+) seal=(\d+)", issuer)
    assert ticket is not None, "original settlement issuer evidence is required"
    specification = importlib.util.spec_from_file_location("utility_transfer_fixture", ROOT / "src/tests/utility_power_save_fixture-2.py")
    transfer = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(transfer)
    try:
        # Each supported older format supplies its own real retained bytes;
        # changing a current version header cannot establish migration behavior.
        for version, path in inputs.items():
            assert read_stream(path)[0] == version, "retained format identity changed"
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md Group9 retained-format acceptance.\nutility_file_check\n", encoding="utf-8")
        run_case("granted", inputs[381], "legacy381-granted", prefix="r2mlvs3mp-w")
        run_case("released", inputs[382], "legacy382-released", ticket.groups(), prefix="r2mlvs3mp-w")
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md Group9 retained-transfer acceptance.\nutility_transfer_check\n", encoding="utf-8")
        transfer.run_case("rebuilt", inputs[383], prefix="r2mlvs3mp-w-legacy383")
        # Version384 owns a real settled town effect, not a modified current
        # header. Its actual original baseline remains the town authority.
        original_effect_log = (ROOT / "logs/r2mlvs3mp-w-power-town-create.log").read_text(encoding="utf-8", errors="replace")
        baseline = re.search(r"UTILITY_TOWN_BASELINE rate=(\d+)", original_effect_log)
        assert baseline is not None, "actual original town baseline is required"
        startup.write_text("# @see r2mlvs14-savegame-determinism-and-lifecycle.md15B retained384 native effects.\nutility_effect_check\n", encoding="utf-8")
        environment = dict(os.environ, OTTD_UTILITY_FILE_CASE="applied", OTTD_UTILITY_FILE_INPUT=str(effects),
                           OTTD_UTILITY_TOWN_BASELINE=baseline[1])
        log = ROOT / "logs/r2mlvs3mp-z-a-legacy384-effects.log"
        with log.open("wb") as sink:
            subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.utility-power-effect-file]"],
                           cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                           creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
        assert "All tests passed" in log.read_text(encoding="utf-8", errors="replace")
        print("Current reader accepted retained381/382/383/384 grants, settlement retry, rebuilt portal flow and native town effects", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        # Migration reads cannot replace historical source files with new output.
        for path, expected in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, "retained utility input changed"


if __name__ == "__main__":
    main()
