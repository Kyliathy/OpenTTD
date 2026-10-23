/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_source_template_fixture.h Shared empty native author for genuine introduction-date cargo producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15EB/15FBA.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Only initial date/funds/content are authored; ordinary loading owns every later purchase, inventory and operation.
 */
#ifndef TESTS_TUBE_SOURCE_TEMPLATE_FIXTURE_H
#define TESTS_TUBE_SOURCE_TEMPLATE_FIXTURE_H

/**
 * Write a reproducible empty map with normal saved settings and genuine native engine/economy startup.
 * @param year Explicit initial scenario calendar/economy year, before any source or vehicle exists.
 * @param scenario Invocation-owned tube evidence scenario directory.
 * @param basename Purpose-named native save basename inside that scenario's saves directory.
 * @param initial_money Disclosed initial author funds; never a top-up of a loaded working world.
 * @return Actual native save path; the fixture restores caller metadata/settings after writing.
 */
std::filesystem::path WriteTubeUsefulSourceTemplate(int year, std::string_view scenario, std::string_view basename, int64_t initial_money);

#endif /* TESTS_TUBE_SOURCE_TEMPLATE_FIXTURE_H */
