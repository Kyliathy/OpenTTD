"""Reconcile exact operator/gallery skips under the user's goal allowance.

@see docs/zz-reach2/upgrades/2026-10/r2mlm-multi-layered-map.md
"""

import re
from pathlib import Path

folder = Path("docs/zz-reach2/upgrades/2026-10")
specifications = {
    "r2mlm12-deep-underground-tunnels-and-elevators.md": {
        "1": ("1.3 1.4 1.5", "The operator-created version-369 portal/vehicle fixture and its historical pre-change captures/report are skipped. The supplied save is version 370; no downgrade or synthetic replacement fixture is manufactured. Task 3.6 separately verifies an authentic bare three-layer save upgrade."),
        "5": ("5.8", "The build/unit portion passes in the final evidence of task 1.2; the road-toolbar/both-picker screenshot operator portion is skipped. No complete picker UI pass is inferred from compilation."),
        "8": ("8.8", "The build/unit portion passes task 1.2. Main/editor dropdowns and N cycling are checked in R2MLM1 Group 7; the complete four-dropdown operator capture matrix is skipped, so no extra-view/smallmap selection-isolation pass is inferred."),
        "9": ("9.1 9.2 9.3 9.4 9.5 9.6", "The full in-game screenshot/refusal/portal-placement matrix is skipped. Stand-in scenes are superseded by generated art and the requested pre-change portal fixture is absent. Task 9.7 records current capture subsets and their limits."),
        "10": ("10.2 10.3 10.4 10.5 10.6 10.7", "User scenarios D1..D6 are skipped. The supplied-save rail crossing smoke is useful evidence but does not prove elevator timing, queues, wait-preserving saves or the complete road/rail comparison. No scenario pass is inferred from unit tests."),
        "11": ("11.7", "The memory-cost approval question is skipped. R2RVM2 task 18.2 has no user answer to reuse; M-9 remains open in the master's Deferred Points. No acceptance or chosen mitigation is inferred."),
    },
    "r2mlm13-layer-art-review-and-new-sprites.md": {
        "4": ("4.2 4.3 4.4 4.5 4.6 4.7", "The human-created 24-end art gallery and its complete 1x/2x review/traffic capture sheets are skipped. The supplied 18-end save is reviewed separately in Groups 9B/9C; it lacks road tunnels and some elevator orientations and is not a substitute for the complete gallery."),
        "5": ("5.1 5.2 5.3 5.4", "The gallery-dependent complete artwork verdict table, optional separate Deep housing decision and human confirmation are skipped. The development work list in 5.5 remains the implementation choice: retain existing generated art with the foreground repairs, extend to 66 sprites, and retain shared Underground/Deep elevator housings. This is not a human art verdict or approval."),
        "6": ("6.3", "Offset 45 is generated, fitted, compared with the elevator icon and integrated. Only its native vanilla-toolbar-crop operator comparison is skipped; the complete combined task is marked s rather than claiming that missing comparison passed."),
        "9": ("9.7", "The all-road-tunnel entering-bus gallery operator check is skipped because the supplied save contains no Surface/Underground road tunnel. New art is generated and integrated; its complete traffic/visibility matrix is not inferred from static or offline previews."),
        "10": ("10.1 10.2 10.3 10.4", "The full gallery capture/approved-verdict comparison and historical stand-in comparison operator matrix is skipped. Current authentic portal, traffic and layer captures are listed in task 10.6. Human approval remains absent; no complete final art acceptance is claimed."),
    },
}

for filename, groups in specifications.items():
    path = folder / filename
    text = path.read_text(encoding="utf-8")
    for group, (task_names, reason) in groups.items():
        match = re.search(rf"^### Group {group} - .*\n", text, re.MULTILINE)
        assert match, (filename, group)
        following = re.search(r"^### ", text[match.end():], re.MULTILINE)
        end = match.end() + following.start() if following else len(text)
        body = text[match.end():end]
        expected = set(task_names.split())
        found = set(re.findall(r"^- \[ \] (\d+\.\d+)\b", body, re.MULTILINE))
        assert found == expected, (filename, group, found, expected)
        body = re.sub(r"^- \[ \]", "- [s]", body, flags=re.MULTILINE)
        note = f"\nClosure, 2026-10-04, under the goal's explicit operator/gate allowance: {reason} Each skipped row is marked `[s]`, never a passing test.\n"
        text = text[:match.end()] + note + body + text[end:]
    closure = "Closed 2026-10-04 under the goal's operator/gate allowance, with each skip recorded at its owning group. Implementation and automated checks are complete: both configurations build without compiler warnings and pass 111/111 unit cases; full release CTest passes 115/115. Human art, cost and scenario decisions are not inferred. The original full runtime/approval criteria below remain future validation criteria."
    text = text.replace("## Completion Conditions\n", "## Completion Conditions\n\n" + closure + "\n", 1)
    path.write_text(text, encoding="utf-8")
    print(filename, "recorded", sum(len(tasks.split()) for tasks, _ in groups.values()), "operator/gate skips")
