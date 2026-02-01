// MazeGameSettings.cpp
#include "MazeGameSettings.h"

const FString UMazeGameSettings::SaveSlotName = TEXT("MazeSettings");
const int32 UMazeGameSettings::UserIndex = 0;

UMazeGameSettings::UMazeGameSettings()
{
    // Default maze size
    MazeRows = 15;
    MazeCols = 15;
    MouseSensitivity = 1.0f;
    GameVolume = 0.8f;
    
    // Level progression defaults
    HighestUnlockedLevel = 1;  // Only Level 1 unlocked initially
    TotalStarsEarned = 0;
    TotalLevelsCompleted = 0;
    LevelStars.Empty();
    LevelBestTimes.Empty();
}
