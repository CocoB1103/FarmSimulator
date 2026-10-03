#include "FarmPlayerController.h"
#include "FarmHUDWidget.h"
#include "Blueprint/UserWidget.h"

void AFarmPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	if (!IsLocalController())
	{
		return;
	}

	HUDWidget = CreateWidget<UFarmHUDWidget>(this, UFarmHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}
