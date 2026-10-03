#include "SellBuilding.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "FarmGameState.h"

ASellBuilding::ASellBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BuildingRoot"));
	RootComponent = BuildingRoot;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(BuildingRoot);
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Real 3D model: a market tent (Kenney Nature Kit, CC0). Native size is ~87x67x56cm,
	// scaled up so it reads as a small building the player can walk up to.
	const float TentScale = 2.3f;
	BuildingMesh->SetRelativeScale3D(FVector(TentScale));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(BuildingRoot);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(48.0f);
	Label->SetText(FText::FromString(TEXT("VENDRE")));
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 56.0f * TentScale + 40.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TentMeshFinder(TEXT("/Game/Assets/Building/tent_detailedOpen.tent_detailedOpen"));
	if (TentMeshFinder.Succeeded())
	{
		BuildingMesh->SetStaticMesh(TentMeshFinder.Object);
	}
}

int32 ASellBuilding::Interact(AFarmGameState* GameState, FText& OutMessage)
{
	if (!GameState)
	{
		OutMessage = FText::GetEmpty();
		return 0;
	}

	if (!GameState->HasAnythingToSell())
	{
		OutMessage = FText::FromString(TEXT("Rien a vendre pour le moment."));
		return 0;
	}

	const int32 Earned = GameState->SellAllInventory();
	OutMessage = FText::FromString(FString::Printf(TEXT("Vendu pour %d$ !"), Earned));
	return Earned;
}

FText ASellBuilding::GetInteractionPrompt() const
{
	return FText::FromString(TEXT("[E] Vendre les recoltes"));
}
