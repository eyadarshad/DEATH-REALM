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
    
    // Save slot name
    static const FString SaveSlotName;
    static const int32 UserIndex;
};
