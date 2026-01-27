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
    class AMonsterAI* SpawnedMonster;
    
    UPROPERTY()
    class AGoldenStar* SpawnedStar;
    
    UPROPERTY()
    class UUserWidget* CurrentWidget;
    
    // ==================== SPAWN CLASSES ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Classes")
    TSubclassOf<class AMonsterAI> MonsterClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Classes")
    TSubclassOf<class AGoldenStar> GoldenStarClass;
    
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
    
    UPROPERTY()
    class UUserWidget* LoadingScreenWidget;
    
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
    
    // ==================== MAZE TRAP SYSTEM ====================
    
    bool bMazeTrapTriggered;      // Has the trap been triggered this game?
    bool bPlayerTrapped;          // Is player currently trapped?
    float TrapDuration;           // Remaining trap time
    class AMazeCell* TrappedCell; // Cell where player is trapped
    
    // ==================== AUDIO ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* WinSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* LoseSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* MonsterSpeedBoostSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* MonsterTransformSound;  // Sound when monster grows at 30 seconds
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* MazeRegenerationSound;  // Sound for maze trap regeneration
    
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
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    int32 GetMazeRows() const { return CurrentMazeRows; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    int32 GetMazeCols() const { return CurrentMazeCols; }
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMazeRows(int32 Rows);
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMazeCols(int32 Cols);
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SaveSettings();
    
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
    void ApplyMuddyEffect(float Duration, float ResolutionScale);
    
    // ==================== MAZE TRAP FUNCTIONS ====================
    
    UFUNCTION(BlueprintCallable, Category = "Maze Trap")
    void TriggerMazeTrap();
    
    class AMazeCell* GetPlayerCurrentCell();
    void TrapPlayer(AMazeCell* Cell);
    void ReleaseTrap();
};
