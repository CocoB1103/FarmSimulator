#include "FarmPlayerPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "CropPlot.h"
#include "SellBuilding.h"
#include "FarmGameState.h"
#include "FarmGameMode.h"
#include "Kismet/GameplayStatics.h"

AFarmPlayerPawn::AFarmPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	PawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PawnRoot"));
	RootComponent = PawnRoot;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(PawnRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BodyMesh->SetSimulatePhysics(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterialFinder(TEXT("/Game/Materials/M_Base_Color.M_Base_Color"));

	if (CylinderMeshFinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMeshFinder.Object);
		BodyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));
		BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	}
	if (BaseMaterialFinder.Succeeded())
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterialFinder.Object, this);
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.35f, 0.85f));
		BodyMesh->SetMaterial(0, MID);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(PawnRoot);
	CameraBoom->TargetArmLength = 1300.0f;
	CameraBoom->SetRelativeRotation(FRotator(-65.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.0f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->SetFieldOfView(70.0f);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(PawnRoot);
	InteractSphere->SetSphereRadius(190.0f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 600.0f;
	Movement->Acceleration = 4000.0f;
	Movement->Deceleration = 4000.0f;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AFarmPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &AFarmPlayerPawn::OnInteractSphereBeginOverlap);
	InteractSphere->OnComponentEndOverlap.AddDynamic(this, &AFarmPlayerPawn::OnInteractSphereEndOverlap);
}

void AFarmPlayerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!PendingMoveInput.IsNearlyZero())
	{
		const FVector WorldDir(PendingMoveInput.X, PendingMoveInput.Y, 0.0f);
		AddMovementInput(WorldDir.GetSafeNormal(), 1.0f);

		const FRotator TargetRotation = WorldDir.Rotation();
		const FRotator NewRotation = FMath::RInterpTo(BodyMesh->GetComponentRotation(), TargetRotation, DeltaSeconds, 10.0f);
		BodyMesh->SetWorldRotation(NewRotation);
	}
	PendingMoveInput = FVector2D::ZeroVector;

	if (LastMessageTimeRemaining > 0.0f)
	{
		LastMessageTimeRemaining -= DeltaSeconds;
	}
}

void AFarmPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AFarmPlayerPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AFarmPlayerPawn::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AFarmPlayerPawn::OnInteractPressed);
	PlayerInputComponent->BindAction(TEXT("CycleCrop"), IE_Pressed, this, &AFarmPlayerPawn::OnCycleCropPressed);
	PlayerInputComponent->BindAction(TEXT("Speed1"), IE_Pressed, this, &AFarmPlayerPawn::OnSpeed1);
	PlayerInputComponent->BindAction(TEXT("Speed2"), IE_Pressed, this, &AFarmPlayerPawn::OnSpeed2);
	PlayerInputComponent->BindAction(TEXT("Speed3"), IE_Pressed, this, &AFarmPlayerPawn::OnSpeed3);
	PlayerInputComponent->BindAction(TEXT("Restart"), IE_Pressed, this, &AFarmPlayerPawn::OnRestartPressed);
	PlayerInputComponent->BindAction(TEXT("QuickSave"), IE_Pressed, this, &AFarmPlayerPawn::OnQuickSavePressed);
	PlayerInputComponent->BindAction(TEXT("QuickLoad"), IE_Pressed, this, &AFarmPlayerPawn::OnQuickLoadPressed);
}

void AFarmPlayerPawn::MoveForward(float Value)
{
	PendingMoveInput.X += Value;
}

void AFarmPlayerPawn::MoveRight(float Value)
{
	PendingMoveInput.Y += Value;
}

AActor* AFarmPlayerPawn::GetCurrentInteractTarget() const
{
	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Actor : NearbyInteractables)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Actor;
		}
	}
	return Best;
}

void AFarmPlayerPawn::OnInteractPressed()
{
	AFarmGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}

	AActor* Target = GetCurrentInteractTarget();
	if (!Target)
	{
		return;
	}

	FText Message;
	if (ACropPlot* Plot = Cast<ACropPlot>(Target))
	{
		Plot->Interact(GameState, GameState->SelectedCrop, Message);
		ShowMessage(Message);
	}
	else if (ASellBuilding* Building = Cast<ASellBuilding>(Target))
	{
		Building->Interact(GameState, Message);
		ShowMessage(Message);
	}
}

void AFarmPlayerPawn::OnCycleCropPressed()
{
	AFarmGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}
	GameState->CycleSelectedCrop();
	ShowMessage(FText::FromString(FString::Printf(TEXT("Culture selectionnee : %s"), *FFarmCropDatabase::Get(GameState->SelectedCrop).DisplayName)));
}

ECropType AFarmPlayerPawn::GetSelectedCrop() const
{
	if (AFarmGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr)
	{
		return GameState->SelectedCrop;
	}
	return ECropType::None;
}

void AFarmPlayerPawn::OnQuickSavePressed()
{
	if (AFarmGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFarmGameMode>() : nullptr)
	{
		GM->SaveGame();
		if (AFarmGameState* GameState = GetWorld()->GetGameState<AFarmGameState>())
		{
			GameState->ShowSystemMessage(FText::FromString(TEXT("Partie sauvegardee.")));
		}
	}
}

void AFarmPlayerPawn::OnQuickLoadPressed()
{
	if (AFarmGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFarmGameMode>() : nullptr)
	{
		GM->LoadGame();
	}
}

void AFarmPlayerPawn::OnSpeed1()
{
	if (AFarmGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr)
	{
		GS->SetGameSpeed(1);
	}
}

void AFarmPlayerPawn::OnSpeed2()
{
	if (AFarmGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr)
	{
		GS->SetGameSpeed(2);
	}
}

void AFarmPlayerPawn::OnSpeed3()
{
	if (AFarmGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr)
	{
		GS->SetGameSpeed(4);
	}
}

void AFarmPlayerPawn::OnRestartPressed()
{
	AFarmGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr;
	if (GameState && GameState->bYearComplete)
	{
		UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
	}
}

void AFarmPlayerPawn::OnInteractSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && (OtherActor->IsA(ACropPlot::StaticClass()) || OtherActor->IsA(ASellBuilding::StaticClass())))
	{
		NearbyInteractables.AddUnique(OtherActor);
	}
}

void AFarmPlayerPawn::OnInteractSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NearbyInteractables.Remove(OtherActor);
}

void AFarmPlayerPawn::ShowMessage(const FText& Message)
{
	if (!Message.IsEmpty())
	{
		LastMessage = Message;
		LastMessageTimeRemaining = 2.5f;
	}
}
