#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FarmTypes.h"
#include "FarmPlayerPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class AFarmPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AFarmPlayerPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(VisibleAnywhere, Category = "Farm")
	USceneComponent* PawnRoot;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UStaticMeshComponent* BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UCameraComponent* TopDownCamera;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	UFloatingPawnMovement* Movement;

	UPROPERTY(VisibleAnywhere, Category = "Farm")
	USphereComponent* InteractSphere;

	// Returns the closest actor (ACropPlot or ASellBuilding) the player can interact with, or nullptr.
	AActor* GetCurrentInteractTarget() const;

	ECropType GetSelectedCrop() const;

	// Last feedback message produced by an interaction, shown briefly on the HUD.
	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	FText LastMessage;

	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	float LastMessageTimeRemaining = 0.0f;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void OnInteractPressed();
	void OnCycleCropPressed();
	void OnSpeed1();
	void OnSpeed2();
	void OnSpeed3();
	void OnRestartPressed();
	void OnQuickSavePressed();
	void OnQuickLoadPressed();

	UFUNCTION()
	void OnInteractSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY()
	TArray<AActor*> NearbyInteractables;

	FVector2D PendingMoveInput = FVector2D::ZeroVector;

	void ShowMessage(const FText& Message);
};
