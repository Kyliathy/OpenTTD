/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_transfer.h Whole paid electrical path preparation and once-only shared-owner settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5
 */

#ifndef UTILITY_POWER_TRANSFER_H
#define UTILITY_POWER_TRANSFER_H

#include "utility_power.h"

/** Original paid transition preceded by its exact installed local electrical inlet; these references create no physical or energy owner. */
struct UtilityTransferLeg {
	UtilityLinkID edge{UINT32_MAX}; ///< Existing original paid riser, never a generic one-hop fallback.
	std::vector<UtilityLinkID> cells; ///< Ordered installed cable, including the sending source/arriving housing and exact machinery port.
	/** @param other Candidate physical leg. @return Equality of original edge and complete ordered inlet references. */
	bool operator==(const UtilityTransferLeg &other) const = default;
};

/** Explicit positive whole transfer request; only the final requested destination receives availability. */
struct UtilityPathTransferRequest {
	UtilityGrantID grant; ///< Exact current source reservation and accounting epoch.
	UtilityConsumerID destination{UINT32_MAX}; ///< Existing same-company electrical request, never a new output account.
	TileIndex source_port = INVALID_TILE; ///< Explicit installed source cell; imported shares must continue from their original arriving housing.
	uint64_t quantity_j = 0; ///< Positive whole quantity for preparation, or upper bound for a pure residual offer.
	std::vector<UtilityTransferLeg> legs; ///< Complete ordered original paid path; no implicit intermediate cable or grant.
	/** @param other Candidate request. @return Exact owned grant, endpoint, quantity and physical references. */
	bool operator==(const UtilityPathTransferRequest &other) const = default;
};

/** Pure bounded availability quote; it holds no reservation, carry or generated/imported energy. */
struct UtilityPathTransferOffer {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Whole refusal or a positive currently transferable quantity.
	uint64_t quantity_j = 0; ///< Maximum whole quantity up to the request, bounded by eligible source slices, destination room and shared physical residuals.
};

/** Exact transient whole-path proposal; authoritative quantities remain solely in the existing shared state. */
struct UtilityPathTransferProposal {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Only None permits whole commit revalidation.
	UtilityPathTransferRequest request; ///< Exact references/epoch/quantity to revalidate, never a copied authoritative balance.
	CompanyID owner = CompanyID::Invalid(); ///< Actual source/destination controller at preparation.
	uint64_t sequence_before = 0; ///< Saved shared sequence observed before the proposed whole action.
	uint64_t seal = 0; ///< Field-wise request fingerprint rejects modified references or quantities.
};

/** @param request Complete paid path and positive quantity upper bound. @return Pure current residual offer/refusal without reserving, publishing or changing any identity, quantity, topology or clock. Paid ElectricalRiser roles use this whole-path family; the older PrepareUtilityMutation(TransferLayer) accepts genuine ElectricalPortal roles only. */
UtilityPathTransferOffer GetUtilityPathTransferOffer(const UtilityPathTransferRequest &request);
/** @param request Complete paid path and exact positive quantity. @return Pure sealed whole-path proposal, or unchanged refusal; every original source and physical participant is revalidated before any endpoint may change. */
UtilityPathTransferProposal PrepareUtilityPathTransfer(const UtilityPathTransferRequest &request);
/** @param proposal Exact previously prepared request. @return One atomic original-source-preserving transfer/sequence receipt, or unchanged failure; no intermediate consumer grant or generation event is created. */
UtilityMutationReceipt CommitUtilityPathTransfer(const UtilityPathTransferProposal &proposal);

#endif /* UTILITY_POWER_TRANSFER_H */
