/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver.h The Surface Earth reclamation site and Rock crusher (Plan 5 Group 8): native first-party
 * industries installed after GRF loading, whose bounded intake, interval processing, fees and saved side state are
 * owned here. Delivery reaches them through the ordinary station catchment and economy path.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-d-recipes-and-bootstrap.md, Group 7 (frozen recipes)
 */

#ifndef SPOIL_RECEIVER_H
#define SPOIL_RECEIVER_H

#include "cargo_type.h"
#include "company_type.h"
#include "economy_type.h"
#include "industry_type.h"
#include "newgrf_config.h"
#include "station_type.h"

struct Industry;
struct Station;

/** Which spoil a receiver takes; the saved value. */
enum class SpoilReceiverKind : uint8_t {
	Earth, ///< Earth reclamation/disposal site, taking ERTH.
	Rock, ///< Rock crusher/disposal site, taking ROCK.
	End, ///< End marker; never a live receiver.
};

/** What a receiver does with its stock; the saved value. */
enum class SpoilReceiverMode : uint8_t {
	Process, ///< Earth is disposed; Rock is crushed into aggregate and residue.
	SurplusDisposal, ///< Rock only: every batch is disposed, at the higher surplus fee.
	End, ///< End marker; never a live mode.
};

/** One frozen receiver recipe (r2mlvs1-d Group 7); quantities in whole tonnes, fees in internal Money units per tonne. */
struct SpoilReceiverRecipe {
	CargoLabel cargo; ///< Spoil label taken.
	uint16_t batch_t; ///< Tonnes per processing batch.
	uint16_t stock_cap_t; ///< Most spoil the receiver holds.
	uint16_t interval_intake_t; ///< Most spoil admitted per interval; also the most processed per interval.
	Money fee_per_t; ///< Fee per admitted tonne in Process mode.
	Money surplus_fee_per_t; ///< Fee per admitted tonne in SurplusDisposal mode, or 0 where that mode does not exist.
	uint16_t aggregate_per_batch_t; ///< Aggregate made per crushing batch, or 0.
	uint16_t residue_per_batch_t; ///< Residue recorded per crushing batch, or 0.
	uint16_t aggregate_cap_t; ///< Most aggregate held, or 0.
};

/** Synchronized ticks in one receiver interval (r2mlvs1-d Group 7's production interval). */
static constexpr uint16_t SPOIL_RECEIVER_INTERVAL_TICKS = 74;

/** Receiver state beside its industry; the industry's own accepted stock is the spoil held. */
struct SpoilReceiver {
	IndustryID industry = IndustryID::Invalid(); ///< The receiver industry.
	SpoilReceiverKind kind = SpoilReceiverKind::End; ///< Which spoil it takes.
	SpoilReceiverMode mode = SpoilReceiverMode::Process; ///< Processing choice.
	uint16_t interval_ticks = 0; ///< Ticks into the current interval, below SPOIL_RECEIVER_INTERVAL_TICKS.
	uint16_t interval_intake = 0; ///< Tonnes admitted in the current interval.
	uint16_t aggregate = 0; ///< Construction aggregate held (Rock).
	uint64_t disposed_t = 0; ///< Tonnes disposed: Earth, and Rock in surplus disposal.
	uint64_t crushed_t = 0; ///< Rock tonnes crushed.
	uint64_t aggregate_t = 0; ///< Aggregate tonnes ever made.
	uint64_t residue_t = 0; ///< Residue tonnes ever recorded.
	uint64_t fees = 0; ///< Money units ever charged at intake.
	uint64_t revision = 1; ///< Change stamp, bumped when stock, intake, totals or mode change.

	/**
	 * @param other Receiver to compare.
	 * @return Whether every field matches.
	 */
	bool operator==(const SpoilReceiver &other) const = default;
};

/** Refusals of the receiver commands. */
enum class SpoilReceiverError : uint8_t {
	None, ///< Accepted.
	InvalidIndustry, ///< No receiver industry.
	InvalidMode, ///< Unknown mode, or a mode this kind does not have.
	NotFounder, ///< Only the founding company (or the game script) may choose the mode.
};

/**
 * @param kind Live kind.
 * @return Its frozen recipe.
 */
const SpoilReceiverRecipe &GetSpoilReceiverRecipe(SpoilReceiverKind kind);

/**
 * Install both receiver industries and their tiles after GRF polishing, reusing saved IIDS/TIDS identities.
 * @param activated Actual activated GRF configuration.
 * @return Whether the exact first-party package is active and both definitions are installed.
 */
bool InitialiseSpoilReceiverDefinitions(const GRFConfigList &activated);

/**
 * @param kind Live kind.
 * @return Its installed industry type, or IT_INVALID when the definitions are not installed.
 */
IndustryType GetSpoilReceiverIndustryType(SpoilReceiverKind kind);

/**
 * @param type Industry type.
 * @return The receiver kind of a native receiver definition, or End for every other type.
 */
SpoilReceiverKind GetSpoilReceiverKind(IndustryType type);

/**
 * @param cargo Cargo type.
 * @return The receiver kind that takes it, or End for any cargo that is not ERTH or ROCK.
 */
SpoilReceiverKind GetSpoilReceiverKindForCargo(CargoType cargo);

/**
 * @param industry Industry identity.
 * @return Its receiver state, or nullptr.
 */
const SpoilReceiver *GetSpoilReceiver(IndustryID industry);

/** @return Every receiver in industry ID order. */
std::vector<SpoilReceiver> CaptureSpoilReceivers();

/**
 * Give a newly built receiver industry its state.
 * @param industry The built industry; any other type is ignored.
 */
void RegisterSpoilReceiver(const Industry &industry);

/**
 * Drop the state of a removed industry; nothing happens for any other industry.
 * @param industry Industry identity.
 */
void UnregisterSpoilReceiver(IndustryID industry);

/** Clear every receiver and any unpublished load staging when the world initializes. */
void ResetSpoilReceiverState();

/**
 * @param industry Receiver industry.
 * @return Tonnes it can admit now: the smaller of its free stock and its interval intake left; 0 for any other industry.
 */
uint GetSpoilReceiverRoom(const Industry &industry);

/**
 * The spoil a station can deliver now: the room of every receiver of that spoil in its catchment that would take it
 * from this station, under the same filters as the delivery to industries.
 * @param station Unloading station.
 * @param cargo Cargo type.
 * @return Total room, or UINT_MAX for any cargo that is not spoil.
 */
uint GetSpoilReceiverRoomAt(const Station &station, CargoType cargo);

/**
 * Admit delivered spoil into a receiver, recording the interval intake and charging the fee once. The caller adds the
 * admitted tonnes to the industry's stock, as for any delivery.
 * @param industry Receiver industry.
 * @param cargo Delivered cargo.
 * @param requested Tonnes offered.
 * @param company Delivering company, which pays the fee.
 * @return Tonnes admitted, at most the receiver's room; 0 for any other industry or cargo.
 */
uint AdmitSpoilToReceiver(Industry &industry, CargoType cargo, uint requested, CompanyID company);

/**
 * One synchronized tick: at each interval end the intake allowance resets and up to one interval's worth of complete
 * batches is processed. Full aggregate pauses crushing, and the stock waits.
 * @param industry Receiver industry; any other industry is ignored.
 */
void AdvanceSpoilReceiver(Industry &industry);

/**
 * Choose crushing or surplus disposal for a Rock crusher.
 * @param industry Receiver industry.
 * @param mode New mode.
 * @param execute Apply; otherwise only check.
 * @return None, InvalidIndustry, InvalidMode or NotFounder.
 */
SpoilReceiverError SetSpoilReceiverMode(IndustryID industry, SpoilReceiverMode mode, bool execute);

/**
 * Hold structurally checked loaded rows until the industries are rebuilt.
 * @param rows Loaded rows in industry ID order.
 */
void StageLoadedSpoilReceivers(std::vector<SpoilReceiver> rows);

/**
 * Publish the staged rows after AfterLoadGame: every receiver industry needs exactly one row of its own kind with
 * in-range fields, and every row a receiver industry.
 * @return Whether the rows were accepted; nothing staged also succeeds.
 */
bool AcceptLoadedSpoilReceivers();

/** @return Diagnostic digest of every receiver field, in industry ID order. */
uint64_t ComputeSpoilReceiverDigest();

/** Clear unpublished receiver transport (saveload owner). */
void ResetSpoilReceiverLoadStaging();

/** Check the copied receiver family of a save preview without touching live owners (saveload owner). */
void ValidateSpoilReceiverLoadPreview();

#endif /* SPOIL_RECEIVER_H */
