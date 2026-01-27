// Moon.cpp
#include "Moon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"

AMoon::AMoon()
{
    PrimaryActorTick.bCanEverTick = true;  // Enable tick to follow player

    // Create root component
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Create moon mesh (sphere)
    MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
    MoonMesh->SetupAttachment(RootComponent);
    MoonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Load sphere mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        MoonMesh->SetStaticMesh(SphereMesh.Object);
        MoonMesh->SetRelativeScale3D(FVector(20.0f, 20.0f, 20.0f)); // Much larger for visibility
    }

    // Create moon light (soft white glow)
    MoonLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(RootComponent);
    MoonLight->SetIntensity(8000.0f);  // Increased for better visibility
    MoonLight->SetAttenuationRadius(15000.0f); // Large radius for ambient lighting
    MoonLight->SetLightColor(FLinearColor(0.8f, 0.85f, 1.0f)); // Soft blue-white
    MoonLight->SetCastShadows(false); // No shadows for performance
}

void AMoon::BeginPlay()
{
    Super::BeginPlay();
    
    // Don't try to set emissive material - just use the default white sphere
    // The point light will provide the glow effect
}

void AMoon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Follow player position
    APawn* Player = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (Player)
    {
        FVector PlayerLocation = Player->GetActorLocation();
        FRotator PlayerRotation = Player->GetActorRotation();
        
        // Get forward vector to position moon ahead of player
        FVector ForwardVector = PlayerRotation.Vector();
        
        // Position moon ahead and high in sky for camera visibility
        FVector MoonLocation = FVector(
            PlayerLocation.X + (ForwardVector.X * 2000.0f),  // 2000 units ahead
            PlayerLocation.Y + (ForwardVector.Y * 2000.0f),  // 2000 units ahead
            4000.0f                                          // High in sky
        );
        
        SetActorLocation(MoonLocation);
    }
}
