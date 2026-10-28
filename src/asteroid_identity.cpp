/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_identity.cpp Compact live storage and monotonic identities, with disposable lookup only.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups12/14/14B.
 */
#include "stdafx.h"
#include "asteroid.h"
#include "safeguards.h"

/** @copydoc AsteroidField::operator== */
bool AsteroidField::operator==(const AsteroidField &other) const
{
	return this->generation_version == other.generation_version && this->lifecycle_version == other.lifecycle_version && this->width == other.width && this->height == other.height &&
		this->seed == other.seed && this->elapsed_ticks == other.elapsed_ticks && this->density == other.density &&
		this->band == other.band && this->starter_input == other.starter_input && this->renewal == other.renewal &&
		this->next_body_id == other.next_body_id && this->bodies == other.bodies && this->surveys == other.surveys && this->knowledge == other.knowledge;
}

/** @copydoc GetAsteroidBodiesInIDOrder */
std::vector<const AsteroidBody *> GetAsteroidBodiesInIDOrder(const AsteroidField &field)
{
	std::vector<const AsteroidBody *> ordered;
	ordered.reserve(field.bodies.size());
	for (const auto &body : field.bodies) ordered.push_back(&body);
	/** @param a First live record. @param b Second live record. @return Stable identity order, independent of dense slot history. */
	std::ranges::sort(ordered, [](const auto *a, const auto *b) { return a->id < b->id; });
	return ordered;
}

/** @copydoc ValidateAsteroidBodyIdentities */
bool ValidateAsteroidBodyIdentities(const AsteroidField &field)
{
	if (field.bodies.size() > MAX_ORBITAL_BODIES) return false;
	OrbitalBodyID previous = INVALID_ORBITAL_BODY;
	// Validate raw copied records, not a possibly stale cache inherited from
	// another field. Issued gaps are legitimate history; duplicate IDs are not.
	for (const auto *body : GetAsteroidBodiesInIDOrder(field)) {
		if (body->id.base() >= field.next_body_id || body->id == previous) return false;
		previous = body->id;
	}
	return true;
}

/** @copydoc RebuildAsteroidBodyLookup */
bool RebuildAsteroidBodyLookup(AsteroidField &field)
{
	if (!ValidateAsteroidBodyIdentities(field)) return false;
	std::unordered_map<uint32_t, size_t> slots;
	slots.reserve(field.bodies.size());
	for (size_t slot = 0; slot < field.bodies.size(); ++slot) slots.emplace(field.bodies[slot].id.base(), slot);
	field.body_slots = std::move(slots);
	return true;
}

/** @copydoc ResolveAsteroidSelection */
std::optional<OrbitalBodyID> ResolveAsteroidSelection(const AsteroidField &field, std::optional<OrbitalBodyID> selected)
{
	// A surviving selected ID retains its meaning even after vector compaction;
	// missing selections recover deterministically without borrowing the old slot.
	if (selected && FindAsteroidBody(field, *selected) != nullptr) return selected;
	const auto lowest = std::ranges::min_element(field.bodies, {}, &AsteroidBody::id);
	return lowest == field.bodies.end() ? std::nullopt : std::optional{lowest->id};
}

/** @copydoc RemoveAsteroidKnowledge */
void RemoveAsteroidKnowledge(AsteroidField &field, OrbitalBodyID id)
{
	/** @param receipt One original disclosure. @return Whether it belongs to the terminal body rather than any surviving ID. */
	std::erase_if(field.knowledge, [id](const auto &receipt) { return receipt.body == id; });
}

/** @copydoc GetAsteroidIdentityRecords */
std::vector<AsteroidIdentityRecord> GetAsteroidIdentityRecords(const AsteroidField &field)
{
	std::vector<AsteroidIdentityRecord> records;
	records.reserve(field.bodies.size());
	for (const auto *body : GetAsteroidBodiesInIDOrder(field)) records.push_back({body->id, body->resource_class});
	return records;
}

/** @copydoc ValidateAsteroidIdentityRecords */
bool ValidateAsteroidIdentityRecords(std::span<const AsteroidIdentityRecord> records, uint32_t next_body_id)
{
	if (records.size() > MAX_ORBITAL_BODIES) return false;
	std::optional<OrbitalBodyID> previous;
	// Candidate row order is evidence from the decoded owner, not an invitation
	// to sort corrupt duplicate/descending IDs into apparent validity.
	for (const auto &record : records) {
		if (record.id.base() >= next_body_id || to_underlying(record.resource_class) > to_underlying(AsteroidClass::Rich) ||
			(previous && record.id <= *previous)) return false;
		previous = record.id;
	}
	return true;
}

/** @copydoc FindAsteroidIdentity */
const AsteroidIdentityRecord *FindAsteroidIdentity(std::span<const AsteroidIdentityRecord> records, OrbitalBodyID id)
{
	const auto found = std::ranges::lower_bound(records, id, {}, &AsteroidIdentityRecord::id);
	return found != records.end() && found->id == id ? &*found : nullptr;
}

/** @copydoc ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord>, std::span<const AsteroidIdentityRecord>) */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, std::span<const AsteroidIdentityRecord> bodies)
{
	// A previous candidate or currently open world cannot lend missing identity
	// evidence; each receipt joins only to this candidate's actual body rows.
	if (records.size() > bodies.size() || !ValidateAsteroidKnowledgeShape(records)) return false;
	for (const auto &record : records) if (FindAsteroidIdentity(bodies, record.body) == nullptr) return false;
	return true;
}

/** @copydoc FindAsteroidBody(const AsteroidField &, OrbitalBodyID) */
const AsteroidBody *FindAsteroidBody(const AsteroidField &field, OrbitalBodyID id)
{
	if (id == INVALID_ORBITAL_BODY) return nullptr;
	const auto found = field.body_slots.find(id.base());
	// A decoded/copy-edited vector can retain another layout's map. Verify the
	// cached identity before borrowing it; lookup itself never repairs state.
	if (found != field.body_slots.end() && found->second < field.bodies.size() && field.bodies[found->second].id == id) return &field.bodies[found->second];
	// Uninstalled copied records still have meaningful IDs. A read-only fallback
	// supports validation and staged data without publishing a second registry.
	const auto body = std::ranges::find(field.bodies, id, &AsteroidBody::id);
	return body == field.bodies.end() ? nullptr : &*body;
}

/** @copydoc FindAsteroidBody(AsteroidField &, OrbitalBodyID) */
AsteroidBody *FindAsteroidBody(AsteroidField &field, OrbitalBodyID id)
{
	return const_cast<AsteroidBody *>(FindAsteroidBody(std::as_const(field), id));
}

/** @copydoc InsertAsteroidBody */
bool InsertAsteroidBody(AsteroidField &field, AsteroidBody body)
{
	// Only a fresh issuance may grow bounded live storage. A recycled ID, full
	// field, unrebuilt layout or exhausted cursor must leave issuance untouched.
	if (field.next_body_id == UINT32_MAX || body.id.base() != field.next_body_id || field.bodies.size() >= MAX_ORBITAL_BODIES ||
		field.body_slots.size() != field.bodies.size() || field.body_slots.contains(body.id.base())) return false;
	const auto slot = field.bodies.size();
	try {
		field.bodies.push_back(std::move(body));
		try {
			field.body_slots.emplace(field.next_body_id, slot);
		} catch (const std::bad_alloc &) {
			field.bodies.pop_back();
			return false;
		}
	} catch (const std::bad_alloc &) {
		return false;
	}
	++field.next_body_id;
	return true;
}

/** @copydoc EraseAsteroidBody */
bool EraseAsteroidBody(AsteroidField &field, OrbitalBodyID id)
{
	const auto found = field.body_slots.find(id.base());
	// Structural mutation requires a rebuilt lookup; a stale borrowed layout
	// must never erase a different body or silently consume its saved reserve.
	if (found == field.body_slots.end() || field.body_slots.size() != field.bodies.size() ||
		found->second >= field.bodies.size() || field.bodies[found->second].id != id) return false;
	const size_t slot = found->second;
	if (slot != field.bodies.size() - 1) {
		field.bodies[slot] = std::move(field.bodies.back());
		field.body_slots.at(field.bodies[slot].id.base()) = slot;
	}
	field.bodies.pop_back();
	field.body_slots.erase(found);
	return true;
}
