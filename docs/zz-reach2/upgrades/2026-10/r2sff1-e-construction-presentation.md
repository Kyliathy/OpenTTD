# Construction presentation implementation and evidence

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group 8.
Architectural boundary: [GUI invariants](../../architecture/gui/archi-gui.md#invariants).
Native prerequisite evidence: [construction diagnosis and continuation](r2sff1-d-construction-notes.md).

## Implementation

The construction window and inspector share `GetSpaceConstructionBuilderText`,
`GetSpaceConstructionMaterialText` and the localized readiness reasons declared
in `space_construction_gui.h`. They read the sole native
`GetOrbitalConstructionReadiness` producer and original job/packet containers.
No window allocates power, reserves stock or advances a work clock.

The builder's actual current instruction is shown independently of the selected
blueprint. Queue construction identifies its selected builder/site and explains
that an edit restarts the queue. A native command rejection uses the existing
Space order error presentation. Hints point to explicit order, visit/load,
reserve, start, service and Core-panel actions.

Cargo already delivered and cargo reserved directly from the adjacent Core share
one exclusive job container. Current saves have no route-of-arrival quantity
split; the UI therefore shows required amounts, inputs held at the site, amounts
still missing, consumed receipts and cargo still aboard the selected builder.
It never adds a displayed delivery quantity to the same reservation again.
Completed frames show consumed material and zero missing inputs even though their
job containers are now empty.

Completing a Construct advances the native queue cursor immediately. The
readiness query recognizes the last completed Construct only if it ends the
queue, the queue is completed and the exact owned frame still exists. A later
Visit, removed/reused site or unrelated job cannot borrow that completion.

The inspector's earlier fixed details section and overflow would clip the new
remedies at the reported scale. Group 8 therefore pulls forward its bounded,
resizable layout foundation: three action columns, full-content pixel scrolling,
wrapped rows, shared hit coordinates and refresh outside paint. Group 30 still
owns the later asteroid/order-policy controls and their coverage. Original
inspector command IDs and arguments remain unchanged.

## Focused verification

Status: Group 8 passed its focused native and visual checks.

| Run | Client and font | Result |
| --- | --- | --- |
| `construction-presentation-final` | 1920x1027, 200% GUI scale, default font | 766 assertions, 9.781 seconds |
| `construction-presentation-final-large-font` | 1280x720, 200% GUI scale, explicit 16-point font | 766 assertions, 9.562 seconds |

Both use test executable SHA256
`e20bf7f4e58ce310f4de4b8c02164fdcb64c90d43e165ed813f2fbde28cccdc7`.
The normal `build/RelWithDebInfo/openttd.exe` was also rebuilt with matching
language data: SHA256
`ac4fa73cd5e26249f30f41ad8e06507e0bac57149bc29efb09f2aeecf15eeefc`.

Visual review confirmed the remedy appears in both views, wraps at enlarged
fonts, and leaves the bottom actions reachable. A selected completed Core shows
16 Steel and 24 Goods consumed, zero missing inputs and 740/740 work. Its selected
builder separately names its completed Storage order, avoiding attribution of
that hull's last work to the selected Core. The original demo save limitation
from the construction notes still applies.

Earlier `construction-presentation-200`/`-large-font` runs passed geometry and
owner checks but visual review found an invalid string parameter in the new
status line. The `-v2` runs correctly fail the added text assertions. The final
code supplies a parameter-free native StringID to `{STRING}`; the `-v3` and final
captures show the actual reason. Keep those earlier logs as diagnostic history,
not acceptance evidence. No world-state or work-accounting change fixed this
presentation defect.

The `presentation` case loads immutable `space-module-cancelled-418.sav`, moves
only real hulls, approaches a real empty paid job, and captures a material wait.
An explicit construction-window Reserve click moves actual Core stock. The
next native opportunity starts work; ordinary ticks complete the recipe. The
case captures both windows at wait, reservation, work, completion and completed
Core inspection, including the bottom of their scroll ranges. It compares
native fleet, job/packet, field, manifest and utility digests across opening,
painting, scrolling, closing and layer changes.

Rebuild `openttd_space_fixes_test`, then run one case at a time:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction presentation --save space-module-cancelled-418.sav --run <fresh-id>
```

Use the harness's `--resolution` and `--font-size` options for the focused smaller
client/enlarged-font variation. Native PPM captures and input/configuration/hash
reports are rebuilt under `test-cache/space-fixes/<run>/`; logs are under
`logs/space-fixes/<run>.log`. Any PNG used for visual review is a lossless
conversion of those actual native captures in the same output directory.
