# R2SFF1 orbital layout implementation notes

Owner: [Space fixes plan, Group 2](r2sff1-space-fixes-features-1.md#group-2---make-every-orbital-action-reachable).
The [GUI architecture](../../architecture/gui/archi-gui.md#invariants) remains
authoritative for retained windows, drawing and command dispatch.

## Delivered implementation

`space_structure_widget.h` names the original widget IDs 0-38 without renumbering
them. Site selection, recipe, selected builder and action-page selector remain
above the scrolling area. Construction, Inventory, Service, Connections and
Manufacture pages use at most two columns and five rows. The window has a resize
box, a saved size and usable-client bounds. Text controls cap their measured
minimum width; their complete action captions remain available as tooltips.

`RefreshPresentation` retains the existing selection and pure command preflights.
It runs from the window timer, input and explicit opening/selection. `OnPaint`
only draws; `DrawWidget` clips wrapped cached rows. Pixel scroll bounds are
rebuilt on resize/content changes, and the fixed site dropdown captures exact
generations so a deleted/reused pool slot cannot silently become its target.
No command, work grant, reservation or cargo owner was added.

A read-only comparison against HEAD found all nineteen synchronized command
expressions and their arguments unchanged after resolving named widget IDs.
Existing commands often return only `CMD_ERROR`; Group 6 supplies precise new
construction readiness and Group 8 presents it. Group 2 preserves the current
admission checks and reason disclosures rather than duplicating those producers.

## Focused validation recipe

Build `openttd_space_fixes_test` in the active configuration; output belongs in
`logs/space-fixes/`. Adding localized labels regenerates the shared string header
and requires an engine rebuild. This is compilation, not a full test regression.

Run one case at a time with
`python -B src/tests/multilayer/acceptance/space_fixes_checks.py --layout --run <fresh-id>`.
Use the factory demo for Core/Storage/Assembly, and
`--save space-heavy-service-440.sav` for the real two-berth Dock.
Compare explicit scales 100/200/250, `--resolution 1280x720`, and
`--font-size 16` (default 0). The fixture also changes the client to 1280x720
after opening the window, checks every active action's hit rectangle, scrolls
each page, compares canonical owner digests and clicks the bottom service-row
panel toggle through native mouse dispatch twice to restore the original state.
Each process has a 110-second deadline.

Native PPMs and JSON reports are rebuilt beneath
`test-cache/space-fixes/<fresh-id>/`; logs are
`logs/space-fixes/<fresh-id>.log`. These are disposable verification artifacts.
Inspect actual captures before accepting the group. Rebuild the ordinary
`openttd` target against the new generated strings before handing the game back.

## Verified results

All five focused native runs passed, one process at a time:

| Run | Conditions | Assertions | Native seconds |
| --- | --- | --- | --- |
| `layout-factory-200` | Factory; Full HD client; 200% | 516 | 7.796 |
| `layout-factory-small` | Factory; 1280x720; 200% | 516 | 6.797 |
| `layout-factory-large-font` | Factory; 1280x720; 200%; actual 32px normal font | 516 | 7.328 |
| `layout-dock-250` | Real two-berth Dock; Full HD client; 250% | 790 | 9.453 |
| `layout-core-100` | Unfinished Core; Full HD client; 100% | 237 | 5.359 |

The reported 200% construction window now opens at 1120x760, below the toolbar,
versus the baseline 1462x1068. The large-font small-client case fits at 1120x640.
The 250% Dock case fits at 1400x943 and also survives the in-process change to
1280x720. Native hit testing reaches all original action IDs on their selected
pages. A mouse click on the bottom service-row panel button executes the real
synchronized command; a second restores its original state. Page/selection/paint
queries preserve structure, vehicle and field digests.

Inspection of the first native image found an unpainted gap on shorter pages;
filler panels fixed it before the large-font, Dock and Core runs. The latter
three use test executable SHA256
`c7f6d234fbcfc1085151bbbe9d27b85324f4f3438ea8f57b3718a724ced929e7`.
The large-font construction and Dock service PNG previews were inspected after
lossless PPM-to-PNG conversion. Recreate these previews with Pillow's
`Image.open(ppm_path).save(ppm_path.with_suffix('.png'))`; both formats belong
beside the run's report in `test-cache/space-fixes/`.

The ordinary RelWithDebInfo game rebuilt successfully and paired its language
packs. Its SHA256 is
`094af71352553091184c5a51fd2d7a3649e6dde9f9606338c00fd69d31968c2e`.
Build evidence: `logs/space-fixes/build-game-layout.log`. Debug compilation and
the Group3 `shortcuts-debug-retained-layout` spot-check also passed: 570 assertions
in 7.563s, including all five pages and actual bottom-row clicks. The five-run
matrix above remains explicitly RelWithDebInfo.
