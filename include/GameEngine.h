#pragma once
#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <deque>

// ==========================================
// ENUMS & DATA STRUCTURES
// ==========================================

enum GameState {
    STATE_MENU,      
    STATE_INTRO,     
    STATE_GAMEPLAY,
    STATE_MAP,       
    STATE_REST,      
    STATE_SCAVENGE,  
    STATE_OUTRO
};

enum ItemType { FOOD, TOOL, WEAPON, HERB, QUEST };

struct Item {
    std::string name;
    ItemType type;
    int effectValue;
    int quantity;

    Item(std::string n = "", ItemType t = TOOL, int v = 0, int q = 1)
        : name(n), type(t), effectValue(v), quantity(q) {}
};

class Inventory {
private:
    std::vector<Item> items;
public:
    void addItem(Item newItem) {
        for (auto& item : items) {
            if (item.name == newItem.name) {
                item.quantity += newItem.quantity;
                return;
            }
        }
        items.push_back(newItem);
    }

    bool removeOne(std::string itemName) {
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (it->name == itemName) {
                it->quantity--;
                if (it->quantity <= 0) items.erase(it);
                return true;
            }
        }
        return false;
    }

    bool hasItem(std::string name) {
        for (const auto& item : items) {
            if (item.name == name) return true;
        }
        return false;
    }

    void clear() { items.clear(); }
    std::vector<Item> toVector() { return items; }
};

struct WolfStats {
    int health;
    int energy;
    int hunger;
    int reputation;
    int dayCount;
    int packSize;
    int lastRestLevel;
    int lastScavengeLevel;
    bool crossedRiverIce;
    bool hasPack;
    bool blizzardTriggered; // Keep for specific flags if needed
    bool bearTriggered;
    bool eventHappened;     // NEW: Tracks if ANY event has occurred this game

    WolfStats() : health(100), energy(100), hunger(0), reputation(0), 
                  dayCount(1), packSize(0), lastRestLevel(-10), lastScavengeLevel(-10),
                  crossedRiverIce(false), hasPack(false), 
                  blizzardTriggered(false), bearTriggered(false), eventHappened(false) {}
};

struct StoryNode {
    int id;
    std::string text;
    std::string mainImage;
    std::vector<std::pair<std::string, int>> children;
    
    int healthChange;
    int energyChange;
    int hungerChange;
    int reputationChange;
    int dayChange;
    std::string requiredItem;
    std::string rewardItem;
    std::vector<std::string> slideshow; 

    StoryNode(int i = 0, std::string t = "", std::string img = "") 
        : id(i), text(t), mainImage(img), healthChange(0), energyChange(0), 
          hungerChange(0), reputationChange(0), dayChange(0), 
          requiredItem("None"), rewardItem("None") {}
};

struct GameStateData {
    int currentNodeID;
    int returnToNodeID;
    WolfStats stats;
    std::vector<Item> inventorySnapshot;
    std::vector<std::string> logSnapshot;
};

class EventSystem {
    std::vector<int> eventQueue;
public:
    void pushEvent(int id, int priority, std::string name) { eventQueue.push_back(id); }
    int popEvent() {
        if (eventQueue.empty()) return -1;
        int id = eventQueue.back();
        eventQueue.pop_back();
        return id;
    }
    bool isEmpty() { return eventQueue.empty(); }
    void clear() { eventQueue.clear(); }
};

// ==========================================
// GAME ENGINE CLASS
// ==========================================

class GameEngine {
public:
    // Core Data
    std::map<int, StoryNode*> storyMap;
    StoryNode* currentNode;
    WolfStats currentStats;
    Inventory inventory;
    std::vector<std::string> gameLog;
    std::deque<GameStateData> undoStack;
    EventSystem eventSystem;
    
    // State Management
    GameState currentState;
    int returnToNodeID = -1;
    bool gameOver = false;
    bool gameWon = false;

    // Intro Story Data
    std::vector<std::string> introLines;
    int introLineIndex = 0;

    // Graphics Data
    std::map<std::string, unsigned int> textureCache;
    std::map<std::string, std::pair<int, int>> textureSizeCache;

    // Core Functions
    void initGame();
    void cleanup();
    
    // Logic
    void makeChoice(int choiceIndex);
    void checkForRandomEvents(int nextNodeID);
    
    // Actions
    bool performGlobalRest();
    bool performGlobalScavenge();
    void toggleMap();
    void useItem(std::string itemName);

    // Save/Load/Undo
    void saveState();
    void undoLastAction();
    void saveGameToFile(std::string filename = "savegame.txt");
    void loadGameFromFile(std::string filename = "savegame.txt");

    // Texture Utils
    unsigned int getNodeTexture(std::string path);
    unsigned int getGeneralTexture(std::string filename); 
    unsigned int loadTextureFromFile(const char* filename); 
    std::pair<int, int> getTextureSize(std::string path);

    // Helpers
    std::string getFinalTitle();

private:
    void addNode(int id, std::string text, std::string img, int h=0, int e=0, int hu=0, int r=0, int d=0, std::string req="None", std::string rew="None");
    void connect(int parentID, std::string choiceText, int childID);
};

#endif