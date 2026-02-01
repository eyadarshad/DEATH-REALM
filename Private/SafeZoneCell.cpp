// SafeZoneCell.cpp
#include "SafeZoneCell.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ASafeZoneCell::ASafeZoneCell()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Create root
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = RootComp;
    
    // Create glowing floor mesh
    GlowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlowMesh"));
    GlowMesh->SetupAttachment(RootComponent);
    GlowMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));  // Slightly above ground
    GlowMesh->SetRelativeScale3D(FVector(3.8f, 3.8f, 0.1f));  // Flat disc
    GlowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Load cylinder mesh for flat disc appearance
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        GlowMesh->SetStaticMesh(CylinderMesh.Object);
    }
    
    // Create purple point light
    PurpleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PurpleLight"));
    PurpleLight->SetupAttachment(RootComponent);
    PurpleLight->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    PurpleLight->SetIntensity(5000.0f);
    PurpleLight->SetAttenuationRadius(600.0f);
    PurpleLight->SetLightColor(FLinearColor(0.6f, 0.0f, 1.0f));  // Purple
    PurpleLight->SetCastShadows(false);
    
    // Create trigger box
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootComponent);
    TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));  // Covers cell area
    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerBox->SetGenerateOverlapEvents(true);
    
    // Initialize state
    bPlayerInside = false;
    bIsActive = false;
    PulseTimer = 0.0f;
    
    // Visual settings
    PulseSpeed = 2.0f;
    MinIntensity = 3000.0f;
    MaxIntensity = 8000.0f;
}

void ASafeZoneCell::BeginPlay()
{
    Super::BeginPlay();
    
    // Bind overlap events
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASafeZoneCell::OnTriggerBeginOverlap);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ASafeZoneCell::OnTriggerEndOverlap);
    
    // Start hidden
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    
    UE_LOG(LogTemp, Log, TEXT("[SafeZoneCell] Created at %s"), *GetActorLocation().ToString());
}

void ASafeZoneCell::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!bIsActive)
        return;
    
    // Pulsing glow effect
    PulseTimer += DeltaTime * PulseSpeed;
    float PulseFactor = (FMath::Sin(PulseTimer) + 1.0f) / 2.0f;  // 0 to 1
    
    float CurrentIntensity = FMath::Lerp(MinIntensity, MaxIntensity, PulseFactor);
    PurpleLight->SetIntensity(CurrentIntensity);
}

void ASafeZoneCell::ActivateSafeZone()
{
    bIsActive = true;
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    
    UE_LOG(LogTemp, Warning, TEXT("[SafeZoneCell] ⚠️ SAFE ZONE ACTIVATED at %s"), *GetActorLocation().ToString());
    
    // Visual feedback
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Purple,
            TEXT("💜 PURPLE SAFE ZONE APPEARED! Monster can't catch you here!"));
    }
}

void ASafeZoneCell::DeactivateSafeZone()
{
    bIsActive = false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    bPlayerInside = false;
    
    UE_LOG(LogTemp, Log, TEXT("[SafeZoneCell] Safe zone deactivated"));
}

void ASafeZoneCell::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                          bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bIsActive)
        return;
    
    // Check if player
    ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
    if (PlayerChar && PlayerChar->IsPlayerControlled())
    {
        bPlayerInside = true;
        UE_LOG(LogTemp, Warning, TEXT("[SafeZoneCell] Player entered safe zone!"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                TEXT("SAFE! Monster cannot catch you in this zone!"));
        }
    }
}

void ASafeZoneCell::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!bIsActive)
        return;
    
    // Check if player
    ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
    if (PlayerChar && PlayerChar->IsPlayerControlled())
    {
        bPlayerInside = false;
        UE_LOG(LogTemp, Warning, TEXT("[SafeZoneCell] ⚠️ Player left safe zone!"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange,
                TEXT("⚠️ You left the safe zone! Monster can catch you now!"));
        }
    }
}
