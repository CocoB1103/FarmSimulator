#include "FarmGameState.h"

AFarmGameState::AFarmGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
}

void AFarmGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SystemMessageTimeRemaining > 0.0f)
	{
		SystemMessageTimeRemaining -= DeltaSeconds;
	}

	if (bYearComplete)
	{
		return;
	}

	const float EffectiveDelta = DeltaSeconds * static_cast<float>(FMath::Max(1, GameSpeed));
	if (SecondsPerGameDay <= 0.0f)
	{
		return;
	}

	// The clock covers an 18 hour span (6h -> 24h) over SecondsPerGameDay real seconds.
	const float HoursPerSecond = 18.0f / SecondsPerGameDay;
	TimeOfDay += EffectiveDelta * HoursPerSecond;

	if (TimeOfDay >= 24.0f)
	{
		TimeOfDay -= 18.0f;
		AdvanceDay();
	}

	OnFarmStateChanged.Broadcast();
}

void AFarmGameState::AddMoney(int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}
	Money += Amount;
	OnFarmStateChanged.Broadcast();
}

bool AFarmGameState::SpendMoney(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	if (Money < Amount)
	{
		return false;
	}
	Money -= Amount;
	OnFarmStateChanged.Broadcast();
	return true;
}

void AFarmGameState::SetGameSpeed(int32 NewSpeed)
{
	GameSpeed = FMath::Clamp(NewSpeed, 1, 4);
	OnFarmStateChanged.Broadcast();
}

float AFarmGameState::GetSeasonGrowthMultiplier(ESeason Season)
{
	switch (Season)
	{
	case ESeason::Spring: return 1.2f;
	case ESeason::Summer: return 1.5f;
	case ESeason::Autumn: return 1.0f;
	case ESeason::Winter: return 0.0f;
	default: return 1.0f;
	}
}

void AFarmGameState::CycleSelectedCrop()
{
	const TArray<ECropType>& All = FFarmCropDatabase::GetAllCropTypes();
	if (All.Num() == 0)
	{
		return;
	}
	int32 Index = All.IndexOfByKey(SelectedCrop);
	Index = (Index + 1) % All.Num();
	SelectedCrop = All[Index];
	OnFarmStateChanged.Broadcast();
}

void AFarmGameState::AddToInventory(ECropType Crop, int32 Amount)
{
	if (Crop == ECropType::None || Amount <= 0)
	{
		return;
	}
	int32& Count = Inventory.FindOrAdd(Crop);
	Count += Amount;
	OnFarmStateChanged.Broadcast();
}

bool AFarmGameState::HasAnythingToSell() const
{
	for (const auto& Pair : Inventory)
	{
		if (Pair.Value > 0)
		{
			return true;
		}
	}
	return false;
}

int32 AFarmGameState::SellAllInventory()
{
	int32 Total = 0;
	for (const auto& Pair : Inventory)
	{
		if (Pair.Value > 0)
		{
			Total += Pair.Value * FFarmCropDatabase::Get(Pair.Key).SellPrice;
		}
	}
	Inventory.Empty();
	if (Total > 0)
	{
		AddMoney(Total);
	}
	OnFarmStateChanged.Broadcast();
	return Total;
}

void AFarmGameState::AdvanceDay()
{
	DayNumber++;
	DayInSeason++;

	if (DayInSeason > DaysPerSeason)
	{
		DayInSeason = 1;

		if (CurrentSeason == ESeason::Winter)
		{
			CurrentSeason = ESeason::Spring;
			YearNumber++;
			bYearComplete = true;
		}
		else
		{
			CurrentSeason = static_cast<ESeason>(static_cast<uint8>(CurrentSeason) + 1);
		}
	}

	OnNewDay.Broadcast(GetSeasonGrowthMultiplier());
}

void AFarmGameState::RestartYear()
{
	Money = 1000;
	TimeOfDay = 6.0f;
	DayNumber = 1;
	DayInSeason = 1;
	YearNumber = 1;
	CurrentSeason = ESeason::Spring;
	GameSpeed = 1;
	bYearComplete = false;
	Inventory.Empty();
	OnFarmStateChanged.Broadcast();
}

void AFarmGameState::ShowSystemMessage(const FText& Message, float Duration)
{
	SystemMessage = Message;
	SystemMessageTimeRemaining = Duration;
}
