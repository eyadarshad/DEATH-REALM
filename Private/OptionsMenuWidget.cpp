// OptionsMenuWidget.cpp
#include "OptionsMenuWidget.h"
#include "MazeGameMode.h"
#include "MazeGameSettings.h"
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
        
        // Load saved quality from MazeGameSettings (supports Ultra High)
        UMazeGameSettings* GameSettings = NewObject<UMazeGameSettings>();
        FString CurrentQuality = GameSettings->LoadGraphicsQuality();
        
        ComboBox_Graphics->SetSelectedOption(CurrentQuality);
        OriginalGraphicsQuality = CurrentQuality;
        
        // Apply the loaded quality immediately
        ApplyGraphicsQuality(CurrentQuality);
        
        ComboBox_Graphics->OnSelectionChanged.AddDynamic(this, &UOptionsMenuWidget::OnGraphicsQualityChanged);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Graphics quality loaded and applied: %s"), *CurrentQuality);
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
    
    // Save graphics quality to custom save game
    if (ComboBox_Graphics)
    {
        FString Quality = ComboBox_Graphics->GetSelectedOption();
        ApplyGraphicsQuality(Quality);
        
        // Save to custom save game (supports Ultra High)
        UMazeGameSettings* GameSettings = NewObject<UMazeGameSettings>();
        GameSettings->SaveGraphicsQuality(Quality);
        
        // Also save to UGameUserSettings for engine settings
        if (Settings)
        {
            Settings->ApplySettings(false);
            Settings->SaveSettings();
        }
        
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
        // Low: 70% resolution for better performance
        if (GEngine)
        {
            GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 70"));
        }
        Settings->SetPostProcessingQuality(0);
        Settings->SetShadowQuality(0);
        Settings->SetTextureQuality(0);
        Settings->SetVisualEffectQuality(0);
        Settings->SetOverallScalabilityLevel(0);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied LOW graphics (70%% resolution)"));
    }
    else if (Quality == TEXT("Medium"))
    {
        // Medium: 80% resolution for balanced performance
        if (GEngine)
        {
            GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 80"));
        }
        Settings->SetPostProcessingQuality(1);
        Settings->SetShadowQuality(1);
        Settings->SetTextureQuality(1);
        Settings->SetVisualEffectQuality(1);
        Settings->SetOverallScalabilityLevel(1);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied MEDIUM graphics (80%% resolution)"));
    }
    else if (Quality == TEXT("High"))
    {
        // High: 90% resolution for high quality
        if (GEngine)
        {
            GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 90"));
        }
        Settings->SetPostProcessingQuality(2);
        Settings->SetShadowQuality(2);
        Settings->SetTextureQuality(2);
        Settings->SetVisualEffectQuality(2);
        Settings->SetOverallScalabilityLevel(2);
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied HIGH graphics (90%% resolution)"));
    }
    else if (Quality == TEXT("Ultra High"))
    {
        // Ultra High: 100% native resolution - MAXIMUM graphics quality
        if (GEngine)
        {
            GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
        }
        Settings->SetPostProcessingQuality(4);  // Epic post-processing
        Settings->SetShadowQuality(4);          // Epic shadows
        Settings->SetTextureQuality(4);         // Epic textures
        Settings->SetVisualEffectQuality(4);    // Epic VFX
        Settings->SetAntiAliasingQuality(4);    // Epic anti-aliasing
        Settings->SetViewDistanceQuality(4);    // Epic view distance
        Settings->SetFoliageQuality(4);         // Epic foliage
        Settings->SetOverallScalabilityLevel(4); // Epic overall
        UE_LOG(LogTemp, Warning, TEXT("[OptionsMenu] Applied ULTRA HIGH graphics (100%% resolution - EPIC quality)"));
    }
}
