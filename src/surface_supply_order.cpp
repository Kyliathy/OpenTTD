/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order.cpp Pure original-owner order/receipt validation and shared day allowance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 2 (include what is used on Linux)
 * No function here installs an owner, allocates cargo, charges/refunds cash or activates saved orders.
 */
#include "stdafx.h"
#include "surface_supply_order.h"
#include "multilayer_bootstrap.h"
#include "timer/timer_game_calendar.h"
#include <utility> // std::as_const in the mutable record lookup.

#include "safeguards.h"

/** Validate only original A provenance without requiring physical historical packets to survive.
 * @param record Common original identity/material/current-owner facts.
 * @param native Explicit native evidence; never constructed from a legacy tariff or live world.
 * @param context Candidate-owned references and geometry.
 * @param definition Existing canonical supply terms, resolved once by the outer validator.
 * @return Exact original A refusal, or complete native receipt agreement.
 */
static SurfaceSupplyRecordError ValidateNativeReceipt(const SurfaceSupplyOrderRecord &record, const NativeSupplyReceipt &native,
	const SurfaceSupplyRecordContext &context, const MultiLayerSurfaceSupply *definition)
{
	// Original payer is immutable history; only a real-domain owner or retired sentinel can retain current control. A retired payer need not exist in this candidate's PLYR.
	if (native.purchaser >= CompanyID::End() || (record.owner != CompanyID::Invalid() && record.owner >= CompanyID::End()) ||
		(native.packet_buyer != Source::Invalid && native.packet_buyer != native.purchaser.base())) return SurfaceSupplyRecordError::Company;
	// Origins remain Surface-local historical coordinates. A deleted terminal station may stay in history but cannot become a hidden-layer destination or future permission.
	if (record.origin >= StationID::End() || native.origin_x >= context.width || native.origin_y >= context.height) return SurfaceSupplyRecordError::Geometry;
	if (native.purchase_year < definition->introduction_year) return SurfaceSupplyRecordError::Date;
	const auto price = CalculateSurfaceSupplyCost(record.label, record.quantity, native.inflation_prices);
	if (!price || native.cost != *price) return SurfaceSupplyRecordError::Price; // Frozen original native pricing must match exactly; current inflation or a forged free receipt cannot finance a refund.
	switch (native.state) {
		case SurfaceSupplyOrderState::Pending: {
			// Only a still-existing owner and the same owned Surface freight receiver may hold a refundable undispatched right; the open world supplies no missing reference.
			if (record.owner >= CompanyID::End() || !context.companies[record.owner.base()]) return SurfaceSupplyRecordError::Company;
			const auto station = std::ranges::lower_bound(context.stations, record.origin, {}, &SurfaceSupplyStationView::station);
			if (station == context.stations.end() || station->station != record.origin || station->owner != record.owner || station->layer != MapLayer::Surface ||
				!station->freight_facility || station->x != native.origin_x || station->y != native.origin_y) return SurfaceSupplyRecordError::Station;
			if (native.issued_packet != CargoPacketID::Invalid()) return SurfaceSupplyRecordError::Packet;
			break;
		}
		case SurfaceSupplyOrderState::Fulfilled:
			if (native.issued_packet >= CargoPacketID::End()) return SurfaceSupplyRecordError::Packet;
			break; // The original index is only issuance history; consumed/merged packets need not exist and cannot grant a second shipment/refund.
		case SurfaceSupplyOrderState::Cancelled:
			if (native.issued_packet != CargoPacketID::Invalid()) return SurfaceSupplyRecordError::Packet;
			break;
		case SurfaceSupplyOrderState::Lost:
			if (record.owner != CompanyID::Invalid() || native.issued_packet != CargoPacketID::Invalid()) return SurfaceSupplyRecordError::State; // Company loss leaves neither a live refund owner nor a dispatched packet.
			break;
		default:
			return SurfaceSupplyRecordError::State;
	}
	return SurfaceSupplyRecordError::None;
}

/** Validate B's saved finite entitlement facts without inventing missing native purchase evidence.
 * @param record Original widened identity, current owner/source, whole quantity and economy day.
 * @param legacy Exact original tariff/calendar/counters/closure.
 * @param context Same-file company/station facts, never active-world lookup.
 * @param definition Existing immutable supply terms, not a second tariff catalogue.
 * @return Complete original B scalar/reference agreement; no stock or refund is created.
 */
static SurfaceSupplyRecordError ValidateLegacyReceipt(const SurfaceSupplyOrderRecord &record, const LegacyBSupplyReceipt &legacy,
	const SurfaceSupplyRecordContext &context, const MultiLayerSurfaceSupply *definition)
{
	// A refunded quantity was physically removed already; it cannot remain open,
	// or combine with prior dispatch to exceed the one original paid quantity.
	if (legacy.refunded > record.quantity || legacy.dispatched > record.quantity - legacy.refunded || (legacy.refunded != 0 && !legacy.closed)) return SurfaceSupplyRecordError::State;
	// Calendar introduction and economy-day caps are distinct historical facts.
	// Neither current inflation nor the running world's clocks may supply either.
	if (record.purchase_day.base() < 0 || record.purchase_day.base() > EconomyTime::MAX_DATE.base() ||
		legacy.calendar_date < 0 || legacy.calendar_date > CalendarTime::MAX_DATE.base() ||
		legacy.calendar_date < TimerGameCalendar::ConvertYMDToDate(definition->introduction_year, 0, 1).base()) return SurfaceSupplyRecordError::Date;
	// Validate the positive original tariff product before arithmetic; a future
	// legitimate partial refund must fit Money without repricing the old purchase.
	if (legacy.unit_price <= Money{0} || legacy.unit_price > Money{INT64_MAX / record.quantity}) return SurfaceSupplyRecordError::Price;
	if (record.owner != CompanyID::Invalid()) {
		// A surviving current rights owner must exist in this candidate; a reused
		// or reserved company ordinal cannot inherit an old refund entitlement.
		if (record.owner >= CompanyID::End() || !context.companies[record.owner.base()]) return SurfaceSupplyRecordError::Company;
	} else {
		if (record.origin != StationID::Invalid()) return SurfaceSupplyRecordError::Company;
	}
	// Source or company loss retires cancellation but not the paid receipt. An
	// invalid source must never be repaired to a current station at similar XY.
	if ((record.owner == CompanyID::Invalid() || record.origin == StationID::Invalid()) && !legacy.closed) return SurfaceSupplyRecordError::State;
	if (record.origin != StationID::Invalid()) {
		const auto station = std::ranges::lower_bound(context.stations, record.origin, {}, &SurfaceSupplyStationView::station);
		// B allowed cancellation after freight-berth removal while its original
		// owned Surface sign still held stock. Do not impose A's saved-anchor rule.
		if (station == context.stations.end() || station->station != record.origin || station->owner != record.owner || station->layer != MapLayer::Surface) return SurfaceSupplyRecordError::Station;
	}
	return SurfaceSupplyRecordError::None;
}

/** Inspect retained size/provenance before counting or accepting any row.
 * @param snapshot Whole canonical history, without an installed-state assumption.
 * @return Size or unknown-tag refusal; native rows retain their original admission bound.
 */
static SurfaceSupplyRecordError ValidateReceiptKinds(const SurfaceSupplyOrderSnapshot &snapshot)
{
	if (snapshot.records.size() > MAX_LEGACY_SURFACE_SUPPLY_RECORDS) return SurfaceSupplyRecordError::Size;
	size_t native_count = 0, legacy_count = 0;
	// Imported paid history may exceed the purchase policy, but cannot disguise
	// extra native purchases or an unknown receipt as a legacy entitlement.
	for (const auto &record : snapshot.records) {
		if (record.Native() != nullptr) {
			if (++native_count > MAX_SURFACE_SUPPLY_RECORDS) return SurfaceSupplyRecordError::Size;
		} else {
			if (record.Legacy() == nullptr) return SurfaceSupplyRecordError::State;
			// B retains its complete original dense prefix for life. A later native
			// purchase or pruned native row cannot hide missing/reforged B history.
			if (native_count != 0 || record.id != ++legacy_count) return SurfaceSupplyRecordError::Identity;
		}
	}
	return SurfaceSupplyRecordError::None;
}

/** @copydoc ValidateSurfaceSupplyRecords */
SurfaceSupplyRecordError ValidateSurfaceSupplyRecords(const SurfaceSupplyOrderSnapshot &snapshot, const SurfaceSupplyRecordContext &context)
{
	const auto kind_error = ValidateReceiptKinds(snapshot);
	if (kind_error != SurfaceSupplyRecordError::None) return kind_error;
	if (snapshot.next_id == 0) return SurfaceSupplyRecordError::Identity;
	// Only a real multilayer profile may retain procurement commitments/history. Empty Classic worlds gain neither implicit imports nor an order on migration.
	if (!snapshot.records.empty() && context.profile != MultiLayerGameplayProfileID::MultilayerIndustry) return SurfaceSupplyRecordError::Profile;
	if (context.width == 0 || context.height == 0) return SurfaceSupplyRecordError::Geometry; // Original map dimensions must exist before interpreting any retained origin coordinate.
	if (context.stations.size() > StationID::End().base()) return SurfaceSupplyRecordError::Station;
	StationID previous_station = StationID::Invalid();
	// Each same-file normal station has one unambiguous identity. Duplicate/reordered context cannot let a caller choose the friendlier owner for a pending paid right.
	for (const auto &station : context.stations) {
		if (station.station >= StationID::End() || (previous_station != StationID::Invalid() && station.station <= previous_station)) return SurfaceSupplyRecordError::Station;
		previous_station = station.station;
	}
	SurfaceSupplyOrderID previous_id = 0;
	// Validate every original receipt before accepting any of the book. Historical packets are not inventory, while pending rights still require their exact original Surface receiver.
	for (const auto &record : snapshot.records) {
		// An original increasing issuance frontier excludes duplicate/reissued intents and prevents a retained receipt from reopening an unissued or retired retry identity.
		if (record.id == 0 || record.id <= previous_id || record.id >= snapshot.next_id) return SurfaceSupplyRecordError::Identity;
		previous_id = record.id;
		const auto *definition = GetSurfaceSupplyDefinition(record.label);
		if (definition == nullptr) return SurfaceSupplyRecordError::Cargo;
		if (record.quantity == 0 || record.quantity > definition->capacity_per_day) return SurfaceSupplyRecordError::Quantity; // A whole original purchase cannot exceed its own canonical company-day cap.
		const auto error = record.Native() != nullptr ? ValidateNativeReceipt(record, *record.Native(), context, definition) :
			ValidateLegacyReceipt(record, *record.Legacy(), context, definition);
		if (error != SurfaceSupplyRecordError::None) return error;
	}
	return SurfaceSupplyRecordError::None;
}

/** @copydoc CountSurfaceSupplyDayUsage */
std::optional<uint32_t> CountSurfaceSupplyDayUsage(const SurfaceSupplyOrderSnapshot &snapshot, CompanyID company, CargoLabel label, TimerGameEconomy::Date day)
{
	// Unsupported queries or oversized unadmitted transport cannot silently produce an allowance that a later command might mistake for unused budget.
	if (ValidateReceiptKinds(snapshot) != SurfaceSupplyRecordError::None || company >= CompanyID::End() || GetSurfaceSupplyDefinition(label) == nullptr) return std::nullopt;
	static_assert(MAX_LEGACY_SURFACE_SUPPLY_RECORDS * uint64_t{UINT16_MAX} < UINT32_MAX);
	uint32_t used = 0;
	// Pending commitments and irreversible fulfilled receipts spend the same owner's day allowance across every station. Cancellation frees only undispatched rights; older days and other owners/labels confer no current allowance.
	for (const auto &record : snapshot.records) {
		if (record.owner != company || record.label != label || record.purchase_day != day) continue;
		const auto *native = record.Native();
		// Every original legacy purchase still consumes its historical day cap,
		// including cancelled/closed stock. Only native cancellation releases it.
		if (native == nullptr || native->state == SurfaceSupplyOrderState::Pending || native->state == SurfaceSupplyOrderState::Fulfilled) used += record.quantity;
	}
	return used;
}

/** @copydoc FindSurfaceSupplyOrderRecord(const SurfaceSupplyOrderSnapshot &, SurfaceSupplyOrderID) */
const SurfaceSupplyOrderRecord *FindSurfaceSupplyOrderRecord(const SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id)
{
	const auto row = std::ranges::lower_bound(snapshot.records, id, {}, &SurfaceSupplyOrderRecord::id);
	// A gap or the next issuance frontier is not a retained paid intent.
	return row != snapshot.records.end() && row->id == id ? &*row : nullptr;
}

/** @copydoc FindSurfaceSupplyOrderRecord(SurfaceSupplyOrderSnapshot &, SurfaceSupplyOrderID) */
SurfaceSupplyOrderRecord *FindSurfaceSupplyOrderRecord(SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id)
{
	// The underlying book is mutable; reuse the exact const search without a
	// second index, issuance cursor or independent lookup implementation.
	return const_cast<SurfaceSupplyOrderRecord *>(FindSurfaceSupplyOrderRecord(std::as_const(snapshot), id));
}

/** @copydoc CanRecordSurfaceSupplyDispatch(const SurfaceSupplyOrderSnapshot &, SurfaceSupplyOrderID, uint16_t) */
bool CanRecordSurfaceSupplyDispatch(const SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id, uint16_t quantity)
{
	const auto *record = FindSurfaceSupplyOrderRecord(snapshot, id);
	// An unrelated native receipt or absent identity cannot back an imported
	// first-departure event; no empty fragment may consume historical rights.
	if (id == 0 || quantity == 0 || record == nullptr || record->Legacy() == nullptr) return false;
	const auto &legacy = *record->Legacy();
	// Check each subtraction before using the remaining original paid amount.
	// Closure/loss preserves history but does not enlarge that finite remainder.
	if (legacy.refunded > record->quantity || legacy.dispatched > record->quantity - legacy.refunded) return false;
	return quantity <= record->quantity - legacy.refunded - legacy.dispatched;
}

/** @copydoc RecordSurfaceSupplyDispatch(SurfaceSupplyOrderSnapshot &, SurfaceSupplyOrderID, uint16_t) */
bool RecordSurfaceSupplyDispatch(SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id, uint16_t quantity)
{
	if (!CanRecordSurfaceSupplyDispatch(snapshot, id, quantity)) return false;
	// The same single-threaded owner performs the checked transition; no
	// packet, callback or financial effect occurs between admission and update.
	FindSurfaceSupplyOrderRecord(snapshot, id)->Legacy()->dispatched += quantity;
	return true;
}

/** @copydoc SurfaceSupplyCustodyCheck::SurfaceSupplyCustodyCheck */
SurfaceSupplyCustodyCheck::SurfaceSupplyCustodyCheck(const SurfaceSupplyOrderSnapshot &snapshot, const SurfaceSupplyRecordContext &context,
	SurfaceSupplyPacketLookup lookup, size_t packet_count) : snapshot(snapshot), context(context), lookup(std::move(lookup)), expected_packets(packet_count)
{
	// Independent bounds and exact identity shape precede temporary allocation;
	// empty callbacks or malformed history cannot supply a convenient empty world.
	if (!this->lookup || packet_count > CargoPacketID::End().base() || ValidateSurfaceSupplyRecords(snapshot, context) != SurfaceSupplyRecordError::None) {
		this->Refuse(); return;
	}
	// Existing scalar validation supplies exact sorted identities and bounded
	// refunded/dispatched subtraction; do not create a second receipt validator.
	this->outstanding.resize(snapshot.records.size()); this->unshipped.resize(snapshot.records.size());
}

/** @copydoc SurfaceSupplyCustodyCheck::LookupPacket */
std::optional<SurfaceSupplyPacketFacts> SurfaceSupplyCustodyCheck::LookupPacket(CargoPacketID id)
{
	if (!this->valid) return std::nullopt;
	if (id >= CargoPacketID::End()) { this->Refuse(); return std::nullopt; }
	const auto facts = this->lookup(id);
	// Original identities and positive physical quantities must agree exactly;
	// a missing row cannot borrow another packet or a receipt's purchased count.
	if (!facts || facts->packet != id || facts->quantity == 0) { this->Refuse(); return std::nullopt; }
	switch (facts->source.type) {
		case SourceType::Industry:
		case SourceType::Town:
		case SourceType::Headquarters:
		case SourceType::Asteroid:
			// Canonical legacy links cannot relabel ordinary/natural cargo into a
			// paid entitlement. Original dialect conversion must occur beforehand.
			if (facts->order != 0 || facts->shipped) { this->Refuse(); return std::nullopt; }
			break;
		case SourceType::CompanySupply:
			// Original payer loss preserves purpose; reserved IDs, feeder credit
			// and shipped-without-link state cannot confer a different payment right.
			if ((facts->source.id != Source::Invalid && facts->source.id >= MAX_COMPANIES) || facts->feeder_share != Money{0} ||
				(facts->order == 0 && facts->shipped)) { this->Refuse(); return std::nullopt; }
			break;
		default: this->Refuse(); return std::nullopt;
	}
	return facts;
}

/** @copydoc SurfaceSupplyCustodyCheck::AddOwned */
bool SurfaceSupplyCustodyCheck::AddOwned(CargoPacketID packet, CargoLabel label, StationID station)
{
	// Container meaning and identity come from actual ownership. A late owner
	// cannot repair an orphan after final allocated-row validation has started.
	if (!this->valid || this->allocated_phase || label.Empty() || (station != StationID::Invalid() && station >= StationID::End())) return this->Refuse();
	const SurfaceSupplyStationView *queue = nullptr;
	if (station != StationID::Invalid()) {
		const auto found = std::ranges::lower_bound(this->context.stations, station, {}, &SurfaceSupplyStationView::station);
		// Only an original same-candidate station can be a goods-queue owner;
		// a copied checker cannot borrow a convenient station from the live world.
		if (found == this->context.stations.end() || found->station != station) return this->Refuse();
		queue = &*found;
	}
	const auto facts = this->LookupPacket(packet);
	if (!facts) return false;
	// Every actual reference is exclusive, including ordinary/unlinked cargo;
	// duplicate owners cannot hide behind matching labels or an empty paid book.
	if (!this->pending_owned.insert(packet).second || this->pending_owned.size() > this->expected_packets) return this->Refuse();
	if (facts->order == 0) return true;
	const auto *record = FindSurfaceSupplyOrderRecord(this->snapshot, facts->order);
	if (record == nullptr) return this->Refuse();
	const auto *legacy = record->Legacy();
	// Native issuance indices are historical only. B never saved an original
	// payer: a linked canonical fragment must retain the unavailable buyer,
	// resolve its exact legacy receipt and match the owner's actual cargo label.
	if (legacy == nullptr || record->label != label || facts->source.id != Source::Invalid) return this->Refuse();
	if (!facts->shipped) {
		// A never-shipped purchase originates in real Surface goods custody,
		// even when owner loss removed its historical station ID from the book.
		if (queue == nullptr || queue->layer != MapLayer::Surface) return this->Refuse();
		if (record->owner == CompanyID::Invalid()) {
			// B retains real source queues after owner loss, without a refundable
			// owner. Lost ownership never legitimizes unshipped carrier/job stock.
			if (!legacy->closed) return this->Refuse();
		} else {
			if (legacy->closed || station != record->origin) return this->Refuse();
		}
	}
	const size_t row = record - this->snapshot.records.data();
	const uint32_t remaining = record->quantity - legacy->refunded;
	const uint32_t never_shipped = remaining - legacy->dispatched;
	// Bound each addition by original finite history before forming a total;
	// consumed, refunded or previously dispatched units cannot reappear here.
	if (facts->quantity > remaining - this->outstanding[row]) return this->Refuse();
	const uint32_t outstanding = this->outstanding[row] + facts->quantity;
	uint32_t unshipped = this->unshipped[row];
	if (!facts->shipped) {
		if (facts->quantity > never_shipped - unshipped) return this->Refuse();
		unshipped += facts->quantity;
	}
	if (outstanding - unshipped > legacy->dispatched) return this->Refuse();
	this->outstanding[row] = outstanding; this->unshipped[row] = unshipped;
	return true;
}

/** @copydoc SurfaceSupplyCustodyCheck::ObserveAllocated */
bool SurfaceSupplyCustodyCheck::ObserveAllocated(CargoPacketID packet)
{
	// Original cardinality and strict ID order make a truncated/duplicated final
	// pass observable; packet allocation still cannot invent any missing owner.
	if (!this->valid || this->allocated_packets >= this->expected_packets ||
		(this->previous_packet != CargoPacketID::Invalid() && packet <= this->previous_packet)) return this->Refuse();
	this->allocated_phase = true;
	const auto facts = this->LookupPacket(packet);
	if (!facts) return false;
	const bool owned = this->pending_owned.erase(packet) != 0;
	// Paid purpose, including unlinked A supplies, requires a real observed
	// physical owner. Ordinary orphan policy remains with the original native
	// cargo validators; this pass grants no owner or payment to those rows.
	if (facts->source.IsCompanySupply() && !owned) return this->Refuse();
	this->previous_packet = packet; ++this->allocated_packets;
	return true;
}


/** @copydoc ComputeSurfaceSupplyOrderDigest */
uint64_t ComputeSurfaceSupplyOrderDigest(const SurfaceSupplyOrderSnapshot &snapshot)
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Explicit scalar bits; fixed eight-byte encoding excludes compiler layout and host byte order. */
	const auto field = [&digest](uint64_t value) {
		for (uint8_t byte = 0; byte < 8; ++byte) { digest ^= value & 0xFF; digest *= 1099511628211ULL; value >>= 8; }
	};
	field(snapshot.next_id); field(snapshot.records.size());
	// Issued order and explicit variant tags retain paid history after dispatch,
	// cancellation or loss; unavailable Bpayer/inflation facts stay unavailable.
	for (const auto &record : snapshot.records) {
		field(record.id); field(record.owner.base()); field(record.origin.base());
		for (const auto byte : record.label) field(byte);
		field(record.quantity); field(static_cast<uint32_t>(record.purchase_day.base()));
		if (const auto *native = record.Native()) {
			field(1); field(native->purchaser.base()); field(native->origin_x); field(native->origin_y);
			field(static_cast<uint64_t>(native->cost)); field(native->inflation_prices); field(static_cast<uint32_t>(native->purchase_year.base()));
			field(native->packet_buyer); field(to_underlying(native->state)); field(native->issued_packet.base());
		} else if (const auto *legacy = record.Legacy()) {
			field(2); field(static_cast<uint64_t>(legacy->unit_price)); field(static_cast<uint32_t>(legacy->calendar_date));
			field(legacy->refunded); field(legacy->dispatched); field(legacy->closed);
		} else {
			field(0); // An invalid detached variant cannot impersonate either admitted paid receipt era in diagnostics.
		}
	}
	return digest;
}
