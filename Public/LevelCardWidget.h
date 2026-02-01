// LevelCardWidget.h - REDESIGNED
// New flip-style card with hover animations
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "LevelCardWidget.generated.h"

UCLASS()
class MAZERUNNER_API ULevelCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Initialize card with level data
    UFUNCTION(BlueprintCallable, Category = "Level Card")
    void InitializeCard(int32 InLevelNumber, bool bInIsLocked, int32 InStars, float InBestTime, const FString& InLevelName);
    
    // Update card visuals
    UFUNCTION(BlueprintCallable, Category = "Level Card")
    void UpdateCardVisuals();
    
    // Hover effects
    UFUNCTION(BlueprintCallable, Category = "Level Card")
    void OnCardHoverStart();
    
    UFUNCTION(BlueprintCallable, Category = "Level Card")
    void OnCardHoverEnd();
    
    // Click handling
    UFUNCTION(BlueprintCallable, Category = "Level Card")
    void OnCardClicked();
    
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
public:
    // Level data
    UPROPERTY(BlueprintReadOnly, Category = "Level Card")
    int32 LevelNumber = 1;
    
    UPROPERTY(BlueprintReadOnly, Category = "Level Card")
    bool bIsLocked = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Level Card")
    int32 StarsEarned = 0;
    
    UPROPERTY(BlueprintReadOnly, Category = "Level Card")
    float BestTime = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Level Card")
    FString LevelName;
    
    // UI Components (bind in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UButton* CardButton;
    
    // Front side (default view)
    UPROPERTY(meta = (BindWidget))
    UImage* TopImage;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TopText1;
    
    // Back side (hover view)
    UPROPERTY(meta = (BindWidget))
    UImage* BackImage;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* BackText1;
    
    // Animation properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float HoverScale = 1.15f;  // Scale up by 15%
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AnimationSpeed = 8.0f;  // Smooth interpolation speed
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float TopImageHoverOpacity = 0.4f;  // Opacity when hovered
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Responsive")
    float BaseCardWidth = 280.0f;  // Base width at 1920x1080
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Responsive")
    float BaseCardHeight = 400.0f;  // Base height at 1920x1080
    
private:
    UFUNCTION()
    void HandleCardClicked();
    
    // Animation state
    bool bIsHovered = false;
    float CurrentScale = 1.0f;
    float TargetScale = 1.0f;
    float CurrentTopOpacity = 1.0f;
    float TargetTopOpacity = 1.0f;
    float CurrentBackOpacity = 0.0f;
    float TargetBackOpacity = 0.0f;
};
