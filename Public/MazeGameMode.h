// MazeGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MazeGameMode.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
    NotStarted  UMETA(DisplayName = "Not Started"),
    Playing     UMETA(DisplayName = "Playing"),
    Paused      UMETA(DisplayName = "Paused"),
    Won         UMETA(DisplayName = "Won"),
    Lost        UMETA(DisplayName = "Lost")
};

class UCreditsWidget;

UCLASS()
class MAZERUNNER_API AMazeGameMode : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    AMazeGameMode();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    
    // ==================== GAME STATE ====================
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    EGameState CurrentGameState;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
    float TotalGameTime;  // Total time limit before player loses
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    float RemainingTime;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
    float MonsterSpawnTime;  // Time delay before monster spawns and starts chasing
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    bool bMonsterSpawned;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    bool bMazeGenerated;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    bool bMonsterSpeedBoosted;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    bool bInMenuPreview;
    
    float OriginalMonsterSpeed;
    
    // Mouse-based camera rotation for menu preview
    FRotator TargetCameraRotation;
    FRotator CurrentCameraRotation;
    float CameraRotationSpeed;
    
    // ==================== REFERENCES ====================
    
    UPROPERTY()
    class AMazeManager* MazeManager;
    
    UPROPERTY()
    class AActor* Player;
    
    UPROPERTY()
    class USpotLightComponent* PlayerFlashlight;  // Reference to player's flashlight
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
    bool bEnableCameraControlledFlashlight;  // Toggle for camera-controlled flashlight (easy to disable)
    
    UPROPERTY()
    TArray<class AMonsterAI*> SpawnedMonsters;  // Track all monsters (Level 5 has 2)
    
    // Level 5 Blood Moon flags
    bool bSecondMonsterSpawned;
    bool bAggressiveModeActivated;
    
    UPROPERTY()
    class AGoldenStar* SpawnedStar;
    
    UPROPERTY()
    class UUserWidget* CurrentWidget;
    
    // ==================== SPAWN CLASSES ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Classes")
    TSubclassOf<class AMonsterAI> MonsterClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Classes")
    TSubclassOf<class AGoldenStar> GoldenStarClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Classes")
    TSubclassOf<class AMuddyPatch> MuddyPatchClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> GameOverWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> PauseMenuWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> WinScreenWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> LoseScreenWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> MainMenuWidgetClass;
    
    UPROPERTY()
    class UUserWidget* HUDWidget;
    
    UPROPERTY()
    class UUserWidget* PauseMenuWidget;
    
    UPROPERTY()
    class UUserWidget* MainMenuWidget;
    
    // ==================== SETTINGS ====================
    
    UPROPERTY()
    class UMazeGameSettings* GameSettings;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> SettingsMenuWidgetClass;
    
    UPROPERTY()
    class UUserWidget* SettingsMenuWidget;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> OptionsMenuWidgetClass;
    
    UPROPERTY()
    class UUserWidget* OptionsMenuWidget;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> LoadingScreenWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class ULevelSelectWidget> LevelSelectWidgetClass;
    
    UPROPERTY()
    class ULevelSelectWidget* LevelSelectWidget;
    
    UPROPERTY()
    class UUserWidget* LoadingScreenWidget;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UCreditsWidget> CreditsWidgetClass;
    
    UPROPERTY()
    class UCreditsWidget* CreditsWidget;
    
    // ==================== LEVEL PROGRESSION ====================
    
    UPROPERTY()
    class ALevelProgressionManager* LevelManager;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Classes")
    TSubclassOf<class ASafeZoneCell> SafeZoneCellClass;
    
    UPROPERTY()
    class ASafeZoneCell* SpawnedSafeZone;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    int32 CurrentLevel;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    int32 StarsEarnedThisLevel;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    float LevelStartTime;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    float LevelCompletionTime;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    bool bSafeZoneActive;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Progression")
    float SafeZoneActivationTime;  // When to spawn safe zone (based on config)
    
    // Current maze size settings
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings")
    int32 CurrentMazeRows;
    
    // Mouse sensitivity
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MouseSensitivity = 1.0f;
    
    // Game volume
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float GameVolume = 0.8f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings")
    int32 CurrentMazeCols;
    
    // ==================== PERFORMANCE MONITORING ====================
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
    float CurrentFPS;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
    float AverageFPS;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bShowPerformanceStats;
    
    float FPSSampleAccumulator;
    int32 FPSSampleCount;
    
    // ==================== MUDDY EFFECT ====================
    
    bool bMuddyEffectActive;
    float MuddyEffectTimer;
    float OriginalResolution;
    float OriginalPlayerSpeed;  // Store original player speed for restoration
    
    // ==================== TRAP CELLS ====================
    
    // Trap Cells (floor-based traps that capture player)
    UPROPERTY(EditDefaultsOnly, Category = "Hazards")
    TSubclassOf<class ATrapCell> TrapCellClass;
    
    UPROPERTY()
    TArray<class ATrapCell*> SpawnedTrapCells;
    
    // ==================== AUDIO ====================
    
    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    class USoundBase* WinSound;  // Voice effect when player escapes maze
    
    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    class USoundBase* CreditsMusic;  // Music for rolling credits
    
    // Track credits music to stop it later
    UPROPERTY()
    class UAudioComponent* CreditsMusicComponent;
    
    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    class USoundBase* LoseSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* MonsterSpeedBoostSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* MonsterTransformSound;  // Sound when monster grows at 30 seconds
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* MazeRegenerationSound;  // Sound for maze trap regeneration
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* TrapSound;  // Sound for maze trap activation
    
    // ==================== FLASHLIGHT ====================
    
    UFUNCTION(BlueprintCallable, Category = "Flashlight")
    void CreatePlayerFlashlight();
    
    // ==================== SPAWN SETTINGS ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
    bool bSpawnRandomly;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings", meta = (EditCondition = "!bSpawnRandomly"))
    int32 StartRow;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings", meta = (EditCondition = "!bSpawnRandomly"))
    int32 StartCol;
    
    UPROPERTY()
    FVector InitialPlayerLocation;
    
    // ==================== GAME FLOW FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void StartGame();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void StartLevel(int32 LevelNumber);
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void CompleteLevel();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void CalculateStarRating();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void ShowLevelBriefing();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void ShowLevelCompleteScreen();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void RetryCurrentLevel();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void LoadNextLevel();
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void LoadSelectedLevel(int32 LevelNumber);
    
    UFUNCTION(BlueprintCallable, Category = "Level Progression")
    void ShowLevelSelectScreen();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void GenerateMaze();  // Generate maze with current settings
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void ShowMainMenu();  // Show main menu
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowLoadingScreen(const FString& Message = TEXT("Loading..."), float MinDuration = 7.0f);
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideLoadingScreen();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void RestartGame();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void StartPlaying();
    
    // Actual spawn implementations
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SpawnPlayer();
    
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SpawnGoldenStar();
    
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SpawnMonster();
    
    // Level 5 Blood Moon functions
    void SpawnSecondMonster();
    void ActivateAggressiveMode();
    void SetBloodMoonAtmosphere();
    
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SpawnStars();
    
    void StartGameTimer();
    
    // ==================== GAME CONDITION CHECKS ====================
    
    void CheckWinCondition();
    void CheckLoseCondition();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void PlayerWon();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void PlayerLost(const FString& Reason);
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void PauseGame();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void ResumeGame();
    
    void ShowGameOverScreen(bool bWon);
    
    // ==================== UI HELPER ====================
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    FString GetTimeString() const;
    
    // ==================== SETTINGS FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowSettingsMenu();  // Show Free2Play mode menu
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CloseSettingsMenu();
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideSettingsMenu();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void QuitGame();
    
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void ReturnToMainMenu();  // Return to main menu with complete game cleanup
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    int32 GetMazeRows() const { return CurrentMazeRows; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    int32 GetMazeCols() const { return CurrentMazeCols; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMazeRows(int32 Rows);
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMazeCols(int32 Cols);
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SaveSettings(int32 NewRows, int32 NewCols);
    
    // ==================== OPTIONS MENU FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowOptionsMenu();  // Show options menu
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CloseOptionsMenu();
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    float GetMouseSensitivity() const { return MouseSensitivity; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMouseSensitivity(float Sensitivity) { MouseSensitivity = Sensitivity; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    float GetGameVolume() const { return GameVolume; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetGameVolume(float Volume);
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void LoadSettings();
    
    // ==================== MUDDY EFFECT ====================
    
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void ApplyMuddyEffect(float Duration);
    
    // ==================== TRAP CELL FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "Hazards")
    void SpawnTrapCells(int32 Count);  // Spawn trap cells in random maze cells
    
    // ==================== LEVEL PROGRESSION FUNCTIONS ====================
    
    void CleanupBeforeLevel();  // Complete cleanup between levels
    
    // ==================== SAFE ZONE FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "Safe Zone")
    void SpawnSafeZone(float ActivationTime);
    
    UFUNCTION(BlueprintCallable, Category = "Safe Zone")
    void CheckSafeZoneProtection();
    
    void CheckSafeZoneStatus();  // Check if player is in safe zone during maze regen
    
    UFUNCTION(BlueprintCallable, Category = "Safe Zone")
    bool IsPlayerInSafeZone() const;
    
    UFUNCTION(BlueprintCallable, Category = "Hazards")
    void SpawnMuddyPatches(int32 Count);  // Spawn muddy patch hazards

    void CleanupForCredits();
    void ShowCredits();
    
    // CHEAT CODE: Instant win
    UFUNCTION(Exec, Category = "Cheats")
    void Win();
};

