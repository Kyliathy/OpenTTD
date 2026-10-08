"""Bounded actual connected/feature-disabled native loading for ground acceptance readiness.

@file multilayer_acceptance-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-integrated-acceptance-and-balance.md Group2
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md genuine legacy ownership
"""

import hashlib
import os
from pathlib import Path
import re
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[2]


def main():
    """Load a real connected three-plane fixture and an unchanged genuine legacy world independently.

    @return None; preserve source/config/startup bytes and require actual native connection-edge evidence.
    """
    inputs = (("connected", ROOT / "src/tests/fixtures/acceptance/ground-connected.sav"),
              ("legacy370", ROOT / "src/tests/fixtures/portals/legacy-original-connected-370.sav"))
    protected = {path: hashlib.sha256(path.read_bytes()).hexdigest() for _, path in inputs}
    # Both selected loaders keep their existing configuration bytes: Native
    # uses its reviewed M1 set, while genuine Classic loading uses its owner fixture.
    for name in ("native-art.cfg", "grftest.cfg"):
        config = ROOT / "build" / name
        protected[config] = hashlib.sha256(config.read_bytes()).hexdigest()
    run_id = uuid.uuid4().hex
    startup = ROOT / "build/scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    try:
        startup.write_text("# @see r2mlvs15-integrated-acceptance-and-balance.md Group2 actual accepted-world readiness.\nlogistics_probe\n", encoding="utf-8")
        # A genuine feature-disabled input and a clearly seeded native connected
        # fixture have different evidence meanings. Neither header injection nor
        # an expected state dictionary substitutes for their actual loaders.
        for case, source in inputs:
            output = source.parent / ("acceptance-" + case + "-roundtrip-" + run_id)
            log = ROOT / "logs" / ("r2mlvs3mp-z-a-readiness-" + case + "-" + run_id + ".log")
            environment = dict(os.environ, OTTD_TBM_LOGISTICS_SOURCE=str(source),
                               OTTD_TBM_LOGISTICS_OUTPUT=str(output), OTTD_TBM_PORTAL_LOAD_ONLY="1")
            environment.pop("OTTD_M1_LOOP_ART", None)
            if case == "connected":
                environment["OTTD_M1_LOOP_ART"] = "1"
            with log.open("wb") as sink:
                subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.tbm-logistics-runtime]"],
                               cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert "All tests passed" in text, case
            assert "PORTAL_NATIVE_EDGES" in text, "actual tile-dispatch edge evidence absent"
            if case == "connected":
                assert "PORTAL_NATIVE_EDGES rail=2/2 road=0/0 elevator=0/0 invalid=0" in text, "native connected pair reconstruction failed"
            else:
                metadata = re.findall(r"MLVS_METADATA ([^\n]+)", text)
                # Classic acceptance requires both actual metadata and its
                # explicit compatibility policy; Native work rows cannot replace it.
                assert metadata and "profile=0 version=0 legacy=true" in metadata[-1], "genuine feature-disabled policy changed"
                assert "PORTAL_NATIVE_EDGES rail=5/5 road=0/0 elevator=4/4 invalid=0" in text, "actual legacy edges changed"
            assert Path(str(output) + "-loaded.sav").is_file(), "actual writer did not emit its accepted world"
            print(case + ": actual native loading and ordinary portal-edge queries passed; log=" + str(log.relative_to(ROOT)), flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        # Protect the exact supplied older bytes and config through failures,
        # while new native outputs remain inside their owning fixture families.
        for path, digest in protected.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, "protected source/config changed"


if __name__ == "__main__":
    main()
