// GoldenStar.cpp
// Implementation of the star that reveals the path

#include "GoldenStar.h"
#include "MazeManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"

AGoldenStar::AGoldenStar()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create Components
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(100.0f);
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    RootComponent = CollisionSphere;

    StarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarMesh"));
    StarMesh->SetupAttachment(RootComponent);
    StarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StarMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));

    // Load sphere mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        StarMesh->SetStaticMesh(SphereMesh.Object);
    }
    
    // Load golden material
    static ConstructorHelpers::FObjectFinder<UMaterial> GoldMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (GoldMaterial.Succeeded())
    {
        StarMesh->SetMaterial(0, GoldMaterial.Object);
    }

    // Initialize vars
    BobTimer = 0.0f;
    InitialZ = 0.0f;
    MazeManager = nullptr;
}

void AGoldenStar::BeginPlay()
{
    Super::BeginPlay();

    InitialZ = GetActorLocation().Z;

    // Find MazeManager
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMazeManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        MazeManager = Cast<AMazeManager>(FoundActors[0]);
    }

    // Add golden point light for glow effect
    UPointLightComponent* GlowLight = NewObject<UPointLightComponent>(this);
    if (GlowLight)
    {
        GlowLight->RegisterComponent();
        GlowLight->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        
        // Golden glow settings
        GlowLight->SetIntensity(3000.0f);
        GlowLight->SetAttenuationRadius(400.0f);
        GlowLight->SetLightColor(FLinearColor(1.0f, 0.8f, 0.2f)); // Golden yellow
        GlowLight->SetCastShadows(false); // No shadows for performance
        
        GlowLight->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
        
        UE_LOG(LogTemp, Warning, TEXT("[GoldenStar] Point light added for glow"));
    }

    // Bind overlap
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGoldenStar::OnOverlapBegin);
}

void AGoldenStar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Bobbing and Rotating animation
    BobTimer += DeltaTime * BobSpeed;
    float NewZ = InitialZ + FMath::Sin(BobTimer) * BobHeight;
    
    FVector NewLoc = GetActorLocation();
    NewLoc.Z = NewZ;
    SetActorLocation(NewLoc);

    AddActorLocalRotation(FRotator(0, RotationSpeed * DeltaTime, 0));
}

void AGoldenStar::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                       bool bFromSweep, const FHitResult& SweepResult)
{
    // Check if player
    if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
    {
        RevealPathToExit();
        
        // Play pickup sound
        if (PickupSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), PickupSound, 0.7f);
        }
        
        // Disable collision and visibility so it looks "Collected"
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        
        UE_LOG(LogTemp, Warning, TEXT("Star Collected! Path revealed."));
        
        // Destroy after delay? or keep hidden. Destroying is fine.
        SetLifeSpan(1.0f); 
    }
}

void AGoldenStar::RevealPathToExit()
{
    if (!MazeManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[GoldenStar] MazeManager is null! Cannot reveal path."));
        return;
    }
    
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!Player)
    {
        UE_LOG(LogTemp, Error, TEXT("[GoldenStar] Player is null! Cannot reveal path."));
        return;
    }

    // 1. Get Player Cell
    FVector PLoc = Player->GetActorLocation();
    float CS = MazeManager->CellSize;
    int32 PR = FMath::RoundToInt(PLoc.X / CS);
    int32 PC = FMath::RoundToInt(PLoc.Y / CS);
    
    AMazeCell* PlayerCell = MazeManager->GetCell(PR, PC);
    
    // 2. Get Exit Cell
    AMazeCell* ExitCell = MazeManager->GetEscapeCell();

    UE_LOG(LogTemp, Warning, TEXT("[GoldenStar] Revealing path from [%d,%d] to exit"), PR, PC);
    
    // 3. Solve Path
    if (PlayerCell && ExitCell)
    {
        TArray<AMazeCell*> Path = MazeManager->FindPathBFS(PlayerCell, ExitCell);
        
        UE_LOG(LogTemp, Warning, TEXT("[GoldenStar] Path found with %d cells"), Path.Num());
        
        // 4. Highlight
        MazeManager->HighlightPath(Path);
        
        UE_LOG(LogTemp, Warning, TEXT("[GoldenStar] ✓ Path highlighted successfully!"));
    }
    else
    {
        if (!PlayerCell)
            UE_LOG(LogTemp, Error, TEXT("[GoldenStar] PlayerCell is null at [%d,%d]!"), PR, PC);
        if (!ExitCell)
            UE_LOG(LogTemp, Error, TEXT("[GoldenStar] ExitCell is null!"));
    }
}
