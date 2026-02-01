// LevelSelectWidget.cpp
#include "LevelSelectWidget.h"
#include "MazeGameMode.h"
#include "LevelProgressionManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/Button.h"

void ULevelSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind back button
    if (BackButton)
    {
        BackButton->OnClicked.AddDynamic(this, &ULevelSelectWidget::HandleBackButtonClicked);
    }
    
    // Populate cards on construction
    PopulateCards();
}

void ULevelSelectWidget::PopulateCards()
{
    if (!LevelGrid || !LevelCardClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[LevelSelect] Missing LevelGrid or LevelCardClass!"));
        return;
    }
    
    // Clear existing cards
    LevelGrid->ClearChildren();
    LevelCards.Empty();
    
    // Get game mode and level manager
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GameMode || !GameMode->LevelManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[LevelSelect] No GameMode or LevelManager!"));
        return;
    }
    
    ALevelProgressionManager* LevelManager = GameMode->LevelManager;
    
    // Create 5 level cards - WrapBox handles wrapping automatically!
    for (int32 i = 1; i <= 5; i++)
    {
        // Create card widget
        ULevelCardWidget* Card = CreateWidget<ULevelCardWidget>(GetWorld(), LevelCardClass);
        if (!Card)
        {
            UE_LOG(LogTemp, Error, TEXT("[LevelSelect] Failed to create card for Level %d"), i);
            continue;
        }
        
        // Get level data
        bool bIsLocked = !LevelManager->IsLevelUnlocked(i);
        FLevelStats Stats = LevelManager->GetLevelStats(i);
        FLevelConfig Config = LevelManager->GetLevelConfig(i);
        
        // Initialize card
        Card->InitializeCard(i, bIsLocked, Stats.StarsEarned, Stats.BestTime, Config.LevelName);
        
        // Add to WrapBox - it will automatically wrap to next row if needed!
        UWrapBoxSlot* WrapSlot = LevelGrid->AddChildToWrapBox(Card);
        if (WrapSlot)
        {
            WrapSlot->SetHorizontalAlignment(HAlign_Center);
            WrapSlot->SetVerticalAlignment(VAlign_Center);
            WrapSlot->SetPadding(FMargin(15.0f)); // Space between cards
        }
        
        LevelCards.Add(Card);
        
        UE_LOG(LogTemp, Log, TEXT("[LevelSelect] Created card for Level %d (Locked: %s, Stars: %d)"), 
               i, bIsLocked ? TEXT("Yes") : TEXT("No"), Stats.StarsEarned);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[LevelSelect] ✓ Populated %d level cards in WrapBox"), LevelCards.Num());
}

void ULevelSelectWidget::RefreshCardStates()
{
    // Refresh all cards with latest data
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GameMode || !GameMode->LevelManager) return;
    
    ALevelProgressionManager* LevelManager = GameMode->LevelManager;
    
    for (int32 i = 0; i < LevelCards.Num(); i++)
    {
        if (LevelCards[i])
        {
            int32 LevelNum = i + 1;
            bool bIsLocked = !LevelManager->IsLevelUnlocked(LevelNum);
            FLevelStats Stats = LevelManager->GetLevelStats(LevelNum);
            FLevelConfig Config = LevelManager->GetLevelConfig(LevelNum);
            
            LevelCards[i]->InitializeCard(LevelNum, bIsLocked, Stats.StarsEarned, Stats.BestTime, Config.LevelName);
        }
    }
}

void ULevelSelectWidget::HandleBackButtonClicked()
{
    OnBackButtonClicked();
}

void ULevelSelectWidget::OnBackButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[LevelSelect] Back button clicked"));
    
    // Remove this widget from viewport first
    RemoveFromParent();
    
    // Get game mode and return to main menu
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        // Clear the reference in game mode
        GameMode->LevelSelectWidget = nullptr;
        
        // Show main menu
        GameMode->ShowMainMenu();
        
        UE_LOG(LogTemp, Warning, TEXT("[LevelSelect] ✓ Returned to main menu"));
    }
}
