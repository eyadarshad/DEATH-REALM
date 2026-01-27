#include "WeatherManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"

AWeatherManager::AWeatherManager()
{
    PrimaryActorTick.bCanEverTick = true;
    
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    
    // Create rain particle component
    RainParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RainParticles"));
    RainParticles->SetupAttachment(RootComponent);
    RainParticles->bAutoActivate = false;
    
    // Create lightning light
    LightningLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightningLight"));
    LightningLight->SetupAttachment(RootComponent);
    LightningLight->SetIntensity(0.0f);
    LightningLight->SetLightColor(FLinearColor(0.53f, 0.6f, 1.0f));
    LightningLight->SetAttenuationRadius(3000.0f);
    LightningLight->SetCastShadows(false);
    LightningLight->bUseInverseSquaredFalloff = false;
    
    // Create audio components
    RainAmbientSound = CreateDefaultSubobject<UAudioComponent>(TEXT("RainAmbientSound"));
    RainAmbientSound->SetupAttachment(RootComponent);
    RainAmbientSound->bAutoActivate = false;
    
    ThunderSound = CreateDefaultSubobject<UAudioComponent>(TEXT("ThunderSound"));
    ThunderSound->SetupAttachment(RootComponent);
    ThunderSound->bAutoActivate = false;
    
    // Default values
    RainIntensity = 1000.0f;
    RainAreaRadius = 3000.0f;
    bFollowPlayer = true;
    FollowHeight = 2000.0f;
    MinLightningInterval = 5.0f;
    MaxLightningInterval = 15.0f;
    MinLightningIntensity = 3000.0f;
    MaxLightningIntensity = 8000.0f;
    LightningColor = FLinearColor(0.53f, 0.6f, 1.0f);
    LightningFlashDuration = 0.15f;
    LightningAttenuationRadius = 3000.0f;
    RainVolume = 0.3f;
    ThunderVolume = 0.5f;
    MinThunderDelay = 0.3f;
    MaxThunderDelay = 1.5f;
    
    bWeatherActive = false;
    CachedPlayerPawn = nullptr;
}

void AWeatherManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache player reference
    CachedPlayerPawn = GetPlayerPawn();
    
    // Auto-start weather
    StartWeather();
}

void AWeatherManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(LightningTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(LightningFadeHandle);
        GetWorld()->GetTimerManager().ClearTimer(ThunderDelayHandle);
    }
}

void AWeatherManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bWeatherActive && bFollowPlayer)
    {
        UpdateRainPosition();
    }
}

void AWeatherManager::StartWeather()
{
    if (bWeatherActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] Weather already active"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] Starting dark weather system..."));
    
    // Start rain particles
    if (RainParticleSystem && RainParticles)
    {
        RainParticles->SetAsset(RainParticleSystem);
        
        // Set Niagara parameters
        RainParticles->SetFloatParameter(FName("SpawnRate"), RainIntensity);
        RainParticles->SetFloatParameter(FName("SpawnRadius"), RainAreaRadius);
        
        RainParticles->Activate();
        UE_LOG(LogTemp, Log, TEXT("[WeatherManager] Rain particles activated"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] Rain particle system not set"));
    }
    
    // Start ambient rain sound
    if (RainLoopSound && RainAmbientSound)
    {
        RainAmbientSound->SetSound(RainLoopSound);
        RainAmbientSound->SetVolumeMultiplier(RainVolume);
        RainAmbientSound->Play();
        UE_LOG(LogTemp, Log, TEXT("[WeatherManager] Rain sound started"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] Rain sound not set"));
    }
    
    // Configure lightning
    LightningLight->SetIntensity(0.0f);
    LightningLight->SetLightColor(LightningColor);
    LightningLight->SetAttenuationRadius(LightningAttenuationRadius);
    
    bWeatherActive = true;
    
    // Schedule first lightning
    ScheduleNextLightning();
    
    UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] Weather active - lightning every %.1f-%.1fs"), 
           MinLightningInterval, MaxLightningInterval);
}

void AWeatherManager::StopWeather()
{
    if (!bWeatherActive)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] Stopping weather..."));
    
    if (RainParticles)
    {
        RainParticles->Deactivate();
    }
    
    if (RainAmbientSound)
    {
        RainAmbientSound->Stop();
    }
    
    LightningLight->SetIntensity(0.0f);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(LightningTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(LightningFadeHandle);
        GetWorld()->GetTimerManager().ClearTimer(ThunderDelayHandle);
    }
    
    bWeatherActive = false;
}

void AWeatherManager::TriggerLightning()
{
    if (!bWeatherActive || !GetWorld())
    {
        return;
    }
    
    // Randomize intensity for variety
    float Intensity = FMath::RandRange(MinLightningIntensity, MaxLightningIntensity);
    LightningLight->SetIntensity(Intensity);
    
    // Schedule fade out
    GetWorld()->GetTimerManager().SetTimer(
        LightningFadeHandle,
        this,
        &AWeatherManager::FadeLightningOut,
        LightningFlashDuration,
        false
    );
    
    // Schedule thunder with realistic delay
    float ThunderDelay = FMath::RandRange(MinThunderDelay, MaxThunderDelay);
    GetWorld()->GetTimerManager().SetTimer(
        ThunderDelayHandle,
        this,
        &AWeatherManager::PlayThunderSound,
        ThunderDelay,
        false
    );
}

void AWeatherManager::ScheduleNextLightning()
{
    if (!bWeatherActive || !GetWorld())
    {
        return;
    }
    
    float NextInterval = FMath::RandRange(MinLightningInterval, MaxLightningInterval);
    
    GetWorld()->GetTimerManager().SetTimer(
        LightningTimerHandle,
        [this]()
        {
            TriggerLightning();
            ScheduleNextLightning();
        },
        NextInterval,
        false
    );
}

void AWeatherManager::FadeLightningOut()
{
    LightningLight->SetIntensity(0.0f);
}

void AWeatherManager::PlayThunderSound()
{
    if (ThunderSoundEffect && ThunderSound)
    {
        ThunderSound->SetSound(ThunderSoundEffect);
        ThunderSound->SetVolumeMultiplier(ThunderVolume);
        ThunderSound->Play();
    }
}

void AWeatherManager::UpdateRainPosition()
{
    if (!CachedPlayerPawn)
    {
        CachedPlayerPawn = GetPlayerPawn();
    }
    
    if (CachedPlayerPawn)
    {
        FVector PlayerLocation = CachedPlayerPawn->GetActorLocation();
        FVector NewLocation = FVector(PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z + FollowHeight);
        SetActorLocation(NewLocation);
    }
}

APawn* AWeatherManager::GetPlayerPawn()
{
    if (GetWorld())
    {
        return UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }
    return nullptr;
}
