// CreditsWidget.h - Rolling credits after Level 5
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CreditsWidget.generated.h"

UCLASS()
class MAZERUNNER_API UCreditsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Credits")
    void StartCreditsRoll();
    
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
public:
    // UI Components
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* CreditsContainer;
    
    // Animation settings - UPDATED for slower, more readable credits
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Credits")
    float ScrollSpeed = 75.0f;  // Slower for better readability
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Credits")
    float TotalDuration = 75.0f;  // 75 seconds total - plenty of time to read all 27 credits
    
private:
    void CreateCreditsText();
    
    float CurrentScrollOffset = 0.0f;
    float ElapsedTime = 0.0f;
    bool bIsRolling = false;
};
