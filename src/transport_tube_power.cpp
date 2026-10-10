/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_power.cpp Physical paid tube connections and atomic invoices, using the sole native electrical ledger.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group5
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "multilayer_bootstrap.h"
#include "company_base.h"
#include "company_func.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "rail_map.h"
#include "station_map.h"
#include "core/backup_type.hpp"
#include "timer/timer_game_economy.h"
#include <set>

#include "safeguards.h"

static std::vector<TubePowerConnection> _tube_power; ///< Physical/invoice owner only; no duplicate joules, grants or generation cursor.
static std::optional<std::vector<TubePowerConnection>> _pending_tube_power; ///< Unaccepted file transport, inaccessible to live power queries.
static bool _tube_power_committing = false; ///< Complete physical participants cannot reenter connection creation or invoice mutation.

/** @copydoc IsTubeExternalPowerCommitting */
bool IsTubeExternalPowerCommitting() { return _tube_power_committing; }

/** @param producer Actual ledger source. @return Mutable retained connection, or nullptr without allocating one. */
static TubePowerConnection *FindTubePowerConnection(UtilityProducerID producer)
{
	const auto found = std::ranges::find(_tube_power, producer, &TubePowerConnection::producer);
	return found == _tube_power.end() ? nullptr : &*found;
}

/** @copydoc GetTubePowerConnection */
const TubePowerConnection *GetTubePowerConnection(UtilityProducerID producer) { return FindTubePowerConnection(producer); }

/**
 * Validate the complete actual physical service before publication or use.
 * @param connection Explicit owned portal, interchangeable facility and excavated wire corridor.
 * @return Specific owner/date/topology refusal; no nearby building acquires implicit power.
 */
static TubePowerError CheckTubePowerTopology(const TubePowerConnection &connection)
{
	if (!Company::IsValidID(connection.owner)) return TubePowerError::InvalidOwner;
	const auto &terms = GetMultiLayerTubePowerContract();
	// Early service has its own calendar introduction; the shared construction guard
	// separately applies Deep's 1980 date and geological requirements to every wire cell.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{terms.introduction_year}, 0, 1)) return TubePowerError::Unavailable;
	if (connection.portal.base() >= Map::Size()) return TubePowerError::InvalidTopology;
	const auto pair = GetLayerPortalPairKey(connection.portal);
	if (!pair.has_value()) return TubePowerError::InvalidTopology;
	const auto *record = GetLayerPortalWorkRecord(connection.portal);
	// Both real approaches must be commissioned and accessible; an old revision,
	// sealed pair, foreign owner or road/elevator housing cannot bootstrap tubes.
	if (record == nullptr || record->state != LayerPortalWorkState::Connected || record->company != connection.owner ||
		record->revision != connection.portal_revision || GetLayerPortalTransportType(connection.portal) != TransportType::Rail ||
		CheckLayerPortalAvailability(connection.portal) != LayerPortalWorkError::None) return TubePowerError::InvalidTopology;
	if (connection.interchange.base() >= Map::Size()) return TubePowerError::InvalidTopology;
	const auto layer = GetTileLayer(connection.portal);
	// The explicit electrical corridor stays within one rock layer. Conventional
	// freight remains responsible for cargo's separate physical portal crossing.
	if ((layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) ||
		GetTileLayer(connection.interchange) != layer || GetTileOwner(connection.interchange) != connection.owner.base()) return TubePowerError::InvalidTopology;
	const bool facility = IsRailDepotTile(connection.interchange) || IsRailStationTile(connection.interchange);
	if (!facility) return TubePowerError::InvalidTopology;
	if (!IsTubeRailType(GetRailType(connection.interchange))) return TubePowerError::InvalidTopology;
	// Whole corridor admission requires actual excavated floor, not just a
	// matching coordinate or a valid terminal at the other end of solid ground.
	if (connection.utility_path.empty() || connection.utility_path.size() > MAX_TUBE_POWER_PATH ||
		connection.utility_path.front() != GetLayerPortalDoorTile(connection.portal) ||
		connection.utility_path.back() != connection.interchange) return TubePowerError::InvalidTopology;
	if (CheckTubeConstruction(connection.utility_path).Failed()) return TubePowerError::InvalidTopology;
	std::set<TileIndex> seen;
	TileIndex previous = INVALID_TILE;
	// A simple adjacent owned/available wire corridor is an explicit installed
	// connection. Repeated or disconnected cells cannot inflate its physical reach.
	for (const auto tile : connection.utility_path) {
		if (!seen.insert(tile).second) return TubePowerError::InvalidTopology;
		if (IsTileType(tile, TileType::TunnelBridge)) return TubePowerError::InvalidTopology;
		// Clear excavated floor has no company owner; occupied infrastructure must
		// belong to this service owner before its footprint can host a wire.
		if (!IsTileType(tile, TileType::Clear) && GetTileOwner(tile) != connection.owner.base()) return TubePowerError::InvalidTopology;
		if (previous != INVALID_TILE) {
			const uint distance = abs(static_cast<int>(TileX(tile)) - static_cast<int>(TileX(previous))) +
				abs(static_cast<int>(TileY(tile)) - static_cast<int>(TileY(previous)));
			if (distance != 1) return TubePowerError::InvalidTopology;
		}
		previous = tile;
	}
	return TubePowerError::None;
}

/** @copydoc ConnectTubeExternalPower */
TubePowerResult ConnectTubeExternalPower(TileIndex portal, TileIndex interchange, std::span<const TileIndex> path, bool execute)
{
	if (_tube_power_committing) return {TubePowerError::Refused};
	TubePowerConnection connection;
	connection.owner = _current_company; connection.portal = portal; connection.interchange = interchange;
	const auto *record = portal.base() < Map::Size() ? GetLayerPortalWorkRecord(portal) : nullptr;
	if (record == nullptr) return {TubePowerError::InvalidTopology};
	connection.portal_revision = record->revision;
	// Reject oversized caller input before copying any wire transport. Aggregate
	// path and retained connection bounds also limit lifecycle and save work.
	if (path.size() > MAX_TUBE_POWER_PATH || _tube_power.size() >= MAX_TUBE_POWER_CONNECTIONS) return {TubePowerError::Limit};
	size_t cells = path.size();
	for (const auto &existing : _tube_power) {
		cells += existing.utility_path.size();
		// One active installed inlet per interchange prevents repeated build
		// requests from stacking free capacity or daily contracts at one building.
		if (existing.active && existing.interchange == interchange) return {TubePowerError::InvalidTopology};
	}
	if (cells > MAX_TUBE_POWER_PATH_CELLS) return {TubePowerError::Limit};
	connection.utility_path.assign(path.begin(), path.end());
	const auto error = CheckTubePowerTopology(connection);
	if (error != TubePowerError::None) return {error};
	const auto &terms = GetMultiLayerTubePowerContract();
	if (Company::Get(connection.owner)->money < Money{terms.lease_per_day}) return {TubePowerError::Funds};
	const auto source = RegisterUtilityPowerProducer(connection.owner, GetTileLayer(portal), UtilityGenerationKind::External, false);
	if (!source.has_value()) return {TubePowerError::Ledger};
	connection.producer = *source; connection.lease_day = TimerGameEconomy::date.base();
	connection.invoiced_money = terms.lease_per_day;
	if (execute) {
		_tube_power.reserve(_tube_power.size() + 1); // Finish fallible physical storage acquisition before ledger identity publication.
		if (RegisterUtilityPowerProducer(connection.owner, GetTileLayer(portal), UtilityGenerationKind::External) != source) return {TubePowerError::Ledger};
		_tube_power.push_back(std::move(connection));
		SubtractMoneyFromCompany(_current_company, CommandCost(ExpensesType::Other, Money{terms.lease_per_day}));
	}
	return {TubePowerError::None, *source};
}

/** @copydoc PublishTubeExternalPower */
TubePowerError PublishTubeExternalPower(UtilityProducerID producer, uint64_t interval, uint32_t ticks, bool execute)
{
	if (_tube_power_committing) return TubePowerError::Refused;
	auto *connection = FindTubePowerConnection(producer);
	if (connection == nullptr) return TubePowerError::InvalidTopology;
	// A retained inactive or foreign service cannot publish energy, even when
	// its original IDs and invoice remain visible for recovery/history.
	if (!connection->active || connection->owner != _current_company) return TubePowerError::InvalidOwner;
	const auto error = CheckTubePowerTopology(*connection);
	if (error != TubePowerError::None) return error;
	const auto &terms = GetMultiLayerTubePowerContract();
	const uint16_t lease = connection->lease_day == TimerGameEconomy::date.base() ? 0 : terms.lease_per_day;
	if (Company::Get(connection->owner)->money < Money{lease}) return TubePowerError::Funds;
	if (ticks == 0) return TubePowerError::Refused;
	if (connection->invoiced_money > UINT64_MAX - lease) return TubePowerError::Limit;
	if (PublishUtilityGeneration(producer, interval, terms.capacity_w, ticks, false) != UtilityPowerError::None) return TubePowerError::Ledger;
	if (execute) {
		if (PublishUtilityGeneration(producer, interval, terms.capacity_w, ticks) != UtilityPowerError::None) return TubePowerError::Ledger;
		SubtractMoneyFromCompany(connection->owner, CommandCost(ExpensesType::Other, Money{lease}));
		connection->lease_day = TimerGameEconomy::date.base(); connection->invoiced_money += lease;
	}
	return TubePowerError::None;
}

/** Complete physical work and paid tariff share one electrical transaction; cache/query callbacks never invoice. */
class TubePaidWork final : public UtilityMutationParticipant {
	TubePowerConnection &connection; ///< Real physical service/invoice owner; source quantities remain with the ledger.
	UtilityMutationParticipant &work; ///< Actual complete movement/service owner, not a copied work accumulator.
	uint64_t cost; ///< Whole tariff Money computed with the saved unpaid numerator.
	uint16_t remainder; ///< Exact proposed fractional tariff after this physical use.
public:
	/** @param connection Actual service. @param work Complete physical participant. @param quantity_j Exact admitted actual energy use. */
	TubePaidWork(TubePowerConnection &connection, UtilityMutationParticipant &work, uint64_t quantity_j) : connection(connection), work(work)
	{
		const auto denominator = GetMultiLayerTubePowerContract().joules_per_money;
		const uint64_t numerator = quantity_j + connection.usage_remainder_j;
		this->cost = numerator / denominator; this->remainder = static_cast<uint16_t>(numerator % denominator);
	}
	/** @param proposal Revalidated owned grant. @return Whole physical/financial admission; saved fractions never grant credit. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		// Reserve the final fractional Money as well as today's whole tariff.
		// Running out of cash cannot erase a small unpaid use at service closure.
		if (proposal.owner != this->connection.owner || this->cost > static_cast<uint64_t>(INT64_MAX) ||
			this->connection.invoiced_money > UINT64_MAX - this->cost) return false;
		if (Company::Get(this->connection.owner)->money < Money{static_cast<int64_t>(this->cost + (this->remainder != 0))}) return false;
		return this->work.Validate(proposal);
	}
	/** @param proposal Exact checked physical use. @return Complete success or unchanged refusal, with no callbacks after the physical commit. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		if (!this->work.Commit(proposal)) return false;
		SubtractMoneyFromCompany(this->connection.owner, CommandCost(ExpensesType::Other, Money{static_cast<int64_t>(this->cost)}));
		this->connection.usage_remainder_j = this->remainder; this->connection.invoiced_money += this->cost;
		return true;
	}
	/** @return The actual participant's native effect identity; the invoice does not introduce another work effect. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return this->work.EffectKind(); }
};

/** @copydoc ConsumeTubeExternalPower */
UtilityMutationReceipt ConsumeTubeExternalPower(UtilityProducerID producer, UtilityConsumerID consumer, uint64_t quantity_j,
	UtilityMutationParticipant &participant)
{
	if (_tube_power_committing) return {UtilityPowerError::Refused};
	auto *connection = FindTubePowerConnection(producer);
	const auto *demand = GetUtilityPowerConsumer(consumer);
	// Physical source, actual TubeWork consumer and explicit sole-source link
	// must agree; a numeric grant or another company's service is insufficient.
	if (connection == nullptr || demand == nullptr || !connection->active || connection->owner != _current_company ||
		demand->owner != connection->owner || demand->meter.layer != GetTileLayer(connection->portal) ||
		demand->effect != UtilityConsumerEffect::TubeWork || demand->linked_producers.size() != 1 ||
		demand->linked_producers.front() != producer) return {UtilityPowerError::InvalidReference};
	if (CheckTubePowerTopology(*connection) != TubePowerError::None) return {UtilityPowerError::InvalidTopology};
	const auto grant = GetUtilityAllocation(consumer);
	if (!grant.has_value()) return {UtilityPowerError::InvalidInterval};
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {consumer, grant->accounting_interval}, quantity_j);
	if (proposal.error != UtilityPowerError::None) return {proposal.error};
	TubePaidWork action(*connection, participant, quantity_j);
	AutoRestoreBackup guard(_tube_power_committing, true);
	return CommitUtilityMutation(proposal, &action);
}

/** @copydoc CloseTubeExternalPower */
TubePowerError CloseTubeExternalPower(UtilityProducerID producer, bool execute)
{
	if (_tube_power_committing) return TubePowerError::Refused;
	auto *connection = FindTubePowerConnection(producer);
	if (connection == nullptr) return TubePowerError::InvalidTopology;
	if (!Company::IsValidID(connection->owner)) return TubePowerError::InvalidOwner;
	if (connection->owner != _current_company) return TubePowerError::InvalidOwner;
	if (!connection->active) return TubePowerError::Refused;
	const uint16_t closing = connection->usage_remainder_j != 0 ? 1 : 0;
	if (Company::Get(connection->owner)->money < Money{closing}) return TubePowerError::Funds;
	if (connection->invoiced_money > UINT64_MAX - closing) return TubePowerError::Limit;
	if (RemoveUtilityPowerProducer(producer, false) != UtilityPowerError::None) return TubePowerError::Ledger;
	if (execute) {
		if (RemoveUtilityPowerProducer(producer) != UtilityPowerError::None) return TubePowerError::Ledger;
		SubtractMoneyFromCompany(connection->owner, CommandCost(ExpensesType::Other, Money{closing}));
		connection->usage_remainder_j = 0; connection->invoiced_money += closing; connection->active = false;
	}
	return TubePowerError::None;
}

/** @copydoc CaptureTubePowerConnections */
std::vector<TubePowerConnection> CaptureTubePowerConnections() { return _tube_power; }

/** @copydoc ResetTubePowerState */
void ResetTubePowerState()
{
	_tube_power.clear(); _pending_tube_power.reset(); _tube_power_committing = false;
	ResetTubePowerVehicles();
	ResetTubePowerTopology();
	ResetTubePowerLoadStaging();
}

/** @copydoc ValidateTubePowerConnections */
TubePowerError ValidateTubePowerConnections(std::span<const TubePowerConnection> rows, const MultiLayerProfileSelection &profile,
	bool world_refs, const UtilityPowerState *utility)
{
	if (rows.size() > MAX_TUBE_POWER_CONNECTIONS) return TubePowerError::Limit;
	if (rows.empty()) return TubePowerError::None;
	if (profile.id != MultiLayerGameplayProfileID::MultilayerIndustry) return TubePowerError::Unavailable;
	size_t cells = 0;
	std::set<UtilityProducerID> sources;
	std::set<TileIndex> active_facilities;
	// Every retained invoice belongs to one actual ledger source. Structural
	// preview bounds are checked independently before real references resolve.
	for (const auto &row : rows) {
		cells += row.utility_path.size();
		// Malformed source, fraction, identity, revision or path cannot be repaired
		// into a usable service or another company's forgotten paid contract.
		if (!sources.insert(row.producer).second || row.producer.base() == UINT32_MAX || row.owner.base() >= MAX_COMPANIES ||
			row.portal == INVALID_TILE || row.interchange == INVALID_TILE || row.portal_revision == 0 ||
			row.utility_path.empty() || row.utility_path.size() > MAX_TUBE_POWER_PATH || row.utility_path.back() != row.interchange ||
			row.usage_remainder_j >= GetMultiLayerTubePowerContract().joules_per_money || row.lease_day == INT32_MIN ||
			row.invoiced_money < GetMultiLayerTubePowerContract().lease_per_day || cells > MAX_TUBE_POWER_PATH_CELLS) return TubePowerError::InvalidTopology;
		if (row.active && !active_facilities.insert(row.interchange).second) return TubePowerError::InvalidTopology;
		// Basic staging can precede electrical decoding. Independent complete
		// preview supplies its copied ledger so malformed invoices refuse there too.
		if (!world_refs && utility == nullptr) continue;
		const auto *source = world_refs ? GetUtilityPowerProducer(row.producer) :
			row.producer.base() < utility->producers.size() ? &utility->producers[row.producer.base()] : nullptr;
		// Sources and physical services retire together; tombstones keep their
		// original invoices even when the physical owner or portal later disappears.
		if (source == nullptr || source->kind != UtilityGenerationKind::External || source->active != row.active ||
			source->owner != row.owner) return TubePowerError::InvalidTopology;
		const auto &terms = GetMultiLayerTubePowerContract();
		const bool closed_paid = !row.active && row.usage_remainder_j == 0;
		const uint64_t billed_use = source->consumed_lifetime_j / terms.joules_per_money +
			(closed_paid && source->consumed_lifetime_j % terms.joules_per_money != 0);
		// Used joules have exactly one tariff history. A malformed fraction or
		// invoice must not erase debt, invent extra credit or exceed source capacity.
		if (row.usage_remainder_j != (closed_paid ? 0 : source->consumed_lifetime_j % terms.joules_per_money) ||
			row.invoiced_money < billed_use + terms.lease_per_day ||
			(row.invoiced_money - billed_use) % terms.lease_per_day != 0 || source->meter.generation_rate_w > terms.capacity_w) return TubePowerError::InvalidTopology;
		if (!world_refs || !row.active) continue;
		AutoRestoreBackup actor(_current_company, row.owner);
		if (row.portal.base() >= Map::Size()) return TubePowerError::InvalidTopology;
		if (source->meter.layer != GetTileLayer(row.portal)) return TubePowerError::InvalidTopology;
		const auto error = CheckTubePowerTopology(row);
		if (error != TubePowerError::None) return error;
	}
	return TubePowerError::None;
}

/** @copydoc StageLoadedTubePowerConnections */
void StageLoadedTubePowerConnections(std::vector<TubePowerConnection> rows) { _pending_tube_power = std::move(rows); }

/** @copydoc AcceptLoadedTubePowerConnections */
bool AcceptLoadedTubePowerConnections()
{
	if (!_pending_tube_power.has_value()) return true;
	if (ValidateTubePowerConnections(*_pending_tube_power, GetCurrentMultiLayerProfileSelection()) != TubePowerError::None) return false;
	_tube_power = std::move(*_pending_tube_power); _pending_tube_power.reset(); return true;
}

/** @copydoc ComputeTubePowerDigest */
uint64_t ComputeTubePowerDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit saved scalar; hash fixed bytes without host padding or pointers. */
	const auto field = [&hash](uint64_t value) {
		for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	field(_tube_power.size());
	// Saved source ordering and every invoice/path field distinguish physical
	// contract history from a different rendered or reconstructed power network.
	for (const auto &row : _tube_power) {
		field(row.producer.base()); field(row.owner.base()); field(row.portal.base()); field(row.interchange.base());
		field(row.portal_revision); field(static_cast<uint32_t>(row.lease_day)); field(row.usage_remainder_j); field(row.invoiced_money); field(row.active);
		field(row.utility_path.size());
		for (const auto tile : row.utility_path) field(tile.base());
	}
	return hash;
}

/** @param connection Actual service losing its physical authority. Expire unused energy, settle any affordable final fraction once and retain unpaid historical debt otherwise. */
static void RetirePhysicalTubePower(TubePowerConnection &connection)
{
	if (!connection.active) return;
	const auto *source = GetUtilityPowerProducer(connection.producer);
	if (source == nullptr) return;
	// The common company lifecycle owns source lineage. Mirror that actual
	// retained invoice owner after takeover, without manufacturing a new source.
	connection.owner = source->owner;
	if (source->active) {
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		RemoveUtilityPowerProducer(connection.producer);
	}
	// Physical loss cannot continue supplying free energy while waiting for
	// cash. A payable final fraction settles once; bankruptcy retains the old
	// unpaid numerator and exact already-paid history as an inactive tombstone.
	if (connection.usage_remainder_j != 0 && Company::IsValidID(connection.owner) &&
		Company::Get(connection.owner)->money >= Money{1} && connection.invoiced_money < UINT64_MAX) {
		SubtractMoneyFromCompany(connection.owner, CommandCost(ExpensesType::Other, Money{1}));
		connection.usage_remainder_j = 0; ++connection.invoiced_money;
	}
	connection.active = false;
}

/** @copydoc InvalidateTubePowerForPortal */
void InvalidateTubePowerForPortal(TileIndex portal)
{
	// Sealing/demolition expires both attached endpoint uses before the old
	// commissioning revision disappears. Historical generation/use survives.
	for (auto &connection : _tube_power) {
		if (!connection.active) continue;
		const auto pair = GetLayerPortalPairKey(connection.portal);
		const auto changed = GetLayerPortalPairKey(portal);
		if (pair.has_value() && changed.has_value() && pair == changed) RetirePhysicalTubePower(connection);
	}
	InvalidateTubePowerTopology(INVALID_TILE);
}

/** @copydoc InvalidateTubePowerForTile */
void InvalidateTubePowerForTile(TileIndex tile)
{
	if (tile == INVALID_TILE) return;
	// Inspect only bounded registered footprints. An unrelated ordinary
	// track mutation cannot retire a distant valid paid inlet.
	for (auto &connection : _tube_power) {
		if (!connection.active) continue;
		if (std::ranges::find(connection.utility_path, tile) == connection.utility_path.end()) continue;
		AutoRestoreBackup actor(_current_company, connection.owner);
		if (CheckTubePowerTopology(connection) != TubePowerError::None) RetirePhysicalTubePower(connection);
	}
}

/** @copydoc ChangeTubePowerCompanyOwnership */
void ChangeTubePowerCompanyOwnership(Owner old_owner, [[maybe_unused]] Owner new_owner)
{
	const auto bindings = CaptureTubePowerVehicles();
	// A takeover/removal cannot give a reused pool identity another capsule's
	// stale offer. Retire actual bindings while native companies/vehicles still
	// exist; the common ledger has already applied its exact lineage policy.
	for (const auto &binding : bindings) {
		const auto *connection = GetTubePowerConnection(binding.source);
		if (binding.active && connection != nullptr && connection->owner == old_owner) RetireTubePowerVehicle(binding.vehicle);
	}
	for (auto &connection : _tube_power) {
		if (connection.owner != old_owner) continue;
		if (connection.active) RetirePhysicalTubePower(connection);
		const auto *source = GetUtilityPowerProducer(connection.producer);
		if (source != nullptr) connection.owner = source->owner;
	}
	InvalidateTubePowerTopology(INVALID_TILE);
}
