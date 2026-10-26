# R2SFF1 shortcut implementation notes

Owner: [Space fixes plan, Group 3](r2sff1-space-fixes-features-1.md#group-3---deliver-the-promised-shortcuts-once).
Follow the nearest **Window layout and keyboard dispatch** mandatory reading
block and the global ownership/invariant list before resuming after compaction.
The [GUI architecture](../../architecture/gui/archi-gui.md#invariants) and
[map-layer architecture](../../architecture/map-layers/archi-map-layers.md#layer-dropdowns-and-view-state)
remain the owners of input routing and viewport coordinates.

## Delivered implementation

- `SelectMainViewportLayer` checks allocation before calling `SetViewportLayer`.
  Main/editor Alt+1..4 actions explicitly select Surface, Underground, Deep
  Underground and Space. Enum ordinals differ: Space is2 and Deep is3.
- Main/editor F2 calls `HandleWindowOpeningHotkey` in `window.cpp` before opening
  absent Game Options. The real identity is
  `GameOptionsWindowNumber::GameOptions` (4), **not zero**. Existing windows are
  raised without recreation, or closed only when front-most, unpinned and
  closable. Modal windows/progress retain control. Menu opening is unchanged.
- `gui.hotkey_toggles_window` defaults true and is client-only, outside saves
  and network sync. Its metadata, type, strings and General settings-tree entry
  are present. Escape behavior and other window-opening hotkeys are unchanged.
- Debug defaults move to Ctrl+Alt+0/1/2. `HotkeyList::Load` migrates only the
  corresponding exact single old global bindings; empty/custom/multiple entries
  are retained. The original user's files are never rewritten by these tests.
- Toolbar layer tooltips and the Space walkthrough name the delivered shortcuts.
  The walkthrough no longer promises automatic small-map following or the older
  plan's unfinished Escape window-close policy.

## Focused verification to run

`openttd_space_fixes_test` includes a pure `[space-shortcut-config]` case for
old, custom, multiple and empty bindings. Redirect its output to
`logs/space-fixes/shortcuts-config-<configuration>.log`.

The existing `space_fixes_checks.py` wrapper now accepts `--shortcuts` or
`--shortcuts-only`, plus `--hotkeys fresh|retained|custom`. Custom copies remap
Space to Alt+9 in both toolbars and Debug money to Ctrl+Shift+M. The native checks
exercise actual normal/editor input, plane-position preservation, independent
extra viewports, N cycling, covered/repeated F2, the setting off, pinned windows,
an actual Advanced-settings editbox, modal and non-closable window guards.

The original `regression/stationlist/test.sav` loads successfully but the live
loader expands it to four planes. The attempted one-layer assertion correctly
failed and does not qualify unavailable-layer behavior. `Map::Allocate` documents
that live games store all four planes while its one-plane default serves tests.
The focused `[space-shortcut-layers]` case now uses that existing allocation and
native toolbar fixture for one and three planes, including allocated Space with
unallocated Deep Underground. Run it from the build directory so it reads the
generated `lang/english.lng` through the established fixture.

Run fresh/retained/custom Space cases and the partial-allocation case separately, in both
RelWithDebInfo and Debug, with unique run IDs. Each native process has the
existing 110-second cap and all artifacts stay in `test-cache/space-fixes/` or
`logs/space-fixes/`. Rebuild the ordinary game targets with their matching
language packs after the focused checks pass. Adding the setting and string
IDs requires a shared engine rebuild; it is not a full test regression.

## Release verification completed

- `shortcuts-config-release.log`: 27 assertions pass for legacy/custom/multiple/
  empty bindings. `shortcuts-layers-release.log`: 12 assertions pass through
  actual editor input with one and three allocated planes.
- `shortcuts-release-fresh-2`, `shortcuts-release-retained` and
  `shortcuts-release-custom`: 61 assertions pass each, under three seconds per
  native process. They exercise both actual toolbar classes and the real Game
  Options window. The covering fixture uses the same native z-priority as Game
  Options; an ordinary extra viewport cannot cover this higher-priority window.
- `shortcuts-release-custom-final` repeats the native custom case after adding
  the allocation fixture and passes 61 assertions in 1.813s. Test executable:
  `c45ff1bf5f44ae390dd7363c0fbf4121f7c04ce9933e99ac0919efb2be582a4b`.
- `shortcuts-release-retained-final` passes 62 assertions in 2.047s after adding
  the explicit unchanged-money check; test executable SHA256
  `c36d5177a02e1ab877346b4ff32afe560cfcecb08f66937b9582ca8acde585b9`.

The ordinary release application rebuilt and paired its language packs; SHA256
`9206c93d7a208f37a78a1b70e41952ef94f32f797668c5a783fd32e65e949306`.
Initial harness compile/cover mistakes are retained in their earlier failed
logs; only the named passing runs qualify these results.

## Debug verification completed

`shortcuts-units-debug.log` passes 39 assertions across exact binding migration
and native partial-layer dispatch. `shortcuts-debug-fresh` and
`shortcuts-debug-custom` pass 62 assertions each, in 4.141s and 4.125s. These include
an explicit unchanged-company-money check after all native layer/window input.

`shortcuts-debug-retained-layout` passes 570 assertions in 7.563s, combining the
same input checks with a focused Group2 layout spot-check. Its baseline dispatch
reports Alt+4 reaching Space, no global Alt+1/2 matches, F2 first open and second
closed. It validates all five orbital pages and actual bottom-row mouse dispatch.
Debug test executable SHA256:
`04dc66cb56d10d03ebe072a0e1a1a79ef205854d6291455cd0b53de11c98a897`.

The ordinary Debug application rebuilt and paired its language packs; SHA256
`bde130b3f191f676db2894c39018dc3b6056a9c097b2c099fc87f3343f67776c`.
No full regression was run.
Do not repeat completed groups merely because a context compaction occurred.

## Exact overlap with the earlier demo-fix plan

For [r2df-demo-fixing-oct-15](r2df-demo-fixing-oct-15.md), this delivery satisfies:

- Group2.4's direct Alt+1..4 game/editor actions, allocation guard and tooltips.
- Group2.5's Debug default relocation and old-default migration, implemented in
  the existing loader. No active `build/hotkeys.cfg` existed to back up; immutable
  retained inputs and repository-local copies were used instead.
- The `hotkey_toggles_window` portion of3.1; its separate Escape setting is pending.
- F2's portion of3.4 and its corresponding3.6 coverage, including covered,
  pinned, protected and modal windows. Other opening shortcuts and Escape/TBM
  regressions remain pending in that plan.

Small-map sizing/following, Escape policy and unrelated Underground work were
not implemented here. Group34 owns the final cross-plan checkbox reconciliation;
these partial overlaps must not close whole older groups prematurely.
