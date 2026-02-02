// MazeGameSettings.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MazeGameSettings.generated.h"

/**
 * SaveGame class to store player's maze settings
 */
UCLASS()
class MAZERUNNER_API UMazeGameSettings : public USaveGame
{
    GENERATED_BODY()
    
public:
    UMazeGameSettings();
    
    // Maze size settings
    UPROPERTY(VisibleAnywhere, Category = "Maze Settings")
    int32 MazeRows;
    
    UPROPERTY(VisibleAnywhere, Category = "Maze Settings")
    int32 MazeCols;
    
    UPROPERTY(VisibleAnywhere, Category = "Options")
    float MouseSensitivity;
    
    UPROPERTY(VisibleAnywhere, Category = "Options")
    float GameVolume;
    
    UPROPERTY(VisibleAnywhere, Category = "Options")
    FString GraphicsQuality;  // "Low", "Medium", "High", "Ultra High"
    
    // ==================== LEVEL PROGRESSION DATA ====================
    
    UPROPERTY(VisibleAnywhere, Category = "Level Progression")
    int32 HighestUnlockedLevel;
    
    UPROPERTY(VisibleAnywhere, Category = "Level Progression")
    TMap<int32, int32> LevelStars;  // Level -> Stars earned (0-3)
    
    UPROPERTY(VisibleAnywhere, Category = "Level Progression")
    TMap<int32, float> LevelBestTimes;  // Level -> Best completion time
    
    UPROPERTY(VisibleAnywhere, Category = "Level Progression")
    int32 TotalStarsEarned;
    
    UPROPERTY(VisibleAnywhere, Category = "Level Progression")
    int32 TotalLevelsCompleted;
    
    // Save/Load functions for settings
    void SaveGraphicsQuality(const FString& Quality);
    FString LoadGraphicsQuality();
    void SaveMouseSensitivity(float Sensitivity);
    float LoadMouseSensitivity();
    void SaveGameVolume(float Volume);
    float LoadGameVolume();
    
    // Save slot name
    static const FString SaveSlotName;
    static const int32 UserIndex;
};
