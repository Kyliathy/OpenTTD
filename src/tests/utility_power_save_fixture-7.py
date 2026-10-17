"""Focused real company removal/ID reuse, retired-account preview and native repeated loading.

@file utility_power_save_fixture-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group15B
@see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-utility-lifecycle-policy.md company rows
"""

import hashlib
import importlib.util
import lzma
import uuid

from utility_power_save_fixture import ROOT, DIRECTORY, run_case, utility_chunks
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-7")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)


def main():
    """Remove an actual loaded company, reuse its ID and verify retained original settlement after two real loads.

    @return None; exact source/config/startup bytes survive every failure and tombstones never provide grants.
    """
    source = resolve_input_path(INPUTS / "utility-granted.sav")
    assert read_stream(source)[0] == 384, "migration starts from the retained genuine effect-format save"
    protected = {path: hashlib.sha256(path.read_bytes()).hexdigest()
                 for path in (source, ROOT / "build/native-art.cfg")}
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    prefix = "r2mlvs3mp-z-a-company-" + uuid.uuid4().hex
    try:
        startup.write_text("# @see r2mlvs15-savegame-determinism-and-lifecycle.md15B actual company lifecycle.\nutility_file_check\n", encoding="utf-8")
        run_case("company-remove", source, "remove", prefix=prefix)
        retired = DIRECTORY / "utility-company-retired.sav"
        version, stream = read_stream(retired)
        assert version == 385, "retirement must use the explicit current wire version"
        records = {row["name"]: row for row in chunks(stream)}
        assert int.from_bytes(records[b"UPLH"]["payload"][:8], "big") == 5, "wrong lifecycle format"
        consumers = [row for row in records[b"UPLD"]["rows"][1:] if row[0] == 1]
        # Exact saved liveness distinguishes old retained accounts from the
        # actual replacement company even though their numeric owner agrees.
        assert len(consumers) == 2 and all(len(row) == 104 and row[12] == 0 for row in consumers)
        expected = utility_chunks(retired)
        # Corrupt one field at a time in actual current transport, preserving
        # every unrelated map/company/cargo chunk. The native preview below
        # must reject both illegal liveness and renewed retired free energy.
        for fault in ("active", "balance"):
            candidate = chunks(stream)
            table = next(row for row in candidate if row["name"] == b"UPLD")
            kind = 1 if fault == "active" else 3
            index = next(index for index, row in enumerate(table["rows"][1:], 1) if row[0] == kind)
            wire = bytearray(table["rows"][index])
            if fault == "active":
                wire[12] = 2
            else:
                wire[37:45] = bytes(8)
            table["rows"][index] = bytes(wire)
            table["edited"] = True
            encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in candidate) + bytes(4)
            (DIRECTORY / ("utility-company-corrupt-" + fault + ".sav")).write_bytes(retired.read_bytes()[:8] + lzma.compress(encoded))
        run_case("company-retired", retired, "reload-1", prefix=prefix)
        restored = DIRECTORY / "utility-company-retired-reloaded.sav"
        assert utility_chunks(restored) == expected, "first load changed retired original balances"
        run_case("company-retired", restored, "reload-2", prefix=prefix)
        assert utility_chunks(restored) == expected, "second load changed retained settlement"
        # A retained genuinely connected version383 world exercises takeover
        # of actual portal eligibility, independently of the local retirement case.
        transfer_source = resolve_input_path(INPUTS / "utility-v383-transfer-rebuilt.sav")
        protected[transfer_source] = hashlib.sha256(transfer_source.read_bytes()).hexdigest()
        specification = importlib.util.spec_from_file_location("utility_company_transfer", ROOT / "src/tests/utility_power_save_fixture-2.py")
        transfer = importlib.util.module_from_spec(specification)
        specification.loader.exec_module(transfer)
        transfer.CONTEXT = CONTEXT
        startup.write_text("# @see r2mlvs15-savegame-determinism-and-lifecycle.md15B real physical-route takeover.\nutility_transfer_check\n", encoding="utf-8")
        transfer.run_case("company-takeover", transfer_source, context=CONTEXT, prefix=prefix)
        print("Company lifecycle: actual removal/reused company ID, format385/5 retirement, valid preview and two exact native reloads", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        # Historical sources/configs remain immutable regardless of native
        # cleanup or loader failure; new outputs stay in the utility repository family.
        for path, expected in protected.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, "protected utility source/config changed"


if __name__ == "__main__":
    main()
