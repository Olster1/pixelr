#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <cstdio>
#include <map>
#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#include "./murmur3.c"
#include "./3DMaths.h"
#include <cstdio>
#include "./threads.cpp"

#define ENUM(value) value,
#define STRING(value) #value,

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef intptr_t intprt;

#if !__cplusplus
typedef u32 bool;
#define true 1
#define false 0
#endif 

#define internal static 


enum MouseKeyState {
  MOUSE_BUTTON_NONE,
  MOUSE_BUTTON_PRESSED,
  MOUSE_BUTTON_DOWN,
  MOUSE_BUTTON_RELEASED,
};

enum KeyTypes {
  KEY_UP,
  KEY_DOWN,
  KEY_E,
  KEY_Q,
  KEY_X,
  KEY_Z,
  KEY_N,
  KEY_S,
  KEY_C,
  KEY_V,
  KEY_A,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_SPACE,
  KEY_SHIFT,
  KEY_ESCAPE,
  KEY_DELETE,
  KEY_BACKSPACE,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_COMMAND,

  ///////////
  KEY_COUNTS
};

struct KeyStates {
  MouseKeyState keys[KEY_COUNTS];
};


#include "./main.cpp"

float getBestDt(float secondsElapsed) {
      float frameRates[] = {1.0f/15.0f, 1.0f/20.0f, 1.0f/30.0f, 1.0f/60.0f, 1.0f/120.0f};
        //NOTE: Clmap the dt so werid bugs happen if huge lag like startup
      float closestFrameRate = 0;
      float minDiff = FLT_MAX;
      for(int i = 0; i < arrayCount(frameRates); i++) {
        float dt_ = frameRates[i];
        float diff_ = get_abs_value(dt_ - secondsElapsed);

        if(diff_ < minDiff) {
          minDiff = diff_;
          closestFrameRate = dt_;
        }
      }
      // printf("frames per second: %d\n", (int)round(1.0f / closestFrameRate));              
      return closestFrameRate;
}

void updateKeyState(GameState *gameState, KeyTypes type, bool value) {
  MouseKeyState result = MOUSE_BUTTON_NONE;
  MouseKeyState state = gameState->keys.keys[type];

  if(value) {
    if(state == MOUSE_BUTTON_NONE) {
      gameState->keys.keys[type] = MOUSE_BUTTON_PRESSED;
    } else if(state == MOUSE_BUTTON_PRESSED) {
      gameState->keys.keys[type] = MOUSE_BUTTON_DOWN;
    }
  } else if(gameState->keys.keys[type] == MOUSE_BUTTON_DOWN || gameState->keys.keys[type] == MOUSE_BUTTON_PRESSED) {
    gameState->keys.keys[type] = MOUSE_BUTTON_RELEASED;
  }
}

int main(int argc, char **argv) {
  int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
  if (SDL_Init(SDL_INIT_EVERYTHING)) {
    return 0;
  }

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                      SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

  GameState *gameState = (GameState *)malloc(sizeof(GameState));
  memset(gameState, 0, sizeof(GameState));
  gameState->screenWidth = 0.5f*1920;
  gameState->aspectRatio_y_over_x = (1080.f / 1920.0f);
  gameState->mouseLeftBtn = MOUSE_BUTTON_NONE;
  gameState->quit = false;

  for(int i = 0; i < arrayCount(gameState->keys.keys); ++i) {
      gameState->keys.keys[i] = MOUSE_BUTTON_NONE;
    } 

  globalSoundState = (EasySound_SoundState *)malloc(sizeof(EasySound_SoundState));

  initAudioSpec(&gameState->audioSpec, 44100);
  initAudio(&gameState->audioSpec);

  SDL_Window *window = SDL_CreateWindow("Pixelr",  SDL_WINDOWPOS_CENTERED,  SDL_WINDOWPOS_CENTERED, gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x, flags);

  SDL_GLContext renderContext = SDL_GL_CreateContext(window);

  if(renderContext) {
      if(SDL_GL_MakeCurrent(window, renderContext) == 0) {
          
          if(SDL_GL_SetSwapInterval(1) == 0) {
              // Success
          } else {
              printf("Couldn't set swap interval\n");
          }
      } else {
          printf("Couldn't make context current\n");
      }
  }

  ImGuiIO& imguiIo = initMyImGui(renderContext, window);

  initBackendRenderer();

  SDL_EventState(SDL_MULTIGESTURE, SDL_ENABLE);

  //NOTE: Hide the cursor
  // SDL_ShowCursor(SDL_DISABLE);

  SDL_Event e;
  Uint32 start = SDL_GetTicks();
  while (!gameState->quit) {
    
    Uint32 end = SDL_GetTicks();
    float secondsElapsed = (end - start) / 1000.0f;
    start = end;
    gameState->dt = getBestDt(secondsElapsed);

    //NOTE: Clear the mouse states
    for(int i = 0; i < arrayCount(gameState->keys.keys); ++i) {
      if(gameState->keys.keys[i] == MOUSE_BUTTON_RELEASED) {
        gameState->keys.keys[i] = MOUSE_BUTTON_NONE;
      }
    } 
    gameState->keys.keys[KEY_X] = MOUSE_BUTTON_NONE;
    gameState->keys.keys[KEY_Z] = MOUSE_BUTTON_NONE;

    gameState->scrollSpeed = 0;

    while (SDL_PollEvent(&e)) {
       if (e.type == SDL_QUIT) {
          gameState->quit = true;
        } else if (e.type == SDL_MOUSEWHEEL) {
          gameState->scrollSpeed = e.wheel.y;
        } else if(e.type == SDL_KEYDOWN) {
          SDL_Scancode scancode = e.key.keysym.scancode; 
          if(scancode == SDL_SCANCODE_X) {
            
            gameState->keys.keys[KEY_X] = MOUSE_BUTTON_DOWN;
          } else if(scancode == SDL_SCANCODE_Z) {
            
            gameState->keys.keys[KEY_Z] = MOUSE_BUTTON_DOWN;
          }
        } else if(e.type == SDL_KEYUP) {
          SDL_Scancode scancode = e.key.keysym.scancode; 
          if(scancode == SDL_SCANCODE_X) {
            gameState->keys.keys[KEY_X] = MOUSE_BUTTON_RELEASED;
          } else if(scancode == SDL_SCANCODE_Z) {
            gameState->keys.keys[KEY_Z] = MOUSE_BUTTON_RELEASED;
          }
        }
      ImGui_ImplSDL2_ProcessEvent(&e);
    }

    updateMyImgui(gameState, imguiIo);

    const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
    
    updateKeyState(gameState, KEY_UP, currentKeyStates[SDL_SCANCODE_UP] == 1 || currentKeyStates[SDL_SCANCODE_W] == 1);
    updateKeyState(gameState, KEY_E, currentKeyStates[SDL_SCANCODE_E] == 1);
    updateKeyState(gameState, KEY_N, currentKeyStates[SDL_SCANCODE_N] == 1);
    updateKeyState(gameState, KEY_S, currentKeyStates[SDL_SCANCODE_S] == 1);
    updateKeyState(gameState, KEY_Q, currentKeyStates[SDL_SCANCODE_Q] == 1);
    updateKeyState(gameState, KEY_C, currentKeyStates[SDL_SCANCODE_C] == 1);
    updateKeyState(gameState, KEY_V, currentKeyStates[SDL_SCANCODE_V] == 1);
    updateKeyState(gameState, KEY_A, currentKeyStates[SDL_SCANCODE_A] == 1);
    updateKeyState(gameState, KEY_COMMAND, currentKeyStates[SDL_SCANCODE_LGUI] == 1);
    updateKeyState(gameState, KEY_DOWN, currentKeyStates[SDL_SCANCODE_DOWN] == 1 || currentKeyStates[SDL_SCANCODE_S] == 1);
    updateKeyState(gameState, KEY_LEFT, currentKeyStates[SDL_SCANCODE_LEFT] == 1 || currentKeyStates[SDL_SCANCODE_A] == 1);
    updateKeyState(gameState, KEY_RIGHT, currentKeyStates[SDL_SCANCODE_RIGHT] == 1 || currentKeyStates[SDL_SCANCODE_D] == 1);
    updateKeyState(gameState, KEY_SPACE, currentKeyStates[SDL_SCANCODE_SPACE] == 1);
    updateKeyState(gameState, KEY_SHIFT, currentKeyStates[SDL_SCANCODE_LSHIFT] == 1);
    updateKeyState(gameState, KEY_ESCAPE, currentKeyStates[SDL_SCANCODE_ESCAPE] == 1);
    updateKeyState(gameState, KEY_DELETE, currentKeyStates[SDL_SCANCODE_DELETE] == 1);
    updateKeyState(gameState, KEY_BACKSPACE, currentKeyStates[SDL_SCANCODE_BACKSPACE] == 1);
    updateKeyState(gameState, KEY_1, currentKeyStates[SDL_SCANCODE_1] == 1);
    updateKeyState(gameState, KEY_2, currentKeyStates[SDL_SCANCODE_2] == 1);
    updateKeyState(gameState, KEY_3, currentKeyStates[SDL_SCANCODE_3] == 1);
    updateKeyState(gameState, KEY_4, currentKeyStates[SDL_SCANCODE_4] == 1);
    updateKeyState(gameState, KEY_5, currentKeyStates[SDL_SCANCODE_5] == 1);
    updateKeyState(gameState, KEY_6, currentKeyStates[SDL_SCANCODE_6] == 1);
    updateKeyState(gameState, KEY_7, currentKeyStates[SDL_SCANCODE_7] == 1);
    updateKeyState(gameState, KEY_8, currentKeyStates[SDL_SCANCODE_8] == 1);
    int w; 
    int h;
    SDL_GetWindowSize(window, &w, &h);
    gameState->screenWidth = (float)w;
    gameState->aspectRatio_y_over_x = (float)h / (float)w;
    // printf("w: %d\n", w);
    // printf("ap: %d\n", h);

    int x; 
    int y;
    Uint32 mouseState = SDL_GetMouseState(&x, &y);
    gameState->mouseP_screenSpace.x = (float)x;
    gameState->mouseP_screenSpace.y = (float)(-y); //NOTE: Bottom corner is origin 

    gameState->mouseP_01.x = gameState->mouseP_screenSpace.x / w;
    gameState->mouseP_01.y = (gameState->mouseP_screenSpace.y / h) + 1.0f;

  if(mouseState && SDL_BUTTON(1)) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_NONE) {
      gameState->mouseLeftBtn = MOUSE_BUTTON_PRESSED;
    } else if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
      gameState->mouseLeftBtn = MOUSE_BUTTON_DOWN;
    }
  } else {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
      gameState->mouseLeftBtn = MOUSE_BUTTON_RELEASED;
    } else {
      gameState->mouseLeftBtn = MOUSE_BUTTON_NONE;
    }
  }

    // Clear screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(gameState->bgColor.x, gameState->bgColor.y, gameState->bgColor.z, gameState->bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

    updateGame(gameState);
    imguiEndFrame();
    SDL_GL_SwapWindow(window);

    //NOTE: For 3d to move camera aroumd
    gameState->lastMouseP = make_float2(0.5f*gameState->screenWidth, -0.5f*gameState->screenWidth);
    if(gameState->useCameraMovement) {
      SDL_WarpMouseInWindow(window, 0.5f*gameState->screenWidth, 0.5f*gameState->screenWidth);
    } else {
      gameState->lastMouseP = gameState->mouseP_screenSpace;
    }          
  }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(renderContext);
    SDL_DestroyWindow(window);
    SDL_Quit();


  return 0;
}