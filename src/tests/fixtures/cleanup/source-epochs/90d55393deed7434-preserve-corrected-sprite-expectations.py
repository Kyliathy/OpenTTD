"""Select the pre-existing corrected Ground witness while retaining its superseded epoch.

@file preserve-corrected-sprite-expectations.py Preservation only for r2fc-file-cleanup3.md18.
Reads original79FHCCB witnesses, never newly generated expectation producer output.
"""

import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
target = ROOT / "src/tests/fixtures/graphics/expected-sprite-facts.json"
if hashlib.sha256(target.read_bytes()).hexdigest() != "607ecf1f120bb585603887d757c8eddbd3f191b7ae1d2783f85e5927503b511c":
    raise ValueError("corrected witness preservation requires the exact original five captured epochs")
facts = json.loads(target.read_text(encoding="utf-8"))
facts["_documentation"] = "Owner: r2fc-file-cleanup3.md18; identities of original pre-adaptation expectation witnesses. Ground explicitly selects the existing79FHCCB corrected witness and preserves the superseded79E epoch. Historical paths/producer/reader/publication hashes are provenance only; ordinary checks never dereference them or qualify current binaries. No expectation is derived from a changed producer."
publication_path = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1/publication.json"
publication = json.loads(publication_path.read_text(encoding="utf-8"))
build_reference = publication["build_proof"]
build_path = ROOT / build_reference["path"]
if hashlib.sha256(build_path.read_bytes()).hexdigest() != build_reference["sha256"]:
    raise ValueError("original corrected publication/build witness changed")
build = json.loads(build_path.read_text(encoding="utf-8"))
reference = build["fixture"]
payload = (ROOT / reference["path"]).read_bytes()
# The existing corrected publication, before cleanup, qualified exactly these
# station tuples while keeping every other native identity/byte unchanged.
if (publication["passed"] is not True or publication["native_loading_verified"] is not True
        or publication["changed_native_sprites"] != 432 or build["unchanged_native_sprites"] != 21586
        or hashlib.sha256(payload).hexdigest() != reference["sha256"]):
    raise ValueError("original corrected source expectation publication is incomplete or changed")
producer = ROOT / "src/tests/multilayer/ground_station_offset_payload_checks.py"
reader = ROOT / "src/tests/ground_station_offset_load.cpp"
facts["families"]["ground"] = dict(original_path=reference["path"], sha256=reference["sha256"],
    bytes=len(payload), magic_hex=payload[:8].hex(), record_count=struct.unpack_from("<H", payload, 8)[0],
    native_bound=struct.unpack_from("<H", payload, 10)[0],
    original_producer=dict(path=producer.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(producer.read_bytes()).hexdigest()),
    original_native_reader=dict(path=reader.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(reader.read_bytes()).hexdigest()),
    original_publication=dict(path=publication_path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(publication_path.read_bytes()).hexdigest()),
    original_build=build_reference, changed_station_records=432, unchanged_native_records=21586,
    historical_versions=[facts["families"]["ground"]])
target.write_text(json.dumps(facts, indent=2) + "\n", encoding="utf-8")
print("preserved corrected existing Ground witness and superseded epoch", hashlib.sha256(target.read_bytes()).hexdigest())
