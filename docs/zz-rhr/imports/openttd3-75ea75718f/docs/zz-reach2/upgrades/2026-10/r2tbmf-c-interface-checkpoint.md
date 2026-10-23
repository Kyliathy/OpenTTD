> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2tbmf-c-interface-checkpoint.md`; exact source SHA256 `d752c1ba83977e6aacaa87cf6851e98389f59da67546045230c22897f6be2caa`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2TBMF C - Compact construction interface checkpoint

Owner: [TBM fixing plan, Group2](r2tbmf-tbm-fixes.md#group-2---make-the-interface-compact-and-discoverable). Architecture: [GUI](../../architecture/gui/archi-gui.md). Related public contracts: [Plan7](r2mlvs7-underground-construction-and-fleet.md#public-handoff-contract) and [Plan13 native controls](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md). Verified2026-10-09; Group3 existing-machine planning is next.

## Implemented behavior

The palette now uses persistent placement buttons for Survey, New area and exchange-bay placement. Native mouse dispatch previously lowered momentary buttons before the placement helper ran, causing immediate cancellation. Projects now opens a real owned-project browser for the explicit rock layer. Its selected row resolves the actual train/controller/job again before actions; foreign, removed or missing owners do not retain actionable selection. The list shows vehicle identity, phase, completed cuts and current hold, with wrapped text, native resizing and scrolling. Selecting a footprint consumes the existing geological owner's accepted cells and sole progress cursor.

The default palette has one contextual instruction. Requirements/calendar, quoted materials/work/cash and selected-job progress only occupy space when applicable. New area requests an owned compatible portal, explains an existing project or missing cutter/origin, and leaves final admission with the original commissioning command. Locked Deep tools expose the calendar refusal even with optional guidance collapsed. Escape, right-click window close and changing an extra viewport's layer release placement. Selecting Projects deliberately clears an armed construction cursor.

The ordinary main toolbar, its dropdown/split layout and scenario editor entry use the packed native TBM control family. Existing enabled, pressed and disabled states remain driven by actual widget/company state. No new raster family, gameplay ID or persistent field was introduced.

## Actual evidence

| Check | Result |
| --- | --- |
| Optimized game and test build | [Final production build] (historical reference: `../../../../logs/r2tbmf-c-ui-build-3.log`); [final observer build] (historical reference: `../../../../logs/r2tbmf-c-ui-build-9.log`). |
| Active interface/commissioning/excavation checks | [1,048 assertions,15 cases] (historical reference: `../../../../logs/r2tbmf-c-ui-final-focused.log`), under one second; includes real new-project command flow, both viewport contexts, refusal, ownership and existing excavation workflow. |
| Ordinary application load and native mouse/render loop | [208 assertions, one case] (historical reference: `../../../../logs/r2tbmf-c-native-ui-reload.log`), approximately5.8 seconds. Includes real menus/buttons, actual train and footprint selection, cancellation, extra-view layer menu, scaled layouts, toolbar variants and protected-floor construction refusal. |
| Supplied-size default palette | [396x232 at1280x733/150%](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-palette.png), compared with the preserved [786x569 baseline](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-feedback-palette.png). Approximately79% less window area. |
| Small/high-scale expanded guidance | [264x200 at1024x600/100%](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-scale-100.png); [528x400 at1280x733/200%](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-scale-200.png). All controls remain reachable. |
| Projects | [Actual owned row](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-projects-click.png), [native train](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-project-machine.png), [selected footprint](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-project-selected.png). |
| Toolbar variants | [Pressed/menu](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-toolbar.png), [disabled spectator](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-toolbar-disabled.png), [split at200%](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-toolbar-split.png), [editor](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-toolbar-editor.png). |
| Protected construction | [Actual rail refusal](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-protected-support.png); no support or terrain guard bypass. |
| Running company label | [Actual game](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-ui-running.png) displays Underground Mines after native reload. |

The renderer uses loaded native content and its real text/widgets, not a mockup. Captures pump scheduled resize events between differing canvas sizes; input still goes through ordinary mouse hit testing and deferred dropdown timers. Baseline captures remain separate. The renderer and preview never advance excavation, purchase a machine or alter geology. The bounded observed20,000-tick cycle still records3,411 unblocked Bore frames, confirming that this interface group has not applied the later pacing change.

## Playable copy and provenance

`.\play.ps1 demo` now resolves to [underground-demo-tbm-ui-393.sav](../../../../../../../../src/tests/fixtures/haulage/planning/underground-demo-tbm-ui-393.sav). Launcher `-DryRun` verifies this path without launching or changing configuration. [Player guide](d2um-underground-mines.md) describes Projects, New area and the expanded requirements. The historical390 fixture's company had zero default-name fields and an empty custom name. The replacement was written after the normal RenameCompany command; a separate native-loader run verifies its retained custom name without renaming or rewriting it again.

- Historical original SHA256: `16ff76021341cf734b0e5ccc7b906abf90705481bbc2c031d6c01fd8eab61dbf`.
- Replacement393 SHA256: `fe54be254fde9957478dccfcec2407f045ae45747ec2bc1396dd71b68e4b1f38`.
- Optimized ordinary game SHA256: `702179415bfec7d8264eb3696d0dd3a98613a9f5a0939bb88261ff7aacea8fef`.
- Final native observer SHA256: `38639b2248c40499c6d93457aff9fab59f0daaf3175d1600c598eb4d97399304`.
- Preserved `build/play.cfg` SHA256: `18f5df22c65e4c0a79fe36cc5f26551c8a9607f8f9c432f340139dea4a94cf87`.
- Preserved `build/native-art.cfg` SHA256: `396092066483e9eddf9713618ceb44eb391d124677c0be5a4bf14d417a809862`.

[Runner](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/307a935b151996fa-r2tbmf-a-native-feedback.py) restores exact startup-script bytes and protects both saves/configuration during reload verification. The ordinary launcher keeps its English-US configuration; these native construction strings fall back to the English master because English-US defines no competing translations. The original screenshot's executable hash remains unknown.

## Explicit remaining work

This completes the interface group only. Existing-machine extension/reassignment, explicit corridor intent, missing-target recovery, slower cutting/half spoil, automatic-hauler advice, return/park/release, alternate hangars/exitB, geology variation and final integrated acceptance remain in subsequent fixing groups. The legacy vehicle window still says No orders during automatic work; this is retained in the running capture and belongs to Group6. Do not mark the complete fixing plan done or resume geothermal from this checkpoint.
