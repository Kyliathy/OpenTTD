"""Record remaining operator acceptance skips without claiming they passed.

@see docs/zz-reach2/upgrades/2026-10/r2mlm10-multi-layer-acceptance.md
"""

import re
from pathlib import Path

path = Path("docs/zz-reach2/upgrades/2026-10/r2mlm10-multi-layer-acceptance.md")
text = path.read_text(encoding="utf-8")
groups = {
    "1": ("1.1 1.2 1.3 1.4 1.5", "The dedicated-server/GUI-client construction, two-year network run, mid-game joins, client-only layer switching and complete desync-log operator workflow are skipped. No multiplayer determinism or joining-during-wait pass is inferred from singleplayer/unit evidence."),
    "2": ("2.2 2.3 2.4 2.5 2.6 2.7 2.8", "The remaining month-play, TTO, autosave-traffic, scenario-editor-to-game, preview-dialog, large-map and combined three-year traffic operator rows are skipped. Modern paused upgrades and complete-map round trips pass R2RVM2 task 18.1. The plan's legacy example trt00.sv0 is TTD and passes a Debug month/round trip; actual TTO name-conversion failures reproduce in the pre-epic build and are recorded in the master's Deferred Points. No completed full save matrix is claimed."),
    "3": ("3.1 3.2 3.3", "The framerate-window and multi-size process-memory operator measurements, including the conditional cost approval gate, are skipped. No threshold result, timing/memory number, user acceptance or reduced tick rate is inferred. Every layer still ticks as AD-6 requires; M-9 remains an open cost decision."),
}
for group, (tasks, reason) in groups.items():
    heading = re.search(rf"^### Group {group} - .*\n", text, re.MULTILINE)
    following = re.search(r"^### ", text[heading.end():], re.MULTILINE)
    end = heading.end() + following.start() if following else len(text)
    body = text[heading.end():end]
    assert set(re.findall(r"^- \[ \] (\d+\.\d+)\b", body, re.MULTILINE)) == set(tasks.split())
    body = re.sub(r"^- \[ \]", "- [s]", body, flags=re.MULTILINE)
    note = f"\nClosure, 2026-10-04, under the goal's explicit operator/gate allowance: {reason} Each skipped row is marked `[s]`, never a passing test.\n"
    text = text[:heading.end()] + note + body + text[end:]
closure = "Closed 2026-10-04 under the goal's operator/gate allowance. Automated save/digest, regression, unit, source-audit and documentation tasks are complete. Groups 1..3 record the skipped operator work; multiplayer, the full traffic/save matrix and memory/performance acceptance are not claimed as passed. The invariant table records actual evidence and remaining validation limits. Original unskipped full-acceptance criteria below remain future validation criteria."
text = text.replace("## Completion Conditions\n", "## Completion Conditions\n\n" + closure + "\n", 1)
path.write_text(text, encoding="utf-8")
print("Acceptance operator skips recorded; passing automated evidence remains separate.")
