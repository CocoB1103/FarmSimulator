#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FarmTypes.h"
#include "CropPlot.generated.h"

class UStaticMeshComponent;
class AFarmGameState;

// A single plantable plot of soil. Lifecycle: Untilled -> Tilled -> Planted -> (grows through
// 5 stages) -> ready to harvest -> back to Tilled.
// Visuals use real 3D models (Kenney "Nature Kit" / "Food Kit", CC0) swapped per growth stage.
UCLASS()
class ACropPlot : public AActor
{
	GENERATED_BODY()

public:
	ACropPlot();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(VisibleAnywhere, Category = "Farm")
	USceneComponent* PlotRoot;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UStaticMeshComponent* SoilMesh;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UStaticMeshComponent* PlantMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	EPlotState State = EPlotState::Untilled;

	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	ECropType PlantedCrop = ECropType::None;

	// 0-4. Stage 4 means the crop is ready to harvest.
	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	int32 GrowthStage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	float GrowthProgress = 0.0f;

	static constexpr int32 MaxGrowthStage = 4;

	// Performs the context-sensitive action for this plot (till / plant / harvest).
	// Returns true if something happened.
	bool Interact(AFarmGameState* GameState, ECropType SelectedCropToPlant, FText& OutMessage);

	// Returns the hint text to show the player for the current context (e.g. "[E] Planter Carotte (20$)").
	FText GetInteractionPrompt(ECropType SelectedCropToPlant) const;

	UFUNCTION()
	void HandleNewDay(float GrowthMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	int32 PlotIndex = INDEX_NONE;

	struct FPlotSaveData GetSaveData() const;
	void ApplySaveData(const struct FPlotSaveData& Data);

private:
	UPROPERTY()
	AFarmGameState* CachedGameState;

	void RefreshVisuals();

	// Returns the static mesh + uniform scale to use for the current crop/stage, or nullptr if nothing should show.
	static UStaticMesh* GetCropStageMesh(ECropType Crop, int32 Stage, float& OutScale);
};
