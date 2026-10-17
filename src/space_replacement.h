/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_replacement.h Paid replacement admission and prepared native old/new settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 * Existing manufacturing jobs and Earth assembly hulls remain the physical producers; replacement intent creates no second inventory, hull pool or finance ledger.
 */
#ifndef SPACE_REPLACEMENT_H
#define SPACE_REPLACEMENT_H
#include "space_production.h"
#include "space_orders.h"
#include "command_type.h"

/** Detached native structure transport; replacement link validation never resolves its fields against another live world. */
struct SpaceStructureRecord;

/** Actual physical handover site; exactly one orbital factory/inventory pair or one Surface center identifies the existing producer. */
struct SpaceReplacementSite {
	SpaceStructureRef factory; ///< Current original paid Assembly incarnation, empty for Earth work.
	SpaceStructureRef inventory; ///< Its actual adjacent owned input/service berth, empty for Earth work.
	LaunchCenterRef center; ///< Current original Earth workshop/storage owner, empty for orbital work.
};

/** Prepared independent storage and exact old incarnation for one synchronous native settlement; this grants no new identity or sale credit. */
struct PreparedSpaceReplacement {
	SpaceCraftRef source; ///< Actual old hull that must still survive with the prepared rights until commit.
	std::string name; ///< Original custom name storage prepared before native hull/material mutations.
	SpaceOrderQueue orders; ///< Independent instructions/current index, without an old physical approach or active partial work.
	CargoType cargo = INVALID_CARGO; ///< Actual retained freight label, or the target's canonical default when no freight is held.
	uint16_t capacity = 0; ///< Full target-role physical capacity admitted before any old packet moves.
	uint32_t sale_credit = 0; ///< Actual depreciated native value observed during final preflight, never projected admission income.
};

/** Check immutable native role compatibility before configuring a company replacement rule; actual holdings/rights remain final handover checks.
 * @param source_model Original canonical paid role.
 * @param target_model Requested canonical native destination role.
 * @return Whether both have delivered producers and rocket transition history remains within its original role.
 */
bool AreSpaceReplacementModelsCompatible(uint16_t source_model, uint16_t target_model);

/** Find whether an original incarnation already has an unfinished actual paid replacement, without resolving a reused numeric source ID.
 * @param source Exact original source incarnation.
 * @return Whether any original manufacturing job or actual Earth assembly hull retains that pending request.
 */
bool HasPendingSpaceReplacement(SpaceCraftRef source);

/** Validate detached historical source/retirement bounds against the same snapshot without resolving open-world pools.
 * @param intent Original source incarnation/model and optional final settlement receipt.
 * @param target_model Actual paid new recipe role from the containing producer.
 * @param next_vehicle Same-file next craft generation; a lost original source remains bounded historical intent.
 * @param product Actual already issued Earth/completed orbital product, or empty before orbital materialization.
 * @return Whether bounded source/model/credit fields and relative issuance could belong to one native replacement.
 */
bool ValidateSpaceReplacementIntent(const SpaceReplacementIntent &intent, uint16_t target_model, uint64_t next_vehicle, SpaceCraftRef product = {});

/** Validate contained replacement ownership using only one detached snapshot, including Earth receipts and orbital jobs.
 * @param structures Same-file original structures and contained manufacturing jobs.
 * @param products Same-file existing native hull receipts.
 * @param next_vehicle Same-file next craft generation, including lost historical sources.
 * @return Whether each original incarnation has one bounded producer and completed retirement cannot leave that original alive.
 */
bool ValidateSpaceReplacementLinks(std::span<const SpaceStructureRecord> structures, std::span<const SpaceProductionRecord> products, uint64_t next_vehicle);

/** Quote/admit an actual native replacement selected by existing company/group renewal policy.
 * @param flags Original synchronized request flags; QueryCost remains pure even together with Execute.
 * @param source Actual original paid owned hull; it and every asset remain intact while a new producer works.
 * @param engine Existing canonical target Engine from native replacement/renewal policy, never a speculative definition.
 * @return Fresh full physical capital cost or unchanged refusal; original sale proceeds belong only to final settlement.
 */
CommandCost ReplaceSpaceVehicle(DoCommandFlags flags, SpaceVehicle &source, EngineID engine);

/** Prepare safe current source rights, physical handover and all configuration storage before a new product/material commit.
 * @param intent Original pending replacement, never a completed receipt.
 * @param target_model Actual new paid recipe role.
 * @param owner Current actual factory/Earth product owner, including legitimate whole-company takeover.
 * @param site Actual producer and its owned physical handover berth.
 * @return Prepared independent storage/value, or no result while old identity, holdings, orders or physical rights are unsafe.
 */
std::optional<PreparedSpaceReplacement> PrepareSpaceReplacement(const SpaceReplacementIntent &intent, uint16_t target_model, CompanyID owner, const SpaceReplacementSite &site);

/** Transfer preflighted old configuration/assets and retire/account the actual original once through native owners; a completed receipt makes repeats inert.
 * @param intent Actual contained producer's pending receipt; marked settled only by this synchronous commit.
 * @param product Actual freshly paid new hull, never an estimate or proxy.
 * @param prepared Independent storage and exact original incarnation prepared before materialization.
 */
void CommitSpaceReplacement(SpaceReplacementIntent &intent, SpaceVehicle &product, PreparedSpaceReplacement &&prepared);

/** Recognize only the exact old hull inside the private prepared retirement scope used by native replacement settlement.
 * @param source Actual native hull considered by the existing sell/destruction command.
 * @return Whether this synchronous prepared handover may retire the stopped emptied hull away from its old home berth.
 */
bool IsSpaceReplacementRetirement(const SpaceVehicle &source);

/** Offer one genuine completed Visit arrival to the existing native automatic replacement queue.
 * @param source Actual arrived old hull; a pure native policy/facility quote must succeed before queuing.
 */
void QueueSpaceReplacementOnArrival(SpaceVehicle &source);

/** Retain a later explicit start/stop, order-edit or mission running choice in the actual pending orbital producer.
 * @param source Original incarnation whose actual native running-state command just executed.
 * @param resume Requested running state; Earth products remain stopped and have no orbital resume intent.
 */
void SetPendingSpaceReplacementResume(SpaceCraftRef source, bool resume);

/** Release a cancelled producer's running request only when its exact original is still safe at that physical handover.
 * @param intent Actual cancelled pending job, never a completed retirement receipt.
 * @param target_model Original new recipe against which every current whole holding was admitted.
 * @param owner Current actual physical producer owner.
 * @param site Actual factory/input or Earth center handover.
 */
void RestoreCancelledSpaceReplacement(const SpaceReplacementIntent &intent, uint16_t target_model, CompanyID owner, const SpaceReplacementSite &site);

/** Detach pending Earth replacement intent from an existing paid assembly body; its materials/work and the original hull survive as independent owned assets.
 * @param flags Quote or execute; QueryCost always remains pure.
 * @param product Actual paid new assembly body index.
 * @param generation Exact product incarnation, never numerical pool-slot reuse.
 * @return Successful zero-refund detachment or unchanged refusal; no unconsumed orbital packet owner is involved.
 */
CommandCost CmdCancelEarthSpaceReplacement(DoCommandFlags flags, VehicleID product, uint64_t generation);

#endif
