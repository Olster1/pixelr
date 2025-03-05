ImGuiIO& initMyImGui(SDL_GLContext gl_context, SDL_Window* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    assert(io.BackendPlatformUserData == nullptr && "ImGui SDL2 backend already initialized!");
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    io.Fonts->AddFontDefault(); // Regular font
    static const ImWchar icon_ranges[] = { 0x0020, 0xFFFF, 0 };
    ImFontConfig config;
    config.MergeMode = true;
    ImFont* iconFont = io.Fonts->AddFontFromFileTTF("./fonts/fa1.otf", 16.0f, &config, icon_ranges);
    if (!iconFont) {
        assert(false);
    }

  io.Fonts->Build();


    return io;

}

#include "imgui.h"

void drawTabs(GameState *state) {
      // Push a style to keep the tab bar at the top of the screen
      ImGui::SetNextWindowPos(ImVec2(0, 15));
      ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 50));
  
      // Begin a window without decorations, keeping it at the top
      if (ImGui::Begin("Top Tab Bar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground))
      {
          if (ImGui::BeginTabBar("TopTabs"))
          {

            for(int i = 0; i < getArrayLength(state->canvasTabs); ++i) {
              CanvasTab *t = state->canvasTabs + i;

              char *id = easy_createString_printf(&globalPerFrameArena, "%s%d", (char *)t->fileName, i);
              ImGui::PushID(id);
            bool open = true;
              if (ImGui::BeginTabItem(t->fileName, &open))
              {
                state->activeCanvasTab = i;
              
                  ImGui::EndTabItem();
              }

              ImGui::PopID();
            } 

           
  
              ImGui::EndTabBar();

               // Align button next to the tab bar
                ImGui::SameLine();
                
                if (ImGui::Button("+"))
                {
                    CanvasTab tab = CanvasTab(16, 16, easyString_copyToHeap("Untitled"));
                    pushArrayItem(&state->canvasTabs, tab, CanvasTab);
                    state->activeCanvasTab = getArrayLength(state->canvasTabs) - 1;
                }
          }
      }
      ImGui::End(); // End the top bar window
  
}

void drawAnimationTimeline(GameState *state, float deltaTime) {

    PlayBackAnimation *anim = &state->playBackAnimation;

    ImGui::Begin("Animation Timeline");

    // Play/Pause/Reset buttons
    if (ImGui::Button(anim->playing ? "\uf04c Pause" : "\uf04b Play")) {
        anim->playing = !anim->playing;
    }
    ImGui::SameLine();
    if (ImGui::Button("\uf2f9 Reset")) {
        anim->currentFrame = 0;
        anim->elapsedTime = 0.0f;
        anim->playing = false;
    }

    ImGui::Separator();

    // Display all frames in a row
    if (getArrayLength(anim->frameTextures) > 0) {
      ImGui::Text("Frames:");
  
      for (int i = 0; i < getArrayLength(anim->frameTextures); i++) {
          if (i > 0) ImGui::SameLine(); // Keep frames on the same line
  
          ImVec2 imageSize(64, 64); // Adjust as needed
          ImVec2 cursorPos = ImGui::GetCursorScreenPos();
          ImVec4 borderColor = (i == anim->currentFrame) ? ImVec4(1, 1, 0, 1) : ImVec4(0, 0, 0, 0); // Yellow for current frame
  
          // Make a button that overlays the image so we can detect clicks
          ImGui::PushID(i); // Unique ID for each frame
          if (ImGui::InvisibleButton("##frame", imageSize)) {
              anim->currentFrame = i; // Change current frame
              anim->playing = false; // Stop animation
          }
          ImGui::PopID();
  
          // Draw frame image over the invisible button
          ImGui::SetCursorScreenPos(cursorPos); // Reset position to draw the image
          ImGui::Image((ImTextureID)(intptr_t)anim->frameTextures[i], imageSize);
  
          // Draw border if this is the current frame
          if (i == anim->currentFrame) {
              ImGui::GetWindowDrawList()->AddRect(
                  cursorPos, ImVec2(cursorPos.x + imageSize.x, cursorPos.y + imageSize.y),
                  IM_COL32(255, 255, 0, 255), // Yellow border
                  0.0f, 0, 2.0f // Thickness
              );
          }
      }
  }

    ImGui::Separator();

    ImGui::SliderFloat("Time per Frame", &anim->frameTime, 0.01f, 2.0f);

   
    if (ImGui::Button("Add New Frame +")) {
        // Example: Add a placeholder OpenGL texture ID (should load real textures)
        pushArrayItem(&state->playBackAnimation.frameTextures, state->grassTexture.handle, u32);
    }

    // Animation playback logic
    if (anim->playing && getArrayLength(anim->frameTextures) > 1) {
        anim->elapsedTime += deltaTime;
        if (anim->elapsedTime >= anim->frameTime) {
            anim->elapsedTime = 0.0f;
            anim->currentFrame = (anim->currentFrame + 1) % getArrayLength(anim->frameTextures);
        }
    }

    ImGui::End();
}

void updateNewCanvasWindow(GameState *gameState) {
  if(gameState->showNewCanvasWindow) {
    //NOTE: Create new canvas
    ImGui::Begin("Canvas Size", &gameState->showNewCanvasWindow);       
    
    ImGui::Text("How big do you want the canvas?"); 
    if(gameState->autoFocus) {
       ImGui::SetKeyboardFocusHere();
       gameState->autoFocus = false;
    }
    ImGui::InputText("Width", gameState->dimStr0, IM_ARRAYSIZE(gameState->dimStr0));
    ImGui::InputText("Height", gameState->dimStr1, IM_ARRAYSIZE(gameState->dimStr1));
    if (ImGui::Button("Create")) {
      //  gameState->canvasW = atoi(gameState->dimStr0);
      //  gameState->canvasH = atoi(gameState->dimStr1);

       clearUndoRedoList(gameState);
       startCanvas(gameState);
       
       gameState->showNewCanvasWindow = false;
    }

    ImGui::End();
  }
}

void showMainMenuBar(GameState *state)
{
    if (ImGui::BeginMainMenuBar()) // Creates the top bar
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New")) { showNewCanvas(state); }
            if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Handle Open */ }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { saveProjectFile(state); }
            if (ImGui::MenuItem("Export", "Ctrl+E")) { saveFileToPNG(getActiveCanvas(state)); }
            if (ImGui::MenuItem("Exit")) { state->quit = true;  }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* Handle Undo */ }
            if (ImGui::MenuItem("Redo", "Ctrl+SHIFT+Z")) { /* Handle Redo */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* Handle Cut */ }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* Handle Copy */ }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* Handle Paste */ }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void updateMyImgui(GameState *state, ImGuiIO& io) {

  
      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      bool show_demo_window = true;

      ImVec2 window_pos = ImVec2(10.0f, io.DisplaySize.y - 10.0f); // Offset slightly from edges
      ImVec2 window_pivot = ImVec2(0, 1.0f); // Bottom-right corner

      ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);
      ImGui::SetNextWindowBgAlpha(0.35f); // Optional: Make it semi-transparent

      // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
      // ImGui::ShowDemoWindow(&show_demo_window);

      {
          ImGui::Begin("Color Palette");
          ImGui::ColorEdit3("Brush", (float*)&state->colorPicked);
          ImGui::SliderFloat("Opacity", &state->opacity, 0.0f, 1.0f);
          ImGui::ColorEdit3("Background", (float*)&state->bgColor);
          ImGui::Checkbox("Check Background", &state->checkBackground);
          ImGui::Checkbox("Draw Grid", &state->drawGrid); 
          ImGui::SliderFloat("Eraser", &state->eraserSize, 1.0f, 100.0f);


          if (ImGui::Button("\uf0b2")) {
            //NOTE: MOVE
            state->interactionMode = CANVAS_MOVE_MODE;
          } 
          if(state->interactionMode == CANVAS_MOVE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
          if (ImGui::Button("\uf1fc")) {
            //NOTE: BRUSH
            state->interactionMode = CANVAS_DRAW_MODE;
          } 
          if(state->interactionMode == CANVAS_DRAW_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
          
          if (ImGui::Button("\uf575")) {
            //NOTE: FILL
            state->interactionMode = CANVAS_FILL_MODE;
          } 
          if(state->interactionMode == CANVAS_FILL_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
          
          if (ImGui::Button("\uf111")) {
            //NOTE: circle shape
            state->interactionMode = CANVAS_DRAW_CIRCLE_MODE;
          }
          if(state->interactionMode == CANVAS_DRAW_CIRCLE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
          
          if (ImGui::Button("\uf0c8")) {
            //NOTE: rectangle shape
            state->interactionMode = CANVAS_DRAW_RECTANGLE_MODE;
          }
          if(state->interactionMode == CANVAS_DRAW_RECTANGLE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
          
          if (ImGui::Button("\uf12d")) {
            state->interactionMode = CANVAS_ERASE_MODE;
            
          }
          if(state->interactionMode == CANVAS_ERASE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}

          if (ImGui::Button("\uf068")) {
            state->interactionMode = CANVAS_DRAW_LINE_MODE;
            
          }
          if(state->interactionMode == CANVAS_DRAW_LINE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}

          

          ImGui::End();
      }

      showMainMenuBar(state);
      updateNewCanvasWindow(state);
      drawTabs(state);

      drawAnimationTimeline(state, state->dt);
}



void imguiEndFrame() {
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool isInteractingWithIMGUI() {
  return (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered());
}

