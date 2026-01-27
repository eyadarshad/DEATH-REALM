// MonsterAI.cpp - FIXED WITH CORRECT INCLUDES
#include "MonsterAI.h"
#include "MazeManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "MazeManager.h"
#include "MazeCell.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PointLightComponent.h"
#include "Sound/SoundWave.h"

AMonsterAI::AMonsterAI()
{
    PrimaryActorTick.bCanEverTick = true;
    
    TargetPlayer = nullptr;
    CurrentWaypointIndex = 0;
    PathUpdateTimer = 0.0f;
    MazeManager = nullptr;
    bIsChasing = false;
    
    // Modern AI initialization
    SteeringUpdateTimer = 0.0f;
    LastPlayerPosition = FVector::ZeroVector;
    PlayerVelocity = FVector::ZeroVector;
    
    MoveSpeed = 300.0f;  // Reverted to original speed
    PathUpdateInterval = 1.0f;  // Reduced update frequency for better performance
    
    // CRITICAL FIX: Enable AI control
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
    
    // Set capsule size for monster
    GetCapsuleComponent()->InitCapsuleSize(50.0f, 100.0f);
    
    // Add skeletal mesh for animated monster
    USkeletalMeshComponent* MonsterMesh = GetMesh();
    if (MonsterMesh)
    {
        MonsterMesh->SetupAttachment(GetCapsuleComponent());
        MonsterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));
        MonsterMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Face forward
        MonsterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        
        // Load the Mixamo monster mesh
        static ConstructorHelpers::FObjectFinder<USkeletalMesh> MonsterMeshAsset(TEXT("/Game/Characters/Monster/Ch25_nonPBR"));
        if (MonsterMeshAsset.Succeeded())
        {
            MonsterMesh->SetSkeletalMesh(MonsterMeshAsset.Object);
            UE_LOG(LogTemp, Warning, TEXT("✅ Monster skeletal mesh loaded successfully!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to load monster mesh! Check path: /Game/Characters/Monster/Ch25_nonPBR"));
        }
        
        // Load and apply Animation Blueprint for walking/idle animations
        static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Characters/Monster/ABP_Monster"));
        if (AnimBP.Succeeded())
        {
            MonsterMesh->SetAnimInstanceClass(AnimBP.Class);
            UE_LOG(LogTemp, Warning, TEXT("✅ Monster Animation Blueprint loaded! Monster will now animate!"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ Animation Blueprint not found. Monster will slide without animation."));
        }
    }
    
    // Set movement speed
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
        GetCharacterMovement()->bOrientRotationToMovement = true; // Face movement direction
        GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // Fast turning
    }
}

void AMonsterAI::BeginPlay()
{
    Super::BeginPlay();
    
    // Find the maze manager
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMazeManager::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        MazeManager = Cast<AMazeManager>(FoundActors[0]);
        UE_LOG(LogTemp, Warning, TEXT("Monster found MazeManager"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Monster could not find MazeManager!"));
    }
    
    // Add a bright RED POINT LIGHT to the monster (works with or without Bloom!)
    UPointLightComponent* RedLight = NewObject<UPointLightComponent>(this, UPointLightComponent::StaticClass());
    if (RedLight)
    {
        RedLight->RegisterComponent();
        RedLight->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        RedLight->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f)); // Above monster
        
        // Set bright RED color
        RedLight->SetLightColor(FLinearColor(1.0f, 0.0f, 0.0f)); // Pure red
        RedLight->SetIntensity(2500.0f); // Bright but not excessive
        RedLight->SetAttenuationRadius(300.0f); // Moderate radius - visible from nearby
        RedLight->SetCastShadows(false); // No shadows for performance
        
        UE_LOG(LogTemp, Warning, TEXT("Monster: RED LIGHT added - you WILL see this!"));
    }
    
    // Start chasing
    bIsChasing = true;
    
    // Setup footstep audio component for 3D spatial sound
    if (FootstepSound)
    {
        // Enable looping on the sound itself
        if (USoundWave* SoundWave = Cast<USoundWave>(FootstepSound))
        {
            SoundWave->bLooping = true;
        }
        
        FootstepAudioComponent = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass());
        if (FootstepAudioComponent)
        {
            FootstepAudioComponent->RegisterComponent();
            FootstepAudioComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
            FootstepAudioComponent->SetSound(FootstepSound);
            FootstepAudioComponent->bAutoActivate = false;
            FootstepAudioComponent->SetVolumeMultiplier(0.5f);  // Start at medium volume
            
            // Enable 3D spatialization (sound comes from monster's direction)
            FootstepAudioComponent->bAllowSpatialization = true;
            FootstepAudioComponent->bIsUISound = false;
            
            // Start playing (will loop because we set bLooping on the sound wave)
            FootstepAudioComponent->Play();
            
            UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] Footstep audio component created and playing (LOOPING)"));
        }
    }
    
    // Growl sound removed - was causing it to play during maze regeneration
}

void AMonsterAI::Initialize()
{
    // Find the player
    TargetPlayer = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (TargetPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] Player target found during manual initialization"));
    }
    
    // Find the maze manager
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMazeManager::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        MazeManager = Cast<AMazeManager>(FoundActors[0]);
        UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] MazeManager found during manual initialization"));
    }
    
    // Reset pathfinding state
    CurrentPath.Empty();
    CurrentWaypointIndex = 0;
    PathUpdateTimer = 0.0f;
    bIsChasing = true;  // Enable chasing immediately after respawn
    
    UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] Manual initialization complete - chasing enabled"));
}

void AMonsterAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update footstep volume based on distance to player
    if (FootstepAudioComponent && TargetPlayer)
    {
        float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
        
        // Calculate volume based on distance (closer = louder)
        float VolumeMultiplier = 1.0f;
        if (Distance > FootstepMaxDistance)
        {
            VolumeMultiplier = FootstepMinVolume;
        }
        else
        {
            // Linear interpolation from max volume (close) to min volume (far)
            float DistanceRatio = Distance / FootstepMaxDistance;
            VolumeMultiplier = FMath::Lerp(FootstepMaxVolume, FootstepMinVolume, DistanceRatio);
        }
        
        FootstepAudioComponent->SetVolumeMultiplier(VolumeMultiplier);
    }
    
    if (bIsChasing && TargetPlayer && MazeManager)
    {
        // Update path periodically
        PathUpdateTimer += DeltaTime;
        if (PathUpdateTimer >= PathUpdateInterval)
        {
            PathUpdateTimer = 0.0f;
            UpdatePathToPlayer();
        }
        
        // Move along the path
        MoveAlongPath(DeltaTime);
    }
}

void AMonsterAI::StartChasing(AActor* Target)
{
    if (!Target || !MazeManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start chasing: invalid target or no maze manager"));
        return;
    }
    
    TargetPlayer = Target;
    bIsChasing = true;
    
    // Calculate initial path
    UpdatePathToPlayer();
    
    UE_LOG(LogTemp, Warning, TEXT("Monster started chasing player!"));
}

void AMonsterAI::StopChasing()
{
    bIsChasing = false;
    CurrentPath.Empty();
    CurrentWaypointIndex = 0;
    
    UE_LOG(LogTemp, Warning, TEXT("Monster stopped chasing"));
}

void AMonsterAI::UpdatePathToPlayer()
{
    if (!MazeManager || !TargetPlayer) return;
    
    // Get current cells
    AMazeCell* MonsterCell = GetCurrentCell();
    AMazeCell* PlayerCell = GetPlayerCell();
    
    if (!MonsterCell || !PlayerCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not determine cells for pathfinding"));
        return;
    }
    
    // Find new path using A* (faster and smoother than BFS)
    TArray<AMazeCell*> NewPath = MazeManager->FindPathAStar(MonsterCell, PlayerCell);
    
    if (NewPath.Num() > 0)
    {
        // CRITICAL FIX: Only reset waypoint index if path actually changed!
        // Check if the new path is different from current path
        bool bPathChanged = (CurrentPath.Num() != NewPath.Num());
        if (!bPathChanged && CurrentPath.Num() > 0)
        {
            // Check if first few waypoints are different
            for (int32 i = 0; i < FMath::Min(3, CurrentPath.Num()) && i < NewPath.Num(); i++)
            {
                if (CurrentPath[i] != NewPath[i])
                {
                    bPathChanged = true;
                    break;
                }
            }
        }
        
        CurrentPath = NewPath;
        
        // Only reset waypoint index if path significantly changed
        if (bPathChanged)
        {
            CurrentWaypointIndex = 0;
            UE_LOG(LogTemp, Warning, TEXT("🔄 Monster path changed! New path length: %d"), NewPath.Num());
        }
        else
        {
            // Keep current waypoint index, just update the path
            // Clamp to valid range
            CurrentWaypointIndex = FMath::Min(CurrentWaypointIndex, CurrentPath.Num() - 1);
            UE_LOG(LogTemp, Log, TEXT("✅ Monster path updated, continuing from waypoint %d"), CurrentWaypointIndex);
        }
        
        // Debug: Draw the path (DISABLED)
        /*
        #if WITH_EDITOR
        for (int32 i = 0; i < CurrentPath.Num() - 1; i++)
        {
            if (CurrentPath[i] && CurrentPath[i + 1])
            {
                FVector Start = CurrentPath[i]->GetActorLocation() + FVector(0, 0, 200);
                FVector End = CurrentPath[i + 1]->GetActorLocation() + FVector(0, 0, 200);
                DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, PathUpdateInterval, 0, 10.0f);
            }
        }
        #endif
        */
    }
}

void AMonsterAI::MoveAlongPath(float DeltaTime)
{
    if (CurrentPath.Num() == 0 || CurrentWaypointIndex >= CurrentPath.Num())
    {
        return;
    }
    
    // Update player velocity tracking (for prediction)
    if (TargetPlayer)
    {
        FVector CurrentPlayerPos = TargetPlayer->GetActorLocation();
        PlayerVelocity = (CurrentPlayerPos - LastPlayerPosition) / DeltaTime;
        LastPlayerPosition = CurrentPlayerPos;
    }
    
    // MODERN AI: Use steering behaviors WITH A* path (not direct pursuit)
    if (bUseModernAI)
    {
        SteeringUpdateTimer += DeltaTime;
        
        // Get current waypoint from A* path
        AMazeCell* TargetCell = CurrentPath[CurrentWaypointIndex];
        if (!TargetCell) 
        {
            CurrentWaypointIndex++;
            return;
        }
        
        FVector WaypointLocation = TargetCell->GetActorLocation();
        FVector CurrentLocation = GetActorLocation();
        WaypointLocation.Z = CurrentLocation.Z = 200.0f;
        
        // Check if we reached this waypoint
        float DistanceToWaypoint = FVector::Dist2D(CurrentLocation, WaypointLocation);
        if (DistanceToWaypoint < 100.0f)
        {
            CurrentWaypointIndex++;
            UE_LOG(LogTemp, Log, TEXT("Monster reached waypoint %d"), CurrentWaypointIndex);
        }
        
        // Update steering at reduced frequency (performance optimization)
        if (SteeringUpdateTimer >= SteeringUpdateInterval)
        {
            SteeringUpdateTimer = 0.0f;
            
            // CRITICAL FIX: Steer toward WAYPOINT, not directly to player
            // This respects walls while still being smooth
            FVector SteeringForce = CalculateSteeringForce(WaypointLocation);
            
            // Add obstacle avoidance
            FVector AvoidanceForce = AvoidObstacles();
            
            // Blend steering forces (avoidance has priority)
            FVector FinalSteering = SteeringForce + (AvoidanceForce * 1.5f);
            FinalSteering.Normalize();
            
            // Apply movement
            if (!FinalSteering.IsNearlyZero())
            {
                AddMovementInput(FinalSteering, 1.0f);
                
                // Smooth rotation toward movement direction
                FRotator NewRotation = FinalSteering.Rotation();
                SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 10.0f));
            }
        }
        else
        {
            // Continue with last steering direction (smooth between updates)
            FVector CurrentForward = GetActorForwardVector();
            AddMovementInput(CurrentForward, 1.0f);
        }
    }
    else
    {
        // LEGACY AI: Original waypoint following (fallback for performance)
        AMazeCell* TargetCell = CurrentPath[CurrentWaypointIndex];
        if (!TargetCell) 
        {
            CurrentWaypointIndex++;
            return;
        }
        
        FVector TargetLocation = TargetCell->GetActorLocation();
        FVector CurrentLocation = GetActorLocation();
        
        // Adjust heights
        TargetLocation.Z = CurrentLocation.Z = 200.0f;
        
        FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
        float DistanceToWaypoint = FVector::Dist2D(CurrentLocation, TargetLocation);
        
        // Waypoint reached threshold
        if (DistanceToWaypoint < 100.0f)
        {
            CurrentWaypointIndex++;
        }
        
        // Move towards waypoint
        if (DistanceToWaypoint > 10.0f)
        {
            AddMovementInput(Direction, 1.0f);
        }
        
        // Face movement direction
        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = Direction.Rotation();
            SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 20.0f));
        }
    }
}

AMazeCell* AMonsterAI::GetCurrentCell() const
{
    if (!MazeManager) return nullptr;
    
    FVector Location = GetActorLocation();
    float CellSize = MazeManager->CellSize;
    
    // Convert to grid coordinates
    int32 Row = FMath::RoundToInt(Location.X / CellSize);
    int32 Col = FMath::RoundToInt(Location.Y / CellSize);
    
    return MazeManager->GetCell(Row, Col);
}

AMazeCell* AMonsterAI::GetPlayerCell() const
{
    if (!MazeManager || !TargetPlayer) return nullptr;
    
    FVector Location = TargetPlayer->GetActorLocation();
    float CellSize = MazeManager->CellSize;
    
    // Convert to grid coordinates
    int32 Row = FMath::RoundToInt(Location.X / CellSize);
    int32 Col = FMath::RoundToInt(Location.Y / CellSize);
    
    return MazeManager->GetCell(Row, Col);
}

bool AMonsterAI::HasCaughtPlayer() const
{
    if (!TargetPlayer) return false;
    
    float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
    // FIXED: Increased from 200 to 250 for more reliable detection
    return Distance < 250.0f; // Catch distance (2.5 meters)
}

// ==================== MODERN AI FUNCTIONS ====================

FVector AMonsterAI::CalculatePursuitTarget()
{
    if (!TargetPlayer)
    {
        return GetActorLocation();
    }
    
    FVector PlayerPos = TargetPlayer->GetActorLocation();
    FVector MonsterPos = GetActorLocation();
    
    // Calculate distance to player
    float DistanceToPlayer = FVector::Dist(PlayerPos, MonsterPos);
    
    // Adjust prediction time based on distance (closer = less prediction)
    float AdjustedPredictionTime = FMath::Clamp(DistanceToPlayer / 1000.0f, 0.2f, PredictionTime);
    
    // Predict where player will be
    FVector PredictedPos = PlayerPos + (PlayerVelocity * AdjustedPredictionTime);
    
    // Clamp to maze bounds (prevent predicting outside maze)
    if (MazeManager)
    {
        float MaxX = MazeManager->Rows * MazeManager->CellSize;
        float MaxY = MazeManager->Cols * MazeManager->CellSize;
        
        PredictedPos.X = FMath::Clamp(PredictedPos.X, 0.0f, MaxX);
        PredictedPos.Y = FMath::Clamp(PredictedPos.Y, 0.0f, MaxY);
    }
    
    return PredictedPos;
}

FVector AMonsterAI::CalculateSteeringForce(const FVector& TargetPosition)
{
    FVector MonsterPos = GetActorLocation();
    FVector DesiredVelocity = (TargetPosition - MonsterPos).GetSafeNormal();
    
    // Steering force = desired direction
    return DesiredVelocity;
}

FVector AMonsterAI::AvoidObstacles()
{
    FVector AvoidanceForce = FVector::ZeroVector;
    FVector MonsterPos = GetActorLocation();
    FVector Forward = GetActorForwardVector();
    
    // Cast 3 rays: forward, left-forward, right-forward
    TArray<FVector> RayDirections;
    RayDirections.Add(Forward);
    RayDirections.Add(Forward.RotateAngleAxis(30.0f, FVector::UpVector));
    RayDirections.Add(Forward.RotateAngleAxis(-30.0f, FVector::UpVector));
    
    int32 HitCount = 0;
    
    for (const FVector& RayDir : RayDirections)
    {
        FVector RayStart = MonsterPos;
        FVector RayEnd = RayStart + (RayDir * AvoidanceRadius);
        
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        if (TargetPlayer)
        {
            QueryParams.AddIgnoredActor(TargetPlayer);
        }
        
        // Check for walls
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            RayStart,
            RayEnd,
            ECC_Visibility,
            QueryParams
        );
        
        if (bHit)
        {
            // Steer away from obstacle
            FVector AwayFromWall = (MonsterPos - HitResult.ImpactPoint).GetSafeNormal();
            AvoidanceForce += AwayFromWall;
            HitCount++;
        }
    }
    
    // Normalize if we hit multiple obstacles
    if (HitCount > 0)
    {
        AvoidanceForce /= HitCount;
        AvoidanceForce.Normalize();
    }
    
    return AvoidanceForce;
}

