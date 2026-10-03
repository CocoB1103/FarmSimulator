#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SellBuilding.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class AFarmGameState;

// Simple building where the player sells everything in their inventory.
UCLASS()
class ASellBuilding : public AActor
{
	GENERATED_BODY()

public:
	ASellBuilding();

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	USceneComponent* BuildingRoot;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UStaticMeshComponent* BuildingMesh;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UTextRenderComponent* Label;

	// Sells the whole inventory. Returns the amount of money earned (0 if nothing to sell).
	int32 Interact(AFarmGameState* GameState, FText& OutMessage);

	FText GetInteractionPrompt() const;
};
