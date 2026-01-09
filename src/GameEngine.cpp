#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GameEngine.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream> 
#include <glfw3.h> 
#include <windows.h> 

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// =========================================================
// TEXTURE UTILS
// =========================================================

unsigned int GameEngine::getNodeTexture(std::string path) {
    if (path.empty()) return 0;
    if (textureCache.find(path) != textureCache.end()) return textureCache[path];
    unsigned int texID = loadTextureFromFile(path.c_str());
    textureCache[path] = texID;
    return texID;
}

unsigned int GameEngine::getGeneralTexture(std::string filename) {
     if (filename.empty()) return 0;
     if (textureCache.find(filename) != textureCache.end()) return textureCache[filename];
     unsigned int texID = loadTextureFromFile(filename.c_str());
     textureCache[filename] = texID;
     return texID;
}

unsigned int GameEngine::loadTextureFromFile(const char* filename) {
    std::string fn = filename;
    
    std::vector<std::string> pathsToCheck = {
        fn,                                     
        "Icons/" + fn,                          
        "Images/" + fn,                         
        "Assets/Icons/" + fn,                   
        "Assets/Images/" + fn,                  
        "../Icons/" + fn,                       
        "../Images/" + fn,                      
        "../Assets/Icons/" + fn,                
        "../Assets/Images/" + fn                
    };

    std::string validPath = "";
    for (const auto& path : pathsToCheck) {
        std::ifstream check(path);
        if (check.good()) {
            validPath = path;
            break;
        }
    }

    if (validPath.empty()) {
        std::cout << "Texture not found: " << filename << std::endl; 
        return 0; 
    }

    int width, height, nrChannels;
    unsigned char* data = stbi_load(validPath.c_str(), &width, &height, &nrChannels, 0);
    if (!data) return 0; 
    
    textureSizeCache[filename] = {width, height};
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (nrChannels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    else if (nrChannels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return textureID;
}

std::pair<int, int> GameEngine::getTextureSize(std::string path) { 
    if(textureSizeCache.count(path)) return textureSizeCache[path];
    return {0, 0}; 
}

// =========================================================
// INPUT HANDLING
// =========================================================

void GameEngine::toggleMap() {
    if (currentState == STATE_GAMEPLAY) currentState = STATE_MAP;
    else if (currentState == STATE_MAP) currentState = STATE_GAMEPLAY;
}

// =========================================================
// UNDO & SAVE SYSTEM
// =========================================================

void GameEngine::saveState() {
    GameStateData state;
    state.currentNodeID = currentNode ? currentNode->id : 1;
    state.returnToNodeID = returnToNodeID;
    state.stats = currentStats;
    state.inventorySnapshot = inventory.toVector(); 
    state.logSnapshot = gameLog; 

    undoStack.push_back(state);
    if (undoStack.size() > 5) undoStack.pop_front();
}

void GameEngine::undoLastAction() {
    if (undoStack.empty()) {
        gameLog.push_back(">> Cannot Undo (Stack Empty)");
        return;
    }

    GameStateData state = undoStack.back();
    undoStack.pop_back();

    if (storyMap.count(state.currentNodeID)) 
        currentNode = storyMap[state.currentNodeID];
    
    returnToNodeID = state.returnToNodeID;
    currentStats = state.stats;
    gameLog = state.logSnapshot;
    gameLog.push_back(">> UNDO PERFORMED");

    inventory.clear();
    for (const auto& item : state.inventorySnapshot) {
        inventory.addItem(item);
    }
    
    gameOver = false; 
    gameWon = false;
}

void GameEngine::saveGameToFile(std::string filename) {
    if (filename.empty()) filename = "savegame";
    if (filename.find(".txt") == std::string::npos) filename += ".txt";

    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << (currentNode ? currentNode->id : 1) << "\n";
    file << currentStats.health << " " << currentStats.hunger << " " 
         << currentStats.energy << " " << currentStats.reputation << " " 
         << currentStats.dayCount << "\n";
    
    file << currentStats.eventHappened << "\n"; 

    std::vector<Item> items = inventory.toVector();
    file << items.size() << "\n";
    for (const auto& item : items) {
        file << item.name << "\n" << item.type << "\n" 
             << item.effectValue << "\n" << item.quantity << "\n";
    }
    file.close();
    gameLog.push_back(">> GAME SAVED to " + filename);
}

void GameEngine::loadGameFromFile(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        gameLog.push_back(">> SAVE FILE NOT FOUND");
        return;
    }

    int nodeID;
    file >> nodeID;
    if (storyMap.count(nodeID)) currentNode = storyMap[nodeID];

    file >> currentStats.health >> currentStats.hunger 
         >> currentStats.energy >> currentStats.reputation 
         >> currentStats.dayCount;
    
    if (file.peek() != EOF) {
        file >> currentStats.eventHappened;
    } else {
        currentStats.eventHappened = false;
    }

    inventory.clear();
    int count;
    file >> count;
    std::string temp; std::getline(file, temp); 

    for(int i=0; i<count; i++) {
        std::string name;
        int type, val, qty;
        std::getline(file, name);
        file >> type >> val >> qty;
        std::getline(file, temp); 
        inventory.addItem(Item(name, (ItemType)type, val, qty));
    }
    file.close();
    gameLog.push_back(">> GAME LOADED");
}

// =========================================================
// GAME ACTIONS
// =========================================================

void ClampStats(WolfStats& s) {
    if (s.health < 0) s.health = 0; if (s.health > 100) s.health = 100;
    if (s.energy < 0) s.energy = 0; if (s.energy > 100) s.energy = 100;
    if (s.hunger < 0) s.hunger = 0; if (s.hunger > 100) s.hunger = 100;
    if (s.reputation < 0) s.reputation = 0; if (s.reputation > 100) s.reputation = 100;
}

std::string GameEngine::getFinalTitle() {
    if (currentStats.reputation >= 80) return "Legendary Alpha";
    if (currentStats.reputation >= 40) return "Pack Leader";
    return "Lone Survivor";
}

bool GameEngine::performGlobalRest() {
    if (currentState != STATE_GAMEPLAY) return false;
    
    if ((currentNode->id - currentStats.lastRestLevel) < 5) {
        gameLog.push_back("Cannot Rest: Unsafe area or rested recently.");
        return false; 
    }

    currentState = STATE_REST; 
    saveState();
    
    currentStats.lastRestLevel = currentNode->id;
    currentStats.energy = std::min(100, currentStats.energy + 40);
    currentStats.health = std::min(100, currentStats.health + 20); 
    currentStats.hunger += 10; 
    currentStats.dayCount++;
    ClampStats(currentStats);
    gameLog.push_back("Rested (+20 HP, +40 Energy).");
    return true; 
}

bool GameEngine::performGlobalScavenge() {
    if (currentState != STATE_GAMEPLAY) return false;

    if ((currentNode->id - currentStats.lastScavengeLevel) < 3) {
        gameLog.push_back("Nothing to scavenge here.");
        return false;
    }

    if (currentStats.energy <= 10) { 
        gameLog.push_back("Too tired to scavenge."); 
        return false;
    }
    
    currentState = STATE_SCAVENGE; 
    saveState();
    
    currentStats.lastScavengeLevel = currentNode->id;
    currentStats.energy -= 10;
    currentStats.dayCount++;
    
    int randVal = rand() % 100;
    if (randVal < 60) {
        inventory.addItem(Item("Meat", FOOD, 30, 1));
        gameLog.push_back("Found Meat!");
    } else {
        gameLog.push_back("Found nothing.");
    }
    ClampStats(currentStats);
    return true;
}

void GameEngine::useItem(std::string itemName) {
    if (itemName == "Map") {
        toggleMap(); 
        return;
    }
    if (inventory.removeOne(itemName)) {
        if (itemName == "Meat") {
            currentStats.hunger = std::max(0, currentStats.hunger - 30);
            gameLog.push_back("Ate Meat (-30 Hunger).");
        }
    }
}

// =========================================================
// CORE LOGIC & RANDOM EVENTS
// =========================================================

void GameEngine::makeChoice(int choiceIndex) {
    if (gameOver || gameWon || !currentNode) return;
    saveState();
    
    currentStats.hunger += 5; 
    currentStats.energy -= 5; 

    if (choiceIndex >= currentNode->children.size()) return;
    int nextID = currentNode->children[choiceIndex].second;
    
    // --- 1. HANDLE RETURN FROM EVENT (-99) ---
    if (nextID == -99) {
        if (returnToNodeID != -1 && storyMap.count(returnToNodeID)) {
            currentNode = storyMap[returnToNodeID];
            returnToNodeID = -1; 
            gameLog.push_back("You continue on your journey...");
        } else {
            currentNode = storyMap[1]; 
        }
        return; 
    }

    // --- 2. RANDOM EVENT TRIGGER ---
    bool isTransitioningToEnding = (nextID == 997 || nextID == 999);
    int eventID = -1;

    if (!isTransitioningToEnding && nextID != -99) {
        // STRICT RANGE CHECKS
        
        // Blizzard: Levels 9-12
        if (nextID >= 9 && nextID <= 12 && !currentStats.eventHappened) {
            int roll = rand() % 100;
            if (roll < 30) eventID = 901; 
        }
        // Bear: Levels 13-16
        else if (nextID >= 13 && nextID <= 16 && !currentStats.eventHappened) {
            int roll = rand() % 100;
            if (roll < 30) eventID = 902;
        }
        // Force Event: ONLY if entering Level 17 (Leaving Zone 16) and nothing happened
        // Fixed: Use '==' to avoid triggering on branch IDs (101, 102)
        else if (nextID == 17 && !currentStats.eventHappened) {
            eventID = 902; // Force Bear
        }

        if (eventID != -1) {
            returnToNodeID = nextID; 
            nextID = eventID;        
            currentStats.eventHappened = true; 
            gameLog.push_back(">> A RANDOM EVENT INTERRUPTS YOUR PATH!");
        }
    }

    // --- 3. APPLY NEXT NODE & REWARDS ---
    if (storyMap.find(nextID) != storyMap.end()) {
        currentNode = storyMap[nextID];
        
        // Update Stats
        currentStats.health += currentNode->healthChange;
        currentStats.energy += currentNode->energyChange;
        currentStats.hunger += currentNode->hungerChange;
        currentStats.reputation += currentNode->reputationChange;
        currentStats.dayCount += currentNode->dayChange;

        // --- ITEM REWARD LOGIC (Was missing) ---
        if (currentNode->rewardItem == "Meat") {
            inventory.addItem(Item("Meat", FOOD, 30, 1));
            gameLog.push_back(">> GAINED: Meat");
        }
        else if (currentNode->rewardItem == "Herbs") {
            inventory.addItem(Item("Herbs", HERB, 20, 1));
            gameLog.push_back(">> GAINED: Herbs");
        }

        // Manual Multi-Reward Override for Bosses (Give both!)
        if (currentNode->id == 9021 || currentNode->id == 2001) { 
             inventory.addItem(Item("Herbs", HERB, 20, 1));
             gameLog.push_back(">> GAINED: Herbs (Bonus)");
        }
    }
    
    ClampStats(currentStats);

    if (currentStats.health <= 0) {
        gameOver = true;
        currentNode = storyMap[997]; 
    }
    if (currentStats.hunger >= 100) {
        gameOver = true;
        currentNode = storyMap[997]; 
    }
}

void GameEngine::checkForRandomEvents(int nextNodeID) { }

// =========================================================
// INIT
// =========================================================

void GameEngine::addNode(int id, std::string text, std::string img, int h, int e, int hu, int r, int d, std::string req, std::string rew) {
    StoryNode* node = new StoryNode(id, text, img);
    node->healthChange = h;
    node->energyChange = e;
    node->hungerChange = hu;
    node->reputationChange = r;
    node->dayChange = d;
    node->requiredItem = req;
    node->rewardItem = rew;
    storyMap[id] = node;
}

void GameEngine::connect(int parentID, std::string choiceText, int childID) {
    if (storyMap.count(parentID)) storyMap[parentID]->children.push_back({choiceText, childID});
}

void GameEngine::cleanup() {
    for(auto const& [key, val] : storyMap) delete val;
    storyMap.clear();
}

void GameEngine::initGame() {
    srand(time(0));
    
    currentStats = WolfStats(); 
    currentStats.lastRestLevel = -10; 
    currentStats.lastScavengeLevel = -10;
    ClampStats(currentStats);

    inventory.clear(); 
    storyMap.clear();
    gameLog.clear();
    undoStack.clear();
    eventSystem.clear();
    textureCache.clear();
    introLines.clear();
    
    gameLog.push_back("--- NEW GAME STARTED ---");
    gameOver = false;
    gameWon = false;
    
    // Set State to MENU initially
    currentState = STATE_MENU; 

    inventory.addItem(Item("Map", TOOL, 0, 1));

    // ========================================
    // INITIAL STORY (Intro Text)
    // ========================================
    introLines.push_back("In the heart of a vast forest, the Nightclaw clan lived under the guidance of Aeron Nightclaw and his mate Sera Silverpaw.");
    introLines.push_back("Together they had a small child, Alex Nightclaw. But dark times approached.");
    introLines.push_back("A hunger crisis struck. Zolver Nighttreaver, ambitious and cruel, plotted to overthrow the alpha.");
    introLines.push_back("One night, while the forest slept, Zolver attacked. Aeron and Sera were murdered.");
    introLines.push_back("You are Alex. Alone. Vulnerable. You must survive.");

    // ========================================
    // GAME NODES (Level 1 Starts Here)
    // ========================================

    // --- LEVEL 1-9 ---
    addNode(1, "LEVEL 1: Frozen Awakening\nThe cold bites deep, sharper than a blade. You wake to a deafening silence. The pack is gone. You must move, or you will freeze.", "1.png", 0, 0, 0, 0, 0);
    connect(1, "Search for Shelter", 101); 
    connect(1, "Search for Food", 102);    

    // Branch A: Shelter
    addNode(101, "You find a hollow beneath the roots of an ancient pine. The shivering stops as warmth slowly returns to your stiff limbs.", "1(a).png", +15, 0, +10, 0, 0, "None", "None");
    connect(101, "Continue", 2);

    // Branch B: Search for Food
    addNode(102, "Your nose catches a faint metallic scent. Digging through the drift, you find a frozen carcass. It isn't much, but the meat fuels your fire.", "1(b).png", 0, 0, -15, 0, 0, "None", "Meat");
    connect(102, "Continue", 2);
    
    addNode(2, "LEVEL 2: Echoes in the Snow\nEvery snapped twig sounds like a gunshot. Ghostly echoes of familiar howls play tricks on your ears.", "2.png", 0, 0, 0, 0, 1);
    connect(2, "Move Carefully", 3); 
    connect(2, "Move Fast", 3);      

    // --- LEVEL 3: The First Hunger ---
    addNode(3, "LEVEL 3: The First Hunger\nYour stomach twists in knots. It has been days since the kill. You need to eat soon, or your body will fail you.", "3.png", -5, -10, 0, 5, 1);
    connect(3, "Hunt Rabbit", 301);     
    connect(3, "Forage Herbs", 302);    

    // Branch A: Hunt Rabbit
    addNode(301, "The white hare freezes. You lunge—a blur of fur and teeth. The chase is short. You claim your prize.", "3(a).png", 0, 0, -10, 0, 0, "None", "Meat");
    connect(301, "Continue", 4);

    // Branch B: Forage Herbs
    addNode(302, "Beneath the ice-crusted brush, you find green shoots. They are bitter, but they possess the old magic of healing.", "3(b).png", 0, 0, -10, 0, 0, "None", "Herbs");
    connect(302, "Continue", 4);    

    // --- LEVEL 4: Silent Trees ---
    addNode(4, "LEVEL 4: Silent Trees\nThe birds have stopped singing. The forest is holding its breath. You are being watched by something unseen.", "4.png", 0, 0, 0, 0, 1);
    connect(4, "Hide", 401); 
    connect(4, "Run", 402);  

    // Branch A: Hide
    addNode(401, "You press your belly to the snow, becoming a shadow. Hours pass. Your stomach screams, but the predator passes without seeing you.", "4(a).png", -5, -5, +5, 0, 0);
    connect(401, "Continue", 5);

    // Branch B: Run
    addNode(402, "You explode into a sprint, tearing through the brambles. Lungs burning, you put miles between you and the eyes in the dark.", "4(b).png", -5, -15, +10, 0, 0);
    connect(402, "Continue", 5);

    addNode(5, "LEVEL 5: First Blood\nThe scent of raw meat is intoxicating. Do you feast now to heal, or save rations for the cruel night?", "5.png", 0, 0, 0, 0, 1);
    connect(5, "Eat Now (Heal)", 501);  
    connect(5, "Save Food", 502);

    // Branch A: Eat
    addNode(501, "You tear into the meat. The warmth spreads through your chest. You feel revitalized.", "5.png", +20, 0, -35, 0, 0, "Meat", "None");
    connect(501, "Continue", 6);

    // Branch B: Save
    addNode(502, "You bury the meat deep in the snow to mask the scent, saving it for the journey ahead.", "5.png", 0, 0, 0, 0, 0);
    connect(502, "Continue", 6);

    addNode(6, "LEVEL 6: Cold Night\nDarkness swallows the trees. Fatigue pulls at your eyelids, but shadows move in the distance. To sleep is to trust the dark.", "6.png", 0, 0, 0, 0, 1);
    connect(6, "Sleep", 601);    
    connect(6, "Stay Alert", 602); 

    // Branch A: Sleep
    addNode(601, "You curl into a tight ball to preserve heat. You sleep deeply, restoring your health and energy.", "6(a).png", +15, +25, 0, 0, 0);
    connect(601, "Continue", 7);

    // Branch B: Stay Alert
    addNode(602, "You force your eyes open, watching the shadows. You are tired, but you are safe.", "6.png", 0, 0, -10, +10, 0);
    connect(602, "Continue", 7);
 
    // LEVEL 7: A Distant Howl
    addNode(7, "LEVEL 7: A Distant Howl\nA howl cuts through the frost. It is not Zolver's pack. Do you answer and risk exposure, or remain a ghost?", "7.png", 0, 0, 0, 0, 1);
    connect(7, "Follow Howl", 701); 
    connect(7, "Ignore", 702);      

    addNode(701, "You return the call, your voice rising to the stars. The response is welcoming. You feel less alone.", "7.png", 0, 0, 0, +10, 0);
    connect(701, "Continue", 8);

    addNode(702, "You stay silent, letting the howl fade into the wind. You remain a ghost in the night.", "7.png", 0, 0, 0, 0, 0);
    connect(702, "Continue", 8);    

    // LEVEL 8: Hidden Claws
    addNode(8, "LEVEL 8: Hidden Claws\nThe snow explodes! A rogue wolf, desperate and feral, crashes into you. There is no time to think, only to act!", "8.png", 0, 0, 0, 0, 1);
    connect(8, "Fight Back", 801); 
    connect(8, "Escape", 802);     

    addNode(801, "You fought fiercely, teeth meeting fur and bone. The rogue falls. You claim the spoils of victory.", "19.png", -20, 0, -20, 0, 0, "None", "Meat");
    connect(801, "Continue", 9);

    addNode(802, "You scramble away, battered and bleeding. You escaped with your life, but nothing else.", "4(b).png", -10, 0, -30, 0, 0, "None", "None");
    connect(802, "Continue", 9);    

    addNode(9, "LEVEL 9: Bleeding Path\nBright red spots mark your trail. The pain is a dull throb. You must decide how to handle your injuries.", "9.png", 0, 0, 0, 0, 1, "None", "Meat");
    connect(9, "Heal Wounds", 10); 
    connect(9, "Push On", 10);     

    // --- LEVEL 10 BRANCH ---
    addNode(10, "LEVEL 10: The Frozen River\nA jagged scar of ice divides the land. The river groans. The bank is safer but choked with mud.", "10.png", 0, 0, 0, 0, 1);
    connect(10, "Cross Ice (Fast)", 110); 
    connect(10, "Follow Bank (Slow)", 111);

    // 110: Ice
    addNode(110, "LEVEL 11A: Thin Ice\nThe ice screams and gives way! You plunge into the freezing water, clutching the carcass you found.", "11 (a).png", -5, 0, 0, 0, 0, "None", "Meat");
    connect(110, "Scramble Up", 12); 
    connect(110, "Swim", 12);

    // 111: Bank
    addNode(111, "LEVEL 11B: Muddy Bank\nThe mud drags at your paws. A viper strikes from the reeds! You recoil, but the venom burns.", "11(b).png", -15, -10, 0, 10, 1, "None", "Meat");
    connect(111, "Trudge On", 12);

    // LEVEL 12: Lonely Stars
    addNode(12, "LEVEL 12: Lonely Stars\nYou reach the far bank. The sky is a canvas of cold diamonds. You feel small, but alive.", "12.png", 0, 0, 0, 0, 1);
    connect(12, "Rest", 1201); 

    addNode(1201, "The exhaustion finally takes you. You sleep fitfully under the stars, waking up energized.", "12.png", +15, +100, 0, 0, 0);
    connect(1201, "Wake Up", 13);

    // LEVEL 13: Strength in Silence
    addNode(13, "LEVEL 13: Strength in Silence\nIsolation is a harsh teacher. Your senses are sharper, your muscles harder. The wild is not conquering you.", "13.png", 0, 0, 0, 0, 1);
    connect(13, "Train", 1301);   
    connect(13, "Explore", 1302); 

    addNode(1301, "You push your muscles to failure and beyond. When you return, the pack will respect this power.", "13.png", 0, 0, -20, +15, 0, "None", "None");
    connect(1301, "Continue", 14);

    addNode(1302, "You scour the area. You chew on bitter medicinal roots and manage to bag some small game.", "13.png", +10, 0, -20, 0, 0, "None", "Meat");
    connect(1302, "Continue", 14);

    addNode(14, "LEVEL 14: Human Scent\nAcrid and chemical. The scent of smoke and tanned leather. Man. The most dangerous predator is near.", "14.png", 0, 0, 0, 0, 1);
    connect(14, "Hide", 15);
    connect(14, "Flee", 15);

    addNode(15, "LEVEL 15: Marking the Land\nThis ridge overlooks the valley. To mark it is to challenge the world. This land could be yours.", "15.png", 0, 0, 0, 10, 1);
    connect(15, "Mark Territory", 16); 
    connect(15, "Observe", 16);

    addNode(16, "LEVEL 16: Silverpaw Borders\nYou have crossed into claimed territory. Strange scent markers line the trees. Eyes are watching.", "16.png", 0, 0, 0, 0, 1);
    connect(16, "Respect Borders", 17);
    connect(16, "Trespass", 17);

    addNode(17, "LEVEL 17: Trust or Fear\nThree gaunt figures step from the treeline. Wanderers. They look for a leader. They look at you.", "17.png", 0, 0, 0, 0, 1, "None", "Meat");
    connect(17, "Form Pack", 18); 
    connect(17, "Stay Alone", 18);

    addNode(18, "LEVEL 18: Old Truths\nIn the dirt, a familiar scent. Your parents were here. The past rushes back, painful and sharp.", "18.png", 0, 0, 0, 0, 1);
    connect(18, "Vow Revenge", 19);
    connect(18, "Seek Peace", 19);

    addNode(19, "LEVEL 19: Preparing for War\nRetribution burns in your blood. Zolver knows you are coming. You must be ready to kill.", "19.png", +10, 0, 0, 0, 1);
    connect(19, "Sharpen Claws", 20); 
    connect(19, "Rest", 20);

    // LEVEL 20: Rival Alpha (The Encounter)
    addNode(20, "LEVEL 20: Rival Alpha\nA massive grey wolf blocks the path. 'This mountain belongs to the Nighttreaver,' he snarls.", "20 (a).png", 0, 0, 0, 0, 1);
    connect(20, "Duel for Dominance", 2001); 

    // Child Node: The Duel
    addNode(2001, "You lunged at the Alpha! The battle was fierce, but your strength prevailed.", "20 (a).png", -20, -15, +10, +30, 0, "None", "Meat");
    storyMap[2001]->slideshow.push_back("20 (b).png"); 
    storyMap[2001]->slideshow.push_back("20 (c).png"); 
    storyMap[2001]->slideshow.push_back("20 (d).png"); 

    connect(2001, "Claim Victory", 21);

    addNode(21, "LEVEL 21: Scars of Victory\nThe rival lies defeated in the snow. You are the Alpha now, but the victory has left deep wounds.", "21.png", +25, +100, 0, 0, 1);
    connect(21, "Heal Wounds", 22);

    addNode(22, "LEVEL 22: Call of the Pack\nHowls erupt around you. Not in challenge, but in greeting. The scattered wolves are gathering.", "22.png", 0, 0, 0, 0, 1);
    connect(22, "Recruit Them", 23); 
    connect(22, "Walk Alone", 23);

    addNode(23, "LEVEL 23: Leader's Trial\nYour new pack is restless. Chaos threatens your order. A leader must be firm, or the pack will devour itself.", "23.png", 0, 0, 0, 10, 1);
    connect(23, "Protect Pack", 24);
    connect(23, "Command Strictly", 24);

    addNode(24, "LEVEL 24: Territory Invasion\nZolver's scouts have sent his scouts. They tear at your borders. If you yield ground, you look weak.", "24.png", 0, 0, 0, 0, 1);
    connect(24, "Defend Territory", 25);
    connect(24, "Retreat", 25);

    addNode(25, "LEVEL 25: Strength of Bonds\nThe pack is solidifying. They move as one entity now. Unity is your greatest weapon against the coming storm.", "25.png", 0, 0, 0, 5, 1);
    connect(25, "Bond with Pack", 26);
    connect(25, "Scout Ahead", 26);

    addNode(26, "LEVEL 26: March Toward Fate\nThe peak of Black Mountain looms. Zolver awaits at the summit. You must prepare your body for the final ascent.", "26.png", 0, 0, 0, 0, 1);
    connect(26, "Train Hard", 2601); 
    connect(26, "Rest", 2602);       

    addNode(2601, "You push your muscles to absolute failure. Your body aches, but your spirit is iron. You are ready.", "26.png", 0, -10, 0, +15, 0);
    connect(2601, "Continue", 27);

    addNode(2602, "You sleep deeply, dreamlessly. You wake with your energy reserves overflowing. The mountain awaits.", "26(a).png", +10, +100, 0, 0, 0);
    connect(2602, "Continue", 27);

    addNode(27, "LEVEL 27: Old Wounds\nThe altitude bites. Every old scar throbs in the cold. The pain is a reminder of what you survived.", "27.png", 0, 0, 0, 0, 1);
    connect(27, "Heal", 28);
    connect(27, "Ignore Pain", 28);

    addNode(28, "LEVEL 28: Calm Before the Storm\nThe wind dies down. The silence is heavy, pressing against your ears. The final battle is near.", "28.png", 0, 0, 0, 0, 1);
    connect(28, "Reflect on Journey", 29);
    connect(28, "Stay Alert", 29);

    // --- LEVEL 29/30: FINAL BOSS SLIDESHOW ---
    addNode(29, "LEVEL 29: Clash of Clans\nA sea of glowing eyes in the dark. Zolver's army stands ready. The snow will turn red tonight.", "29.png", 0, 0, 0, 30, 1, "None", "Meat");
    connect(29, "Lead the Charge", 30); 
    connect(29, "Command from Rear", 30); 

    addNode(30, "LEVEL 30: Zolver Nighttreaver\nHe is massive, a shadow made flesh. He laughs—a dry, rasping sound. 'You are nothing,' he hisses.", "30 (a).png", 0, 0, 0, 0, 1);
    storyMap[30]->slideshow.push_back("30(b).png"); 
    storyMap[30]->slideshow.push_back("30 (c).png"); 
    storyMap[30]->slideshow.push_back("30 (d).png"); 
    connect(30, "Strike for the Throat", 999); 
    connect(30, "Counter-Attack", 999); 

    // ENDINGS
    addNode(999, "VICTORY: ALPHA LEGEND\nYou have torn the tyrant down. The territory is yours. Your legend begins now.", "victory.png", 0, 0, 0, 0, 0);
    addNode(997, "GAME OVER\nThe cold takes you. Your journey ends here, buried beneath the snow.", "defeat.png", 0, 0, 0, 0, 0);

    // --- EVENTS ---
    addNode(901, "EVENT: BLIZZARD\nThe sky turns white. The wind screams, erasing the world in a blinding vortex of ice.", "blizzard (a).png", 0, 0, 0, 0, 0);
    connect(901, "Dig In (-20 Energy)", 9011);       
    connect(901, "Push Through (-20 Health)", 9012); 

    addNode(9011, "You burrow into the snow. The wind howls over you, but you conserve heat.", "blizzard (c).png", 0, -20, 0, 0, 0); 
    connect(9011, "The storm passes...", -99); 

    addNode(9012, "You fight the wind. Ice cuts your face, but you cover ground.", "blizzard (b).png", -20, 0, 0, 0, 0); 
    connect(9012, "The storm passes...", -99);

    addNode(902, "EVENT: BEAR AMBUSH\nA mountain of fur and muscle crashes through the trees! A starving grizzly roars, shaking the ground.", "bear (a).png", 0, 0, 0, 0, 0);
    connect(902, "Fight (-35 HP)", 9021); 
    connect(902, "Run (-15 HP)", 9022);   

    addNode(9021, "CLASH OF FANGS\nYou lunge at the grizzly! The battle is brutal, a blur of claws and teeth. You drive it back, but not without a cost.", "bear (b).png", -35, -20, 0, 20, 0, "None", "Meat");
    storyMap[9021]->slideshow.push_back("bear (c).png"); 
    storyMap[9021]->slideshow.push_back("bear (d).png"); 
    connect(9021, "Lick your wounds...", -99); 

    addNode(9022, "THE ESCAPE\nYou scramble up a loose scree slope. The massive bear slides backward, roaring in frustration as you escape into the mist.", "4(b).png",-15, 0, 0, 0, 0);
    connect(9022, "Catch your breath...", -99);
    

    currentNode = storyMap[1];
}