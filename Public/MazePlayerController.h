// MazePlayerController.h
// Player controller with ESC key handling for pause menu

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MazePlayerController.generated.h"

UCLASS()
class MAZERUNNER_API AMazePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMazePlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ==================== AUDIO ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* FootstepSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* JumpSound;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float FootstepInterval = 0.4f; // Time between footsteps
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float MovementThreshold = 100.0f; // Minimum speed to play footsteps
    
private:
    float FootstepTimer = 0.0f;
    
public:
    // ==================== INPUT HANDLING ====================
    
    // Handle ESC key press for pause menu
    void OnPausePressed();
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void JumpPressed();
    
    // ==================== CONSOLE COMMANDS ====================
    
    UFUNCTION(Exec)
    void StartLevel(int32 LevelNumber);
};
