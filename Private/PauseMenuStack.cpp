// PauseMenu.cpp
#include "PauseMenuStack.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

APauseMenuStack::APauseMenuStack()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsPaused = false;
    PauseWidget = nullptr;
}

void APauseMenuStack::BeginPlay()
{
    Super::BeginPlay();
}

void APauseMenuStack::TogglePause()
{
    bIsPaused = !bIsPaused;
    
    if (bIsPaused)
    {
        // Show pause menu
        if (PauseWidgetClass && !PauseWidget)
        {
            PauseWidget = CreateWidget<UUserWidget>(GetWorld(), PauseWidgetClass);
            if (PauseWidget)
            {
                PauseWidget->AddToViewport();
            }
        }
        
        // Set input mode to UI
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeUIOnly());
        }
        
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        UE_LOG(LogTemp, Log, TEXT("Game Paused"));
    }
    else
    {
        // Hide pause menu
        if (PauseWidget)
        {
            PauseWidget->RemoveFromParent();
            PauseWidget = nullptr;
        }
        
        // Set input mode back to game
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
        
        UGameplayStatics::SetGamePaused(GetWorld(), false);
        UE_LOG(LogTemp, Log, TEXT("Game Resumed"));
    }
}
