/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo.h Historical natural provenance over explicit original or copied field metadata.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group16.
 * These queries grant no live target, lease, reserve, packet or inventory rights.
 */
#ifndef ASTEROID_CARGO_H
#define ASTEROID_CARGO_H
#include "asteroid.h"
#include "source_type.h"

/** @param origin Complete original packet provenance. @param field Prevalidated original/copied field metadata. @return Whether the ID was issued, its mineral is known, and any surviving body agrees; an absent issued body remains historical without allocation. */
bool ValidateAsteroidCargoOrigin(const AsteroidCargoOrigin &origin, const AsteroidField &field);
/** @param origin Complete original packet provenance. @param bodies Validated sorted same-file live identity/class rows. @param next_body_id Same-file issuance cursor. @return The same historical-origin check using only candidate metadata, never the running world's bodies or cargo slots. */
bool ValidateAsteroidCargoOrigin(const AsteroidCargoOrigin &origin, std::span<const AsteroidIdentityRecord> bodies, uint32_t next_body_id);
/** @param source Canonical modern or old-era asteroid source. @param field Prevalidated original/copied field metadata. @return Validated full origin; legacy inference requires its actual surviving body and never repairs a missing retired source. Does not rewrite the source or enable its new wire format. */
std::optional<AsteroidCargoOrigin> ResolveAsteroidCargoOrigin(const Source &source, const AsteroidField &field);
/** @param source Canonical modern or old-era candidate source. @param bodies Validated sorted same-file live rows. @param next_body_id Same-file issuance cursor. @return Validated full provenance, deriving legacy meaning only from this candidate's surviving body. No live CargoSpec or field lookup participates. */
std::optional<AsteroidCargoOrigin> ResolveAsteroidCargoOrigin(const Source &source, std::span<const AsteroidIdentityRecord> bodies, uint32_t next_body_id);
/** @param field Prevalidated original or copied metadata used only for provenance; all containers remain the actual native owners. @return Whether original asteroid packets have compatible cargo/production meaning and exactly one physical owner, including after source retirement. No source, packet, container or field is mutated. */
bool ValidateAsteroidPacketCustody(const AsteroidField &field);
/** Normalize genuine older native asteroid packets after their original field has installed.
 * @param field This loaded file's already validated original field, never a regenerated replacement.
 * @return Whether all legacy origins resolved before any packet changed; conversion retains every original pool/container/quantity/credit/travel field. Caller gates this to genuine pre-origin save versions.
 */
bool NormalizeLegacyAsteroidCargoPackets(const AsteroidField &field);

#endif /* ASTEROID_CARGO_H */
