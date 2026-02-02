// LoadingScreenWidget.cpp
#include "LoadingScreenWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void ULoadingScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    InitializeGameplayTips();
    ShowNextTip();  // Show first tip
    RotationAngle = 0.0f;
    TipTimer = 0.0f;
}

void ULoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Rotate the spinner
    if (Image_Spinner)
    {
        RotationAngle += RotationSpeed * InDeltaTime;
        if (RotationAngle >= 360.0f)
        {
            RotationAngle -= 360.0f;
        }
        
        // Apply rotation to image
        Image_Spinner->SetRenderTransformAngle(RotationAngle);
    }
    
    // Rotate tips every 5 seconds
    TipTimer += InDeltaTime;
    if (TipTimer >= TipChangeInterval)
    {
        TipTimer = 0.0f;
        ShowNextTip();
    }
}

void ULoadingScreenWidget::SetInstructions(const FString& Instructions)
{
    if (Text_Instructions)
    {
        Text_Instructions->SetText(FText::FromString(Instructions));
    }
}

void ULoadingScreenWidget::InitializeGameplayTips()
{
    GameplayTips.Empty();
    
    // Controls
    GameplayTips.Add(TEXT("Use W, A, S, D to move through the maze"));
    GameplayTips.Add(TEXT("Move your mouse to look around and navigate"));
    GameplayTips.Add(TEXT("Press SPACE to jump over obstacles"));
    GameplayTips.Add(TEXT("Press ESC to pause the game"));
    
    // Objectives
    GameplayTips.Add(TEXT("Find the glowing Golden Star to escape the maze!"));
    GameplayTips.Add(TEXT("Locate the escape route before time runs out"));
    GameplayTips.Add(TEXT("You have limited time to complete the maze"));
    
    // Traps and Hazards
    GameplayTips.Add(TEXT("Avoid muddy patches - they slow you down and blur your vision!"));
    GameplayTips.Add(TEXT("Muddy traps reduce your movement speed temporarily"));
    GameplayTips.Add(TEXT("Some traps regenerate the maze - plan your route carefully!"));
    
    // Monster
    GameplayTips.Add(TEXT("A monster spawns after some time - stay alert!"));
    GameplayTips.Add(TEXT("The monster becomes EXTREMELY dangerous in the last 30 seconds!"));
    GameplayTips.Add(TEXT("Avoid the monster at all costs - it gets faster over time"));
    
    // Strategy
    GameplayTips.Add(TEXT("Explore quickly but carefully - time is limited"));
    GameplayTips.Add(TEXT("Listen for audio cues to locate the Golden Star"));
    
    // Shuffle tips for random order
    int32 LastIndex = GameplayTips.Num() - 1;
    for (int32 i = 0; i <= LastIndex; ++i)
    {
        int32 RandomIndex = FMath::RandRange(i, LastIndex);
        GameplayTips.Swap(i, RandomIndex);
    }
}

void ULoadingScreenWidget::ShowNextTip()
{
    if (GameplayTips.Num() > 0)
    {
        SetInstructions(GameplayTips[CurrentTipIndex]);
        CurrentTipIndex = (CurrentTipIndex + 1) % GameplayTips.Num();
    }
}

void ULoadingScreenWidget::SetLevelNumber(int32 LevelNum)
{
    if (Text_LevelNumber)
    {
        // Set the level number text
        FString LevelText = FString::Printf(TEXT("Starting Level %d..."), LevelNum);
        Text_LevelNumber->SetText(FText::FromString(LevelText));
        Text_LevelNumber->SetVisibility(ESlateVisibility::Visible);
        
        UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] Displaying: %s"), *LevelText);
        
        // Hide after 2 seconds
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (Text_LevelNumber)
            {
                Text_LevelNumber->SetVisibility(ESlateVisibility::Collapsed);
                UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] Level number hidden after 2 seconds"));
            }
        }, 2.0f, false);
    }
}

