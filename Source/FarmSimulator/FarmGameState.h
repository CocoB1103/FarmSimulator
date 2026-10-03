#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FarmTypes.h"
#include "FarmGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewDay, float, GrowthMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFarmStateChanged);

// Central "game manager": holds money, the calendar (time of day / day / season / year)
// and the player's harvested-crop inventory. Equivalent to the BP_GameManager described
// in the original design documents, reimplemented in C++.
UCLASS()
class AFarmGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFarmGameState();

	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Money ----------------
	UPROPERTY(BlueprintReadOnly, Category = "Farm|Money")
	int32 Money = 1000;

	void AddMoney(int32 Amount);
	bool SpendMoney(int32 Amount);

	UPROPERTY(EditDefaultsOnly, Category = "Farm|Money")
	int32 GoalMoney = 5000;

	// ---------------- Calendar ----------------
	// Hour of the day, from 6.0 (6AM) to 24.0 (midnight).
	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	float TimeOfDay = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	int32 DayNumber = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	int32 DayInSeason = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	int32 YearNumber = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	ESeason CurrentSeason = ESeason::Spring;

	UPROPERTY(EditDefaultsOnly, Category = "Farm|Time")
	int32 DaysPerSeason = 30;

	// How many real-time seconds a full in-game day takes at 1x speed.
	UPROPERTY(EditDefaultsOnly, Category = "Farm|Time")
	float SecondsPerGameDay = 30.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	int32 GameSpeed = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Farm|Time")
	bool bYearComplete = false;

	void SetGameSpeed(int32 NewSpeed);

	float GetSeasonGrowthMultiplier() const { return GetSeasonGrowthMultiplier(CurrentSeason); }
	static float GetSeasonGrowthMultiplier(ESeason Season);

	// ---------------- Selected crop (shared so it survives save/load even before the pawn exists) ----------------
	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	ECropType SelectedCrop = ECropType::Carrot;

	void CycleSelectedCrop();

	// ---------------- Inventory ----------------
	UPROPERTY(BlueprintReadOnly, Category = "Farm|Inventory")
	TMap<ECropType, int32> Inventory;

	void AddToInventory(ECropType Crop, int32 Amount = 1);

	// Sells everything currently in the inventory, adds the money and returns the amount earned.
	int32 SellAllInventory();

	bool HasAnythingToSell() const;

	// ---------------- Delegates ----------------
	// Broadcast once per in-game day change, used by crop plots to grow.
	UPROPERTY(BlueprintAssignable, Category = "Farm")
	FOnNewDay OnNewDay;

	// Broadcast whenever money/time/inventory changes, used by the HUD to refresh.
	UPROPERTY(BlueprintAssignable, Category = "Farm")
	FOnFarmStateChanged OnFarmStateChanged;

	void RestartYear();

	// Short-lived feedback message for system events (save/load), shown by the HUD.
	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	FText SystemMessage;

	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	float SystemMessageTimeRemaining = 0.0f;

	void ShowSystemMessage(const FText& Message, float Duration = 3.0f);

private:
	void AdvanceDay();
};
