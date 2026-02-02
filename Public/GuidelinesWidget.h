// GuidelinesWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GuidelinesWidget.generated.h"

/**
 * Guidelines widget - shows comprehensive game instructions, controls, tips, and rules
 */
UCLASS()
class MAZERUNNER_API UGuidelinesWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    virtual void NativeConstruct() override;
    
protected:
    UPROPERTY(meta = (BindWidgetOptional))
    class UScrollBox* ScrollBox_Guidelines;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* Text_Guidelines;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* Button_Close;
    
    UFUNCTION()
    void OnCloseClicked();
    
private:
    void PopulateGuidelines();
    FString GetGuidelinesText();
};
