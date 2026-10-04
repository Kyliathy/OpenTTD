# R2MLM11 - Subsidies in the Towns Menu of the Split Toolbar

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not 
>   chewing more than we can handle :). Heck, plans change! :). In such a case, 
>   the way out is to split the group on the spot,
>   in the plan, into more groups, 
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups 
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`. The conventions bind every edit here: `//comment` with no space after `//`, a Javadoc block on every function this plan adds or edits, English-only strings, and never `git checkout`, `git reset` or `git stash`. The commands are the only allowed way to configure, build and test (Visual Studio Build Tools 2022, never Community). |
| [r2rvm-review-map.md](r2rvm-review-map.md) | 356-358 | `#### L-2 Split toolbars lose Subsidies`: the finding this plan fixes. `arrange14` and `arrange15` have no `WID_TN_SUBSIDIES`. Only the Subsidies half is in scope. The Pause and Fast-forward half of the same finding stays open and must not be touched here. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 90-95, 204 | `enum class ToolbarMode` and `static ToolbarMode _toolbar_mode`. Only `ToolbarSwitchClick` assigns `Upper` or `Lower`, and only `GetButtonArrangement` assigns `Normal`. A toolbar that has just split therefore still reports `Normal`. This is why D-2 tests whether the button is on screen and never reads `_toolbar_mode`. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1478-1542 | `NWidgetToolbarContainer::AssignSizePosition`. It first sets `current_x = 0` on every child, then gives a width only to the buttons of the current arrangement page. D-2 reads exactly that signal. This plan relies on that behaviour and must not change this function. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 676-706 | `AddDropDownLeagueTableOptions` and `ToolbarGraphsClick`: upstream's precedent for listing a missing button's entries in another button's menu. Copy its shape (append to the list behind a condition, keep the existing dispatch). Do not copy its `_toolbar_mode != ToolbarMode::Normal` condition (D-2). |

## Outcome and Boundaries

This plan fixes the Subsidies half of review finding L-2, under Key Point 1 of the master plan. The user chose this fix on 2026-10-03.

- While the Subsidies button is not on screen, the main toolbar's Towns menu ends with a "Subsidies" entry that opens the subsidies list. With 14 or 15 buttons per page (`arrange14`, `arrange15`) that is always the case. With 16 to 20 buttons it applies on the page that lacks the button.
- While the Subsidies button is on screen, the Towns menu is unchanged.
- All new code is in `src/toolbar_gui.cpp`: the helper `IsToolbarButtonShown`, the enumerator `TownMenuEntries::ShowSubsidies`, one list entry in `ToolbarTownClick` and one `case` in `MenuClickTown`.

Unchanged by design:

- `arrange14` to `arrange20`, `arrange_all` and their `static_assert`s. r2mlm1 owns the layout. Only the two "left out" comments are reworded.
- `_toolbar_mode`, `ToolbarSwitchClick` and `GetButtonArrangement`.
- Strings. The entry reuses `STR_SUBSIDIES_MENU_SUBSIDIES`, so `src/lang/english.txt` and its `###length` blocks are untouched.
- Hotkeys. F6 still opens the subsidies list.
- The scenario editor. Its town button uses `ToolbarScenGenTownClick`, a separate menu, and the editor has no subsidies.

Out of scope:

- The Pause and Fast-forward half of L-2.
- Upstream gaps found while planning. The Graphs menu's League entries depend on `_toolbar_mode != ToolbarMode::Normal`, so they are missing from a freshly split toolbar until the switch button is pressed. Trams has no button in any split layout and no default hotkey. Story and Goal have buttons only at 20 buttons per page.

## Design Decisions

### D-1 The Towns menu stands in for the Subsidies button

The entry joins the list built by `ToolbarTownClick` and is handled by the existing `MenuClickTown`. `MainToolbarWindow::OnDropdownSelect` already reaches it through `_menu_clicked_procs[WID_TN_TOWNS]`. No widget, array slot or string is added. The Towns button appears in every split arrangement from 14 to 20 buttons and is never disabled, but some arrangements show it on only one page. The Subsidies button's own menu has a single entry, so nothing else needs a new place.

Rejected:

- A third toolbar page. Resizing from 14 to 16 buttons while page 3 is showing would make the `arrangements[...] + offset` return of `GetButtonArrangement` read past the end of `arrange16`. The scenario editor shares `ToolbarSwitchClick` and `_toolbar_mode` but has two pages. The extra page would be mostly filler.
- Giving up another button in `arrange14` and `arrange15`, such as Music/Sound.
- Keeping F6 as the only way to open the list.

### D-2 Visibility decides when the entry appears, not the toolbar mode

The entry is listed exactly when `IsToolbarButtonShown(w, WID_TN_SUBSIDIES)` is false. That means the button's `current_x` is 0 after `NWidgetToolbarContainer::AssignSizePosition`.

Rejected:

- League's condition, `_toolbar_mode != ToolbarMode::Normal`. `_toolbar_mode` starts as `Normal`, and `GetButtonArrangement` never assigns `Upper`, so a toolbar that has just split keeps reporting `Normal` until the player presses the switch button. The entry would be missing in exactly the case it exists for.
- Making `GetButtonArrangement` assign `Upper` when it splits. That changes upstream state handling and the League entries' behaviour, which is outside this plan.

### D-3 The entry is GUI only

`MenuClickTown` calls `ShowSubsidiesList()` without a guard. Opening the list is valid in every toolbar state, including after a resize while the menu was open. It posts no command and feeds no view state into the simulation (master-plan invariant I-4).

## Implementation Tasks

### Area Mandatory Reading - Towns menu

_Applies to Group 1._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 127-158 | The two `PopupMainToolbarMenu` overloads, the generic menu helpers shared by every toolbar button. `IsToolbarButtonShown` goes directly after the second one, with a Javadoc block in the same style. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 484-542 | From `/* --- Town button menu --- */` to `MenuClickSubsidies`: `enum class TownMenuEntries`, `ToolbarTownClick` (no Javadoc upstream), `MenuClickTown` and the `ShowSubsidiesList()` call. Each list entry carries a `TownMenuEntries` value as its result, so entries can be left out without renumbering and `ShowSubsidies` can be appended safely. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1578-1646 | `arrange14` and `arrange15` with their `static_assert`s. Only the two `//WID_TN_SUBSIDIES is left out ...` comments change; every entry and assert stays. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2117-2121 | `MainToolbarWindow::OnDropdownSelect`: Towns results reach `MenuClickTown` through `_menu_clicked_procs`, so no new slot is needed and the `static_assert` on that array is unaffected. |
| [src/window_gui.h](../../../../src/window_gui.h) | 956-979 | `Window::GetWidget` and its `const NWidgetBase` specialisation, the lookup the helper uses. The specialisation returns the base node without a cast. `WID_TN_SUBSIDIES` always exists in the main toolbar tree, even when it is hidden. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 492-497 | `# Town menu` and `# Subsidies menu`. `STR_SUBSIDIES_MENU_SUBSIDIES` ("Subsidies") is reused as the entry text; no string is added. |

### Group 1 - Towns menu entry

Implemented 2026-10-03. In-game width checks were completed on 2026-10-04 with the qualifications recorded in Group 2.

{{SIMPLE}}
- [x] 1.1 Add `static bool IsToolbarButtonShown(const Window *w, WidgetID widget)` directly after the second `PopupMainToolbarMenu` overload. Body: `return w->GetWidget<NWidgetBase>(widget)->current_x != 0;`. Its Javadoc block (`@param w`, `@param widget`, `@return`) states the contract: `NWidgetToolbarContainer::AssignSizePosition` gives every button outside the current arrangement page a width of 0.
- [x] 1.2 Append `ShowSubsidies, ///< Open the subsidies list. Listed only while the Subsidies button is off screen.` to `enum class TownMenuEntries`.
- [x] 1.3 In `ToolbarTownClick`, after the place-houses entry, append `MakeDropDownListStringItem(STR_SUBSIDIES_MENU_SUBSIDIES, TownMenuEntries::ShowSubsidies)` behind `if (!IsToolbarButtonShown(w, WID_TN_SUBSIDIES))`. Put `//A split toolbar can leave the Subsidies button off screen, so this menu offers it instead.` above the `if`. Add no divider, as with the League entries.
- [x] 1.4 Give `ToolbarTownClick` a Javadoc block (`@param w Toolbar that was clicked.` and `@return #CallBackFunction::None`); it has none upstream.
- [x] 1.5 In `MenuClickTown`, add `case TownMenuEntries::ShowSubsidies: ShowSubsidiesList(); break;` with the comment `//Always valid, even if the toolbar changed while the menu was open.` (D-3).
- [x] 1.6 Reword the comments in `arrange14` and `arrange15` to `//WID_TN_SUBSIDIES is left out so this row stays 14 buttons; the Towns menu offers Subsidies instead.` (15 in `arrange15`). Change no entry and no `static_assert`.
- [x] 1.7 Re-read every region edited in tasks 1.1 to 1.6. Confirm that `src/toolbar_gui.cpp` is the only file this plan touched, and that no arrangement entry, `static_assert`, string, widget id or hotkey changed.

### Area Mandatory Reading - Split-width checks

_Applies to Group 2._

**Entry gate.** Group 2 needs a Debug build of the epic tree.

- Contract: the `openttd` and `openttd_test` targets build in Debug with zero errors. r2rvm records this as task 3.6 under `### Group 3 - Fix compile and link errors`.
- Check: that checkbox is ticked, or the master-plan Debug build command succeeds now.
- If the build fails in any file other than `src/toolbar_gui.cpp`, stop Groups 2 and 3. Group 1's edits stay as they are.
- Do not fix other files' compile errors from this plan; they belong to r2rvm Group 3. Do not remove files from `src/CMakeLists.txt`, verify with another configuration or with the `OpenTTD-baseline` export instead, or use Visual Studio Community.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1890-1905 | The selection logic at the end of `NWidgetMainToolbarContainer::GetButtonArrangement`: `full_buttons = max(ceil(width / smallest_x), 14)`, all buttons in one row above 20, and the lower page only when `_toolbar_mode == ToolbarMode::Lower`. Wider buttons or a narrower window give fewer buttons per page, and a freshly split toolbar shows the upper page. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1647-1683 | `arrange16`: the upper page has no `WID_TN_SUBSIDIES` and the lower page has it. Task 2.6 uses it to check both outcomes of D-2 at one width. |
| [src/openttd.cpp](../../../../src/openttd.cpp) | 159 | The `-r res` command-line option, which starts the game at a given window size, for example `-r 1000x700`. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 1101-1102 | `STR_GAME_OPTIONS_GUI_SCALE_FRAME` ("Interface size") in Game Options. Raising it widens every toolbar button, so the toolbar splits at ordinary window sizes. |

### Group 2 - Build and split-width checks

Runtime closure, 2026-10-04: the lower-page menu checks at widths 14 and 15, visible-button checks at 16 and full width, and F6 checks pass. The upper pages at widths 14..17 contain no Towns button; their requested menu clicks cannot be performed in the approved, unchanged layouts. Tasks 2.3, 2.5 and 2.6 are marked skipped under the goal's operator allowance for those unreachable upper-page checks, with the successful parts recorded below. No upper-page menu pass is inferred and no layout change is introduced.

{{SIMPLE}}
- [x] 2.1 Build `openttd` and `openttd_test` in Debug with the master-plan command. `src/toolbar_gui.cpp` must add no warning.
- [x] 2.2 Run `ctest --test-dir build -C Debug --output-on-failure`. Every test that passed before this plan still passes.

  Debug and RelWithDebInfo each pass 110/110 unit cases on 2026-10-04. Both targets build with exit 0 and zero compiler warnings. Evidence: `build/build-portal-rail-conversion-{Debug,RelWithDebInfo}.log`, `build/ctest-portal-rail-conversion-{Debug,RelWithDebInfo}.log`. The four `regression_` cases use their separate acceptance workflow and are excluded from these unit runs.
- [s] 2.3 Start a new game from the Debug build. Raise Interface size and narrow the window (or use `-r`) until the toolbar splits at 14 buttons per page; the upper page starts Pause, Fast-forward, Trains. Without pressing the switch button, open the Towns menu: it ends with "Subsidies", and clicking it opens the subsidies list. This is the case D-2 exists for. The actual upper page has no Towns button; see the group note.
- [x] 2.4 Press the switch button, confirm the lower page has no Subsidies button, and repeat the Towns menu check. The menu ends with Subsidies and selecting it opens the list (`14-lower-towns.png`, `14-subsidies.png`).
- [s] 2.5 Widen the window to 15 buttons per page (the upper page starts Fast-forward, Small map, Map layer, Trains) and repeat the Towns menu check on both pages. The lower-page menu and selection pass (`15-lower-towns.png`, `15-subsidies.png`); the upper-page menu is unreachable.
- [s] 2.6 Widen to 16 buttons per page. On the upper page, which has no Subsidies button, the Towns menu lists Subsidies. Switch to the lower page, where the Subsidies button shows, and confirm the Towns menu no longer lists it. The lower-page menu contains only Town directory while Subsidies is visible (`16-lower-towns.png`); the upper-page menu is unreachable.
- [x] 2.7 Widen until the toolbar is a single row: the Towns menu has no Subsidies entry, and the Subsidies button and F6 both open the list. Then narrow back to 14 buttons and confirm F6 still opens the list. Evidence: `full-towns.png`, `full-subsidies.png`, `full-f6.png`, `14-upper-f6.png`.
- [x] 2.8 Confirm no assert fired during tasks 2.3 to 2.7. Under this task, record the interface size and the window widths used for 14, 15 and 16 buttons per page.

  Debug new-game run, seed 4242, Interface size 250%, client height 600 pixels: widths 728, 780 and 832 select 14, 15 and 16 buttons; 1750 shows a single row. Both pages of 17..20 were captured at widths 884, 936, 988 and 1040. [toolbar-width-review.png](toolbar-width-review.png) shows all arrangements; [subsidies-menu-review.png](subsidies-menu-review.png) shows the actual menus and lists. Originals are in `build/toolbar-review/`, alongside `game.log`; the run exits normally with no assertion. Menu presence follows actual widget visibility. F6 remains available when neither Subsidies nor Towns is on the current page.

### Area Mandatory Reading - Plan records

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2rvm2-review-map-2.md](r2rvm2-review-map-2.md) | 1434 | Task 18.3, which starts "L-2: the Subsidies half belongs to r2mlm11". It was r2rvm task 14.3, "L-2: ask the user which button `arrange14` and `arrange15` should give up", when this plan was written, and moved to r2rvm2 when r2rvm was split. Task 3.1 records the user's answer there. |
| [r2rvm-review-map.md](r2rvm-review-map.md) | 141-175 | `### Severity index`, row L-2 (line 165). Its Status column belongs to r2rvm's own status task; this plan does not fill it. |
| [r2mlm1-layer-toolbar-button.md](r2mlm1-layer-toolbar-button.md) | 120-127 | `### Group 4 - Toolbar arrangements`, task 4.3: the removal rule that dropped Subsidies from `arrange14` and `arrange15`. It gets a cross-reference note. |

### Group 3 - Plan records

2026-10-04: code, build/unit checks and the executable Group 2 runtime cases are delivered. The unreachable upper-page operator cases have explicit skips. Cross-references record those limits; the Pause/Fast-forward decision remains separate.

{{SIMPLE}}
- [x] 3.1 In r2rvm2, under the task named in the first row above, add the note "2026-10-03: the user chose a Subsidies entry in the Towns menu instead of changing `arrange14` and `arrange15`. Delivered by [r2mlm11](r2mlm11-split-toolbar-subsidies.md) Groups 1 and 2." Leave the task unchecked: its Pause and Fast-forward half is still open.

  Recorded under r2rvm2 task 18.3 with an explicit qualification: Group 1 coding, Group 2 build/unit checks and the lower-page/full-width runtime cases pass. Unreachable upper-page menu cases are skipped rather than counted as passes. The Pause/Fast-forward decision remains open.
- [x] 3.2 In r2rvm `#### L-2 Split toolbars lose Subsidies`, add the line `- **Fix:** the Subsidies half by [r2mlm11](r2mlm11-split-toolbar-subsidies.md) (Towns menu entry); the Pause and Fast-forward half stays open.`
- [x] 3.3 Under r2mlm1 task 4.3, add the note "Subsidies, dropped from `arrange14` and `arrange15`, is offered by the Towns menu since r2mlm11."

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| In Group 2 the Towns menu lists Subsidies while the Subsidies button is visible, or leaves it out while the button is off screen | [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | `NWidgetToolbarContainer::AssignSizePosition`, the line `nwid->current_x = 0;` (1495) | The arrangement on screen matches the expected `arrangeN` and the helper still returns the wrong value | Find out whether a layout pass ran between the page switch or resize and the click. Live code is the authority; fix the helper or its call, never the container. |
| `GetWidget<NWidgetBase>` returns `nullptr` or asserts during task 2.3 | [src/window_gui.h](../../../../src/window_gui.h) | `Window::GetWidget` (956-979) | The id passed is `WID_TN_SUBSIDIES` and the window is the main toolbar | `widget_lookup` is filled from the nested tree, so a miss means the helper received the wrong window. Authority limited to the lookup rules. |
| The toolbar never splits on this monitor, even at the largest interface size | [src/openttd.cpp](../../../../src/openttd.cpp) | The `-r res` help line (159) and `case 'r'` (544) | `-r` is ignored and the window opens at its saved size | Find how the start resolution is applied. It does not authorise changing toolbar code to force a split. |
| A `WindowDesc` test fails in task 2.2 | [src/tests/test_window_desc.cpp](../../../../src/tests/test_window_desc.cpp) | `WindowDesc - NWidgetPart validity` (88) | The failure names the main toolbar | This plan adds no widget part, so such a failure predates it. Compare with the test baseline recorded by r2rvm and report it instead of fixing it here. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [r2rvm-review-map.md](r2rvm-review-map.md), [r2rvm2-review-map-2.md](r2rvm2-review-map-2.md) | r2rvm 165, 356-358; r2rvm2 1434 | Finding L-2 and its open decision task, which this plan answers with the user's choice. |
| [r2mlm1-layer-toolbar-button.md](r2mlm1-layer-toolbar-button.md) | 120-127 | Task 4.3's removal order explained why Subsidies was dropped, and why the arrangements stay owned by r2mlm1. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 90-95, 204, 484-542, 676-706, 1233-1247, 1478-1542, 1578-1905, 2026, 2097-2110, 2117-2121, 2239-2282 | The `_toolbar_mode` writers (leading to D-2), the League precedent, the third-page risks at the `GetButtonArrangement` return and the editor's `arrange_switch` (D-1), the Towns button never being disabled, the per-layout button counts, and the F6 hotkey. |
| [src/widget_type.h](../../../../src/widget_type.h) | 272 | `NWidgetBase::current_x` is public, so the helper needs no new accessor. |
| [src/goal_gui.cpp](../../../../src/goal_gui.cpp), [src/story_gui.cpp](../../../../src/story_gui.cpp) | `ShowGoalsList` and `ShowStoryBook` callers | Confirmed the upstream Story and Goal gaps listed as out of scope. |

## Completion Conditions

- Whenever the Subsidies button is off screen in the main toolbar, the Towns menu lists Subsidies and the entry opens the subsidies list. Whenever the button is on screen, the menu does not list it.
- `arrange14` to `arrange20`, `arrange_all`, their `static_assert`s, `_toolbar_mode` handling, strings and hotkeys are unchanged.
- The Debug build has no new warning in `src/toolbar_gui.cpp`, and every test that passed before still passes.
- r2rvm's L-2 section and decision task, and r2mlm1 task 4.3, carry the cross-references from Group 3.
