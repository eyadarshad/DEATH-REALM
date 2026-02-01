// LevelSelectWidget.h
// Main level selection screen with card grid
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/Button.h"
#include "LevelCardWidget.h"
#include "LevelSelectWidget.generated.h"

UCLASS()
class MAZERUNNER_API ULevelSelectWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Level Select")
    void PopulateCards();
    
    UFUNCTION(BlueprintCallable, Category = "Level Select")
    void RefreshCardStates();
    
    UFUNCTION(BlueprintCallable, Category = "Level Select")
    void OnBackButtonClicked();
    
protected:
    virtual void NativeConstruct() override;
    
public:
    // UI Components (bind in Blueprint)
    UPROPERTY(meta = (BindWidget))
    class UWrapBox* LevelGrid;  // Changed to WrapBox for auto-wrapping
    
    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;
    
    // Level card widget class
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Select")
    TSubclassOf<ULevelCardWidget> LevelCardClass;
    
private:
    UPROPERTY()
    TArray<ULevelCardWidget*> LevelCards;
    
    UFUNCTION()
    void HandleBackButtonClicked();
};
