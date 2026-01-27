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
}
