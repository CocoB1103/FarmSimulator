#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FarmPlayerController.generated.h"

class UFarmHUDWidget;

UCLASS()
class AFarmPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "Farm")
	UFarmHUDWidget* HUDWidget;
};
