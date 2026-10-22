"""Audit completed tasks and explicit skips in the epic's actual plan index.

@see docs/zz-reach2/upgrades/2026-10/r2mlm-multi-layered-map.md
"""

import re
from pathlib import Path

folder = Path("docs/zz-reach2/upgrades/2026-10")
master = (folder / "r2mlm-multi-layered-map.md").read_text(encoding="utf-8")
rows = re.findall(r"^\| \[([X~ ])\] \| \[([^]]+\.md)\]", master, re.MULTILINE)
assert len(rows) == 15
report = ["# Plan closure under the explicit goal operator/gate allowance, 2026-10-04.", "# x = completed; s = authorised skip, never a passing test."]
completed = skipped = 0
for status, name in rows:
    assert status == "X", (name, status)
    content = (folder / name).read_text(encoding="utf-8")
    tasks = re.findall(r"^- \[([xs ])\] ([0-9]+[A-Z]*\.[0-9]+)\b", content, re.MULTILINE)
    assert tasks and len({number for _, number in tasks}) == len(tasks), name
    pending = [number for flag, number in tasks if flag == " "]
    assert not pending, (name, pending)
    for heading, body in re.findall(r"(^### Group [^\n]+\n)(.*?)(?=^### |\Z)", content, re.MULTILINE | re.DOTALL):
        if re.search(r"^- \[s\]", body, re.MULTILINE):
            assert "skip" in body.lower() and "goal" in body.lower(), (name, heading)
    count = sum(flag == "x" for flag, _ in tasks)
    skip_count = sum(flag == "s" for flag, _ in tasks)
    completed += count
    skipped += skip_count
    report.append(f"{name}: completed {count}; explicit skips {skip_count}; pending 0")
report.append(f"TOTAL: 15 plans closed; completed {completed}; explicit skips {skipped}; pending 0")
(folder / "tools/plan-closure-status.txt").write_text("\n".join(report) + "\n", encoding="utf-8")
print("\n".join(report))
