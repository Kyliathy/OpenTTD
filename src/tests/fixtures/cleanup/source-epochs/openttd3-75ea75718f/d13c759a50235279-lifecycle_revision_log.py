"""Validate the exact native revision-log append when older originals meet a new build.

@file lifecycle_revision_log.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15D.
Only one independently decoded Load/Revision event may differ; every gameplay chunk
and every earlier diagnostic byte remains exact. No save is rewritten here.
"""

import ast
import operator
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from save_tables import saved_table, chunks, read_stream
from geothermal_tube_binding_files import world_chunks
from local_electrical_feed_checks import identity


def constant_integer(node):
    """Read only the native generator's integer literal arithmetic.

    @param node Parsed expression node for the generated NewGRF version constant.
    @return Integer result; names, calls and arbitrary Python code always refuse.
    """
    operations = {ast.Add: operator.add, ast.LShift: operator.lshift, ast.BitOr: operator.or_}
    # The generated build constant uses arithmetic and bit packing only.
    # Admitting executable expressions would make expected identity untrusted.
    if isinstance(node, ast.Constant) and type(node.value) is int:
        return node.value
    if isinstance(node, ast.BinOp) and type(node.op) in operations:
        return operations[type(node.op)](constant_integer(node.left), constant_integer(node.right))
    raise AssertionError("unsupported generated native revision constant")


def expected_revision():
    """Read the current generated build's actual native diagnostic revision contract.

    @return Exact development revision/NewGRF/save-version/modified fields and source identity.
    """
    path = ROOT / "build/generated/rev.cpp"
    source = path.read_text(encoding="utf-8")
    revision = re.search(r'_openttd_revision_hash\s*=\s*"([a-f0-9]{40})";', source)
    modified = re.search(r"_openttd_revision_modified\s*=\s*([0-2]);", source)
    version = re.search(r"_openttd_newgrf_version\s*=\s*([^;]+);", source)
    assert revision is not None and modified is not None and version is not None
    newgrf = constant_integer(ast.parse(version[1], mode="eval").body)
    assert newgrf & (1 << 19) == 0, "this scoped helper requires the actual development-build revision policy"
    state = int(modified[1])
    return {"text": "gum"[state] + revision[1], "newgrf": newgrf, "slver": 434, "modified": state,
            "generated_source": identity(path)}


def validate_append(before, after, before_table, after_table, tick, expected):
    """Verify one exact appended native Load/Revision action and unchanged prior bytes.

    @param before Original complete GLOG transport and rows.
    @param after Current ordinary output's GLOG transport and rows.
    @param before_table Fully decoded original named rows.
    @param after_table Fully decoded output named rows, including the new action.
    @param tick Actual original DATE tick at which this native file was loaded.
    @param expected Current generated native build revision fields.
    @return Decoded appended event; extra changes, altered history or wrong fields refuse.
    """
    assert before["kind"] == after["kind"] == 3
    assert after["rows"][:-1] == before["rows"], "earlier native diagnostic history changed"
    assert len(after_table) == len(before_table) + 1
    row = after_table[len(before_table)]
    prefix = "action[0]."
    wanted = {"at": 1, "tick": tick, "action.count": 1, prefix + "ct": 1}
    # The added load may contain exactly one Revision object. Settings, GRF,
    # mode and all other changes remain forbidden even when their counts fit.
    for name in ("mode", "revision", "oldver", "setting", "grfadd", "grfrem", "grfcompat", "grfparam", "grfmove", "grfbug", "emergency"):
        wanted[prefix + name + ".count"] = 1 if name == "revision" else 0
    for name in ("text", "newgrf", "slver", "modified"):
        value = expected[name]
        wanted[prefix + "revision[0].revision." + name] = value.encode("utf-8").hex() if name == "text" else value
    assert row == wanted, "native revision append contains unexpected fields or values"
    previous = []
    for record in before_table.values():  # The native logger appends a revision only when the most recent original revision differs; duplicate same-build records cannot be relabelled as expected migration.
        for index in range(record["action.count"]):
            root = f"action[{index}]."
            if record[root + "ct"] == 1:
                previous.append({name: record[root + "revision[0].revision." + name] for name in ("text", "newgrf", "slver", "modified")})
    current = {name: wanted[prefix + "revision[0].revision." + name] for name in ("text", "newgrf", "slver", "modified")}
    assert previous
    # Gamelog::TestRevision compares these three fields; save-version alone
    # cannot justify a new event from an otherwise identical native executable.
    assert any(previous[-1][name] != current[name] for name in ("text", "newgrf", "modified")), "same native revision cannot append a new revision change"
    return row


def compare_original(first, second, load_tick):
    """Compare every native owner, allowing only a strictly checked build-revision append.

    @param first Exact original world or untouched next-opportunity baseline identity.
    @param second Ordinary current read or actual complete next-opportunity output identity.
    @param load_tick Tick from the earlier loaded original, distinct from its later baseline tick.
    @return Complete byte-exact chunk count and optional independently checked diagnostic event.
    """
    left_path, right_path = ROOT / first["path"], ROOT / second["path"]
    left, right = world_chunks(left_path), world_chunks(right_path)
    assert left.keys() == right.keys() and len(left) == 126
    changed = {name for name in left if left[name] != right[name]}
    if not changed:
        return {"complete_chunks_exact": 126, "revision_append": None}
    assert changed == {b"GLOG"}, "gameplay or other diagnostic chunks changed during native lifecycle read"
    a = next(row for row in chunks(read_stream(left_path)[1]) if row["name"] == b"GLOG")
    b = next(row for row in chunks(read_stream(right_path)[1]) if row["name"] == b"GLOG")
    expected = expected_revision()
    event = validate_append(a, b, saved_table(left_path, b"GLOG"), saved_table(right_path, b"GLOG"), load_tick, expected)
    return {"complete_chunks_exact": 125, "revision_append": {"event": event, "expected_build": expected,
            "previous_complete_rows_exact": len(a["rows"]), "all_other_complete_chunks_exact": True}}
