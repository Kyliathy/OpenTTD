"""Preserve existing pre-adaptation payload identities without retaining generated binaries.

@file preserve-sprite-expectations.py One-time preservation for r2fc-file-cleanup3.md18.
Requires the original five witnesses; refuses to replace an existing expectation.
"""

import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
target = ROOT / "src/tests/fixtures/graphics/expected-sprite-facts.json"
if target.exists():
    raise ValueError("refuse replacing independently preserved original payload facts")
catalogue = json.loads((ROOT / "src/tests/fixtures/cleanup/graphics-dependencies.json").read_text(encoding="utf-8"))
facts = {}
# Original inventory supplies actual producer/reader/path assignments. Preserve
# witness hashes before editing any producer, never derive expected hashes from
# a newly generated actual or copy entire old binary caches into fixtures.
for row in catalogue["expected_payloads"]:
    payload = (ROOT / row["old_output"]).read_bytes()
    facts[row["id"]] = dict(original_path=row["old_output"], sha256=hashlib.sha256(payload).hexdigest(),
        bytes=len(payload), magic_hex=payload[:8].hex(),
        record_count=struct.unpack_from("<H", payload, 8)[0], native_bound=struct.unpack_from("<H", payload, 10)[0],
        original_producer=dict(path=row["producer"], sha256=hashlib.sha256((ROOT / row["producer"]).read_bytes()).hexdigest()),
        original_native_reader=dict(path=row["native_reader"], sha256=hashlib.sha256((ROOT / row["native_reader"]).read_bytes()).hexdigest()))
document = dict(_documentation="Owner: r2fc-file-cleanup3.md18; exact original source-expectation payload identities captured before producer adaptation. Historical paths/producer/reader hashes are provenance only; ordinary checks never dereference them or qualify current native binaries.",
    schema_version=1, kind="original_sprite_expectation_facts", native_loading_verified=False, families=facts)
target.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
print("preserved", len(facts), "original payload identities", hashlib.sha256(target.read_bytes()).hexdigest())
