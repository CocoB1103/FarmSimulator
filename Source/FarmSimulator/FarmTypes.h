#pragma once

#include "CoreMinimal.h"
#include "FarmTypes.generated.h"

UENUM(BlueprintType)
enum class ESeason : uint8
{
	Spring UMETA(DisplayName = "Printemps"),
	Summer UMETA(DisplayName = "Ete"),
	Autumn UMETA(DisplayName = "Automne"),
	Winter UMETA(DisplayName = "Hiver")
};

UENUM(BlueprintType)
enum class ECropType : uint8
{
	None UMETA(DisplayName = "Aucune"),
	Wheat UMETA(DisplayName = "Ble"),
	Carrot UMETA(DisplayName = "Carotte"),
	Potato UMETA(DisplayName = "Pomme de terre"),
	Corn UMETA(DisplayName = "Mais"),
	Strawberry UMETA(DisplayName = "Fraise"),
	Pumpkin UMETA(DisplayName = "Citrouille")
};

UENUM(BlueprintType)
enum class EPlotState : uint8
{
	Untilled UMETA(DisplayName = "Non laboure"),
	Tilled UMETA(DisplayName = "Laboure"),
	Planted UMETA(DisplayName = "Plante")
};

USTRUCT(BlueprintType)
struct FCropDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	ECropType Type = ECropType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 SeedCost = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 SellPrice = 20;

	// Number of in-game days needed to progress one growth stage (there are 5 stages: 0-4).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	float DaysPerStage = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FLinearColor YoungColor = FLinearColor(0.25f, 0.55f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FLinearColor RipeColor = FLinearColor(1.0f, 0.5f, 0.0f);
};

// Simple static database of all crop definitions used in the game.
struct FFarmCropDatabase
{
	static const FCropDefinition& Get(ECropType Type);
	static const TArray<ECropType>& GetAllCropTypes();
	static FText GetSeasonDisplayName(ESeason Season);
};
