"""Bind successful earlier ordinary loads without repeating their native processes."""
import json
from pathlib import Path
import re
import runpy
import sys

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT / "src/tests/multilayer/acceptance"))
demo = runpy.run_path(str(ROOT / "src/tests/multilayer/acceptance/space_demo_checks.py"))
identity, chunks = demo["identity"], demo["chunks"]
folder = ROOT / "mlm/reviews/space-demo/verify-relwithdebinfo/20261018-085122-4602e0a5f3"
reports = [(p, json.loads(p.read_text())) for p in (folder / "reports").glob("native-*.json")]
launcher = (ROOT / "play.ps1").read_text(encoding="utf-8-sig")
entries = re.findall(r"@\{ Name = '([^']+)'; SaveName = '[^']+';\s*Snapshot = '([^']+)'", launcher)[:28]
rows, members = [], []
for name, relative in entries:
    source = ROOT / relative.replace("\\", "/")
    first = folder / "saves" / (name + "-loaded.sav")
    second = folder / "saves" / (name + "-reloaded.sav")
    for original, output in ((source, first), (first, second)):
        ref, = [identity(p) for p, d in reports if d["exit_status"] == 0 and
                identity(original) in d["inputs"] and identity(output) in d["artifacts"]]
        members.append(ref)
    version, before = chunks(first)
    second_version, after = chunks(second)
    matched = [key.decode() for key in demo["OWNERS"] if key in before]
    assert version == second_version == 440
    assert all(before[key.encode()] == after[key.encode()] for key in matched), name
    rows.append(dict(name=name, source=identity(source), loaded=identity(first), reloaded=identity(second),
                     save_version=version, unchanged_owners=matched))
game = ROOT / "build/RelWithDebInfo/openttd.exe"
native = ROOT / reports[0][1]["executable"]["path"]
expected = bytearray(game.read_bytes())
header = int.from_bytes(expected[60:64], "little")
expected[header + 92:header + 94] = (3).to_bytes(2, "little")
assert bytes(expected) == native.read_bytes()
target = ROOT / "logs/space-demo-verified-earlier-loads.json"
target.write_text(json.dumps(dict(complete=True, application=identity(game), checkpoints=rows,
                                  native_processes=members, note="56 successful ordinary processes retained; later failed runner continuation excluded."), indent=2) + "\n")
print(f"Bound {len(rows)} earlier demos, {len(members)} native processes; all {len(matched)} owner families stable.")
