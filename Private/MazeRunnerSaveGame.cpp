// MazeRunnerSaveGame.cpp
#include "MazeRunnerSaveGame.h"

UMazeRunnerSaveGame::UMazeRunnerSaveGame()
{
    HighestUnlockedLevel = 1;  // Level 1 always unlocked
    TotalStarsEarned = 0;
    
    // Default settings - Ultra High quality
    GraphicsQuality = TEXT("Ultra High");
    MouseSensitivity = 1.0f;
    GameVolume = 0.8f;
}
