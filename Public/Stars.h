// Stars.h - Simple star system for night sky
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Stars.generated.h"

UCLASS()
class MAZERUNNER_API AStars : public AActor
{
    GENERATED_BODY()
    
public:    
    AStars();

protected:
    virtual void BeginPlay() override;

private:
    void CreateStars();
    
    UPROPERTY()
    TArray<class UPointLightComponent*> StarLights;
    
    UPROPERTY(EditAnywhere, Category = "Stars")
    int32 NumberOfStars = 50;
    
    UPROPERTY(EditAnywhere, Category = "Stars")
    float SkyHeight = 4000.0f;
    
    UPROPERTY(EditAnywhere, Category = "Stars")
    float SpreadRadius = 5000.0f;
};
