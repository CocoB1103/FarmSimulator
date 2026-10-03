#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FarmTypes.h"
#include "FarmSaveGame.generated.h"

USTRUCT()
struct FPlotSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	EPlotState State = EPlotState::Untilled;

	UPROPERTY()
	ECropType PlantedCrop = ECropType::None;

	UPROPERTY()
	int32 GrowthStage = 0;

	UPROPERTY()
	float GrowthProgress = 0.0f;
};

UCLASS()
class UFarmSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Money = 1000;

	UPROPERTY()
	float TimeOfDay = 6.0f;

	UPROPERTY()
	int32 DayNumber = 1;

	UPROPERTY()
	int32 DayInSeason = 1;

	UPROPERTY()
	int32 YearNumber = 1;

	UPROPERTY()
	ESeason CurrentSeason = ESeason::Spring;

	UPROPERTY()
	int32 GameSpeed = 1;

	UPROPERTY()
	TMap<ECropType, int32> Inventory;

	UPROPERTY()
	ECropType SelectedCrop = ECropType::Carrot;

	UPROPERTY()
	TArray<FPlotSaveData> Plots;

	static const FString SlotName;
};
