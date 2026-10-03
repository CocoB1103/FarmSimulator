#include "FarmHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "FarmGameState.h"
#include "FarmPlayerPawn.h"
#include "CropPlot.h"
#include "SellBuilding.h"
#include "Kismet/GameplayStatics.h"

void UFarmHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// Top-left: money & calendar
	MoneyText = MakeText(RootCanvas, TEXT("Argent : 1000$"), FVector2D(30, 30), FVector2D(500, 50), 32, FLinearColor(1.0f, 0.85f, 0.2f));
	DateText = MakeText(RootCanvas, TEXT("Printemps - Jour 1/30 - An 1"), FVector2D(30, 85), FVector2D(600, 40), 22, FLinearColor::White);

	// Top-right: time & speed
	TimeText = MakeText(RootCanvas, TEXT("06:00"), FVector2D(-30, 30), FVector2D(300, 50), 28, FLinearColor::White, FVector2D(1, 0), FVector2D(1, 0), ETextJustify::Right);
	SpeedText = MakeText(RootCanvas, TEXT("Vitesse x1 (touches 1/2/3)"), FVector2D(-30, 85), FVector2D(400, 40), 20, FLinearColor(0.7f, 0.9f, 1.0f), FVector2D(1, 0), FVector2D(1, 0), ETextJustify::Right);

	// Bottom-left: selected crop & inventory
	SelectedCropText = MakeText(RootCanvas, TEXT("Culture : Carotte (C pour changer)"), FVector2D(30, -130), FVector2D(600, 40), 22, FLinearColor(0.6f, 1.0f, 0.6f), FVector2D(0, 1), FVector2D(0, 1));
	InventoryText = MakeText(RootCanvas, TEXT("Inventaire : (vide)"), FVector2D(30, -88), FVector2D(700, 40), 20, FLinearColor::White, FVector2D(0, 1), FVector2D(0, 1));

	// Top-center: save/load feedback
	SystemMessageText = MakeText(RootCanvas, TEXT(""), FVector2D(0, 30), FVector2D(700, 40), 20, FLinearColor(0.5f, 0.85f, 1.0f), FVector2D(0.5f, 0), FVector2D(0.5f, 0), ETextJustify::Center);

	// Bottom-center: interaction prompt & feedback message
	PromptText = MakeText(RootCanvas, TEXT(""), FVector2D(0, -220), FVector2D(800, 50), 26, FLinearColor(1.0f, 0.95f, 0.4f), FVector2D(0.5f, 1), FVector2D(0.5f, 1), ETextJustify::Center);
	MessageText = MakeText(RootCanvas, TEXT(""), FVector2D(0, -175), FVector2D(900, 40), 20, FLinearColor(0.8f, 1.0f, 0.85f), FVector2D(0.5f, 1), FVector2D(0.5f, 1), ETextJustify::Center);

	// Bottom bar: always-visible control reminders (plain text, no background box -
	// UBorder's default brush does not reliably render for widgets built purely in C++).
	ControlsHintText = MakeText(RootCanvas, TEXT(
		"Z Q S D : Deplacer   |   E : Interagir   |   C : Changer de culture   |   1/2/3 : Vitesse   |   F5 : Sauvegarder   |   F9 : Charger   |   R : Rejouer (fin d'annee)"),
		FVector2D(0, -15), FVector2D(1300, 40), 18, FLinearColor(1.0f, 1.0f, 0.75f), FVector2D(0.5f, 1), FVector2D(0.5f, 1), ETextJustify::Center);

	// Center message shown at the end of a year (plain text, large and bold-ish via font size).
	YearEndText = MakeText(RootCanvas, TEXT(""), FVector2D(0, 0), FVector2D(900, 400), 32,
		FLinearColor(1.0f, 0.95f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), ETextJustify::Center);
	YearEndText->SetAutoWrapText(true);
	YearEndText->SetVisibility(ESlateVisibility::Collapsed);
}

UTextBlock* UFarmHUDWidget::MakeText(UCanvasPanel* Parent, const FString& InitialText, FVector2D Position, FVector2D Size, int32 FontSize, FLinearColor Color, FVector2D Anchor, FVector2D Alignment, ETextJustify::Type Justify)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(InitialText));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(Justify);

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);

	UCanvasPanelSlot* CanvasSlot = Parent->AddChildToCanvas(TextBlock);
	CanvasSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetSize(Size);
	CanvasSlot->SetPosition(Position);

	return TextBlock;
}

AFarmPlayerPawn* UFarmHUDWidget::GetWatchedPawn() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<AFarmPlayerPawn>(PC->GetPawn()) : nullptr;
}

void UFarmHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AFarmGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFarmGameState>() : nullptr;
	AFarmPlayerPawn* Pawn = GetWatchedPawn();

	if (GameState)
	{
		MoneyText->SetText(FText::FromString(FString::Printf(TEXT("Argent : %d$"), GameState->Money)));
		DateText->SetText(FText::FromString(FString::Printf(TEXT("%s - Jour %d/%d - An %d"),
			*FFarmCropDatabase::GetSeasonDisplayName(GameState->CurrentSeason).ToString(),
			GameState->DayInSeason, GameState->DaysPerSeason, GameState->YearNumber)));

		const int32 Hour = FMath::FloorToInt(GameState->TimeOfDay);
		const int32 Minute = FMath::FloorToInt((GameState->TimeOfDay - Hour) * 60.0f);
		TimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Hour % 24, Minute)));
		SpeedText->SetText(FText::FromString(FString::Printf(TEXT("Vitesse x%d (touches 1/2/3)"), GameState->GameSpeed)));

		FString InventoryString = TEXT("Inventaire : ");
		bool bAny = false;
		for (const ECropType CropType : FFarmCropDatabase::GetAllCropTypes())
		{
			if (const int32* Count = GameState->Inventory.Find(CropType))
			{
				if (*Count > 0)
				{
					InventoryString += FString::Printf(TEXT("%s x%d  "), *FFarmCropDatabase::Get(CropType).DisplayName, *Count);
					bAny = true;
				}
			}
		}
		if (!bAny)
		{
			InventoryString += TEXT("(vide)");
		}
		InventoryText->SetText(FText::FromString(InventoryString));

		if (SystemMessageText)
		{
			SystemMessageText->SetText(GameState->SystemMessageTimeRemaining > 0.0f ? GameState->SystemMessage : FText::GetEmpty());
		}

		if (YearEndText)
		{
			if (GameState->bYearComplete)
			{
				YearEndText->SetVisibility(ESlateVisibility::Visible);
				const bool bWon = GameState->Money >= GameState->GoalMoney;
				YearEndText->SetText(FText::FromString(FString::Printf(
					TEXT("%s\n\nAn %d termine !\nArgent final : %d$ (objectif : %d$)\n\nAppuyez sur [R] pour rejouer"),
					bWon ? TEXT("VICTOIRE !") : TEXT("Annee terminee"),
					GameState->YearNumber, GameState->Money, GameState->GoalMoney)));
			}
			else
			{
				YearEndText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	if (Pawn)
	{
		SelectedCropText->SetText(FText::FromString(FString::Printf(TEXT("Culture : %s (C pour changer)"),
			*FFarmCropDatabase::Get(Pawn->GetSelectedCrop()).DisplayName)));

		AActor* Target = Pawn->GetCurrentInteractTarget();
		if (ACropPlot* Plot = Cast<ACropPlot>(Target))
		{
			PromptText->SetText(Plot->GetInteractionPrompt(Pawn->GetSelectedCrop()));
		}
		else if (ASellBuilding* Building = Cast<ASellBuilding>(Target))
		{
			PromptText->SetText(Building->GetInteractionPrompt());
		}
		else
		{
			PromptText->SetText(FText::GetEmpty());
		}

		if (Pawn->LastMessageTimeRemaining > 0.0f)
		{
			MessageText->SetText(Pawn->LastMessage);
		}
		else
		{
			MessageText->SetText(FText::GetEmpty());
		}
	}
}
