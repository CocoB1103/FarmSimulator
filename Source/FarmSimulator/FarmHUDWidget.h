#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FarmHUDWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class AFarmPlayerPawn;

// The entire HUD is built procedurally in C++ (no WBP asset / UMG designer needed).
UCLASS()
class UFarmHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// Widget tree must be built before the Slate hierarchy is first constructed, so this is
	// done in NativeOnInitialized() rather than NativeConstruct() (which runs too late).
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	UCanvasPanel* RootCanvas;

	UPROPERTY()
	UTextBlock* MoneyText;

	UPROPERTY()
	UTextBlock* DateText;

	UPROPERTY()
	UTextBlock* TimeText;

	UPROPERTY()
	UTextBlock* SpeedText;

	UPROPERTY()
	UTextBlock* InventoryText;

	UPROPERTY()
	UTextBlock* SelectedCropText;

	UPROPERTY()
	UTextBlock* PromptText;

	UPROPERTY()
	UTextBlock* MessageText;

	UPROPERTY()
	UTextBlock* SystemMessageText;

	UPROPERTY()
	UTextBlock* ControlsHintText;

	UPROPERTY()
	UTextBlock* YearEndText;

	UTextBlock* MakeText(UCanvasPanel* Parent, const FString& InitialText, FVector2D Position, FVector2D Size, int32 FontSize, FLinearColor Color,
		FVector2D Anchor = FVector2D(0.0f, 0.0f), FVector2D Alignment = FVector2D(0.0f, 0.0f), ETextJustify::Type Justify = ETextJustify::Left);

	AFarmPlayerPawn* GetWatchedPawn() const;
};
