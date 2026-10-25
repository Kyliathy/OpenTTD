"""Preserve incoming pure-reader imports through A's sole save-table owner.

@file save_tables.py Compatibility exports only; no duplicate parser or runtime setup.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Incoming controllers may retain their original import spelling while complete
TABLE/nested/dense-hole and merged-family behavior stays in savegame_tables.py.
"""
from savegame_tables import (Reader, read_stream, chunks, table_header, table_record,
                             saved_table, saved_owner_table, merged_world_chunks,
                             compare_merged_worlds, complete_world)
