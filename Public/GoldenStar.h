// GoldenStar.h
// Collectible golden star that reveals the shortest path to exit using BFS

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GoldenStar.generated.h"

UCLASS()
class MAZERUNNER_API AGoldenStar : public AActor
{
    GENERATED_BODY()
    
public:    
    AGoldenStar();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void Tick(float DeltaTime) override;

    // ==================== COMPONENTS ====================
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Golden Star")
    UStaticMeshComponent* StarMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Golden Star")
    USphereComponent* CollisionSphere;

    // ==================== STAR BEHAVIOR ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golden Star")
    float RotationSpeed = 90.0f; // Degrees per second
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golden Star")
    float BobSpeed = 2.0f; // How fast it bobs up and down
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golden Star")
    float BobHeight = 30.0f; // How high it bobs
    
    // ==================== AUDIO ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* PickupSound;

    // ==================== COLLECTION ====================
    
    // Called when player overlaps with star
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                       bool bFromSweep, const FHitResult& SweepResult);
    
    // Reveal path to exit
    UFUNCTION(BlueprintCallable, Category = "Golden Star")
    void RevealPathToExit();

private:
    // Timer for bobbing animation
    float BobTimer;
    
    // Initial Z position
    float InitialZ;
    
    // Reference to maze manager
    UPROPERTY()
    class AMazeManager* MazeManager;
};
