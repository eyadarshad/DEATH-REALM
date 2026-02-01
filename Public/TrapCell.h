// TrapCell.h - Trap cell that captures player for 5 seconds
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TrapCell.generated.h"

UCLASS()
class MAZERUNNER_API ATrapCell : public AActor
{
    GENERATED_BODY()
    
public:    
    ATrapCell();

protected:
    virtual void BeginPlay() override;

public:    
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* TriggerBox;  // Floor-level collision trigger
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TrapMesh;  // Visual floor material
    
    // Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap Cell")
    float TrapDuration = 5.0f;  // 5 seconds
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap Cell")
    UMaterialInterface* TrapMaterial;  // Red/warning material
    
    // Cell reference
    UPROPERTY()
    class AMazeCell* AssociatedCell;
    
    // State
    bool bTrapActivated = false;
    
    // Initialize with cell reference
    void Initialize(AMazeCell* Cell, float CellSize);

private:
    // Overlap events
    UFUNCTION()
    void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                               bool bFromSweep, const FHitResult& SweepResult);
    
    // Trap activation
    void ActivateTrap(AActor* Player);
    void DragPlayerToCenter(AActor* Player);
    void ShowCellWalls();
    void ReleaseTrap();
    
    FTimerHandle ReleaseTimerHandle;
};
