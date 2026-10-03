#include "CropPlot.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "FarmGameState.h"
#include "FarmSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Real-world size of the imported dirt tile mesh is ~40x35cm; scale it up to fill a plot.
	constexpr float DirtTileScale = 4.5f;
	constexpr float DirtTileTopZ = 10.0f * DirtTileScale * 0.5f; // half of the imported mesh's ~10cm height
}

ACropPlot::ACropPlot()
{
	PrimaryActorTick.bCanEverTick = false;

	PlotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlotRoot"));
	RootComponent = PlotRoot;

	SoilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoilMesh"));
	SoilMesh->SetupAttachment(PlotRoot);
	SoilMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SoilMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SoilMesh->SetMobility(EComponentMobility::Movable);
	SoilMesh->SetRelativeScale3D(FVector(DirtTileScale));
	SoilMesh->SetVisibility(false);

	PlantMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlantMesh"));
	PlantMesh->SetupAttachment(PlotRoot);
	PlantMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlantMesh->SetMobility(EComponentMobility::Movable);
	PlantMesh->SetRelativeLocation(FVector(0.0f, 0.0f, DirtTileTopZ * 2.0f));
	PlantMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DirtMeshFinder(TEXT("/Game/Assets/Crops/crops_dirtSingle.crops_dirtSingle"));
	if (DirtMeshFinder.Succeeded())
	{
		SoilMesh->SetStaticMesh(DirtMeshFinder.Object);
	}
}

void ACropPlot::BeginPlay()
{
	Super::BeginPlay();

	CachedGameState = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr;
	if (CachedGameState)
	{
		CachedGameState->OnNewDay.AddDynamic(this, &ACropPlot::HandleNewDay);
	}

	RefreshVisuals();
}

void ACropPlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedGameState)
	{
		CachedGameState->OnNewDay.RemoveDynamic(this, &ACropPlot::HandleNewDay);
	}
	Super::EndPlay(EndPlayReason);
}

bool ACropPlot::Interact(AFarmGameState* GameState, ECropType SelectedCropToPlant, FText& OutMessage)
{
	if (!GameState)
	{
		return false;
	}

	switch (State)
	{
	case EPlotState::Untilled:
	{
		State = EPlotState::Tilled;
		RefreshVisuals();
		OutMessage = FText::FromString(TEXT("Parcelle labouree."));
		return true;
	}
	case EPlotState::Tilled:
	{
		if (SelectedCropToPlant == ECropType::None)
		{
			OutMessage = FText::FromString(TEXT("Choisissez une culture avec C."));
			return false;
		}

		const FCropDefinition& Def = FFarmCropDatabase::Get(SelectedCropToPlant);
		if (!GameState->SpendMoney(Def.SeedCost))
		{
			OutMessage = FText::FromString(TEXT("Pas assez d'argent pour planter."));
			return false;
		}

		PlantedCrop = SelectedCropToPlant;
		State = EPlotState::Planted;
		GrowthStage = 0;
		GrowthProgress = 0.0f;
		RefreshVisuals();
		OutMessage = FText::FromString(FString::Printf(TEXT("%s plante !"), *Def.DisplayName));
		return true;
	}
	case EPlotState::Planted:
	{
		if (GrowthStage >= MaxGrowthStage)
		{
			const FCropDefinition& Def = FFarmCropDatabase::Get(PlantedCrop);
			GameState->AddToInventory(PlantedCrop, 1);
			OutMessage = FText::FromString(FString::Printf(TEXT("%s recolte !"), *Def.DisplayName));
			PlantedCrop = ECropType::None;
			State = EPlotState::Tilled;
			GrowthStage = 0;
			GrowthProgress = 0.0f;
			RefreshVisuals();
			return true;
		}
		OutMessage = FText::FromString(TEXT("La culture pousse encore..."));
		return false;
	}
	}

	return false;
}

FText ACropPlot::GetInteractionPrompt(ECropType SelectedCropToPlant) const
{
	switch (State)
	{
	case EPlotState::Untilled:
		return FText::FromString(TEXT("[E] Labourer"));
	case EPlotState::Tilled:
		if (SelectedCropToPlant == ECropType::None)
		{
			return FText::FromString(TEXT("[C] Choisir une culture"));
		}
		else
		{
			const FCropDefinition& Def = FFarmCropDatabase::Get(SelectedCropToPlant);
			return FText::FromString(FString::Printf(TEXT("[E] Planter %s (%d$)"), *Def.DisplayName, Def.SeedCost));
		}
	case EPlotState::Planted:
		if (GrowthStage >= MaxGrowthStage)
		{
			const FCropDefinition& Def = FFarmCropDatabase::Get(PlantedCrop);
			return FText::FromString(FString::Printf(TEXT("[E] Recolter %s"), *Def.DisplayName));
		}
		else
		{
			const FCropDefinition& Def = FFarmCropDatabase::Get(PlantedCrop);
			return FText::FromString(FString::Printf(TEXT("%s : pousse %d/%d"), *Def.DisplayName, GrowthStage, MaxGrowthStage));
		}
	}
	return FText::GetEmpty();
}

void ACropPlot::HandleNewDay(float GrowthMultiplier)
{
	if (State != EPlotState::Planted || GrowthStage >= MaxGrowthStage)
	{
		return;
	}

	if (GrowthMultiplier <= 0.0f)
	{
		return;
	}

	const FCropDefinition& Def = FFarmCropDatabase::Get(PlantedCrop);
	if (Def.DaysPerStage <= 0.0f)
	{
		return;
	}

	GrowthProgress += GrowthMultiplier / Def.DaysPerStage;
	while (GrowthProgress >= 1.0f && GrowthStage < MaxGrowthStage)
	{
		GrowthProgress -= 1.0f;
		GrowthStage++;
	}
	if (GrowthStage >= MaxGrowthStage)
	{
		GrowthProgress = 0.0f;
	}

	RefreshVisuals();
}

UStaticMesh* ACropPlot::GetCropStageMesh(ECropType Crop, int32 Stage, float& OutScale)
{
	OutScale = 1.0f;
	const TCHAR* Path = nullptr;

	switch (Crop)
	{
	case ECropType::Wheat:
		Path = (Stage <= 1) ? TEXT("/Game/Assets/Crops/crops_wheatStageA.crops_wheatStageA")
							: TEXT("/Game/Assets/Crops/crops_wheatStageB.crops_wheatStageB");
		break;
	case ECropType::Carrot:
		Path = TEXT("/Game/Assets/Crops/crop_carrot.crop_carrot");
		OutScale = FMath::Lerp(0.4f, 1.1f, static_cast<float>(Stage) / MaxGrowthStage);
		break;
	case ECropType::Potato:
		Path = TEXT("/Game/Assets/Crops/crop_turnip.crop_turnip");
		OutScale = FMath::Lerp(0.4f, 1.1f, static_cast<float>(Stage) / MaxGrowthStage);
		break;
	case ECropType::Corn:
		switch (Stage)
		{
		case 0: Path = TEXT("/Game/Assets/Crops/crops_cornStageA.crops_cornStageA"); break;
		case 1: Path = TEXT("/Game/Assets/Crops/crops_cornStageB.crops_cornStageB"); break;
		case 2: Path = TEXT("/Game/Assets/Crops/crops_cornStageC.crops_cornStageC"); break;
		default: Path = TEXT("/Game/Assets/Crops/crops_cornStageD.crops_cornStageD"); break;
		}
		break;
	case ECropType::Strawberry:
		if (Stage <= 1) Path = TEXT("/Game/Assets/Crops/crops_leafsStageA.crops_leafsStageA");
		else if (Stage <= 3) Path = TEXT("/Game/Assets/Crops/crops_leafsStageB.crops_leafsStageB");
		else { Path = TEXT("/Game/Assets/Crops/strawberry.strawberry"); OutScale = 2.4f; }
		break;
	case ECropType::Pumpkin:
		if (Stage <= 1) Path = TEXT("/Game/Assets/Crops/crops_leafsStageA.crops_leafsStageA");
		else if (Stage <= 3) Path = TEXT("/Game/Assets/Crops/crops_leafsStageB.crops_leafsStageB");
		else { Path = TEXT("/Game/Assets/Crops/crop_pumpkin.crop_pumpkin"); OutScale = 1.4f; }
		break;
	default:
		return nullptr;
	}

	return Path ? LoadObject<UStaticMesh>(nullptr, Path) : nullptr;
}

void ACropPlot::RefreshVisuals()
{
	// Always show the tilled soil mesh so the player can see every plantable plot at a glance.
	SoilMesh->SetVisibility(true);

	const bool bShouldShowPlant = (State == EPlotState::Planted);
	PlantMesh->SetVisibility(bShouldShowPlant);

	if (bShouldShowPlant)
	{
		float Scale = 1.0f;
		if (UStaticMesh* Mesh = GetCropStageMesh(PlantedCrop, GrowthStage, Scale))
		{
			PlantMesh->SetStaticMesh(Mesh);
			PlantMesh->SetRelativeScale3D(FVector(Scale));
		}
	}
}

FPlotSaveData ACropPlot::GetSaveData() const
{
	FPlotSaveData Data;
	Data.State = State;
	Data.PlantedCrop = PlantedCrop;
	Data.GrowthStage = GrowthStage;
	Data.GrowthProgress = GrowthProgress;
	return Data;
}

void ACropPlot::ApplySaveData(const FPlotSaveData& Data)
{
	State = Data.State;
	PlantedCrop = Data.PlantedCrop;
	GrowthStage = Data.GrowthStage;
	GrowthProgress = Data.GrowthProgress;
	RefreshVisuals();
}
