#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem> // Required for scanning save files

#include "GameEngine.h"

namespace fs = std::filesystem;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

GameEngine engine;
bool showInventory = false;
bool showMap = false;

// Popup States
bool showSavePopup = false;
bool showLoadPopup = false;
char saveFileNameBuffer[128] = "savegame"; 
std::string statusMessage = "";
float statusTimer = 0.0f;

// Typewriter variables
std::string displayedIntro = "";
float typeTimer = 0.0f;
int typeCharIndex = 0;

void setupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.15f, 0.95f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.3f, 0.4f, 1.0f);
    style.FrameRounding = 5.0f;
}

void DrawBackgroundCover(unsigned int texID, int screenW, int screenH) {
    if (texID == 0) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0,0), ImVec2((float)screenW, (float)screenH), IM_COL32(20,20,20,255));
        return;
    }
    ImTextureID my_tex_id = (ImTextureID)(intptr_t)texID;
    float imgW = 1920.0f; float imgH = 1080.0f;
    float screenAspect = (float)screenW / (float)screenH;
    float imgAspect = imgW / imgH;
    float drawW, drawH;

    if (screenAspect > imgAspect) { drawW = (float)screenW; drawH = drawW / imgAspect; } 
    else { drawH = (float)screenH; drawW = drawH * imgAspect; }

    float x = (screenW - drawW) * 0.5f;
    float y = (screenH - drawH) * 0.5f;

    ImGui::GetBackgroundDrawList()->AddImage(my_tex_id, ImVec2(x, y), ImVec2(x + drawW, y + drawH), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Alex The Wolf", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImFont* titleFont = io.Fonts->AddFontFromFileTTF("Assets/Fonts/pixel_font.ttf", 32.0f);
    ImFont* bodyFont = io.Fonts->AddFontFromFileTTF("Assets/Fonts/pixel_font.ttf", 16.0f);
    if (!titleFont) titleFont = io.Fonts->AddFontDefault();
    if (!bodyFont) bodyFont = io.Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    setupImGuiStyle();

    engine.initGame(); 
    engine.updateMusicSystem(); // Start playing menu music
    unsigned int menuBg = engine.getGeneralTexture("start_screen.png"); 
    
    // UI ICONS (Explicit Paths to avoid mixups)
    unsigned int iconMap = engine.getGeneralTexture("map_icon.png");       
    unsigned int overlayMap = engine.getGeneralTexture("map.png");   

    unsigned int iconInventory = engine.getGeneralTexture("inventory.png");
    unsigned int iconScavenge = engine.getGeneralTexture("scavenge.png");
    unsigned int iconRest = engine.getGeneralTexture("rest.png");
    unsigned int iconUndo = engine.getGeneralTexture("undo.png");
    unsigned int iconSave = engine.getGeneralTexture("save.png");

    // STAT ICONS
    unsigned int iconHealth = engine.getGeneralTexture("health.png");
    unsigned int iconEnergy = engine.getGeneralTexture("energy.png");
    unsigned int iconHunger = engine.getGeneralTexture("hunger.png");
    unsigned int iconRep = engine.getGeneralTexture("reputation.png");

    std::string cachedImageName = "";
    unsigned int cachedTextureID = 0;
    int slideIndex = 0; float slideTimer = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Status Message Timer
        if (!statusMessage.empty()) {
            statusTimer += ImGui::GetIO().DeltaTime;
            if (statusTimer > 3.0f) { statusMessage = ""; statusTimer = 0; }
        }

        // ==========================================
        // 1. MENU STATE
        // ==========================================
        if (engine.currentState == STATE_MENU) {
            DrawBackgroundCover(menuBg, display_w, display_h);
            ImGui::SetNextWindowPos(ImVec2(display_w/2 - 150, display_h/2 - 100));
            ImGui::SetNextWindowSize(ImVec2(300, 250));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.8f));
            if (ImGui::Begin("MainMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                ImGui::PushFont(titleFont);
                float textW = ImGui::CalcTextSize("ALEX THE WOLF").x;
                ImGui::SetCursorPosX((300 - textW) * 0.5f);
                ImGui::Text("ALEX THE WOLF");
                ImGui::PopFont();
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                if (ImGui::Button("NEW GAME", ImVec2(280, 50))) {
                    engine.initGame(); 
                    engine.currentState = STATE_INTRO;
                    displayedIntro = ""; typeCharIndex = 0;
                }
                ImGui::Spacing();
                if (ImGui::Button("LOAD GAME", ImVec2(280, 50))) {
                    showLoadPopup = true; // Open Load Modal
                }
                ImGui::Spacing();
                if (ImGui::Button("QUIT", ImVec2(280, 50))) glfwSetWindowShouldClose(window, true);
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // ==========================================
        // 2. INTRO STORY
        // ==========================================
        else if (engine.currentState == STATE_INTRO) {
            DrawBackgroundCover(menuBg, display_w, display_h);
            ImGui::SetNextWindowPos(ImVec2(50, display_h - 250));
            ImGui::SetNextWindowSize(ImVec2(display_w - 100, 200));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.9f));
            if (ImGui::Begin("IntroBox", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                
                std::string targetText = engine.introLines[engine.introLineIndex];
                if (typeCharIndex < targetText.length()) {
                    typeTimer += ImGui::GetIO().DeltaTime;
                    if (typeTimer > 0.03f) { typeTimer = 0; typeCharIndex++; displayedIntro = targetText.substr(0, typeCharIndex); }
                }

                ImGui::PushFont(bodyFont);
                ImGui::TextWrapped("%s", displayedIntro.c_str());
                ImGui::PopFont();

                ImGui::SetCursorPosY(150);
                if (ImGui::Button("NEXT >>", ImVec2(100, 30))) {
                    if (typeCharIndex < targetText.length()) {
                        typeCharIndex = targetText.length(); displayedIntro = targetText;
                    } else {
                        engine.introLineIndex++;
                        if (engine.introLineIndex >= engine.introLines.size()) engine.currentState = STATE_GAMEPLAY;
                        else { typeCharIndex = 0; displayedIntro = ""; }
                    }
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // ==========================================
        // 3. GAMEPLAY
        // ==========================================
        else if (engine.currentState == STATE_GAMEPLAY || engine.currentState == STATE_OUTRO) {
            
            if (engine.currentNode) {
                if (!engine.currentNode->slideshow.empty()) {
                    slideTimer += ImGui::GetIO().DeltaTime;
                    if (slideTimer > 1.5f) { slideTimer = 0.0f; slideIndex = (slideIndex + 1) % engine.currentNode->slideshow.size(); }
                    if (slideIndex < engine.currentNode->slideshow.size()) cachedImageName = engine.currentNode->slideshow[slideIndex];
                } else {
                    if (cachedImageName != engine.currentNode->mainImage) { cachedImageName = engine.currentNode->mainImage; slideIndex = 0; }
                }
                cachedTextureID = engine.getNodeTexture(cachedImageName);
                DrawBackgroundCover(cachedTextureID, display_w, display_h);
            }

            // HUD
            ImGui::SetNextWindowPos(ImVec2(20, 20));
            ImGui::SetNextWindowSize(ImVec2(350, 220));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
            if (ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                ImGui::PushFont(bodyFont);
                
                ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Day: %d", engine.currentStats.dayCount);
                if(!statusMessage.empty()) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), ">> %s", statusMessage.c_str());
                else ImGui::Spacing();

                ImGui::Separator(); 

                // HP
                if(iconHealth) ImGui::Image((ImTextureID)(intptr_t)iconHealth, ImVec2(24,24)); else ImGui::Text("HP ");
                ImGui::SameLine(); ImGui::ProgressBar(engine.currentStats.health / 100.0f, ImVec2(200, 24), std::to_string(engine.currentStats.health).c_str());

                // EN
                if(iconEnergy) ImGui::Image((ImTextureID)(intptr_t)iconEnergy, ImVec2(24,24)); else ImGui::Text("EN ");
                ImGui::SameLine(); 
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.9f, 1.0f)); 
                ImGui::ProgressBar(engine.currentStats.energy / 100.0f, ImVec2(200, 24), std::to_string(engine.currentStats.energy).c_str());
                ImGui::PopStyleColor();

                // HUNGER
                if(iconHunger) ImGui::Image((ImTextureID)(intptr_t)iconHunger, ImVec2(24,24)); else ImGui::Text("FD ");
                ImGui::SameLine(); 
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.4f, 0.1f, 1.0f)); 
                ImGui::ProgressBar(engine.currentStats.hunger / 100.0f, ImVec2(200, 24), std::to_string(engine.currentStats.hunger).c_str());
                ImGui::PopStyleColor();

                ImGui::Spacing();
                
                // REP (Number only, no /100)
                if(iconRep) ImGui::Image((ImTextureID)(intptr_t)iconRep, ImVec2(24,24)); else ImGui::Text("REP");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.6f, 0.2f, 0.8f, 1.0f)); 
                char repBuf[16]; sprintf(repBuf, "%d", engine.currentStats.reputation); // FIX: Just number
                ImGui::ProgressBar(engine.currentStats.reputation / 100.0f, ImVec2(200, 24), repBuf);
                ImGui::PopStyleColor();
                
                ImGui::SetCursorPosX(55);
                ImGui::Text("Rank: %s", engine.getFinalTitle().c_str());

                ImGui::PopFont();
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // ACTIONS
            ImGui::SetNextWindowPos(ImVec2(display_w - 420, 20));
            ImGui::SetNextWindowSize(ImVec2(400, 80));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
            if (ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                float iconSize = 48.0f;
                ImVec2 btnSize = ImVec2(50, 40);

                if (iconInventory && ImGui::ImageButton("inv_btn", (ImTextureID)(intptr_t)iconInventory, ImVec2(iconSize, iconSize))) showInventory = !showInventory;
                else if (!iconInventory && ImGui::Button("INV", btnSize)) showInventory = !showInventory;
                ImGui::SameLine();

                if (iconMap && ImGui::ImageButton("map_btn", (ImTextureID)(intptr_t)iconMap, ImVec2(iconSize, iconSize))) showMap = !showMap;
                else if (!iconMap && ImGui::Button("MAP", btnSize)) showMap = !showMap;
                ImGui::SameLine();

                if (iconScavenge && ImGui::ImageButton("scav_btn", (ImTextureID)(intptr_t)iconScavenge, ImVec2(iconSize, iconSize))) engine.performGlobalScavenge();
                else if (!iconScavenge && ImGui::Button("HUNT", btnSize)) engine.performGlobalScavenge();
                ImGui::SameLine();

                if (iconRest && ImGui::ImageButton("rest_btn", (ImTextureID)(intptr_t)iconRest, ImVec2(iconSize, iconSize))) engine.performGlobalRest();
                else if (!iconRest && ImGui::Button("REST", btnSize)) engine.performGlobalRest();
                ImGui::SameLine();

                if (iconUndo && ImGui::ImageButton("undo_btn", (ImTextureID)(intptr_t)iconUndo, ImVec2(iconSize, iconSize))) { engine.undoLastAction(); statusMessage = "Undo Performed"; }
                else if (!iconUndo && ImGui::Button("UNDO", btnSize)) { engine.undoLastAction(); statusMessage = "Undo Performed"; }
                ImGui::SameLine();

                if (iconSave && ImGui::ImageButton("save_btn", (ImTextureID)(intptr_t)iconSave, ImVec2(iconSize, iconSize))) showSavePopup = true;
                else if (!iconSave && ImGui::Button("SAVE", btnSize)) showSavePopup = true;
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // STORY
            ImGui::SetNextWindowPos(ImVec2(50, display_h - 250));
            ImGui::SetNextWindowSize(ImVec2(display_w - 100, 200));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.85f)); 
            if (ImGui::Begin("StoryBox", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                ImGui::PushFont(titleFont);
                ImGui::TextWrapped("%s", engine.currentNode ? engine.currentNode->text.c_str() : "Error");
                ImGui::PopFont();
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                if (engine.currentNode) {
                    ImGui::PushFont(bodyFont);
                    for (int i = 0; i < engine.currentNode->children.size(); i++) {
                        ImGui::PushID(i); 
                        if (ImGui::Button(engine.currentNode->children[i].first.c_str(), ImVec2(0, 40))) engine.makeChoice(i);
                        ImGui::PopID();
                        if (i < engine.currentNode->children.size() - 1) ImGui::SameLine();
                    }
                    ImGui::PopFont();
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // INVENTORY
            if (showInventory) {
                ImGui::SetNextWindowPos(ImVec2(display_w/2 - 200, display_h/2 - 200));
                ImGui::SetNextWindowSize(ImVec2(400, 400));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.98f));
                if (ImGui::Begin("Backpack", &showInventory, ImGuiWindowFlags_NoCollapse)) {
                    ImGui::PushFont(titleFont); ImGui::Text("Pack Contents"); ImGui::PopFont(); ImGui::Separator();
                    ImGui::PushFont(bodyFont);
                    std::vector<Item> items = engine.inventory.toVector();
                    if (items.empty()) ImGui::TextDisabled("Empty.");
                    else {
                        for (int i = 0; i < items.size(); i++) {
                            ImGui::PushID(i);
                            ImGui::Text("%s x%d", items[i].name.c_str(), items[i].quantity); ImGui::SameLine(300);
                            if (items[i].type == FOOD || items[i].type == HERB) { if (ImGui::Button("USE")) engine.useItem(items[i].name); }
                            else ImGui::TextDisabled("[TOOL]");
                            ImGui::PopID();
                        }
                    }
                    ImGui::PopFont();
                }
                ImGui::End();
                ImGui::PopStyleColor();
            }

            // MAP OVERLAY
            if (showMap) {
                ImGui::SetNextWindowPos(ImVec2(50, 50));
                ImGui::SetNextWindowSize(ImVec2(display_w - 100, display_h - 100));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.95f));
                if (ImGui::Begin("MapOverlay", &showMap, ImGuiWindowFlags_NoDecoration)) {
                    ImGui::PushFont(titleFont);
                    ImGui::Text("World Map");
                    ImGui::SameLine(display_w - 400);
                    ImGui::Text("Current Location: %s", engine.currentNode ? engine.currentNode->text.substr(0, 15).c_str() : "Unknown");
                    ImGui::PopFont();
                    ImGui::Separator();

                    if (overlayMap) {
                        float mapH = display_h - 200; 
                        float mapW = mapH * (1920.0f/1080.0f); 
                        ImGui::SetCursorPosX((display_w - 100 - mapW) * 0.5f);
                        ImGui::Image((ImTextureID)(intptr_t)overlayMap, ImVec2(mapW, mapH));
                    } else {
                        ImGui::SetCursorPos(ImVec2(display_w/2 - 100, display_h/2));
                        ImGui::Text("Map Image Not Found.");
                    }

                    ImGui::SetCursorPosY(display_h - 160);
                    if (ImGui::Button("CLOSE MAP", ImVec2(display_w - 140, 40))) showMap = false;
                }
                ImGui::End();
                ImGui::PopStyleColor();
            }
        }
        // ==========================================
        // 4. REST & SCAVENGE POPUPS
        // ==========================================
        else if (engine.currentState == STATE_REST || engine.currentState == STATE_SCAVENGE) {
            
            // LOAD SPECIFIC BACKGROUND SCREEN
            // If you have "rest_screen.png" in Images/ or Icons/, the engine will find it.
            // If not found, it falls back to the menu background.
            unsigned int bgTex = 0;
            if (engine.currentState == STATE_REST) bgTex = engine.getGeneralTexture("rest_screen.png");
            else bgTex = engine.getGeneralTexture("scavenge_screen.png");

            if (bgTex == 0) bgTex = menuBg; // Fallback to start screen if specific image not found
            
            DrawBackgroundCover(bgTex, display_w, display_h);

            // Darken slightly to make text pop
            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0,0), ImVec2((float)display_w, (float)display_h), IM_COL32(0,0,0,100));

            ImGui::SetNextWindowPos(ImVec2(display_w/2 - 200, display_h/2 - 100));
            ImGui::SetNextWindowSize(ImVec2(400, 200));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
            
            if (ImGui::Begin("Popup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                ImGui::PushFont(titleFont);
                ImGui::Text(engine.currentState == STATE_REST ? "RESTING..." : "SCAVENGING...");
                ImGui::PopFont(); 
                ImGui::Separator();
                
                ImGui::PushFont(bodyFont);
                if (!engine.gameLog.empty()) ImGui::TextWrapped("%s", engine.gameLog.back().c_str());
                ImGui::PopFont();
                
                ImGui::SetCursorPosY(150);
                if (ImGui::Button("CONTINUE", ImVec2(380, 40))) engine.currentState = STATE_GAMEPLAY;
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // ==========================================
        // 4. SAVE POPUP (MODAL)
        // ==========================================
        if (showSavePopup) ImGui::OpenPopup("Save Game");
        if (ImGui::BeginPopupModal("Save Game", &showSavePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter name for your save file:");
            ImGui::InputText("##savename", saveFileNameBuffer, 128);
            ImGui::Separator();
            if (ImGui::Button("SAVE", ImVec2(120, 0))) { 
                engine.saveGameToFile(saveFileNameBuffer); 
                statusMessage = "Game Saved!";
                showSavePopup = false; 
                ImGui::CloseCurrentPopup(); 
            }
            ImGui::SameLine();
            if (ImGui::Button("CANCEL", ImVec2(120, 0))) { showSavePopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        // ==========================================
        // 5. LOAD POPUP (MODAL - LIST FILES)
        // ==========================================
        if (showLoadPopup) ImGui::OpenPopup("Load Game");
        if (ImGui::BeginPopupModal("Load Game", &showLoadPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select a file to load:");
            ImGui::Separator();
            ImGui::BeginChild("FileList", ImVec2(300, 200), true);
            
            // Scan Directory for .txt files
            try {
                for (const auto& entry : fs::directory_iterator(".")) {
                    if (entry.path().extension() == ".txt" && entry.path().string().find("save") != std::string::npos) {
                        if (ImGui::Button(entry.path().filename().string().c_str(), ImVec2(280, 0))) {
                            engine.loadGameFromFile(entry.path().string());
                            if (engine.currentState == STATE_MENU) engine.currentState = STATE_GAMEPLAY;
                            statusMessage = "Game Loaded!";
                            showLoadPopup = false;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            } catch (...) {
                ImGui::TextColored(ImVec4(1,0,0,1), "Error scanning files.");
            }
            ImGui::EndChild();
            ImGui::Separator();
            if (ImGui::Button("CANCEL", ImVec2(300, 0))) { showLoadPopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}