"""Record exact authorised operator skips in the completed portal implementation plans.

@see docs/zz-reach2/upgrades/2026-10/r2mlm-multi-layered-map.md
"""

import re
from pathlib import Path

folder = Path("docs/zz-reach2/upgrades/2026-10")
specifications = {
    "r2mlm7-layer-portal-foundation.md": {
        "7": ("7.5", "The build/removal/owner-change infrastructure-window comparison is skipped. No complete ownership/runtime counter pass is inferred from code or unit checks."),
        "8": ("8.3", "The advanced-settings-window operator check is skipped. Both settings and generated help strings are delivered and built; window visibility is not claimed from a build."),
    },
    "r2mlm8-bus-elevators.md": {
        "2": ("2.7", "The complete placement/cost/removal operator check is skipped. The representative save contains elevator pairs, but that does not verify every command accounting action."),
        "3": ("3.7", "The shaft-blocking/queue operator run is skipped; no vehicle-ahead pass is inferred from controller implementation."),
        "4": ("4.4 4.5", "Stop/breakdown and articulated save-in-shaft operator checks are skipped. R2MLM12 also supersedes the original capped-speed ride with a saved 370-tick wait."),
        "5": ("5.2 5.3 5.4 5.5", "The path-cache, route-estimate, cross-layer servicing and lost-vehicle operator matrix is skipped; code integration and unit results do not pass these live vehicle checks."),
        "6": ("6.1 6.2 6.3 6.4 6.5", "The underground road-building, station naming/cargo/payment, depot/autoreplace and vehicle-follow operator matrix is skipped. The supplied save contains buses/roads; no complete matrix pass is inferred."),
        "7": ("7.3", "The every-direction bus visibility operator run is skipped. R2MLM13 repairs foreground coverage and records static elevator captures, without claiming a complete bus-entering matrix."),
        "8": ("8.1 8.2 8.3 8.4", "The year-long bus run, occupied removal, bankruptcy and traffic-save operator scenarios are skipped. Paused complete-map round trips pass R2RVM2 task 18.1 and R2MLM1 Group 7B, but do not cover these scenarios."),
    },
    "r2mlm9-rail-layer-portals.md": {
        "2": ("2.7", "The complete placement/cost/removal/infrastructure-window operator check is skipped. Existing pairs and the command implementation are not a runtime accounting pass."),
        "3": ("3.7", "The straddling-consist reversal operator run is skipped; the supplied-save round trips do not include a reversal at that exact position."),
        "4": ("4.4 4.5", "The occupied block/path-signal operator matrix is skipped. R2MLM13 Group 9C successfully places path signals and runs the connected train across layers, but does not test an occupied path/block on both ends."),
        "5": ("5.3 5.4 5.5", "The year-long cached/uncached search, nearest-depot/safe-wait and reversal operator matrix is skipped. Short supplied-save Debug smoke is recorded separately, with no year-long claim."),
        "6": ("6.1 6.2 6.3 6.4 6.5", "The complete underground construction, naming/payment, servicing/autoreplace and vehicle-follow operator matrix is skipped. Group 9C of R2MLM13 proves connected platforms and station stops in the supplied game; it does not pass every row here."),
        "7": ("7.3 7.4", "The every-direction entering-train and monorail/maglev operator checks are skipped. R2MLM13 records all existing rail-end static captures and a Surface-SW traffic burst after the front repair; that subset is not the full matrix."),
        "8": ("8.1 8.2 8.3 8.4 8.5", "The multi-train two-year run, straddling save, occupied/reserved removal, conversion and year-long cache operator scenarios are skipped. Rail-pair conversion/accounting code is fixed by R2RVM2 task 18.8 and tested at build/unit level, without a conversion-scenario pass."),
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
        note = f"\nClosure, 2026-10-04, under the goal's explicit operator allowance: {reason} Each skipped row is marked `[s]`, never a passing test.\n"
        text = text[:match.end()] + note + body + text[end:]
    if filename == "r2mlm9-rail-layer-portals.md":
        text = text.replace("The r2mlm6 sprite constants are not in `src/table/sprites.h`, so this group stays skipped and the depot stand-in remains.", "The R2MLM6 sprite constants and art are now integrated; the obsolete missing-constants note is superseded by Group 7's delivery record.")
    closure = "Closed 2026-10-04 with the operator skips recorded in each affected group above. Coding and automated checks are delivered: zero compiler warnings, 111/111 unit cases in both configurations and 115/115 full release CTest cases (`build/build-console-input-*.log`, `build/ctest-console-input-*.log`, `build/ctest-console-final-RelWithDebInfo.log`). The original full runtime criteria below remain future validation criteria; closure under the goal allowance does not mean skipped scenarios passed."
    text = text.replace("## Completion Conditions\n", "## Completion Conditions\n\n" + closure + "\n", 1)
    path.write_text(text, encoding="utf-8")
    print(filename, "recorded", sum(len(tasks.split()) for tasks, _ in groups.values()), "operator skips")
