#include "FarmGameMode.h"
#include "FarmPlayerPawn.h"
#include "FarmPlayerController.h"
#include "FarmGameState.h"
#include "CropPlot.h"
#include "SellBuilding.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "GameFramework/PlayerStart.h"
#include "FarmSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AFarmGameMode::AFarmGameMode()
{
	DefaultPawnClass = AFarmPlayerPawn::StaticClass();
	PlayerControllerClass = AFarmPlayerController::StaticClass();
	GameStateClass = AFarmGameState::StaticClass();
}

void AFarmGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// Build the world before any actor BeginPlay/player login happens, so the
	// PlayerStart we spawn here is already present when the player is spawned.
	BuildWorld();
}

void AFarmGameMode::BuildWorld()
{
	SpawnGround();
	SpawnLighting();
	SpawnCropPlots();
	SpawnSellBuilding();
	SpawnPlayerStart();
	SpawnDecorations();
	SpawnAmbientMusic();

	if (AFarmGameState* FarmGameState = GetGameState<AFarmGameState>())
	{
		FarmGameState->OnNewDay.AddDynamic(this, &AFarmGameMode::HandleNewDayAutosave);
	}

	LoadGame();
}

void AFarmGameMode::SpawnGround()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Base_Color.M_Base_Color"));

	FActorSpawnParameters Params;
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Ground)
	{
		Ground->SetMobility(EComponentMobility::Static);
		UStaticMeshComponent* Comp = Ground->GetStaticMeshComponent();
		Comp->SetStaticMesh(PlaneMesh);
		Comp->SetWorldScale3D(FVector(50.0f, 50.0f, 1.0f));
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionResponseToAllChannels(ECR_Block);

		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, Ground);
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.4f, 0.1f));
			MID->SetScalarParameterValue(TEXT("Roughness"), 1.0f);
			Comp->SetMaterial(0, MID);
		}
	}
}

void AFarmGameMode::SpawnLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;

	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 500), FRotator(-55.0f, -30.0f, 0.0f), Params);
	if (Sun)
	{
		Sun->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* Comp = Sun->GetComponent())
		{
			Comp->SetIntensity(8.0f);
			Comp->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));
			Comp->SetAtmosphereSunLight(true);
		}
	}

	ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Sky)
	{
		if (USkyLightComponent* Comp = Sky->GetLightComponent())
		{
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetIntensity(1.3f);
			Comp->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			Comp->bRealTimeCapture = true;
		}
	}
}

void AFarmGameMode::SpawnCropPlots()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float StartX = -((PlotColumns - 1) * PlotSpacing) * 0.5f;
	const float StartY = 300.0f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Row = 0; Row < PlotRows; ++Row)
	{
		for (int32 Col = 0; Col < PlotColumns; ++Col)
		{
			const FVector Location(StartX + Col * PlotSpacing, StartY + Row * PlotSpacing, 0.0f);
			if (ACropPlot* Plot = World->SpawnActor<ACropPlot>(Location, FRotator::ZeroRotator, Params))
			{
				Plot->PlotIndex = AllPlots.Num();
				AllPlots.Add(Plot);
			}
		}
	}
}

void AFarmGameMode::SpawnSellBuilding()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	World->SpawnActor<ASellBuilding>(FVector(0.0f, -500.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f), Params);
}

void AFarmGameMode::SpawnPlayerStart()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	World->SpawnActor<APlayerStart>(FVector(0.0f, -150.0f, 110.0f), FRotator(0.0f, 90.0f, 0.0f), Params);
}

void AFarmGameMode::SpawnDecorations()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStaticMesh* FenceMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Decoration/fence_simple.fence_simple"));
	UStaticMesh* TreeDefault = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Decoration/tree_default.tree_default"));
	UStaticMesh* TreePine = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Decoration/tree_pineRoundA.tree_pineRoundA"));
	UStaticMesh* FlowerRed = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Decoration/flower_redA.flower_redA"));
	UStaticMesh* FlowerYellow = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Decoration/flower_yellowA.flower_yellowA"));
	UStaticMesh* RockMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Decoration/rock_smallA.rock_smallA"));

	auto SpawnDeco = [World](UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, float Scale)
	{
		if (!Mesh)
		{
			return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Loc, Rot, Params))
		{
			Actor->SetMobility(EComponentMobility::Static);
			if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
			{
				Comp->SetStaticMesh(Mesh);
				Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Comp->SetWorldScale3D(FVector(Scale));
			}
		}
	};

	const float StartX = -((PlotColumns - 1) * PlotSpacing) * 0.5f;
	const float StartY = 300.0f;
	const float Margin = 160.0f;
	const float MinX = StartX - Margin;
	const float MaxX = StartX + (PlotColumns - 1) * PlotSpacing + Margin;
	const float MinY = StartY - Margin;
	const float MaxY = StartY + (PlotRows - 1) * PlotSpacing + Margin;

	// Fence lining the north/south edges of the field.
	for (float X = MinX; X <= MaxX + 1.0f; X += 100.0f)
	{
		SpawnDeco(FenceMesh, FVector(X, MinY, 0.0f), FRotator::ZeroRotator, 1.0f);
		SpawnDeco(FenceMesh, FVector(X, MaxY, 0.0f), FRotator::ZeroRotator, 1.0f);
	}
	// Fence lining the east/west edges of the field.
	for (float Y = MinY; Y <= MaxY + 1.0f; Y += 100.0f)
	{
		SpawnDeco(FenceMesh, FVector(MinX, Y, 0.0f), FRotator(0.0f, 90.0f, 0.0f), 1.0f);
		SpawnDeco(FenceMesh, FVector(MaxX, Y, 0.0f), FRotator(0.0f, 90.0f, 0.0f), 1.0f);
	}

	// A handful of trees just outside the four corners for atmosphere.
	SpawnDeco(TreeDefault, FVector(MinX - 180.0f, MinY - 180.0f, 0.0f), FRotator::ZeroRotator, 1.1f);
	SpawnDeco(TreePine, FVector(MaxX + 180.0f, MinY - 180.0f, 0.0f), FRotator::ZeroRotator, 1.0f);
	SpawnDeco(TreePine, FVector(MinX - 180.0f, MaxY + 180.0f, 0.0f), FRotator::ZeroRotator, 1.0f);
	SpawnDeco(TreeDefault, FVector(MaxX + 180.0f, MaxY + 180.0f, 0.0f), FRotator::ZeroRotator, 1.1f);
	SpawnDeco(TreeDefault, FVector(StartX + (PlotColumns - 1) * PlotSpacing * 0.5f, MaxY + 220.0f, 0.0f), FRotator::ZeroRotator, 1.2f);

	// Flowers and rocks scattered along the fence line for a bit of color.
	SpawnDeco(FlowerRed, FVector(MinX - 60.0f, StartY, 0.0f), FRotator::ZeroRotator, 1.0f);
	SpawnDeco(FlowerYellow, FVector(MinX - 60.0f, StartY + PlotSpacing, 0.0f), FRotator::ZeroRotator, 1.0f);
	SpawnDeco(FlowerRed, FVector(MaxX + 60.0f, StartY + PlotSpacing * 2.0f, 0.0f), FRotator::ZeroRotator, 1.0f);
	SpawnDeco(FlowerYellow, FVector(MaxX + 60.0f, StartY, 0.0f), FRotator::ZeroRotator, 1.0f);
	SpawnDeco(RockMesh, FVector(MinX - 70.0f, MinY - 70.0f, 0.0f), FRotator::ZeroRotator, 1.3f);
	SpawnDeco(RockMesh, FVector(MaxX + 70.0f, MaxY + 70.0f, 0.0f), FRotator::ZeroRotator, 1.1f);
}

void AFarmGameMode::SpawnAmbientMusic()
{
	USoundBase* Music = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/BGM_Farm_Relaxed.BGM_Farm_Relaxed"));
	if (USoundWave* Wave = Cast<USoundWave>(Music))
	{
		Wave->bLooping = true;
	}
	if (Music)
	{
		UGameplayStatics::SpawnSound2D(GetWorld(), Music, 0.35f);
	}
}

void AFarmGameMode::HandleNewDayAutosave(float GrowthMultiplier)
{
	SaveGame();
}

void AFarmGameMode::SaveGame()
{
	AFarmGameState* GS = GetGameState<AFarmGameState>();
	if (!GS)
	{
		return;
	}

	UFarmSaveGame* Save = Cast<UFarmSaveGame>(UGameplayStatics::CreateSaveGameObject(UFarmSaveGame::StaticClass()));
	if (!Save)
	{
		return;
	}

	Save->Money = GS->Money;
	Save->TimeOfDay = GS->TimeOfDay;
	Save->DayNumber = GS->DayNumber;
	Save->DayInSeason = GS->DayInSeason;
	Save->YearNumber = GS->YearNumber;
	Save->CurrentSeason = GS->CurrentSeason;
	Save->GameSpeed = GS->GameSpeed;
	Save->Inventory = GS->Inventory;
	Save->SelectedCrop = GS->SelectedCrop;

	Save->Plots.Reserve(AllPlots.Num());
	for (ACropPlot* Plot : AllPlots)
	{
		if (IsValid(Plot))
		{
			Save->Plots.Add(Plot->GetSaveData());
		}
	}

	UGameplayStatics::SaveGameToSlot(Save, UFarmSaveGame::SlotName, 0);
}

bool AFarmGameMode::LoadGame()
{
	if (!UGameplayStatics::DoesSaveGameExist(UFarmSaveGame::SlotName, 0))
	{
		return false;
	}

	UFarmSaveGame* Save = Cast<UFarmSaveGame>(UGameplayStatics::LoadGameFromSlot(UFarmSaveGame::SlotName, 0));
	if (!Save)
	{
		return false;
	}

	AFarmGameState* GS = GetGameState<AFarmGameState>();
	if (!GS)
	{
		return false;
	}

	GS->Money = Save->Money;
	GS->TimeOfDay = Save->TimeOfDay;
	GS->DayNumber = Save->DayNumber;
	GS->DayInSeason = Save->DayInSeason;
	GS->YearNumber = Save->YearNumber;
	GS->CurrentSeason = Save->CurrentSeason;
	GS->GameSpeed = Save->GameSpeed;
	GS->Inventory = Save->Inventory;
	GS->SelectedCrop = Save->SelectedCrop;
	GS->bYearComplete = false;

	for (int32 Index = 0; Index < AllPlots.Num() && Index < Save->Plots.Num(); ++Index)
	{
		if (IsValid(AllPlots[Index]))
		{
			AllPlots[Index]->ApplySaveData(Save->Plots[Index]);
		}
	}

	GS->ShowSystemMessage(FText::FromString(TEXT("Partie chargee.")));
	return true;
}
