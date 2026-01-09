#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <string>
#include <vector>
#include <iostream>

// ==========================================
// ENUMS
// ==========================================
enum ItemType { FOOD, HERB, TOOL, WEAPON };
enum GameState { STATE_MENU, STATE_BACKSTORY, STATE_GAMEPLAY, STATE_OUTRO }; 

// ==========================================
// STRUCTS
// ==========================================

struct WolfStats {
    int health = 100;
    int energy = 100;
    int hunger = 0;
    int reputation = 0;
    int dayCount = 1;
    int packSize = 0;
    
    // Cooldown trackers
    int lastRestLevel = -10;
    int lastScavengeLevel = -10;

    // Flags
    bool crossedRiverIce = false;
    bool hasPack = false;
    bool blizzardTriggered = false;
    bool bearTriggered = false;
};

struct Item {
    std::string name;
    ItemType type;
    int effectValue;
    int quantity;
    
    Item(std::string n, ItemType t, int e, int q) 
        : name(n), type(t), effectValue(e), quantity(q) {}
    Item() {}
};

// Used for Undo System
struct GameStateData {
    int currentNodeID;
    int returnToNodeID;
    WolfStats stats;
    std::vector<Item> inventorySnapshot;
    std::vector<std::string> logSnapshot;
};

struct StoryNode {
    int id;
    std::string text;
    std::string mainImage;
    
    // Using pair for simple choice text -> child ID mapping
    // This matches what you use in GameEngine.cpp
    std::vector<std::pair<std::string, int>> children; 
    
    // Stats changes
    int healthChange = 0;
    int energyChange = 0;
    int hungerChange = 0;
    int reputationChange = 0;
    int dayChange = 0;

    // Requirements & Rewards
    std::string requiredItem = "None";
    std::string rewardItem = "None";

    // Slideshow images (for events)
    std::vector<std::string> slideshow;

    StoryNode(int i, std::string t, std::string img) : id(i), text(t), mainImage(img) {}
    StoryNode() {}
};

#endif