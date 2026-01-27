// LoadingScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

/**
 * Loading screen widget - shows during level transitions
 * Design: Rotating spinner on left, instructions on right, in semi-transparent box at bottom
 */
UCLASS()
class MAZERUNNER_API ULoadingScreenWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
    // Widget bindings
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* Image_Spinner;  // Rotating arrow/spinner
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* Text_Instructions;  // Loading instructions
    
    // Set loading message/instructions
    UFUNCTION(BlueprintCallable, Category = "Loading")
    void SetInstructions(const FString& Instructions);
    
private:
    float RotationAngle = 0.0f;
    float RotationSpeed = 180.0f;  // Degrees per second
    
    // Tips rotation
    float TipTimer = 0.0f;
    float TipChangeInterval = 5.0f;  // Change tip every 5 seconds
    int32 CurrentTipIndex = 0;
    TArray<FString> GameplayTips;
    
    void InitializeGameplayTips();
    void ShowNextTip();
};
