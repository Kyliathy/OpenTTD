/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology.cpp Finite sparse seams and deterministic company claims. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md */

#include "stdafx.h"
#include "excavation.h"
#include "multilayer_recipe.h"
#include "tile_map.h"

#include "safeguards.h"

/**
 * Encode a fixed-width layer-local bucket, independent of map width or camera.
 * @param layer Rock layer.
 * @param x Plane column.
 * @param y Plane row.
 * @return Stable cache key, separate for both rock planes.
 */
static uint32_t GeologyBucketKey(MapLayer layer, uint x, uint y)
{
	return (layer == MapLayer::DeepUnderground ? 65536U : 0U) + (y / GEOLOGY_BUCKET_SIZE) * 256 + x / GEOLOGY_BUCKET_SIZE;
}

/**
 * Check whether an explicit local position is part of a canonical bounded seam.
 * @param deposit Geological shape authority.
 * @param x Plane column.
 * @param y Plane row.
 * @return True only inside one saved span.
 */
static bool DepositContains(const GeologyDeposit &deposit, uint x, uint y)
{
	// At most 64 spans are examined; a seam's bounding rectangle must never claim unrelated rock.
	for (const auto &span : deposit.spans) {
		// The row and both inclusive ends define the same geological owner, not a proximity claim.
		if (span.y == y && x >= span.first_x && x <= span.last_x) return true;
	}
	return false;
}

/**
 * Order seam spans independently of caller insertion order.
 * @param a First bounded span.
 * @param b Second bounded span.
 * @return Canonical row/column ordering for save and overlap validation.
 */
static bool GeologySpanBefore(const GeologySpan &a, const GeologySpan &b)
{
	return std::pair{a.y, a.first_x} < std::pair{b.y, b.first_x};
}

/**
 * Canonicalize one fresh record and reject malformed/unsupported reserve ownership.
 * @param deposit Candidate updated only by sorting its spans.
 * @return Specific structural/content refusal.
 */
static GeologyError ValidateDeposit(GeologyDeposit &deposit)
{
	// Only the two rock layers can carry persistent mineral seams.
	if (deposit.layer != MapLayer::Underground && deposit.layer != MapLayer::DeepUnderground) return GeologyError::InvalidShape;
	if (!MultiLayerMap::IsValidLayer(deposit.layer)) return GeologyError::InvalidShape;
	if (deposit.spans.empty()) return GeologyError::InvalidShape;
	if (deposit.spans.size() > MAX_GEOLOGY_SPANS) return GeologyError::Limit;
	if (to_underlying(deposit.hardness) > to_underlying(GeologyHardness::Basalt)) return GeologyError::InvalidShape;
	// Each resource retains the frozen layer/cargo meaning even if a later profile rearranges runtime slots.
	const auto recipes = GetMultiLayerMineRecipes();
	const auto recipe = std::ranges::find(recipes, deposit.resource, &MultiLayerMineRecipe::resource);
	if (recipe == recipes.end()) return GeologyError::InvalidResource;
	if (recipe->layer != deposit.layer) return GeologyError::InvalidResource;
	// Initial positive reserve and a bounded balance prevent load/editor paths from creating mineral debt or replenishment.
	if (deposit.initial_reserve == 0 || deposit.remaining_reserve > deposit.initial_reserve) return GeologyError::InvalidReserve;
	// Claims refer to real company slots, with one explicit unclaimed sentinel.
	if (deposit.claimant != CompanyID::Invalid() && deposit.claimant >= CompanyID::End()) return GeologyError::ClaimConflict;
	if (deposit.surveyed_companies >> MAX_COMPANIES != 0) return GeologyError::ClaimConflict;
	std::ranges::sort(deposit.spans, GeologySpanBefore);
	uint32_t cells = 0;
	// Validate every span/cell before insertion; void-fence policies can remove otherwise in-range coordinates.
	for (size_t i = 0; i < deposit.spans.size(); ++i) {
		const auto &span = deposit.spans[i];
		// Shapes are bounded to 4096 cells, and neither arithmetic underflow nor an out-of-map row is a legal seam.
		if (span.first_x > span.last_x || span.last_x >= Map::SizeX() || span.y >= Map::SizeY() || span.last_x - span.first_x >= 64) return GeologyError::InvalidShape;
		// Same-row spans cannot claim a tile twice within their own deposit.
		if (i != 0 && deposit.spans[i - 1].y == span.y && deposit.spans[i - 1].last_x >= span.first_x) return GeologyError::Overlap;
		cells += span.last_x - span.first_x + 1;
		for (uint x = span.first_x; x <= span.last_x; ++x) {
			if (!IsValidTile(TileXYL(x, span.y, deposit.layer))) return GeologyError::InvalidShape;
		}
	}
	// One exact initial ore quantum per shape cell lets later cuts destroy reserve once without fractional ore or another per-tile ledger.
	if (deposit.initial_reserve < cells || deposit.initial_reserve % cells != 0) return GeologyError::InvalidReserve;
	return GeologyError::None;
}

/**
 * Look up exact mutable authority; the spatial cache cannot own reserve balances.
 * @param id Stable record identity.
 * @return Record or nullptr.
 */
GeologyDeposit *GeologyDepositStore::Mutable(GeologyDepositID id)
{
	auto found = std::ranges::lower_bound(this->deposits, id, {}, &GeologyDeposit::id);
	// Sorted IDs may have holes after editor deletion; the next greater record is not a replacement owner.
	if (found == this->deposits.end() || found->id != id) return nullptr;
	return &*found;
}

/**
 * Read exact geological authority without exposing reserve mutation.
 * @param id Stable deposit identity.
 * @return Read-only record or nullptr.
 */
const GeologyDeposit *GeologyDepositStore::Get(GeologyDepositID id) const
{
	auto found = std::ranges::lower_bound(this->deposits, id, {}, &GeologyDeposit::id);
	// A deleted/invalid ID must not alias the next record in sorted storage.
	if (found == this->deposits.end() || found->id != id) return nullptr;
	return &*found;
}

/**
 * Resolve one playable tile from a bounded candidate list and canonical spans.
 * @param tile Explicit world coordinate.
 * @return Unique seam identity or invalid.
 */
GeologyDepositID GeologyDepositStore::Find(TileIndex tile) const
{
	if (!IsValidTile(tile)) return GeologyDepositID{UINT32_MAX};
	const auto layer = GetTileLayer(tile);
	// Coordinate coincidence across Surface/Space/rock planes never creates a geological claim.
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return GeologyDepositID{UINT32_MAX};
	const auto found = this->buckets.find(GeologyBucketKey(layer, TileX(tile), TilePlaneY(tile)));
	if (found == this->buckets.end()) return GeologyDepositID{UINT32_MAX};
	// At most 32 stable IDs, each with at most 64 spans; insertion rejects all overlaps instead of selecting by iteration order.
	for (const auto id : found->second) {
		const auto *deposit = this->Get(id);
		if (deposit == nullptr) continue;
		if (DepositContains(*deposit, TileX(tile), TilePlaneY(tile))) return id;
	}
	return GeologyDepositID{UINT32_MAX};
}

/**
 * Insert a finite seam only after every shape, overlap and cache bound passes.
 * @param deposit Fresh next-ID candidate, canonicalized locally.
 * @return Refusal or complete insertion.
 */
GeologyError GeologyDepositStore::Insert(GeologyDeposit deposit)
{
	// Deleted IDs cannot be recycled into new reserves; all-ones remains permanently invalid.
	if (deposit.id != this->NextID() || this->next_id == UINT32_MAX) return GeologyError::InvalidID;
	if (this->deposits.size() >= MAX_GEOLOGY_DEPOSITS) return GeologyError::Limit;
	const auto error = ValidateDeposit(deposit);
	if (error != GeologyError::None) return error;
	std::set<uint32_t> touched;
	// Complete shape validation precedes cache/authority writes so an intersecting candidate cannot partially claim rock.
	for (const auto &span : deposit.spans) {
		for (uint x = span.first_x; x <= span.last_x; ++x) {
			if (this->Find(TileXYL(x, span.y, deposit.layer)) != GeologyDepositID{UINT32_MAX}) return GeologyError::Overlap;
			touched.insert(GeologyBucketKey(deposit.layer, x, span.y));
		}
	}
	// Prepare replacement cache entries before publishing the record; bucket overflow changes no existing claim.
	auto updated_buckets = this->buckets;
	for (const uint32_t key : touched) {
		auto &entries = updated_buckets[key];
		if (entries.size() >= MAX_GEOLOGY_BUCKET_ENTRIES) return GeologyError::Limit;
		entries.push_back(deposit.id);
	}
	this->deposits.push_back(std::move(deposit));
	this->buckets.swap(updated_buckets);
	++this->next_id;
	return GeologyError::None;
}

/**
 * Establish one company owner without a second mine/job reserve ledger.
 * @param id Nondepleted geological identity.
 * @param company Real company slot.
 * @return Refusal or ownership acceptance.
 */
GeologyError GeologyDepositStore::Claim(GeologyDepositID id, CompanyID company)
{
	auto *deposit = this->Mutable(id);
	if (deposit == nullptr) return GeologyError::InvalidID;
	if (company >= CompanyID::End()) return GeologyError::ClaimConflict;
	if (deposit->remaining_reserve == 0) return GeologyError::Depleted;
	// A company's multiple disjoint mine/job footprints may share a seam, but a rival cannot acquire its finite reserve.
	if (deposit->claimant != CompanyID::Invalid() && deposit->claimant != company) return GeologyError::ClaimConflict;
	deposit->claimant = company;
	return GeologyError::None;
}

/**
 * Release company ownership while preserving consumed and remaining quantities.
 * @param id Existing deposit.
 * @param company Current claimant.
 * @return Refusal or completed release.
 */
GeologyError GeologyDepositStore::ReleaseClaim(GeologyDepositID id, CompanyID company)
{
	auto *deposit = this->Mutable(id);
	if (deposit == nullptr) return GeologyError::InvalidID;
	// An invalid/unclaimed sentinel cannot authorize a release on behalf of another producer.
	if (company >= CompanyID::End() || deposit->claimant != company) return GeologyError::ClaimConflict;
	deposit->claimant = CompanyID::Invalid();
	return GeologyError::None;
}

/**
 * Debit ore once for a completed producer transaction, never for an estimate or discovery.
 * @param id Claimed deposit.
 * @param company Authorized claimant.
 * @param amount Whole tonnes removed from recoverable reserve.
 * @return Refusal or one complete balance debit.
 */
GeologyError GeologyDepositStore::ConsumeReserve(GeologyDepositID id, CompanyID company, uint64_t amount)
{
	auto *deposit = this->Mutable(id);
	if (deposit == nullptr) return GeologyError::InvalidID;
	// Only the actual owner can spend reserve; the unclaimed sentinel is never a valid producer.
	if (company >= CompanyID::End() || deposit->claimant != company) return GeologyError::ClaimConflict;
	if (amount > deposit->remaining_reserve) return GeologyError::Depleted;
	deposit->remaining_reserve -= amount;
	return GeologyError::None;
}

/** @copydoc GeologyDepositStore::TransferMineClaim */
GeologyError GeologyDepositStore::TransferMineClaim(GeologyDepositID id, CompanyID old_company, CompanyID new_company)
{
	auto *deposit = this->Mutable(id);
	if (deposit == nullptr) return GeologyError::InvalidID;
	// Transfer only the actual exclusive owner to a real company or the explicit abandoned sentinel; exhausted ore stays exhausted.
	if (old_company >= CompanyID::End() || deposit->claimant != old_company ||
			(new_company != CompanyID::Invalid() && new_company >= CompanyID::End())) return GeologyError::ClaimConflict;
	deposit->claimant = new_company;
	return GeologyError::None;
}

/**
 * Remove only geological editor/generation authority; normal demolition releases a claim instead.
 * @param id Existing record identity.
 * @return Whether a record was removed; monotonic identity remains unchanged.
 */
bool GeologyDepositStore::Erase(GeologyDepositID id)
{
	const auto found = std::ranges::lower_bound(this->deposits, id, {}, &GeologyDeposit::id);
	// A missing identity cannot delete its next sorted neighbor.
	if (found == this->deposits.end() || found->id != id) return false;
	this->deposits.erase(found);
	this->RebuildIndex();
	return true;
}

/** Reconstruct deterministic bucket candidates from validated authority; no reserve or visibility is changed. */
void GeologyDepositStore::RebuildIndex()
{
	std::map<uint32_t, std::vector<GeologyDepositID>> fresh;
	// Saved records/spans are authoritative; one deposit is listed once per touched bucket in increasing ID order.
	for (const auto &deposit : this->deposits) {
		std::set<uint32_t> touched;
		for (const auto &span : deposit.spans) {
			// Jump to the next bucket boundary; every occupied bucket is included without indexing each seam cell separately.
			for (uint x = span.first_x; x <= span.last_x; x += GEOLOGY_BUCKET_SIZE - x % GEOLOGY_BUCKET_SIZE) {
				touched.insert(GeologyBucketKey(deposit.layer, x, span.y));
			}
		}
		for (uint32_t key : touched) fresh[key].push_back(deposit.id);
	}
	this->buckets.swap(fresh);
}

/**
 * Restore only bounded canonical records with a cursor that cannot recycle historical ore identities.
 * @param records Saved authority, including possible deleted-ID holes.
 * @param saved_next_id Monotonic cursor strictly beyond all retained records.
 * @return Refusal or complete replacement and rebuilt lookup.
 */
GeologyError GeologyDepositStore::RestoreRecords(std::span<const GeologyDeposit> records, uint32_t saved_next_id)
{
	if (records.size() > MAX_GEOLOGY_DEPOSITS) return GeologyError::Limit;
	GeologyDepositStore prepared;
	// Replay canonical authority into a separate candidate so any bad ID/shape/reserve/overlap leaves the live store untouched.
	for (const auto &record : records) {
		// Retained IDs may skip deleted records but can never decrease, duplicate or exceed the saved fresh-ID cursor.
		if (record.id.base() < prepared.next_id || record.id.base() >= saved_next_id) return GeologyError::InvalidID;
		prepared.next_id = record.id.base();
		const auto error = prepared.Insert(record);
		if (error != GeologyError::None) return error;
	}
	prepared.next_id = saved_next_id;
	*this = std::move(prepared);
	return GeologyError::None;
}

/** @return Cache payload separate from fixed sparse record/span storage and allocator-specific map-node bookkeeping. */
size_t GeologyDepositStore::IndexPayloadBytes() const
{
	size_t bytes = this->buckets.size() * sizeof(decltype(this->buckets)::value_type);
	// Cache payload depends on occupied chunks, not on allocating a lookup object for every map tile.
	for (const auto &[key, entries] : this->buckets) bytes += entries.size() * sizeof(GeologyDepositID);
	return bytes;
}

/**
 * Debit complete-cut loss from the sole finite balance without taking another producer's company claim.
 * @param id Actual touched seam.
 * @param company Real validated cutter company.
 * @param amount Recoverable ore destroyed by this cut, not delivered freight.
 * @param expected_remaining Exact preflight balance.
 * @return Complete acceptance or unchanged identity/owner/balance refusal.
 */
GeologyError GeologyDepositStore::DebitExcavationReserve(GeologyDepositID id, CompanyID company, uint64_t amount, uint64_t expected_remaining)
{
	auto *deposit = this->Mutable(id);
	if (deposit == nullptr) return GeologyError::InvalidID;
	if (company.base() >= MAX_COMPANIES) return GeologyError::ClaimConflict;
	// Only unclaimed/same-company ore may be destroyed; this operation never acquires or releases a mine's exclusive claim.
	if (deposit->claimant != CompanyID::Invalid() && deposit->claimant != company) return GeologyError::ClaimConflict;
	if (deposit->remaining_reserve != expected_remaining || amount > expected_remaining) return GeologyError::InvalidReserve; // Stale or excessive loss cannot underflow the sole balance.
	deposit->remaining_reserve -= amount;
	return GeologyError::None;
}
