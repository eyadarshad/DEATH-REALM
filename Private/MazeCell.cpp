// MazeCell.cpp
#include "MazeCell.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PointLightComponent.h"

// Constants for cell dimensions and lighting
namespace MazeCellConstants
{
    const float WallHeight = 1600.0f;
    const float WallThickness = 150.0f;
    const float FloorThickness = 20.0f;
    
    const float PathLightIntensity = 5000.0f;
    const float PathLightRadius = 400.0f;
    const float PathEmissiveIntensity = 2.0f;
    
    const float ExitLightIntensity = 3000.0f;
    const float ExitLightRadius = 400.0f;
    const float ExitEmissiveIntensity = 2.0f;
    
    const FLinearColor GoldenColor = FLinearColor(1.0f, 0.84f, 0.0f);
    const FLinearColor GreenColor = FLinearColor(0.0f, 1.0f, 0.0f);
}

AMazeCell::AMazeCell()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Load cube mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
    UStaticMesh* CubeMeshAsset = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;

    // Create floor
    Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
    Floor->SetupAttachment(RootComponent);
    if (CubeMeshAsset)
    {
        Floor->SetStaticMesh(CubeMeshAsset);
        Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Floor->SetCollisionResponseToAllChannels(ECR_Block);
    }

    // Create walls using helper function
    CreateWallComponent(WallNorth, TEXT("WallNorth"), CubeMeshAsset);
    CreateWallComponent(WallSouth, TEXT("WallSouth"), CubeMeshAsset);
    CreateWallComponent(WallEast, TEXT("WallEast"), CubeMeshAsset);
    CreateWallComponent(WallWest, TEXT("WallWest"), CubeMeshAsset);

    // Initialize properties
    Row = 0;
    Col = 0;
    bVisited = false;
    bInMaze = false;
    bIsEscapeCell = false;
    ParentCell = nullptr;
    Distance = -1;
    PulseTimer = 0.0f;
    FloorMaterial = nullptr;
    ExitMaterial = nullptr;
    PathLight = nullptr;

    // Initialize all walls as active
    for (int32 i = 0; i < 4; i++) 
    {
        bWallsActive[i] = true;
    }
}

void AMazeCell::BeginPlay()
{
    Super::BeginPlay();

    // Create dynamic material for floor
    if (Floor)
    {
        UMaterialInterface* BaseMaterial = Floor->GetMaterial(0);
        if (BaseMaterial)
        {
            FloorMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            if (FloorMaterial)
            {
                Floor->SetMaterial(0, FloorMaterial);
                FloorMaterial->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor(0.0f, 0.0f, 0.0f));
                FloorMaterial->SetScalarParameterValue(FName("EmissiveIntensity"), 0.0f);
            }
        }
    }
}

void AMazeCell::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Pulse animation for escape cell (reduced tick rate for performance)
    if (bIsEscapeCell && ExitMaterial)
    {
        PulseTimer += DeltaTime * 2.0f;
        float PulseValue = FMath::Sin(PulseTimer);
        float Intensity = 2.0f + (PulseValue * 0.5f);
        
        ExitMaterial->SetVectorParameterValue(FName("EmissiveColor"), 
            MazeCellConstants::GreenColor * Intensity);
    }
}

void AMazeCell::UpdateCellSize(float NewSize)
{
    using namespace MazeCellConstants;
    
    const float HalfCell = NewSize / 2.0f;
    const float HalfWall = WallThickness / 2.0f;
    
    // Floor setup
    Floor->SetRelativeScale3D(FVector(
        NewSize / 100.0f,
        NewSize / 100.0f,
        FloorThickness / 100.0f
    ));
    Floor->SetRelativeLocation(FVector(0.0f, 0.0f, -FloorThickness));
    
    // North wall (at -X boundary)
    if (WallNorth)
    {
        WallNorth->SetRelativeScale3D(FVector(
            WallThickness / 100.0f,
            NewSize / 100.0f,  // FIXED: Removed WallThickness to prevent overlap
            WallHeight / 100.0f
        ));
        WallNorth->SetRelativeLocation(FVector(
            -(HalfCell + HalfWall),
            0.0f,
            WallHeight / 2.0f - FloorThickness
        ));
    }
    
    // South wall (at +X boundary)
    if (WallSouth)
    {
        WallSouth->SetRelativeScale3D(FVector(
            WallThickness / 100.0f,
            NewSize / 100.0f,  // FIXED: Removed WallThickness to prevent overlap
            WallHeight / 100.0f
        ));
        WallSouth->SetRelativeLocation(FVector(
            (HalfCell + HalfWall),
            0.0f,
            WallHeight / 2.0f - FloorThickness
        ));
    }
    
    // East wall (at +Y boundary)
    if (WallEast)
    {
        WallEast->SetRelativeScale3D(FVector(
            NewSize / 100.0f,
            WallThickness / 100.0f,
            WallHeight / 100.0f
        ));
        WallEast->SetRelativeLocation(FVector(
            0.0f,
            (HalfCell + HalfWall),
            WallHeight / 2.0f - FloorThickness
        ));
    }
    
    // West wall (at -Y boundary)
    if (WallWest)
    {
        WallWest->SetRelativeScale3D(FVector(
            NewSize / 100.0f,
            WallThickness / 100.0f,
            WallHeight / 100.0f
        ));
        WallWest->SetRelativeLocation(FVector(
            0.0f,
            -(HalfCell + HalfWall),
            WallHeight / 2.0f - FloorThickness
        ));
    }
}

UStaticMeshComponent* AMazeCell::GetWallComponent(EMazeDirection Direction) const
{
    switch (Direction)
    {
        case EMazeDirection::North: return WallNorth;
        case EMazeDirection::East:  return WallEast;
        case EMazeDirection::South: return WallSouth;
        case EMazeDirection::West:  return WallWest;
        default: return nullptr;
    }
}

void AMazeCell::RemoveWall(EMazeDirection Direction)
{
    UStaticMeshComponent* Wall = GetWallComponent(Direction);
    if (Wall && bWallsActive[static_cast<int32>(Direction)])
    {
        Wall->SetVisibility(false);
        Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        bWallsActive[static_cast<int32>(Direction)] = false;
    }
}

bool AMazeCell::HasWall(EMazeDirection Direction) const
{
    return bWallsActive[static_cast<int32>(Direction)];
}

void AMazeCell::RemoveAllWalls()
{
    for (int32 i = 0; i < 4; i++)
    {
        RemoveWall(static_cast<EMazeDirection>(i));
    }
}

void AMazeCell::HighlightPath(bool bEnable)
{
    using namespace MazeCellConstants;
    
    if (bEnable)
    {
        // Create a subtle golden point light for path highlighting
        if (!PathLight)
        {
            PathLight = CreateHighlightLight(GoldenColor, 1500.0f, 300.0f, 100.0f);
            UE_LOG(LogTemp, Log, TEXT("[MazeCell] Path light created at [%d,%d]"), Row, Col);
        }
    }
    else
    {
        // Remove the path light
        if (PathLight)
        {
            PathLight->DestroyComponent();
            PathLight = nullptr;
        }
    }
}

void AMazeCell::MarkAsEscape()
{
    using namespace MazeCellConstants;
    
    bIsEscapeCell = true;
    
    // Create emissive material for exit
    if (Floor)
    {
        // Reuse existing FloorMaterial if available, otherwise create new one
        if (!FloorMaterial)
        {
            UMaterialInterface* BaseMaterial = Floor->GetMaterial(0);
            if (BaseMaterial)
            {
                FloorMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                if (FloorMaterial)
                {
                    Floor->SetMaterial(0, FloorMaterial);
                }
            }
        }
        
        // Use FloorMaterial as ExitMaterial to preserve ground texture
        ExitMaterial = FloorMaterial;
        if (ExitMaterial)
        {
            SetupEmissiveMaterial(ExitMaterial, GreenColor, ExitEmissiveIntensity);
        }
    }
    
    // Add green point light
    CreateHighlightLight(GreenColor, ExitLightIntensity, ExitLightRadius, 200.0f);
    
    // Enable ticking for pulse animation at reduced rate (30 FPS for performance)
    SetActorTickEnabled(true);
    PrimaryActorTick.TickInterval = 0.033f; // ~30 FPS tick rate
}

// Helper function implementations
void AMazeCell::CreateWallComponent(UStaticMeshComponent*& Wall, const FName& Name, UStaticMesh* Mesh)
{
    Wall = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Wall->SetupAttachment(RootComponent);
    if (Mesh)
    {
        Wall->SetStaticMesh(Mesh);
        Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Wall->SetCollisionResponseToAllChannels(ECR_Block);
    }
}

void AMazeCell::SetupEmissiveMaterial(UMaterialInstanceDynamic*& Material, const FLinearColor& Color, float Intensity)
{
    if (Material)
    {
        Material->SetVectorParameterValue(FName("EmissiveColor"), Color);
        Material->SetScalarParameterValue(FName("EmissiveIntensity"), Intensity);
    }
}

UPointLightComponent* AMazeCell::CreateHighlightLight(const FLinearColor& Color, float Intensity, float Radius, float Height)
{
    UPointLightComponent* Light = NewObject<UPointLightComponent>(this, UPointLightComponent::StaticClass());
    if (Light)
    {
        Light->RegisterComponent();
        Light->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        Light->SetRelativeLocation(FVector(0.0f, 0.0f, Height));
        Light->SetLightColor(Color);
        Light->SetIntensity(Intensity);
        Light->SetAttenuationRadius(Radius);
        Light->SetCastShadows(false);
    }
    return Light;
}

void AMazeCell::ShowAllWalls()
{
    if (WallNorth)
    {
        WallNorth->SetVisibility(true);
        WallNorth->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WallNorth->SetHiddenInGame(false);
    }
    if (WallSouth)
    {
        WallSouth->SetVisibility(true);
        WallSouth->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WallSouth->SetHiddenInGame(false);
    }
    if (WallEast)
    {
        WallEast->SetVisibility(true);
        WallEast->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WallEast->SetHiddenInGame(false);
    }
    if (WallWest)
    {
        WallWest->SetVisibility(true);
        WallWest->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WallWest->SetHiddenInGame(false);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeCell] All walls shown and collision enabled for cell (%d, %d)"), Row, Col);
}

void AMazeCell::HideAllWalls()
{
    if (WallNorth)
    {
        WallNorth->SetVisibility(false);
        WallNorth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WallNorth->SetHiddenInGame(true);
    }
    if (WallSouth)
    {
        WallSouth->SetVisibility(false);
        WallSouth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WallSouth->SetHiddenInGame(true);
    }
    if (WallEast)
    {
        WallEast->SetVisibility(false);
        WallEast->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WallEast->SetHiddenInGame(true);
    }
    if (WallWest)
    {
        WallWest->SetVisibility(false);
        WallWest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WallWest->SetHiddenInGame(true);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeCell] All walls hidden and collision disabled for cell (%d, %d)"), Row, Col);
}
