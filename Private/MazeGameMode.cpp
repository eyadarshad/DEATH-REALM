// MazeGameMode.cpp - COMPLETE WITH LEVEL PROGRESSION SYSTEM
#include "MazeGameMode.h"
#include "MazeManager.h"
#include "MazeCell.h"
#include "MonsterAI.h"
#include "GoldenStar.h"
#include "MazePlayerController.h"
#include "MazeGameSettings.h"
#include "LoadingScreenWidget.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Components/TextBlock.h"
#include "Components/SpotLightComponent.h"
#include "AudioDevice.h"
#include "LevelProgressionManager.h"
#include "LevelSelectWidget.h"
#include "SafeZoneCell.h"
#include "CreditsWidget.h"
#include "MuddyPatch.h"
#include "TrapCell.h"
#include "Engine/DirectionalLight.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

AMazeGameMode::AMazeGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    PlayerControllerClass = AMazePlayerController::StaticClass();
    
    // Prevent automatic player spawning - we'll spawn manually after main menu
    bStartPlayersAsSpectators = true;
    
    CurrentGameState = EGameState::NotStarted;
    // FIXED: 30 second time limit for the entire game
    RemainingTime = TotalGameTime = 30.0f;
    // FIXED: Monster spawns after 5 seconds to give player time to explore
    MonsterSpawnTime = 5.0f;
    bMonsterSpawned = false;
    bMazeGenerated = false;
    bMonsterSpeedBoosted = false;
    bInMenuPreview = false;
    OriginalMonsterSpeed = 0.0f;
    
    bSpawnRandomly = true;
    StartRow = 0;
    StartCol = 0;
    
    // Initialize settings
    CurrentMazeRows = 15;
    CurrentMazeCols = 15;
    
    // Performance monitoring
    CurrentFPS = 0.0f;
    AverageFPS = 0.0f;
    FPSSampleAccumulator = 0.0f;
    FPSSampleCount = 0;
    
    // Muddy effect initialization
    bMuddyEffectActive = false;
    MuddyEffectTimer = 0.0f;
    OriginalResolution = 100.0f;  // 100% resolution
    OriginalPlayerSpeed = 600.0f;  // Default Unreal character speed
    bShowPerformanceStats = false;
    
    // Flashlight control
    PlayerFlashlight = nullptr;
    bEnableCameraControlledFlashlight = false;  // Disabled - using fixed flashlight position
    
    // Camera rotation for menu preview
    CurrentCameraRotation = FRotator::ZeroRotator;
    TargetCameraRotation = FRotator::ZeroRotator;
    CameraRotationSpeed = 2.0f;  // Smooth interpolation speed

    // Level progression initialization
    CurrentLevel = 1;
    StarsEarnedThisLevel = 0;
    LevelCompletionTime = 0.0f;
    LevelStartTime = 0.0f;
    bSafeZoneActive = false;
    SafeZoneActivationTime = -1.0f;
}

void AMazeGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Show loading screen immediately
    ShowLoadingScreen("Loading DEATH REALM...", 3.0f);  // Minimum 3 seconds
    
    // CRITICAL FIX: Reset resolution and speed at BeginPlay (handles level reload from RestartGame)
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
        UE_LOG(LogTemp, Warning, TEXT("[BeginPlay] ✓ Resolution reset to 100%%"));
    }
    bMuddyEffectActive = false;
    MuddyEffectTimer = 0.0f;
    
    // Step 1: Get player reference
    Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!Player)
    {
        UE_LOG(LogTemp, Error, TEXT("✗ NO PLAYER CHARACTER! Check Default Pawn Class in GameMode Blueprint."));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("✓ Player character found"));
    
    // Reset player speed to default
    ACharacter* PlayerChar = Cast<ACharacter>(Player);
    if (PlayerChar && PlayerChar->GetCharacterMovement())
    {
        PlayerChar->GetCharacterMovement()->MaxWalkSpeed = 600.0f;
        UE_LOG(LogTemp, Warning, TEXT("[BeginPlay] ✓ Player speed reset to 600"));
    }
    
    // Step 2: Find or create MazeManager
    TArray<AActor*> FoundManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMazeManager::StaticClass(), FoundManagers);
    
    if (FoundManagers.Num() > 0)
    {
        MazeManager = Cast<AMazeManager>(FoundManagers[0]);
        UE_LOG(LogTemp, Warning, TEXT("Found existing MazeManager"));
    }
    else
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MazeManager = GetWorld()->SpawnActor<AMazeManager>(
            AMazeManager::StaticClass(), 
            FVector::ZeroVector, 
            FRotator::ZeroRotator, 
            SpawnParams
        );
        UE_LOG(LogTemp, Warning, TEXT("Created new MazeManager"));
    }
    
    if (!MazeManager)
    {
        UE_LOG(LogTemp, Error, TEXT("✗ FAILED to get MazeManager!"));
        return;
    }
    
    // Step 3: Verify MazeCellClass is set
    if (!MazeManager->MazeCellClass)
    {
        UE_LOG(LogTemp, Error, TEXT("✗ CRITICAL: MazeCellClass not set in MazeManager Blueprint!"));
        UE_LOG(LogTemp, Error, TEXT("  1. Select BP_MazeManager in World Outliner"));
        UE_LOG(LogTemp, Error, TEXT("  2. In Details panel, set 'Maze Cell Class' to BP_MazeCell"));
        return;
    }
    
    // Step 3.5: Load settings
    LoadSettings();
    UE_LOG(LogTemp, Warning, TEXT("Settings loaded - Maze size: %dx%d"), CurrentMazeRows, CurrentMazeCols);
    
    // Step 3.6: Initialize Level Progression Manager
    TArray<AActor*> FoundLevelManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALevelProgressionManager::StaticClass(), FoundLevelManagers);
    if (FoundLevelManagers.Num() > 0)
    {
        LevelManager = Cast<ALevelProgressionManager>(FoundLevelManagers[0]);
    }
    else
    {
        FActorSpawnParameters LevelSpawnParams;
        LevelSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        LevelManager = GetWorld()->SpawnActor<ALevelProgressionManager>(
            ALevelProgressionManager::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            LevelSpawnParams
        );
    }
    if (LevelManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelProgression] Level Manager initialized"));
    }
    
    // Step 4: Generate maze BEFORE showing main menu for preview
    UE_LOG(LogTemp, Warning, TEXT("[MenuPreview] Generating maze for preview..."));
    MazeManager->SetMazeSize(CurrentMazeRows, CurrentMazeCols);
    MazeManager->GenerateMazeImmediate();
    bMazeGenerated = true;
    
    if (!MazeManager->GetEscapeCell())
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Maze generation FAILED during preview!"));
        return;
    }
    
    // Step 5: Spawn player and golden star for preview (NO MONSTER YET)
    FTimerHandle PreviewSpawnTimer;
    GetWorldTimerManager().SetTimer(PreviewSpawnTimer, [this]()
    {
        SpawnPlayer();
        SpawnGoldenStar();
        CreatePlayerFlashlight();  // FIX #1: Add flashlight to preview
        bInMenuPreview = true;
        UE_LOG(LogTemp, Warning, TEXT("[MenuPreview] Player, star, and flashlight spawned for preview"));
    }, 0.2f, false);
    
    // Step 6: Show main menu AFTER maze is ready
    if (MainMenuWidgetClass)
    {
        FTimerHandle MenuTimer;
        GetWorldTimerManager().SetTimer(MenuTimer, [this]()
        {
            MainMenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
            if (MainMenuWidget)
            {
                MainMenuWidget->AddToViewport(100);
                
                APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
                if (PC)
                {
                    PC->bShowMouseCursor = true;
                    PC->SetInputMode(FInputModeGameAndUI());
                }
                
                UE_LOG(LogTemp, Warning, TEXT("Main Menu displayed with interactive preview!"));
            }
        }, 0.5f, false);
    }
    else
    {
        // No main menu, start game immediately
        UE_LOG(LogTemp, Warning, TEXT("⚠ No Main Menu set - starting game immediately"));
        FTimerHandle StartTimer;
        GetWorldTimerManager().SetTimer(StartTimer, this, &AMazeGameMode::StartGame, 0.1f, false);
    }
}

void AMazeGameMode::StartGame()
{
    if (!MazeManager || !Player)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start: Missing MazeManager or Player"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] STARTING NEW GAME"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    
    // CRITICAL FIX: Reset resolution to 100% at start of every game
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Resolution reset to 100%%"));
    }
    bMuddyEffectActive = false;
    MuddyEffectTimer = 0.0f;
    
    // CRITICAL FIX: Reset player speed to default at start of every game
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            PlayerChar->GetCharacterMovement()->MaxWalkSpeed = 600.0f;
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player speed reset to 600"));
        }
    }
    
    // Check if maze size changed (settings were saved)
    bool bSizeChanged = false;
    if (MazeManager)
    {
        int32 CurrentRows = MazeManager->Rows;
        int32 CurrentCols = MazeManager->Cols;
        
        if (CurrentRows != CurrentMazeRows || CurrentCols != CurrentMazeCols)
        {
            bSizeChanged = true;
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze size changed: %dx%d -> %dx%d"), 
                   CurrentRows, CurrentCols, CurrentMazeRows, CurrentMazeCols);
        }
    }
    
    if (!bMazeGenerated || bSizeChanged)
    {
        MazeManager->SetMazeSize(CurrentMazeRows, CurrentMazeCols);
        MazeManager->GenerateMazeImmediate();
        bMazeGenerated = true;
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze generated immediately"));
        
        // CRITICAL FIX: Destroy old star if it exists (prevents instant win)
        if (SpawnedStar)
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Destroying old star before respawn"));
            SpawnedStar->Destroy();
            SpawnedStar = nullptr;
        }
        
        // Spawn stars in the sky
        SpawnStars();
        
        if (!MazeManager->GetEscapeCell())
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Maze generation FAILED - no escape cell!"));
            return;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Escape cell created at: Row=%d, Col=%d"), 
               MazeManager->GetEscapeCell()->Row, MazeManager->GetEscapeCell()->Col);
    }
    
    // Step 1: Initialize game state
    CurrentGameState = EGameState::Playing;
    RemainingTime = TotalGameTime;
    bMonsterSpawned = false;
    bMonsterSpeedBoosted = false;
    bInMenuPreview = false;  // Exit preview mode
    OriginalMonsterSpeed = 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Transitioning from preview to gameplay"));
    
    // Step 2: Create and show HUD
    if (HUDWidgetClass)
    {
        HUDWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
            UE_LOG(LogTemp, Warning, TEXT("HUD Widget created and displayed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠ HUDWidgetClass not set - no HUD will be shown"));
    }
    
    // Step 3: Player and star handling
    if (!Player || bSizeChanged)
    {
        if (bSizeChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze regenerated, respawning player at new start location..."));
            
            // CRITICAL: Destroy old flashlight before respawning
            if (PlayerFlashlight)
            {
                PlayerFlashlight->DestroyComponent();
                PlayerFlashlight = nullptr;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player missing, spawning..."));
        }
        
        SpawnPlayer();
    }
    
    // CRITICAL FIX: Always respawn star if maze was regenerated (escape cell changed)
    if (!SpawnedStar || bSizeChanged)
    {
        if (bSizeChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze regenerated, respawning star at new location..."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Star missing, spawning..."));
        }
        
        // Destroy old star if it exists
        if (SpawnedStar)
        {
            SpawnedStar->Destroy();
            SpawnedStar = nullptr;
        }
        
        SpawnGoldenStar();
    }
    
    // Step 4: Monster spawn timer (starts NOW when game begins) - only if enabled
    if (MonsterSpawnTime >= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster will spawn in %.0f seconds"), MonsterSpawnTime);
        FTimerHandle MonsterTimer;
        GetWorldTimerManager().SetTimer(MonsterTimer, this, &AMazeGameMode::SpawnMonster, MonsterSpawnTime, false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster disabled for this level"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Game Started!"));
    UE_LOG(LogTemp, Warning, TEXT("  - Time limit: %.0f seconds"), TotalGameTime);
    UE_LOG(LogTemp, Warning, TEXT("  - Monster spawns: %.0f seconds"), MonsterSpawnTime);
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void AMazeGameMode::GenerateMaze()
{
    if (!MazeManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[Free2Play] MazeManager not found!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Generating maze with size: %dx%d"), CurrentMazeRows, CurrentMazeCols);
    
    // Set the maze size
    MazeManager->SetMazeSize(CurrentMazeRows, CurrentMazeCols);
    
    // Generate the maze
    MazeManager->GenerateMaze();
    
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Maze generated successfully"));
}

void AMazeGameMode::StartPlaying()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Starting game..."));
    
    // Hide main menu
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
    }
    
    // Hide settings menu if visible
    if (SettingsMenuWidget)
    {
        SettingsMenuWidget->RemoveFromParent();
        SettingsMenuWidget = nullptr;
    }
    
    // Set game state
    CurrentGameState = EGameState::Playing;
    UGameplayStatics::SetGamePaused(GetWorld(), false);
    
    // Hide pause menu
    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent();
    }
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
        
        // Reset camera rotation to neutral position
        CurrentCameraRotation = FRotator::ZeroRotator;
        TargetCameraRotation = FRotator::ZeroRotator;
        PC->SetControlRotation(FRotator::ZeroRotator);
    }
    
    // FIX #3: Create flashlight for Free2Play mode
    CreatePlayerFlashlight();
    UE_LOG(LogTemp, Warning, TEXT("[StartPlaying] Flashlight created for Free2Play mode"));
    
    // Start the game
    StartGame();
}

void AMazeGameMode::ShowMainMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Showing main menu"));
    
    // Stop only credits music (not all sounds)
    if (CreditsMusicComponent && CreditsMusicComponent->IsPlaying())
    {
        CreditsMusicComponent->Stop();
        CreditsMusicComponent = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[ShowMainMenu] Credits music stopped"));
    }
    
    // Hide any existing UI
    if (HUDWidget)
    {
        HUDWidget->RemoveFromParent();
        HUDWidget = nullptr;
    }
    
    // Show main menu
    if (MainMenuWidgetClass)
    {
        if (!MainMenuWidget)
        {
            MainMenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
        }
        
        if (MainMenuWidget)
        {
            MainMenuWidget->AddToViewport(100);
            
            // Set input mode to UI only
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
            
            UE_LOG(LogTemp, Warning, TEXT("Main Menu displayed!"));
        }
    }
}

void AMazeGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Performance monitoring
    if (DeltaTime > 0.0f)
    {
        CurrentFPS = 1.0f / DeltaTime;
        FPSSampleAccumulator += CurrentFPS;
        FPSSampleCount++;
        
        // Calculate average every 30 frames
        if (FPSSampleCount >= 30)
        {
            AverageFPS = FPSSampleAccumulator / FPSSampleCount;
            FPSSampleAccumulator = 0.0f;
            FPSSampleCount = 0;
        }
        
        // Display performance stats if enabled
        if (bShowPerformanceStats && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Yellow, 
                FString::Printf(TEXT("FPS: %.1f | Avg: %.1f"), CurrentFPS, AverageFPS));
        }
    }
    
    // MENU PREVIEW MODE: Mouse-based camera rotation
    if (bInMenuPreview && Player)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            // Get mouse position in viewport
            float MouseX, MouseY;
            if (PC->GetMousePosition(MouseX, MouseY))
            {
                // Get viewport size
                int32 ViewportSizeX, ViewportSizeY;
                PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
                
                if (ViewportSizeX > 0 && ViewportSizeY > 0)
                {
                    // Normalize mouse position to -1 to 1 range
                    float NormalizedX = (MouseX / ViewportSizeX) - 0.5f;
                    float NormalizedY = (MouseY / ViewportSizeY) - 0.5f;
                    
                    // Calculate target rotation (limited range for subtle effect)
                    float MaxYaw = 30.0f;   // Max 30 degrees left/right
                    float MaxPitch = 20.0f; // Max 20 degrees up/down
                    
                    TargetCameraRotation.Yaw = NormalizedX * MaxYaw * 2.0f;
                    TargetCameraRotation.Pitch = -NormalizedY * MaxPitch * 2.0f;  // Inverted for natural feel
                    TargetCameraRotation.Roll = 0.0f;
                    
                    // Smoothly interpolate current rotation to target
                    CurrentCameraRotation = FMath::RInterpTo(
                        CurrentCameraRotation, 
                        TargetCameraRotation, 
                        DeltaTime, 
                        CameraRotationSpeed
                    );
                    
                    // Apply rotation to player's control rotation
                    PC->SetControlRotation(CurrentCameraRotation);
                }
            }
        }
    }
    
    if (CurrentGameState == EGameState::Playing)
    {
        // Update timer
        RemainingTime -= DeltaTime;
        
        // Update HUD timer display
        if (HUDWidget)
        {
            UTextBlock* TimerText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Text")));
            if (TimerText)
            {
                TimerText->SetText(FText::FromString(GetTimeString()));
            }
        }
        
        // Check conditions
        CheckWinCondition();
        CheckLoseCondition();
        
        // FEATURE: Increase monster speed and size in last 30 seconds
        if (RemainingTime <= 30.0f && !bMonsterSpeedBoosted && SpawnedMonsters.Num() > 0)
        {
            bMonsterSpeedBoosted = true;
            
            // Boost first monster
            AMonsterAI* FirstMonster = SpawnedMonsters[0];
            if (FirstMonster)
            {
                // Store original speed if not already stored
                if (OriginalMonsterSpeed == 0.0f)
                {
                    OriginalMonsterSpeed = FirstMonster->MoveSpeed;
                }
                
                // Increase speed by 50%
                float NewSpeed = OriginalMonsterSpeed * 1.5f;
                FirstMonster->MoveSpeed = NewSpeed;
                
                // Update character movement component
                if (FirstMonster->GetCharacterMovement())
                {
                    FirstMonster->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
                }
                
                // GROW MONSTER TO 1.7x SIZE - Make it terrifying!
                FVector NewScale = FVector(1.7f, 1.7f, 1.7f);
                FirstMonster->SetActorScale3D(NewScale);
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] 👹 MONSTER GREW TO 1.7x SIZE! 👹"));
                
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster speed boosted from %.0f to %.0f"), 
                       OriginalMonsterSpeed, NewSpeed);
            }
            
            // Show warning
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange,
                    TEXT("⚠️ MONSTER IS NOW FASTER AND BIGGER!"));
            }
            
            // Play transformation sound if set
            if (MonsterTransformSound)
            {
                UGameplayStatics::PlaySound2D(GetWorld(), MonsterTransformSound, 1.0f);
            }
            else if (MonsterSpeedBoostSound)
            {
                UGameplayStatics::PlaySound2D(GetWorld(), MonsterSpeedBoostSound, 1.0f);
            }
        }
        
        // Time's up
        if (RemainingTime <= 0.0f)
        {
            RemainingTime = 0.0f;
            PlayerLost("Time's up!");
        }
        
        // CRITICAL: Keep player above floor
        if (Player)
        {
            FVector Loc = Player->GetActorLocation();
            if (Loc.Z < -100.0f) // Falling through floor
            {
                Loc.Z = 150.0f; // Reset to safe height
                Player->SetActorLocation(Loc);
                
                // Stop falling velocity
                ACharacter* PlayerChar = Cast<ACharacter>(Player);
                if (PlayerChar && PlayerChar->GetCharacterMovement())
                {
                    PlayerChar->GetCharacterMovement()->Velocity = FVector::ZeroVector;
                }
                
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] Caught player falling! Reset to safe height"));
            }
        }
        
        // FEATURE: Camera-controlled flashlight (can be easily disabled)
        if (bEnableCameraControlledFlashlight && PlayerFlashlight && Player)
        {
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                // Get camera rotation (where player is looking)
                FRotator CameraRotation = PC->GetControlRotation();
                
                // Apply camera rotation to flashlight (points where player looks)
                PlayerFlashlight->SetWorldRotation(CameraRotation);
            }
        }
        
        // MUDDY EFFECT: Handle resolution reduction timer
        if (bMuddyEffectActive)
        {
            MuddyEffectTimer -= DeltaTime;
            
            if (MuddyEffectTimer <= 0.0f)
            {
                // Effect expired - restore resolution and speed
                bMuddyEffectActive = false;
                
                if (GEngine)
                {
                    GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.ScreenPercentage %f"), OriginalResolution));
                    UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] Effect expired! Resolution restored to %.0f%%"), OriginalResolution);
                    
                    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                        TEXT("Vision and speed restored!"));
                }
                
                // Restore player speed
                if (Player)
                {
                    ACharacter* PlayerChar = Cast<ACharacter>(Player);
                    if (PlayerChar && PlayerChar->GetCharacterMovement())
                    {
                        PlayerChar->GetCharacterMovement()->MaxWalkSpeed = OriginalPlayerSpeed;
                        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] Player speed restored to %.0f"), OriginalPlayerSpeed);
                    }
                }
            }
        }
        
        // Level 5 Blood Moon mechanics
        if (CurrentLevel == 5 && CurrentGameState == EGameState::Playing && LevelManager)
        {
            FLevelConfig Config = LevelManager->GetLevelConfig(5);
            
            // Second monster spawn at 1:30
            if (!bSecondMonsterSpawned && Config.SecondMonsterSpawnTime > 0.0f)
            {
                if (RemainingTime <= Config.SecondMonsterSpawnTime)
                {
                    SpawnSecondMonster();
                    bSecondMonsterSpawned = true;
                }
            }
            
            // Aggressive mode at 1:00
            if (!bAggressiveModeActivated && Config.AggressiveModeTime > 0.0f)
            {
                if (RemainingTime <= Config.AggressiveModeTime)
                {
                    ActivateAggressiveMode();
                    bAggressiveModeActivated = true;
                }
            }
        }
    }
}

void AMazeGameMode::SpawnPlayer()
{
    if (!MazeManager || !Player) 
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Cannot spawn player: invalid state"));
        return;
    }
    
    AMazeCell* SpawnCell = nullptr;
    AMazeCell* EscapeCell = MazeManager->GetEscapeCell();
    int32 Attempts = 0;
    int32 MaxAttempts = 100;
    float MinDistanceFromEscape = MazeManager->CellSize * 7.0f; // At least 7 cells away
    
    if (bSpawnRandomly)
    {
        // Find random non-exit cell that is far from escape
        do {
            SpawnCell = MazeManager->GetRandomCell();
            Attempts++;
            
            if (SpawnCell && !SpawnCell->bIsEscapeCell && EscapeCell)
            {
                float Distance = FVector::Dist(SpawnCell->GetActorLocation(), EscapeCell->GetActorLocation());
                if (Distance >= MinDistanceFromEscape)
                {
                    break; // Good spawn location - far enough from escape
                }
            }
        } while (Attempts < MaxAttempts);
        
        // Log the distance for verification
        if (SpawnCell && EscapeCell)
        {
            float FinalDistance = FVector::Dist(SpawnCell->GetActorLocation(), EscapeCell->GetActorLocation());
            float CellsAway = FinalDistance / MazeManager->CellSize;
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player spawn is %.1f cells away from escape"), CellsAway);
        }
    }
    else
    {
        SpawnCell = MazeManager->GetCell(StartRow, StartCol);
    }
    
    if (!SpawnCell)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] FAILED to find spawn cell! Using fallback."));
        SpawnCell = MazeManager->GetCell(0, 0);
    }
    
    if (SpawnCell)
    {
        FVector CellLocation = SpawnCell->GetActorLocation();
        
        // FIXED: Spawn at exact center - no random offset
        FVector SpawnLocation = CellLocation;
        SpawnLocation.Z = 100.0f; // 1 meter above ground
        
        Player->SetActorLocation(SpawnLocation);
        Player->SetActorRotation(FRotator::ZeroRotator);
        
        InitialPlayerLocation = SpawnLocation;
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player spawned at cell [%d,%d]"), 
               SpawnCell->Row, SpawnCell->Col);
    }
}

void AMazeGameMode::SpawnGoldenStar()
{
    if (!MazeManager || !GoldenStarClass) 
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Cannot spawn star: missing MazeManager or GoldenStarClass"));
        return;
    }
    
    AMazeCell* StarCell = nullptr;
    int32 Attempts = 0;
    float MinDistance = MazeManager->CellSize * 3.0f; // At least 3 cells away
    
    do {
        StarCell = MazeManager->GetRandomCell();
        Attempts++;
        
        if (StarCell && !StarCell->bIsEscapeCell)
        {
            float Distance = FVector::Dist(StarCell->GetActorLocation(), InitialPlayerLocation);
            if (Distance >= MinDistance)
            {
                break; // Good spawn location
            }
        }
    } while (Attempts < 100);
    
    if (StarCell)
    {
        FVector StarLocation = StarCell->GetActorLocation();
        StarLocation.Z = 300.0f; // Floating at 3 meters height
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        SpawnedStar = GetWorld()->SpawnActor<AGoldenStar>(
            GoldenStarClass, 
            StarLocation, 
            FRotator::ZeroRotator, 
            SpawnParams
        );
        
        if (SpawnedStar)
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Golden Star spawned at [%d,%d]"), 
                   StarCell->Row, StarCell->Col);
        }
    }
}

void AMazeGameMode::SpawnMonster()
{
	if (!MazeManager || !MonsterClass || !Player || bMonsterSpawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Cannot spawn monster: invalid state"));
		return;
	}
	
	// FIXED: Spawn monster far from player (at least 5 cells away)
	AMazeCell* MonsterCell = nullptr;
	int32 Attempts = 0;
	float MinDistance = MazeManager->CellSize * 5.0f; // At least 5 cells away
	
	do {
		MonsterCell = MazeManager->GetRandomCell();
		Attempts++;
		
		if (MonsterCell && !MonsterCell->bIsEscapeCell)
		{
			float Distance = FVector::Dist(MonsterCell->GetActorLocation(), InitialPlayerLocation);
			if (Distance >= MinDistance)
			{
				break; // Good spawn location
			}
		}
	} while (Attempts < 100);
	
	if (!MonsterCell)
	{
		// Fallback: spawn at opposite corner
		MonsterCell = MazeManager->GetCell(MazeManager->Rows - 1, MazeManager->Cols - 1);
	}
	
	// CRITICAL FIX: Add random offset to avoid spawning in walls
	float SafeOffset = 200.0f;
	float RandomX = FMath::FRandRange(-SafeOffset, SafeOffset);
	float RandomY = FMath::FRandRange(-SafeOffset, SafeOffset);
	
	FVector MonsterLocation = MonsterCell->GetActorLocation();
	MonsterLocation.X += RandomX;
	MonsterLocation.Y += RandomY;
	MonsterLocation.Z = 100.0f; // Same height as player
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AMonsterAI* Monster = GetWorld()->SpawnActor<AMonsterAI>(
		MonsterClass, 
		MonsterLocation, 
		FRotator::ZeroRotator, 
		SpawnParams
	);
	
	if (Monster)
    {
        Monster->StartChasing(Player);
        
        // Play growl sound on initial spawn only
        if (Monster->GrowlSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), Monster->GrowlSound, 0.9f);
        }
        
        // Show monster spawn warning
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
                TEXT("⚠️ MONSTER SPAWNED! RUN!"));
        }
        
        // Add to monsters array
        SpawnedMonsters.Add(Monster);
        bMonsterSpawned = true;
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster spawned (Total: %d)"), SpawnedMonsters.Num());
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] MONSTER SPAWNED - RUN!"));
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster at [%d,%d], Player at initial location"), 
               MonsterCell->Row, MonsterCell->Col);
    }
}

void AMazeGameMode::SpawnStars()
{
	if (!MazeManager)
	{
		return;
	}
	
	// Calculate maze center
	float MazeCenterX = (MazeManager->Rows * MazeManager->CellSize) / 2.0f;
	float MazeCenterY = (MazeManager->Cols * MazeManager->CellSize) / 2.0f;
	
	// Spawn stars at maze center
	FVector StarsLocation = FVector(MazeCenterX, MazeCenterY, 0.0f);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	// Spawn stars actor
	UClass* StarsClass = LoadClass<AActor>(nullptr, TEXT("/Script/MazeRunner.Stars"));
	if (StarsClass)
	{
		AActor* Stars = GetWorld()->SpawnActor<AActor>(StarsClass, StarsLocation, FRotator::ZeroRotator, SpawnParams);
		if (Stars)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] Stars spawned in the sky"));
		}
	}
}
// MazeGameMode.cpp - PART 2 (Continue from Part 1)

void AMazeGameMode::CheckWinCondition()
{
    if (!Player || !MazeManager || CurrentGameState != EGameState::Playing) return;
    
    FVector PlayerLoc = Player->GetActorLocation();
    float CellSize = MazeManager->CellSize;
    AMazeCell* EscapeCell = MazeManager->GetEscapeCell();
    
    if (!EscapeCell) return;
    
    FVector EscapeCellLoc = EscapeCell->GetActorLocation();
    
    // ONLY win if player has moved BEYOND the maze boundary through the escape opening
    bool bEscaped = false;
    
    // Check if player has exited through the escape opening
    if (EscapeCell->Row == 0 && PlayerLoc.X < EscapeCellLoc.X - CellSize * 0.6f)
    {
        bEscaped = true;  // North exit - player went far beyond
        UE_LOG(LogTemp, Warning, TEXT("[Win] Player escaped through NORTH exit!"));
    }
    else if (EscapeCell->Row == MazeManager->Rows - 1 && PlayerLoc.X > EscapeCellLoc.X + CellSize * 0.6f)
    {
        bEscaped = true;  // South exit
        UE_LOG(LogTemp, Warning, TEXT("[Win] Player escaped through SOUTH exit!"));
    }
    else if (EscapeCell->Col == 0 && PlayerLoc.Y < EscapeCellLoc.Y - CellSize * 0.6f)
    {
        bEscaped = true;  // West exit
        UE_LOG(LogTemp, Warning, TEXT("[Win] Player escaped through WEST exit!"));
    }
    else if (EscapeCell->Col == MazeManager->Cols - 1 && PlayerLoc.Y > EscapeCellLoc.Y + CellSize * 0.6f)
    {
        bEscaped = true;  // East exit
        UE_LOG(LogTemp, Warning, TEXT("[Win] Player escaped through EAST exit!"));
    }
    
    if (bEscaped)
    {
        PlayerWon();
    }
}

void AMazeGameMode::CheckLoseCondition()
{
    if (SpawnedMonsters.Num() == 0 || !Player || CurrentGameState != EGameState::Playing) return;
    
    // Check if any monster caught the player
    for (AMonsterAI* Monster : SpawnedMonsters)
    {
        if (Monster && Monster->HasCaughtPlayer())
        {
            PlayerLost("Monster caught you!");
            return;
        }
    }
}

void AMazeGameMode::PlayerWon()
{
    if (CurrentGameState == EGameState::Playing)
    {
        CompleteLevel();  // Use level progression system
    }
}

void AMazeGameMode::PlayerLost(const FString& Reason)
{
    if (CurrentGameState == EGameState::Playing)
    {
        CurrentGameState = EGameState::Lost;
        
        // Record death in level progression
        if (LevelManager)
        {
            bool bByMonster = Reason.Contains("Monster") || Reason.Contains("caught");
            LevelManager->RecordDeath(CurrentLevel, bByMonster);
        }
        
        // Play lose sound
        if (LoseSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), LoseSound, 0.8f);
        }
        
        // Hide HUD
        if (HUDWidget)
        {
            HUDWidget->RemoveFromParent();
        }
        
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT("=== ✗✗✗ DEFEAT: %s ✗✗✗ ==="), *Reason);
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        
        ShowGameOverScreen(false);
    }
}

void AMazeGameMode::ShowGameOverScreen(bool bWon)
{
    // Choose the correct widget class based on win/lose
    TSubclassOf<UUserWidget> WidgetClassToUse = bWon ? WinScreenWidgetClass : LoseScreenWidgetClass;
    
    if (WidgetClassToUse)
    {
        if (CurrentWidget)
        {
            CurrentWidget->RemoveFromParent();
        }
        
        CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClassToUse);
        if (CurrentWidget)
        {
            CurrentWidget->AddToViewport();
            
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
                UGameplayStatics::SetGamePaused(GetWorld(), true);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠ %s Widget Class not set!"), bWon ? TEXT("Win Screen") : TEXT("Lose Screen"));
    }
}

FString AMazeGameMode::GetTimeString() const
{
    int32 Minutes = FMath::FloorToInt(RemainingTime / 60.0f);
    int32 Seconds = FMath::FloorToInt(FMath::Fmod(RemainingTime, 60.0f));
    return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

void AMazeGameMode::PauseGame()
{
    if (CurrentGameState != EGameState::Playing)
    {
        return; // Can only pause when playing
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Game Paused"));
    
    CurrentGameState = EGameState::Paused;
    UGameplayStatics::SetGamePaused(GetWorld(), true);
    
    // Show pause menu
    if (PauseMenuWidgetClass)
    {
        if (!PauseMenuWidget)
        {
            PauseMenuWidget = CreateWidget<UUserWidget>(GetWorld(), PauseMenuWidgetClass);
        }
        
        if (PauseMenuWidget)
        {
            PauseMenuWidget->AddToViewport(100); // High Z-order to appear on top
            
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
        }
    }
}

void AMazeGameMode::ResumeGame()
{
    if (CurrentGameState != EGameState::Paused)
    {
        return; // Can only resume when paused
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Game Resumed"));
    
    CurrentGameState = EGameState::Playing;
    UGameplayStatics::SetGamePaused(GetWorld(), false);
    
    // Hide pause menu
    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent();
    }
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
}

void AMazeGameMode::RestartGame()
{
    // Show loading screen immediately
    ShowLoadingScreen("Restarting...", 3.0f);  // Minimum 3 seconds
    
    UE_LOG(LogTemp, Warning, TEXT("[RestartGame] Restarting current level/game..."));
    
    // CRITICAL FIX: Reset resolution before restart
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
        UE_LOG(LogTemp, Warning, TEXT("[Restart] Resolution reset to 100%%"));
    }
    
    // Reset muddy effect state
    bMuddyEffectActive = false;
    MuddyEffectTimer = 0.0f;
    
    // Reset player speed before restart
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            PlayerChar->GetCharacterMovement()->MaxWalkSpeed = 600.0f;
            UE_LOG(LogTemp, Warning, TEXT("[Restart] Player speed reset to 600"));
        }
    }
    
    // Unpause if paused
    UGameplayStatics::SetGamePaused(GetWorld(), false);
    
    // Hide pause menu
    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent();
        PauseMenuWidget = nullptr;
    }
    
    // CRITICAL FIX: Hide Win/Lose widgets on restart!
    TArray<UUserWidget*> FoundWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UUserWidget::StaticClass());
    for (UUserWidget* Widget : FoundWidgets)
    {
        if (Widget && Widget->IsInViewport())
        {
            FString WidgetName = Widget->GetName();
            // Remove Win and Lose widgets
            if (WidgetName.Contains("Win") || WidgetName.Contains("Lose") || 
                WidgetName.Contains("Victory") || WidgetName.Contains("Defeat"))
            {
                Widget->RemoveFromParent();
                UE_LOG(LogTemp, Warning, TEXT("[RestartGame] Removed widget: %s"), *WidgetName);
            }
        }
    }
    
    // FIX: Delay restart to allow loading screen to show
    FTimerHandle RestartTimer;
    GetWorldTimerManager().SetTimer(RestartTimer, [this]()
    {
        // FIX: Check if in Free2Play mode or Level mode
        if (bInMenuPreview || CurrentLevel == 0)
        {
            // Free2Play mode - complete cleanup and regenerate
            UE_LOG(LogTemp, Warning, TEXT("[RestartGame] Free2Play mode - complete restart"));
            
            // COMPLETE CLEANUP - Delete everything
            // 1. Destroy all maze cells
            if (MazeManager)
            {
                for (auto& Row : MazeManager->MazeGrid)
                {
                    for (AMazeCell* Cell : Row)
                    {
                        if (Cell)
                        {
                            Cell->Destroy();
                        }
                    }
                }
                MazeManager->MazeGrid.Empty();
            }
            
            // 2. Destroy monster
            // Destroy all monsters
            for (AMonsterAI* Monster : SpawnedMonsters)
            {
                if (Monster)
                {
                    Monster->Destroy();
                }
            }
            SpawnedMonsters.Empty();
            
            // 3. Destroy star
            if (SpawnedStar)
            {
                SpawnedStar->Destroy();
                SpawnedStar = nullptr;
            }
            
            // 4. Destroy flashlight
            if (PlayerFlashlight)
            {
                PlayerFlashlight->DestroyComponent();
                PlayerFlashlight = nullptr;
            }
            
            // 5. Destroy player
            if (Player)
            {
                Player->Destroy();
                Player = nullptr;
            }
            
            // 6. Destroy muddy patches
            TArray<AActor*> MuddyPatches;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMuddyPatch::StaticClass(), MuddyPatches);
            for (AActor* Patch : MuddyPatches)
            {
                Patch->Destroy();
            }
            
            // 7. Destroy safe zones
            if (SpawnedSafeZone)
            {
                SpawnedSafeZone->Destroy();
                SpawnedSafeZone = nullptr;
            }
            
            // FIX ISSUE #3: Destroy HUD/Timer widget
            if (HUDWidget)
            {
                HUDWidget->RemoveFromParent();
                HUDWidget = nullptr;
            }
            
            // Reset all flags
            bMonsterSpawned = false;
            bMonsterSpeedBoosted = false;
            bSafeZoneActive = false;
            
            // NOW regenerate everything fresh
            MazeManager->SetMazeSize(CurrentMazeRows, CurrentMazeCols);
            MazeManager->GenerateMazeImmediate();
            
            SpawnPlayer();
            SpawnGoldenStar();
            CreatePlayerFlashlight();
            
            // Restart game
            StartGame();
        }
        else
        {
            // Level mode - complete cleanup and restart level
            UE_LOG(LogTemp, Warning, TEXT("[RestartGame] Level mode - restarting Level %d"), CurrentLevel);
            
            // COMPLETE CLEANUP - Delete everything
            CleanupBeforeLevel();
            
            // FIX: Also destroy HUD/Timer in Level mode
            if (HUDWidget)
            {
                HUDWidget->RemoveFromParent();
                HUDWidget = nullptr;
            }
            
            // Restart the same level (this will regenerate maze, spawn player, etc.)
            StartLevel(CurrentLevel);
        }
        
        // Set input mode back to game
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[RestartGame] Restart complete!"));
        
    }, 0.5f, false);
}

void AMazeGameMode::QuitGame()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Quitting game..."));
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        PC->ConsoleCommand("quit");
    }
}

void AMazeGameMode::ReturnToMainMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Returning to main menu with complete cleanup..."));
    
    // 1. Stop all timers for this object
    GetWorldTimerManager().ClearAllTimersForObject(this);
    UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] All timers cleared"));
    
    // 2. Stop credits music if playing
    if (CreditsMusicComponent && CreditsMusicComponent->IsPlaying())
    {
        CreditsMusicComponent->Stop();
        CreditsMusicComponent = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Credits music stopped"));
    }
    
    // 3. Complete cleanup of all game entities
    CleanupBeforeLevel();
    UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Game entities cleaned up"));
    
    // 4. Reset game state
    CurrentGameState = EGameState::NotStarted;
    CurrentLevel = 0;
    bInMenuPreview = true;
    UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Game state reset to Menu"));
    
    // 5. Hide all UI widgets
    if (HUDWidget)
    {
        HUDWidget->RemoveFromParent();
        HUDWidget = nullptr;
    }
    
    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent();
        PauseMenuWidget = nullptr;
    }
    
    if (SettingsMenuWidget)
    {
        SettingsMenuWidget->RemoveFromParent();
        SettingsMenuWidget = nullptr;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] All UI widgets hidden"));
    
    // 6. Reset blood moon lighting (Level 5)
    TArray<AActor*> FoundLights;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), FoundLights);
    for (AActor* LightActor : FoundLights)
    {
        ADirectionalLight* DirLight = Cast<ADirectionalLight>(LightActor);
        if (DirLight)
        {
            // Reset to normal white light
            DirLight->SetLightColor(FLinearColor::White);
            if (DirLight->GetLightComponent())
            {
                DirLight->GetLightComponent()->SetIntensity(1.0f);  // Normal intensity
            }
            UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Directional light reset to normal"));
        }
    }
    
    // 7. Restore audio (ensure game sounds work)
    if (UWorld* World = GetWorld())
    {
        if (FAudioDeviceHandle AudioDeviceHandle = World->GetAudioDevice())
        {
            if (FAudioDevice* AudioDevice = AudioDeviceHandle.GetAudioDevice())
            {
                AudioDevice->SetTransientPrimaryVolume(1.0f);
                UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Audio restored"));
            }
        }
    }
    
    // 8. Reset time dilation
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    
    // 9. Show main menu
    ShowMainMenu();
    
    UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Complete! Main menu displayed"));
}

void AMazeGameMode::ShowSettingsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Showing settings menu (Free2Play)"));
    
    // FIX: Remove main menu completely and recreate on return
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[ShowSettingsMenu] Main menu removed"));
    }
    
    if (SettingsMenuWidgetClass)
    {
        if (!SettingsMenuWidget)
        {
            SettingsMenuWidget = CreateWidget<UUserWidget>(GetWorld(), SettingsMenuWidgetClass);
        }
        
        if (SettingsMenuWidget)
        {
            SettingsMenuWidget->AddToViewport(200);
            
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
        }
    }
}

void AMazeGameMode::HideSettingsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Hiding settings menu"));
    
    if (SettingsMenuWidget)
    {
        SettingsMenuWidget->RemoveFromParent();
        SettingsMenuWidget = nullptr;
    }
    
    // Recreate main menu
    ShowMainMenu();
}

void AMazeGameMode::SaveSettings(int32 NewRows, int32 NewCols)
{
    UE_LOG(LogTemp, Warning, TEXT("[Settings] Saving maze size: %dx%d"), NewRows, NewCols);
    
    CurrentMazeRows = NewRows;
    CurrentMazeCols = NewCols;
    
    // Save to game settings
    UMazeGameSettings* Settings = GetMutableDefault<UMazeGameSettings>();
    if (Settings)
    {
        Settings->MazeRows = NewRows;
        Settings->MazeCols = NewCols;
        Settings->SaveConfig();
        
        UE_LOG(LogTemp, Warning, TEXT("[Settings] Settings saved to config file"));
    }
}

void AMazeGameMode::LoadSettings()
{
    UMazeGameSettings* Settings = GetMutableDefault<UMazeGameSettings>();
    if (Settings)
    {
        CurrentMazeRows = Settings->MazeRows;
        CurrentMazeCols = Settings->MazeCols;
        
        UE_LOG(LogTemp, Warning, TEXT("[Settings] Settings loaded: %dx%d"), CurrentMazeRows, CurrentMazeCols);
    }
}

void AMazeGameMode::ShowLoadingScreen(const FString& Message, float Duration)
{
    if (!LoadingScreenWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Loading] LoadingScreenWidgetClass not set"));
        return;
    }
    
    ULoadingScreenWidget* LoadingScreen = CreateWidget<ULoadingScreenWidget>(GetWorld(), LoadingScreenWidgetClass);
    if (LoadingScreen)
    {
        LoadingScreen->SetInstructions(Message);
        LoadingScreen->AddToViewport(999); // Highest Z-order
        
        // Remove after duration
        FTimerHandle LoadingTimer;
        GetWorldTimerManager().SetTimer(LoadingTimer, [LoadingScreen]()
        {
            if (LoadingScreen)
            {
                LoadingScreen->RemoveFromParent();
            }
        }, Duration, false);
        
        UE_LOG(LogTemp, Warning, TEXT("[Loading] Showing loading screen: %s"), *Message);
    }
}

void AMazeGameMode::ApplyMuddyEffect(float Duration)
{
    if (bMuddyEffectActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] Already active, extending duration"));
        MuddyEffectTimer = Duration;
        return;
    }
    
    bMuddyEffectActive = true;
    MuddyEffectTimer = Duration;
    
    // FIX ISSUE #2: Reduce resolution to 10% (was 50%)
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 10"));
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] ✗ Resolution reduced to 10%%"));
        
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
            FString::Printf(TEXT("✗ MUDDY! Vision blurred for %.0f seconds!"), Duration));
    }
    
    // FIX ISSUE #2: Reduce player speed much more (was 60%, now 30%)
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            float CurrentSpeed = PlayerChar->GetCharacterMovement()->MaxWalkSpeed;
            OriginalPlayerSpeed = CurrentSpeed;
            float NewSpeed = CurrentSpeed * 0.3f; // 30% of original speed (very slow)
            PlayerChar->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
            UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] ✗ Player speed reduced from %.0f to %.0f"), CurrentSpeed, NewSpeed);
        }
    }
}

void AMazeGameMode::SpawnTrapCells(int32 Count)
{
    if (!TrapCellClass || !MazeManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpawnTrapCells] TrapCellClass or MazeManager not set"));
        return;
    }
    
    // Get all maze cells
    TArray<AMazeCell*> AllCells;
    for (int32 Row = 0; Row < CurrentMazeRows; Row++)
    {
        for (int32 Col = 0; Col < CurrentMazeCols; Col++)
        {
            AMazeCell* Cell = MazeManager->GetCell(Row, Col);
            if (Cell)
            {
                AllCells.Add(Cell);
            }
        }
    }
    
    if (AllCells.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[SpawnTrapCells] No maze cells found!"));
        return;
    }
    
    // Randomly select cells for traps (avoid start cell at 0,0 and escape cell)
    int32 SpawnedCount = 0;
    for (int32 i = 0; i < Count && AllCells.Num() > 0; i++)
    {
        // Pick random cell
        int32 RandomIndex = FMath::RandRange(0, AllCells.Num() - 1);
        AMazeCell* Cell = AllCells[RandomIndex];
        
        // Skip start cell and escape cell
        if ((Cell->Row == 0 && Cell->Col == 0) || Cell->bIsEscapeCell)
        {
            AllCells.RemoveAt(RandomIndex);
            i--; // Try again
            continue;
        }
        
        // Spawn trap cell
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        ATrapCell* TrapCell = GetWorld()->SpawnActor<ATrapCell>(TrapCellClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (TrapCell)
        {
            TrapCell->Initialize(Cell, MazeManager->CellSize);
            SpawnedTrapCells.Add(TrapCell);
            SpawnedCount++;
            UE_LOG(LogTemp, Warning, TEXT("[SpawnTrapCells] Spawned trap cell at [%d,%d]"), Cell->Row, Cell->Col);
        }
        
        AllCells.RemoveAt(RandomIndex);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[SpawnTrapCells] Spawned %d trap cells"), SpawnedCount);
}

// ==================== LEVEL PROGRESSION FUNCTIONS ====================

void AMazeGameMode::StartLevel(int32 LevelNumber)
{
    if (!LevelManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[StartLevel] LevelManager not found!"));
        return;
    }
    
    CurrentLevel = LevelNumber;
    FLevelConfig Config = LevelManager->GetLevelConfig(LevelNumber);
    
    UE_LOG(LogTemp, Warning, TEXT("[StartLevel] Starting Level %d: %s"), LevelNumber, *Config.LevelName);
    
    // Record level attempt
    LevelManager->RecordLevelAttempt(LevelNumber);
    
    // Show level briefing
    if (LoadingScreenWidgetClass)
    {
        ULoadingScreenWidget* Briefing = CreateWidget<ULoadingScreenWidget>(GetWorld(), LoadingScreenWidgetClass);
        if (Briefing)
        {
            FString BriefingText = FString::Printf(TEXT("LEVEL %d: %s\n\n"), LevelNumber, *Config.LevelName);
            for (const FString& Msg : Config.BriefingMessages)
            {
                BriefingText += Msg + TEXT("\n");
            }
            
            Briefing->SetInstructions(BriefingText);
            Briefing->AddToViewport(999);
            
            // Remove briefing after 8 seconds and start level
            FTimerHandle BriefingTimer;
            GetWorldTimerManager().SetTimer(BriefingTimer, [this, Briefing, Config, LevelNumber]()
            {
                Briefing->RemoveFromParent();
                
                // Apply level configuration
                CurrentMazeRows = Config.MazeRows;
                CurrentMazeCols = Config.MazeCols;
                TotalGameTime = Config.TimeLimit;
                MonsterSpawnTime = Config.MonsterSpawnDelay;
                
                // Complete cleanup before starting
                CleanupBeforeLevel();
                
                // Generate new maze
                MazeManager->SetMazeSize(Config.MazeRows, Config.MazeCols);
                MazeManager->GenerateMazeImmediate();
                bMazeGenerated = true;
                
                // Spawn entities
                SpawnStars();
                SpawnPlayer();
                CreatePlayerFlashlight();
                // Spawn golden star (if enabled for this level)
                if (Config.bEnableGoldenStar)
                {
                    SpawnGoldenStar();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Level %d] 🔴 Golden star disabled - PURE SURVIVAL MODE!"), LevelNumber);
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
                            TEXT("🔴 NO GOLDEN STAR - SURVIVE TO ESCAPE!"));
                    }
                }
                
                // Spawn muddy patches if configured
                if (Config.NumMuddyPatches > 0)
                {
                    SpawnMuddyPatches(Config.NumMuddyPatches);
                    
                    // Spawn trap cells (use config count)
                    if (Config.NumTrapCells > 0)
                    {
                        SpawnTrapCells(Config.NumTrapCells);
                        UE_LOG(LogTemp, Warning, TEXT("[Level %d] Spawned %d trap cells"), LevelNumber, Config.NumTrapCells);
                    }
                }
                
                // Spawn safe zone if configured (only for levels 3-5)
                if (Config.MazeRegenTime > 0.0f && CurrentLevel > 2)
                {
                    SpawnSafeZone(Config.MazeRegenTime);
                    
                    // Show warning about maze regeneration
                    if (GEngine)
                    {
                        // Calculate REMAINING time when regen happens
                        float TimeWhenRegenHappens = TotalGameTime - Config.MazeRegenTime;
                        float RegenMinutes = FMath::FloorToInt(TimeWhenRegenHappens / 60.0f);
                        float RegenSeconds = FMath::FloorToInt(FMath::Fmod(TimeWhenRegenHappens, 60.0f));
                        GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow,
                            FString::Printf(TEXT("⚠️ MAZE WILL REGENERATE AT %02d:%02d REMAINING - REACH SAFE ZONE!"), 
                            (int32)RegenMinutes, (int32)RegenSeconds));
                    }
                }
                
                // Start game
                LevelStartTime = GetWorld()->GetTimeSeconds();
                StartGame();
                
                // Level 5 Blood Moon atmosphere
                if (CurrentLevel == 5)
                {
                    SetBloodMoonAtmosphere();
                }
                
                // Set input mode to game only
                APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
                if (PC)
                {
                    PC->bShowMouseCursor = false;
                    PC->SetInputMode(FInputModeGameOnly());
                }
                
            }, 8.0f, false);
        }
    }
}

void AMazeGameMode::CompleteLevel()
{
    if (CurrentGameState != EGameState::Playing) return;
    
    CurrentGameState = EGameState::Won;
    
    // Calculate completion time
    LevelCompletionTime = TotalGameTime - RemainingTime;
    
    // Calculate stars
    StarsEarnedThisLevel = LevelManager->CalculateStarRating(CurrentLevel, LevelCompletionTime);
    
    // Record completion
    LevelManager->RecordLevelCompletion(CurrentLevel, LevelCompletionTime, StarsEarnedThisLevel);
    
    // Unlock next level
    if (CurrentLevel < 5)
    {
        LevelManager->UnlockLevel(CurrentLevel + 1);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[CompleteLevel] Level %d completed in %.2f seconds - %d stars!"), 
           CurrentLevel, LevelCompletionTime, StarsEarnedThisLevel);
    
    // Play win sound
    if (WinSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), WinSound, 0.8f);
    }
    
    // Hide HUD
    if (HUDWidget)
    {
        HUDWidget->RemoveFromParent();
    }
    
    // Stop all monsters
    for (AMonsterAI* Monster : SpawnedMonsters)
    {
        if (Monster)
        {
            Monster->StopChasing();
        }
    }
    
    // Show completion message
    if (GEngine)
    {
        FString StarText = FString::Printf(TEXT("⭐ %d STAR%s!"), StarsEarnedThisLevel, StarsEarnedThisLevel == 1 ? TEXT("") : TEXT("S"));
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow,
            FString::Printf(TEXT("🎉 LEVEL %d COMPLETE! %s 🎉"), CurrentLevel, *StarText));
    }
    
    // Wait 3 seconds then proceed
    FTimerHandle CompletionTimer;
    GetWorldTimerManager().SetTimer(CompletionTimer, [this]()
    {
        if (CurrentLevel == 5)
        {
            // Final level - show credits
            ShowCredits();
        }
        else
        {
            // Advance to next level
            StartLevel(CurrentLevel + 1);
        }
    }, 3.0f, false);
}

void AMazeGameMode::ShowLevelSelectScreen()
{
    if (!LevelSelectWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[ShowLevelSelectScreen] LevelSelectWidgetClass not set!"));
        return;
    }
    
    // FIX #2: Hide main menu properly
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[ShowLevelSelectScreen] Main menu removed"));
    }
    
    // FIX #2: Store level select widget reference for proper cleanup
    if (!LevelSelectWidget)
    {
        LevelSelectWidget = CreateWidget<ULevelSelectWidget>(GetWorld(), LevelSelectWidgetClass);
    }
    
    if (LevelSelectWidget)
    {
        LevelSelectWidget->AddToViewport(100);
        
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeUIOnly());
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[ShowLevelSelectScreen] Level select displayed"));
    }
}

void AMazeGameMode::LoadSelectedLevel(int32 LevelNumber)
{
    UE_LOG(LogTemp, Warning, TEXT("[LoadSelectedLevel] Loading Level %d"), LevelNumber);
    
    // FIX #2: Remove level select widget
    if (LevelSelectWidget)
    {
        LevelSelectWidget->RemoveFromParent();
        LevelSelectWidget = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[LoadSelectedLevel] Level select widget removed"));
    }
    
    // Complete cleanup
    CleanupBeforeLevel();
    
    // Start the selected level
    StartLevel(LevelNumber);
}

void AMazeGameMode::ShowCredits()
{
    if (!CreditsWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[ShowCredits] CreditsWidgetClass not set!"));
        ShowMainMenu();  // FIX ISSUE #2: Return to main menu, not restart
        return;
    }
    
    // FIX: Use member variable instead of local variable to prevent garbage collection!
    CreditsWidget = CreateWidget<UCreditsWidget>(GetWorld(), CreditsWidgetClass);
    if (CreditsWidget)
    {
        CreditsWidget->AddToViewport(100);
        
        // FIX ISSUE #1: Start the rolling animation
        CreditsWidget->StartCreditsRoll();
        
        // Play credits music and store reference
        if (CreditsMusic)
        {
            CreditsMusicComponent = UGameplayStatics::SpawnSound2D(GetWorld(), CreditsMusic, 1.0f);
            UE_LOG(LogTemp, Warning, TEXT("[ShowCredits] Credits music playing"));
        }
        
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeUIOnly());
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[ShowCredits] Credits widget stored in member variable - will not be garbage collected"));
    }
}

void AMazeGameMode::CleanupBeforeLevel()
{
    UE_LOG(LogTemp, Warning, TEXT("[CleanupBeforeLevel] Cleaning up all entities..."));
    
    // Destroy all maze cells
    if (MazeManager)
    {
        for (auto& Row : MazeManager->MazeGrid)
        {
            for (AMazeCell* Cell : Row)
            {
                if (Cell)
                {
                    Cell->Destroy();
                }
            }
        }
        MazeManager->MazeGrid.Empty();
    }
    
    // Destroy monster
    // Destroy all monsters
    for (AMonsterAI* Monster : SpawnedMonsters)
    {
        if (Monster)
        {
            Monster->Destroy();
        }
    }
    SpawnedMonsters.Empty();
    
    // Destroy star
    if (SpawnedStar)
    {
        SpawnedStar->Destroy();
        SpawnedStar = nullptr;
    }
    
    // Destroy muddy patches
    TArray<AActor*> MuddyPatches;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMuddyPatch::StaticClass(), MuddyPatches);
    for (AActor* Patch : MuddyPatches)
    {
        Patch->Destroy();
    }
    // Cleanup monsters
    for (AMonsterAI* Monster : SpawnedMonsters)
    {
        if (Monster)
        {
            Monster->Destroy();
        }
    }
    SpawnedMonsters.Empty();
    
    // Destroy safe zones
    if (SpawnedSafeZone)
    {
        SpawnedSafeZone->Destroy();
        SpawnedSafeZone = nullptr;
    }
    
    // Destroy flashlight
    if (PlayerFlashlight)
    {
        PlayerFlashlight->DestroyComponent();
        PlayerFlashlight = nullptr;
    }
    
    // Reset flags
    bMonsterSpawned = false;
    bMonsterSpeedBoosted = false;
    bMuddyEffectActive = false;
    bSafeZoneActive = false;
    
    // Level 5 Blood Moon
    bSecondMonsterSpawned = false;
    bAggressiveModeActivated = false;
    
    // Reset player speed
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            PlayerChar->GetCharacterMovement()->MaxWalkSpeed = 600.0f;
        }
    }
    
    // Reset resolution
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[CleanupBeforeLevel] ✓ Cleanup complete"));
}

void AMazeGameMode::CreatePlayerFlashlight()
{
    if (!Player)
    {
        UE_LOG(LogTemp, Error, TEXT("[CreatePlayerFlashlight] No player!"));
        return;
    }
    
    // FIX: Don't create if flashlight already exists
    if (PlayerFlashlight)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CreatePlayerFlashlight] Flashlight already exists, skipping creation"));
        return;
    }
    
    PlayerFlashlight = NewObject<USpotLightComponent>(Player);
    if (PlayerFlashlight)
    {
        PlayerFlashlight->RegisterComponent();
        PlayerFlashlight->AttachToComponent(Player->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        
        // ENHANCED: Increased intensity and range for better visibility
        PlayerFlashlight->SetIntensity(10000.0f);           // Increased from 6000 - much brighter
        PlayerFlashlight->SetInnerConeAngle(20.0f);         // Tighter from 25 - sharper beam
        PlayerFlashlight->SetOuterConeAngle(45.0f);         // Tighter from 50 - more focused
        PlayerFlashlight->SetAttenuationRadius(3500.0f);    // Increased from 2500 - longer range
        PlayerFlashlight->SetLightColor(FLinearColor(1.0f, 0.98f, 0.9f)); // Slightly warmer white
        PlayerFlashlight->SetCastShadows(false);  // Disabled for performance
        
        PlayerFlashlight->SetRelativeLocation(FVector(50.0f, 0.0f, 60.0f));
        PlayerFlashlight->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
        
        UE_LOG(LogTemp, Warning, TEXT("[CreatePlayerFlashlight] Flashlight created (Enhanced: 10000 intensity, 3500 range)"));
    }
}

void AMazeGameMode::SpawnSafeZone(float ActivationTime)
{
    if (!SafeZoneCellClass || !MazeManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpawnSafeZone] SafeZoneCellClass not set!"));
        return;
    }
    
    // Find random cell (not escape cell)
    AMazeCell* SafeCell = nullptr;
    int32 Attempts = 0;
    do {
        SafeCell = MazeManager->GetRandomCell();
        Attempts++;
    } while (SafeCell && SafeCell->bIsEscapeCell && Attempts < 100);
    
    if (SafeCell)
    {
        FVector SpawnLoc = SafeCell->GetActorLocation();
        SpawnLoc.Z = 50.0f;
        
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        SpawnedSafeZone = GetWorld()->SpawnActor<ASafeZoneCell>(SafeZoneCellClass, SpawnLoc, FRotator::ZeroRotator, Params);
        
        if (SpawnedSafeZone)
        {
            bSafeZoneActive = true;
            SafeZoneActivationTime = ActivationTime;
            UE_LOG(LogTemp, Warning, TEXT("[SpawnSafeZone] ✓ Safe zone spawned at [%d,%d]"), SafeCell->Row, SafeCell->Col);
        }
    }
}

void AMazeGameMode::CheckSafeZoneStatus()
{
    if (!Player || !SpawnedSafeZone) return;
    
    FVector PlayerLoc = Player->GetActorLocation();
    FVector SafeZoneLoc = SpawnedSafeZone->GetActorLocation();
    float Distance = FVector::Dist2D(PlayerLoc, SafeZoneLoc);
    
    if (Distance < 300.0f)
    {
        // Player is safe!
        UE_LOG(LogTemp, Warning, TEXT("[SafeZone] ✓ Player is in safe zone!"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("✓ SAFE! You're protected!"));
        }
    }
    else
    {
        // Player is NOT safe - vulnerable to monster
        UE_LOG(LogTemp, Warning, TEXT("[SafeZone] ✗ Player NOT in safe zone - vulnerable!"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("⚠️ NOT SAFE! Get to safe zone!"));
        }
    }
}

void AMazeGameMode::SpawnMuddyPatches(int32 Count)
{
    if (!MuddyPatchClass || !MazeManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpawnMuddyPatches] MuddyPatchClass not set!"));
        return;
    }
    
    for (int32 i = 0; i < Count; i++)
    {
        AMazeCell* PatchCell = MazeManager->GetRandomCell();
        if (PatchCell && !PatchCell->bIsEscapeCell)
        {
            FVector SpawnLoc = PatchCell->GetActorLocation();
            SpawnLoc.Z = 10.0f;
            
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            AMuddyPatch* Patch = GetWorld()->SpawnActor<AMuddyPatch>(MuddyPatchClass, SpawnLoc, FRotator::ZeroRotator, Params);
            if (Patch)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SpawnMuddyPatches] ✓ Muddy patch %d spawned at [%d,%d]"), 
                       i + 1, PatchCell->Row, PatchCell->Col);
            }
        }
    }
}

// ==================== STUB IMPLEMENTATIONS FOR UNUSED FUNCTIONS ====================
// These functions are declared in header but not currently used in the game flow
// Implementing as stubs to avoid linker errors

void AMazeGameMode::CalculateStarRating()
{
    // This functionality is now handled in CompleteLevel() using LevelManager->CalculateStarRating()
    UE_LOG(LogTemp, Warning, TEXT("[CalculateStarRating] Stub - use CompleteLevel() instead"));
}

void AMazeGameMode::ShowLevelBriefing()
{
    // This functionality is now handled in StartLevel() with 8-second briefing
    UE_LOG(LogTemp, Warning, TEXT("[ShowLevelBriefing] Stub - use StartLevel() instead"));
}

void AMazeGameMode::ShowLevelCompleteScreen()
{
    // This functionality is now handled in CompleteLevel() which auto-advances to next level
    UE_LOG(LogTemp, Warning, TEXT("[ShowLevelCompleteScreen] Stub - use CompleteLevel() instead"));
}

void AMazeGameMode::RetryCurrentLevel()
{
    // Restart current level
    UE_LOG(LogTemp, Warning, TEXT("[RetryCurrentLevel] Restarting level %d"), CurrentLevel);
    StartLevel(CurrentLevel);
}

void AMazeGameMode::LoadNextLevel()
{
    // Load next level
    if (CurrentLevel < 5)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoadNextLevel] Loading level %d"), CurrentLevel + 1);
        StartLevel(CurrentLevel + 1);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoadNextLevel] Already at final level"));
        ShowCredits();
    }
}

void AMazeGameMode::HideLoadingScreen()
{
    // Loading screens auto-hide after duration in ShowLoadingScreen()
    UE_LOG(LogTemp, Warning, TEXT("[HideLoadingScreen] Stub - loading screens auto-hide"));
}

void AMazeGameMode::CloseSettingsMenu()
{
    // Use HideSettingsMenu() instead
    UE_LOG(LogTemp, Warning, TEXT("[CloseSettingsMenu] Calling HideSettingsMenu()"));
    HideSettingsMenu();
}

void AMazeGameMode::SetMazeRows(int32 Rows)
{
    CurrentMazeRows = FMath::Clamp(Rows, 5, 30);
    UE_LOG(LogTemp, Warning, TEXT("[SetMazeRows] Maze rows set to %d"), CurrentMazeRows);
}

void AMazeGameMode::SetMazeCols(int32 Cols)
{
    CurrentMazeCols = FMath::Clamp(Cols, 5, 30);
    UE_LOG(LogTemp, Warning, TEXT("[SetMazeCols] Maze cols set to %d"), CurrentMazeCols);
}

void AMazeGameMode::ShowOptionsMenu()
{
    // Options menu functionality
    UE_LOG(LogTemp, Warning, TEXT("[ShowOptionsMenu] Showing options menu"));
    
    if (OptionsMenuWidgetClass)
    {
        if (!OptionsMenuWidget)
        {
            OptionsMenuWidget = CreateWidget<UUserWidget>(GetWorld(), OptionsMenuWidgetClass);
        }
        
        if (OptionsMenuWidget)
        {
            OptionsMenuWidget->AddToViewport(200);
            
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
        }
    }
}

void AMazeGameMode::CloseOptionsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[CloseOptionsMenu] Closing options menu"));
    
    if (OptionsMenuWidget)
    {
        OptionsMenuWidget->RemoveFromParent();
        OptionsMenuWidget = nullptr;
    }
    
    // Return to previous state
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        if (CurrentGameState == EGameState::Playing)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
        else
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeUIOnly());
        }
    }
}

void AMazeGameMode::SetGameVolume(float Volume)
{
    GameVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    
    // Apply volume to audio device
    if (UWorld* World = GetWorld())
    {
        if (FAudioDevice* AudioDevice = World->GetAudioDeviceRaw())
        {
            AudioDevice->SetTransientPrimaryVolume(GameVolume);
            UE_LOG(LogTemp, Warning, TEXT("[SetGameVolume] Volume set to %.2f"), GameVolume);
        }
    }
}

void AMazeGameMode::CheckSafeZoneProtection()
{
    // Use CheckSafeZoneStatus() instead
    UE_LOG(LogTemp, Warning, TEXT("[CheckSafeZoneProtection] Calling CheckSafeZoneStatus()"));
    CheckSafeZoneStatus();
}

bool AMazeGameMode::IsPlayerInSafeZone() const
{
    if (!Player || !SpawnedSafeZone)
    {
        return false;
    }
    
    FVector PlayerLoc = Player->GetActorLocation();
    FVector SafeZoneLoc = SpawnedSafeZone->GetActorLocation();
    float Distance = FVector::Dist2D(PlayerLoc, SafeZoneLoc);
    
    return Distance < 300.0f;
}

// CHEAT CODE: Instant Win
void AMazeGameMode::Win()
{
    if (CurrentGameState != EGameState::Playing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CHEAT] Win command ignored - not in playing state"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
                TEXT("Cheat 'win' only works during gameplay!"));
        }
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[CHEAT] Win command activated! Instant victory!"));
    
    // Show cheat message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
            TEXT("🎮 CHEAT ACTIVATED: Instant Win!"));
    }
    
    // Complete the level instantly
    CompleteLevel();
}

// ==================== LEVEL 5 BLOOD MOON FUNCTIONS ====================

void AMazeGameMode::SpawnSecondMonster()
{
    UE_LOG(LogTemp, Warning, TEXT("[BloodMoon] ⚠️ SECOND MONSTER SPAWNING!"));
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
            TEXT("🔴 BLOOD MOON: SECOND MONSTER AWAKENS!"));
    }
    
    // CRITICAL FIX: Reset bMonsterSpawned flag so SpawnMonster() works!
    bMonsterSpawned = false;
    
    // Spawn using existing logic
    SpawnMonster();
}

void AMazeGameMode::ActivateAggressiveMode()
{
    UE_LOG(LogTemp, Warning, TEXT("[BloodMoon] ⚠️ AGGRESSIVE MODE ACTIVATED!"));
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
            TEXT("🔴 BLOOD MOON: MONSTERS ENRAGED! SPEED INCREASED!"));
    }
    
    // Make all monsters faster
    for (AMonsterAI* Monster : SpawnedMonsters)
    {
        if (Monster && Monster->GetCharacterMovement())
        {
            float CurrentSpeed = Monster->GetCharacterMovement()->MaxWalkSpeed;
            Monster->GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed * 1.5f;  // 50% faster
            UE_LOG(LogTemp, Warning, TEXT("[BloodMoon] Monster speed: %.0f -> %.0f"), 
                CurrentSpeed, Monster->GetCharacterMovement()->MaxWalkSpeed);
        }
    }
}

void AMazeGameMode::SetBloodMoonAtmosphere()
{
    UE_LOG(LogTemp, Warning, TEXT("[BloodMoon] 🔴 Setting blood red atmosphere..."));
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
            TEXT("🔴 BLOOD MOON RISES - SURVIVE THE CRIMSON NIGHT!"));
    }
    
    // Find directional light and set to blood red
    TArray<AActor*> FoundLights;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), FoundLights);
    for (AActor* LightActor : FoundLights)
    {
        ADirectionalLight* DirLight = Cast<ADirectionalLight>(LightActor);
        if (DirLight)
        {
            // Blood red color
            DirLight->SetLightColor(FLinearColor(0.8f, 0.0f, 0.0f));  // Deep red
            // REDUCED INTENSITY - flashlight still useful!
            if (DirLight->GetLightComponent())
            {
                DirLight->GetLightComponent()->SetIntensity(0.3f);  // Very low - flashlight needed!
            }
            
            UE_LOG(LogTemp, Warning, TEXT("[BloodMoon] ✓ Directional light set to blood red"));
        }
    }
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
            TEXT("🔴 BLOOD MOON RISES - SURVIVE THE CRIMSON NIGHT!"));
    }
}

