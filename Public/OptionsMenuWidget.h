// OptionsMenuWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OptionsMenuWidget.generated.h"

/**
 * Options menu widget - handles mouse sensitivity, sound, and graphics settings
 */
UCLASS()
class MAZERUNNER_API UOptionsMenuWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    virtual void NativeConstruct() override;
    
protected:
    // UI Components - will be bound automatically by name
    UPROPERTY(meta = (BindWidgetOptional))
    class USpinBox* SpinBox_MouseSensitivity;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class USpinBox* SpinBox_GameSound;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UComboBoxString* ComboBox_Graphics;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* Button_Save;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* Button_Cancel;
    
    // Button click handlers
    UFUNCTION()
    void OnSaveClicked();
    
    UFUNCTION()
    void OnCancelClicked();
    
    // Value changed handlers
    UFUNCTION()
    void OnMouseSensitivityChanged(float Value);
    
    UFUNCTION()
    void OnGameSoundChanged(float Value);
    
    UFUNCTION()
    void OnGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    
private:
    // Store original values for cancel functionality
    float OriginalMouseSensitivity;
    float OriginalGameSound;
    FString OriginalGraphicsQuality;
    
    // Apply graphics settings based on quality level
    void ApplyGraphicsQuality(const FString& Quality);
};
