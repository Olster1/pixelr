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

void closeCanvasTabUI(GameState *state, CanvasTab *t, int i) {
  if(t) {
    t->dispose();
    //NOTE: Remove from the list
    bool found = removeArrayAtIndex(state->canvasTabs, i);
    assert(found);

    if(i == state->activeCanvasTab)  {
      state->activeCanvasTab--;
      if(state->activeCanvasTab < 0) {
        state->activeCanvasTab = 0;
      }
    }
    CanvasTab *tab = getActiveCanvasTab(state);
    if(tab) {
      tab->uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;
    }
  }
}

void drawTabs(GameState *state) {
      // Push a style to keep the tab bar at the top of the screen
      ImGui::SetNextWindowPos(ImVec2(0, 15));
      ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 50));
  
      // Begin a window without decorations, keeping it at the top
      if (ImGui::Begin("Top Tab Bar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground))
      {
          if (ImGui::BeginTabBar("TopTabs", ImGuiTabBarFlags_Reorderable))
          {

            for(int i = 0; i < getArrayLength(state->canvasTabs); ++i) {
              CanvasTab *t = state->canvasTabs + i;

              char *id = easy_createString_printf(&globalPerFrameArena, "%ld", t->frames);
              ImGui::PushID(id);

              u32 unsavedFlag = ImGuiTabItemFlags_UnsavedDocument;

              //NOTE: Check if the tab is at a saved state
              if(isCanvasTabSaved(t)) 
              {
                unsavedFlag = 0;
              }
            
              if (ImGui::BeginTabItem(t->fileName, &t->isOpen, t->uiTabSelectedFlag | unsavedFlag))
              {
                if (state->activeCanvasTab != i)
                {
                    state->activeCanvasTab = i;
                    CanvasTab *tab = getActiveCanvasTab(state);
                    if(tab) {
                      //NOTE: Reset the camera position
                      state->camera.T.pos.xy = tab->cameraP;
                      state->camera.fov = tab->zoomFactor;
                    }
                }
              
                  ImGui::EndTabItem();
              }
              //NOTE: Clear the ui selected flags now
              t->uiTabSelectedFlag = 0;

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
          if(isCanvasTabSaved(t)) {
              closeCanvasTabUI(state, t, i);
          } else {
            state->closeCanvasTabInfo.UIshowUnsavedWindow = true;
            state->closeCanvasTabInfo.canvasTab = t;
            state->closeCanvasTabInfo.arrayIndex = i;
            t->uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;  
            t->isOpen = true;
            i++;
          }
        } else {
          ++i;
        }
      }
}

int getActiveCanvasCount(Frame *frame) { 
  int result = 0;
  for (int i = 0; i < getArrayLength(frame->layers); i++) {
    if(!frame->layers[i].deleted) {
      result++;
    }
  }
  return result;
}

void drawChip(char *chipTitle) {
  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 200, 255, 255));  // background
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(170, 170, 230, 255));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 150, 210, 255));

  if (ImGui::Button(chipTitle)) {
      
  }

  ImGui::PopStyleColor(3);
}

void drawAnimationTimeline(GameState *state, float deltaTime) {

    CanvasTab *canvasTab = getActiveCanvasTab(state);

    if(canvasTab) {
      PlayBackAnimation *anim = &canvasTab->playback;

      ImGui::SetNextWindowBgAlpha(1);
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
            canvasTab->frames[canvasTab->activeFrame].deleted = true;
            FrameInfo frameUndoInfo = {};
            frameUndoInfo.canvasType = UNDO_REDO_FRAME_DELETE;
            frameUndoInfo.frameIndex = canvasTab->activeFrame;
            frameUndoInfo.beforeActiveLayer = canvasTab->activeFrame;
             
            canvasTab->activeFrame--;
            if(canvasTab->activeFrame < 0) {
              canvasTab->activeFrame = 0;
            }
            frameUndoInfo.afterActiveLayer = canvasTab->activeFrame;

            canvasTab->addUndoInfo(frameUndoInfo);
          }
      }

      ImGui::Separator();

      // Display all frames in a row
      if (getArrayLength(canvasTab->frames) > 0) {
        ImGui::Text("Frames:");
        
        int addedIndex = 0;
        for (int i = 0; i < getArrayLength(canvasTab->frames); i++) {
          Frame *frame = canvasTab->frames + i;
          if(!frame->deleted) {
            if (addedIndex > 0) ImGui::SameLine(); // Keep frames on the same line
    
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
            ImGui::Image((ImTextureID)(intptr_t)canvasTab->frames[i].frameBufferHandle.textureHandle, imageSize, ImVec2(0, 1), ImVec2(1, 0));
    
            // Draw border if this is the current frame
            if (i == canvasTab->activeFrame) {
                ImGui::GetWindowDrawList()->AddRect(
                    cursorPos, ImVec2(cursorPos.x + imageSize.x, cursorPos.y + imageSize.y),
                    IM_COL32(255, 255, 0, 255), // Yellow border
                    0.0f, 0, 2.0f // Thickness
                );
            }
            addedIndex++;
          }
        }
    }

      ImGui::Separator();

      ImGui::SliderFloat("Time per Frame", &anim->frameTime, 0.01f, 2.0f);
      if(canvasTab->onionSkinningFrames > (getArrayLength(canvasTab->frames) - 1)) {
        canvasTab->onionSkinningFrames = getArrayLength(canvasTab->frames) - 1;
      }
      ImGui::SliderInt("Onion Skinning", &canvasTab->onionSkinningFrames, 0, MathMax(getArrayLength(canvasTab->frames) - 1, 0));
      

      ImGui::Checkbox("Copy Frame on add", &canvasTab->copyFrameOnAdd);
      if (ImGui::Button("Add New Frame +")) {
          Frame f_ = Frame(canvasTab->w, canvasTab->h);
          Frame *f = pushArrayItem(&canvasTab->frames, f_, Frame);
          Frame *activeFrame = getActiveFrame(state);
          if(activeFrame && canvasTab->copyFrameOnAdd) {
            int addedIndex = 0;
            for(int j = 0; j < getArrayLength(activeFrame->layers); ++j) {
              if(!activeFrame->layers[j].deleted) {
                if(addedIndex > 0) {
                  //NOTE: Creating a frame automatically adds a canvas, so use that one first
                  Canvas newCanvas = Canvas(canvasTab->w, canvasTab->h);
                  pushArrayItem(&f->layers, newCanvas, Canvas);
                }
                easyPlatform_copyMemory(f->layers[j].pixels, activeFrame->layers[j].pixels, sizeof(u32)*canvasTab->w*canvasTab->h);
                addedIndex++;
              }
              
            }
            updateGpuCanvasTextures(state);
          }

          FrameInfo frameUndoInfo = {};
          frameUndoInfo.canvasType = UNDO_REDO_FRAME_CREATE;
          frameUndoInfo.frameIndex = getArrayLength(canvasTab->frames) - 1;
          frameUndoInfo.beforeActiveLayer = canvasTab->activeFrame;
          canvasTab->activeFrame = getArrayLength(canvasTab->frames) - 1;
          printf("%d\n", getArrayLength(canvasTab->frames));
          frameUndoInfo.afterActiveLayer = canvasTab->activeFrame;
          canvasTab->addUndoInfo(frameUndoInfo);
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

void outputLayerList(GameState *state, CanvasTab *canvasTab, Frame *activeFrame) {
  if (ImGui::BeginTable("Layers Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
    for (int i = 0; i < getArrayLength(activeFrame->layers); i++)
    {
        if (activeFrame->layers[i].deleted) continue;

        ImGui::TableNextRow();

        // --- Column 1: Thumbnail ---
        ImGui::TableNextColumn();

        ImVec2 imageSize(64, 64);
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();

        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##frame", imageSize))
        {
            activeFrame->activeLayer = i;
        }
        ImGui::PopID();

        // Draw white background
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(cursorPos,
            ImVec2(cursorPos.x + imageSize.x, cursorPos.y + imageSize.y),
            IM_COL32(255, 255, 255, 255));

        ImGuiDragDropFlags src_flags = 0;
        src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
        src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
        if (ImGui::BeginDragDropSource(src_flags))
        {
            ImGui::SetDragDropPayload("LAYER_ROW", &i, sizeof(int));
            ImGui::Text("Moving Layer %d", i);
            ImGui::EndDragDropSource();
        }
        
        // Accept drop target (this row)
        if (ImGui::BeginDragDropTarget())
        { 
          const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_ROW");
            if (payload)
            {
                int srcIndex = *(const int*)payload->Data;
                if (srcIndex != i)
                {
                    // swap layers
                    Canvas temp = activeFrame->layers[i];
                    activeFrame->layers[i] = activeFrame->layers[srcIndex];
                    activeFrame->layers[srcIndex] = temp;

                    //NOTE: Add Undo redo action
                    FrameInfo frameUndoInfo = {};
                    frameUndoInfo.canvasType = UNDO_REDO_CANVAS_SWAP;
                    frameUndoInfo.frameIndex = canvasTab->activeFrame;
                    frameUndoInfo.canvasIndex = i;
                    frameUndoInfo.canvasIndexB = srcIndex;

                    frameUndoInfo.beforeActiveLayer = activeFrame->activeLayer;

                    if (activeFrame->activeLayer == srcIndex) {
                      activeFrame->activeLayer = i;
                    }

                    frameUndoInfo.afterActiveLayer = activeFrame->activeLayer;
                    canvasTab->addUndoInfo(frameUndoInfo);
                        
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- AUTO SCROLL WHILE DRAGGING ---
        if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
        {
            if (payload->IsDataType("LAYER_ROW")) // only for our row drags
            {
                // Visible area of the current window
                ImVec2 windowPos  = ImGui::GetWindowPos();
                ImVec2 windowSize = ImGui::GetWindowSize();

                float scrollY     = ImGui::GetScrollY();
                float scrollMaxY  = ImGui::GetScrollMaxY();

                ImVec2 mousePos   = ImGui::GetIO().MousePos;

                const float scrollZone  = 30.0f;  // px from edge
                const float scrollSpeed = 10.0f;  // px per frame

                // Near top? scroll up
                if (mousePos.y < windowPos.y + scrollZone && scrollY > 0.0f)
                {
                    ImGui::SetScrollY(scrollY - scrollSpeed);
                }

                // Near bottom? scroll down
                if (mousePos.y > windowPos.y + windowSize.y - scrollZone && scrollY < scrollMaxY)
                {
                    ImGui::SetScrollY(scrollY + scrollSpeed);
                }
            }
        }

        // Draw actual image
        ImGui::SetCursorScreenPos(cursorPos);
        ImGui::Image((ImTextureID)(intptr_t)activeFrame->layers[i].gpuHandle,
                     imageSize, ImVec2(0, 1), ImVec2(1, 0));

        // Yellow border if active
        if (i == activeFrame->activeLayer)
        {
            drawList->AddRect(cursorPos,
                ImVec2(cursorPos.x + imageSize.x, cursorPos.y + imageSize.y),
                IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }

        // --- Column 2: Buttons ---
        ImGui::TableNextColumn();
        ImGui::PushID(i);
        if (ImGui::Button("\uf1f8"))
        {
            // Delete canvas logic
            if (getActiveCanvasCount(activeFrame) > 1)
            {
                FrameInfo frameUndoInfo = {};
                activeFrame->layers[i].deleted = true;
                frameUndoInfo.canvasType = UNDO_REDO_CANVAS_DELETE;
                frameUndoInfo.frameIndex = canvasTab->activeFrame;
                frameUndoInfo.canvasIndex = i;

                frameUndoInfo.beforeActiveLayer = activeFrame->activeLayer;

                if (i >= activeFrame->activeLayer)
                    activeFrame->activeLayer--;
                if (activeFrame->activeLayer < 0)
                    activeFrame->activeLayer = 0;

                frameUndoInfo.afterActiveLayer = activeFrame->activeLayer;
                canvasTab->addUndoInfo(frameUndoInfo);
            }
        }
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::Button((activeFrame->layers[i].visible) ? "\uf06e" : "\uf070"))
        {
            activeFrame->layers[i].visible = !activeFrame->layers[i].visible;
            updateGpuCanvasTextures(state);
        }
        ImGui::PopID();

        // char *id = easy_createString_printf(&globalPerFrameArena, "%s%d", "dragHandle", i);
        // ImGui::PushID(id);
        // // small “handle” button with ≡ style
        // if (ImGui::Button("\uf58e"))
        // {
        //     // optional normal click action
        // }
        // ImGui::PopID();

        
        
    }
  ImGui::EndTable();
}

}

void imgui_createNewCanvasFromSize(GameState *gameState, int w, int h) {
  CanvasTab tab = CanvasTab(w, h, 0);
  pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
  gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
  gameState->showNewCanvasWindow = false;

  CanvasTab *newTab = getActiveCanvasTab(gameState);
  if(newTab) {
    newTab->uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;  
  }
}

void drawLayersWindow(GameState *state, float deltaTime) {

    CanvasTab *canvasTab = getActiveCanvasTab(state);

    if(canvasTab) {
      ImGuiIO& io = ImGui::GetIO();
      ImVec2 window_pos = ImVec2(io.DisplaySize.x, 20);
      ImVec2 window_pivot = ImVec2(1.0f, 0.0f);

      ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);
      ImGui::SetNextWindowBgAlpha(1);
      ImGui::Begin("Layers");

      Frame *activeFrame = getActiveFrame(state);
      // Display all frames in a row
      if (activeFrame && getArrayLength(activeFrame->layers) > 0) {
        ImGui::Text("Layers:");
        outputLayerList(state, canvasTab, activeFrame);
      }

      ImGui::Separator();
      if (ImGui::Button("Add New Layer +")) {
          Canvas f = Canvas(canvasTab->w, canvasTab->h);
          pushArrayItem(&activeFrame->layers, f, Canvas);
          // if(activeFrame && canvasTab->copyFrameOnAdd) {
          //   easyPlatform_copyMemory(f.layers[0].pixels, activeFrame->layers[0].pixels, sizeof(u32)*canvasTab->w*canvasTab->h);
            updateGpuCanvasTextures(state);
          // }

          FrameInfo frameUndoInfo = {};
          frameUndoInfo.canvasType = UNDO_REDO_CANVAS_CREATE;
          frameUndoInfo.frameIndex = canvasTab->activeFrame;
          frameUndoInfo.canvasIndex = getArrayLength(activeFrame->layers) - 1; 
          frameUndoInfo.beforeActiveLayer = activeFrame->activeLayer;
          activeFrame->activeLayer = getArrayLength(activeFrame->layers) - 1;
          frameUndoInfo.afterActiveLayer = activeFrame->activeLayer;
          canvasTab->addUndoInfo(frameUndoInfo);
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

      saveSpriteSheetToPNG(gameState->renderer, getActiveCanvasTab(gameState), w, h);

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
    ImGui::Spacing();    
    if (ImGui::Button("16 x 16")) {
      imgui_createNewCanvasFromSize(gameState, 16, 16);
    }
    ImGui::SameLine();
    if (ImGui::Button("32 x 32")) {
      imgui_createNewCanvasFromSize(gameState, 32, 32);
    }
    ImGui::SameLine();
    if (ImGui::Button("64 x 64")) {
      imgui_createNewCanvasFromSize(gameState, 64, 64);
    }
    if (ImGui::Button("128 x 128")) {
      imgui_createNewCanvasFromSize(gameState, 128, 128);
    }
    ImGui::SameLine();
    if (ImGui::Button("192 x 192")) {
      imgui_createNewCanvasFromSize(gameState, 192, 192);
    }
    ImGui::SameLine();
    if (ImGui::Button("256 x 256")) {
      imgui_createNewCanvasFromSize(gameState, 256, 256);
    }
    if (ImGui::Button("512 x 512")) {
      imgui_createNewCanvasFromSize(gameState, 512, 512);
    }
    ImGui::SameLine();
    if (ImGui::Button("1024 x 1024")) {
      imgui_createNewCanvasFromSize(gameState, 1024, 1024);
    }
    ImGui::SameLine();
    if (ImGui::Button("2048 x 2048")) {
      imgui_createNewCanvasFromSize(gameState, 2048, 2048);
    }
    ImGui::Spacing();    
    ImGui::Spacing();    
    if (ImGui::Button("Create")) {
      int w = atoi(gameState->dimStr0);
      int h = atoi(gameState->dimStr1);
      if(w > 0 && h > 0) {
        imgui_createNewCanvasFromSize(gameState, w, h);
      }
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

      if(w > 0 && h > 0) {
        openSpriteSheet(gameState, w, h);
      } else {
        addIMGUIToast("Width & Height must be more than zero.", 2);
      }
    }

    ImGui::End();
  }
}

void outputPalette(GameState *gameState, CanvasTab *tab, u32 *palette, int count, float size, float spacing) {
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  ImVec2 pos = ImGui::GetCursorScreenPos();

  int perRow = 10; // how many per line
  for (int i = 0; i < count; ++i) {
      int col = i % perRow;         // column index
      int row = i / perRow;         // row index

      ImVec2 p_min = ImVec2(
          pos.x + col * (size + spacing),
          pos.y + row * (size + spacing)
      );
      ImVec2 p_max = ImVec2(p_min.x + size, p_min.y + size);

      drawList->AddRectFilled(p_min, p_max, palette[i]);
  }

  // Advance ImGui’s cursor for the whole drawn area:
  int rows = (count + perRow - 1) / perRow; // ceiling division
  ImGui::Dummy(ImVec2(
      perRow * (size + spacing),
      rows * (size + spacing)
  ));
  ImGui::PushID(palette);  // Ensure unique ID for each color
  if (ImGui::Button("Load Palette")) {
      if(gameState->clearPaletteOnLoad) {
          tab->palletteCount = 0;
      }
      for (int i = 0; i < count; ++i) {
        tab->addColorToPalette(palette[i]);
      }
  }
  ImGui::PopID();  // Ensure unique ID for each color
}

void updateEditPaletteWindow (GameState *gameState) {
  if(gameState->showEditPalette) {
    //NOTE: Edit palette 
    ImGui::Begin("Edit Palette", &gameState->showEditPalette);    
    ImGui::Text("Your Palette");   
    ImGui::Dummy(ImVec2(0.0f, 10.0f)); // vertical padding

    float size = 30.0f;  // Square size
    float spacing = 5.0f; // Spacing between squares

    CanvasTab *tab = getActiveCanvasTab(gameState);

    u32 colorPicked = 0x00000000;

    if(gameState->editPaletteIndex >= tab->palletteCount) {
      gameState->editPaletteIndex = tab->palletteCount - 1;
    }

      if(tab) {
        ImVec2 child_size = ImVec2(0, 200); // width 0 = fill, height = 200px
        ImGui::BeginChild("ScrollingRegion", child_size, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int i = 0; i < tab->palletteCount; ++i) {
            ImGui::PushID(i);  // Ensure unique ID for each color
    
            // Get cursor position for drawing the square
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
    
            // Create an invisible button for the selectable color square
            if (ImGui::InvisibleButton("color_btn", ImVec2(size, size))) {
              colorPicked = tab->colorsPallete[i];
              gameState->editPaletteIndex = i;
            }
            u32 a = tab->colorsPallete[i];

            // Draw color square
            drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), (a));
    
            // Draw border if selected
            if (i == gameState->editPaletteIndex) {
                drawList->AddRect(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(255, 255, 255, 255), 0.0f, 0, 3.0f);
            }
    
            ImGui::PopID();
    
            // Layout: Move to next row if needed
            if ((i + 1) % 10 != 0) {
                if(i < tab->palletteCount -1) {
                  ImGui::SameLine(0, spacing);
                }
                
            }
      }
      ImGui::EndChild();
    }
   ImGui::Dummy(ImVec2(0.0f, 10.0f)); // vertical padding
    
    if (ImGui::Button("Delete Color")) {
      if(gameState->editPaletteIndex >= 0) {
        for(int i = gameState->editPaletteIndex; i < (tab->palletteCount - 1); ++i) {
          tab->colorsPallete[i] = tab->colorsPallete[i + 1];
        }
        gameState->editPaletteIndex--;
        tab->palletteCount--;
        if(tab->palletteCount < 0) {
          tab->palletteCount = 0;
        }
        savePalleteDefaultThreaded(globalThreadInfo, tab);
      }
      
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Palette")) {
      savePallete(tab);
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Bulk")) {
      gameState->showColorPalleteEnter = true;
    }
   
    
    char *currentString = "";
    if (ImGui::Button("Copy All to Clipoard")) {
      for (int j = 0; j < tab->palletteCount; ++j) {
        u32 color = tab->colorsPallete[j];
        currentString = easy_createString_printf(&globalPerFrameArena, "%s0x%x,", currentString, color);
      }
      //NOTE: Add to the clipbaord now
      platform_copyToClipboard(currentString);
    }
    ImGui::SameLine();
     if (ImGui::Button("Delete All Colors")) {
      tab->palletteCount = 0;
      savePalleteDefaultThreaded(globalThreadInfo, tab);
      
    }
    
    ImGui::Dummy(ImVec2(0.0f, 10.0f)); // vertical padding
    ImGui::Separator();
    ImGui::Text("Load Palettes");
    ImGui::Dummy(ImVec2(0.0f, 10.0f)); // vertical padding
    ImGui::Checkbox("Clear Palette on palette load", &gameState->clearPaletteOnLoad);
    ImGui::Dummy(ImVec2(0.0f, 10.0f)); // vertical padding
    if (ImGui::Button("Load PNG color palette")) {
      loadPngColorPalette(tab, gameState->clearPaletteOnLoad);
      
    }
    
    ImGui::Dummy(ImVec2(0.0f, 10.0f)); // vertical padding
    if (ImGui::CollapsingHeader("Apollo Adam C Younis", 0)) {
        outputPalette(gameState, tab, global_adamPalettes, arrayCount(global_adamPalettes), size, spacing);
    }

    if (ImGui::CollapsingHeader("Fantasy", 0)) {
        outputPalette(gameState, tab, global_fantasyPalettes, arrayCount(global_fantasyPalettes), size, spacing);
    }

    if (ImGui::CollapsingHeader("Eerie Blue", 0)) {
        outputPalette(gameState, tab, global_eerieBlue, arrayCount(global_eerieBlue), size, spacing);
    }

    if (ImGui::CollapsingHeader("Jehkoba8", 0)) {
        outputPalette(gameState, tab, global_jehkobaPalettes, arrayCount(global_jehkobaPalettes), size, spacing);
    }

    if (ImGui::CollapsingHeader("Lush Sunset", 0)) {
        outputPalette(gameState, tab, global_lushSunsetPalettes, arrayCount(global_lushSunsetPalettes), size, spacing);
    }

    if (ImGui::CollapsingHeader("Midnight ablaze", 0)) {
        outputPalette(gameState, tab, global_mignightBlazePalette, arrayCount(global_mignightBlazePalette), size, spacing);
    }
    if (ImGui::CollapsingHeader("Oil 6", 0)) {
        outputPalette(gameState, tab, global_oilPalettes, arrayCount(global_oilPalettes), size, spacing);
    }
    if (ImGui::CollapsingHeader("Resurrect 64", 0)) {
        outputPalette(gameState, tab, global_resurectPalettes, arrayCount(global_resurectPalettes), size, spacing);
    }
    if (ImGui::CollapsingHeader("SLSO8", 0)) {
        outputPalette(gameState, tab, global_slsoPalettes, arrayCount(global_slsoPalettes), size, spacing);
    }

    if (ImGui::CollapsingHeader("Vinik24", 0)) {
        outputPalette(gameState, tab, global_vinik, arrayCount(global_vinik), size, spacing);
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

void updateAboutWindow(GameState *gameState) {
  if(gameState->showAboutWindow) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 400), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("About Pixelr", &gameState->showAboutWindow);       
    ImGui::Text("Version %s", gameState->versionString); 

    ImGui::End();

  }
}


void updateCanvasSettingsWindow(GameState *gameState) {
  if(gameState->showStokeSmoothingWindow) {
    //NOTE: Create new canvas
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 150), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Canvas Settings", &gameState->showStokeSmoothingWindow);  
    ImGui::SliderInt("Stroke Smoothing Percent", &gameState->runningAverageCount, 1, 30);    
    ImGui::ColorEdit3("Background", (float*)&gameState->bgColor);
    CanvasTab *tab = getActiveCanvasTab(gameState);
    if(tab) {
      ImGui::Checkbox("Check Background", &tab->checkBackground);
    }
    
    ImGui::Checkbox("Draw Per Pixel Grid", &gameState->drawGrid); 
    ImGui::Checkbox("Nearest Filter", &gameState->nearest);
    ImGui::Text("Whether to use a Linear or Nearest Filter when moving or rotating select shapes."); 

    ImGui::End();
  }
}

void loadProjectAndStoreTab(GameState *gameState) {
  bool valid = true;
  CanvasTab tab = loadProjectFromFile(&valid);
  if(valid) {
    tab.uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;
    pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
    gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
  }
}

void showMainMenuBar(GameState *state)
{
  CanvasTab *tab = getActiveCanvasTab(state);
    if (ImGui::BeginMainMenuBar()) // Creates the top bar
    {
        if (ImGui::BeginMenu("Pixelr"))
        {
            if (ImGui::MenuItem("About Pixelr")) { state->showAboutWindow = true; }
            if (ImGui::MenuItem("Quit")) { state->quit = true; }
            
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("File"))
        {
            bool dummy = false;
            if (ImGui::MenuItem("New")) { showNewCanvas(state); }
            if (ImGui::MenuItem("Open Image", "Ctrl+O")) { openPlainImage(state); }
            if (ImGui::MenuItem("Load Sprite Sheet", "")) { setDefaultSpriteSize(state); state->openSpriteSheetWindow = true; }
            if (ImGui::MenuItem("Save Project", "Ctrl+S", &dummy, tab)) { saveProjectToFile(tab); }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) { loadProjectAndStoreTab(state); }
            if (ImGui::MenuItem("Save Pallete", "", &dummy, tab)) {  }
            if (ImGui::MenuItem("Load Pallete", "")) {  }
            if (ImGui::MenuItem("Export Image", "Ctrl+E", &dummy, tab)) { saveFileToPNG(state->renderer, getActiveFrame(state), getActiveCanvasTab(state)); }
            if (ImGui::MenuItem("Export Sprite Sheet", "", &dummy, tab)) { state->showExportWindow = true; }
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
            if (ImGui::MenuItem("Outline Image", "")) { outlineCanvas(state); }
            if (ImGui::MenuItem("Add Color Pallete", "")) { 
              state->colorsPalleteBuffer[0] = '\0';
              state->showColorPalleteEnter = true; 
            }
            
            ImGui::EndMenu();
        }

         if (ImGui::BeginMenu("Canvas"))
        {
            if (ImGui::MenuItem("Settings")) { state->showStokeSmoothingWindow = true; }
            
            ImGui::EndMenu();
        }

        

        ImGui::EndMainMenuBar();
    }
}

void addModeSelection(GameState *state, char *unicodeIcon, CanvasInteractionMode mode) {
  bool pushStyle = false;
  if (state->interactionMode == mode) {
    pushStyle = true;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow color
  }

  if (ImGui::Button(unicodeIcon)) {
      state->interactionMode = mode;
  }

  if (pushStyle) {
      ImGui::PopStyleColor();
  }
}
void Spinner(const char* label, float radius, int thickness, ImU32 color) {
  ImGuiIO& io = ImGui::GetIO();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  ImVec2 pos = ImGui::GetCursorScreenPos();
  float time = ImGui::GetTime();
  int num_segments = 30;
  float start_angle = time * 100.0f;  // Controls rotation speed

  for (int i = 0; i < num_segments; i++) {
      float angle = start_angle + ((float)i / num_segments) * 2.0f * PI32;
      float x = cos(angle) * radius;
      float y = sin(angle) * radius;
      float alpha = (float)i / num_segments;

      ImU32 col = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, alpha));
      
      draw_list->AddLine(ImVec2(pos.x + radius + x, pos.y + radius + y),
                         ImVec2(pos.x + radius + x * 0.6f, pos.y + radius + y * 0.6f),
                         col, thickness);
  }
}

void showUnSavedWindow(GameState *gameState) {
  if(gameState->closeCanvasTabInfo.UIshowUnsavedWindow) {
    //NOTE: Create new canvas
    ImGui::Begin("Unsaved Changes", &gameState->closeCanvasTabInfo.UIshowUnsavedWindow);       

    ImGui::Text("You have unsaved changes. Do you want to save?"); 
    if (ImGui::Button("Yes")) {
      if(!isCanvasTabSaved(gameState->closeCanvasTabInfo.canvasTab)) {
        saveProjectToFile(gameState->closeCanvasTabInfo.canvasTab);
        addIMGUIToast("Project Saved", 2);
      } 
      closeCanvasTabUI(gameState, gameState->closeCanvasTabInfo.canvasTab, gameState->closeCanvasTabInfo.arrayIndex);
      gameState->closeCanvasTabInfo.UIshowUnsavedWindow = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("No")) {
      closeCanvasTabUI(gameState, gameState->closeCanvasTabInfo.canvasTab, gameState->closeCanvasTabInfo.arrayIndex);
      gameState->closeCanvasTabInfo.UIshowUnsavedWindow = false;
    }

    ImGui::End();
  }
  
}

void updateMyImgui(GameState *state, ImGuiIO& io) {

      CanvasInteractionMode startMode = state->interactionMode;

      CanvasTab *tab = getActiveCanvasTab(state);
  
      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      if(state->drawState && state->drawState->openState == EASY_PROFILER_DRAW_CLOSED) {

        bool show_demo_window = true;

        ImVec2 window_pos = ImVec2(0, io.DisplaySize.y);
        ImVec2 window_pivot = ImVec2(0, 1.0f);

        showUnSavedWindow(state);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);
        ImGui::SetNextWindowBgAlpha(1);

        // ImGui::ShowDemoWindow(&show_demo_window);
        drawTabs(state); //NOTE: This has to be at the start because they can remove a canvas tab here
        showMainMenuBar(state);
        updateNewCanvasWindow(state);
        updateSpriteSheetWindow(state);
        if(tab) {
          {
              ImGui::Begin("Color Palette");
              ImGui::ColorEdit3("Brush", (float*)&tab->colorPicked);

              ImU32 col = ImGui::GetColorU32(ImVec4(tab->colorPicked.x, tab->colorPicked.y, tab->colorPicked.z, tab->opacity));
              
              char *strToWrite = easy_createString_printf(&globalPerFrameArena, "0x%x", col);
              ImGui::InputText("##", strToWrite, easyString_getSizeInBytes_utf8(strToWrite) + 1, ImGuiInputTextFlags_ReadOnly);

              // Detect when the color picker is closed after changes
              ImGui::SliderFloat("Opacity", &tab->opacity, 0.0f, 1.0f);
              
              int brushSize = tab->eraserSize;
              ImGui::SliderInt((state->interactionMode != CANVAS_ERASE_MODE) ? "Brush Size" : "Eraser Size", &brushSize, 1, 100);
              tab->eraserSize = brushSize;
              
              ImGui::Checkbox("SELECT", &state->selectMode);

              const char* brushShapes[] = { "Square", "Circle" };
              ImGui::Text("Brush Shape");
              ImGui::SameLine();
              if (ImGui::BeginCombo("##BrushShape", brushShapes[(int)tab->brushShape])) // Label shown on the combo
              {
                  for (int n = 0; n < IM_ARRAYSIZE(brushShapes); n++)
                  {
                      bool isSelected = (tab->brushShape == n);
                      if (ImGui::Selectable(brushShapes[n], isSelected))
                          tab->brushShape = (BrushShapeType)n;

                      // Set default focus when opening
                      if (isSelected)
                          ImGui::SetItemDefaultFocus();
                  }
                  ImGui::EndCombo();
              }

              addModeSelection(state, "\uf0b2", CANVAS_MOVE_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf1fc", CANVAS_DRAW_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf575", CANVAS_FILL_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf111", CANVAS_DRAW_CIRCLE_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf245", CANVAS_MOVE_SELECT_MODE);

              addModeSelection(state, "\uf0c8", CANVAS_DRAW_RECTANGLE_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf12d", CANVAS_ERASE_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf068", CANVAS_DRAW_LINE_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf248", CANVAS_SELECT_RECTANGLE_MODE);
              ImGui::SameLine();
              addModeSelection(state, "\uf1fb", CANVAS_COLOR_DROPPER);
              addModeSelection(state, "\uf5bd", CANVAS_SPRAY_CAN);

              ImGui::Text("Select a Color:");
              ImGui::SameLine();
              if (ImGui::Button("\uf141")) {
                state->showEditPalette = true;
                
              }
              float size = 30.0f;  // Square size
              float spacing = 5.0f; // Spacing between squares

              CanvasTab *tab = getActiveCanvasTab(state);
          
              if(tab) {
                ImVec2 child_size = ImVec2(0, 200); // width 0 = fill, height = 200px
                ImGui::BeginChild("ScrollingRegion", child_size, true, ImGuiWindowFlags_HorizontalScrollbar);

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
              ImGui::EndChild(); // End of scroll area
            }

              

              ImGui::End();
          }

          
          exportWindow(state);
          updateColorPaletteEnter(state);
          updateEditPaletteWindow(state);
          drawAnimationTimeline(state, state->dt);
          drawLayersWindow(state, state->dt);
          updateCanvasSettingsWindow(state);
          updateAboutWindow(state);

          if(startMode != state->interactionMode) {
            clearSelection(tab);
          }
        }
        renderIMGUIToasts();
      }
      
}



void imguiEndFrame() {
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

