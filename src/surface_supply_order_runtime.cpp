/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_runtime.cpp Private native commitment/history owner; physical stock and finance retain their original owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CCC/13CCD.
 * Only synchronized commands create/cancel paid commitments; the original command kernel settles their costs.
 */
#include "stdafx.h"
#include "surface_supply_order_runtime.h"
#include "surface_supply_cmd.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "station_base.h"
#include "map_func.h"
#include "cargotype.h"
#include "cargo_custody.h"
#include "window_func.h"
#include "safeguards.h"

/** Unpublished native family; its clock stamp must join independently decoded original DATE before any right is installed. */
struct SurfaceSupplyLoadCandidate {
	SurfaceSupplyOrderSnapshot snapshot; ///< Candidate's bounded commitment/history transport, not usable stock.
	TimerGameEconomy::Date saved_day{0}; ///< Family's original stamp, independent of reconstructed native time.
};

/** Independent original MAPS evidence; this is transport provenance, not another map/geometry authority. */
struct SurfaceSupplyLoadedMap {
	uint32_t width = 0, height = 0; ///< Exact original native plane dimensions.
	uint8_t layers = 0; ///< Original native stored plane count.
};

static SurfaceSupplyOrderSnapshot supply_orders; ///< Sole installed native commitment/history owner; packets own every physical unit.
static std::optional<SurfaceSupplyLoadCandidate> supply_candidate; ///< One unpublished full-file family, never a shadow inventory.
static std::optional<TimerGameEconomy::Date> supply_loaded_day; ///< Actual independently decoded full DATE evidence, cleared between files.
static std::optional<SurfaceSupplyLoadedMap> supply_loaded_map; ///< Actual full MAPS evidence; defaults/current map alone cannot validate an absent source family.

/** Refresh existing native presentation after a real receipt transition.
 * @param owner Current company whose station-list and company-wide allowance views changed.
 * Every open station quote shares the company-day cap, so dirty all station
 * views; invalidation itself owns no quote, cargo, money or simulation tick.
 */
static void InvalidateSurfaceSupplyWindows(CompanyID owner)
{
	SetWindowClassesDirty(WindowClass::StationView);
	if (Company::IsValidID(owner)) InvalidateWindowData(WindowClass::StationList, owner);
}

/** Read one actual reconstructed packet's original canonical scalars without assigning it a container.
 * @param id Exact native allocated identity requested by the shared custody checker.
 * @return Original packet facts or absence; no allocation, source inference, dispatch or current-price fallback.
 */
static std::optional<SurfaceSupplyPacketFacts> NativeSurfaceSupplyPacketFacts(CargoPacketID id)
{
	const auto *packet = CargoPacket::GetIfValid(id);
	if (packet == nullptr) return std::nullopt;
	return SurfaceSupplyPacketFacts{id, packet->Count(), packet->GetSource(), packet->GetFeederShare(), packet->GetCompanySupplyOrder(), packet->HasCompanySupplyShipped()};
}

/** Join a complete candidate book to the currently reconstructed original native owners without publishing or repairing anything.
 * @param snapshot Whole admitted transport or current installed native metadata.
 * @return Complete actual-reference agreement; a missing/foreign/wrong-layer receiver supplies no pending right.
 */
static bool ValidateNativeSurfaceSupplySnapshot(const SurfaceSupplyOrderSnapshot &snapshot)
{
	SurfaceSupplyRecordContext context;
	context.profile = GetCurrentMultiLayerProfileSelection().id;
	context.width = Map::SizeX(); context.height = Map::SizeY();
	// Actual native company existence grants only pending ownership. Original retired payer/terminal history stays historical and cannot borrow a new company's slot as a refund right.
	for (const auto *company : Company::Iterate()) context.companies[company->index.base()] = true;
	std::vector<SurfaceSupplyStationView> stations;
	stations.reserve(Station::GetNumItems());
	// Read the original normal-station pool through the map/layer owners; neither selected camera nor matching coordinates in another layer may validate the receiving freight facility.
	for (const auto *station : Station::Iterate()) {
		stations.push_back({station->index, station->owner, static_cast<uint16_t>(TileX(station->xy)), static_cast<uint16_t>(TilePlaneY(station->xy)),
			GetTileLayer(station->xy), station->facilities.Any({StationFacility::Train, StationFacility::TruckStop})});
	}
	context.stations = stations;
	SurfaceSupplyCustodyCheck check(snapshot, context, NativeSurfaceSupplyPacketFacts, CargoPacket::GetNumItems());
	/** Bind actual physical owner facts to the same original packet lookup.
	 * @param packet Actual fragment held by this visited native container.
	 * @param cargo Container's resolved native cargo slot.
	 * @param station Actual original goods queue, or Invalid for every other container.
	 * @return Whether shared scalar, identity, label and finite-history admission remains valid.
	 */
	const auto observe = [&](const CargoPacket &packet, CargoType cargo, StationID station) {
		return check.AddOwned(packet.index, CargoSpec::Get(cargo)->label, station);
	};
	if (!VisitNativeCargoPackets(observe)) return false;
	// Only after real owners have supplied their references can allocated rows
	// expose paid orphans or incomplete evidence. The pool never supplies custody.
	for (const auto *packet : CargoPacket::Iterate()) {
		if (!check.ObserveAllocated(packet->index)) return false;
	}
	return check.Valid();
}

/** @copydoc ResetSurfaceSupplyOrderLoadStaging */
void ResetSurfaceSupplyOrderLoadStaging() { supply_candidate.reset(); supply_loaded_day.reset(); supply_loaded_map.reset(); }

/** @copydoc ResetSurfaceSupplyOrders */
void ResetSurfaceSupplyOrders() { supply_orders = {}; ResetSurfaceSupplyOrderLoadStaging(); }

/** @copydoc CaptureSurfaceSupplyOrders */
SurfaceSupplyOrderSnapshot CaptureSurfaceSupplyOrders() { return supply_orders; }

/** @copydoc GetNextSurfaceSupplyOrderID */
SurfaceSupplyOrderID GetNextSurfaceSupplyOrderID() { return supply_orders.next_id; }

/** @copydoc GetSurfaceSupplyOrder */
const SurfaceSupplyOrderRecord *GetSurfaceSupplyOrder(SurfaceSupplyOrderID id)
{
	return FindSurfaceSupplyOrderRecord(supply_orders, id);
}

/** @copydoc GetSurfaceSupplyOrderDayUsage */
std::optional<uint32_t> GetSurfaceSupplyOrderDayUsage(CompanyID company, CargoLabel label)
{
	return CountSurfaceSupplyDayUsage(supply_orders, company, label, TimerGameEconomy::date);
}

/** @copydoc ValidateSurfaceSupplyOrderBook */
bool ValidateSurfaceSupplyOrderBook() { return ValidateNativeSurfaceSupplySnapshot(supply_orders); }

/** @copydoc PrepareLegacySurfaceSupplyCancellation */
std::optional<LegacySurfaceSupplyCancellation> PrepareLegacySurfaceSupplyCancellation(SurfaceSupplyOrderID intent)
{
	// Only actual company play in the paid-supply profile can exercise an old
	// entitlement. Neither editor/deity identities nor another profile mint it.
	if (!Company::IsValidID(_current_company) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto *record = GetSurfaceSupplyOrder(intent);
	// New native orders keep their Pending-only refund rule. A closed or
	// foreign original legacy receipt cannot reopen because stock returned.
	if (record == nullptr || record->Legacy() == nullptr || record->Legacy()->closed || record->owner != _current_company) return std::nullopt;
	const auto *source = Station::GetIfValid(record->origin);
	// B allowed its original owned Surface sign to refund remaining stock
	// after berth removal, but never a foreign/hidden/replacement source.
	if (source == nullptr || source->owner != record->owner || !MultiLayerMap::IsTileInLayer(source->xy, MapLayer::Surface)) return std::nullopt;
	const auto cargo = GetCargoTypeByLabel(record->label);
	if (!IsValidCargoType(cargo)) return std::nullopt;
	// All allocation/reference/scalar checking precedes removal. Shared full
	// custody rejects duplicates/orphans and bounds the positive saved tariff
	// times the whole original purchase; this is not another receipt validator.
	if (!ValidateSurfaceSupplyOrderBook()) return std::nullopt;
	const auto eligible = source->goods[cargo].HasData() ? source->goods[cargo].GetData().cargo.CountUnshippedCompanySupply(intent) : 0;
	const auto *legacy = record->Legacy();
	const uint32_t remaining = record->quantity - legacy->refunded - legacy->dispatched;
	if (eligible > remaining) return std::nullopt;
	const Money refund = legacy->unit_price * static_cast<uint16_t>(eligible);
	const Money cash = Company::Get(record->owner)->money;
	// A positive refund must fit before any stock is removed. Subtracting a
	// negative wallet from INT64_MAX would itself overflow, so only positive
	// balances need this upper-bound subtraction; debt cannot cause overflow.
	if (cash > Money{0} && refund > Money{INT64_MAX} - cash) return std::nullopt;
	return LegacySurfaceSupplyCancellation{intent, record->origin, cargo, static_cast<uint16_t>(eligible), refund};
}

/** @copydoc CanRecordSurfaceSupplyDispatch(SurfaceSupplyOrderID, uint16_t) */
bool CanRecordSurfaceSupplyDispatch(SurfaceSupplyOrderID id, uint16_t quantity)
{
	return CanRecordSurfaceSupplyDispatch(supply_orders, id, quantity);
}

/** @copydoc RecordSurfaceSupplyDispatch(SurfaceSupplyOrderID, uint16_t) */
bool RecordSurfaceSupplyDispatch(SurfaceSupplyOrderID id, uint16_t quantity)
{
	return RecordSurfaceSupplyDispatch(supply_orders, id, quantity);
}

/** @copydoc DispatchPendingSurfaceSupplyOrders */
void DispatchPendingSurfaceSupplyOrders(SurfaceSupplyStationReceiver receive)
{
	// Only the real active profile and original station ingress can take custody; an inactive profile or missing receiver cannot turn saved commitments into stock.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || receive == nullptr) return;
	// Traverse the bounded sole book in original intent order, once per station acceptance interval. Capacity refusals leave their paid rights pending; later attempts never recharge or duplicate a fulfilled purchase.
	for (auto &record : supply_orders.records) {
		// Legacy receipts describe already delivered stock, never pending native
		// ingress; dispatching them would create a second unpaid physical purchase.
		if (record.Native() == nullptr || record.Native()->state != SurfaceSupplyOrderState::Pending) continue;
		const SurfaceSupplyOrderRequest request{record.origin, record.owner, record.label, record.quantity};
		const auto view = CaptureSurfaceSupplyOrderView(request, 0);
		const auto *station = Station::GetIfValid(record.origin);
		// Pending rights require the exact original physical owned freight facility, not a reused station slot or matching hidden-layer coordinate. Already-paid purchases ignore today's cash, cap and inflation.
		if (!view.company_exists || !view.station_exists || view.station_owner != record.owner || view.station_layer != MapLayer::Surface || !view.loading_station ||
			TileX(station->xy) != record.Native()->origin_x || TilePlaneY(station->xy) != record.Native()->origin_y) continue;
		// Native whole-cargo storage and packet admission may change while an order waits. A refusal preserves the entire unshipped commitment and its receiving reservation, with no partial allocation or terminal receipt.
		if (!IsValidCargoType(view.cargo) || view.station_quantity > UINT32_MAX - record.quantity || !view.packet_room) continue;
		const auto issued = receive(record.origin, view.cargo, record.quantity, {record.Native()->packet_buyer, SourceType::CompanySupply});
		if (issued == CargoPacketID::Invalid()) continue;
		record.Native()->issued_packet = issued; record.Native()->state = SurfaceSupplyOrderState::Fulfilled;
		InvalidateSurfaceSupplyWindows(record.owner);
	}
}

/** @copydoc ReconcileSurfaceSupplyStation */
void ReconcileSurfaceSupplyStation(StationID station, bool removed)
{
	const auto *receiver = removed ? nullptr : Station::GetIfValid(station);
	// Reconcile this exact station before its native identity can be reused.
	// Native Pending and original legacy stock have different loss rules;
	// neither branch invents a replacement source from nearby coordinates.
	for (auto &record : supply_orders.records) {
		if (record.origin != station) continue;
		if (auto *legacy = record.Legacy()) {
			// B's entitlement survives loss of the freight berth or a sign move
			// within the same owned Surface station. Source/ownership loss closes
			// it without credit; actual packets retain native destruction/custody.
			if (receiver != nullptr && receiver->owner == record.owner && MultiLayerMap::IsTileInLayer(receiver->xy, MapLayer::Surface)) continue;
			legacy->closed = true; record.origin = StationID::Invalid();
			InvalidateSurfaceSupplyWindows(record.owner);
			continue;
		}
		// Only unshipped commitments at the affected receiver are candidates for a refund; unrelated stations and already delivered purchases retain their own custody/history.
		if (record.Native() == nullptr || record.Native()->state != SurfaceSupplyOrderState::Pending) continue;
		// The actual original owned Surface freight facility must still match its saved physical anchor. An alive station sign/airport/bus stop cannot retain an unusable receiving reservation.
		if (receiver != nullptr && receiver->owner == record.owner && GetTileLayer(receiver->xy) == MapLayer::Surface &&
			receiver->facilities.Any({StationFacility::Train, StationFacility::TruckStop}) && TileX(receiver->xy) == record.Native()->origin_x && TilePlaneY(receiver->xy) == record.Native()->origin_y) continue;
		if (!Company::IsValidID(record.owner)) {
			record.Native()->state = SurfaceSupplyOrderState::Lost; record.owner = CompanyID::Invalid(); continue;
		}
		// Retire the unshipped right before native account/window invalidation, so a repeated station notification can neither refund twice nor leave usable purchased cargo behind.
		record.Native()->state = SurfaceSupplyOrderState::Cancelled;
		SubtractMoneyFromCompany(record.owner, CommandCost(ExpensesType::Property, -record.Native()->cost));
	}
}

/** @copydoc ChangeSurfaceSupplyOrderCompanyOwnership */
void ChangeSurfaceSupplyOrderCompanyOwnership(CompanyID old_owner, CompanyID new_owner)
{
	// Only a real retiring company and a distinct valid successor/loss event may change rights; invalid/self transfers must not erase provenance or manufacture a refund.
	if (old_owner >= CompanyID::End() || old_owner == new_owner || (new_owner != CompanyID::Invalid() && !Company::IsValidID(new_owner))) return;
	// Retain immutable original purchase history while moving only current rights/cap ownership. Retired buyer sentinels prevent a reused company slot from becoming the purchaser of old native cargo.
	for (auto &record : supply_orders.records) {
		// Unknown provenance is never repaired into a transferable entitlement;
		// normal admission excludes it before any live book can be installed.
		if (record.Native() == nullptr && record.Legacy() == nullptr) continue;
		if (auto *native = record.Native()) {
			if (native->packet_buyer == old_owner.base()) native->packet_buyer = Source::Invalid;
		}
		if (record.owner != old_owner) continue;
		record.owner = new_owner;
		if (auto *legacy = record.Legacy()) {
			// A takeover inherits the same finite history and company-day usage.
			// Loss without a successor closes rights before station demolition,
			// preserving unavailable original buyer evidence and actual packets.
			if (new_owner == CompanyID::Invalid()) { legacy->closed = true; record.origin = StationID::Invalid(); }
			continue;
		}
		// Company loss occurs before native station demolition: an unshipped purchase is lost with the company and must not turn demolition into a refund to a successor/reused slot.
		if (record.Native() != nullptr && new_owner == CompanyID::Invalid() && record.Native()->state == SurfaceSupplyOrderState::Pending) record.Native()->state = SurfaceSupplyOrderState::Lost;
	}
	CargoPacket::InvalidateAllFrom({old_owner.base(), SourceType::CompanySupply});
	InvalidateSurfaceSupplyWindows(old_owner); InvalidateSurfaceSupplyWindows(new_owner);
}

/** @copydoc StageLoadedSurfaceSupplyOrders */
bool StageLoadedSurfaceSupplyOrders(SurfaceSupplyOrderSnapshot snapshot, TimerGameEconomy::Date saved_day)
{
	// Duplicate, oversized or identity-less transport cannot replace the installed owner or hide a previously staged native family.
	if (supply_candidate || snapshot.records.size() > MAX_LEGACY_SURFACE_SUPPLY_RECORDS || snapshot.next_id == 0) return false;
	size_t native_records = 0;
	// Complete B history can exceed new-purchase policy without truncation, but
	// that larger transport bound must never enlarge native receipt admission.
	for (const auto &record : snapshot.records) {
		if (record.Native() != nullptr && ++native_records > MAX_SURFACE_SUPPLY_RECORDS) return false;
	}
	supply_candidate = SurfaceSupplyLoadCandidate{std::move(snapshot), saved_day};
	return true;
}

/** @copydoc ObserveLoadedSurfaceSupplyEconomyDate */
bool ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::Date day)
{
	if (supply_loaded_day) return false;
	supply_loaded_day = day; return true;
}

/** @copydoc ObserveLoadedSurfaceSupplyMap */
bool ObserveLoadedSurfaceSupplyMap(uint32_t width, uint32_t height, uint8_t layers)
{
	// Only bounded original geometry may supply this file's MAPS evidence; duplicate transport or malformed dimensions cannot borrow an initialized running map.
	if (supply_loaded_map || !Map::DecodeStoredTile(TileIndex{0}, width, height, layers)) return false;
	supply_loaded_map = SurfaceSupplyLoadedMap{width, height, layers}; return true;
}

/** @copydoc AcceptLoadedSurfaceSupplyOrders */
bool AcceptLoadedSurfaceSupplyOrders(bool family_required)
{
	if (!supply_candidate) {
		if (family_required) return false;
		// Genuine old absence cannot erase original linked history while its
		// actual paid packets still exist. Validate empty history against owners
		// first; unlinked A supplies remain legal in their real native containers.
		if (!ValidateNativeSurfaceSupplySnapshot({})) return false;
		// Genuine older files gain no procurement history, reservation, expense or seed packet; migration installs only the empty private book.
		supply_orders = {}; ResetSurfaceSupplyOrderLoadStaging(); return true;
	}
	// The family stamp and actual same-file DATE must agree with reconstructed native economy time. A current world/calendar default cannot replace absent or changed original day evidence.
	if (!supply_loaded_day || *supply_loaded_day != supply_candidate->saved_day || *supply_loaded_day != TimerGameEconomy::date) return false;
	// A paid right must join the original allocated storage geometry too; initialized defaults cannot substitute for a candidate that omitted or changed its MAPS owner.
	if (!supply_loaded_map || supply_loaded_map->width != Map::SizeX() || supply_loaded_map->height != Map::SizeY() || supply_loaded_map->layers > Map::LayerCount()) return false;
	// Native MAPS reconstruction may initialize missing planes up to MAX_MAP_LAYERS; this preserves the original Surface and never treats new empty planes as serialized receiving stations.
	if (!ValidateNativeSurfaceSupplySnapshot(supply_candidate->snapshot)) return false;
	supply_orders = std::move(supply_candidate->snapshot);
	ResetSurfaceSupplyOrderLoadStaging(); return true;
}

/** Decide whether history can retire without releasing pending stock reservations or today's spent allowance.
 * @param record Original retained commitment/receipt.
 * @return True only for terminal history strictly older than the current economy day; retired IDs remain behind next_id forever.
 */
static bool CanRetireSurfaceSupplyOrder(const SurfaceSupplyOrderRecord &record)
{
	// Imported receipts retain lifetime history. Native terminal rows alone can
	// retire under the original policy without inventing a legacy cap/refund reset.
	return record.Native() != nullptr && record.Native()->state != SurfaceSupplyOrderState::Pending && record.purchase_day < TimerGameEconomy::date;
}

/** @copydoc CmdPlaceSurfaceSupplyOrder */
CommandCost CmdPlaceSurfaceSupplyOrder(DoCommandFlags flags, StationID origin,
	uint32_t label_value, uint16_t quantity, SurfaceSupplyOrderID intent)
{
	// A synchronized intent belongs to an actual company in the selected paid-supply profile; deity/editor callers cannot mint commitments.
	if (!Company::IsValidID(_current_company) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return CMD_ERROR;
	const uint8_t bytes[4] = {static_cast<uint8_t>(label_value >> 24), static_cast<uint8_t>(label_value >> 16), static_cast<uint8_t>(label_value >> 8), static_cast<uint8_t>(label_value)};
	const CargoLabel label{bytes};
	if (const auto *existing = GetSurfaceSupplyOrder(intent)) {
		// Retries acknowledge only this company's identical live/fulfilled purchase. Changed payload, foreign control and refunded/lost history cannot acquire another charge or stock right.
		if (existing->Native() == nullptr || existing->owner != _current_company || existing->origin != origin || existing->label != label || existing->quantity != quantity ||
			(existing->Native()->state != SurfaceSupplyOrderState::Pending && existing->Native()->state != SurfaceSupplyOrderState::Fulfilled)) return CMD_ERROR;
		return CommandCost(ExpensesType::Property, 0);
	}
	// Exactly the never-issued frontier may start a purchase; a stale/guessed gap or exhausted counter must never reissue a retired intent.
	if (intent != supply_orders.next_id || intent == UINT64_MAX) return CMD_ERROR;
	const auto usage = GetSurfaceSupplyOrderDayUsage(_current_company, label);
	if (!usage) return CMD_ERROR;
	const SurfaceSupplyOrderRequest request{origin, _current_company, label, quantity};
	auto view = CaptureSurfaceSupplyOrderView(request, *usage);
	uint64_t receiving_quantity = view.station_quantity;
	size_t retained = 0;
	// All unshipped orders reserve space in the original receiving cargo list, even across economy days. Only old terminal history may retire to make bounded metadata room; no second stock/cap cache is maintained.
	for (const auto &record : supply_orders.records) {
		if (!CanRetireSurfaceSupplyOrder(record)) ++retained;
		// Other labels/stations cannot consume this receiving reservation, while an older unpaid-to-stock commitment still must fit when eventually dispatched.
		if (record.Native() != nullptr && record.Native()->state == SurfaceSupplyOrderState::Pending && record.origin == origin && record.label == label) receiving_quantity += record.quantity;
	}
	// Refuse the whole new purchase before any charge if existing stock plus commitments cannot fit, or if mandatory retained history already fills the bounded book.
	if (receiving_quantity > UINT32_MAX || retained >= MAX_SURFACE_SUPPLY_RECORDS) return CMD_ERROR;
	view.station_quantity = static_cast<uint32_t>(receiving_quantity);
	const auto quote = PrepareSurfaceSupplyOrder(request, view);
	if (quote.error != SurfaceSupplyOrderError::None) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		// Reserve before publishing the paid right; allocation failure must precede metadata mutation and native financial settlement. Execution recaptured all owners independently of the preceding estimate.
		supply_orders.records.reserve(retained + 1);
		std::erase_if(supply_orders.records, CanRetireSurfaceSupplyOrder);
		const auto *station = Station::Get(origin);
		SurfaceSupplyOrderRecord record;
		record.id = intent; record.Native()->purchaser = record.owner = _current_company; record.origin = origin;
		record.Native()->origin_x = static_cast<uint16_t>(TileX(station->xy)); record.Native()->origin_y = static_cast<uint16_t>(TilePlaneY(station->xy));
		record.label = label; record.quantity = quantity; record.Native()->cost = quote.cost; record.Native()->inflation_prices = view.inflation_prices;
		record.Native()->purchase_year = view.year; record.purchase_day = TimerGameEconomy::date; record.Native()->packet_buyer = _current_company.base();
		supply_orders.records.push_back(record); ++supply_orders.next_id;
		InvalidateSurfaceSupplyWindows(record.owner);
	}
	// The original top-level command kernel charges this exact complete expense once; the order owner never directly changes company money or ordinary freight revenue.
	return CommandCost(ExpensesType::Property, quote.cost);
}

/** @copydoc CmdCancelSurfaceSupplyOrder */
CommandCost CmdCancelSurfaceSupplyOrder(DoCommandFlags flags, SurfaceSupplyOrderID intent)
{
	if (!Company::IsValidID(_current_company)) return CMD_ERROR;
	const auto found = FindSurfaceSupplyOrderRecord(supply_orders, intent);
	// Only the current company's exact unshipped right may return its frozen expense. Fulfilled, lost, cancelled and retired history cannot refund usable/consumed cargo or pay twice.
	if (found == nullptr || found->Native() == nullptr || found->owner != _current_company || found->Native()->state != SurfaceSupplyOrderState::Pending) return CMD_ERROR;
	const auto refund = -found->Native()->cost;
	if (flags.Test(DoCommandFlag::Execute)) {
		found->Native()->state = SurfaceSupplyOrderState::Cancelled;
		InvalidateSurfaceSupplyWindows(found->owner);
	}
	return CommandCost(ExpensesType::Property, refund);
}

/** @copydoc CmdCancelLegacySurfaceSupplyOrder */
CommandCost CmdCancelLegacySurfaceSupplyOrder(DoCommandFlags flags, SurfaceSupplyOrderID intent, uint16_t expected_quantity)
{
	const auto quote = PrepareLegacySurfaceSupplyCancellation(intent);
	// Both command phases independently require the complete originally
	// requested source count. A changed partial remainder cannot be credited
	// merely because its smaller amount would still fit the original bill.
	if (!quote || quote->quantity != expected_quantity) return CMD_ERROR;
	const CommandCost settlement(ExpensesType::Other, -quote->refund);
	if (flags.Test(DoCommandFlag::Execute)) {
		auto *record = FindSurfaceSupplyOrderRecord(supply_orders, intent);
		auto &goods = Station::Get(quote->station)->goods[quote->cargo];
		auto *queue = goods.HasData() ? &goods.GetData().cargo : nullptr;
		// An absent native list can support only a zero-unit closure. Do not
		// allocate goods data just to make an otherwise empty refund possible.
		if (queue == nullptr && expected_quantity != 0) return CMD_ERROR;
		// Queue any allocating GUI invalidation before physical removal. These
		// native notifications only mark presentation; no simulation callback or
		// external mutation intervenes before the exact native count recapture.
		InvalidateSurfaceSupplyWindows(record->owner);
		if (queue != nullptr && !queue->RemoveUnshippedCompanySupply(intent, expected_quantity)) return CMD_ERROR;
		// The admitted original remainder bounds this addition. Everything after
		// removal is scalar no-fail history mutation and the prebuilt expense;
		// the command kernel alone pays it once into the real Other account.
		record->Legacy()->refunded += expected_quantity;
		record->Legacy()->closed = true;
	}
	return settlement;
}
