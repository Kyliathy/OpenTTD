/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_cargo_preview.h Same-file copied packet ownership across ordinary and Space containers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CD
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; same-file hauler cargo meanings, capacities and whole-packet mass.
 * This validation boundary owns no inventory, packet, vehicle, station, work or utility account.
 */
#ifndef SAVELOAD_SPACE_CARGO_PREVIEW_H
#define SAVELOAD_SPACE_CARGO_PREVIEW_H

struct LoadCheckData;

/** Join original copied references to their same-file physical packet identities without consulting runtime pools.
 * @param data Complete unpublished native/feature metadata from one checked file, including legitimately empty native families.
 * @return Whether every referenced packet exists with positive quantity and belongs to exactly one inspected container. Original family validators retain recipe/quantity/role admission.
 */
bool ValidateSpaceCargoPreviewOwners(const LoadCheckData &data);

/** Match immutable descent weights, processor buffers and hauler holdings to this file's configured cargo metadata, without loading NewGRFs.
 * @param data Complete unpublished same-file preview with its version-admitted dimension family.
 * @return Whether configured ordinals are unique/bounded and every known descent, buffer and hauler uses its actual saved cargo meaning/mass. Genuine unknown legacy receipts retain their original native-only weight check.
 */
bool ValidateSpaceCargoPreviewDimensions(const LoadCheckData &data);

/** Validate staged configured dimensions against actual definitions after normal profile/NewGRF reconstruction.
 * @return Whether the current source supplies its exact configured catalogue, or a genuine older source explicitly has none. No gameplay definition is installed from metadata.
 */
bool AcceptLoadedSpaceCargoDimensions();

#endif
