"""Actual393 native unused-right loss, retained use, fresh paid repair and legacy lost-path settlement.

@file r2mlvs3mp-z-q-heat-topology-file-checks.py
@see r2mlvs17-geothermal-energy-and-industry.md Group4.
@see r2mlvs3mp-master-plan.md checkpoint10.3; operating plant output retains10.6.
"""

import hashlib
import importlib.util
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
DIRECTORY = ROOT / "src/tests/fixtures/geothermal"
sys.dont_write_bytecode = True


def main():
    """Verify real paid native break/repair/replan, byte-exact settled reload and ordinary resave.

    @return None; retain all predecessor bytes and restore the exact repository startup script.
    """
    owner = Path(__file__).with_name("r2mlvs3mp-z-p-heat-allocation-file-checks.py")
    spec = importlib.util.spec_from_file_location("native_thermal_file_owner", owner)
    producer = importlib.util.module_from_spec(spec); spec.loader.exec_module(producer)
    producer.PREFIX = "r2mlvs3mp-z-q-heat-topology"
    grant = DIRECTORY / "fixture393-heat-allocation-fixture-grant.sav"
    used = DIRECTORY / "fixture393-heat-allocation-fixture-use.sav"
    originals = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in (grant, used)}
    startup = ROOT / "build/scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group4 actual unused-right topology boundary.\nheat_allocation_check\n", encoding="utf-8")
        # Each independent native process uses its original accepted source,
        # route and stock owners; paid removal can expire only free slices,
        # while actual consumed history and physical references stay original.
        for label, source in (("unused", grant), ("used", used)):
            producer.run_case(f"topology-break-{label}", source)
            split = DIRECTORY / f"fixture393-heat-topology-break-{label}.sav"
            assert producer.authority(split) == producer.authority(source)
            producer.run_case(f"topology-split-{label}-read", split)
            assert producer.authority(DIRECTORY / f"fixture393-heat-topology-split-{label}-read.sav", True) == producer.authority(split, True)
        split = DIRECTORY / "fixture393-heat-topology-break-used.sav"
        producer.run_case("topology-repair", split)
        repaired = DIRECTORY / "fixture393-heat-topology-repair.sav"
        producer.run_case("topology-repaired-read", repaired)
        assert producer.authority(DIRECTORY / "fixture393-heat-topology-repaired-read.sav", True) == producer.authority(repaired, True)
        producer.run_case("topology-replan", repaired)
        replanned = DIRECTORY / "fixture393-heat-topology-replan.sav"
        producer.run_case("topology-replanned-read", replanned)
        assert producer.authority(DIRECTORY / "fixture393-heat-topology-replanned-read.sav", True) == producer.authority(replanned, True)
        # Explicit compatibility transport reproduces the previously valid393
        # lost-cell promise. Actual load must settle only that unused quantity
        # once; the following reader proves idempotence and exact saved use.
        producer.run_case("topology-legacy-stale", used)
        stale = DIRECTORY / "fixture393-heat-topology-legacy-stale.sav"
        producer.run_case("topology-legacy-settled-read", stale)
        settled = DIRECTORY / "fixture393-heat-topology-legacy-settled-read.sav"
        producer.run_case("topology-legacy-idempotent-read", settled)
        assert producer.authority(DIRECTORY / "fixture393-heat-topology-legacy-idempotent-read.sav", True) == producer.authority(settled, True)
        # Independent preview must accept exact consumed history over retired
        # original cells while refusing missing/wrong ordered references. Keep
        # its intentionally malformed copies separate from Group3 fixtures.
        producer.DIRECTORY = DIRECTORY / "topology"
        producer.DIRECTORY.mkdir(exist_ok=True)
        for corrupt in producer.corrupt_routes(split):
            producer.run_case("preview", split, corrupt)
        output = DIRECTORY / "fixture393-heat-topology-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group4 ordinary repaired allocation reload.\nset savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        runner = ROOT / "docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh"
        log = ROOT / "logs/r2mlvs3mp-z-q-heat-topology-normal-loader.log"
        subprocess.run(["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", "native-art.cfg", "-g", replanned.as_posix()],
                       cwd=ROOT / "build", env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
        assert producer.authority(output, True) == producer.authority(replanned, True)
        print("Actual393 heat topology: paid unused/part-used removal, exact settled reload, fresh paid repair without old-right revival, next-interval replan/use, legacy lost-path settlement/idempotence and ordinary resave pass. Original native source/stock/path/input/startup preserved.", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        for path, original in originals.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == original, f"retained source changed: {path.name}"


if __name__ == "__main__":
    main()
