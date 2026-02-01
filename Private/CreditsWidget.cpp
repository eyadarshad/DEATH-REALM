// CreditsWidget.cpp - Rolling credits implementation
#include "CreditsWidget.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "MazeGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/VerticalBoxSlot.h"

void UCreditsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    CreateCreditsText();
}

void UCreditsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    if (!bIsRolling)
        return;
    
    ElapsedTime += InDeltaTime;
    CurrentScrollOffset += ScrollSpeed * InDeltaTime;
    
    // Move credits container upward
    if (CreditsContainer)
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CreditsContainer->Slot);
        if (CanvasSlot)
        {
            FVector2D CurrentPosition = CanvasSlot->GetPosition();
            CurrentPosition.Y = 800.0f - CurrentScrollOffset;  // Start below screen, scroll up
            CanvasSlot->SetPosition(CurrentPosition);
        }
    }
    
    // When credits finish rolling, just stop (don't auto-return)
    if (ElapsedTime >= TotalDuration)
    {
        bIsRolling = false;
        UE_LOG(LogTemp, Warning, TEXT("[Credits] Credits finished rolling - waiting for user to click Return button"));
    }
}

void UCreditsWidget::StartCreditsRoll()
{
    bIsRolling = true;
    CurrentScrollOffset = 0.0f;
    ElapsedTime = 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("[Credits] Credits rolling started!"));
}

void UCreditsWidget::CreateCreditsText()
{
    if (!CreditsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("[Credits] CreditsContainer not found!"));
        return;
    }
    
    // Clear existing content
    CreditsContainer->ClearChildren();
    
    // Helper function to add text
    auto AddCreditLine = [this](const FString& Text, int32 FontSize, FLinearColor Color, float TopPadding)
    {
        UTextBlock* TextBlock = NewObject<UTextBlock>(this);
        if (TextBlock)
        {
            TextBlock->SetText(FText::FromString(Text));
            TextBlock->SetJustification(ETextJustify::Center);
            TextBlock->SetColorAndOpacity(FSlateColor(Color));
            
            FSlateFontInfo FontInfo = TextBlock->GetFont();
            FontInfo.Size = FontSize;
            TextBlock->SetFont(FontInfo);
            
            CreditsContainer->AddChild(TextBlock);
            
            // Add padding
            if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(TextBlock->Slot))
            {
                Slot->SetPadding(FMargin(0, TopPadding, 0, 0));
                Slot->SetHorizontalAlignment(HAlign_Center);
            }
        }
    };
    
    // UPDATED: Blood dark red for headings, white for names, MORE SPACING
    FLinearColor HeadingColor = FLinearColor(0.6f, 0.0f, 0.0f, 1.0f);  // Blood dark red
    FLinearColor NameColor = FLinearColor::White;
    
    int32 HeadingSize = 28;
    int32 NameSize = 18;
    float SectionPadding = 50.0f;  // MORE SPACING (was 25)
    float LinePadding = 12.0f;     // MORE SPACING (was 6)
    
    // Title
    AddCreditLine("DEATH REALM", HeadingSize + 12, HeadingColor, 100.0f);
    AddCreditLine("", NameSize, NameColor, 20.0f);
    
    // Game Design
    AddCreditLine("GAME DESIGN", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("EYAD", NameSize, NameColor, LinePadding);
    
    // Programming
    AddCreditLine("PROGRAMMING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Level Design
    AddCreditLine("LEVEL DESIGN", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // AI Programming
    AddCreditLine("AI PROGRAMMING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Maze Generation
    AddCreditLine("MAZE GENERATION", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // UI/UX Design
    AddCreditLine("UI/UX DESIGN", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Art Direction
    AddCreditLine("ART DIRECTION", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // 3D Modeling
    AddCreditLine("3D MODELING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Texturing
    AddCreditLine("TEXTURING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Lighting
    AddCreditLine("LIGHTING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Sound Design
    AddCreditLine("SOUND DESIGN", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Music Composition
    AddCreditLine("MUSIC COMPOSITION", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Audio Engineering
    AddCreditLine("AUDIO ENGINEERING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Quality Assurance
    AddCreditLine("QUALITY ASSURANCE", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Testing
    AddCreditLine("TESTING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Bug Fixing
    AddCreditLine("BUG FIXING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Performance Optimization
    AddCreditLine("PERFORMANCE OPTIMIZATION", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Producer
    AddCreditLine("PRODUCER", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Project Management
    AddCreditLine("PROJECT MANAGEMENT", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Documentation
    AddCreditLine("DOCUMENTATION", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Marketing
    AddCreditLine("MARKETING", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Community Management
    AddCreditLine("COMMUNITY MANAGEMENT", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Social Media
    AddCreditLine("SOCIAL MEDIA", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Coffee Maker
    AddCreditLine("COFFEE MAKER", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Animation
    AddCreditLine("ANIMATION", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // VFX
    AddCreditLine("VISUAL EFFECTS", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Cinematics
    AddCreditLine("CINEMATICS", HeadingSize, HeadingColor, SectionPadding);
    AddCreditLine("Eyad", NameSize, NameColor, LinePadding);
    
    // Special Thanks Section - FINAL SECTION
    AddCreditLine("", NameSize, NameColor, SectionPadding * 2);
    AddCreditLine("SPECIAL THANKS", HeadingSize + 6, FLinearColor(1.0f, 0.8f, 0.0f, 1.0f), SectionPadding);
    AddCreditLine("", NameSize, NameColor, LinePadding * 2);
    AddCreditLine("🐸", 44, FLinearColor(0.2f, 1.0f, 0.2f, 1.0f), LinePadding);
    AddCreditLine("", NameSize, NameColor, LinePadding);
    AddCreditLine("Ghulam", NameSize + 6, NameColor, LinePadding);
    
    AddCreditLine("", NameSize, NameColor, 300.0f);  // Extra space at end
    
    UE_LOG(LogTemp, Warning, TEXT("[Credits] Credits created with slow 30-second timing"));
}
