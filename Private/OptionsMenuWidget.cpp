// OptionsMenuWidget.cpp
#include "OptionsMenuWidget.h"
#include "MazeGameMode.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "AudioDevice.h"

void UOptionsMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] === NativeConstruct called ==="));
    
    // Get current settings
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    
    // Initialize Mouse Sensitivity SpinBox
    if (SpinBox_MouseSensitivity)
    {
        SpinBox_MouseSensitivity->SetMinValue(0.1f);
        SpinBox_MouseSensitivity->SetMaxValue(5.0f);
        SpinBox_MouseSensitivity->SetDelta(0.1f);
        
        // Load current sensitivity from GameMode
        float CurrentSensitivity = GameMode ? GameMode->GetMouseSensitivity() : 1.0f;
        SpinBox_MouseSensitivity->SetValue(CurrentSensitivity);
        OriginalMouseSensitivity = CurrentSensitivity;
        
        SpinBox_MouseSensitivity->OnValueChanged.AddDynamic(this, &UOptionsMenuWidget::OnMouseSensitivityChanged);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Mouse sensitivity initialized: %.2f"), CurrentSensitivity);
    }
    
    // Initialize Game Sound SpinBox
    if (SpinBox_GameSound)
    {
        SpinBox_GameSound->SetMinValue(0.0f);
        SpinBox_GameSound->SetMaxValue(1.0f);
        SpinBox_GameSound->SetDelta(0.05f);
        
        // Load current volume from GameMode
        float CurrentVolume = GameMode ? GameMode->GetGameVolume() : 0.8f;
        SpinBox_GameSound->SetValue(CurrentVolume);
        OriginalGameSound = CurrentVolume;
        
        SpinBox_GameSound->OnValueChanged.AddDynamic(this, &UOptionsMenuWidget::OnGameSoundChanged);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Game sound initialized: %.2f"), CurrentVolume);
    }
    
    // Initialize Graphics Quality ComboBox
    if (ComboBox_Graphics)
    {
        ComboBox_Graphics->ClearOptions();
        ComboBox_Graphics->AddOption(TEXT("Low"));
        ComboBox_Graphics->AddOption(TEXT("Medium"));
        ComboBox_Graphics->AddOption(TEXT("High"));
        ComboBox_Graphics->AddOption(TEXT("Ultra High"));
        
        // Set current quality
        FString CurrentQuality = TEXT("High"); // Default
        if (Settings)
        {
            int32 QualityLevel = Settings->GetOverallScalabilityLevel();
            switch (QualityLevel)
            {
                case 0: CurrentQuality = TEXT("Low"); break;
                case 1: CurrentQuality = TEXT("Medium"); break;
                case 2: CurrentQuality = TEXT("High"); break;
                case 3: CurrentQuality = TEXT("Ultra High"); break;
                default: CurrentQuality = TEXT("High"); break;
            }
        }
        
        ComboBox_Graphics->SetSelectedOption(CurrentQuality);
        OriginalGraphicsQuality = CurrentQuality;
        
        ComboBox_Graphics->OnSelectionChanged.AddDynamic(this, &UOptionsMenuWidget::OnGraphicsQualityChanged);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Graphics quality initialized: %s"), *CurrentQuality);
    }
    
    // Bind button click events
    if (Button_Save)
    {
        Button_Save->OnClicked.AddDynamic(this, &UOptionsMenuWidget::OnSaveClicked);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Save button bound"));
    }
    
    if (Button_Cancel)
    {
        Button_Cancel->OnClicked.AddDynamic(this, &UOptionsMenuWidget::OnCancelClicked);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Cancel button bound"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] === Initialization complete ==="));
}

void UOptionsMenuWidget::OnSaveClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Save button clicked - saving settings"));
    
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    
    // Save mouse sensitivity
    if (SpinBox_MouseSensitivity && GameMode)
    {
        float Sensitivity = SpinBox_MouseSensitivity->GetValue();
        GameMode->SetMouseSensitivity(Sensitivity);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Saved mouse sensitivity: %.2f"), Sensitivity);
    }
    
    // Save game sound
    if (SpinBox_GameSound && GameMode)
    {
        float Volume = SpinBox_GameSound->GetValue();
        GameMode->SetGameVolume(Volume);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Saved game volume: %.2f"), Volume);
    }
    
    // Save graphics quality
    if (ComboBox_Graphics && Settings)
    {
        FString Quality = ComboBox_Graphics->GetSelectedOption();
        ApplyGraphicsQuality(Quality);
        Settings->ApplySettings(false);
        Settings->SaveSettings();
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Saved graphics quality: %s"), *Quality);
    }
    
    // Close the options menu
    if (GameMode)
    {
        GameMode->CloseOptionsMenu();
    }
}

void UOptionsMenuWidget::OnCancelClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Cancel button clicked - discarding changes"));
    
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    
    // Restore original values
    if (SpinBox_MouseSensitivity && GameMode)
    {
        GameMode->SetMouseSensitivity(OriginalMouseSensitivity);
    }
    
    if (SpinBox_GameSound)
    {
        // Restore original volume
    }
    
    if (ComboBox_Graphics)
    {
        ApplyGraphicsQuality(OriginalGraphicsQuality);
    }
    
    // Close the options menu
    if (GameMode)
    {
        GameMode->CloseOptionsMenu();
    }
}

void UOptionsMenuWidget::OnMouseSensitivityChanged(float Value)
{
    // Apply in real-time for preview
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->SetMouseSensitivity(Value);
        UE_LOG(LogTemp, Log, TEXT("[OptionsMenu] Mouse sensitivity changed: %.2f"), Value);
    }
}

void UOptionsMenuWidget::OnGameSoundChanged(float Value)
{
    // Apply volume in real-time
    if (UWorld* World = GetWorld())
    {
        FAudioDevice* AudioDevice = World->GetAudioDeviceRaw();
        if (AudioDevice)
        {
            AudioDevice->SetTransientPrimaryVolume(Value);
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[OptionsMenu] Game volume changed: %.2f"), Value);
}

void UOptionsMenuWidget::OnGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType != ESelectInfo::Direct)
    {
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Graphics quality changed: %s"), *SelectedItem);
        // Don't apply immediately - wait for Save button
    }
}

void UOptionsMenuWidget::ApplyGraphicsQuality(const FString& Quality)
{
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (!Settings) return;
    
    if (Quality == TEXT("Low"))
    {
        // Low: Post-processing OFF
        Settings->SetPostProcessingQuality(0);
        Settings->SetShadowQuality(0);
        Settings->SetTextureQuality(0);
        Settings->SetVisualEffectQuality(0);
        Settings->SetOverallScalabilityLevel(0);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied LOW graphics (Post-processing OFF)"));
    }
    else if (Quality == TEXT("Medium"))
    {
        // Medium: Post-processing PARTIALLY ON
        Settings->SetPostProcessingQuality(1);
        Settings->SetShadowQuality(1);
        Settings->SetTextureQuality(1);
        Settings->SetVisualEffectQuality(1);
        Settings->SetOverallScalabilityLevel(1);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied MEDIUM graphics (Post-processing PARTIAL)"));
    }
    else if (Quality == TEXT("High"))
    {
        // High: Post-processing ON with medium-high settings
        Settings->SetPostProcessingQuality(2);
        Settings->SetShadowQuality(2);
        Settings->SetTextureQuality(2);
        Settings->SetVisualEffectQuality(2);
        Settings->SetOverallScalabilityLevel(2);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied HIGH graphics (Post-processing MEDIUM-HIGH)"));
    }
    else if (Quality == TEXT("Ultra High"))
    {
        // Ultra High: MAXIMUM graphics quality - all settings at epic level
        Settings->SetPostProcessingQuality(4);  // Epic post-processing
        Settings->SetShadowQuality(4);          // Epic shadows
        Settings->SetTextureQuality(4);         // Epic textures
        Settings->SetVisualEffectQuality(4);    // Epic VFX
        Settings->SetAntiAliasingQuality(4);    // Epic anti-aliasing
        Settings->SetViewDistanceQuality(4);    // Epic view distance
        Settings->SetFoliageQuality(4);         // Epic foliage
        Settings->SetOverallScalabilityLevel(4); // Epic overall
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied ULTRA HIGH graphics (EPIC quality - maximum settings)"));
    }
}
