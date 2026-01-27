// MuddyPatch.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MuddyPatch.generated.h"

UCLASS()
class MAZERUNNER_API AMuddyPatch : public AActor
{
    GENERATED_BODY()
    
public:    
    AMuddyPatch();

protected:
    virtual void BeginPlay() override;

public:    
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* TriggerSphere;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MuddyMesh;
    
    // Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muddy Patch")
    float EffectDuration = 10.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muddy Patch")
    float ResolutionScale = 0.1f;  // 10% resolution
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muddy Patch")
    UMaterialInterface* MuddyMaterial;  // Custom muddy material
    
private:
    // Overlap events
    UFUNCTION()
    void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                               bool bFromSweep, const FHitResult& SweepResult);
};
