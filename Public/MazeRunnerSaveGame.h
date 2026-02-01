// MazeRunnerSaveGame.h - Save game data
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MazeRunnerSaveGame.generated.h"

UCLASS()
class MAZERUNNER_API UMazeRunnerSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UMazeRunnerSaveGame();

    UPROPERTY()
    int32 HighestUnlockedLevel;

    UPROPERTY()
    int32 TotalStarsEarned;

    UPROPERTY()
    TMap<int32, int32> LevelStars;  // Level number -> Stars earned

    UPROPERTY()
    TMap<int32, float> LevelBestTimes;  // Level number -> Best time
};
