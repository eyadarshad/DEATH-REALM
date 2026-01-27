// SettingsMenuWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenuWidget.generated.h"

/**
 * Settings menu widget - handles all UI logic in C++
 * Just add the visual elements in UMG Designer with the correct names
 */
UCLASS()
class MAZERUNNER_API USettingsMenuWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    virtual void NativeConstruct() override;
    
protected:
    // UI Components - will be bound automatically by name
    // Use BindWidgetOptional so it doesn't crash if not found
    UPROPERTY(meta = (BindWidgetOptional))
    class USpinBox* SpinBox_Rows;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class USpinBox* SpinBox_Cols;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* Button_Save;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* Button_Cancel;
    
    // Button click handlers
    UFUNCTION(BlueprintCallable, Category = "Free2Play")
    void StartGame();  // Generate maze with custom settings and start game
    
    UFUNCTION(BlueprintCallable, Category = "Free2Play")
    void BackToMainMenu();  // Return to main menu
    
    // SpinBox value changed handlers (optional - for live preview)
    UFUNCTION()
    void OnRowsChanged(float Value);
    
    UFUNCTION()
    void OnColsChanged(float Value);
};
