# R2MLM1 - Layer Vocabulary and the Toolbar Layer Button

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 62-64 | `### AD-1 Layer vocabulary`: the exact `MapLayer` enumerators, `MAX_MAP_LAYERS`, and the rule that names and icons live in `src/map_layer_gui.cpp`. This plan is the owner of that vocabulary; later plans consume it unchanged, so the values written here are final. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 95-97 | `### AD-7 Viewing a layer is view state`: `Viewport::layer` is client state and never read by commands or ticks. Constrains every task here: the button changes view state only, posts no command, and nothing in this plan is saved. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 157-174 | `## Engineering Conventions` and `## Build and Test Commands`: the user's comment style (`//comment`, Javadoc on every function and parameter), file naming, the no-move/no-delete rule, and the exact build and test commands used for verification. |
| [src/widgets/toolbar_widget.h](../../../../src/widgets/toolbar_widget.h) | 13-49 | `enum ToolbarNormalWidgets`: its order is the index of `_toolbar_button_procs`, `_menu_clicked_procs` and `_toolbar_button_sprites`, and the offset of each tooltip from `STR_TOOLBAR_TOOLTIP_PAUSE_GAME`. Inserting `WID_TN_MAP_LAYER` shifts every later index, so all parallel arrays and the tooltip block change in the same group. |
| [docs/zz-reach2/architecture/gui/archi-gui.md](../../architecture/gui/archi-gui.md) | 217-223 | `## Invariants`: `OnPaint` and `DrawWidget` only draw, the GUI changes the world only through commands, widget indices name nested-tree nodes. Decides that the button's icon is updated from `OnInvalidateData`, never from `OnPaint`. |

## Outcome and Boundaries

This plan delivers Key Point 1 of the master plan.

- `src/map_layer_type.h` with `enum class MapLayer` and `MAX_MAP_LAYERS` exactly as AD-1 states.
- `Viewport::layer` (default `MapLayer::Surface`) in `src/viewport_type.h`.
- `src/map_layer_gui.h` / `src/map_layer_gui.cpp` exporting `GetMapLayerName`, `GetMapLayerIcon`, `GetNextMapLayer`, `SetViewportLayer` and `CycleMainViewportLayer`.
- `WID_TN_MAP_LAYER` immediately after `WID_TN_SMALL_MAP` in the main toolbar, and `WID_TE_MAP_LAYER` immediately after `WID_TE_SMALL_MAP` in the scenario editor toolbar, with tooltips and a hotkey.

Interim behaviour, by design: the button cycles the main viewport's `layer` over all `MAX_MAP_LAYERS` values and updates its icon and tooltip, but the map drawn does not change yet. r2mlm2 limits the cycle to `MultiLayerMap::GetLayerCount()`; r2mlm5 replaces the body of `SetViewportLayer` so the view really switches. Stand-in icons until r2mlm6 delivers art: Surface `SPR_IMG_PLANTTREES`, Underground `SPR_IMG_TUNNEL_RAIL`, Space `SPR_IMG_AIRPLANESLIST`.

Out of scope: tile storage, drawing, savegame (the view layer is never saved), and any command.

## Implementation Tasks

### Area Mandatory Reading - Layer vocabulary and view state

_Applies to Groups 1-2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport_type.h](../../../../src/viewport_type.h) | 30-46 | `struct Viewport`: the plain data struct that receives the new `layer` member. `ViewportData` derives from it, so every viewport (main, extra, vehicle view) inherits the field. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 209-274 | `InitializeWindowViewport`: the only constructor path for viewports; it centres on a tile, a vehicle, or the main window's position. Task 2.2 initialises `layer` here, copying the main viewport's layer when the focus is `INVALID_TILE`. |
| [src/window_func.h](../../../../src/window_func.h) | 20-20 | `GetMainWindow()`: the accessor `CycleMainViewportLayer` uses to reach the main viewport. |
| [src/CMakeLists.txt](../../../../src/CMakeLists.txt) | 266-270 | The alphabetical `add_files` entries around `map.cpp`, `map_func.h`, `map_type.h`: where the new headers and sources are registered. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 477-481 | `# Map menu` strings: the neighbourhood where the three layer names are added. |

### Group 1 - Layer vocabulary

{{SIMPLE}}
- [ ] 1.1 Create `src/map_layer_type.h` with the standard OpenTTD licence header, `@file` comment and include guard `MAP_LAYER_TYPE_H`, including `core/enum_type.hpp`.
- [ ] 1.2 Define `enum class MapLayer : uint8_t { Surface = 0, Underground = 1, Space = 2, End, Invalid = 0xFF }` with a `///<` comment per enumerator, and a Javadoc block on the enum that states the values are the stacking order and must never be reordered.
- [ ] 1.3 Define `static constexpr uint MAX_MAP_LAYERS = to_underlying(MapLayer::End);` with a `static_assert(MAX_MAP_LAYERS == 3)` and a comment that AD-1 is the authority.
- [ ] 1.4 Register `map_layer_type.h` in `src/CMakeLists.txt` in alphabetical position after `map_func.h`.

### Group 2 - View state and layer GUI helpers

{{MEDIUM}}
- [ ] 2.1 Add `MapLayer layer = MapLayer::Surface; ///< Map layer shown by this viewport. View state only, never read by the simulation.` to `struct Viewport` and include `map_layer_type.h` in `viewport_type.h`.
- [ ] 2.2 In `InitializeWindowViewport`, set `vp->layer` explicitly: copy `GetMainWindow()->viewport->layer` in the `INVALID_TILE` branch and use `MapLayer::Surface` otherwise; add the business comment explaining the copy.
- [ ] 2.3 Add strings `STR_MAP_LAYER_SURFACE`, `STR_MAP_LAYER_UNDERGROUND`, `STR_MAP_LAYER_SPACE` under a new `# Map layers` comment after the map menu strings in `src/lang/english.txt`.
- [ ] 2.4 Create `src/map_layer_gui.h` declaring `StringID GetMapLayerName(MapLayer layer)`, `SpriteID GetMapLayerIcon(MapLayer layer)`, `MapLayer GetNextMapLayer(MapLayer layer, uint layer_count)`, `void SetViewportLayer(Window *w, MapLayer layer)` and `void CycleMainViewportLayer()`, each with a Javadoc block and `@param` lines.
- [ ] 2.5 Create `src/map_layer_gui.cpp` implementing the name and icon lookups with a `switch` over `MapLayer` (stand-in icons from `## Outcome and Boundaries`), and `GetNextMapLayer` wrapping to `MapLayer::Surface` after `layer_count - 1`.
- [ ] 2.6 Implement the interim `SetViewportLayer`: assign `w->viewport->layer`, mark the window dirty, and call `InvalidateWindowData(WindowClass::MainToolbar, 0)`; add a comment that r2mlm5 replaces this body with scroll translation.
- [ ] 2.7 Implement `CycleMainViewportLayer` as `SetViewportLayer(GetMainWindow(), GetNextMapLayer(current, MAX_MAP_LAYERS))`, with a comment that r2mlm2 swaps `MAX_MAP_LAYERS` for `MultiLayerMap::GetLayerCount()`.
- [ ] 2.8 Register `map_layer_gui.h` and `map_layer_gui.cpp` in `src/CMakeLists.txt`, then build both configurations.

### Area Mandatory Reading - Main toolbar wiring

_Applies to Groups 3-5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1340-1373 | `_menu_clicked_procs`: dropdown handlers indexed by widget with `// N` comments. The new button has no dropdown, so it gets a `nullptr` slot at index 5 and every later comment is renumbered. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1522-1842 | `NWidgetMainToolbarContainer::GetButtonArrangement`: `arrange14` to `arrange20` (two rows of N buttons each) and `arrange_all`. The comment at the top requires each array to hold exactly `2 * N` entries. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 866-884 | `ToolbarZoomInClick` / `ToolbarZoomOutClick`: the push-button pattern (`HandleButtonClick` with the editor or normal widget id) the layer button copies. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1964-2003 | `ToolbarButtonProc` and `_toolbar_button_procs`: click handlers indexed by widget. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2006-2186 | `struct MainToolbarWindow`: `OnClick`, `OnHotkey`, `OnInvalidateData` (zoom button state) and `hotkeys`. The layer button's icon refresh and hotkey go here. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2188-2261 | `_toolbar_button_sprites`, `GetToolbarMaximalImageSize`, `MakeMainToolbar`: widget type and initial sprite per button, and the spacer placed before `WID_TN_SMALL_MAP` (nothing may be inserted between the map and layer buttons). |
| [src/toolbar_gui.h](../../../../src/toolbar_gui.h) | 16-57 | `enum MainToolbarHotkeys`: the new `MTHK_CYCLE_MAP_LAYER` is appended at the end. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 384-416 | `# Tooltips for the main toolbar` with `###length 31`: the offset-indexed tooltip block. The new tooltip goes right after `STR_TOOLBAR_TOOLTIP_DISPLAY_MAP` and the length becomes 32. |
| [src/rail_gui.cpp](../../../../src/rail_gui.cpp) | 505-514 | `BuildRailToolbarWindow::OnTooltip`: the pattern for a dynamic tooltip with `GuiShowTooltips` and an encoded string. |

### Group 3 - Main toolbar widget and parallel arrays

{{MEDIUM}}
- [ ] 3.1 Insert `WID_TN_MAP_LAYER, ///< Cycle the map layer shown by the main viewport.` directly after `WID_TN_SMALL_MAP` in `ToolbarNormalWidgets`.
- [ ] 3.2 Insert `STR_TOOLBAR_TOOLTIP_CYCLE_MAP_LAYER :{BLACK}Switch the main view between the surface, underground and space maps` right after `STR_TOOLBAR_TOOLTIP_DISPLAY_MAP` and change `###length 31` to `###length 32`.
- [ ] 3.3 Insert `ToolbarMapLayerClick` (declared in Group 5) after `ToolbarMapClick` in `_toolbar_button_procs`, keeping a forward declaration above the array.
- [ ] 3.4 Insert `nullptr, // 5` after `MenuClickMap, // 4` in `_menu_clicked_procs` and renumber every following `// N` comment.
- [ ] 3.5 Insert `{WID_TN_MAP_LAYER, WWT_PUSHIMGBTN, SPR_IMG_PLANTTREES}` right after the `WID_TN_SMALL_MAP` row of `_toolbar_button_sprites`.
- [ ] 3.6 Add `static_assert(std::size(_toolbar_button_procs) == WID_TN_END)` and `static_assert(std::size(_menu_clicked_procs) == WID_TN_SWITCH_BAR)` with a comment explaining the parallel-array contract, then build.

### Group 4 - Toolbar arrangements

{{MEDIUM}}
- [ ] 4.1 Insert `WID_TN_MAP_LAYER` directly after `WID_TN_SMALL_MAP` in `arrange_all`.
- [ ] 4.2 In each of `arrange14` to `arrange20`, insert `WID_TN_MAP_LAYER` directly after every `WID_TN_SMALL_MAP` entry.
- [ ] 4.3 Restore exactly N entries per row in each split array by removing one entry from the affected row, choosing in this order: a button that appears in both rows (never `WID_TN_SWITCH_BAR`), then `WID_TN_SUBSIDIES`, then `WID_TN_LEAGUE`, `WID_TN_STORY`, `WID_TN_GOAL`; record each removal as a comment on the row.
- [ ] 4.4 Add a `static_assert` per split array that its length is `2 * N`, so a future insertion cannot unbalance the rows.
- [ ] 4.5 Build, start the game, and resize the window through every split width (14 to 20 buttons) plus full width; confirm the layer button always sits immediately right of the map button and no row overflows.

### Group 5 - Button behaviour, icon and hotkey

{{MEDIUM}}
- [ ] 5.1 Implement `static CallBackFunction ToolbarMapLayerClick(Window *w)` with a Javadoc block: call `CycleMainViewportLayer()`, then `w->HandleButtonClick` with `WID_TE_MAP_LAYER` in the editor and `WID_TN_MAP_LAYER` otherwise, play `SndClickBeep()`, return `CallBackFunction::None`.
- [ ] 5.2 In `MainToolbarWindow::OnInvalidateData`, set the button sprite with `this->GetWidget<NWidgetCore>(WID_TN_MAP_LAYER)->SetSprite(GetMapLayerIcon(GetMainWindow()->viewport->layer))` and mark the widget dirty; set it once in the constructor as well.
- [ ] 5.3 Override `MainToolbarWindow::OnTooltip` for `WID_TN_MAP_LAYER` only, showing a new `STR_TOOLBAR_TOOLTIP_MAP_LAYER_CURRENT :{BLACK}Switch the main view between the surface, underground and space maps. Now showing: {STRING}` with the current layer name; return `false` for every other widget.
- [ ] 5.4 Append `MTHK_CYCLE_MAP_LAYER` to `MainToolbarHotkeys`, handle it in `OnHotkey` by calling `ToolbarMapLayerClick(this)`, and add `Hotkey(0, "cycle_map_layer", MTHK_CYCLE_MAP_LAYER)`; if a free single key exists after grepping every `HotkeyList` in `src/`, bind it and record the choice here.
- [ ] 5.5 Build, start a new game, click the button three times and use the hotkey; confirm the icon and tooltip go Surface, Underground, Space, Surface and that nothing else on screen changes (interim behaviour).

### Area Mandatory Reading - Scenario editor toolbar

_Applies to Group 6._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/widgets/toolbar_widget.h](../../../../src/widgets/toolbar_widget.h) | 51-76 | `enum ToolbarEditorWidgets`: `WID_TE_MAP_LAYER` is inserted after `WID_TE_SMALL_MAP`, shifting the editor arrays. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1845-1960 | `NWidgetScenarioToolbarContainer::GetButtonArrangement`: `arrange_all`, `arrange_nopanel`, `arrange_switch` (two halves). The panel-width arithmetic counts entries, so every array gains exactly one entry. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2279-2360 | `_scen_toolbar_dropdown_procs`, `_scen_toolbar_button_procs`, `enum class MainToolbarEditorHotkeys`. The hotkey note requires button-like hotkeys to equal their widget value. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2540-2605 | Editor `hotkeys`, `_nested_toolb_scen_inner_widgets`, `MakeScenarioToolbar`: the editor builds its buttons from an explicit part list, with explicit tooltips. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 418-433 | `# Extra tooltips for the scenario editor toolbar`: where the editor tooltip string is added. |

### Group 6 - Scenario editor toolbar

{{SIMPLE}}
- [ ] 6.1 Insert `WID_TE_MAP_LAYER, ///< Cycle the map layer shown by the main viewport.` after `WID_TE_SMALL_MAP` in `ToolbarEditorWidgets`.
- [ ] 6.2 Insert `nullptr, // 9` after `MenuClickMap, // 8` in `_scen_toolbar_dropdown_procs`, renumber the comments, and insert `ToolbarMapLayerClick` after `ToolbarScenMapTownDir` in `_scen_toolbar_button_procs`.
- [ ] 6.3 Add `NWidget(WWT_PUSHIMGBTN, Colours::Grey, WID_TE_MAP_LAYER), SetSpriteTip(SPR_IMG_PLANTTREES, STR_SCENEDIT_TOOLBAR_CYCLE_MAP_LAYER_TOOLTIP),` directly after the `WID_TE_SMALL_MAP` part, before the following spacer, and add the tooltip string.
- [ ] 6.4 Insert `WID_TE_MAP_LAYER` after every `WID_TE_SMALL_MAP` entry in `arrange_all`, `arrange_nopanel` and both halves of `arrange_switch`.
- [ ] 6.5 Add `CycleMapLayer = WID_TE_MAP_LAYER` to `MainToolbarEditorHotkeys`, handle it in the editor window's hotkey dispatch, and add `Hotkey(0, "cycle_map_layer", MainToolbarEditorHotkeys::CycleMapLayer)`.
- [ ] 6.6 Refresh the editor button sprite from `ScenarioEditorToolbarWindow::OnInvalidateData`, mirroring task 5.2.

### Area Mandatory Reading - Verification

_Applies to Group 7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/test_window_desc.cpp](../../../../src/tests/test_window_desc.cpp) | 79-99 | `WindowDesc - NWidgetParts properly closed` and `WindowDesc - NWidgetPart validity`: the unit tests that catch a malformed editor part list after task 6.3. |
| [src/map_layer_gui.cpp](../../../../src/map_layer_gui.cpp) | Live definitions; no fixed range | Produced by Group 2. `CycleMainViewportLayer` and `SetViewportLayer` are the authoritative behaviour being verified; the interim body must still be in place. |

### Group 7 - Verification

{{SIMPLE}}
- [ ] 7.1 Build RelWithDebInfo and Debug with no new warnings compared with the baseline from master task 1.3.
- [ ] 7.2 Run `ctest`; all baseline tests pass, including the `WindowDesc` tests.
- [ ] 7.3 In a Debug build, cycle the layer in a new game and in the scenario editor; confirm no assert fires and the button stays next to the map button.
- [ ] 7.4 Save and reload a game while viewing Underground; confirm the game loads and the view starts on Surface (the layer is not saved, AD-7).
- [ ] 7.5 Grep `src/` for `->layer` and `.layer` uses added by this plan; confirm none is reachable from command or tick code (I-4).

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| The toolbar shows a wrong tooltip on any button after task 3.2 | [docs/eints.md](../../../../docs/eints.md) | `### I want to add a new string.` (lines 70-73) | A translator tool or CI check complains about the `###length` block | Confirm that `###length` is a translation-tool marker and that translations must not be edited; the offset arithmetic in `MakeMainToolbar` stays the runtime authority. |
| A split toolbar row overflows or a button disappears in task 4.5 | [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | `NWidgetToolbarContainer::SetupSmallestSize` and `AssignSizePosition` | The overflow persists with balanced arrays | Understand how `arrangeable_count` and spacers drive layout. |
| `SetSprite` on the nested widget does not repaint | [docs/zz-reach2/architecture/gui/archi-gui.md](../../architecture/gui/archi-gui.md) | `## Widget tables` (lines 32-50) | The widget stays stale after `SetWidgetDirty` | Check how nested widgets store sprite data and dirty state. |
| A hotkey key choice collides with an existing binding | [src/hotkeys.cpp](../../../../src/hotkeys.cpp) | `HotkeyList` registration | Two lists claim the same key in one window context | Decide whether the key is safe; leave the default unbound if unsure. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 425-480, 1340-1373, 1522-1842, 1964-2261, 2279-2605 | Showed the four parallel arrays and two arrangement sets that every new toolbar button must touch, which set the Group 3 to Group 6 split. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 384-433 | The offset-indexed tooltip block and `###length` rule behind task 3.2. |
| [src/toolbar_gui.h](../../../../src/toolbar_gui.h) | 16-57 | Hotkey enum layout for task 5.4. |
| [docs/zz-reach2/architecture/gui/archi-gui.md](../../architecture/gui/archi-gui.md) | 52-138 | Event-hook surface (`OnClick`, `OnInvalidateData`, `OnTooltip`) used to place the icon refresh outside `OnPaint`. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 1248-1272, 1533 | Existing GUI sprites chosen as stand-in icons. |

## Completion Conditions

- `MapLayer` and `MAX_MAP_LAYERS` exist in `src/map_layer_type.h` exactly as AD-1 states.
- `Viewport::layer` exists, defaults to Surface, and is never read by command or tick code.
- The main toolbar and the scenario editor toolbar show the layer button immediately right of the map button at every toolbar width.
- Clicking the button or pressing its hotkey cycles the icon and tooltip through Surface, Underground and Space; the drawn map does not change yet.
- Both configurations build without new warnings and all baseline tests pass.
