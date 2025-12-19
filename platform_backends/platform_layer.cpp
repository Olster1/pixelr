#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <cstdio>
#include <map>
#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#include "../libs/murmur3.c"
#include "../3DMaths.h"
#include <cstdio>
#include "../threads.cpp"
#include <ctime>
static ThreadsInfo *globalThreadInfo = 0;

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
  KEY_O,
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
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,
  KEY_COMMAND,

  ///////////
  KEY_COUNTS
};

struct KeyStates {
  MouseKeyState keys[KEY_COUNTS];
};

void platform_copyToClipboard(char *str) {
  if (SDL_SetClipboardText(str) == 0) {
    printf("Copied to clipboard!\n");
  } else {
      printf("Clipboard error: %s\n", SDL_GetError());
  }
}

static u64 GlobalTimeFrequencyDatum;

inline u64 EasyTime_GetTimeCount()
{
    u64 s = SDL_GetPerformanceCounter();
    
    return s;
}
inline float EasyTime_GetMillisecondsElapsed(u64 CurrentCount, u64 LastCount)
{
    u64 Difference = CurrentCount - LastCount;
    assert(Difference >= 0); //user put them in the right order
    double Seconds = (float)Difference / (float)GlobalTimeFrequencyDatum;
	float millseconds = (float)(Seconds * 1000.0);     
    return millseconds;
    
}
#include "../main.cpp"

float getBestDt(float secondsElapsed) {
      float frameRates[] = {1.0f/15.0f, 1.0f/20.0f, 1.0f/30.0f, 1.0f/60.0f, 1.0f/120.0f, 1.0f/240.0f};
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

void processEvent(GameState *gameState, SDL_Event *e) {
  if (e->type == SDL_QUIT) {
      gameState->shouldQuit = true;
    } else if (e->type == SDL_DROPFILE) {
      if(gameState->droppedFileCount < arrayCount(gameState->droppedFilePaths)) {
        gameState->droppedFilePaths[gameState->droppedFileCount++] = e->drop.file;
      }
    } else if (e->type == SDL_MOUSEWHEEL) {
      gameState->scrollSpeed = e->wheel.y;
    } else if(e->type == SDL_KEYDOWN) {
      //NOTE: We do this here just for z becuase we want the lag between when the user presses the first z for undo redo
      SDL_Scancode scancode = e->key.keysym.scancode; 
      if(scancode == SDL_SCANCODE_Z) {
        gameState->keys.keys[KEY_Z] = MOUSE_BUTTON_DOWN;
      }
    } else if(e->type == SDL_KEYUP) {
      //NOTE: We do this here just for z becuase we want the lag between when the user presses the first z for undo redo
      SDL_Scancode scancode = e->key.keysym.scancode; 
      if(scancode == SDL_SCANCODE_Z) {
        gameState->keys.keys[KEY_Z] = MOUSE_BUTTON_RELEASED;
      }
    } 
    
    ImGui_ImplSDL2_ProcessEvent(e);
}

int main(int argc, char **argv) {
  int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
  if (SDL_Init(SDL_INIT_EVERYTHING)) {
    return 0;
  }
  GlobalTimeFrequencyDatum = SDL_GetPerformanceFrequency();
  DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(beginFrameProfiler, "Main: Intial setup");\

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
  gameState->shouldQuit = false;
  gameState->exePath = SDL_GetBasePath();
  gameState->quit = false;

  for(int i = 0; i < arrayCount(gameState->keys.keys); ++i) {
      gameState->keys.keys[i] = MOUSE_BUTTON_NONE;
    } 

  SDL_Window *window = SDL_CreateWindow(DEFINED_APP_NAME,  SDL_WINDOWPOS_CENTERED,  SDL_WINDOWPOS_CENTERED, gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x, flags);

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


  if (!gladLoadGL()) {
    assert(false);
  }


  ImGuiIO& imguiIo = initMyImGui(renderContext, window, gameState->exePath);
  
  initBackendRenderer();

  SDL_EventState(SDL_MULTIGESTURE, SDL_ENABLE);

  gameState->appDataFolderName = getPlatformSaveFilePath();

  SDL_Event e;
  Uint32 start = SDL_GetTicks();
  int framesAfterEventCount = 0;
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
    gameState->keys.keys[KEY_Z] = MOUSE_BUTTON_NONE;

    gameState->scrollSpeed = 0;

    if(shouldKeepUpdateing(gameState) || framesAfterEventCount < MAX_FRAMES_AFTER_EVENT_TO_RUN) {
      // printf("UPDATEING %f\n", gameState->dt);
      while(SDL_PollEvent(&e)) {
        processEvent(gameState, &e);
        framesAfterEventCount = 0;
      }
    } else if (SDL_WaitEvent(&e)) {
        // printf("WAITING %f\n", gameState->dt);
        do {
          processEvent(gameState, &e);
          framesAfterEventCount = 0;
        } while(SDL_PollEvent(&e));
    }

    //NOTE: This is to get the last interaction mode
    gameState->interactionModeStartOfFrame = gameState->interactionMode;
    
    updateMyImgui(gameState, imguiIo);
    
    const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
    
    updateKeyState(gameState, KEY_UP, currentKeyStates[SDL_SCANCODE_UP] == 1);
    updateKeyState(gameState, KEY_E, currentKeyStates[SDL_SCANCODE_E] == 1);
    updateKeyState(gameState, KEY_O, currentKeyStates[SDL_SCANCODE_O] == 1);
    updateKeyState(gameState, KEY_N, currentKeyStates[SDL_SCANCODE_N] == 1);
    updateKeyState(gameState, KEY_S, currentKeyStates[SDL_SCANCODE_S] == 1);
    updateKeyState(gameState, KEY_Q, currentKeyStates[SDL_SCANCODE_Q] == 1);
    updateKeyState(gameState, KEY_C, currentKeyStates[SDL_SCANCODE_C] == 1);
    updateKeyState(gameState, KEY_V, currentKeyStates[SDL_SCANCODE_V] == 1);
    updateKeyState(gameState, KEY_X, currentKeyStates[SDL_SCANCODE_X] == 1);
    updateKeyState(gameState, KEY_A, currentKeyStates[SDL_SCANCODE_A] == 1);
#if defined(__APPLE__)
//NOTE: Command for mac
    updateKeyState(gameState, KEY_COMMAND, currentKeyStates[SDL_SCANCODE_LGUI] == 1);
#else
  updateKeyState(gameState, KEY_COMMAND, currentKeyStates[SDL_SCANCODE_LCTRL] == 1);
#endif
    
    updateKeyState(gameState, KEY_DOWN, currentKeyStates[SDL_SCANCODE_DOWN] == 1);
    updateKeyState(gameState, KEY_LEFT, currentKeyStates[SDL_SCANCODE_LEFT] == 1);
    updateKeyState(gameState, KEY_RIGHT, currentKeyStates[SDL_SCANCODE_RIGHT] == 1);
    updateKeyState(gameState, KEY_SPACE, currentKeyStates[SDL_SCANCODE_SPACE] == 1);
    updateKeyState(gameState, KEY_SHIFT, currentKeyStates[SDL_SCANCODE_LSHIFT] == 1);
    updateKeyState(gameState, KEY_ESCAPE, currentKeyStates[SDL_SCANCODE_ESCAPE] == 1);
    updateKeyState(gameState, KEY_DELETE, currentKeyStates[SDL_SCANCODE_DELETE] == 1);
    updateKeyState(gameState, KEY_BACKSPACE, currentKeyStates[SDL_SCANCODE_BACKSPACE] == 1);
    updateKeyState(gameState, KEY_ENTER, currentKeyStates[SDL_SCANCODE_RETURN] == 1);
    updateKeyState(gameState, KEY_1, currentKeyStates[SDL_SCANCODE_1] == 1);
    updateKeyState(gameState, KEY_2, currentKeyStates[SDL_SCANCODE_2] == 1);
    updateKeyState(gameState, KEY_3, currentKeyStates[SDL_SCANCODE_3] == 1);
    updateKeyState(gameState, KEY_4, currentKeyStates[SDL_SCANCODE_4] == 1);
    updateKeyState(gameState, KEY_5, currentKeyStates[SDL_SCANCODE_5] == 1);
    updateKeyState(gameState, KEY_6, currentKeyStates[SDL_SCANCODE_6] == 1);
    updateKeyState(gameState, KEY_7, currentKeyStates[SDL_SCANCODE_7] == 1);
    updateKeyState(gameState, KEY_8, currentKeyStates[SDL_SCANCODE_8] == 1);
    updateKeyState(gameState, KEY_9, currentKeyStates[SDL_SCANCODE_9] == 1);
    updateKeyState(gameState, KEY_0, currentKeyStates[SDL_SCANCODE_0] == 1);
    int w; 
    int h;
    SDL_GetWindowSize(window, &w, &h);
    gameState->screenWidth = (float)w;
    gameState->aspectRatio_y_over_x = (float)h / (float)w;
    // printf("w: %d\n", w);
    // printf("ap: %d\n", h);
    glViewport(0, 0, w, h);

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

  if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
    
    gameState->mouseCountAt++;
    if(gameState->mouseCountAt >= arrayCount(gameState->mouseP_01_array)) {
      gameState->mouseCountAt = arrayCount(gameState->mouseP_01_array);
    }

    int indexToAddTo = gameState->mouseIndexAt++;
    if(gameState->mouseIndexAt >= arrayCount(gameState->mouseP_01_array)) {
      gameState->mouseIndexAt = 0;
    }
    assert(indexToAddTo >= 0 && indexToAddTo < arrayCount(gameState->mouseP_01_array));
    
    gameState->mouseP_01_array[indexToAddTo] = gameState->mouseP_01;
  } else {
    gameState->mouseCountAt = 0;
    gameState->mouseIndexAt = 0;
  }

    // Clear screen
    backendRenderer_clearDefaultFrameBuffer(gameState->bgColor);
    
    updateGame(gameState);

    {
      
      DEBUG_TIME_BLOCK_NAMED("imguiEndFrame")
      imguiEndFrame();
      
    }
    
    {
      DEBUG_TIME_BLOCK_NAMED("SWAP WINDOW")
      SDL_GL_SwapWindow(window);
    }

    if(gameState->interactionModeStartOfFrame != gameState->interactionMode) {
        gameState->lastInteractionMode = gameState->interactionModeStartOfFrame;
    }

    gameState->lastMouseP = make_float2(0.5f*gameState->screenWidth, -0.5f*gameState->screenWidth);
    gameState->lastMouseP = gameState->mouseP_screenSpace;

    //NOTE: Free the dropped filepaths if there were ones
    if(gameState->droppedFileCount > 0) {
      for(int i = 0; i < gameState->droppedFileCount; ++i) {
        SDL_free(gameState->droppedFilePaths[i]);  
      }
      
      gameState->droppedFileCount = 0;
    } 

    DEBUG_TIME_BLOCK_FOR_FRAME_END(beginFrameProfiler, gameState->keys.keys[KEY_SPACE] == MOUSE_BUTTON_PRESSED);
    DEBUG_TIME_BLOCK_FOR_FRAME_START(beginFrameProfiler, "Per frame");

    if(framesAfterEventCount < MAX_FRAMES_AFTER_EVENT_TO_RUN) {
      framesAfterEventCount++;
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