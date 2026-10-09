#!/usr/bin/env python3
"""Actual game-loader native freight round trip with immutable input and owned evidence.

@file underground_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 10B
"""

import hashlib
import os
from pathlib import Path
import re
import subprocess

from geology_save_fixture import chunks
from geology_generation_fixture import read_stream
from multilayer_state_fixture import log_path

ROOT = Path(__file__).resolve().parents[2]
AREA = ROOT / "docs/zz-reach2/upgrades/2026-10"
FIXTURES = ROOT / "src/tests/fixtures/underground"


# Each producer declares its feature; saved-input names never select output ownership.
from functools import partial
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import output_save_path
CONTEXT = current_run_context("vehicles", "underground-save-fixture")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime("grftest.cfg")
output_save_path = partial(output_save_path, context=CONTEXT)
log_path = partial(log_path, context=CONTEXT)


def transport(path):
    """Read the real writer's engine, vehicle, cargo, order and controller descriptors.

    @param path Immutable command-generated source or the actual game-loader resave.
    @return Version and exact owned chunk bytes; no gameplay objects or IDs are invented.
    """
    version, stream = read_stream(path)
    required = {b"EIDS", b"VEHS", b"ORDL", b"TBMH", b"TBMP", b"TBMS",
                b"TBXH", b"TBXA", b"TBXS", b"TBXW"}
    records = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in required}
    assert set(records) == required, "actual native fleet descriptor family incomplete"
    return version, records


def main():
    """Reload and resave a bought/loaded native fleet through the actual game, preserving original input and startup bytes.

    @return None; any missing native definition, association, cargo/order change or loader refusal fails.
    """
    source = FIXTURES / "native-fleet.sav"
    output = CONTEXT.saves / "native-fleet-roundtrip.sav"
    log = log_path("r2mlvs7-cn-fleet-runtime-roundtrip.log")
    startup = RUNTIME / "scripts/game_start.scr"
    original_startup = startup.read_bytes()
    original_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    version, before = transport(source)
    assert version >= 380, "native fleet requires the delivered save families"
    assert not output.exists(), "retain any previous owned round-trip evidence before rerunning this fixture"
    try:
        startup.write_text("# @see r2mlvs7-underground-construction-and-fleet.md, actual native fleet reload.\n"
                           'dump_info underground_fleet\n'
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        environment = dict(os.environ, OTTD_TIMEOUT="60", OTTD_DEBUG="misc=5,net=2,sl=3,grf=2,desync=5,script=2")
        environment.pop("OTTD_MULTILAYER_LOADER_PROBE", None)
        # The existing runner creates a hidden-console-compatible repository binary and preserves real startup/loader behavior.
        run_native(CONTEXT, ["C:/Program Files/Git/bin/bash.exe", (ROOT / "mlm/tools/run/run_dedicated.sh").as_posix(),
                        (ROOT / "build").as_posix(), "Debug", log.as_posix(), "-X", "-c", "grftest.cfg",
                        "-g", source.as_posix()], env=environment, check=True, timeout=65)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        assert "MLVS_METADATA schema=1 profile=1" in evidence, "real native profile activation missing"
        assert "MLVS_FREIGHT_FILTER underground=7 deep=7" in evidence, "loaded calendar/profile filters changed"
        # Every logical native conventional design must resolve to an actual reconstructed definition in the real loaded engine owner.
        for model in range(2, 9):
            assert re.search(rf"MLVS_FREIGHT_MODEL model={model} engine=\d+ type=\d+ capacity=\d+", evidence), (model, "actual engine missing")
        assert re.search(r"MLVS_FREIGHT_VEHICLE id=\d+ model=7 engine=\d+ layer=1 cargo=\d+ packets=[1-9]\d* orders=2", evidence), "loaded mine truck cargo/station/service orders missing"
        assert re.search(r"MLVS_FREIGHT_VEHICLE id=\d+ model=4 engine=\d+ layer=1 cargo=\d+ packets=0 orders=2 project=0", evidence), "actual native hauler association missing"
        after_version, after = transport(output)
        assert after_version == version, "save version changed"
        # Full exact descriptor comparisons cover EIDS identity, vehicle cargo/order references and controller-owned associations together.
        for name in before:
            assert after[name] == before[name], (name.decode(), "owned native fleet transport changed")
        print("Native fleet: actual loader preserved engine identities, cargo, station/service orders, filters and active association", flush=True)
    finally:
        startup.write_bytes(original_startup)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original_hash, "immutable native fleet source changed"


if __name__ == "__main__":
    main()
