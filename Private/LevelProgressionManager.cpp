// LevelProgressionManager.cpp
#include "LevelProgressionManager.h"
#include "Kismet/GameplayStatics.h"

ALevelProgressionManager::ALevelProgressionManager()
{
    PrimaryActorTick.bCanEverTick = false;
    
    CurrentLevel = 1;
    HighestUnlockedLevel = 1;
    TotalStarsEarned = 0;
}

void ALevelProgressionManager::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeLevelConfigs();
    LoadProgress();  // Load saved progress
    
    UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] Initialized with %d levels"), LevelConfigs.Num());
}

void ALevelProgressionManager::InitializeLevelConfigs()
{
    LevelConfigs.Empty();
    
    CreateLevel1Config();
    CreateLevel2Config();
    CreateLevel3Config();
    CreateLevel4Config();
    CreateLevel5Config();
    
    UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] ✓ All 5 levels configured"));
}

void ALevelProgressionManager::CreateLevel1Config()
{
    FLevelConfig Level1;
    Level1.LevelNumber = 1;
    Level1.LevelName = TEXT("First Steps");
    Level1.LevelDescription = TEXT("Learn the basics and escape your first maze.");
    
    // Maze settings - Easy 10x10 for beginners
    Level1.MazeRows = 10;
    Level1.MazeCols = 10;
    Level1.LoopProbability = 0.1f;
    
    // Timing - Generous
    Level1.TimeLimit = 300.0f;  // 5 minutes
    Level1.MonsterSpawnDelay = -1.0f;  // No monster!
    Level1.MazeRegenTime = -1.0f;  // No maze regeneration
    
    // Monster - Disabled
    Level1.bEnableMonster = false;
    Level1.MonsterSpeed = 0.0f;
    
    // Hazards - None
    Level1.NumMuddyPatches = 0;
    Level1.bEnableMazeTraps = false;
    Level1.MazeTrapProbability = 0.0f;
    
    // Star thresholds
    Level1.ThreeStarTime = 120.0f;  // 2 minutes for 3 stars
    Level1.TwoStarTime = 180.0f;    // 3 minutes for 2 stars
    Level1.OneStarTime = 300.0f;    // 5 minutes for 1 star
    
    // Tutorial briefing - SIMPLE CONTROLS ONLY
    Level1.BriefingMessages.Add(TEXT("=== LEVEL 1: TUTORIAL ==="));
    Level1.BriefingMessages.Add(TEXT(""));
    Level1.BriefingMessages.Add(TEXT("CONTROLS:"));
    Level1.BriefingMessages.Add(TEXT("  W / A / S / D  -  Move Forward / Left / Back / Right"));
    Level1.BriefingMessages.Add(TEXT("  MOUSE          -  Look Around"));
    Level1.BriefingMessages.Add(TEXT("  SPACE          -  Jump"));
    Level1.BriefingMessages.Add(TEXT("  ESC            -  Pause Menu"));
    Level1.BriefingMessages.Add(TEXT(""));
    Level1.BriefingMessages.Add(TEXT("OBJECTIVE:"));
    Level1.BriefingMessages.Add(TEXT("  1. Find the Golden Star"));
    Level1.BriefingMessages.Add(TEXT("  2. Star reveals path to exit"));
    Level1.BriefingMessages.Add(TEXT("  3. Reach glowing exit to win"));
    Level1.BriefingMessages.Add(TEXT(""));
    Level1.BriefingMessages.Add(TEXT("Time Limit: 5 minutes"));
    Level1.BriefingMessages.Add(TEXT("No hazards - learn the basics!"));
    
    Level1.NewHazards.Add(TEXT("No hazards - just learn the basics!"));
    
    LevelConfigs.Add(Level1);
}

void ALevelProgressionManager::CreateLevel2Config()
{
    FLevelConfig Level2;
    Level2.LevelNumber = 2;
    Level2.LevelName = TEXT("The Awakening");
    Level2.LevelDescription = TEXT("The monster awakens. Move quickly!");
    
    // Maze settings
    Level2.MazeRows = 18;
    Level2.MazeCols = 18;
    Level2.LoopProbability = 0.15f;
    
    // Timing
    Level2.TimeLimit = 240.0f;  // 4 minutes
    Level2.MonsterSpawnDelay = 90.0f;  // Monster spawns after 1.5 minutes
    Level2.MazeRegenTime = -1.0f;  // No maze regen yet
    
    // Monster
    Level2.bEnableMonster = true;
    Level2.NumMonsters = 1;  // Single monster
    Level2.MonsterSpeed = 300.0f;
    
    // Hazards
    Level2.NumMuddyPatches = 3;
    Level2.bEnableMazeTraps = false;
    Level2.MazeTrapProbability = 0.0f;
    
    // Star thresholds
    Level2.ThreeStarTime = 120.0f;
    Level2.TwoStarTime = 180.0f;
    Level2.OneStarTime = 240.0f;
    
    // Briefing
    Level2.BriefingMessages.Add(TEXT("=== LEVEL 2: THE AWAKENING ==="));
    Level2.BriefingMessages.Add(TEXT(""));
    Level2.BriefingMessages.Add(TEXT("⚠️ WARNING: NEW THREATS DISCOVERED ⚠️"));
    Level2.BriefingMessages.Add(TEXT(""));
    Level2.BriefingMessages.Add(TEXT("THREAT #1: MONSTER"));
    Level2.BriefingMessages.Add(TEXT("  • Spawns after 90 seconds"));
    Level2.BriefingMessages.Add(TEXT("  • Uses intelligent pathfinding to hunt you"));
    Level2.BriefingMessages.Add(TEXT("  • HOW TO AVOID: Keep moving, don't get cornered"));
    Level2.BriefingMessages.Add(TEXT(""));
    Level2.BriefingMessages.Add(TEXT("THREAT #2: MUDDY PATCHES"));
    Level2.BriefingMessages.Add(TEXT("  • Brown patches on the ground (3 total)"));
    Level2.BriefingMessages.Add(TEXT("  • Slows movement + blurs vision"));
    Level2.BriefingMessages.Add(TEXT("  • HOW TO AVOID: Jump over them!"));
    Level2.BriefingMessages.Add(TEXT(""));
    Level2.BriefingMessages.Add(TEXT("Time Limit: 4 minutes"));
    
    Level2.NewHazards.Add(TEXT("Monster AI (spawns after 90s)"));
    Level2.NewHazards.Add(TEXT("Muddy Patches (3 total)"));
    
    LevelConfigs.Add(Level2);
}

void ALevelProgressionManager::CreateLevel3Config()
{
    FLevelConfig Level3;
    Level3.LevelNumber = 3;
    Level3.LevelName = TEXT("Dark Descent");
    Level3.LevelDescription = TEXT("The maze fights back with regeneration traps.");
    
    // Maze settings
    Level3.MazeRows = 20;
    Level3.MazeCols = 20;
    Level3.LoopProbability = 0.2f;
    
    // Timing
    Level3.TimeLimit = 210.0f;  // 3.5 minutes
    Level3.MonsterSpawnDelay = 60.0f;  // Monster spawns after 1 minute
    Level3.MazeRegenTime = 120.0f;  // Maze regenerates at 2:00 mark (90s remaining)
    
    // Monster
    Level3.bEnableMonster = true;
    Level3.NumMonsters = 1;  // Single monster
    Level3.MonsterSpeed = 350.0f;
    
    // Hazards
    Level3.NumMuddyPatches = 5;
    Level3.NumTrapCells = 10;  // DOUBLED - more dandelion traps!
    Level3.bEnableMazeTraps = true;
    Level3.MazeTrapProbability = 0.1f;  // 10% chance
    
    // Star thresholds
    Level3.ThreeStarTime = 120.0f;
    Level3.TwoStarTime = 165.0f;
    Level3.OneStarTime = 210.0f;
    
    // Briefing
    Level3.BriefingMessages.Add(TEXT("=== LEVEL 3: DARK DESCENT ==="));
    Level3.BriefingMessages.Add(TEXT(""));
    Level3.BriefingMessages.Add(TEXT("⚠️ WARNING: MAZE REGENERATION TRAP ⚠️"));
    Level3.BriefingMessages.Add(TEXT(""));
    Level3.BriefingMessages.Add(TEXT("THREAT #3: MAZE REGENERATION"));
    Level3.BriefingMessages.Add(TEXT("  • At 2:00 remaining, PURPLE GLOWING CELL appears"));
    Level3.BriefingMessages.Add(TEXT("  • You MUST be standing in it when timer hits 2:00"));
    Level3.BriefingMessages.Add(TEXT("  • HOW TO AVOID: Find purple cell, stand in it at 2:00"));
    Level3.BriefingMessages.Add(TEXT("  • PENALTY: Trapped 5 seconds + maze regenerates"));
    Level3.BriefingMessages.Add(TEXT(""));
    Level3.BriefingMessages.Add(TEXT("ADDITIONAL THREATS:"));
    Level3.BriefingMessages.Add(TEXT("  • Monster spawns at 1:00 (faster spawn!)"));
    Level3.BriefingMessages.Add(TEXT("  • Monster speed increased"));
    Level3.BriefingMessages.Add(TEXT("  • 5 muddy patches (more traps!)"));
    Level3.BriefingMessages.Add(TEXT(""));
    Level3.BriefingMessages.Add(TEXT("Time Limit: 3 minutes 30 seconds"));
    
    Level3.NewHazards.Add(TEXT("Maze Regeneration at 2:00 (find purple safe zone!)"));
    Level3.NewHazards.Add(TEXT("Faster Monster (spawns at 1:00)"));
    
    LevelConfigs.Add(Level3);
}

void ALevelProgressionManager::CreateLevel4Config()
{
    FLevelConfig Level4;
    Level4.LevelNumber = 4;
    Level4.LevelName = TEXT("Nightmare Realm");
    Level4.LevelDescription = TEXT("Only the skilled survive this nightmare.");
    
    // Maze settings
    Level4.MazeRows = 25;
    Level4.MazeCols = 25;
    Level4.LoopProbability = 0.25f;
    
    // Timing
    Level4.TimeLimit = 180.0f;  // 3 minutes
    Level4.MonsterSpawnDelay = 30.0f;  // Monster spawns after 30 seconds!
    Level4.MazeRegenTime = 90.0f;  // Maze regenerates at 1:30 mark
    
    // Monster
    Level4.bEnableMonster = true;
    Level4.NumMonsters = 1;  // Single monster
    Level4.MonsterSpeed = 400.0f;
    
    // Hazards
    Level4.NumMuddyPatches = 8;
    Level4.NumTrapCells = 16;  // DOUBLED - more dandelion traps!
    Level4.bEnableMazeTraps = true;
    Level4.MazeTrapProbability = 0.2f;  // 20% chance
    
    // Star thresholds
    Level4.ThreeStarTime = 90.0f;
    Level4.TwoStarTime = 135.0f;
    Level4.OneStarTime = 180.0f;
    
    // Briefing
    Level4.BriefingMessages.Add(TEXT("Level 4: Nightmare Realm"));
    Level4.BriefingMessages.Add(TEXT(""));
    Level4.BriefingMessages.Add(TEXT("⚠️ EXTREME DIFFICULTY ⚠️"));
    Level4.BriefingMessages.Add(TEXT(""));
    Level4.BriefingMessages.Add(TEXT("CHALLENGES:"));
    Level4.BriefingMessages.Add(TEXT("• HUGE MAZE: 25x25 cells"));
    Level4.BriefingMessages.Add(TEXT("• AGGRESSIVE MONSTER: Spawns in 30 seconds!"));
    Level4.BriefingMessages.Add(TEXT("• EARLY MAZE REGEN: Purple cell at 1:30"));
    Level4.BriefingMessages.Add(TEXT("• MORE TRAPS: 8 muddy patches, 20% trap chance"));
    Level4.BriefingMessages.Add(TEXT("• TIGHT TIME: Only 3 minutes total"));
    Level4.BriefingMessages.Add(TEXT(""));
    Level4.BriefingMessages.Add(TEXT("STRATEGY:"));
    Level4.BriefingMessages.Add(TEXT("• Move FAST - monster spawns early"));
    Level4.BriefingMessages.Add(TEXT("• Watch for purple cell at 1:30 mark"));
    Level4.BriefingMessages.Add(TEXT("• Avoid all muddy patches - you can't afford slowdown"));
    Level4.BriefingMessages.Add(TEXT(""));
    Level4.BriefingMessages.Add(TEXT("Good luck. You'll need it."));
    
    Level4.NewHazards.Add(TEXT("Larger maze (25x25)"));
    Level4.NewHazards.Add(TEXT("Very aggressive monster (30s spawn)"));
    Level4.NewHazards.Add(TEXT("Earlier maze regeneration (1:30)"));
    
    LevelConfigs.Add(Level4);
}

void ALevelProgressionManager::CreateLevel5Config()
{
    FLevelConfig Level5;
    Level5.LevelNumber = 5;
    Level5.LevelName = TEXT("Death's Labyrinth");
    Level5.LevelDescription = TEXT("The ultimate test. Survive if you can.");
    
    // Maze settings - BLOOD MOON
    Level5.MazeRows = 20;  // Reduced from 30 for balance
    Level5.MazeCols = 20;
    Level5.LoopProbability = 0.3f;
    
    // Timing - BRUTAL
    Level5.TimeLimit = 150.0f;  // 2.5 minutes
    Level5.MonsterSpawnDelay = 0.0f;  // INSTANT MONSTER SPAWN!
    Level5.MazeRegenTime = -1.0f;  // Disabled (no maze regen)
    Level5.SecondMonsterSpawnTime = 90.0f;  // Second monster at 1:30 (60s elapsed)
    Level5.AggressiveModeTime = 120.0f;  // Aggressive mode at 1:00 (30s elapsed)
    
    // Monster - MAXIMUM THREAT
    Level5.bEnableMonster = true;
    Level5.NumMonsters = 2;  // TWO MONSTERS! Ultimate challenge!
    Level5.MonsterSpeed = 450.0f;
    
    // Hazards - BLOOD MOON
    Level5.NumMuddyPatches = 12;
    Level5.NumTrapCells = 24;  // DOUBLED - MAXIMUM DANDELIONS!
    Level5.bEnableMazeTraps = false;  // Disabled (old system)
    Level5.bEnableGoldenStar = false;  // NO GOLDEN STAR - Pure survival!
    
    // Star thresholds - VERY TIGHT
    Level5.ThreeStarTime = 75.0f;   // 1:15 for 3 stars
    Level5.TwoStarTime = 112.0f;    // 1:52 for 2 stars
    Level5.OneStarTime = 150.0f;    // 2:30 for 1 star
    
    // Briefing
    Level5.BriefingMessages.Add(TEXT("Level 5: Death's Labyrinth"));
    Level5.BriefingMessages.Add(TEXT(""));
    Level5.BriefingMessages.Add(TEXT("🔥 FINAL CHALLENGE 🔥"));
    Level5.BriefingMessages.Add(TEXT(""));
    Level5.BriefingMessages.Add(TEXT("This is it. The ultimate test of skill."));
    Level5.BriefingMessages.Add(TEXT(""));
    Level5.BriefingMessages.Add(TEXT("NIGHTMARE MODE:"));
    Level5.BriefingMessages.Add(TEXT("• 🔴 BLOOD MOON: 20x20 maze under crimson sky"));
    Level5.BriefingMessages.Add(TEXT("• 👹 DUAL MONSTERS: Second spawns at 1:30"));
    Level5.BriefingMessages.Add(TEXT("• ⚡ AGGRESSIVE MODE: Both enraged at 1:00"));
    Level5.BriefingMessages.Add(TEXT("• ⭐ NO GOLDEN STAR: Pure survival challenge"));
    Level5.BriefingMessages.Add(TEXT("• 🔥 6 TRAP CELLS: Avoid the glowing floors"));
    Level5.BriefingMessages.Add(TEXT("• 💀 12 MUDDY PATCHES: Slow you down"));
    Level5.BriefingMessages.Add(TEXT(""));
    Level5.BriefingMessages.Add(TEXT("Survive 2:30 to escape the Blood Moon."));
    Level5.BriefingMessages.Add(TEXT(""));
    Level5.BriefingMessages.Add(TEXT("Are you ready?"));
    
    Level5.NewHazards.Add(TEXT("BLOOD MOON ATMOSPHERE"));
    Level5.NewHazards.Add(TEXT("DUAL MONSTER SPAWNING"));
    Level5.NewHazards.Add(TEXT("AGGRESSIVE MODE AT 1:00"));
    Level5.NewHazards.Add(TEXT("NO GOLDEN STAR - PURE SURVIVAL"));
    
    LevelConfigs.Add(Level5);
}

FLevelConfig ALevelProgressionManager::GetLevelConfig(int32 LevelNumber) const
{
    if (LevelNumber < 1 || LevelNumber > LevelConfigs.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("[LevelProgressionManager] Invalid level number: %d"), LevelNumber);
        return FLevelConfig();
    }
    
    return LevelConfigs[LevelNumber - 1];
}

bool ALevelProgressionManager::IsLevelUnlocked(int32 LevelNumber) const
{
    return LevelNumber <= HighestUnlockedLevel;
}

void ALevelProgressionManager::UnlockLevel(int32 LevelNumber)
{
    if (LevelNumber > HighestUnlockedLevel && LevelNumber <= 5)
    {
        HighestUnlockedLevel = LevelNumber;
        SaveProgress();  // Save when unlocking
        UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] ✓ Level %d unlocked and saved!"), LevelNumber);
    }
}

int32 ALevelProgressionManager::CalculateStarRating(int32 LevelNumber, float CompletionTime) const
{
    FLevelConfig Config = GetLevelConfig(LevelNumber);
    
    if (CompletionTime <= Config.ThreeStarTime)
    {
        return 3;
    }
    else if (CompletionTime <= Config.TwoStarTime)
    {
        return 2;
    }
    else if (CompletionTime <= Config.OneStarTime)
    {
        return 1;
    }
    
    return 0;  // Failed (shouldn't happen if level completed)
}

void ALevelProgressionManager::RecordLevelCompletion(int32 LevelNumber, float CompletionTime, int32 Stars)
{
    if (!LevelStatistics.Contains(LevelNumber))
    {
        LevelStatistics.Add(LevelNumber, FLevelStats());
    }
    
    FLevelStats& Stats = LevelStatistics[LevelNumber];
    
    // Update best time
    if (Stats.BestTime == 0.0f || CompletionTime < Stats.BestTime)
    {
        Stats.BestTime = CompletionTime;
        UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] New best time for Level %d: %.2fs"), LevelNumber, CompletionTime);
    }
    
    // Update stars (keep highest)
    if (Stars > Stats.StarsEarned)
    {
        int32 OldStars = Stats.StarsEarned;
        Stats.StarsEarned = Stars;
        TotalStarsEarned += (Stars - OldStars);
        UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] Level %d stars: %d -> %d"), LevelNumber, OldStars, Stars);
    }
    
    Stats.TotalCompletions++;
    
    // Unlock next level
    if (LevelNumber < 5)
    {
        UnlockLevel(LevelNumber + 1);
    }
}

void ALevelProgressionManager::RecordLevelAttempt(int32 LevelNumber)
{
    if (!LevelStatistics.Contains(LevelNumber))
    {
        LevelStatistics.Add(LevelNumber, FLevelStats());
    }
    
    LevelStatistics[LevelNumber].TotalAttempts++;
}

void ALevelProgressionManager::RecordDeath(int32 LevelNumber, bool bByMonster)
{
    if (!LevelStatistics.Contains(LevelNumber))
    {
        LevelStatistics.Add(LevelNumber, FLevelStats());
    }
    
    if (bByMonster)
    {
        LevelStatistics[LevelNumber].DeathsByMonster++;
    }
    else
    {
        LevelStatistics[LevelNumber].DeathsByTimeout++;
    }
}

FLevelStats ALevelProgressionManager::GetLevelStats(int32 LevelNumber) const
{
    if (LevelStatistics.Contains(LevelNumber))
    {
        return LevelStatistics[LevelNumber];
    }
    
    return FLevelStats();
}

int32 ALevelProgressionManager::GetTotalStars() const
{
    return TotalStarsEarned;
}

void ALevelProgressionManager::ResetProgress()
{
    HighestUnlockedLevel = 1;
    CurrentLevel = 1;
    TotalStarsEarned = 0;
    LevelStatistics.Empty();
    
    SaveProgress();  // Save the reset
    
    UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] Progress reset - back to Level 1"));
}

void ALevelProgressionManager::SaveProgress()
{
    UMazeRunnerSaveGame* SaveGameInstance = Cast<UMazeRunnerSaveGame>(UGameplayStatics::CreateSaveGameObject(UMazeRunnerSaveGame::StaticClass()));
    
    if (SaveGameInstance)
    {
        // Save progress data
        SaveGameInstance->HighestUnlockedLevel = HighestUnlockedLevel;
        SaveGameInstance->TotalStarsEarned = TotalStarsEarned;
        
        // Save level stars
        for (auto& Pair : LevelStatistics)
        {
            SaveGameInstance->LevelStars.Add(Pair.Key, Pair.Value.StarsEarned);
            SaveGameInstance->LevelBestTimes.Add(Pair.Key, Pair.Value.BestTime);
        }
        
        // Save to slot
        if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveSlotName, 0))
        {
            UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] ✓ Progress saved (Highest Level: %d, Total Stars: %d)"), 
                   HighestUnlockedLevel, TotalStarsEarned);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[LevelProgressionManager] ✗ Failed to save progress!"));
        }
    }
}

void ALevelProgressionManager::LoadProgress()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        UMazeRunnerSaveGame* LoadedGame = Cast<UMazeRunnerSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
        
        if (LoadedGame)
        {
            HighestUnlockedLevel = LoadedGame->HighestUnlockedLevel;
            TotalStarsEarned = LoadedGame->TotalStarsEarned;
            
            // Load level stars
            for (auto& Pair : LoadedGame->LevelStars)
            {
                if (!LevelStatistics.Contains(Pair.Key))
                {
                    LevelStatistics.Add(Pair.Key, FLevelStats());
                }
                LevelStatistics[Pair.Key].StarsEarned = Pair.Value;
            }
            
            // Load best times
            for (auto& Pair : LoadedGame->LevelBestTimes)
            {
                if (!LevelStatistics.Contains(Pair.Key))
                {
                    LevelStatistics.Add(Pair.Key, FLevelStats());
                }
                LevelStatistics[Pair.Key].BestTime = Pair.Value;
            }
            
            UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] ✓ Progress loaded (Highest Level: %d, Total Stars: %d)"), 
                   HighestUnlockedLevel, TotalStarsEarned);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelProgressionManager] No save file found - starting fresh"));
    }
}

