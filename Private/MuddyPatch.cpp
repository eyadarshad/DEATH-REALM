// MuddyPatch.cpp
#include "MuddyPatch.h"
#include "MazeGameMode.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

AMuddyPatch::AMuddyPatch()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Create root component
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    
    // Create trigger sphere (small and flat for easy jumping)
    TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
    TriggerSphere->SetupAttachment(RootComponent);
    TriggerSphere->SetSphereRadius(100.0f);  // Small radius: 200 unit diameter (half of cell size)
    TriggerSphere->SetRelativeLocation(FVector(0.0f, 0.0f, 15.0f));  // Slightly above ground
    TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerSphere->SetGenerateOverlapEvents(true);
    
    // Create muddy mesh (visual indicator) - sphere mesh
    MuddyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuddyMesh"));
    MuddyMesh->SetupAttachment(RootComponent);
    MuddyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 1.0f));  // Slightly above floor
    MuddyMesh->SetRelativeScale3D(FVector(2.5f, 2.5f, 0.5f));  // Flattened sphere
    MuddyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Load sphere mesh for muddy visual
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        MuddyMesh->SetStaticMesh(SphereMesh.Object);
    }
    
    // Initialize material property (will be set in Blueprint)
    MuddyMaterial = nullptr;
}

void AMuddyPatch::BeginPlay()
{
    Super::BeginPlay();
    
    // Create irregular, organic shape by randomizing scale
    float BaseScale = 2.5f;
    float ScaleVariationX = FMath::RandRange(0.7f, 1.3f);  // Random variation on X axis
    float ScaleVariationY = FMath::RandRange(0.7f, 1.3f);  // Random variation on Y axis
    float RandomRotation = FMath::RandRange(0.0f, 360.0f); // Random rotation for more variety
    
    MuddyMesh->SetRelativeScale3D(FVector(BaseScale * ScaleVariationX, BaseScale * ScaleVariationY, 0.01f));
    MuddyMesh->SetRelativeRotation(FRotator(0.0f, RandomRotation, 0.0f));
    
    // Also slightly randomize sphere radius for irregular collision
    float RadiusVariation = FMath::RandRange(0.8f, 1.2f);
    TriggerSphere->SetSphereRadius(100.0f * RadiusVariation);
    
    UE_LOG(LogTemp, Log, TEXT("[MuddyPatch] Irregular shape: Scale(%.2f, %.2f), Rotation: %.0f°, Radius: %.0f"), 
           BaseScale * ScaleVariationX, BaseScale * ScaleVariationY, RandomRotation, 100.0f * RadiusVariation);
    
    // Apply custom material if set in Blueprint
    if (MuddyMaterial)
    {
        MuddyMesh->SetMaterial(0, MuddyMaterial);
        UE_LOG(LogTemp, Warning, TEXT("[MuddyPatch] Using custom muddy material"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MuddyPatch] No material set - using default mesh material"));
    }
    
    // Bind overlap event
    TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AMuddyPatch::OnTriggerBeginOverlap);
    
    UE_LOG(LogTemp, Warning, TEXT("[MuddyPatch] Spawned at %s (Irregular organic sphere)"), *GetActorLocation().ToString());
}

void AMuddyPatch::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
                                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                                        bool bFromSweep, const FHitResult& SweepResult)
{
    // Check if player character touched the muddy patch
    ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
    if (!PlayerChar || !PlayerChar->IsPlayerControlled())
    {
        return;
    }
    
    // Check if player is jumping (Z velocity > 0 means in air)
    FVector Velocity = PlayerChar->GetVelocity();
    if (Velocity.Z > 50.0f)  // Player is jumping/in air
    {
        UE_LOG(LogTemp, Warning, TEXT("[MuddyPatch] Player jumped over muddy patch! No effect."));
        return;
    }
    
    // Player touched while on ground - apply muddy effect!
    UE_LOG(LogTemp, Warning, TEXT("[MuddyPatch] ⚠️ Player stepped in mud! Resolution reduced!"));
    
    // Get game mode and apply resolution effect
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->ApplyMuddyEffect(EffectDuration, ResolutionScale);
    }
    
    // Visual feedback
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, 
            TEXT("⚠️ STEPPED IN MUD! Vision blurred for 10 seconds!"));
    }
}
