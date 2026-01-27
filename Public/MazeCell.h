// MazeCell.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeCell.generated.h"

UENUM(BlueprintType)
enum class EMazeDirection : uint8
{
    North = 0 UMETA(DisplayName = "North"),
    East  = 1 UMETA(DisplayName = "East"),
    South = 2 UMETA(DisplayName = "South"),
    West  = 3 UMETA(DisplayName = "West")
};

UCLASS()
class MAZERUNNER_API AMazeCell : public AActor
{
    GENERATED_BODY()
    
public:    
    AMazeCell();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
        
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* Floor;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WallNorth;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WallEast;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WallSouth;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WallWest;
    
    // Grid position
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze")
    int32 Row;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze")
    int32 Col;
    
    // State flags
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze")
    bool bVisited;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze")
    bool bInMaze;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze")
    bool bIsEscapeCell;
    
    bool bWallsActive[4];
    
    // Pathfinding
    UPROPERTY()
    AMazeCell* ParentCell;
    
    UPROPERTY()
    int32 Distance;
    
    // A* pathfinding scores
    float GScore;  // Cost from start to this cell
    float HScore;  // Heuristic cost to goal
    float FScore;  // GScore + HScore (total estimated cost)
    
    // Materials and lighting
    UPROPERTY()
    class UMaterialInstanceDynamic* FloorMaterial;
    
    UPROPERTY()
    class UMaterialInstanceDynamic* ExitMaterial;
    
    UPROPERTY()
    class UPointLightComponent* PathLight;
    
    float PulseTimer;
    
    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void UpdateCellSize(float NewSize);
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    class UStaticMeshComponent* GetWallComponent(EMazeDirection Direction) const;
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void RemoveWall(EMazeDirection Direction);
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    bool HasWall(EMazeDirection Direction) const;
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void RemoveAllWalls();
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void HighlightPath(bool bEnable);
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void MarkAsEscape();
    
    // Maze trap functions
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void ShowAllWalls();
    
    UFUNCTION(BlueprintCallable, Category = "Maze")
    void HideAllWalls();

private:
    // Helper functions
    void CreateWallComponent(UStaticMeshComponent*& Wall, const FName& Name, UStaticMesh* Mesh);
    void SetupEmissiveMaterial(UMaterialInstanceDynamic*& Material, const FLinearColor& Color, float Intensity);
    UPointLightComponent* CreateHighlightLight(const FLinearColor& Color, float Intensity, float Radius, float Height);
};
