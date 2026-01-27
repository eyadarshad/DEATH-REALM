// PauseMenuStack.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PauseMenuStack.generated.h"

UCLASS()
class MAZERUNNER_API APauseMenuStack : public AActor
{
    GENERATED_BODY()
    
public:    
    APauseMenuStack();

protected:
    virtual void BeginPlay() override;

public:    
    // Toggle pause state
    UFUNCTION(BlueprintCallable, Category = "Pause Menu")
    void TogglePause();
    
    // Check if game is paused
    UFUNCTION(BlueprintPure, Category = "Pause Menu")
    bool IsPaused() const { return bIsPaused; }

private:
    bool bIsPaused;
    
    UPROPERTY()
    class UUserWidget* PauseWidget;
    
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UUserWidget> PauseWidgetClass;
};
