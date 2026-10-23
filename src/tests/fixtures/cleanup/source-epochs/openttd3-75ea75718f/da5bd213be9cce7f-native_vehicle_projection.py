"""Compare legacy VEHS records across405's appended, empty native Space column.

Every pre-existing descriptor and record byte must remain identical. This accepts
only the explicit empty-column migration, never vehicle-field changes.
"""
from savegame_plane_digest import Reader


def legacy_vehicle_projection(raw, version):
    reader = Reader(raw)
    header = Reader(reader.take(reader.gamma() - 1))

    def table(root=False):
        entries = []
        while True:
            start = header.pos
            kind = header.byte()
            if kind == 0:
                break
            name = header.take(header.gamma())
            entries.append((kind, name, header.data[start:header.pos]))
        children = []
        for kind, name, _ in entries:
            if kind & 15 == 11:
                children.append((name, table()))
        removed = [name for _, name, _ in entries if root and name == b"space"]
        if root:
            assert removed == ([b"space"] if version >= 405 else [])
        return b"".join(data for _, name, data in entries if not (root and name == b"space")) + b"\0" + b"".join(data for name, data in children if not (root and name == b"space"))

    normalized = table(True)
    assert header.pos == len(header.data)
    rows = []
    while True:
        length = reader.gamma()
        if length == 0:
            break
        data = reader.take(length - 1)
        row = Reader(data)
        row.gamma()  # Sparse native VehicleID, retained in the raw comparison.
        assert row.byte() < 6, "legacy projection cannot discard a native Space vehicle"
        if version >= 405:
            assert data[-1:] == b"\0", "Space struct unexpectedly populated"
            data = data[:-1]
        rows.append(data)
    assert reader.pos == len(reader.data)
    return normalized, tuple(rows)
