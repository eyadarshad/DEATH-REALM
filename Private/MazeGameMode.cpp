// MazeGameMode.cpp - FIXED MONSTER SPAWN LOCATION + WALL OVERLAPS + SPEED BOOST
#include "MazeGameMode.h"
#include "MazeManager.h"
#include "MazeCell.h"
#include "MonsterAI.h"
#include "GoldenStar.h"
#include "MuddyPatch.h"
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
    
    // Maze trap initialization
    bMazeTrapTriggered = false;
    bPlayerTrapped = false;
    TrapDuration = 5.0f;
    TrappedCell = nullptr;
    
    // Flashlight control
    PlayerFlashlight = nullptr;
    bEnableCameraControlledFlashlight = false;  // Disabled - using fixed flashlight position
    
    // Camera rotation for menu preview
    CurrentCameraRotation = FRotator::ZeroRotator;
    TargetCameraRotation = FRotator::ZeroRotator;
    CameraRotationSpeed = 2.0f;  // Smooth interpolation speed
}

void AMazeGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Show loading screen immediately (reduced to 2s for better UX)
    ShowLoadingScreen("Initializing Maze Runner...", 2.0f);
    
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
        UE_LOG(LogTemp, Warning, TEXT("✓ Found existing MazeManager"));
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
        UE_LOG(LogTemp, Warning, TEXT("✓ Created new MazeManager"));
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
    UE_LOG(LogTemp, Warning, TEXT("✓ Settings loaded - Maze size: %dx%d"), CurrentMazeRows, CurrentMazeCols);
    
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
        bInMenuPreview = true;
        UE_LOG(LogTemp, Warning, TEXT("[MenuPreview] ✓ Player and star spawned for preview"));
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
                
                UE_LOG(LogTemp, Warning, TEXT("✓ Main Menu displayed with interactive preview!"));
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
    
    // Show loading screen AFTER validation to cover cleanup/regeneration glitches
    // Check if size will change to show appropriate message
    bool bWillRegenerate = false;
    if (MazeManager)
    {
        int32 CurrentRows = MazeManager->Rows;
        int32 CurrentCols = MazeManager->Cols;
        
        if (CurrentRows != CurrentMazeRows || CurrentCols != CurrentMazeCols)
        {
            bWillRegenerate = true;
            ShowLoadingScreen(FString::Printf(TEXT("Generating %dx%d Maze..."), CurrentMazeRows, CurrentMazeCols), 3.0f);
        }
    }
    
    if (!bWillRegenerate)
    {
        // First time start - shorter loading
        ShowLoadingScreen("Preparing Your Challenge...", 2.0f);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] STARTING NEW GAME"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    
    // CRITICAL FIX: Reset resolution to 100% at start of every game
    // This fixes the bug where restarting during muddy effect keeps low resolution
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Resolution reset to 100%%"));
    }
    bMuddyEffectActive = false;
    MuddyEffectTimer = 0.0f;
    
    // CRITICAL FIX: Reset player speed to default at start of every game
    // This fixes the bug where restarting during muddy effect keeps slow speed
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            PlayerChar->GetCharacterMovement()->MaxWalkSpeed = 600.0f;  // Default Unreal speed
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Player speed reset to 600"));
        }
    }
    
    // COMPLETE CLEAN RESET FOR FREE2PLAY MODE
    // Always check if maze size changed OR if this is a fresh start
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
    
    // CRITICAL: ALWAYS do complete cleanup if size changed
    if (bSizeChanged)
    {
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] COMPLETE CLEAN RESET"));
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        
        // 1. Destroy ALL maze cells (old maze)
        TArray<AActor*> FoundCells;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMazeCell::StaticClass(), FoundCells);
        for (AActor* Cell : FoundCells)
        {
            if (Cell)
            {
                Cell->Destroy();
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] ✓ Destroyed %d maze cells"), FoundCells.Num());
        
        // 2. Destroy all muddy patches
        TArray<AActor*> FoundMuddyPatches;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMuddyPatch::StaticClass(), FoundMuddyPatches);
        for (AActor* Patch : FoundMuddyPatches)
        {
            if (Patch)
            {
                Patch->Destroy();
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] ✓ Destroyed %d muddy patches"), FoundMuddyPatches.Num());
        
        // 3. Destroy golden star
        if (SpawnedStar)
        {
            SpawnedStar->Destroy();
            SpawnedStar = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] ✓ Destroyed golden star"));
        }
        
        // 4. Destroy monster
        if (SpawnedMonster)
        {
            SpawnedMonster->Destroy();
            SpawnedMonster = nullptr;
            bMonsterSpawned = false;
            UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] ✓ Destroyed monster"));
        }
        
        // 5. Destroy old flashlight
        if (PlayerFlashlight)
        {
            PlayerFlashlight->DestroyComponent();
            PlayerFlashlight = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] ✓ Destroyed flashlight"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] ✓ All old objects destroyed - ready for fresh generation"));
    }
    
    if (!bMazeGenerated || bSizeChanged)
    {
        if (bSizeChanged)
        {
            UE_LOG(LogTemp, Warning, TEXT("[FREE2PLAY] Generating FRESH maze with size: %dx%d"), CurrentMazeRows, CurrentMazeCols);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze not pre-generated, generating now..."));
        }
        
        MazeManager->SetMazeSize(CurrentMazeRows, CurrentMazeCols);
        MazeManager->GenerateMazeImmediate();
        bMazeGenerated = true;
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze generated immediately"));
        
        // Spawn stars in the sky
        SpawnStars();
        
        if (!MazeManager->GetEscapeCell())
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Maze generation FAILED - no escape cell!"));
            return;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Escape cell created at: Row=%d, Col=%d"), 
               MazeManager->GetEscapeCell()->Row, MazeManager->GetEscapeCell()->Col);
    }
    
    // Step 1: Initialize game state
    CurrentGameState = EGameState::Playing;
    RemainingTime = TotalGameTime;
    bMonsterSpawned = false;
    bMonsterSpeedBoosted = false;
    bMazeTrapTriggered = false;  // RESET trap flag for new game
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
            UE_LOG(LogTemp, Warning, TEXT("✓ HUD Widget created and displayed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠ HUDWidgetClass not set - no HUD will be shown"));
    }
    
    // Step 3: Player and star handling
    // CRITICAL FIX: Always respawn player if maze was regenerated
    // Old player position might be beyond new escape cell boundary
    
    // FORCE respawn if maze size changed (player exists but needs new position)
    if (bSizeChanged)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze regenerated, FORCE respawning player at new start location..."));
        
        // CRITICAL: Destroy old flashlight before respawning
        if (PlayerFlashlight)
        {
            PlayerFlashlight->DestroyComponent();
            PlayerFlashlight = nullptr;
        }
        
        SpawnPlayer();
    }
    else if (!Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player missing, spawning..."));
        SpawnPlayer();
    }
    
    // CRITICAL FIX: Always respawn star if maze was regenerated (escape cell changed)
    if (bSizeChanged)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Maze regenerated, FORCE respawning star at new location..."));
        
        // Destroy old star if it exists
        if (SpawnedStar)
        {
            SpawnedStar->Destroy();
            SpawnedStar = nullptr;
        }
        
        SpawnGoldenStar();
    }
    else if (!SpawnedStar)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Star missing, spawning..."));
        SpawnGoldenStar();
    }
    
    // Step 4: Monster spawn timer (starts NOW when game begins)
    // CRITICAL FIX: Reset monster spawn flag if maze regenerated
    if (bSizeChanged)
    {
        bMonsterSpawned = false;
        if (SpawnedMonster)
        {
            SpawnedMonster->Destroy();
            SpawnedMonster = nullptr;
        }
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster reset for new maze size"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster will spawn in %.0f seconds"), MonsterSpawnTime);
    FTimerHandle MonsterTimer;
    GetWorldTimerManager().SetTimer(MonsterTimer, this, &AMazeGameMode::SpawnMonster, MonsterSpawnTime, false);
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Game Started!"));
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
        
        // CRITICAL FIX: Always reset camera rotation to prevent menu rotation persisting
        CurrentCameraRotation = FRotator::ZeroRotator;
        TargetCameraRotation = FRotator::ZeroRotator;
        PC->SetControlRotation(FRotator::ZeroRotator);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Camera rotation reset to zero"));
    }
    
    // Start the game
    StartGame();
}

void AMazeGameMode::ShowMainMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Showing main menu"));
    
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
            
            UE_LOG(LogTemp, Warning, TEXT("✓ Main Menu displayed!"));
        }
    }
}

void AMazeGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Performance monitoring (SKIP during pause for accurate metrics)
    if (DeltaTime > 0.0f && CurrentGameState != EGameState::Paused)
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
        if (RemainingTime <= 30.0f && !bMonsterSpeedBoosted && SpawnedMonster)
        {
            bMonsterSpeedBoosted = true;
            
            // Store original speed if not already stored
            if (OriginalMonsterSpeed == 0.0f)
            {
                OriginalMonsterSpeed = SpawnedMonster->MoveSpeed;
            }
            
            // Increase speed by 50%
            float NewSpeed = OriginalMonsterSpeed * 1.5f;
            SpawnedMonster->MoveSpeed = NewSpeed;
            
            // Update character movement component
            if (SpawnedMonster->GetCharacterMovement())
            {
                SpawnedMonster->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
            }
            
            // GROW MONSTER TO 1.7x SIZE - Make it terrifying!
            FVector NewScale = FVector(1.7f, 1.7f, 1.7f);
            SpawnedMonster->SetActorScale3D(NewScale);
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] 👹 MONSTER GREW TO 1.7x SIZE! 👹"));
            
            // Play transformation sound if set (different from speed boost sound)
            if (MonsterTransformSound)
            {
                UGameplayStatics::PlaySound2D(GetWorld(), MonsterTransformSound, 1.0f);
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] 🔊 MONSTER TRANSFORMATION SOUND! 🔊"));
            }
            else if (MonsterSpeedBoostSound)
            {
                // Fallback to speed boost sound if transform sound not set
                UGameplayStatics::PlaySound2D(GetWorld(), MonsterSpeedBoostSound, 1.0f);
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] ⚡ MONSTER SPEED INCREASED! ⚡"));
            }
            
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster speed boosted from %.0f to %.0f (Last 30 seconds!)"), 
                   OriginalMonsterSpeed, NewSpeed);
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
                    UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] ✓ Effect expired! Resolution restored to %.0f%%"), OriginalResolution);
                    
                    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                        TEXT("✓ Vision and speed restored!"));
                }
                
                // Restore player speed
                if (Player)
                {
                    ACharacter* PlayerChar = Cast<ACharacter>(Player);
                    if (PlayerChar && PlayerChar->GetCharacterMovement())
                    {
                        PlayerChar->GetCharacterMovement()->MaxWalkSpeed = OriginalPlayerSpeed;
                        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] ✓ Player speed restored to %.0f"), OriginalPlayerSpeed);
                    }
                }
            }
        }
        
        // MAZE TRAP: Check for trap trigger at 60 seconds elapsed
        if (!bMazeTrapTriggered && CurrentGameState == EGameState::Playing)
        {
            float ElapsedTime = TotalGameTime - RemainingTime;
            if (ElapsedTime >= 60.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] ⚠️ 60 seconds elapsed - triggering maze trap!"));
                TriggerMazeTrap();
            }
        }
        
        // MAZE TRAP: Handle trap countdown
        if (bPlayerTrapped)
        {
            TrapDuration -= DeltaTime;
            if (TrapDuration <= 0.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Trap duration expired - releasing player"));
                ReleaseTrap();
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
    
    // CRITICAL FIX: Scale minimum distance based on maze size to prevent instant wins
    // For small mazes (2x2), use at least 30% of diagonal distance
    float MazeDiagonal = FMath::Sqrt((float)(FMath::Square(MazeManager->Rows) + FMath::Square(MazeManager->Cols)));
    float MinCellsAway = FMath::Max(7.0f, MazeDiagonal * 0.3f);  // At least 7 cells OR 30% of diagonal
    float MinDistanceFromEscape = MazeManager->CellSize * MinCellsAway;
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Spawn distance: %.1f cells (maze: %dx%d, diagonal: %.1f)"), 
           MinCellsAway, MazeManager->Rows, MazeManager->Cols, MazeDiagonal);
    
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
        // Random offset was causing spawns at wall edges
        float SafeOffset = 0.0f;  // Changed from 200.0f to 0.0f
        float RandomX = 0.0f;  // No random offset
        float RandomY = 0.0f;  // No random offset
        
        FVector SpawnLocation = CellLocation;
        SpawnLocation.X += RandomX;
        SpawnLocation.Y += RandomY;
        SpawnLocation.Z = 100.0f; // 1 meter above ground
        
        Player->SetActorLocation(SpawnLocation);
        Player->SetActorRotation(FRotator::ZeroRotator);
        
        InitialPlayerLocation = SpawnLocation;
        
        // Add flashlight to player (fixed position, increased range)
        PlayerFlashlight = NewObject<USpotLightComponent>(Player);
        if (PlayerFlashlight)
        {
            PlayerFlashlight->RegisterComponent();
            PlayerFlashlight->AttachToComponent(Player->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            
            // Enhanced flashlight settings with fixed position
            PlayerFlashlight->SetIntensity(6000.0f);  // Increased brightness
            PlayerFlashlight->SetInnerConeAngle(25.0f);  // Wider inner cone
            PlayerFlashlight->SetOuterConeAngle(50.0f);  // Wider outer cone
            PlayerFlashlight->SetAttenuationRadius(2500.0f);  // Significantly increased range
            PlayerFlashlight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));  // Warm light
            PlayerFlashlight->SetCastShadows(true);
            
            // Fixed position: at head level, pointing slightly downward
            PlayerFlashlight->SetRelativeLocation(FVector(50.0f, 0.0f, 60.0f));
            PlayerFlashlight->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));  // Slight downward angle
            
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Fixed-position flashlight attached to player"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Player spawned at cell [%d,%d]"), 
               SpawnCell->Row, SpawnCell->Col);
        UE_LOG(LogTemp, Warning, TEXT("  Cell Location: %s"), *CellLocation.ToString());
        UE_LOG(LogTemp, Warning, TEXT("  Player Location: %s (offset: %.0f, %.0f)"), 
               *SpawnLocation.ToString(), RandomX, RandomY);
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
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Golden Star spawned at [%d,%d]"), 
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
	
	SpawnedMonster = GetWorld()->SpawnActor<AMonsterAI>(
		MonsterClass, 
		MonsterLocation, 
		FRotator::ZeroRotator, 
		SpawnParams
	);
	
	if (SpawnedMonster)
    {
        SpawnedMonster->StartChasing(Player);
        
        // Play growl sound on initial spawn only
        if (SpawnedMonster->GrowlSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), SpawnedMonster->GrowlSound, 0.9f);
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Monster growl sound played on initial spawn"));
        }
        
        bMonsterSpawned = true;
        
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓✓✓ MONSTER SPAWNED - RUN! ✓✓✓"));
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


void AMazeGameMode::CheckWinCondition()
{
    // CRITICAL FIX: Prevent multiple win triggers
    if (!Player || !MazeManager || CurrentGameState != EGameState::Playing) return;
    if (CurrentGameState == EGameState::Won) return;
    
    FVector PlayerLoc = Player->GetActorLocation();
    float CellSize = MazeManager->CellSize;
    AMazeCell* EscapeCell = MazeManager->GetEscapeCell();
    
    if (!EscapeCell) return;
    
    FVector EscapeCellLoc = EscapeCell->GetActorLocation();
    
    // ONLY win if player has moved BEYOND the maze boundary through the escape opening
    // Increased threshold to 0.6 (60% of cell size) to make it more obvious
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
    if (!SpawnedMonster || !Player || CurrentGameState != EGameState::Playing) return;
    
    if (SpawnedMonster->HasCaughtPlayer())
    {
        PlayerLost("Monster caught you!");
    }
}

void AMazeGameMode::PlayerWon()
{
    if (CurrentGameState == EGameState::Playing)
    {
        CurrentGameState = EGameState::Won;
        
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
        
        if (SpawnedMonster)
        {
            SpawnedMonster->StopChasing();
        }
        
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT("=== ✓✓✓ VICTORY! PLAYER ESCAPED! ✓✓✓ ==="));
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        
        ShowGameOverScreen(true);
    }
}

void AMazeGameMode::PlayerLost(const FString& Reason)
{
    if (CurrentGameState == EGameState::Playing)
    {
        CurrentGameState = EGameState::Lost;
        
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
    // Show loading screen immediately (reduced to 2s for better UX)
    ShowLoadingScreen("Returning to Menu...", 2.0f);
    
    UE_LOG(LogTemp, Error, TEXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
    UE_LOG(LogTemp, Error, TEXT("!!! RESTART GAME FUNCTION CALLED !!!"));
    UE_LOG(LogTemp, Error, TEXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ========== RESTARTING GAME =========="));
    
    // CRITICAL FIX: Destroy flashlight to prevent memory leak
    if (PlayerFlashlight)
    {
        PlayerFlashlight->DestroyComponent();
        PlayerFlashlight = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Flashlight destroyed to prevent memory leak"));
    }
    
    // Unpause first
    UGameplayStatics::SetGamePaused(GetWorld(), false);
    
    // Remove all UI widgets
    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }
    
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
    
    // Reset input mode
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Input mode reset to game only"));
    }
    
    // Reload level
    FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Reloading level: %s"), *CurrentLevel);
    UGameplayStatics::OpenLevel(this, FName(*CurrentLevel), false);
}

// ==================== OPTIONS MENU FUNCTIONS ====================

void AMazeGameMode::ShowOptionsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[Options] Opening Options menu"));
    
    // Hide main menu but keep the preview active
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        UE_LOG(LogTemp, Warning, TEXT("[Options] Main menu hidden"));
    }
    
    // Create and show options menu
    if (OptionsMenuWidgetClass)
    {
        if (!OptionsMenuWidget)
        {
            OptionsMenuWidget = CreateWidget<UUserWidget>(GetWorld(), OptionsMenuWidgetClass);
        }
        
        if (OptionsMenuWidget)
        {
            OptionsMenuWidget->AddToViewport(200); // High Z-order
            
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
            
            UE_LOG(LogTemp, Warning, TEXT("✓ Options menu displayed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠ OptionsMenuWidgetClass not set!"));
    }
}

void AMazeGameMode::CloseOptionsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[Options] Closing Options menu"));
    
    if (OptionsMenuWidget)
    {
        OptionsMenuWidget->RemoveFromParent();
        OptionsMenuWidget = nullptr;
    }
    
    // Show main menu again
    ShowMainMenu();
    
    // Return to main menu input mode
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeUIOnly());
    }
}

// ==================== SETTINGS FUNCTIONS ====================

void AMazeGameMode::ShowSettingsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Opening Free2Play Mode menu"));
    
    // Hide main menu but keep the preview active
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Main menu hidden"));
    }
    
    // Create and show settings menu (Free2Play mode)
    if (SettingsMenuWidgetClass)
    {
        if (!SettingsMenuWidget)
        {
            SettingsMenuWidget = CreateWidget<UUserWidget>(GetWorld(), SettingsMenuWidgetClass);
        }
        
        if (SettingsMenuWidget)
        {
            SettingsMenuWidget->AddToViewport(200); // High Z-order
            
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
            
            UE_LOG(LogTemp, Warning, TEXT("✓ Free2Play Mode menu displayed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠ SettingsMenuWidgetClass not set!"));
    }
}

void AMazeGameMode::CloseSettingsMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[Free2Play] Closing Free2Play Mode menu"));
    
    if (SettingsMenuWidget)
    {
        SettingsMenuWidget->RemoveFromParent();
        SettingsMenuWidget = nullptr;
    }
}

void AMazeGameMode::SetMazeRows(int32 NewRows)
{
    CurrentMazeRows = FMath::Clamp(NewRows, 1, 30);
    UE_LOG(LogTemp, Log, TEXT("[Settings] Maze rows set to %d"), CurrentMazeRows);
}

void AMazeGameMode::SetMazeCols(int32 NewCols)
{
    CurrentMazeCols = FMath::Clamp(NewCols, 1, 30);
    UE_LOG(LogTemp, Log, TEXT("[Settings] Maze cols set to %d"), CurrentMazeCols);
}

void AMazeGameMode::SaveSettings()
{
    UE_LOG(LogTemp, Warning, TEXT("[Settings] Saving settings: %dx%d"), CurrentMazeRows, CurrentMazeCols);
    
    // Check if maze size changed
    bool bSizeChanged = false;
    if (MazeManager)
    {
        bSizeChanged = (MazeManager->Rows != CurrentMazeRows || MazeManager->Cols != CurrentMazeCols);
        UE_LOG(LogTemp, Warning, TEXT("[Settings] Current maze: %dx%d, New settings: %dx%d, Changed: %s"), 
               MazeManager->Rows, MazeManager->Cols, CurrentMazeRows, CurrentMazeCols, 
               bSizeChanged ? TEXT("YES") : TEXT("NO"));
    }
    
    // Create or update save game object
    UMazeGameSettings* SaveGameInstance = Cast<UMazeGameSettings>(
        UGameplayStatics::CreateSaveGameObject(UMazeGameSettings::StaticClass())
    );
    
    if (SaveGameInstance)
    {
        SaveGameInstance->MazeRows = CurrentMazeRows;
        SaveGameInstance->MazeCols = CurrentMazeCols;
        SaveGameInstance->MouseSensitivity = MouseSensitivity;
        SaveGameInstance->GameVolume = GameVolume;
        
        bool bSuccess = UGameplayStatics::SaveGameToSlot(
            SaveGameInstance,
            UMazeGameSettings::SaveSlotName,
            UMazeGameSettings::UserIndex
        );
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("✓ Settings saved successfully!"));
            
            // IMMEDIATE APPLY: Regenerate maze if size changed
            if (bSizeChanged && MazeManager)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Settings] ========================================"));
                UE_LOG(LogTemp, Warning, TEXT("[Settings] REGENERATING MAZE WITH NEW SIZE..."));
                UE_LOG(LogTemp, Warning, TEXT("[Settings] ========================================"));
                
                // Get old cell count for verification
                int32 OldCellCount = 0;
                for (const auto& Row : MazeManager->MazeGrid)
                {
                    OldCellCount += Row.Num();
                }
                
                // Apply new size and regenerate
                MazeManager->SetMazeSize(CurrentMazeRows, CurrentMazeCols);
                MazeManager->GenerateMazeImmediate();
                bMazeGenerated = true;
                
                // Verify regeneration
                int32 NewCellCount = 0;
                for (const auto& Row : MazeManager->MazeGrid)
                {
                    NewCellCount += Row.Num();
                }
                
                UE_LOG(LogTemp, Warning, TEXT("[Settings] ✓ Maze regenerated!"));
                UE_LOG(LogTemp, Warning, TEXT("[Settings]   Old cells: %d (%dx%d)"), OldCellCount, MazeManager->Rows, MazeManager->Cols);
                UE_LOG(LogTemp, Warning, TEXT("[Settings]   New cells: %d (%dx%d)"), NewCellCount, CurrentMazeRows, CurrentMazeCols);
                UE_LOG(LogTemp, Warning, TEXT("[Settings] ========================================"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Failed to save settings!"));
        }
    }
}

void AMazeGameMode::SetGameVolume(float Volume)
{
    GameVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    
    // Apply volume immediately
    if (UWorld* World = GetWorld())
    {
        FAudioDevice* AudioDevice = World->GetAudioDeviceRaw();
        if (AudioDevice)
        {
            AudioDevice->SetTransientPrimaryVolume(GameVolume);
        }
    }
}

void AMazeGameMode::LoadSettings()
{
    UE_LOG(LogTemp, Log, TEXT("[Settings] Loading settings..."));
    
    // Try to load existing save game
    UMazeGameSettings* LoadedGame = Cast<UMazeGameSettings>(
        UGameplayStatics::LoadGameFromSlot(
            UMazeGameSettings::SaveSlotName,
            UMazeGameSettings::UserIndex
        )
    );
    
    if (LoadedGame)
    {
        CurrentMazeRows = FMath::Clamp(LoadedGame->MazeRows, 1, 30);
        CurrentMazeCols = FMath::Clamp(LoadedGame->MazeCols, 1, 30);
        UE_LOG(LogTemp, Warning, TEXT("✓ Settings loaded: %dx%d"), CurrentMazeRows, CurrentMazeCols);
    }
    else
    {
        // No save file exists, use defaults
        CurrentMazeRows = 15;
        CurrentMazeCols = 15;
        UE_LOG(LogTemp, Warning, TEXT("⚠ No saved settings found, using defaults: 15x15"));
    }
}

void AMazeGameMode::ApplyMuddyEffect(float Duration, float ResolutionScale)
{
    if (bMuddyEffectActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] Effect already active - extending duration"));
        MuddyEffectTimer = FMath::Max(MuddyEffectTimer, Duration);  // Extend if new duration is longer
        return;  // CRITICAL FIX: Don't stack effects, just extend timer
    }
    
    // Store original resolution
    if (GEngine)
    {
        OriginalResolution = 100.0f;  // Always assume 100% at start
    }
    
    // Store original player speed and reduce it (ONLY if effect not already active)
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            OriginalPlayerSpeed = PlayerChar->GetCharacterMovement()->MaxWalkSpeed;
            float ReducedSpeed = OriginalPlayerSpeed * 0.5f;  // Reduce to 50% (half speed)
            PlayerChar->GetCharacterMovement()->MaxWalkSpeed = ReducedSpeed;
            
            UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] Player speed reduced: %.0f -> %.0f (50%%)"), 
                   OriginalPlayerSpeed, ReducedSpeed);
        }
    }
    
    // Apply muddy effect
    bMuddyEffectActive = true;
    MuddyEffectTimer = Duration;
    
    if (GEngine)
    {
        // CRITICAL FIX: Cap minimum resolution at 30% to prevent unplayable state
        float TargetResolution = FMath::Max(ResolutionScale * 100.0f, 30.0f);
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] Target resolution capped at %.0f%% (requested %.0f%%)"), 
               TargetResolution, ResolutionScale * 100.0f);
        GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.ScreenPercentage %f"), TargetResolution));
        
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect] ⚠️ Applied muddy effect!"));
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect]   Resolution: %.0f%% -> %.0f%%"), OriginalResolution, TargetResolution);
        UE_LOG(LogTemp, Warning, TEXT("[MuddyEffect]   Duration: %.1f seconds"), Duration);
    }
}

// ==================== MAZE TRAP SYSTEM ====================

AMazeCell* AMazeGameMode::GetPlayerCurrentCell()
{
    if (!Player || !MazeManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeTrap] Cannot get player cell - missing Player or MazeManager"));
        return nullptr;
    }
    
    FVector PlayerLocation = Player->GetActorLocation();
    
    // Iterate through all cells to find which one contains the player
    for (int32 Row = 0; Row < MazeManager->Rows; Row++)
    {
        for (int32 Col = 0; Col < MazeManager->Cols; Col++)
        {
            AMazeCell* Cell = MazeManager->GetCell(Row, Col);
            if (Cell)
            {
                FVector CellLocation = Cell->GetActorLocation();
                float CellSize = MazeManager->CellSize;
                
                // Check if player is within this cell's bounds
                float HalfSize = CellSize / 2.0f;
                if (FMath::Abs(PlayerLocation.X - CellLocation.X) <= HalfSize &&
                    FMath::Abs(PlayerLocation.Y - CellLocation.Y) <= HalfSize)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Player is in cell (%d, %d)"), Row, Col);
                    return Cell;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Error, TEXT("[MazeTrap] Could not find player's current cell!"));
    return nullptr;
}

void AMazeGameMode::TrapPlayer(AMazeCell* Cell)
{
    if (!Cell || !Player)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeTrap] Cannot trap player - missing Cell or Player"));
        return;
    }
    
    // Teleport player to center of cell
    FVector CellCenter = Cell->GetActorLocation();
    CellCenter.Z = Player->GetActorLocation().Z;  // Keep player's height
    Player->SetActorLocation(CellCenter);
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Player teleported to cell center: %s"), *CellCenter.ToString());
    
    // Show all 4 walls to trap player
    Cell->ShowAllWalls();
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] All walls shown - player is trapped!"));
    
    // Disable player movement
    ACharacter* PlayerChar = Cast<ACharacter>(Player);
    if (PlayerChar && PlayerChar->GetCharacterMovement())
    {
        PlayerChar->GetCharacterMovement()->DisableMovement();
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Player movement disabled"));
    }
    
    // Show on-screen message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
            TEXT("⚠️ TRAPPED! Maze is regenerating..."));
    }
    
    // Play regeneration sound
    if (MazeRegenerationSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), MazeRegenerationSound);
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Playing maze regeneration sound"));
    }
    
    bPlayerTrapped = true;
    TrapDuration = 5.0f;  // Reset trap duration
}

void AMazeGameMode::ReleaseTrap()
{
    if (!TrappedCell)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeTrap] Cannot release trap - no trapped cell stored"));
        bPlayerTrapped = false;
        return;
    }
    
    // Hide all 4 walls to free player
    TrappedCell->HideAllWalls();
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] All walls hidden - player is free!"));
    
    // Re-enable player movement
    if (Player)
    {
        ACharacter* PlayerChar = Cast<ACharacter>(Player);
        if (PlayerChar && PlayerChar->GetCharacterMovement())
        {
            PlayerChar->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Player movement re-enabled"));
        }
    }
    
    // Show on-screen message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
            TEXT("✓ Released! Find the new escape!"));
    }
    
    // Path will automatically update when player moves if they have the star
    // No need to manually call HighlightPath here
    
    bPlayerTrapped = false;
    TrappedCell = nullptr;
}

void AMazeGameMode::TriggerMazeTrap()
{
    if (bMazeTrapTriggered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Trap already triggered this game"));
        return;
    }
    
    if (!MazeManager || !Player)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeTrap] Cannot trigger trap - missing MazeManager or Player"));
        return;
    }
    
    bMazeTrapTriggered = true;
    
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] ⚠️ MAZE TRAP TRIGGERED!"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    
    // Step 1: Get player's current cell
    AMazeCell* PlayerCell = GetPlayerCurrentCell();
    if (!PlayerCell)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeTrap] Failed to get player cell - aborting trap"));
        return;
    }
    
    TrappedCell = PlayerCell;
    int32 PlayerRow = TrappedCell->Row;
    int32 PlayerCol = TrappedCell->Col;
    
    // Step 2: Teleport player to cell center (but don't show walls yet)
    FVector CellCenter = PlayerCell->GetActorLocation();
    CellCenter.Z = Player->GetActorLocation().Z;
    Player->SetActorLocation(CellCenter);
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Player teleported to cell center: %s"), *CellCenter.ToString());
    
    // Disable player movement
    ACharacter* PlayerChar = Cast<ACharacter>(Player);
    if (PlayerChar && PlayerChar->GetCharacterMovement())
    {
        PlayerChar->GetCharacterMovement()->DisableMovement();
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Player movement disabled"));
    }
    
    // Show on-screen message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
            TEXT("⚠️ TRAPPED! Maze is regenerating..."));
    }
    
    // Play regeneration sound
    if (MazeRegenerationSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), MazeRegenerationSound);
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Playing maze regeneration sound"));
    }
    
    bPlayerTrapped = true;
    TrapDuration = 5.0f;
    
    // Step 3: Despawn monster
    if (SpawnedMonster)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Despawning monster"));
        SpawnedMonster->Destroy();
        SpawnedMonster = nullptr;
    }
    
    // Step 4: Despawn golden star
    if (SpawnedStar)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Despawning golden star"));
        SpawnedStar->Destroy();
        SpawnedStar = nullptr;
    }
    
    // Step 5: Regenerate maze (preserve trapped cell)
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Regenerating maze while preserving trapped cell..."));
    MazeManager->GenerateMaze(TrappedCell);  // Pass trapped cell to preserve it
    
    // Step 6: NOW show all walls of trapped cell (after regeneration)
    // The cell was preserved, so now we just make its walls visible
    TrappedCell->ShowAllWalls();
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Trapped cell walls shown - player is now visibly trapped!"));
    
    // Step 7: Respawn monster at least 4 cells away from player
    TArray<AMazeCell*> ValidMonsterCells;
    for (int32 Row = 0; Row < MazeManager->Rows; Row++)
    {
        for (int32 Col = 0; Col < MazeManager->Cols; Col++)
        {
            int32 Distance = FMath::Abs(Row - PlayerRow) + FMath::Abs(Col - PlayerCol);
            if (Distance >= 4)
            {
                AMazeCell* Cell = MazeManager->GetCell(Row, Col);
                if (Cell)
                {
                    ValidMonsterCells.Add(Cell);
                }
            }
        }
    }
    
    if (ValidMonsterCells.Num() > 0 && MonsterClass)
    {
        int32 RandomIndex = FMath::RandRange(0, ValidMonsterCells.Num() - 1);
        AMazeCell* MonsterCell = ValidMonsterCells[RandomIndex];
        
        FVector MonsterSpawnLocation = MonsterCell->GetActorLocation();
        MonsterSpawnLocation.Z += 100.0f;
        
        SpawnedMonster = GetWorld()->SpawnActor<AMonsterAI>(
            MonsterClass,
            MonsterSpawnLocation,
            FRotator::ZeroRotator
        );
        
        if (SpawnedMonster)
        {
            // Manually initialize monster AI since it was spawned mid-game
            SpawnedMonster->Initialize();
            
            UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Monster respawned at (%d, %d) and manually initialized"),
                   MonsterCell->Row, MonsterCell->Col);
        }
    }
    
    // Step 7: Respawn golden star at least 4 cells away from player
    TArray<AMazeCell*> ValidStarCells;
    for (int32 Row = 0; Row < MazeManager->Rows; Row++)
    {
        for (int32 Col = 0; Col < MazeManager->Cols; Col++)
        {
            int32 Distance = FMath::Abs(Row - PlayerRow) + FMath::Abs(Col - PlayerCol);
            if (Distance >= 4)
            {
                AMazeCell* Cell = MazeManager->GetCell(Row, Col);
                if (Cell && Cell != MazeManager->EscapeCell)
                {
                    ValidStarCells.Add(Cell);
                }
            }
        }
    }
    
    if (ValidStarCells.Num() > 0 && GoldenStarClass)
    {
        int32 RandomIndex = FMath::RandRange(0, ValidStarCells.Num() - 1);
        AMazeCell* StarCell = ValidStarCells[RandomIndex];
        
        FVector StarSpawnLocation = StarCell->GetActorLocation();
        StarSpawnLocation.Z += 50.0f;
        
        SpawnedStar = GetWorld()->SpawnActor<AGoldenStar>(
            GoldenStarClass,
            StarSpawnLocation,
            FRotator::ZeroRotator
        );
        
        if (SpawnedStar)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Golden star respawned at (%d, %d)"),
                   StarCell->Row, StarCell->Col);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeTrap] Trap sequence complete - player will be released in 5 seconds"));
}

void AMazeGameMode::ShowLoadingScreen(const FString& Message, float MinDuration)
{
    if (LoadingScreenWidgetClass)
    {
        if (!LoadingScreenWidget)
        {
            LoadingScreenWidget = CreateWidget<UUserWidget>(GetWorld(), LoadingScreenWidgetClass);
        }
        
        if (LoadingScreenWidget)
        {
            LoadingScreenWidget->AddToViewport(1000);
            
            ULoadingScreenWidget* LoadingScreen = Cast<ULoadingScreenWidget>(LoadingScreenWidget);
            if (LoadingScreen)
            {
                LoadingScreen->SetInstructions(Message);
            }
            
            // CRITICAL FIX: Use GetWorldTimerManager with a lambda to ensure hiding works
            FTimerHandle LoadingTimer;
            GetWorldTimerManager().SetTimer(LoadingTimer, [this]()
            {
                HideLoadingScreen();
            }, MinDuration, false);
            
            UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] ✓ Showing for %.1fs: %s"), MinDuration, *Message);
        }
    }
}

void AMazeGameMode::HideLoadingScreen()
{
    if (LoadingScreenWidget && LoadingScreenWidget->IsInViewport())
    {
        LoadingScreenWidget->RemoveFromParent();
        UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] ✓ Hidden"));
    }
}
