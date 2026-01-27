// Stars.cpp - Simple star system implementation
#include "Stars.h"
#include "Components/PointLightComponent.h"

AStars::AStars()
{
    PrimaryActorTick.bCanEverTick = false;
    
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void AStars::BeginPlay()
{
    Super::BeginPlay();
    CreateStars();
}

void AStars::CreateStars()
{
    FVector CenterLocation = GetActorLocation();
    
    for (int32 i = 0; i < NumberOfStars; i++)
    {
        // Random position in sky
        float RandomX = FMath::RandRange(-SpreadRadius, SpreadRadius);
        float RandomY = FMath::RandRange(-SpreadRadius, SpreadRadius);
        float RandomZ = FMath::RandRange(SkyHeight - 500.0f, SkyHeight + 500.0f);
        
        FVector StarLocation = FVector(RandomX, RandomY, RandomZ);
        
        // Create star light
        UPointLightComponent* StarLight = NewObject<UPointLightComponent>(this);
        if (StarLight)
        {
            StarLight->RegisterComponent();
            StarLight->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
            
            // Small, dim white lights for stars
            StarLight->SetIntensity(FMath::RandRange(500.0f, 1500.0f));
            StarLight->SetAttenuationRadius(2000.0f);
            StarLight->SetLightColor(FLinearColor(1.0f, 1.0f, 1.0f)); // White
            StarLight->SetCastShadows(false);
            
            StarLight->SetRelativeLocation(StarLocation);
            
            StarLights.Add(StarLight);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Stars] Created %d stars in the sky"), NumberOfStars);
}
