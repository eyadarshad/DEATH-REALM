// TrapCell.cpp - Implementation of trap cell mechanics
#include "TrapCell.h"
#include "MazeCell.h"
#include "MazeGameMode.h"
#include "MazeManager.h"  // ADDED: Need full definition to access Rows/Cols
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"

ATrapCell::ATrapCell()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root component
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Create trigger box for collision detection
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootComponent);
    TriggerBox->SetBoxExtent(FVector(10.0f, 10.0f, 50.0f));  // 20% size - must be RIGHT on dandelion!
    TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerBox->SetGenerateOverlapEvents(true);

    // Create visual mesh (flat plane)
    TrapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrapMesh"));
    TrapMesh->SetupAttachment(RootComponent);
    
    // Load plane mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane"));
    if (PlaneMesh.Succeeded())
    {
        TrapMesh->SetStaticMesh(PlaneMesh.Object);
        TrapMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.0f));  // Smaller trap size
        TrapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));  // Slightly above floor
        TrapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    bTrapActivated = false;
}

void ATrapCell::BeginPlay()
{
    Super::BeginPlay();
    
    // Bind overlap event
    if (TriggerBox)
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATrapCell::OnTriggerBeginOverlap);
    }
    
    // Apply trap material if set
    if (TrapMaterial && TrapMesh)
    {
        TrapMesh->SetMaterial(0, TrapMaterial);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Trap cell initialized at %s"), *GetActorLocation().ToString());
}

void ATrapCell::Initialize(AMazeCell* Cell, float CellSize)
{
    if (!Cell)
    {
        UE_LOG(LogTemp, Error, TEXT("[TrapCell] Initialize called with null cell!"));
        return;
    }
    
    AssociatedCell = Cell;
    
    // Position trap at cell location
    FVector CellLocation = Cell->GetActorLocation();
    SetActorLocation(CellLocation);
    
    // DON'T scale trigger - keep small 10 unit trigger from constructor!
    // The Initialize function was overriding the small trigger box
    
    if (TrapMesh)
    {
        float Scale = (CellSize / 100.0f) * 1.5f;  // Scale to 1.5x
        TrapMesh->SetRelativeScale3D(FVector(Scale, Scale, 1.0f));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Initialized trap at cell [%d,%d] - keeping small 10 unit trigger"), Cell->Row, Cell->Col);
}

void ATrapCell::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                                       bool bFromSweep, const FHitResult& SweepResult)
{
    // Check if player touched the trap
    if (!OtherActor || bTrapActivated) return;
    
    // Check if it's the player (has "Player" tag or is a Character)
    if (OtherActor->ActorHasTag(FName("Player")) || OtherActor->IsA(ACharacter::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Player triggered trap!"));
        ActivateTrap(OtherActor);
    }
}

void ATrapCell::ActivateTrap(AActor* Player)
{
    if (bTrapActivated || !AssociatedCell) return;
    
    bTrapActivated = true;
    
    UE_LOG(LogTemp, Warning, TEXT("[TrapCell] ⚠️ TRAP ACTIVATED! Player trapped for %.1f seconds"), TrapDuration);
    
    // Play maze regeneration sound
    if (AMazeGameMode* GameMode = Cast<AMazeGameMode>(GetWorld()->GetAuthGameMode()))
    {
        if (GameMode->MazeRegenerationSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), GameMode->MazeRegenerationSound, 1.0f);
            UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Maze regeneration sound playing"));
        }
    }
    
    // Show warning message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
            FString::Printf(TEXT("⚠️ TRAPPED! Escape in %.0f seconds!"), TrapDuration));
    }
    
    // 1. Drag player to cell center
    DragPlayerToCenter(Player);
    
    // 2. Show all 4 walls to trap player
    ShowCellWalls();
    
    // 3. Hide the trap mesh (trap is triggered)
    if (TrapMesh)
    {
        TrapMesh->SetVisibility(false);
    }
    
    // 4. Set timer for release
    GetWorldTimerManager().SetTimer(ReleaseTimerHandle, this, &ATrapCell::ReleaseTrap, TrapDuration, false);
}

void ATrapCell::DragPlayerToCenter(AActor* Player)
{
    if (!Player || !AssociatedCell) return;
    
    // Get cell center location
    FVector CellCenter = AssociatedCell->GetActorLocation();
    CellCenter.Z += 100.0f;  // Slightly above floor to avoid clipping
    
    // Teleport player to center
    Player->SetActorLocation(CellCenter);
    
    UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Player dragged to cell center: %s"), *CellCenter.ToString());
}

void ATrapCell::ShowCellWalls()
{
    if (!AssociatedCell) return;
    
    // Show all 4 walls with collision
    AssociatedCell->ShowAllWalls();
    
    UE_LOG(LogTemp, Warning, TEXT("[TrapCell] All walls shown - player is trapped!"));
}

void ATrapCell::ReleaseTrap()
{
    if (!AssociatedCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Cannot release - no associated cell"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Releasing trap - hiding walls"));
    
    // CRITICAL FIX: Don't hide edge walls to prevent void access!
    // Get maze dimensions from GameMode
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode && GameMode->MazeManager)
    {
        int32 MaxRow = GameMode->MazeManager->Rows - 1;
        int32 MaxCol = GameMode->MazeManager->Cols - 1;
        
        bool bIsEdgeCell = (AssociatedCell->Row == 0 || 
                            AssociatedCell->Col == 0 ||
                            AssociatedCell->Row == MaxRow ||
                            AssociatedCell->Col == MaxCol);
        
        if (bIsEdgeCell)
        {
            // Keep edge walls visible, only hide interior walls
            UE_LOG(LogTemp, Warning, TEXT("[TrapCell] Edge cell detected - keeping boundary walls"));
            // Don't call HideAllWalls() for edge cells
        }
        else
        {
            // Hide all walls to release player (safe for interior cells)
            AssociatedCell->HideAllWalls();
        }
    }
    else
    {
        // Fallback: hide all walls if can't determine edge status
        AssociatedCell->HideAllWalls();
    }
    
    // Show release message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Trap released! You're free!"));
    }
    
    // Destroy trap cell (one-time use)
    Destroy();
}
