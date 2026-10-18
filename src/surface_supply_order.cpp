/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order.cpp Pure original-owner order/receipt validation and shared day allowance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * No function here installs an owner, allocates cargo, charges/refunds cash or activates saved orders.
 */
#include "stdafx.h"
#include "surface_supply_order.h"
#include "multilayer_bootstrap.h"
#include "safeguards.h"

/** @copydoc ValidateSurfaceSupplyRecords */
SurfaceSupplyRecordError ValidateSurfaceSupplyRecords(const SurfaceSupplyOrderSnapshot &snapshot, const SurfaceSupplyRecordContext &context)
{
	if (snapshot.records.size() > MAX_SURFACE_SUPPLY_RECORDS) return SurfaceSupplyRecordError::Size;
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
		// Original payer is immutable history; only a real-domain owner or retired sentinel can retain current control. A retired payer need not exist in this candidate's PLYR.
		if (record.purchaser >= CompanyID::End() || (record.owner != CompanyID::Invalid() && record.owner >= CompanyID::End()) ||
			(record.packet_buyer != Source::Invalid && record.packet_buyer != record.purchaser.base())) return SurfaceSupplyRecordError::Company;
		// Origins remain Surface-local historical coordinates. A deleted terminal station may stay in history but cannot become a hidden-layer destination or future permission.
		if (record.origin >= StationID::End() || record.origin_x >= context.width || record.origin_y >= context.height) return SurfaceSupplyRecordError::Geometry;
		const auto *definition = GetSurfaceSupplyDefinition(record.label);
		if (definition == nullptr) return SurfaceSupplyRecordError::Cargo;
		if (record.quantity == 0 || record.quantity > definition->capacity_per_day) return SurfaceSupplyRecordError::Quantity; // A whole original purchase cannot exceed its own canonical company-day cap.
		if (record.purchase_year < definition->introduction_year) return SurfaceSupplyRecordError::Date;
		const auto price = CalculateSurfaceSupplyCost(record.label, record.quantity, record.inflation_prices);
		if (!price || record.cost != *price) return SurfaceSupplyRecordError::Price; // Frozen original native pricing must match exactly; current inflation or a forged free receipt cannot finance a refund.
		switch (record.state) {
			case SurfaceSupplyOrderState::Pending: {
				// Only a still-existing owner and the same owned Surface freight receiver may hold a refundable undispatched right; the open world supplies no missing reference.
				if (record.owner >= CompanyID::End() || !context.companies[record.owner.base()]) return SurfaceSupplyRecordError::Company;
				const auto station = std::ranges::lower_bound(context.stations, record.origin, {}, &SurfaceSupplyStationView::station);
				if (station == context.stations.end() || station->station != record.origin || station->owner != record.owner || station->layer != MapLayer::Surface ||
					!station->freight_facility || station->x != record.origin_x || station->y != record.origin_y) return SurfaceSupplyRecordError::Station;
				if (record.issued_packet != CargoPacketID::Invalid()) return SurfaceSupplyRecordError::Packet;
				break;
			}
			case SurfaceSupplyOrderState::Fulfilled:
				if (record.issued_packet >= CargoPacketID::End()) return SurfaceSupplyRecordError::Packet;
				break; // The original index is only issuance history; consumed/merged packets need not exist and cannot grant a second shipment/refund.
			case SurfaceSupplyOrderState::Cancelled:
				if (record.issued_packet != CargoPacketID::Invalid()) return SurfaceSupplyRecordError::Packet;
				break;
			case SurfaceSupplyOrderState::Lost:
				if (record.owner != CompanyID::Invalid() || record.issued_packet != CargoPacketID::Invalid()) return SurfaceSupplyRecordError::State; // Company loss leaves neither a live refund owner nor a dispatched packet.
				break;
			default:
				return SurfaceSupplyRecordError::State;
		}
	}
	return SurfaceSupplyRecordError::None;
}

/** @copydoc CountSurfaceSupplyDayUsage */
std::optional<uint32_t> CountSurfaceSupplyDayUsage(const SurfaceSupplyOrderSnapshot &snapshot, CompanyID company, CargoLabel label, TimerGameEconomy::Date day)
{
	// Unsupported queries or oversized unadmitted transport cannot silently produce an allowance that a later command might mistake for unused budget.
	if (snapshot.records.size() > MAX_SURFACE_SUPPLY_RECORDS || company >= CompanyID::End() || GetSurfaceSupplyDefinition(label) == nullptr) return std::nullopt;
	static_assert(MAX_SURFACE_SUPPLY_RECORDS * uint64_t{UINT16_MAX} < UINT32_MAX);
	uint32_t used = 0;
	// Pending commitments and irreversible fulfilled receipts spend the same owner's day allowance across every station. Cancellation frees only undispatched rights; older days and other owners/labels confer no current allowance.
	for (const auto &record : snapshot.records) {
		if (record.owner != company || record.label != label || record.purchase_day != day) continue;
		if (record.state == SurfaceSupplyOrderState::Pending || record.state == SurfaceSupplyOrderState::Fulfilled) used += record.quantity;
	}
	return used;
}
