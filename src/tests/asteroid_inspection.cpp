/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_inspection.cpp Company-isolated read-only resource and signed motion presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14
 * Copied completed-scan receipts below are explicit component inputs; native paid scan saves supply separate work/capture evidence.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid_inspection.h"
#include "../company_base.h"
#include "../company_func.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "../safeguards.h"

/** Presentation rounding retains signed original direction, including reflection and stationary motion. */
TEST_CASE("Asteroid trajectory presentation uses bounded signed native motion without moving it", "[asteroid-inspection]")
{
	SpaceMotion motion{{7 * SPACE_Q16, 9 * SPACE_Q16}, 3 * SPACE_Q16, -4 * SPACE_Q16, 12, 24}; const auto before = motion;
	const auto view = GetSpaceTrajectoryDisclosure(motion); CHECK(view.x_milli == 3000); CHECK(view.y_milli == -4000); CHECK(view.speed_milli == 5000); CHECK(motion == before);
	motion.velocity_x = -SPACE_Q16 / 2; motion.velocity_y = SPACE_Q16 / 2; const auto reflected = GetSpaceTrajectoryDisclosure(motion);
	CHECK(reflected.x_milli == -500); CHECK(reflected.y_milli == 500); CHECK(reflected.speed_milli == 707);
	motion.velocity_x = 0; motion.velocity_y = 0; CHECK(GetSpaceTrajectoryDisclosure(motion).speed_milli == 0);
	motion.velocity_x = -1; motion.velocity_y = 1; const auto tiny = GetSpaceTrajectoryDisclosure(motion);
	CHECK(tiny.x_milli == 0); CHECK(tiny.y_milli == 0); CHECK(tiny.x_negative); CHECK_FALSE(tiny.y_negative);
	motion.velocity_x = INT32_MIN; motion.velocity_y = INT32_MIN; const auto extreme = GetSpaceTrajectoryDisclosure(motion);
	CHECK(extreme.x_milli == -32768000); CHECK(extreme.y_milli == -32768000); CHECK(extreme.speed_milli > 32768000);
}

/** Isolated generated body/knowledge owner restored after every company-disclosure case; it creates no hull, packet or paid scan work. */
class AsteroidInspectionRig {
	ExcavationFixture world;
	AsteroidField previous = GetAsteroidField();
public:
	/** Declare the actual generated resource owner; completed survey authorization is supplied explicitly by the test's native copied receipt. */
	AsteroidInspectionRig() { MockEnvironment::Instance(); REQUIRE(GenerateWorldAsteroids(12345)); }
	/** Restore the original field and sparse company knowledge without retaining a component receipt. */
	~AsteroidInspectionRig() { CHECK(InstallAsteroidField(std::move(previous))); }
};

/** One company's copied completed scan authorizes its actual quantities only; inspection grants no other company knowledge or resource mutation. */
TEST_CASE("Asteroid inspector discloses native quantities only to the surveyed company", "[asteroid-inspection]")
{
	AsteroidInspectionRig rig; const auto &body = GetAsteroidField().bodies.front(); const auto before = ComputeAsteroidFieldDigest(GetAsteroidField());
	auto unknown = GetAsteroidInspectionDisclosure(_current_company, body.id); REQUIRE(unknown); CHECK_FALSE(unknown->quantities); CHECK(IsValidCargoType(unknown->cargo));
	CHECK_FALSE(GetAsteroidInspectionDisclosure(_current_company, INVALID_ORBITAL_BODY)); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == before);
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> other{Company::Create()}; REQUIRE(other->index != _current_company);
	const AsteroidKnowledgeRecord receipt{body.id, uint64_t{1} << _current_company.base()}; REQUIRE(InstallLoadedAsteroidKnowledge(std::array{receipt}));
	const auto authorized = ComputeAsteroidFieldDigest(GetAsteroidField()); const auto known = GetAsteroidInspectionDisclosure(_current_company, body.id); REQUIRE(known); REQUIRE(known->quantities);
	CHECK(known->quantities->initial == body.initial_reserve); CHECK(known->quantities->remaining == body.remaining_reserve); CHECK(known->cargo == unknown->cargo);
	CHECK_FALSE(GetAsteroidInspectionDisclosure(other->index, body.id)->quantities); CHECK_FALSE(GetAsteroidInspectionDisclosure(CompanyID::Invalid(), body.id)->quantities);
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == authorized);
}
