// GuidelinesWidget.cpp
#include "GuidelinesWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MazeGameMode.h"
#include "Kismet/GameplayStatics.h"

void UGuidelinesWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    PopulateGuidelines();
    
    // Bind close button
    if (Button_Close)
    {
        Button_Close->OnClicked.AddDynamic(this, &UGuidelinesWidget::OnCloseClicked);
    }
}

void UGuidelinesWidget::OnCloseClicked()
{
    AMazeGameMode* GameMode = Cast<AMazeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->CloseGuidelinesMenu();
    }
}

void UGuidelinesWidget::PopulateGuidelines()
{
    if (Text_Guidelines)
    {
        Text_Guidelines->SetText(FText::FromString(GetGuidelinesText()));
    }
}

FString UGuidelinesWidget::GetGuidelinesText()
{
    FString GuideText = TEXT("");
    
    // Title
    GuideText += TEXT("=== DEATH REALM - COMPLETE GUIDE ===\n\n");
    
    // Objective
    GuideText += TEXT("OBJECTIVE\n");
    GuideText += TEXT("Navigate through deadly mazes, find the Golden Star, and reach the escape cell before time runs out. Survive 5 progressively challenging levels to escape the DEATH REALM.\n\n");
    
    // Controls
    GuideText += TEXT("CONTROLS\n");
    GuideText += TEXT("• W, A, S, D - Move forward, left, backward, right\n");
    GuideText += TEXT("• Mouse - Look around and navigate\n");
    GuideText += TEXT("• F - Toggle flashlight on/off\n");
    GuideText += TEXT("• ESC - Pause game / Open menu\n\n");
    
    // Game Mechanics
    GuideText += TEXT("GAME MECHANICS\n");
    GuideText += TEXT("Golden Star: Find the glowing golden star to unlock the escape cell\n");
    GuideText += TEXT("Escape Cell: Green glowing cell - reach it after collecting the star to complete the level\n");
    GuideText += TEXT("Time Limit: Complete each level before time runs out\n");
    GuideText += TEXT("Monster Spawn: A monster appears after 30 seconds - avoid it at all costs!\n\n");
    
    // Hazards
    GuideText += TEXT("HAZARDS & OBSTACLES\n\n");
    
    GuideText += TEXT("1. MUDDY PATCHES (Brown cells)\n");
    GuideText += TEXT("   • Reduces movement speed by 50%%\n");
    GuideText += TEXT("   • Blurs vision significantly\n");
    GuideText += TEXT("   • Effects last 3 seconds after leaving\n");
    GuideText += TEXT("   • Strategy: Avoid when possible, especially when monster is near\n\n");
    
    GuideText += TEXT("2. DANDELION TRAPS (Yellow flowers)\n");
    GuideText += TEXT("   • Immobilizes you completely for 5 seconds\n");
    GuideText += TEXT("   • Cannot move or escape during trap\n");
    GuideText += TEXT("   • Extremely dangerous if monster is nearby\n");
    GuideText += TEXT("   • Strategy: Memorize trap locations, plan alternate routes\n\n");
    
    GuideText += TEXT("3. SAFE ZONES (Purple glowing cells)\n");
    GuideText += TEXT("   • Complete protection from monster\n");
    GuideText += TEXT("   • Monster cannot enter or attack you inside\n");
    GuideText += TEXT("   • Available from Level 4 onwards\n");
    GuideText += TEXT("   • Strategy: Use as temporary shelter when monster is close\n\n");
    
    GuideText += TEXT("4. THE MONSTER\n");
    GuideText += TEXT("   • Spawns after 30 seconds\n");
    GuideText += TEXT("   • Intelligent pathfinding - actively hunts you\n");
    GuideText += TEXT("   • Speed increases over time\n");
    GuideText += TEXT("   • EXTREMELY dangerous in final 30 seconds\n");
    GuideText += TEXT("   • Instant death on contact\n");
    GuideText += TEXT("   • Strategy: Keep moving, use maze walls as barriers\n\n");
    
    // Level Progression
    GuideText += TEXT("LEVEL PROGRESSION\n\n");
    
    GuideText += TEXT("LEVEL 1: TUTORIAL (10x10 maze)\n");
    GuideText += TEXT("• Learn basic controls and mechanics\n");
    GuideText += TEXT("• No monster, no hazards\n");
    GuideText += TEXT("• Find the star and escape\n\n");
    
    GuideText += TEXT("LEVEL 2: THE HUNT BEGINS (15x15 maze)\n");
    GuideText += TEXT("• Monster appears after 30 seconds\n");
    GuideText += TEXT("• Larger maze with more dead ends\n");
    GuideText += TEXT("• Time management becomes critical\n\n");
    
    GuideText += TEXT("LEVEL 3: TRAPPED (20x20 maze)\n");
    GuideText += TEXT("• Dandelion traps introduced\n");
    GuideText += TEXT("• Getting trapped = likely death\n");
    GuideText += TEXT("• Plan your route carefully\n\n");
    
    GuideText += TEXT("LEVEL 4: SAFE HAVEN (25x25 maze)\n");
    GuideText += TEXT("• Safe zones available\n");
    GuideText += TEXT("• Muddy patches slow you down\n");
    GuideText += TEXT("• Use safe zones strategically\n\n");
    
    GuideText += TEXT("LEVEL 5: BLOOD MOON (30x30 maze)\n");
    GuideText += TEXT("• FINAL CHALLENGE\n");
    GuideText += TEXT("• All hazards present\n");
    GuideText += TEXT("• Red sky, enhanced danger\n");
    GuideText += TEXT("• Monster is fastest and most aggressive\n");
    GuideText += TEXT("• Ultimate test of skill and strategy\n\n");
    
    // Tips & Strategies
    GuideText += TEXT("TIPS & STRATEGIES\n\n");
    
    GuideText += TEXT("Time Management:\n");
    GuideText += TEXT("• First 30 seconds: Explore quickly, find the star\n");
    GuideText += TEXT("• After monster spawns: Balance speed with caution\n");
    GuideText += TEXT("• Final 30 seconds: Maximum urgency, monster is extremely fast\n\n");
    
    GuideText += TEXT("Navigation:\n");
    GuideText += TEXT("• Use flashlight in dark areas\n");
    GuideText += TEXT("• Memorize key paths and dead ends\n");
    GuideText += TEXT("• Keep mental map of hazard locations\n");
    GuideText += TEXT("• Listen for audio cues (star hum, monster sounds)\n\n");
    
    GuideText += TEXT("Monster Evasion:\n");
    GuideText += TEXT("• Never stop moving when monster is active\n");
    GuideText += TEXT("• Use maze corners to break line of sight\n");
    GuideText += TEXT("• Safe zones are your best friend (Level 4+)\n");
    GuideText += TEXT("• Avoid dead ends - they're death traps\n\n");
    
    GuideText += TEXT("Hazard Management:\n");
    GuideText += TEXT("• Muddy patches: Acceptable risk if no monster nearby\n");
    GuideText += TEXT("• Dandelion traps: AVOID at all costs after monster spawns\n");
    GuideText += TEXT("• Plan alternate routes around hazards\n\n");
    
    // Advanced Techniques
    GuideText += TEXT("ADVANCED TECHNIQUES\n");
    GuideText += TEXT("• Backtracking: Sometimes going back is faster than forward\n");
    GuideText += TEXT("• Wall-following: Stick to one wall to eventually find everything\n");
    GuideText += TEXT("• Sprint planning: Know when to risk hazards vs. take safe route\n");
    GuideText += TEXT("• Safe zone timing: Enter just before monster catches up\n\n");
    
    // Graphics Settings
    GuideText += TEXT("GRAPHICS SETTINGS\n");
    GuideText += TEXT("Adjust in Options menu for best performance:\n");
    GuideText += TEXT("• Low: 70%% resolution - maximum FPS\n");
    GuideText += TEXT("• Medium: 80%% resolution - balanced\n");
    GuideText += TEXT("• High: 90%% resolution - good quality\n");
    GuideText += TEXT("• Ultra High: 100%% resolution - best quality (default)\n\n");
    
    // Final Tips
    GuideText += TEXT("FINAL TIPS\n");
    GuideText += TEXT("• Stay calm under pressure\n");
    GuideText += TEXT("• Learn from each death\n");
    GuideText += TEXT("• Practice makes perfect\n");
    GuideText += TEXT("• The monster can be outsmarted\n");
    GuideText += TEXT("• Every level is beatable\n\n");
    
    GuideText += TEXT("Good luck, and may you survive the DEATH REALM!\n");
    
    return GuideText;
}
