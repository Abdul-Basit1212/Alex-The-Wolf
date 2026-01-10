#pragma once
#ifdef NOMINMAX
#undef NOMINMAX
#endif
#define NOMINMAX
#ifndef GAMEENGINE_H
#define GAMEENGINE_H
#include <windows.h> 
#include <mmsystem.h> 
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <deque>
#include "Inventory.h" 

enum GameState {
    STATE_MENU,      
    STATE_INTRO,     
    STATE_GAMEPLAY,
    STATE_MAP,       
    STATE_REST,      
    STATE_SCAVENGE,  
    STATE_OUTRO
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
    bool blizzardTriggered; 
    bool bearTriggered;
    bool eventHappened;     

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
    std::vector<Item> inventorySnapshot; // Works because Item is in Inventory.h
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

class GameEngine {
public:
    std::map<int, StoryNode*> storyMap;
    StoryNode* currentNode;
    WolfStats currentStats;
    
    // UPDATED: Use the class from Inventory.h
    InventoryList inventory; 
    
    std::vector<std::string> gameLog;
    std::deque<GameStateData> undoStack;
    EventSystem eventSystem;
    
    GameState currentState;
    int returnToNodeID = -1;
    bool gameOver = false;
    bool gameWon = false;

    // Typewriter Data
    std::string currentDisplayedText;
    std::string targetText;
    int textCharIndex = 0;
    float textTimer = 0.0f;
    bool textFinished = false;

    std::vector<std::string> introLines;
    int introLineIndex = 0;

    std::map<std::string, unsigned int> textureCache;
    std::map<std::string, std::pair<int, int>> textureSizeCache;

    // Audio Data
    std::string currentMusicAlias = "";
    bool isMuted = false;

    void initGame();
    void cleanup();
    
    void makeChoice(int choiceIndex);
    void checkForRandomEvents(int nextNodeID);
    void updateTypewriter(float deltaTime); 
    void skipTypewriter(); 
    
    bool performGlobalRest();
    bool performGlobalScavenge();
    void toggleMap();
    void useItem(std::string itemName);

    void saveState();
    void undoLastAction();
    void saveGameToFile(std::string filename = "savegame.txt");
    void loadGameFromFile(std::string filename = "savegame.txt");

    unsigned int getNodeTexture(std::string path);
    unsigned int getGeneralTexture(std::string filename); 
    unsigned int loadTextureFromFile(const char* filename); 
    std::pair<int, int> getTextureSize(std::string path);

    // Audio Functions
    void playSound(std::string filename, std::string alias, bool loop = false);
    void stopSound(std::string alias);
    void playBackgroundMusic(std::string trackName);
    void updateMusicSystem();
    void toggleMute();

    std::string getFinalTitle();

private:
    void addNode(int id, std::string text, std::string img, int h=0, int e=0, int hu=0, int r=0, int d=0, std::string req="None", std::string rew="None");
    void connect(int parentID, std::string choiceText, int childID);
};

#endif