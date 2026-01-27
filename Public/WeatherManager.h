#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "WeatherManager.generated.h"

UCLASS()
class MAZERUNNER_API AWeatherManager : public AActor
{
    GENERATED_BODY()
    
public:    
    AWeatherManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

public:    
    // Rain particle system
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Rain")
    class UNiagaraSystem* RainParticleSystem;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Rain")
    UNiagaraComponent* RainParticles;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Rain", meta = (ClampMin = "100", ClampMax = "5000"))
    float RainIntensity;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Rain", meta = (ClampMin = "500", ClampMax = "10000"))
    float RainAreaRadius;
    
    // Follow player option
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Rain")
    bool bFollowPlayer;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Rain", meta = (EditCondition = "bFollowPlayer"))
    float FollowHeight;
    
    // Lightning flash light
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Lightning")
    UPointLightComponent* LightningLight;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float MinLightningInterval;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float MaxLightningInterval;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning", meta = (ClampMin = "1000", ClampMax = "50000"))
    float MinLightningIntensity;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning", meta = (ClampMin = "1000", ClampMax = "50000"))
    float MaxLightningIntensity;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning")
    FLinearColor LightningColor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning", meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float LightningFlashDuration;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Lightning", meta = (ClampMin = "500", ClampMax = "10000"))
    float LightningAttenuationRadius;
    
    // Audio components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Audio")
    UAudioComponent* RainAmbientSound;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Audio")
    UAudioComponent* ThunderSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
    class USoundBase* RainLoopSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
    class USoundBase* ThunderSoundEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RainVolume;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ThunderVolume;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MinThunderDelay;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MaxThunderDelay;
    
    // Control functions
    UFUNCTION(BlueprintCallable, Category = "Weather")
    void StartWeather();
    
    UFUNCTION(BlueprintCallable, Category = "Weather")
    void StopWeather();
    
    UFUNCTION(BlueprintCallable, Category = "Weather")
    void TriggerLightning();

private:
    FTimerHandle LightningTimerHandle;
    FTimerHandle LightningFadeHandle;
    FTimerHandle ThunderDelayHandle;
    
    bool bWeatherActive;
    APawn* CachedPlayerPawn;
    
    void ScheduleNextLightning();
    void FadeLightningOut();
    void PlayThunderSound();
    void UpdateRainPosition();
    APawn* GetPlayerPawn();
};
