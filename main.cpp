#include "includes.h"


void updateGame(GameState *gameState) {
    checkInitGameState(gameState);

    updateCamera(gameState);
  
    float16 screenGuiT = make_ortho_matrix_origin_center(100, 100*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 textGuiT = make_ortho_matrix_top_left_corner_y_down(100, 100*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);

    float16 screenT = make_ortho_matrix_origin_center(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 cameraT = transform_getInverseX(gameState->camera.T);
    float16 cameraTWithoutTranslation = getCameraX_withoutTranslation(gameState->camera.T);

    float16 rot = eulerAnglesToTransform(gameState->camera.T.rotation.y, gameState->camera.T.rotation.x, gameState->camera.T.rotation.z);
    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    drawCanvasGridBackground(gameState, getActiveCanvas(gameState), getActiveCanvasTab(gameState));
   drawLinedGrid(gameState, getActiveCanvas(gameState));
   
   {
        CanvasTab *t = getActiveCanvasTab(gameState);
        if(t && getArrayLength(t->frames) > 0) {
            
            for(int i = gameState->onionSkinningFrames; i >= 0; --i) {
                int frameIndex = t->activeFrame - i;
                if(frameIndex < 0) {
                    frameIndex = getArrayLength(t->frames) + frameIndex;
                }
                
                assert(frameIndex >= 0 && frameIndex < getArrayLength(t->frames));

                Frame *f = t->frames + frameIndex;
                assert(f);

                Canvas *canvas = f->layers + f->activeLayer;
                assert(canvas);

                float onionSkinOpacity = 1.0f / MathMax(i + 1, 1);
                drawCanvas(gameState, canvas, t, onionSkinOpacity);
            }
        }
    }
   
   

    //NOTE: Update interaction with the canvas
    if(!isInteractingWithIMGUI()) {
        updateCanvasZoom(gameState);
        updateUndoState(gameState);
        
        if(gameState->interactionMode == CANVAS_DRAW_RECTANGLE_MODE || gameState->interactionMode == CANVAS_DRAW_CIRCLE_MODE || gameState->interactionMode == CANVAS_DRAW_LINE_MODE) {
           updateDrawShape(gameState, getActiveCanvas(gameState));
        } else if(gameState->interactionMode == CANVAS_ERASE_MODE) {
           updateEraser(gameState, getActiveCanvas(gameState));
        } else if(gameState->interactionMode == CANVAS_FILL_MODE) {
           updateBucket(gameState, getActiveCanvas(gameState), gameState->selectMode);
        } else if(gameState->interactionMode == CANVAS_DRAW_MODE) {
           updateCanvasDraw(gameState, getActiveCanvas(gameState));
        } else if(gameState->interactionMode == CANVAS_MOVE_MODE) {
            updateCanvasMove(gameState);
        } else if(gameState->interactionMode == CANVAS_SELECT_RECTANGLE_MODE) {
            updateCanvasSelect(gameState, getActiveCanvasTab(gameState));
        }

        updateGpuCanvasTextures(gameState);
    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_NONE || gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED || gameState->keys.keys[KEY_ESCAPE] == MOUSE_BUTTON_PRESSED) {
        gameState->paintActive = false;
        gameState->drawingShape = false;
    }


    //NOTE: Update interaction mode
    if(gameState->keys.keys[KEY_1] == MOUSE_BUTTON_PRESSED) {
        gameState->interactionMode = CANVAS_DRAW_MODE;
    } else if(gameState->keys.keys[KEY_2] == MOUSE_BUTTON_PRESSED) {
        gameState->interactionMode = CANVAS_MOVE_MODE;
    }

    if(gameState->keys.keys[KEY_N] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
        showNewCanvas(gameState);
    }

    if(gameState->keys.keys[KEY_E] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
        saveFileToPNG(getActiveCanvas(gameState));
    }

    if(gameState->keys.keys[KEY_ESCAPE] == MOUSE_BUTTON_PRESSED) {
        CanvasTab *t = getActiveCanvasTab(gameState);
        if(t) {
            if(getArrayLength(t->selected) > 0) {
                clearSelection(t);
            }
        }
    }

    if(gameState->keys.keys[KEY_C] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
        CanvasTab *t = getActiveCanvasTab(gameState);
        if(t) {
            for(int i = 0; i < getArrayLength(t->selected); i++) {
                float2 p = t->selected[i];
                Canvas *c = getActiveCanvas(gameState);
                if(c) {
                    u32 color = getCanvasColor(c, (int)p.x, (int)p.y);
                    gameState->clipboard.addPixelInfo((int)p.x, (int)p.y, color);
                }
            }
        }
    }

    if(gameState->keys.keys[KEY_V] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
        Canvas *c = getActiveCanvas(gameState);
        if(c && gameState->clipboard.hasCopy()) {
            for(int i = 0; i < getArrayLength(gameState->clipboard.pixels); i++) {
                PixelClipboardInfo info = gameState->clipboard.pixels[i];

                setCanvasColor(c, info.x, info.y, info.color, gameState->opacity, false);
            }
        }
    }

    if(gameState->keys.keys[KEY_DELETE] == MOUSE_BUTTON_PRESSED || gameState->keys.keys[KEY_BACKSPACE] == MOUSE_BUTTON_PRESSED) {
        Canvas *c = getActiveCanvas(gameState);
        CanvasTab *t = getActiveCanvasTab(gameState);
        if(t && c) {
            for(int i = 0; i < getArrayLength(t->selected); i++) {
                float2 p = t->selected[i];
                setCanvasColor(c, p.x, p.y, 0x00FFFFFF, gameState->opacity, false);
            }
        }
    }

    if(gameState->keys.keys[KEY_A] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
        CanvasTab *t = getActiveCanvasTab(gameState);
        if(t) {
            Canvas *canvas = getActiveCanvas(gameState);
            if(canvas) {
                clearSelection(t);
                

                for(int y = 0; y <= t->h; ++y) {
                    for(int x = 0; x <= t->w; ++x) {
                        float2 f = make_float2(x, y);
                        pushArrayItem(&t->selected, f, float2);
                    }
                }
            }
        }
    }

    
    
    TimeOfDayValues timeOfDayValues = getTimeOfDayValues(gameState);
    // updateAndDrawDebugCode(gameState);
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation, timeOfDayValues, gameState->perlinTestTexture.handle);
    gameState->renderer->timeAccum += 0.1f*gameState->dt;
}   