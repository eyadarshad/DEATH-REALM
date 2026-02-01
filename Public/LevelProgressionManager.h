// LevelProgressionManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeRunnerSaveGame.h"
#include "LevelProgressionManager.generated.h"

// Level statistics tracking
USTRUCT(BlueprintType)
struct FLevelStats
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    float BestTime = 0.0f;
    
    UPROPERTY(BlueprintReadOnly)
    int32 StarsEarned = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 TotalAttempts = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 TotalCompletions = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 DeathsByMonster = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 DeathsByTimeout = 0;
};

// Complete level configuration
USTRUCT(BlueprintType)
struct FLevelConfig
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    int32 LevelNumber = 1;
    
    UPROPERTY(BlueprintReadOnly)
    FString LevelName = TEXT("Unknown");
    
    UPROPERTY(BlueprintReadOnly)
    FString LevelDescription = TEXT("");
    
    // Maze configuration
    UPROPERTY(BlueprintReadOnly)
    int32 MazeRows = 15;
    
    UPROPERTY(BlueprintReadOnly)
    int32 MazeCols = 15;
    
    UPROPERTY(BlueprintReadOnly)
    float LoopProbability = 0.1f;
    
    // Timing
    UPROPERTY(BlueprintReadOnly)
    float TimeLimit = 300.0f;
    
    UPROPERTY(BlueprintReadOnly)
    float MonsterSpawnDelay = -1.0f;  // -1 = disabled
    
    UPROPERTY(BlueprintReadOnly)
    float MazeRegenTime = -1.0f;  // -1 = disabled, otherwise time when regen occurs
    
    // Level 5 Blood Moon - Second Monster
    UPROPERTY(BlueprintReadOnly)
    float SecondMonsterSpawnTime = -1.0f;  // -1 = disabled, time when second monster spawns
    
    // Level 5 Blood Moon - Aggressive Mode
    UPROPERTY(BlueprintReadOnly)
    float AggressiveModeTime = -1.0f;  // -1 = disabled, time when monsters become aggressive
    
    // Golden Star Toggle
    UPROPERTY(BlueprintReadOnly)
    bool bEnableGoldenStar = true;  // Default: enabled, Level 5: disabled
    
    // Trap Cell Count
    UPROPERTY(BlueprintReadOnly)
    int32 NumTrapCells = 2;  // Default: 2 for levels 3-4, Level 5: 6
    
    // Monster configuration
    UPROPERTY(BlueprintReadOnly)
    bool bEnableMonster = false;
    
    UPROPERTY(BlueprintReadOnly)
    int32 NumMonsters = 1;  // Number of monsters to spawn
    
    UPROPERTY(BlueprintReadOnly)
    float MonsterSpeed = 300.0f;
    
    // Hazards
    UPROPERTY(BlueprintReadOnly)
    int32 NumMuddyPatches = 0;
    
    UPROPERTY(BlueprintReadOnly)
    bool bEnableMazeTraps = false;
    
    UPROPERTY(BlueprintReadOnly)
    float MazeTrapProbability = 0.0f;
    
    // Star rating thresholds (in seconds)
    UPROPERTY(BlueprintReadOnly)
    float ThreeStarTime = 0.0f;
    
    UPROPERTY(BlueprintReadOnly)
    float TwoStarTime = 0.0f;
    
    UPROPERTY(BlueprintReadOnly)
    float OneStarTime = 0.0f;
    
    // Pre-level briefing
    UPROPERTY(BlueprintReadOnly)
    TArray<FString> BriefingMessages;
    
    // New hazards introduced this level
    UPROPERTY(BlueprintReadOnly)
    TArray<FString> NewHazards;
};

UCLASS()
class MAZERUNNER_API ALevelProgressionManager : public AActor
{
    GENERATED_BODY()
    
public:    
    ALevelProgressionManager();

protected:
    virtual void BeginPlay() override;

public:    
    // Level configurations (all 5 levels)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    TArray<FLevelConfig> LevelConfigs;
    
    // Current state
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    int32 CurrentLevel;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    int32 HighestUnlockedLevel;
    
    // Statistics
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    TMap<int32, FLevelStats> LevelStatistics;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    int32 TotalStarsEarned;
    
    // ==================== CORE FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void InitializeLevelConfigs();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    FLevelConfig GetLevelConfig(int32 LevelNumber) const;
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    bool IsLevelUnlocked(int32 LevelNumber) const;
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void UnlockLevel(int32 LevelNumber);
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    int32 CalculateStarRating(int32 LevelNumber, float CompletionTime) const;
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void RecordLevelCompletion(int32 LevelNumber, float CompletionTime, int32 Stars);
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void RecordLevelAttempt(int32 LevelNumber);
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void RecordDeath(int32 LevelNumber, bool bByMonster);
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    FLevelStats GetLevelStats(int32 LevelNumber) const;
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    int32 GetTotalStars() const;
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void ResetProgress();
    
    // Save/Load system
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void SaveProgress();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void LoadProgress();

private:
    void CreateLevel1Config();
    void CreateLevel2Config();
    void CreateLevel3Config();
    void CreateLevel4Config();
    void CreateLevel5Config();
    
    // Save game slot name
    FString SaveSlotName = TEXT("MazeRunnerProgress");
};
