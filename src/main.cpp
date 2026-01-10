#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem> 

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

// Helpers
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
    
    // Maintain Aspect Ratio
    float imgW = 1920.0f; 
    float imgH = 1080.0f;
    float screenAspect = (float)screenW / (float)screenH;
    float imgAspect = imgW / imgH;
    float drawW, drawH;

    if (screenAspect > imgAspect) {
        drawW = (float)screenW;
        drawH = drawW / imgAspect;
    } else {
        drawH = (float)screenH;
        drawW = drawH * imgAspect;
    }

    float x = (screenW - drawW) * 0.5f;
    float y = (screenH - drawH) * 0.5f;

    ImGui::GetBackgroundDrawList()->AddImage(my_tex_id, 
        ImVec2(x, y), 
        ImVec2(x + drawW, y + drawH), 
        ImVec2(0, 0), ImVec2(1, 1), 
        IM_COL32(255, 255, 255, 255));
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
    
    // FONT LOADING
    ImFont* titleFont = io.Fonts->AddFontFromFileTTF("Assets/Fonts/pixel_font.ttf", 32.0f);
    ImFont* bodyFont = io.Fonts->AddFontFromFileTTF("Assets/Fonts/pixel_font.ttf", 16.0f);
    if (!titleFont) titleFont = io.Fonts->AddFontDefault();
    if (!bodyFont) bodyFont = io.Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    setupImGuiStyle();

    // INITIALIZE GAME
    engine.initGame(); 
    // Start music immediately
    engine.updateMusicSystem(); 

    // LOAD TEXTURES
    unsigned int menuBg = engine.getGeneralTexture("start_screen.png"); 
    
    // UI Icons
    unsigned int iconMap = engine.getGeneralTexture("map_icon.png");       
    unsigned int overlayMap = engine.getGeneralTexture("map.png");   
    unsigned int iconInventory = engine.getGeneralTexture("inventory.png");
    unsigned int iconScavenge = engine.getGeneralTexture("scavenge.png");
    unsigned int iconRest = engine.getGeneralTexture("rest.png");
    unsigned int iconUndo = engine.getGeneralTexture("undo.png");
    unsigned int iconSave = engine.getGeneralTexture("save.png");
    unsigned int iconMute = engine.getGeneralTexture("mute.png"); // NEW

    // Stat Icons
    unsigned int iconHealth = engine.getGeneralTexture("health.png");
    unsigned int iconEnergy = engine.getGeneralTexture("energy.png");
    unsigned int iconHunger = engine.getGeneralTexture("hunger.png");
    unsigned int iconRep = engine.getGeneralTexture("reputation.png");

    // Background vars
    std::string cachedImageName = "";
    unsigned int cachedTextureID = 0;
    int slideIndex = 0;
    float slideTimer = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Update Global Typewriter Logic
        engine.updateTypewriter(io.DeltaTime);

        // Status Message Timer
        if (!statusMessage.empty()) {
            statusTimer += io.DeltaTime;
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
                    
                    // Setup Intro Text
                    engine.introLineIndex = 0;
                    if (!engine.introLines.empty()) {
                        engine.targetText = engine.introLines[0];
                        engine.textCharIndex = 0;
                        engine.currentDisplayedText = "";
                        engine.textFinished = false;
                    }
                    engine.updateMusicSystem();
                }
                ImGui::Spacing();
                
                if (ImGui::Button("LOAD GAME", ImVec2(280, 50))) {
                    showLoadPopup = true; 
                }
                ImGui::Spacing();

                if (ImGui::Button("QUIT", ImVec2(280, 50))) {
                    glfwSetWindowShouldClose(window, true);
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // ==========================================
        // 2. INTRO STORY (With Typewriter)
        // ==========================================
        else if (engine.currentState == STATE_INTRO) {
            DrawBackgroundCover(menuBg, display_w, display_h);
            
            ImGui::SetNextWindowPos(ImVec2(50, display_h - 250));
            ImGui::SetNextWindowSize(ImVec2(display_w - 100, 200));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.9f));
            
            if (ImGui::Begin("IntroBox", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                
                ImGui::PushFont(bodyFont);
                ImGui::TextWrapped("%s", engine.currentDisplayedText.c_str());
                ImGui::PopFont();

                ImGui::SetCursorPosY(150);
                
                // Next Button
                if (ImGui::Button("NEXT >>", ImVec2(100, 30))) {
                    if (!engine.textFinished) {
                        engine.skipTypewriter(); 
                    } else {
                        engine.introLineIndex++;
                        if (engine.introLineIndex >= engine.introLines.size()) {
                            engine.currentState = STATE_GAMEPLAY;
                            engine.updateMusicSystem();
                            
                            if(engine.currentNode) {
                                engine.targetText = engine.currentNode->text;
                                engine.textCharIndex = 0;
                                engine.currentDisplayedText = "";
                                engine.textFinished = false;
                            }
                        } else {
                            engine.targetText = engine.introLines[engine.introLineIndex];
                            engine.textCharIndex = 0;
                            engine.currentDisplayedText = "";
                            engine.textFinished = false;
                        }
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
            
        // --- BACKGROUND LOGIC ---
            if (engine.currentNode) {
                // 1. Reset logic when moving to a new node
                static int lastNodeID = -1;
                if (engine.currentNode->id != lastNodeID) {
                    slideIndex = 0;      // Reset index
                    slideTimer = 0.0f;   // Reset timer
                    lastNodeID = engine.currentNode->id;
                    cachedImageName = engine.currentNode->mainImage; // Default to main image
                }

                // 2. Safety Check: Does this node actually have a slideshow?
                bool hasSlides = !engine.currentNode->slideshow.empty();

                if (hasSlides) {
                    // Update Timer
                    slideTimer += io.DeltaTime;
                    if (slideTimer > 1.5f) { 
                        slideTimer = 0.0f;
                        // Safe Modulo Arithmetic
                        size_t sz = engine.currentNode->slideshow.size();
                        if (sz > 0) {
                             slideIndex = (slideIndex + 1) % sz;
                        }
                    }
                    
                    // Safe Access
                    if (slideIndex < engine.currentNode->slideshow.size()) {
                        cachedImageName = engine.currentNode->slideshow[slideIndex];
                    } else {
                        // Fallback if index somehow went out of bounds
                        slideIndex = 0;
                        if (!engine.currentNode->slideshow.empty())
                            cachedImageName = engine.currentNode->slideshow[0];
                    }
                } 
                else {
                    // 3. Fallback: No slideshow, ensure we show the main image
                    if (cachedImageName != engine.currentNode->mainImage) {
                        cachedImageName = engine.currentNode->mainImage;
                    }
                }
                
                // 4. Load and Draw
                cachedTextureID = engine.getNodeTexture(cachedImageName);
                DrawBackgroundCover(cachedTextureID, display_w, display_h);
            }

            // --- HUD (STATS) ---
            ImGui::SetNextWindowPos(ImVec2(20, 20));
            ImGui::SetNextWindowSize(ImVec2(350, 220));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
            if (ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                ImGui::PushFont(bodyFont);
                
                ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Day: %d", engine.currentStats.dayCount);
                
                if(!statusMessage.empty()) 
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), ">> %s", statusMessage.c_str());
                else 
                    ImGui::Spacing();

                ImGui::Separator(); 

                // HEALTH
                if(iconHealth) ImGui::Image((ImTextureID)(intptr_t)iconHealth, ImVec2(24,24)); 
                else ImGui::Text("HP ");
                ImGui::SameLine(); 
                ImGui::ProgressBar(engine.currentStats.health / 100.0f, ImVec2(200, 24), std::to_string(engine.currentStats.health).c_str());

                // ENERGY
                if(iconEnergy) ImGui::Image((ImTextureID)(intptr_t)iconEnergy, ImVec2(24,24)); 
                else ImGui::Text("EN ");
                ImGui::SameLine(); 
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.9f, 1.0f)); 
                ImGui::ProgressBar(engine.currentStats.energy / 100.0f, ImVec2(200, 24), std::to_string(engine.currentStats.energy).c_str());
                ImGui::PopStyleColor();

                // HUNGER
                if(iconHunger) ImGui::Image((ImTextureID)(intptr_t)iconHunger, ImVec2(24,24)); 
                else ImGui::Text("FD ");
                ImGui::SameLine(); 
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.4f, 0.1f, 1.0f)); 
                ImGui::ProgressBar(engine.currentStats.hunger / 100.0f, ImVec2(200, 24), std::to_string(engine.currentStats.hunger).c_str());
                ImGui::PopStyleColor();

                ImGui::Spacing();
                
                // REPUTATION
                if(iconRep) ImGui::Image((ImTextureID)(intptr_t)iconRep, ImVec2(24,24)); 
                else ImGui::Text("REP");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.6f, 0.2f, 0.8f, 1.0f)); 
                char repBuf[16]; sprintf(repBuf, "%d", engine.currentStats.reputation);
                ImGui::ProgressBar(engine.currentStats.reputation / 100.0f, ImVec2(200, 24), repBuf);
                ImGui::PopStyleColor();
                
                ImGui::SetCursorPosX(55);
                ImGui::Text("Rank: %s", engine.getFinalTitle().c_str());

                ImGui::PopFont();
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // --- ACTION BAR ---
            ImGui::SetNextWindowPos(ImVec2(display_w - 480, 20)); // WIDENED
            ImGui::SetNextWindowSize(ImVec2(460, 80));           // WIDENED
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
            if (ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                float iconSize = 48.0f;
                ImVec2 btnSize = ImVec2(50, 40);

                // Inventory
                if (iconInventory && ImGui::ImageButton("inv_btn", (ImTextureID)(intptr_t)iconInventory, ImVec2(iconSize, iconSize))) showInventory = !showInventory;
                else if (!iconInventory && ImGui::Button("INV", btnSize)) showInventory = !showInventory;
                ImGui::SameLine();

                // Map
                if (iconMap && ImGui::ImageButton("map_btn", (ImTextureID)(intptr_t)iconMap, ImVec2(iconSize, iconSize))) showMap = !showMap;
                else if (!iconMap && ImGui::Button("MAP", btnSize)) showMap = !showMap;
                ImGui::SameLine();

                // Scavenge
                if (iconScavenge && ImGui::ImageButton("scav_btn", (ImTextureID)(intptr_t)iconScavenge, ImVec2(iconSize, iconSize))) engine.performGlobalScavenge();
                else if (!iconScavenge && ImGui::Button("HUNT", btnSize)) engine.performGlobalScavenge();
                ImGui::SameLine();

                // Rest
                if (iconRest && ImGui::ImageButton("rest_btn", (ImTextureID)(intptr_t)iconRest, ImVec2(iconSize, iconSize))) engine.performGlobalRest();
                else if (!iconRest && ImGui::Button("REST", btnSize)) engine.performGlobalRest();
                ImGui::SameLine();

                // Undo
                if (iconUndo && ImGui::ImageButton("undo_btn", (ImTextureID)(intptr_t)iconUndo, ImVec2(iconSize, iconSize))) { 
                    engine.undoLastAction(); 
                    statusMessage = "Undo Performed"; 
                } 
                else if (!iconUndo && ImGui::Button("UNDO", btnSize)) { 
                    engine.undoLastAction(); 
                    statusMessage = "Undo Performed"; 
                }
                ImGui::SameLine();

                // Save
                if (iconSave && ImGui::ImageButton("save_btn", (ImTextureID)(intptr_t)iconSave, ImVec2(iconSize, iconSize))) showSavePopup = true;
                else if (!iconSave && ImGui::Button("SAVE", btnSize)) showSavePopup = true;

                // --- MUTE BUTTON ---
                ImGui::SameLine();
                // Check if we have the icon
                unsigned int iconMute = engine.getGeneralTexture("mute.png");
                if (iconMute) {
                    // Tint red if muted
                    ImVec4 tint = engine.isMuted ? ImVec4(1, 0.5f, 0.5f, 1) : ImVec4(1, 1, 1, 1);
                    if (ImGui::ImageButton("mute_btn", (ImTextureID)(intptr_t)iconMute, ImVec2(iconSize, iconSize), ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), tint)) {
                        engine.toggleMute();
                    }
                } else {
                    // Fallback Text Button
                    std::string muteLabel = engine.isMuted ? "UNMUTE" : "MUTE";
                    if (ImGui::Button(muteLabel.c_str(), ImVec2(60, 40))) engine.toggleMute();
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // --- STORY BOX (TYPEWRITER) ---
            ImGui::SetNextWindowPos(ImVec2(50, display_h - 250));
            ImGui::SetNextWindowSize(ImVec2(display_w - 100, 200));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.85f)); 
            if (ImGui::Begin("StoryBox", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
                
                ImGui::PushFont(titleFont);
                ImGui::TextWrapped("%s", engine.currentDisplayedText.c_str());
                ImGui::PopFont();
                
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                // Choices
                if (engine.currentNode) {
                    ImGui::PushFont(bodyFont);
                    for (int i = 0; i < engine.currentNode->children.size(); i++) {
                        ImGui::PushID(i); 
                        if (ImGui::Button(engine.currentNode->children[i].first.c_str(), ImVec2(0, 40))) {
                            engine.makeChoice(i);
                        }
                        ImGui::PopID();
                        if (i < engine.currentNode->children.size() - 1) ImGui::SameLine();
                    }
                    ImGui::PopFont();
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // --- INVENTORY OVERLAY ---
            if (showInventory) {
                ImGui::SetNextWindowPos(ImVec2(display_w/2 - 200, display_h/2 - 200));
                ImGui::SetNextWindowSize(ImVec2(400, 400));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.98f));
                
                if (ImGui::Begin("Backpack", &showInventory, ImGuiWindowFlags_NoCollapse)) {
                    ImGui::PushFont(titleFont); 
                    ImGui::Text("Pack Contents"); 
                    ImGui::PopFont(); 
                    ImGui::Separator();
                    
                    ImGui::PushFont(bodyFont);
                    std::vector<Item> items = engine.inventory.toVector();
                    if (items.empty()) {
                        ImGui::TextDisabled("Empty.");
                    } else {
                        for (int i = 0; i < items.size(); i++) {
                            ImGui::PushID(i);
                            ImGui::Text("%s x%d", items[i].name.c_str(), items[i].quantity); 
                            ImGui::SameLine(300);
                            
                            // HERB Logic Added Here
                            if (items[i].type == FOOD || items[i].type == HERB) { 
                                if (ImGui::Button("USE")) engine.useItem(items[i].name); 
                            } else {
                                ImGui::TextDisabled("[TOOL]");
                            }
                            ImGui::PopID();
                        }
                    }
                    ImGui::PopFont();
                }
                ImGui::End();
                ImGui::PopStyleColor();
            }

            // --- MAP OVERLAY ---
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
        // 4. REST & SCAVENGE SCREENS
        // ==========================================
        else if (engine.currentState == STATE_REST || engine.currentState == STATE_SCAVENGE) {
            
            unsigned int bgTex = 0;
            if (engine.currentState == STATE_REST) bgTex = engine.getGeneralTexture("rest_screen.png");
            else bgTex = engine.getGeneralTexture("scavenge_screen.png");

            if (bgTex == 0) bgTex = menuBg; 
            DrawBackgroundCover(bgTex, display_w, display_h);

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
        // 5. SAVE POPUP
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
        // 6. LOAD POPUP
        // ==========================================
        if (showLoadPopup) ImGui::OpenPopup("Load Game");
        if (ImGui::BeginPopupModal("Load Game", &showLoadPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select a file to load:");
            ImGui::Separator();
            ImGui::BeginChild("FileList", ImVec2(300, 200), true);
            
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