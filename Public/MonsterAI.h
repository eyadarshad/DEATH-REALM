// MonsterAI.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/AudioComponent.h"
#include "MonsterAI.generated.h"

UCLASS()
class MAZERUNNER_API AMonsterAI : public ACharacter
{
    GENERATED_BODY()
    
public:
    AMonsterAI();
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 600.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI")
    bool bStopWhenReachesPlayer = true;
    
    // ==================== AUDIO ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* GrowlSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* FootstepSound;  // Looping footstep sound
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float FootstepMaxDistance = 3000.0f;  // Max distance to hear footsteps
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float FootstepMinVolume = 0.0f;  // Volume when far away
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float FootstepMaxVolume = 1.0f;  // Volume when close

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float PathUpdateInterval = 1.0f;
    
    // ==================== MODERN AI NAVIGATION ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modern AI")
    bool bUseModernAI = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modern AI", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float PredictionTime = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modern AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SteeringUpdateInterval = 0.1f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modern AI")
    float AvoidanceRadius = 200.0f;
    
    // Functions
    UFUNCTION(BlueprintCallable, Category = "AI")
    void StartChasing(AActor* Target);
    
    UFUNCTION(BlueprintCallable, Category = "AI")
    void StopChasing();
    
    UFUNCTION(BlueprintPure, Category = "AI")
    bool HasCaughtPlayer() const;
    
    UFUNCTION(BlueprintPure, Category = "AI")
    bool IsChasing() const { return bIsChasing; }
    
    // Manual initialization for respawning
    UFUNCTION(BlueprintCallable, Category = "AI")
    void Initialize();
    
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
private:
    // References
    UPROPERTY()
    AActor* TargetPlayer;
    
    UPROPERTY()
    class AMazeManager* MazeManager;
    
    UPROPERTY()
    class UAudioComponent* FootstepAudioComponent;  // For looping footstep sound
    
    // Pathfinding
    TArray<class AMazeCell*> CurrentPath;
    int32 CurrentWaypointIndex;
    float PathUpdateTimer;
    bool bIsChasing;
    
    // Modern AI state
    float SteeringUpdateTimer;
    FVector LastPlayerPosition;
    FVector PlayerVelocity;
    
    // Internal functions
    void UpdatePathToPlayer();
    void MoveAlongPath(float DeltaTime);
    class AMazeCell* GetCurrentCell() const;
    class AMazeCell* GetPlayerCell() const;
    
    // Modern AI functions
    FVector CalculatePursuitTarget();
    FVector CalculateSteeringForce(const FVector& TargetPosition);
    FVector AvoidObstacles();
};
