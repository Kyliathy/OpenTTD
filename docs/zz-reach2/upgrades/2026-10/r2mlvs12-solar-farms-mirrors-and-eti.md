# R2MLVS12 - Solar Farms, Mirrors and Energy Transition Industries

Status: proposed implementation; initial source investigation completed on 2026-10-04. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Group 1 uses draft definitions from r2mlvs1 Groups 1/2 and feeds its Group 3 closure; orbital mirrors depend on r2mlvs9. No third-party content was installed, copied into the game or executed during planning.

## Outcome and reading

Surface solar farms receive useful additional power from launched orbital mirrors. Each supported farm has at most one +300% bonus, for 4x baseline output. Read master AD-1/13 and I-3/5/11; [industries](../../architecture/industries/archi-industries.md), [NewGRF](../../architecture/newgrf/archi-newgrf.md), [scripting](../../architecture/scripting/archi-scripting.md), industry production/accounting, `src/script/api/script_industry.cpp` and `src/newgrf_industries.cpp`.

## Investigation record

Upstream is Nathan Pitman's [Open-TTD-Renewable-Energy-Mod](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod). Inspected revision: `09da329c29e78eeb1f26aab6b8e6492e7c06daf7`, dated 2026-09-27; its `VERSION` file and source identify version 14. Use this revision for the feasibility fixture, then deliberately recheck upstream before any final dependency pin.

| Verified observation | Planning consequence |
| --- | --- |
| ETI supplies seven new industries/two cargos and a companion Game Script; its current README warns of likely FIRS conflicts. | It is a whole economy integration, not just a solar sprite pack. Test a declared content combination. [Pinned README](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/README.md) |
| NML identifies GRFID `RE01`, solar local industry ID 5 and `POWR` output at base rate 6; `POWR` and `URAN` use slots/bits 32 and 33. Solar layout is 3x2, flat-location checked, default year 1990. The worker mechanic is removed. | Match supported identity/version, reserve cargo compatibility and test actual production. Avoid an adapter based on translated names or the old worker description. [Pinned NML](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition.nml) |
| GS finds `POWR` by label, identifies producers/acceptors, reads last-month production near towns and sets growth; its loop sleeps in ticks. | Extra solar production must appear in normal monthly statistics. Review layer filters, time units and sole authority over growth. [Pinned script](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition_gs/main.nut) |
| GS metadata is version 14, dated 2026-09-27, requesting API version 13. | Verify startup and API compatibility in this fork; matching metadata is not a runtime pass. [Pinned metadata](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition_gs/info.nut) |
| No license file/declaration was found in the inspected repository tree; GitHub's repository API returned no detected license. | Redistribution/modification permission is unresolved. Do not assume the game's GPL covers those files. [Inspected tree](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/tree/09da329c29e78eeb1f26aab6b8e6492e7c06daf7) |

The author's [initial announcement](https://nathanpitman.com/posts/energy-economy-in-open-transport-tycoon-deluxe/) and a cached repository page describe workers; the pinned current README/source remove that mechanic. The source revision above is the planning baseline. Public source availability and an advertised feature are not proof of license terms, local runtime compatibility or final-quality graphics.

**Recommendation:** supply an original solar/power implementation with the Multilayer Industry profile, and add an explicit ETI compatibility profile if the fixture passes. Bundling an ETI derivative requires established redistribution terms and a deliberate content decision. This preserves the requested solar gameplay regardless of upstream packaging. Upstream permission enquiries are future user-owned work; no author contact is authorized by this plan.

## Group 1 - Reproducible ETI feasibility decision

- [ ] 1.1 Reproduce the pinned source/content metadata in a repository-local fixture with content hashes and no user-wide installation. Verify the available distribution terms and record which files can be reused; otherwise keep the original-art/content route.
- [ ] 1.2 Run ETI alone in this fork with its GS. Verify cargo labels, solar placement/output, nuclear Uranium acceptance, monthly history, startup errors and save/reload. Source inspection above does not complete this runtime task.
- [ ] 1.3 Test the intended Multilayer cargo profile against slots 32/33, labels, industry identities and load order. Resolve duplicate Uranium definitions explicitly; reject unsupported replacement sets without corrupting an existing save.
- [ ] 1.4 Audit `POWR` handling: grid credit versus transported acceptance, payment and refits. Define whether it is nontransported utility in this profile and prevent duplicate rewards without modifying unrelated GRFs by label alone.
- [ ] 1.5 Test a Surface-only generator filter and script timing against this fork's economy/calendar modes. Select one town-growth authority; arbitrary simultaneous GS growth control is not promised.
- [ ] 1.6 Record the delivery decision: original native profile, supported separately installed ETI profile, or licensed bundled adaptation. Freeze compatible revision/hash, scope and failure message. A fallback is a completed decision, not a claim ETI passed.

## Group 2 - Solar industry and useful power

- [ ] 2.1 Add the original Surface solar farm and power consumer/grid if ETI is not the selected bundled route. Supply final original sprites and a real consumer effect as defined in r2mlvs1; do not merely increment a decorative number.
- [ ] 2.2 Expose a capability for mirror-eligible solar generation. For ETI, resolve GRFID/local type/version through its mapping; for native definitions use a stable capability flag. Never identify solar farms by name or treat every POWR producer as solar.
- [ ] 2.3 Define baseline production and a single augmentation point before authoritative produced/monthly totals. Check rate-based and callback-based production and the current narrow production/history fields; use sufficient intermediate/storage width or a documented supported bound so 4x cannot wrap or silently disappear. Do not add bonus twice or alter consumed inputs/other cargos.
- [ ] 2.4 Record original/native and ETI compatibility statistics consistently, including partial operational periods. A stopped/depleted/zero-baseline facility gets no bonus.
- [ ] 2.5 Integrate r2mlvs17's geothermal electricity with the same useful-power authority through an explicit Surface export adapter when needed. Count exported energy once, exclude Deep generators from Surface proximity scans and ensure geothermal/heat consumers never inherit mirror eligibility merely by producing or consuming Power.

## Group 3 - Orbital mirror lifecycle

- [ ] 3.1 Deliver Solar Mirror Kits by rocket and deploy a company-owned mirror through Space Construction. Consume the kit once and persist deployment progress, service cost and target reference. No station is required for the first mirror.
- [ ] 3.2 Define a saved coverage range in plane coordinates and a target assignment command. Initial policy: one operational assignment per farm; public industries remain public, but another company cannot take over an occupied mirror assignment. Show conflicts and invalid range before charging.
- [ ] 3.3 Apply `effective = baseline + 3 * baseline` during operational coverage, capped at 4x across all mirrors. Use integer remainder accounting for partial ticks/periods and display baseline/bonus/effective power.
- [ ] 3.4 Stop the bonus immediately on loss of service, removal or invalid target. Clear backlinks on industry deletion so ID reuse cannot boost a replacement industry accidentally.
- [ ] 3.5 Add mirror deployment/aiming/idle and boosted solar-farm graphics, selection link and useful-power presentation. Optical beams are presentation only.

## Group 4 - Acceptance

- [ ] 4.1 Compare identical no-mirror/mirror fixtures at the same base production and duration: 100 normal units become 400 total; two mirrors still yield at most 400; zero remains zero. Test partial coverage without rounding drift.
- [ ] 4.2 Verify effective power reaches actual consumer/grid behavior and the ETI monthly statistic path when that profile is enabled. Town-growth changes occur only under the selected authority.
- [ ] 4.3 Test deployment cargo use, out-of-range targets, farm deletion/ID reuse, mirror sale/bankruptcy, unsupported GRFs and blocked service. Save/reload while deploying and with a fractional production remainder.
- [ ] 4.4 Run the full Surface kit -> rocket -> mirror -> solar bonus chain with visible final graphics and deterministic replay.
- [ ] 4.5 Test solar and exported geothermal production together: solar alone receives the +300% mirror bonus, thermal allocations remain unchanged, and native/ETI modes cannot credit both a Deep generator and its Surface export as separate supplies.

## Completion conditions

The selected content route is documented and reproducible. A launched, maintained mirror measurably increases a real solar farm's useful output by exactly 300%, without stacking, duplicate production/payments or stale target references. ETI source findings, compatibility results and redistribution status are reported separately.
