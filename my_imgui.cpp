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
}

void imguiEndFrame() {
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool isInteractingWithIMGUI() {
  return (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered());
}