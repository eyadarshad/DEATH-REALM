// LevelCardWidget.cpp - REDESIGNED
#include "LevelCardWidget.h"
#include "MazeGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Engine/Engine.h"

void ULevelCardWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind button events
    if (CardButton)
    {
        CardButton->OnClicked.AddDynamic(this, &ULevelCardWidget::HandleCardClicked);
        CardButton->OnHovered.AddDynamic(this, &ULevelCardWidget::OnCardHoverStart);
        CardButton->OnUnhovered.AddDynamic(this, &ULevelCardWidget::OnCardHoverEnd);
    }
    
    // Initialize animation state
    CurrentScale = 1.0f;
    TargetScale = 1.0f;
    CurrentTopOpacity = 1.0f;
    TargetTopOpacity = 1.0f;
    CurrentBackOpacity = 0.0f;
    TargetBackOpacity = 0.0f;
    
    // Set initial back elements to hidden (not just transparent)
    if (BackImage)
    {
        BackImage->SetVisibility(ESlateVisibility::Hidden);
        BackImage->SetRenderOpacity(0.0f);
    }
    
    if (BackText1)
    {
        BackText1->SetVisibility(ESlateVisibility::Hidden);
        BackText1->SetRenderOpacity(0.0f);
    }
}

void ULevelCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Smooth animation interpolation
    CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, InDeltaTime, AnimationSpeed);
    CurrentTopOpacity = FMath::FInterpTo(CurrentTopOpacity, TargetTopOpacity, InDeltaTime, AnimationSpeed);
    CurrentBackOpacity = FMath::FInterpTo(CurrentBackOpacity, TargetBackOpacity, InDeltaTime, AnimationSpeed);
    
    // Apply scale to CardButton (scales entire card)
    if (CardButton)
    {
        CardButton->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
    }
    
    // Apply opacity to TopImage
    if (TopImage)
    {
        TopImage->SetRenderOpacity(CurrentTopOpacity);
    }
    
    // TopText1 - Completely disappear on hover
    if (TopText1)
    {
        TopText1->SetRenderOpacity(CurrentTopOpacity);
        // Also change visibility to hidden when fully transparent
        if (CurrentTopOpacity < 0.1f)
        {
            TopText1->SetVisibility(ESlateVisibility::Hidden);
        }
        else
        {
            TopText1->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
    
    // BackImage and BackText1 - Appear on hover
    if (BackImage)
    {
        BackImage->SetRenderOpacity(CurrentBackOpacity);
        // Change visibility based on opacity
        if (CurrentBackOpacity > 0.01f)
        {
            BackImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            BackImage->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    
    if (BackText1)
    {
        BackText1->SetRenderOpacity(CurrentBackOpacity);
        // Change visibility based on opacity
        if (CurrentBackOpacity > 0.01f)
        {
            BackText1->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            BackText1->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void ULevelCardWidget::InitializeCard(int32 InLevelNumber, bool bInIsLocked, int32 InStars, float InBestTime, const FString& InLevelName)
{
    LevelNumber = InLevelNumber;
    bIsLocked = bInIsLocked;
    StarsEarned = InStars;
    BestTime = InBestTime;
    LevelName = InLevelName;
    
    UpdateCardVisuals();
}

void ULevelCardWidget::UpdateCardVisuals()
{
    // Update TopText1 - "LEVEL 1", "LEVEL 2", etc.
    if (TopText1)
    {
        TopText1->SetText(FText::FromString(FString::Printf(TEXT("LEVEL %d"), LevelNumber)));
        
        // Set text color based on lock state
        FLinearColor TextColor = bIsLocked ? 
            FLinearColor(0.5f, 0.5f, 0.5f, 1.0f) :  // Gray for locked
            FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);   // White for unlocked
        
        TopText1->SetColorAndOpacity(FSlateColor(TextColor));
    }
    
    // Update BackText1 - Level name
    if (BackText1)
    {
        BackText1->SetText(FText::FromString(LevelName));
    }
    
    // Update TopImage color based on lock state
    if (TopImage)
    {
        FLinearColor ImageColor = bIsLocked ?
            FLinearColor(0.4f, 0.4f, 0.4f, 1.0f) :  // Dark gray for locked
            FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);   // Full color for unlocked
        
        TopImage->SetColorAndOpacity(ImageColor);
    }
    
    // Update BackImage color
    if (BackImage)
    {
        FLinearColor BackColor = bIsLocked ?
            FLinearColor(0.3f, 0.3f, 0.3f, 1.0f) :  // Darker gray for locked
            FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);   // Full color for unlocked
        
        BackImage->SetColorAndOpacity(BackColor);
    }
    
    // Update button enabled state
    if (CardButton)
    {
        CardButton->SetIsEnabled(!bIsLocked);
    }
}

void ULevelCardWidget::OnCardHoverStart()
{
    if (bIsLocked)
    {
        // Locked cards don't respond to hover
        return;
    }
    
    bIsHovered = true;
    
    // Set animation targets
    TargetScale = HoverScale;  // Scale up to 1.15
    TargetTopOpacity = 0.0f;  // TopImage AND TopText1 fade to 0.0 (completely transparent)
    TargetBackOpacity = 1.0f;  // BackImage and BackText1 fade in
    
    UE_LOG(LogTemp, Log, TEXT("[LevelCard] Hover started on Level %d"), LevelNumber);
}

void ULevelCardWidget::OnCardHoverEnd()
{
    if (bIsLocked)
    {
        return;
    }
    
    bIsHovered = false;
    
    // Return to normal state
    TargetScale = 1.0f;
    TargetTopOpacity = 1.0f;  // TopImage fully visible
    TargetBackOpacity = 0.0f;  // BackImage and BackText1 invisible
    
    UE_LOG(LogTemp, Log, TEXT("[LevelCard] Hover ended on Level %d"), LevelNumber);
}

void ULevelCardWidget::HandleCardClicked()
{
    if (bIsLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelCard] Level %d is locked!"), LevelNumber);
        return;
    }
    
    OnCardClicked();
}

void ULevelCardWidget::OnCardClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[LevelCard] Level %d selected!"), LevelNumber);
    
    // Get game mode and load level
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->LoadSelectedLevel(LevelNumber);
    }
}
