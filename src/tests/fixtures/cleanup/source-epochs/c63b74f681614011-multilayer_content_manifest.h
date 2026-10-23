/** @file multilayer_content_manifest.h Generated immutable requirements. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */
static constexpr uint16_t _multilayer_profile_version = 1;
static constexpr CargoLabel _multilayer_required_labels[] = {
	CargoLabel{"PASS"},
	CargoLabel{"COAL"},
	CargoLabel{"MAIL"},
	CargoLabel{"OIL_"},
	CargoLabel{"LVST"},
	CargoLabel{"GOOD"},
	CargoLabel{"GRAI"},
	CargoLabel{"WOOD"},
	CargoLabel{"IORE"},
	CargoLabel{"STEL"},
	CargoLabel{"VALU"},
	CargoLabel{"CORE"},
	CargoLabel{"BAUX"},
	CargoLabel{"SALT"},
	CargoLabel{"LITH"},
	CargoLabel{"NORE"},
	CargoLabel{"COBT"},
	CargoLabel{"RARE"},
	CargoLabel{"URAN"},
	CargoLabel{"ERTH"},
	CargoLabel{"ROCK"},
	CargoLabel{"RFMT"},
	CargoLabel{"CHEM"},
	CargoLabel{"BATY"},
	CargoLabel{"ELEC"},
	CargoLabel{"PROP"},
	CargoLabel{"SCMP"},
	CargoLabel{"RBPT"},
	CargoLabel{"SMIK"},
	CargoLabel{"WICE"}
};
static constexpr MultiLayerCargoContract _multilayer_cargo_contracts[] = {
	{CargoLabel{"CORE"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"BAUX"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"SALT"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"LITH"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"NORE"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"COBT"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"RARE"}, MultiLayerCargoForm::CoveredGranules, "tonnes"},
	{CargoLabel{"URAN"}, MultiLayerCargoForm::SealedDrums, "tonnes"},
	{CargoLabel{"ERTH"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"ROCK"}, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CargoLabel{"RFMT"}, MultiLayerCargoForm::Ingots, "tonnes"},
	{CargoLabel{"CHEM"}, MultiLayerCargoForm::Liquid, "kilolitres"},
	{CargoLabel{"BATY"}, MultiLayerCargoForm::Packaged, "tonnes"},
	{CargoLabel{"ELEC"}, MultiLayerCargoForm::Packaged, "tonnes"},
	{CargoLabel{"PROP"}, MultiLayerCargoForm::Liquid, "kilolitres"},
	{CargoLabel{"SCMP"}, MultiLayerCargoForm::Packaged, "tonnes"},
	{CargoLabel{"RBPT"}, MultiLayerCargoForm::Packaged, "tonnes"},
	{CargoLabel{"SMIK"}, MultiLayerCargoForm::Packaged, "tonnes"},
	{CargoLabel{"WICE"}, MultiLayerCargoForm::FrozenBulk, "tonnes"}
};
static constexpr GrfID _multilayer_package_id{"MLVI"};
static constexpr uint32_t _multilayer_package_version = 1;
static constexpr uint8_t _multilayer_package_digest[] = {125, 45, 146, 226, 87, 203, 241, 19, 56, 230, 18, 104, 122, 138, 208, 215};
