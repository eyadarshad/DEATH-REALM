// SettingsMenuWidget.cpp
#include "SettingsMenuWidget.h"
#include "MazeGameMode.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void USettingsMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] === NativeConstruct called ==="));
    
    // Get the game mode
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsWidget] Failed to get MazeGameMode!"));
        return;
    }
    
    // Check if widgets were found
    UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] SpinBox_Rows: %s"), SpinBox_Rows ? TEXT("FOUND") : TEXT("NOT FOUND"));
    UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] SpinBox_Cols: %s"), SpinBox_Cols ? TEXT("FOUND") : TEXT("NOT FOUND"));
    UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] Button_Save: %s"), Button_Save ? TEXT("FOUND") : TEXT("NOT FOUND"));
    UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] Button_Cancel: %s"), Button_Cancel ? TEXT("FOUND") : TEXT("NOT FOUND"));
    
    // Initialize spinbox values from current settings
    if (SpinBox_Rows)
    {
        SpinBox_Rows->SetValue(GameMode->GetMazeRows());
        SpinBox_Rows->SetMinValue(1.0f);
        SpinBox_Rows->SetMaxValue(30.0f);
        SpinBox_Rows->SetDelta(1.0f); // Increment by 1
        SpinBox_Rows->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::OnRowsChanged);
        UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] ✓ Rows spinbox initialized to %d"), GameMode->GetMazeRows());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsWidget] ✗ SpinBox_Rows is NULL! Check widget name in Blueprint."));
    }
    
    if (SpinBox_Cols)
    {
        SpinBox_Cols->SetValue(GameMode->GetMazeCols());
        SpinBox_Cols->SetMinValue(1.0f);
        SpinBox_Cols->SetMaxValue(30.0f);
        SpinBox_Cols->SetDelta(1.0f); // Increment by 1
        SpinBox_Cols->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::OnColsChanged);
        UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] ✓ Cols spinbox initialized to %d"), GameMode->GetMazeCols());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SettingsWidget] ✗ SpinBox_Cols is NULL! Check widget name in Blueprint."));
    }
    
    // Bind button click events
    if (Button_Save)
    {
        Button_Save->OnClicked.AddDynamic(this, &USettingsMenuWidget::StartGame);
        UE_LOG(LogTemp, Warning, TEXT("[Free2Play] ✓ Start button bound"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Free2Play] ✗ Button_Save is NULL! Check widget name in Blueprint."));
    }
    
    if (Button_Cancel)
    {
        Button_Cancel->OnClicked.AddDynamic(this, &USettingsMenuWidget::BackToMainMenu);
        UE_LOG(LogTemp, Warning, TEXT("[Free2Play] ✓ Back button bound"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Free2Play] ✗ Button_Cancel is NULL! Check widget name in Blueprint."));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[SettingsWidget] === Initialization complete ==="));
}

void USettingsMenuWidget::StartGame()
{
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("[Free2Play] GameMode not found!"));
        return;
    }

    // Get values from spinboxes
    int32 SelectedRows = 0;
    int32 SelectedCols = 0;

    if (SpinBox_Rows)
    {
        SelectedRows = FMath::RoundToInt(SpinBox_Rows->GetValue());
    }
    else
    {
        SelectedRows = GameMode->GetMazeRows();
    }

    if (SpinBox_Cols)
    {
        SelectedCols = FMath::RoundToInt(SpinBox_Cols->GetValue());
    }
    else
    {
        SelectedCols = GameMode->GetMazeCols();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Starting game with custom maze: %dx%d"), SelectedRows, SelectedCols);
    
    // Set maze size in GameMode
    GameMode->SetMazeRows(SelectedRows);
    GameMode->SetMazeCols(SelectedCols);
    
    // Remove this widget first
    RemoveFromParent();
    
    // CRITICAL: Reset input mode to game before starting
    // This fixes the cursor/controls glitch in Free2Play mode
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    
    // Regenerate entire maze with new settings
    GameMode->GenerateMaze();
    
    // Start the game directly (no menus, straight to gameplay)
    GameMode->StartGame();
}

void USettingsMenuWidget::BackToMainMenu()
{
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("[Free2Play] GameMode not found!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Returning to main menu"));
    
    // Remove this widget
    RemoveFromParent();
    
    // Show main menu again
    GameMode->ShowMainMenu();
}

void USettingsMenuWidget::OnRowsChanged(float Value)
{
    // Optional: Update settings in real-time as user drags slider
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->SetMazeRows(FMath::RoundToInt(Value));
    }
}

void USettingsMenuWidget::OnColsChanged(float Value)
{
    // Optional: Update settings in real-time as user drags slider
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->SetMazeCols(FMath::RoundToInt(Value));
    }
}
