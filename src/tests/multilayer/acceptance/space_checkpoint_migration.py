"""Explicit original-Space to merged-format structural comparisons.

@file space_checkpoint_migration.py Paused migration, never gameplay authorization.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
The original file supplies existing values. New catalogue definitions and empty
owners are admitted by their documented schema, never a generated golden file.
"""
import json
import struct
import sys
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests"))
from merged_supply_receipts import migration_receipt
from savegame_tables import saved_owner_table as table, merged_world_chunks
from space_preview_checks import chunks


def compare_extension(source, output, family, defaults):
    """Require every old field and row, admitting only named zero-initialized fields.

    @param source Immutable original world.
    @param output Actual merged snapshot.
    @param family Serialized owner name.
    @param defaults Exact new field names and their migration defaults.
    @return None; existing quantities, references and row identities remain exact.
    """
    old, new = table(source, family), table(output, family)
    if old.keys() != new.keys():
        raise ValueError(f"migration changed existing rows: {family!r}")
    # Each existing owner retains all saved fields. New metadata must have its
    # neutral default so migration cannot introduce a paid participant or quota.
    for key, row in old.items():
        if new[key] != row | defaults:
            raise ValueError(f"migration changed original/default fields: {family!r}/{key}")


def compare_catalogue(source, output, family, allowed, field):
    """Preserve all existing mappings while appending only declared content IDs.

    @param source Immutable original world.
    @param output Actual merged snapshot.
    @param family Native mapping table, not physical vehicle or industry state.
    @param allowed Exact newly registered semantic IDs from the merged namespace.
    @param field Mapping's local ID field name.
    @return Newly allocated mapping indices, for checking unused engine history.
    """
    old, new = table(source, family), table(output, family)
    if not old.keys() <= new.keys():
        raise ValueError(f"migration removed catalogue mappings: {family!r}")
    # Saved indices remain the binding for every original vehicle/industry.
    # Compare complete old rows before considering any new definition.
    for key, row in old.items():
        if new[key] != row:
            raise ValueError(f"migration rewrote existing mapping: {family!r}/{key}")
    additions = {key: row for key, row in new.items() if key not in old}
    # Only canonical first-party definitions may be appended. The exact ID set
    # prevents arbitrary new vehicles/content from hiding behind an append rule.
    if {row[field] for row in additions.values()} != set(allowed) or len(additions) != len(allowed):
        raise ValueError(f"unexpected new catalogue definitions: {family!r}")
    for key, row in additions.items():
        # New first-party IDs occupy appended slots; they may not replace an
        # existing mapping or claim another GRF's content ownership.
        if key <= max(old) or row["grfid"] != 1230392397:
            raise ValueError(f"noncanonical catalogue append: {family!r}/{key}")
        # Capsule mappings describe ordinary Train definitions with no fallback
        # replacement; physical vehicle ownership is checked independently.
        if family == b"EIDS" and row != dict(grfid=1230392397, internal_id=row[field], type=0, substitute_id=0):
            raise ValueError("capsule engine mapping has unexpected ownership")
        if family != b"EIDS":
            substitutes = (0, 2, 5, 6, 3, 3, 0, 2, 5, 6, 3, 3, 0, 2, 5, 6)
            substitute = substitutes[(row[field] - 0x8320) % 16] if family == b"TIDS" else 0
            if row != dict(grfid=1230392397, entity_id=row[field], substitute_id=substitute):
                raise ValueError("geothermal definition changed its native substitute")
    return additions


def compare_migration(source, output):
    """Compare every original family with explicit 440-to-441 structural rules.

    @param source Exact selected original 440 continuation, verified by its seal.
    @param output Actual paused 441 successor from the dedicated native callback.
    @return Named normalizations; clocks and revision are checked by the caller.
    """
    version, old = chunks(source)
    merged_world_chunks(output)
    current = chunks(output)[1]
    added = {b"MLLN", b"TGSH", b"TGSD", b"GPGH", b"GPGD", b"UCVH", b"UCVD"}
    # The selected legacy A world has no donor-only owners. Their precise
    # introduction is format migration, with no new paid equipment or stock.
    if version != 440 or set(current) != set(old) | added or set(old) & added:
        raise ValueError("unexpected original-to-merged family closure")
    exceptions = {b"DATE", b"GLOG", b"TIDS", b"IIDS", b"EIDS", b"ENGN", b"UPLD", b"CAPA", b"RAIL", b"TUVD", b"SSOR"}
    # Everything outside individually named structural fields remains exact,
    # including VEHS, packets, companies, terrain, material and purchase history.
    for family, raw in old.items():
        if family not in exceptions and current[family] != raw:
            raise ValueError(f"migration changed original family: {family!r}")
    compare_extension(source, output, b"UPLD", dict(town_supply=0, generator=0, town_terminal=0, town_source_port=0))
    compare_extension(source, output, b"CAPA", dict(company_supply_order=0, company_supply_shipped=0))
    compare_extension(source, output, b"TUVD", {})
    # Geothermal powerhouse/Magma-Loop reserve 8320..833F in their declaration;
    # engine IDs 8009..800D are the five canonical capsule definitions. Existing
    # mappings and all physical vehicle bytes above are independently preserved.
    compare_catalogue(source, output, b"TIDS", range(0x8320, 0x8340), "entity_id")
    compare_catalogue(source, output, b"IIDS", (0x8320, 0x8330), "entity_id")
    engines = compare_catalogue(source, output, b"EIDS", range(0x8009, 0x800E), "internal_id")
    old_engines, new_engines = table(source, b"ENGN"), table(output, b"ENGN")
    if new_engines.keys() != old_engines.keys() | engines.keys():
        raise ValueError("engine history extent differs from canonical definitions")
    # Original availability, reliability, user hiding and names remain exact;
    # the five new entries cannot reset a paid vehicle's existing definition.
    for key, row in old_engines.items():
        if new_engines[key] != row:
            raise ValueError(f"existing engine history changed: {key}")
    names = ("Mole-70 Bulk Capsule", "Crate-75 Container Capsule", "Cistern-80 Fluid Capsule", "Shield-85 Mineral Capsule", "Forge-05 Heavy Capsule")
    years = (1970, 1975, 1980, 1985, 2005)
    # New definition startup is deterministic and RNG-free. Availability follows
    # the original calendar; there is no new hull, age, preview or hidden state.
    for key, mapping in engines.items():
        model = mapping["internal_id"] - 0x8009
        intro = date(years[model], 1, 1).toordinal() + 365
        available = int(table(source, b"DATE")[0]["date"] >= intro)
        expected = dict(intro_date=intro, age=0, reliability=65535, reliability_spd_dec=16,
            reliability_start=0, reliability_max=0, reliability_final=0, duration_phase_1=0,
            duration_phase_2=0, duration_phase_3=0, flags=available, preview_asked=0,
            preview_company=255, preview_wait=0, company_avail=65535 if available else 0,
            company_hidden=0, name=names[model].encode().hex())
        if new_engines[key] != expected:
            raise ValueError(f"unexpected new capsule startup fields: {key}")
    rails, restored = table(source, b"RAIL"), table(output, b"RAIL")
    if rails.keys() != restored.keys():
        raise ValueError("rail mapping extent changed")
    changes = [key for key in rails if rails[key] != restored[key]]
    # The registry occupies one formerly unused slot with UTUB's little-endian
    # label; it cannot rename any original nonempty rail type or physical track.
    if len(changes) != 1:
        raise ValueError("unexpected tube rail mapping changes")
    slot = changes[0]
    if rails[slot] != {"label": 0} or restored[slot] != {"label": int.from_bytes(b"UTUB", "little")}:
        raise ValueError("tube mapping overwrote an original rail type")
    for family in (b"TGSD", b"GPGD", b"UCVD"):
        if table(output, family):
            raise ValueError(f"migration invented donor participants: {family!r}")
    # Header defaults come from the schema, not a current output baseline.
    # Conversion shares the original utility epoch, including an empty family.
    interval = struct.unpack_from(">Q", old[b"UPLH"], 11)[0]
    headers = {b"TGSH": struct.pack(">5I", 1, 0, 0, 0, 0), b"GPGH": struct.pack(">2I", 2, 0)}
    for family, payload in headers.items():
        if current[family][3:] != payload:
            raise ValueError(f"nonempty/new owner header: {family!r}")
    conversion = current[b"UCVH"][3:]
    if len(conversion) != 32 or struct.unpack(">4Q", conversion) != (1, 0, 0, interval):
        raise ValueError("migration invented thermal conversion history")
    receipt = migration_receipt(source, output, output, "A")
    return dict(original_families=len(old), merged_families=len(current),
                structural_families=sorted(name.decode() for name in exceptions - {b"DATE", b"GLOG"}), supply=receipt)
