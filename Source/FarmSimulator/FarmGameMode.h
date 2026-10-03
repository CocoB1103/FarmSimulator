#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FarmGameMode.generated.h"

// Builds the whole playable world procedurally in C++ (ground, lighting, crop plot grid,
// sell building) so that no hand-made level/Blueprint content is required.
UCLASS()
class AFarmGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFarmGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	UPROPERTY(EditDefaultsOnly, Category = "Farm|Layout")
	int32 PlotColumns = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Farm|Layout")
	int32 PlotRows = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Farm|Layout")
	float PlotSpacing = 200.0f;

	void SaveGame();
	bool LoadGame();

private:
	void BuildWorld();
	void SpawnGround();
	void SpawnLighting();
	void SpawnCropPlots();
	void SpawnSellBuilding();
	void SpawnPlayerStart();
	void SpawnDecorations();
	void SpawnAmbientMusic();

	UFUNCTION()
	void HandleNewDayAutosave(float GrowthMultiplier);

	UPROPERTY()
	TArray<class ACropPlot*> AllPlots;
};
