// SafeZoneCell.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "SafeZoneCell.generated.h"

UCLASS()
class MAZERUNNER_API ASafeZoneCell : public AActor
{
    GENERATED_BODY()
    
public:    
    ASafeZoneCell();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void Tick(float DeltaTime) override;
    
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GlowMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPointLightComponent* PurpleLight;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* TriggerBox;
    
    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Safe Zone")
    bool bPlayerInside;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Safe Zone")
    bool bIsActive;
    
    // Visual settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
    float PulseSpeed;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
    float MinIntensity;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
    float MaxIntensity;
    
    // Functions
    UFUNCTION(BlueprintCallable, Category = "Safe Zone")
    void ActivateSafeZone();
    
    UFUNCTION(BlueprintCallable, Category = "Safe Zone")
    void DeactivateSafeZone();
    
    UFUNCTION(BlueprintCallable, Category = "Safe Zone")
    bool IsPlayerInside() const { return bPlayerInside; }
    
private:
    float PulseTimer;
    
    UFUNCTION()
    void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                               bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
