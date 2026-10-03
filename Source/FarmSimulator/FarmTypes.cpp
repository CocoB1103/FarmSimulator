#include "FarmTypes.h"

namespace
{
	TMap<ECropType, FCropDefinition> BuildCropDatabase()
	{
		TMap<ECropType, FCropDefinition> Map;

		FCropDefinition Wheat;
		Wheat.Type = ECropType::Wheat;
		Wheat.DisplayName = TEXT("Ble");
		Wheat.SeedCost = 15;
		Wheat.SellPrice = 35;
		Wheat.DaysPerStage = 4.0f;
		Wheat.YoungColor = FLinearColor(0.3f, 0.6f, 0.15f);
		Wheat.RipeColor = FLinearColor(0.85f, 0.75f, 0.25f);
		Map.Add(ECropType::Wheat, Wheat);

		FCropDefinition Carrot;
		Carrot.Type = ECropType::Carrot;
		Carrot.DisplayName = TEXT("Carotte");
		Carrot.SeedCost = 20;
		Carrot.SellPrice = 50;
		Carrot.DaysPerStage = 5.0f;
		Carrot.YoungColor = FLinearColor(0.25f, 0.55f, 0.15f);
		Carrot.RipeColor = FLinearColor(0.95f, 0.45f, 0.05f);
		Map.Add(ECropType::Carrot, Carrot);

		FCropDefinition Potato;
		Potato.Type = ECropType::Potato;
		Potato.DisplayName = TEXT("Pomme de terre");
		Potato.SeedCost = 30;
		Potato.SellPrice = 70;
		Potato.DaysPerStage = 6.0f;
		Potato.YoungColor = FLinearColor(0.25f, 0.55f, 0.15f);
		Potato.RipeColor = FLinearColor(0.65f, 0.5f, 0.3f);
		Map.Add(ECropType::Potato, Potato);

		FCropDefinition Corn;
		Corn.Type = ECropType::Corn;
		Corn.DisplayName = TEXT("Mais");
		Corn.SeedCost = 40;
		Corn.SellPrice = 110;
		Corn.DaysPerStage = 7.0f;
		Corn.YoungColor = FLinearColor(0.25f, 0.55f, 0.15f);
		Corn.RipeColor = FLinearColor(0.95f, 0.85f, 0.1f);
		Map.Add(ECropType::Corn, Corn);

		FCropDefinition Strawberry;
		Strawberry.Type = ECropType::Strawberry;
		Strawberry.DisplayName = TEXT("Fraise");
		Strawberry.SeedCost = 55;
		Strawberry.SellPrice = 150;
		Strawberry.DaysPerStage = 8.0f;
		Strawberry.YoungColor = FLinearColor(0.25f, 0.55f, 0.15f);
		Strawberry.RipeColor = FLinearColor(0.85f, 0.05f, 0.2f);
		Map.Add(ECropType::Strawberry, Strawberry);

		FCropDefinition Pumpkin;
		Pumpkin.Type = ECropType::Pumpkin;
		Pumpkin.DisplayName = TEXT("Citrouille");
		Pumpkin.SeedCost = 70;
		Pumpkin.SellPrice = 220;
		Pumpkin.DaysPerStage = 9.0f;
		Pumpkin.YoungColor = FLinearColor(0.25f, 0.55f, 0.15f);
		Pumpkin.RipeColor = FLinearColor(0.85f, 0.35f, 0.03f);
		Map.Add(ECropType::Pumpkin, Pumpkin);

		return Map;
	}
}

const FCropDefinition& FFarmCropDatabase::Get(ECropType Type)
{
	static const TMap<ECropType, FCropDefinition> DB = BuildCropDatabase();

	if (const FCropDefinition* Found = DB.Find(Type))
	{
		return *Found;
	}

	static const FCropDefinition None;
	return None;
}

const TArray<ECropType>& FFarmCropDatabase::GetAllCropTypes()
{
	static const TArray<ECropType> All = { ECropType::Wheat, ECropType::Carrot, ECropType::Potato, ECropType::Corn, ECropType::Strawberry, ECropType::Pumpkin };
	return All;
}

FText FFarmCropDatabase::GetSeasonDisplayName(ESeason Season)
{
	switch (Season)
	{
	case ESeason::Spring: return FText::FromString(TEXT("Printemps"));
	case ESeason::Summer: return FText::FromString(TEXT("Ete"));
	case ESeason::Autumn: return FText::FromString(TEXT("Automne"));
	case ESeason::Winter: return FText::FromString(TEXT("Hiver"));
	default: return FText::FromString(TEXT("?"));
	}
}
