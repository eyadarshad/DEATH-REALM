// MazeManager.cpp
#include "MazeManager.h"
#include "MazeCell.h"
#include "MuddyPatch.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "CustomQueue.h"

// Direction constants
const AMazeManager::FDirectionInfo* AMazeManager::GetAllDirections()
{
    static const FDirectionInfo Directions[] = {
        {EMazeDirection::North, -1, 0},
        {EMazeDirection::South, 1, 0},
        {EMazeDirection::East, 0, 1},
        {EMazeDirection::West, 0, -1}
    };
    return Directions;
}

EMazeDirection AMazeManager::GetOppositeDirection(EMazeDirection Dir)
{
    switch (Dir)
    {
        case EMazeDirection::North: return EMazeDirection::South;
        case EMazeDirection::South: return EMazeDirection::North;
        case EMazeDirection::East: return EMazeDirection::West;
        case EMazeDirection::West: return EMazeDirection::East;
        default: return EMazeDirection::North;
    }
}

bool AMazeManager::IsValidCell(int32 Row, int32 Col) const
{
    return Row >= 0 && Row < Rows && Col >= 0 && Col < Cols;
}

AMazeManager::AMazeManager()
{
    PrimaryActorTick.bCanEverTick = false;
    EscapeCell = nullptr;
    CellSize = 500.0f;
    Rows = 15;
    Cols = 15;
    LoopProbability = 0.15f;
    bIsMazeGenerated = false;
}

void AMazeManager::BeginPlay()
{
    Super::BeginPlay();
}

void AMazeManager::GenerateMazeImmediate()
{
    // Always regenerate (allows settings to apply immediately)
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Generating maze %dx%d..."), Rows, Cols);
    bIsMazeGenerated = false;  // Reset flag to allow regeneration
    GenerateMaze();
}

void AMazeManager::GenerateMaze(AMazeCell* PreservedCell)
{
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Generating %dx%d maze..."), Rows, Cols);
    
    if (PreservedCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Preserving cell (%d, %d) during regeneration"), 
               PreservedCell->Row, PreservedCell->Col);
    }
    
    if (!MazeCellClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeManager] MazeCellClass not set!"));
        return;
    }
    
    InitializeMaze(PreservedCell);  // Pass preserved cell to initialization
    
    if (MazeGrid.Num() == 0 || MazeGrid[0].Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[MazeManager] Grid creation failed!"));
        return;
    }
    
    GenerateWithDFS();
    
    if (LoopProbability > 0.0f)
    {
        CreateMazeLoops();
    }
    
    CreateExit(PreservedCell, 4);  // Ensure exit is at least 4 cells away from preserved cell
    VerifyMazeGeneration();
    SpawnMuddyPatches();  // Spawn muddy patches after maze is complete
    
    bIsMazeGenerated = true;
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Generation complete!"));
}

void AMazeManager::InitializeMaze(AMazeCell* PreservedCell)
{
    // Destroy existing cells (except preserved cell)
    TArray<AActor*> ExistingCells;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMazeCell::StaticClass(), ExistingCells);
    for (AActor* Cell : ExistingCells)
    {
        if (Cell != PreservedCell)  // Don't destroy preserved cell
        {
            Cell->Destroy();
        }
    }
    
    // Setup grid
    MazeGrid.Empty();
    MazeGrid.SetNum(Rows);
    
    // Spawn all cells (or restore preserved cell)
    for (int32 Row = 0; Row < Rows; Row++)
    {
        MazeGrid[Row].SetNum(Cols);
        
        for (int32 Col = 0; Col < Cols; Col++)
        {
            // Check if this is the preserved cell position
            if (PreservedCell && PreservedCell->Row == Row && PreservedCell->Col == Col)
            {
                // Restore preserved cell to grid
                MazeGrid[Row][Col] = PreservedCell;
                UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Preserved cell restored at (%d, %d)"), Row, Col);
                continue;  // Skip spawning new cell
            }
            
            FVector Location = FVector(Row * CellSize, Col * CellSize, 0.0f);
            
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            AMazeCell* NewCell = GetWorld()->SpawnActor<AMazeCell>(
                MazeCellClass, 
                Location, 
                FRotator::ZeroRotator, 
                SpawnParams
            );
            
            if (NewCell)
            {
                NewCell->Row = Row;
                NewCell->Col = Col;
                NewCell->bVisited = false;
                NewCell->bInMaze = false;
                NewCell->bIsEscapeCell = false;
                NewCell->ParentCell = nullptr;
                NewCell->Distance = -1;
                NewCell->UpdateCellSize(CellSize);
                NewCell->SetActorLocation(Location);
                MazeGrid[Row][Col] = NewCell;
            }
        }
    }
}

void AMazeManager::GenerateWithDFS()
{
    AMazeCell* StartCell = GetCell(FMath::RandRange(0, Rows - 1), FMath::RandRange(0, Cols - 1));
    
    if (!StartCell)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid start cell!"));
        return;
    }
    
    DFSRecursive(StartCell);
}

TArray<AMazeCell*> AMazeManager::GetUnvisitedNeighbors(AMazeCell* Cell) const
{
    TArray<AMazeCell*> Neighbors;
    if (!Cell) return Neighbors;
    
    const FDirectionInfo* Dirs = GetAllDirections();
    
    for (int32 i = 0; i < 4; i++)
    {
        int32 NewRow = Cell->Row + Dirs[i].RowDelta;
        int32 NewCol = Cell->Col + Dirs[i].ColDelta;
        
        if (IsValidCell(NewRow, NewCol))
        {
            AMazeCell* Neighbor = GetCell(NewRow, NewCol);
            if (Neighbor && !Neighbor->bVisited)
            {
                Neighbors.Add(Neighbor);
            }
        }
    }
    
    return Neighbors;
}

void AMazeManager::DFSRecursive(AMazeCell* Current)
{
    if (!Current) return;
    
    Current->bVisited = true;
    Current->bInMaze = true;
    
    // Get unvisited neighbors and shuffle
    TArray<AMazeCell*> UnvisitedNeighbors = GetUnvisitedNeighbors(Current);
    
    for (int32 i = UnvisitedNeighbors.Num() - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        UnvisitedNeighbors.Swap(i, j);
    }
    
    // Visit each unvisited neighbor
    for (AMazeCell* Neighbor : UnvisitedNeighbors)
    {
        if (!Neighbor->bVisited)
        {
            RemoveWallBetween(Current, Neighbor);
            DFSRecursive(Neighbor);
        }
    }
}

void AMazeManager::RemoveWallBetween(AMazeCell* CellA, AMazeCell* CellB)
{
    if (!CellA || !CellB) return;
    
    int32 dRow = CellB->Row - CellA->Row;
    int32 dCol = CellB->Col - CellA->Col;
    
    const FDirectionInfo* Dirs = GetAllDirections();
    
    for (int32 i = 0; i < 4; i++)
    {
        if (Dirs[i].RowDelta == dRow && Dirs[i].ColDelta == dCol)
        {
            CellA->RemoveWall(Dirs[i].Dir);
            CellB->RemoveWall(GetOppositeDirection(Dirs[i].Dir));
            return;
        }
    }
}

void AMazeManager::CreateMazeLoops()
{
    int32 TargetLoops = FMath::FloorToInt((Rows * Cols) * LoopProbability);
    int32 LoopsCreated = 0;
    
    const FDirectionInfo* Dirs = GetAllDirections();
    
    for (int32 Attempt = 0; Attempt < TargetLoops * 5 && LoopsCreated < TargetLoops; Attempt++)
    {
        AMazeCell* Cell = GetRandomCell();
        if (!Cell) continue;
        
        // Try random direction
        int32 DirIndex = FMath::RandRange(0, 3);
        
        for (int32 i = 0; i < 4; i++)
        {
            int32 CheckIndex = (DirIndex + i) % 4;
            EMazeDirection Dir = Dirs[CheckIndex].Dir;
            
            if (Cell->HasWall(Dir))
            {
                AMazeCell* Neighbor = GetNeighborInDirection(Cell, Dir);
                if (Neighbor)
                {
                    RemoveWallBetween(Cell, Neighbor);
                    LoopsCreated++;
                    break;
                }
            }
        }
    }
}

void AMazeManager::RemoveOuterWall(AMazeCell* Cell)
{
    if (!Cell) return;
    
    if (Cell->Row == 0)
        Cell->RemoveWall(EMazeDirection::North);
    else if (Cell->Row == Rows - 1)
        Cell->RemoveWall(EMazeDirection::South);
    else if (Cell->Col == 0)
        Cell->RemoveWall(EMazeDirection::West);
    else if (Cell->Col == Cols - 1)
        Cell->RemoveWall(EMazeDirection::East);
}

void AMazeManager::CreateExit(AMazeCell* AvoidCell, int32 MinDistance)
{
    // If we need to avoid a cell, filter edge cells by distance
    if (AvoidCell && MinDistance > 0)
    {
        TArray<AMazeCell*> ValidEdgeCells;
        
        for (const auto& Row : MazeGrid)
        {
            for (AMazeCell* Cell : Row)
            {
                if (Cell)
                {
                    bool bIsEdge = (Cell->Row == 0 || Cell->Row == Rows - 1 || 
                                   Cell->Col == 0 || Cell->Col == Cols - 1);
                    
                    if (bIsEdge)
                    {
                        int32 Distance = FMath::Abs(Cell->Row - AvoidCell->Row) + 
                                        FMath::Abs(Cell->Col - AvoidCell->Col);
                        if (Distance >= MinDistance)
                        {
                            ValidEdgeCells.Add(Cell);
                        }
                    }
                }
            }
        }
        
        if (ValidEdgeCells.Num() > 0)
        {
            int32 RandomIndex = FMath::RandRange(0, ValidEdgeCells.Num() - 1);
            EscapeCell = ValidEdgeCells[RandomIndex];
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MazeManager] No valid edge cells found at distance %d, using any edge cell"), MinDistance);
            EscapeCell = GetRandomEdgeCell();
        }
    }
    else
    {
        EscapeCell = GetRandomEdgeCell();
    }
    
    if (!EscapeCell)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create exit!"));
        return;
    }
    
    EscapeCell->MarkAsEscape();
    RemoveOuterWall(EscapeCell);
}

void AMazeManager::VerifyMazeGeneration()
{
    int32 InMazeCount = 0;
    int32 RemovedWallCount = 0;
    
    for (const auto& Row : MazeGrid)
    {
        for (AMazeCell* Cell : Row)
        {
            if (Cell)
            {
                if (Cell->bInMaze) InMazeCount++;
                
                for (int32 i = 0; i < 4; i++)
                {
                    if (!Cell->HasWall(static_cast<EMazeDirection>(i)))
                    {
                        RemovedWallCount++;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Cells: %d/%d, Walls removed: %d"), 
           InMazeCount, Rows * Cols, RemovedWallCount);
}

TArray<AMazeCell*> AMazeManager::FindPathBFS(AMazeCell* Start, AMazeCell* Goal)
{
    TArray<AMazeCell*> Path;
    if (!Start || !Goal) return Path;
    
    // Reset visited flags
    for (auto& Row : MazeGrid)
    {
        for (AMazeCell* Cell : Row)
        {
            if (Cell)
            {
                Cell->bVisited = false;
                Cell->ParentCell = nullptr;
            }
        }
    }
    
    // BFS using custom queue
    CustomQueue<AMazeCell*> Q;
    Q.Enqueue(Start);
    Start->bVisited = true;
    
    bool Found = false;
    
    while (!Q.IsEmpty() && !Found)
    {
        AMazeCell* Current = Q.Front();
        Q.Dequeue();
        
        if (Current == Goal)
        {
            Found = true;
            break;
        }
        
        for (AMazeCell* Neighbor : GetNeighbors(Current, false))
        {
            if (!Neighbor->bVisited)
            {
                Neighbor->bVisited = true;
                Neighbor->ParentCell = Current;
                Q.Enqueue(Neighbor);
            }
        }
    }
    
    // Reconstruct path
    if (Found)
    {
        AMazeCell* Current = Goal;
        while (Current)
        {
            Path.Insert(Current, 0);
            Current = Current->ParentCell;
        }
    }
    
    return Path;
}

// A* Pathfinding - More efficient and smoother than BFS
TArray<AMazeCell*> AMazeManager::FindPathAStar(AMazeCell* Start, AMazeCell* Goal)
{
    TArray<AMazeCell*> Path;
    if (!Start || !Goal) return Path;
    
    // Reset all cells
    for (auto& Row : MazeGrid)
    {
        for (AMazeCell* Cell : Row)
        {
            if (Cell)
            {
                Cell->bVisited = false;
                Cell->ParentCell = nullptr;
                Cell->GScore = FLT_MAX;
                Cell->HScore = 0.0f;
                Cell->FScore = FLT_MAX;
            }
        }
    }
    
    // Initialize start cell
    Start->GScore = 0.0f;
    Start->HScore = CalculateHeuristic(Start, Goal);
    Start->FScore = Start->HScore;
    
    // Open set (cells to evaluate)
    TArray<AMazeCell*> OpenSet;
    OpenSet.Add(Start);
    
    // Closed set (already evaluated)
    TSet<AMazeCell*> ClosedSet;
    
    while (OpenSet.Num() > 0)
    {
        // Find cell with lowest F score in open set
        AMazeCell* Current = OpenSet[0];
        int32 CurrentIndex = 0;
        
        for (int32 i = 1; i < OpenSet.Num(); i++)
        {
            if (OpenSet[i]->FScore < Current->FScore)
            {
                Current = OpenSet[i];
                CurrentIndex = i;
            }
        }
        
        // Found goal!
        if (Current == Goal)
        {
            // Reconstruct path
            AMazeCell* PathCell = Goal;
            while (PathCell)
            {
                Path.Insert(PathCell, 0);
                PathCell = PathCell->ParentCell;
            }
            
            return Path;
        }
        
        // Move current from open to closed
        OpenSet.RemoveAt(CurrentIndex);
        ClosedSet.Add(Current);
        
        // Check all neighbors
        TArray<AMazeCell*> Neighbors = GetNeighbors(Current, false);
        for (AMazeCell* Neighbor : Neighbors)
        {
            if (ClosedSet.Contains(Neighbor))
                continue;
            
            // Calculate tentative G score
            float TentativeGScore = Current->GScore + 1.0f; // Cost of 1 per cell
            
            // Check if this path is better
            if (TentativeGScore < Neighbor->GScore)
            {
                // This path is the best so far
                Neighbor->ParentCell = Current;
                Neighbor->GScore = TentativeGScore;
                Neighbor->HScore = CalculateHeuristic(Neighbor, Goal);
                Neighbor->FScore = Neighbor->GScore + Neighbor->HScore;
                
                // Add to open set if not already there
                if (!OpenSet.Contains(Neighbor))
                {
                    OpenSet.Add(Neighbor);
                }
            }
        }
    }
    
    // No path found
    return Path;
}

// Calculate heuristic (Manhattan distance)
float AMazeManager::CalculateHeuristic(AMazeCell* From, AMazeCell* To) const
{
    if (!From || !To) return 0.0f;
    
    // Manhattan distance in grid coordinates
    int32 DX = FMath::Abs(To->Row - From->Row);
    int32 DY = FMath::Abs(To->Col - From->Col);
    
    return static_cast<float>(DX + DY);
}

TArray<AMazeCell*> AMazeManager::GetNeighbors(AMazeCell* Cell, bool bIgnoreWalls) const
{
    TArray<AMazeCell*> Neighbors;
    if (!Cell) return Neighbors;
    
    const FDirectionInfo* Dirs = GetAllDirections();
    
    for (int32 i = 0; i < 4; i++)
    {
        int32 NewRow = Cell->Row + Dirs[i].RowDelta;
        int32 NewCol = Cell->Col + Dirs[i].ColDelta;
        
        if (IsValidCell(NewRow, NewCol))
        {
            if (bIgnoreWalls || !Cell->HasWall(Dirs[i].Dir))
            {
                AMazeCell* Neighbor = GetCell(NewRow, NewCol);
                if (Neighbor)
                {
                    Neighbors.Add(Neighbor);
                }
            }
        }
    }
    
    return Neighbors;
}

AMazeCell* AMazeManager::GetNeighborInDirection(AMazeCell* Cell, EMazeDirection Dir) const
{
    if (!Cell) return nullptr;
    
    const FDirectionInfo* Dirs = GetAllDirections();
    
    for (int32 i = 0; i < 4; i++)
    {
        if (Dirs[i].Dir == Dir)
        {
            int32 NewRow = Cell->Row + Dirs[i].RowDelta;
            int32 NewCol = Cell->Col + Dirs[i].ColDelta;
            return GetCell(NewRow, NewCol);
        }
    }
    
    return nullptr;
}

void AMazeManager::HighlightPath(const TArray<AMazeCell*>& Path)
{
    // Clear existing highlights
    for (auto& Row : MazeGrid)
    {
        for (AMazeCell* Cell : Row)
        {
            if (Cell && !Cell->bIsEscapeCell)
            {
                Cell->HighlightPath(false);
            }
        }
    }
    
    // Highlight new path
    for (AMazeCell* Cell : Path)
    {
        if (Cell && !Cell->bIsEscapeCell)
        {
            Cell->HighlightPath(true);
        }
    }
}

AMazeCell* AMazeManager::GetCell(int32 Row, int32 Col) const
{
    if (IsValidCell(Row, Col))
    {
        return MazeGrid[Row][Col];
    }
    return nullptr;
}

AMazeCell* AMazeManager::GetRandomCell()
{
    return GetCell(FMath::RandRange(0, Rows - 1), FMath::RandRange(0, Cols - 1));
}

AMazeCell* AMazeManager::GetRandomEdgeCell()
{
    int32 Edge = FMath::RandRange(0, 3);
    
    switch (Edge)
    {
        case 0: return GetCell(0, FMath::RandRange(0, Cols - 1));
        case 1: return GetCell(FMath::RandRange(0, Rows - 1), Cols - 1);
        case 2: return GetCell(Rows - 1, FMath::RandRange(0, Cols - 1));
        case 3: return GetCell(FMath::RandRange(0, Rows - 1), 0);
        default: return GetCell(0, 0);
    }
}

AMazeCell* AMazeManager::GetEscapeCell() const
{
    return EscapeCell;
}

void AMazeManager::SetMazeSize(int32 NewRows, int32 NewCols)
{
    // Validate and clamp values between 1 and 30
    Rows = FMath::Clamp(NewRows, 1, 30);
    Cols = FMath::Clamp(NewCols, 1, 30);
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Maze size set to %dx%d"), Rows, Cols);
}

void AMazeManager::SpawnMuddyPatches()
{
    if (!MuddyPatchClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MazeManager] MuddyPatchClass not set - skipping muddy patches"));
        return;
    }
    
    // Calculate 5% of total cells
    int32 TotalCells = Rows * Cols;
    int32 NumMuddyPatches = FMath::Max(1, FMath::RoundToInt(TotalCells * 0.05f));
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] Spawning %d muddy patches (5%% of %d cells)"), 
           NumMuddyPatches, TotalCells);
    
    // Get cells to avoid (will be set by GameMode)
    TArray<AMazeCell*> CellsToAvoid;
    
    // Always avoid escape cell
    if (EscapeCell)
    {
        CellsToAvoid.Add(EscapeCell);
    }
    
    // Spawn muddy patches
    int32 SpawnedCount = 0;
    int32 Attempts = 0;
    int32 MaxAttempts = NumMuddyPatches * 10;  // Try up to 10x the target count
    
    while (SpawnedCount < NumMuddyPatches && Attempts < MaxAttempts)
    {
        Attempts++;
        
        // Get random cell
        AMazeCell* RandomCell = GetRandomCell();
        if (!RandomCell)
        {
            continue;
        }
        
        // Check if this cell should be avoided
        bool bShouldAvoid = false;
        for (AMazeCell* AvoidCell : CellsToAvoid)
        {
            if (RandomCell == AvoidCell)
            {
                bShouldAvoid = true;
                break;
            }
        }
        
        if (bShouldAvoid)
        {
            continue;
        }
        
        // Spawn muddy patch at this cell
        FVector SpawnLocation = RandomCell->GetActorLocation();
        SpawnLocation.Z = 0.0f;  // On the ground
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        AMuddyPatch* MuddyPatch = GetWorld()->SpawnActor<AMuddyPatch>(
            MuddyPatchClass,
            SpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );
        
        if (MuddyPatch)
        {
            SpawnedCount++;
            CellsToAvoid.Add(RandomCell);  // Don't spawn another patch here
            UE_LOG(LogTemp, Log, TEXT("[MazeManager] Muddy patch %d/%d spawned at cell [%d,%d]"), 
                   SpawnedCount, NumMuddyPatches, RandomCell->Row, RandomCell->Col);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MazeManager] ✓ Spawned %d muddy patches"), SpawnedCount);
}

