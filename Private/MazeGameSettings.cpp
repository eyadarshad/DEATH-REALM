// MazeGameSettings.cpp
#include "MazeGameSettings.h"
#include "MazeRunnerSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UMazeGameSettings::SaveSlotName = TEXT("MazeSettings");
const int32 UMazeGameSettings::UserIndex = 0;

UMazeGameSettings::UMazeGameSettings()
{
    // Default maze size
    MazeRows = 15;
    MazeCols = 15;
    MouseSensitivity = 1.0f;
    GameVolume = 0.8f;
    GraphicsQuality = TEXT("Ultra High");  // Default to Ultra High (100% resolution)
    
    // Level progression defaults
    HighestUnlockedLevel = 1;  // Only Level 1 unlocked initially
    TotalStarsEarned = 0;
    TotalLevelsCompleted = 0;
    LevelStars.Empty();
    LevelBestTimes.Empty();
}

void UMazeGameSettings::SaveGraphicsQuality(const FString& Quality)
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    
    if (!SaveGameInstance)
    {
        SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::CreateSaveGameObject(UMazeRunnerSaveGame::StaticClass()));
    }
    
    if (SaveGameInstance)
    {
        SaveGameInstance->GraphicsQuality = Quality;
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("PlayerSave"), 0);
        UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] Saved graphics quality: %s"), *Quality);
    }
}

FString UMazeGameSettings::LoadGraphicsQuality()
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    
    if (SaveGameInstance && !SaveGameInstance->GraphicsQuality.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] Loaded graphics quality: %s"), *SaveGameInstance->GraphicsQuality);
        return SaveGameInstance->GraphicsQuality;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] No saved graphics quality, defaulting to Ultra High"));
    return TEXT("Ultra High");  // Default to Ultra High
}

void UMazeGameSettings::SaveMouseSensitivity(float Sensitivity)
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    
    if (!SaveGameInstance)
    {
        SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::CreateSaveGameObject(UMazeRunnerSaveGame::StaticClass()));
    }
    
    if (SaveGameInstance)
    {
        SaveGameInstance->MouseSensitivity = Sensitivity;
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("PlayerSave"), 0);
        UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] Saved mouse sensitivity: %.2f"), Sensitivity);
    }
}

float UMazeGameSettings::LoadMouseSensitivity()
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    
    if (SaveGameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] Loaded mouse sensitivity: %.2f"), SaveGameInstance->MouseSensitivity);
        return SaveGameInstance->MouseSensitivity;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] No saved mouse sensitivity, defaulting to 1.0"));
    return 1.0f;  // Default
}

void UMazeGameSettings::SaveGameVolume(float Volume)
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    
    if (!SaveGameInstance)
    {
        SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::CreateSaveGameObject(UMazeRunnerSaveGame::StaticClass()));
    }
    
    if (SaveGameInstance)
    {
        SaveGameInstance->GameVolume = Volume;
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("PlayerSave"), 0);
        UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] Saved game volume: %.2f"), Volume);
    }
}

float UMazeGameSettings::LoadGameVolume()
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    
    if (SaveGameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] Loaded game volume: %.2f"), SaveGameInstance->GameVolume);
        return SaveGameInstance->GameVolume;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeGameSettings] No saved game volume, defaulting to 0.8"));
    return 0.8f;  // Default
}
