"""Publish the independently decoded and natively loaded facility-offset correction.

@file ground_station_offset_publication.py
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 retired cache publication.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only shared-container checksums and the two corrected facility provenance links
change. Raw artwork, gameplay identities, native ranges and earlier reports remain exact.
"""
# Refuse this completed historical entry before importing any old cache readers.
# Current recipes preserve its original facts without rerunning source publication.
if __name__ == "__main__":
    raise SystemExit("Historical station-offset publication is retired; use current original-source recipes and the live source plan. Use artifact_rebuild.py ground-current-producers --configuration RelWithDebInfo for current evidence.")


import copy
import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1"
RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-station-offset-01"
BASELINE = (RUN / "before.json").relative_to(ROOT).as_posix()


def retain(source, destination):
    """Retain exactly qualified candidate bytes without replacing an existing different identity.

    @param source Current source-verified candidate file.
    @param destination Explicit repository proof destination.
    @return Exact retained reference.
    """
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists() and destination.read_bytes() != source.read_bytes():
        raise ValueError("corrected publication archive already has another identity")
    if not destination.exists():
        destination.write_bytes(source.read_bytes())
    return reference(destination)


def publish():
    """Refuse the completed historical publication and its obsolete cache graph.

    @return None; this retired entry always raises before reading or writing artifacts.
    @throws ValueError Current evidence requires the registered recipe and live source plan.
    """
    raise ValueError("Historical station-offset publication is retired; use current original-source recipes and the live source plan.")


if __name__ == "__main__":
    result = publish()
    print(json.dumps({key: result[key] for key in ("passed", "changed_native_sprites", "unchanged_native_sprites", "native_loading_verified")}))
