/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_clone.cpp Ordinary clone intent through the existing material-paid physical purchase owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
 */
#include "stdafx.h"
#include "space_clone.h"
#include "space_vehicle.h"
#include "space_structure.h"
#include "launch_operations.h"
#include "company_func.h"
#include "engine_base.h"
#include "station_base.h"
#include "vehicle_func.h"
#include "string_func.h"
#include "safeguards.h"

/** @copydoc ValidateSpaceCloneIntent */
bool ValidateSpaceCloneIntent(const SpaceCloneIntent &intent, uint16_t model, uint64_t next_vehicle, uint64_t next_structure)
{
	// Only an immutable profile role with an actual native commissioning path may supply clone configuration; passive pods and module recipes are not vehicle Engines.
	const bool native_model = CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::OrbitalAssembly, true) ||
		CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::LaunchCenter, true);
	if (!native_model) return false;
	// Captured source issuance and bounded legal text must precede all decoding-dependent traversal; an absent/future identity cannot be promoted into a paid source.
	if (intent.source.vehicle >= VehicleID::End() || intent.source.generation == 0 || intent.source.generation >= next_vehicle ||
		intent.name.size() > MAX_SPACE_CLONE_NAME_BYTES || intent.name.find('\0') != std::string::npos ||
		!StrValid(std::span{intent.name.c_str(), intent.name.size() + 1}) || Utf8StringLength(intent.name) >= MAX_LENGTH_VEHICLE_NAME_CHARS ||
		intent.orders.size() > MAX_SPACE_ORDERS) return false;
	// Each immutable instruction uses the existing role/domain policy and already issued target epochs; no current-world lookup can validate a copied preview.
	for (const auto &order : intent.orders) {
		if (!IsSpaceOrderAllowed(model, order)) return false;
		if (order.target.kind == SpaceOrderTargetKind::Craft && order.target.generation >= next_vehicle) return false;
		// Static targets share their original incarnation domain, including retained removed blueprints; future IDs cannot acquire a later module.
		if ((order.target.kind == SpaceOrderTargetKind::Structure || order.target.kind == SpaceOrderTargetKind::Blueprint) && order.target.generation >= next_structure) return false;
	}
	return true;
}

/** @copydoc CanApplySpaceCloneIntent */
bool CanApplySpaceCloneIntent(const SpaceCloneIntent &intent, CompanyID owner)
{
	// Only the saved target instructions are consulted: source deletion, rename, transfer or pool reuse cannot change captured configuration.
	for (const auto &order : intent.orders) {
		// Structure and Blueprint navigation share the original static pool; each surviving target must still offer this product owner's rights.
		if (order.target.kind == SpaceOrderTargetKind::Structure || order.target.kind == SpaceOrderTargetKind::Blueprint) {
			const auto *target = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(order.target.id)}, order.target.generation});
			// Removed/unowned targets can remain inert history, but a live foreign facility must not authorize the new product's queue.
			if (target != nullptr && target->state.owner != OWNER_NONE && target->state.owner != owner) return false;
		} else if (order.target.kind == SpaceOrderTargetKind::Craft) {
			const auto *target = ResolveSpaceCraft({VehicleID{order.target.id}, order.target.generation});
			if (target != nullptr && target->owner != owner) return false; // A surviving target retains its own fleet's delivery rights.
		} else if (order.target.kind == SpaceOrderTargetKind::LaunchCenter) {
			const auto *target = ResolveLaunchCenter({StationID{static_cast<uint16_t>(order.target.id)}, order.target.generation});
			if (target != nullptr && target->owner != owner) return false; // A new clone receives no access to another company's Earth service center.
		}
	}
	return true;
}

/** @copydoc PrepareSpaceCloneProductConfiguration */
std::optional<SpaceCloneProductConfiguration> PrepareSpaceCloneProductConfiguration(const SpaceCloneIntent &intent, CompanyID owner)
{
	if (!CanApplySpaceCloneIntent(intent, owner)) return std::nullopt;
	return SpaceCloneProductConfiguration{MakeClonedVehicleName(intent.name), intent.orders};
}

/** @copydoc ApplySpaceCloneProductConfiguration */
void ApplySpaceCloneProductConfiguration(SpaceVehicle &product, SpaceCloneProductConfiguration &&configuration) noexcept
{
	assert(product.space_orders.entries.empty());
	product.name = std::move(configuration.name);
	product.space_orders.entries = std::move(configuration.orders);
	product.vehstatus.Set(VehState::Stopped); // A new clone waits for an explicit start; original action progress/leases and active service remain solely with its source.
}

/** @copydoc CloneSpaceVehicleAtFacility */
SpacePurchaseResult CloneSpaceVehicleAtFacility(DoCommandFlags flags, TileIndex tile, const SpaceVehicle &source, bool share_orders)
{
	// Shared queues and refit subtypes have no native Space producer. A source's current freight label remains its actual cargo owner's state; the independent new hull commissions its canonical default.
	if (share_orders || source.owner != _current_company || source.vehstatus.Test(VehState::Crashed) || !IsSpacePurchaseEngine(source.engine_type) ||
		source.cargo_type >= NUM_CARGO || source.cargo_subtype != 0) return {};
	const auto model = GetSpaceVehicleModel(source);
	const auto *recipe = GetSpaceVehicleConstruction(model);
	// Only an actual original material-paid source may request a new paid product; legacy demonstration hulls do not supply commissioning authority.
	if (recipe == nullptr || !IsSupportedSpaceVehicleContentVersion(source.construction.content_version, model) || source.construction.model != model || source.construction.capital_paid != recipe->capital) return {};
	SpaceCloneIntent intent{GetSpaceCraftReference(source), source.name, source.space_orders.entries};
	// Historical source/target references must be issued already and belong to this actor's current instruction domain before a new financial request is admitted.
	if (!ValidateSpaceCloneIntent(intent, model, GetNextSpaceVehicleGeneration(), GetNextSpaceStructureGeneration()) || !CanApplySpaceCloneIntent(intent, source.owner)) return {};
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	auto quote = PurchaseSpaceVehicleAtFacility({}, tile, source.engine_type, INVALID_CARGO);
	if (quote.cost.Failed()) return quote;
	const bool orbital = quote.factory != SpaceStructureRef{};
	// Existing native validation permits instructions only on deployed orbital hulls. Surface assembly can clone an empty queue without inventing another deferred-order owner.
	if (!orbital && !intent.orders.empty()) return {};
	if (!flags.Test(DoCommandFlag::Execute)) return quote;
	std::optional<SpaceCloneProductConfiguration> configuration;
	if (!orbital) configuration = PrepareSpaceCloneProductConfiguration(intent, source.owner);
	if (!orbital && !configuration) return {}; // Every immediate Earth configuration must be prepared before its paid recipe or real hull changes.
	auto result = PurchaseSpaceVehicleAtFacility(flags, tile, source.engine_type, INVALID_CARGO);
	if (result.cost.Failed()) return result;
	if (orbital) {
		auto *factory = ResolveSpaceStructure(result.factory);
		assert(factory != nullptr && factory->module && factory->module->manufacture && factory->module->manufacture->serial == result.serial);
		factory->module->manufacture->clone.emplace(std::move(intent));
	} else {
		assert(configuration.has_value());
		ApplySpaceCloneProductConfiguration(*SpaceVehicle::Get(result.vehicle), std::move(*configuration));
	}
	return result;
}
