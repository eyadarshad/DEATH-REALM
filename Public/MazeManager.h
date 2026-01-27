// MazeManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeCell.h"
#include "MazeManager.generated.h"

UCLASS()
class MAZERUNNER_API AMazeManager : public AActor
{
    GENERATED_BODY()
    
public:    
    AMazeManager();

protected:
    virtual void BeginPlay() override;

public:    
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze Generation")
    TSubclassOf<class AMazeCell> MazeCellClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze Generation")
    TSubclassOf<class AMuddyPatch> MuddyPatchClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze Generation", meta = (ClampMin = "5", ClampMax = "50"))
    int32 Rows;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze Generation", meta = (ClampMin = "5", ClampMax = "50"))
    int32 Cols;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze Generation", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float CellSize;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maze Generation", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float LoopProbability;
    
    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze State")
    bool bIsMazeGenerated;
    
    TArray<TArray<AMazeCell*>> MazeGrid;
    
    UPROPERTY()
    class AMazeCell* EscapeCell;
    
    // Generation functions
    UFUNCTION(BlueprintCallable, Category = "Maze Generation")
    void GenerateMazeImmediate();
    
    UFUNCTION(BlueprintCallable, Category = "Maze Generation")
    void GenerateMaze(AMazeCell* PreservedCell = nullptr);
    
    void InitializeMaze(AMazeCell* PreservedCell = nullptr);
    void GenerateWithDFS();
    void DFSRecursive(AMazeCell* Current);
    void RemoveWallBetween(AMazeCell* CellA, AMazeCell* CellB);
    void CreateMazeLoops();
    void CreateExit(AMazeCell* AvoidCell = nullptr, int32 MinDistance = 0);
    void VerifyMazeGeneration();
    void SpawnMuddyPatches();
    
    // Pathfinding
    UFUNCTION(BlueprintCallable, Category = "Maze Pathfinding")
    TArray<AMazeCell*> FindPathBFS(AMazeCell* Start, AMazeCell* Goal);
    
    UFUNCTION(BlueprintCallable, Category = "Maze Pathfinding")
    TArray<AMazeCell*> FindPathAStar(AMazeCell* Start, AMazeCell* Goal);
    
    UFUNCTION(BlueprintCallable, Category = "Maze Pathfinding")
    TArray<AMazeCell*> GetNeighbors(AMazeCell* Cell, bool bIgnoreWalls = false) const;
    
    UFUNCTION(BlueprintCallable, Category = "Maze Pathfinding")
    AMazeCell* GetNeighborInDirection(AMazeCell* Cell, EMazeDirection Direction) const;
    
    UFUNCTION(BlueprintCallable, Category = "Maze Pathfinding")
    void HighlightPath(const TArray<AMazeCell*>& Path);
    
    // A* helper
    float CalculateHeuristic(AMazeCell* From, AMazeCell* To) const;
    
    // Utility
    UFUNCTION(BlueprintCallable, Category = "Maze Utility")
    AMazeCell* GetCell(int32 Row, int32 Col) const;
    
    UFUNCTION(BlueprintCallable, Category = "Maze Utility")
    AMazeCell* GetRandomCell();
    
    UFUNCTION(BlueprintCallable, Category = "Maze Utility")
    AMazeCell* GetRandomEdgeCell();
    
    UFUNCTION(BlueprintCallable, Category = "Maze Utility")
    AMazeCell* GetEscapeCell() const;
    
    // Settings
    UFUNCTION(BlueprintCallable, Category = "Maze Settings")
    void SetMazeSize(int32 NewRows, int32 NewCols);

private:
    // Helper functions
    struct FDirectionInfo
    {
        EMazeDirection Dir;
        int32 RowDelta;
        int32 ColDelta;
    };
    
    static const FDirectionInfo* GetAllDirections();
    static EMazeDirection GetOppositeDirection(EMazeDirection Dir);
    bool IsValidCell(int32 Row, int32 Col) const;
    void RemoveOuterWall(AMazeCell* Cell);
    TArray<AMazeCell*> GetUnvisitedNeighbors(AMazeCell* Cell) const;
};
