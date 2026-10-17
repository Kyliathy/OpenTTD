"""Read exact435/436 contained-service framing without native pools or gameplay mutation.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC
Bounded native fields select original job/model/packet offsets. Legacy normalization removes only explicitly appended model bytes and preserves every original paid field.
"""
from launch_center_checks import riff_location


def structure_rows(raw, version):
    """Walk complete current structure rows, including original stock and optional manufacture/clone/replacement owners.

    @param raw Exact decompressed native stream.
    @param version Admitted original435 or current436 schema.
    @return Exact first/second job, immutable role and packet-field offsets for each original site.
    """
    assert version in (435, 436); start, body, length, end = riff_location(raw, b"SPST"); position = body

    def read(width):
        """Admit one field entirely inside the original native family.

        @param width Exact nonnegative native width.
        @return Original big-endian value.
        """
        nonlocal position
        assert 0 <= width <= end - position; value = int.from_bytes(raw[position:position + width], "big"); position += width; return value

    def packets(maximum=64):
        """Retain actual original quantity/count/reference offsets for one bounded container.

        @param maximum Exact containing schema's reference cap.
        @return Original physical-container field locations and packet references.
        """
        quantity_offset = position; quantity = read(2); count_offset = position; count = read(2); assert count <= maximum; offsets = []
        # Every next-hop pair has its own real native extent; reference count is not a substitute for physical CAPA quantity.
        for _ in range(count):
            offsets.append(position); read(6)
        return dict(quantity=quantity, quantity_offset=quantity_offset, count=count, count_offset=count_offset, packet_offsets=offsets)

    def service():
        """Read one original contained job and only its source-era immutable-role append.

        @return Exact old prefix, original packet fields and optional appended role position.
        """
        row = position; craft = read(4); generation = read(8); ticks = read(2); paused = read(1); interval = read(8); serial = read(8)
        parts = [packets(2 if version >= 436 else 1) for _ in range(2)]; model_offset = position if version >= 436 else None; model = read(2) if version >= 436 else 65535
        return dict(start=row, end=position, craft=craft, generation=generation, ticks=ticks, paused=paused, interval=interval, serial=serial, parts=parts, model=model, model_offset=model_offset)

    next_structure = read(8); count = read(2); assert count <= 65535; read(32); rows = {}
    # Every original site retains its complete variable construction/module owners. Walking those extents avoids selecting coincidental ID/model bytes inside cargo or a captured name.
    for _ in range(count):
        row = position; site = read(2); generation = read(8); owner = read(1); kind = read(1); phase = read(1); x = read(2); y = read(2); read(12)
        for _ in range(2):
            read(2); packets(128)
        present = read(1); assert present <= 1; result = dict(start=row, site=site, generation=generation, owner=owner, kind=kind, phase=phase, x=x, y=y, jobs=[])
        if present:
            read(4); count_stock = read(1); assert count_stock <= 64; stock = {}
            for _ in range(count_stock):
                cargo = read(1); assert cargo not in stock; stock[cargo] = packets()
            result["stock"] = stock; result["facility"] = position; read(9); result["history"] = read(8)
            result["jobs"].append(service()); read(4); read(10); result["jobs"].append(service()); read(8); manufacture = read(1); assert manufacture <= 1
            if manufacture:
                read(34)
                for _ in range(4):
                    packets()
                read(40); clone = read(1); assert clone <= 1
                if clone:
                    read(12); size = read(1); assert size <= 124; read(size); orders = read(1); assert orders <= 32; read(14 * orders)
                replacement = read(1); assert replacement <= 1
                if replacement:
                    read(20)
        read(20); result["end"] = position; assert site not in rows; rows[site] = result
    assert position == end; return dict(start=start, body=body, end=end, length=length, next_structure=next_structure, rows=rows)


def legacy_payload(raw, require_unknown=True):
    """Remove only the actual436 two-byte service-role append from each original contained job.

    @param raw Exact current436 native stream.
    @param require_unknown True proves genuine upgraded435 jobs retained unknown; false is only for an explicitly malformed old-version candidate.
    @return Original435 SPST payload, with every original paid field/reference preserved.
    """
    layout = structure_rows(raw, 436); payload = bytearray(raw[layout["body"]:layout["end"]]); jobs = [job for row in layout["rows"].values() for job in row["jobs"]]
    # Reverse offset order preserves every earlier original field while removing precisely the append, never reconstructing, rewriting or inferring the legacy sender's role.
    for job in sorted(jobs, key=lambda job: job["model_offset"], reverse=True):
        assert not require_unknown or job["model"] == 65535; offset = job["model_offset"] - layout["body"]; del payload[offset:offset + 2]
    return bytes(payload)


def replace_family(raw, family, payload):
    """Replace one exact invocation-owned RIFF payload while preserving every unrelated native owner.

    @param raw Original native stream.
    @param family Exact four-byte family identifier.
    @param payload New bounded payload for a deliberate malformed copy.
    @return Same native stream with only that family length/payload changed.
    """
    start, _, _, end = riff_location(raw, family); size = len(payload); assert size < 1 << 28
    header = bytes([(size >> 24) << 4, (size >> 16) & 255, (size >> 8) & 255, size & 255]); return raw[:start + 4] + header + payload + raw[end:]


def approach_slot_offset(raw, vehicle=22, generation=8):
    """Locate the original heavy's saved shared approach, including a legitimately completed Visit cursor.

    @param raw Exact current native stream.
    @param vehicle Original native hull index.
    @param generation Original paid hull incarnation.
    @return Its original one-byte shared slot field position.
    """
    _, body, _, end = riff_location(raw, b"SPOR"); position = body + 8; count = raw[position]; position += 1; selected = []
    # Every exact queued instruction and optional original request is skipped by its own declared native extent; completed Visit queues still retain their actual shared berth.
    for _ in range(count):
        row = position; assert row + 22 <= end; entries = raw[row + 21]; assert entries <= 32; position += 22 + 14 * entries
        assert position < end; present = raw[position]; assert present <= 1; position += 1
        if present:
            assert position + 30 <= end
            if (int.from_bytes(raw[row:row + 4], "big"), int.from_bytes(raw[row + 4:row + 12], "big")) == (vehicle, generation):
                selected.append(position + 21)
            position += 30
    assert position == end and len(selected) == 1; return selected[0]
