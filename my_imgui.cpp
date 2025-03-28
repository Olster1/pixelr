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
            
              if (ImGui::BeginTabItem(t->fileName, &t->isOpen))
              {
                if (ImGui::IsItemClicked()) 
                {
                    state->activeCanvasTab = i;
                }
                
              
                  ImGui::EndTabItem();
              }

              ImGui::PopID();
            } 

           
  
              ImGui::EndTabBar();

               // Align button next to the tab bar
                ImGui::SameLine();
                
                if (ImGui::Button("+"))
                {
                    showNewCanvas(state);

                }
          }
      }
      ImGui::End(); // End the top bar window

      for(int i = 0; i < getArrayLength(state->canvasTabs); ) {
        CanvasTab *t = state->canvasTabs + i;

        

        if(!t->isOpen) {
          t->dispose();
          //NOTE: Remove from the list
          bool found = removeArrayAtIndex(state->canvasTabs, i);
          assert(found);
        } else {
          ++i;
        }
      }
  
}

void drawAnimationTimeline(GameState *state, float deltaTime) {

    CanvasTab *canvasTab = getActiveCanvasTab(state);

    if(canvasTab) {
      PlayBackAnimation *anim = &canvasTab->playback;

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
      ImGui::SameLine();
      if (ImGui::Button("\uf1f8 Delete Frame")) {
          if(getArrayLength(canvasTab->frames) > 1) {
            //TODO: Delete the frame add to undo list
            canvasTab->frames[canvasTab->activeFrame].dispose();
            removeArrayAtIndex(canvasTab->frames, canvasTab->activeFrame);
            canvasTab->activeFrame--;
            if(canvasTab->activeFrame < 0) {
              canvasTab->activeFrame = 0;
            }
          }
      }

      ImGui::Separator();

      // Display all frames in a row
      if (getArrayLength(canvasTab->frames) > 0) {
        ImGui::Text("Frames:");
    
        for (int i = 0; i < getArrayLength(canvasTab->frames); i++) {
            if (i > 0) ImGui::SameLine(); // Keep frames on the same line
    
            ImVec2 imageSize(64, 64); // Adjust as needed
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImVec4 borderColor = (i == canvasTab->activeFrame) ? ImVec4(1, 1, 0, 1) : ImVec4(0, 0, 0, 0); // Yellow for current frame
    
            // Make a button that overlays the image so we can detect clicks
            ImGui::PushID(i); // Unique ID for each frame
            if (ImGui::InvisibleButton("##frame", imageSize)) {
                canvasTab->activeFrame = i; // Change current frame
                anim->playing = false; // Stop animation
            }
            ImGui::PopID();
    
            // Get the position where the image will be drawn
            ImVec2 imagePos = cursorPos;  

            // Get the draw list
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Draw a white background rectangle
            drawList->AddRectFilled(imagePos, ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y), IM_COL32(255, 255, 255, 255));

            // Draw the image on top
            ImGui::SetCursorScreenPos(imagePos); // Reset position to draw the image
            ImGui::Image((ImTextureID)(intptr_t)canvasTab->frames[i].gpuHandle, imageSize, ImVec2(0, 1), ImVec2(1, 0));
    
            // Draw border if this is the current frame
            if (i == canvasTab->activeFrame) {
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
      if(state->onionSkinningFrames > (getArrayLength(canvasTab->frames) - 1)) {
        state->onionSkinningFrames = getArrayLength(canvasTab->frames) - 1;
      }
      ImGui::SliderInt("Onion Skinning", &state->onionSkinningFrames, 0, MathMax(getArrayLength(canvasTab->frames) - 1, 0));
      

      ImGui::Checkbox("Copy Frame on add", &state->copyFrameOnAdd);
      if (ImGui::Button("Add New Frame +")) {
          Frame f = Frame(canvasTab->w, canvasTab->h);
          pushArrayItem(&canvasTab->frames, f, Frame);
          Frame *activeFrame = getActiveFrame(state);
          if(activeFrame && state->copyFrameOnAdd) {
            easyPlatform_copyMemory(f.layers[0].pixels, activeFrame->layers[0].pixels, sizeof(u32)*canvasTab->w*canvasTab->h);
            updateGpuCanvasTextures(state);
          }

          
          canvasTab->activeFrame = getArrayLength(canvasTab->frames) - 1;
      }

      // Animation playback logic
      if (anim->playing && getArrayLength(canvasTab->frames) > 1) {
          anim->elapsedTime += deltaTime;
          if (anim->elapsedTime >= anim->frameTime) {
              anim->elapsedTime = 0.0f;
              canvasTab->activeFrame = (canvasTab->activeFrame + 1) % getArrayLength(canvasTab->frames);
          }
      }

      ImGui::End();
    }
}

void exportWindow(GameState *gameState) {
  if(gameState->showExportWindow) {
    //NOTE: Create new canvas
    ImGui::Begin("Export Sprite Sheet", &gameState->showExportWindow);       
    
    ImGui::Text("How many rows & columns?"); 
    if(gameState->autoFocus) {
       ImGui::SetKeyboardFocusHere();
       gameState->autoFocus = false;
    }
    ImGui::InputText("Columns", gameState->dimStr0, IM_ARRAYSIZE(gameState->dimStr0), (gameState->maxColumnsExport) ? ImGuiInputTextFlags_ReadOnly : 0); 
    ImGui::SameLine();
    if(ImGui::Checkbox("Max", &gameState->maxColumnsExport)) {
      if(gameState->maxColumnsExport) {
        CanvasTab *tab = getActiveCanvasTab(gameState);
        if(tab) {
          snprintf(gameState->dimStr0, IM_ARRAYSIZE(gameState->dimStr0), "%d", getArrayLength(tab->frames));
          snprintf(gameState->dimStr1, IM_ARRAYSIZE(gameState->dimStr1), "%d", 1);
        }
      }
    }
    ImGui::InputText("Rows", gameState->dimStr1, IM_ARRAYSIZE(gameState->dimStr1), (gameState->maxColumnsExport) ? ImGuiInputTextFlags_ReadOnly : 0);
    if (ImGui::Button("Export")) {
      int w = atoi(gameState->dimStr0);
      int h = atoi(gameState->dimStr1);

      saveSpriteSheetToPNG(getActiveCanvasTab(gameState), w, h);

       gameState->showExportWindow = false;
    }

    ImGui::End();
  }
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
      int w = atoi(gameState->dimStr0);
      int h = atoi(gameState->dimStr1);
      CanvasTab tab = CanvasTab(w, h, easyString_copyToHeap("Untitled"));
      pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
      gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;

       gameState->showNewCanvasWindow = false;
    }

    ImGui::End();
  }
}

void setDefaultSpriteSize(GameState *gameState) {
  assert(IM_ARRAYSIZE(gameState->dimStr0) >= 3);
  gameState->dimStr0[0] = '1';
  gameState->dimStr0[1] = '6';
  gameState->dimStr0[2] = '\0';

  assert(IM_ARRAYSIZE(gameState->dimStr1) >= 3);
  gameState->dimStr1[0] = '1';
  gameState->dimStr1[1] = '6';
  gameState->dimStr1[2] = '\0';
  
}

void updateSpriteSheetWindow(GameState *gameState) {
  if(gameState->openSpriteSheetWindow) {
    //NOTE: Create new canvas
    ImGui::Begin("Load Sprite Sheet", &gameState->openSpriteSheetWindow);       

    ImGui::Text("Individual Sprite size?"); 
    ImGui::InputText("Width", gameState->dimStr0, IM_ARRAYSIZE(gameState->dimStr0));
    ImGui::InputText("Height", gameState->dimStr1, IM_ARRAYSIZE(gameState->dimStr1));
    if (ImGui::Button("Load")) {
      int w = atoi(gameState->dimStr0);
      int h = atoi(gameState->dimStr1);

      openSpriteSheet(gameState, w, h);
    }

    ImGui::End();
  }
}


void updateColorPaletteEnter(GameState *gameState) {
  if(gameState->showColorPalleteEnter) {
    //NOTE: Create new canvas
    ImGui::Begin("Enter Color Pallete", &gameState->showColorPalleteEnter);       
    
    if(gameState->blueRedFlippedInUI) {
      ImGui::Text("Enter a comma seperated OxAARRGGBB colors (Same as Html color layout)"); 
    } else {
      ImGui::Text("Enter a comma seperated OxAABBGGRR colors"); 
    }
    ImGui::Checkbox("Flip Red & Blue channels", &gameState->blueRedFlippedInUI);
    
    ImGui::InputTextMultiline("Comma sperated values", gameState->colorsPalleteBuffer, sizeof(gameState->colorsPalleteBuffer), ImVec2(400, 200), ImGuiInputTextFlags_AllowTabInput);
    if (ImGui::Button("Add")) {
      //NOTE: Add the colors
      EasyTokenizer tokenizer = lexBeginParsing(gameState->colorsPalleteBuffer, EASY_LEX_OPTION_EAT_WHITE_SPACE);

        bool parsing = true;
        while(parsing) {
            EasyToken t = lexGetNextToken(&tokenizer);

            if(t.type == TOKEN_NULL_TERMINATOR) {
                parsing = false;
            } else if(t.type == TOKEN_COMMA) {

            } else if(t.type == TOKEN_INTEGER) {
              float4 color = u32_to_float4_color(t.intVal);

              if(gameState->blueRedFlippedInUI) {
                float r = color.x;
                color.x = color.z;
                color.z = r;
              }
              
              CanvasTab *tab = getActiveCanvasTab(gameState);
              if(tab) {
                tab->addColorToPalette(float4_to_u32_color(color));
              }
            }
          }

      

    // for(int i = 0; i < arrayCount(colors); ++i) {
    //   float4 a = u32_to_float4_color(colors[i]);
    //   //NOTE: Expects blue to be in the 16th bit spot and red to be in the 0th bit spot
    //   gameState->colorsPallete[gameState->palletteCount++] = a;
    //   assert(gameState->colorsPallete[i].w = 1);
    // }

      gameState->showColorPalleteEnter = false;
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
            if (ImGui::MenuItem("Open Image", "Ctrl+O")) { openPlainImage(state); }
            if (ImGui::MenuItem("Load Sprite Sheet", "")) { setDefaultSpriteSize(state); state->openSpriteSheetWindow = true; }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {  }
            if (ImGui::MenuItem("Save Pallete", "")) {  }
            if (ImGui::MenuItem("Load Pallete", "")) {  }
            
            if (ImGui::MenuItem("Export Image", "Ctrl+E")) { saveFileToPNG(getActiveCanvas(state)); }
            if (ImGui::MenuItem("Export Sprite Sheet", "")) { state->showExportWindow = true; }
            if (ImGui::MenuItem("Exit")) { state->quit = true;  }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) { updateUndoState(state, true, false); }
            if (ImGui::MenuItem("Redo", "Ctrl+SHIFT+Z")) { updateUndoState(state, false, true); }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* Handle Cut */ }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* Handle Copy */ }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* Handle Paste */ }
            if (ImGui::MenuItem("Add Color Pallete", "")) { 
              state->colorsPalleteBuffer[0] = '\0';
              state->showColorPalleteEnter = true; 
            }
            
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void updateMyImgui(GameState *state, ImGuiIO& io) {

      CanvasInteractionMode startMode = state->interactionMode;

      CanvasTab *tab = getActiveCanvasTab(state);
  
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
      if(tab) {
        {
            ImGui::Begin("Color Palette");
            ImGui::ColorEdit3("Brush", (float*)&tab->colorPicked);
            // Detect when the color picker is closed after changes
            ImGui::SliderFloat("Opacity", &state->opacity, 0.0f, 1.0f);
            ImGui::SliderInt("Running Average", &state->runningAverageCount, 1, arrayCount(state->mouseP_01_array));
            ImGui::ColorEdit3("Background", (float*)&state->bgColor);
            ImGui::Checkbox("Check Background", &state->checkBackground);
            ImGui::Checkbox("nearest", &state->nearest);
            ImGui::Checkbox("Draw Grid", &state->drawGrid); 
            ImGui::SliderFloat("Eraser", &state->eraserSize, 1.0f, 100.0f);
            
            ImGui::Checkbox("SELECT", &state->selectMode);


            if (ImGui::Button("\uf0b2")) {
              //NOTE: MOVE
              state->interactionMode = CANVAS_MOVE_MODE;
            } 
            if(state->interactionMode == CANVAS_MOVE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf1fc")) {
              //NOTE: BRUSH
              state->interactionMode = CANVAS_DRAW_MODE;
            } 
            if(state->interactionMode == CANVAS_DRAW_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf575")) {
              //NOTE: FILL
              state->interactionMode = CANVAS_FILL_MODE;
            } 
            if(state->interactionMode == CANVAS_FILL_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf111")) {
              //NOTE: circle shape
              state->interactionMode = CANVAS_DRAW_CIRCLE_MODE;
            }
            if(state->interactionMode == CANVAS_DRAW_CIRCLE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf245")) {
              //NOTE: select shape shape
              state->interactionMode = CANVAS_MOVE_SELECT_MODE;
            }
            if(state->interactionMode == CANVAS_MOVE_SELECT_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            
            if (ImGui::Button("\uf0c8")) {
              //NOTE: rectangle shape
              state->interactionMode = CANVAS_DRAW_RECTANGLE_MODE;
            }
            if(state->interactionMode == CANVAS_DRAW_RECTANGLE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf12d")) {
              state->interactionMode = CANVAS_ERASE_MODE;
              
            }
            if(state->interactionMode == CANVAS_ERASE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf068")) {
              state->interactionMode = CANVAS_DRAW_LINE_MODE;
              
            }
            if(state->interactionMode == CANVAS_DRAW_LINE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf248")) {
              state->interactionMode = CANVAS_SELECT_RECTANGLE_MODE;
              
            }
            if(state->interactionMode == CANVAS_SELECT_RECTANGLE_MODE) { ImGui::SameLine(); ImGui::Text("\uf00c");}
            ImGui::SameLine();
            if (ImGui::Button("\uf5bd")) {
              state->interactionMode = CANVAS_SPRAY_CAN;
              
            }
            if(state->interactionMode == CANVAS_SPRAY_CAN) { ImGui::SameLine(); ImGui::Text("\uf00c");}

            ImGui::SameLine();
            if (ImGui::Button("\uf1fb")) {
              state->interactionMode = CANVAS_COLOR_DROPPER;
              
            }
            if(state->interactionMode == CANVAS_COLOR_DROPPER) { ImGui::SameLine(); ImGui::Text("\uf00c");}

            ImGui::Text("Select a Color:");
            float size = 30.0f;  // Square size
            float spacing = 5.0f; // Spacing between squares

            CanvasTab *tab = getActiveCanvasTab(state);
        
            if(tab) {
              for (int i = 0; i < tab->palletteCount; ++i) {
                  ImGui::PushID(i);  // Ensure unique ID for each color
          
                  // Get cursor position for drawing the square
                  ImVec2 pos = ImGui::GetCursorScreenPos();
                  ImDrawList* drawList = ImGui::GetWindowDrawList();
          
                  // Create an invisible button for the selectable color square
                  if (ImGui::InvisibleButton("color_btn", ImVec2(size, size))) {
                    tab->colorPicked = u32_to_float4_color(tab->colorsPallete[i]);
                  }
                  u32 a = tab->colorsPallete[i];

                  // Draw color square
                  drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), (a));
          
                  // Draw border if selected
                  if (float4_to_u32_color(tab->colorPicked) == a) {
                      drawList->AddRect(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(255, 255, 255, 255), 0.0f, 0, 3.0f);
                  }
          
                  ImGui::PopID();
          
                  // Layout: Move to next row if needed
                  if ((i + 1) % 5 != 0) {
                      ImGui::SameLine(0, spacing);
                  }
            }
          }

            

            ImGui::End();
        }

        showMainMenuBar(state);
        exportWindow(state);
        updateNewCanvasWindow(state);
        updateColorPaletteEnter(state);
        updateSpriteSheetWindow(state);
        drawTabs(state);

        drawAnimationTimeline(state, state->dt);

        if(startMode != state->interactionMode) {
          clearSelection(tab);
        }
      }
}



void imguiEndFrame() {
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

