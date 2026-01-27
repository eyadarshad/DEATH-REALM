// MazePlayerController.cpp
// Implementation of player controller with pause functionality

#include "MazePlayerController.h"
#include "MazeGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

AMazePlayerController::AMazePlayerController()
{
    // Constructor
}

void AMazePlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("PlayerController initialized - pause handled by GameMode"));
}

void AMazePlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Get player character
    ACharacter* PlayerChar = Cast<ACharacter>(GetPawn());
    if (!PlayerChar) return;
    
    // Get player velocity
    FVector Velocity = PlayerChar->GetVelocity();
    float Speed = Velocity.Size();
    
    // Check if player is moving
    if (Speed > MovementThreshold)
    {
        // Update footstep timer
        FootstepTimer += DeltaTime;
        
        // Play footstep sound at intervals
        if (FootstepTimer >= FootstepInterval && FootstepSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), FootstepSound, 0.4f);
            FootstepTimer = 0.0f; // Reset timer
        }
    }
    else
    {
        // Reset timer when not moving
        FootstepTimer = 0.0f;
    }
}

void AMazePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    
    if (InputComponent)
    {
        InputComponent->BindAction("Pause", IE_Pressed, this, &AMazePlayerController::OnPausePressed);
        InputComponent->BindAction("Jump", IE_Pressed, this, &AMazePlayerController::JumpPressed);
        
        InputComponent->BindAxis("MoveForward", this, &AMazePlayerController::MoveForward);
        InputComponent->BindAxis("MoveRight", this, &AMazePlayerController::MoveRight);
        InputComponent->BindAxis("Turn", this, &AMazePlayerController::Turn);
        InputComponent->BindAxis("LookUp", this, &AMazePlayerController::LookUp);
        
        UE_LOG(LogTemp, Log, TEXT("Movement inputs bound!"));
    }
}

void AMazePlayerController::MoveForward(float Value)
{
    if (Value != 0.0f)
    {
        const FRotator Rotation = GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        GetPawn()->AddMovementInput(Direction, Value);
    }
}

void AMazePlayerController::MoveRight(float Value)
{
    if (Value != 0.0f)
    {
        const FRotator Rotation = GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        GetPawn()->AddMovementInput(Direction, Value);
    }
}

void AMazePlayerController::Turn(float Value)
{
    if (Value != 0.0f)
    {
        // Apply mouse sensitivity from GameMode
        AMazeGameMode* GameMode = Cast<AMazeGameMode>(GetWorld()->GetAuthGameMode());
        float Sensitivity = GameMode ? GameMode->GetMouseSensitivity() : 1.0f;
        
        AddYawInput(Value * Sensitivity);
    }
}

void AMazePlayerController::LookUp(float Value)
{
    if (Value != 0.0f)
    {
        // Apply mouse sensitivity from GameMode
        AMazeGameMode* GameMode = Cast<AMazeGameMode>(GetWorld()->GetAuthGameMode());
        float Sensitivity = GameMode ? GameMode->GetMouseSensitivity() : 1.0f;
        
        AddPitchInput(Value * Sensitivity);
    }
}
void AMazePlayerController::JumpPressed()
{
    if (ACharacter* Char = GetPawn<ACharacter>()) 
    {
        Char->Jump();
        
        // Play jump sound
        if (JumpSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), JumpSound, 0.5f);
        }
    }
}

void AMazePlayerController::OnPausePressed()
{
    // Get game mode and toggle pause
    if (AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        if (GameMode->CurrentGameState == EGameState::Playing)
        {
            GameMode->PauseGame();
        }
        else if (GameMode->CurrentGameState == EGameState::Paused)
        {
            GameMode->ResumeGame();
        }
    }
}
