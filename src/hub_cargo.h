/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file hub_cargo.h Packet-owner transfers for timed launch handling, without synthetic cargo or payment.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native cargo custody.
 */
#ifndef HUB_CARGO_H
#define HUB_CARGO_H
#include "cargopacket.h"

/** Native packet/list transaction boundary; no independent cargo inventory, dispatch history or wallet is introduced. */
class HubCargoTransfer {
public:
	/** Preflight whole source removal, one possible split and aggregate legacy first-departure rights.
	 * @param source Actual native source queue or job list.
	 * @param quantity Whole units to move, positive and within waiting stock.
	 * @return Complete admission without packet/cache/counter/RNG mutation.
	 */
	static bool CanMove(const StationCargoList &source, uint quantity);
	/** Move a whole admitted quantity between existing native owners.
	 * @param source Actual source list; first departure retires legacy refund eligibility once.
	 * @param target Distinct actual destination list, never an inferred owner.
	 * @param quantity Whole units preserving routing, purpose, age and feeder share.
	 * @return Complete movement or unchanged preflight refusal; no finance or replacement stock.
	 */
	static bool Move(StationCargoList &source, StationCargoList &target, uint quantity);
	/** Age real retained stock without simulation RNG. @param cargo Actual native packet list whose age cache is updated. */
	static void Age(StationCargoList &cargo);
	/** Consume deterministic recipe inputs without creating stock, payment or replacement refund rights.
	 * @param source Actual native material owner.
	 * @param quantity Complete positive bill within its waiting stock.
	 * @return Complete removal or unchanged quantity refusal.
	 */
	static bool Consume(StationCargoList &source, uint quantity);
	/** Preflight actual retained vehicle freight, excluding reserved/delivery ranges and forged unshipped legacy units.
	 * @param source Actual carrier list, all in its Keep range.
	 * @param quantity Whole positive amount, with at most one admitted split.
	 * @return Complete nonmutating admission, including existing legacy references.
	 */
	static bool CanMove(const VehicleCargoList &source, uint quantity);
	/** Move actual station/job stock into native Keep custody.
	 * @param source Actual source stock; legacy first departure is recorded once.
	 * @param target Actual carrier with no mixed action ranges.
	 * @param quantity Whole preflighted amount.
	 * @param tile Physical loading tile used by native distance accounting.
	 * @return Complete movement or unchanged admission refusal, without payment.
	 */
	static bool Move(StationCargoList &source, VehicleCargoList &target, uint quantity, TileIndex tile);
	/** Unload actual Keep freight without reopening legacy source eligibility.
	 * @param source Actual carrier list.
	 * @param target Actual station/job list.
	 * @param quantity Whole preflighted amount.
	 * @param tile Physical native unloading tile.
	 * @return Complete movement or unchanged admission refusal, preserving departure flags.
	 */
	static bool Move(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex tile);
	/** Release actual Space freight and rebase its native travel vector for Surface descent.
	 * @param source Actual Keep-only carrier stock.
	 * @param target Empty actual descent owner.
	 * @param quantity Whole preflighted amount.
	 * @param release_tile Actual Space release position, never a view-selected layer.
	 * @return Complete release or unchanged admission refusal; layer offsets create no freight income.
	 */
	static bool ReleaseOrbital(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex release_tile);
};
#endif
