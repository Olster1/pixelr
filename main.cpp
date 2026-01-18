#include "includes.h"

void updateGame(GameState *gameState) {
    DEBUG_TIME_BLOCK()
    checkInitGameState(gameState);
    {
        ImGuiIO* io = &ImGui::GetIO();
        updateMyImgui(gameState, *io);
        
    }
    //NOTE This gets cleared out each frame since it is just an immediate mode setting
    gameState->overideDrawModeState = CANVAS_INTERACTION_MODE_NONE;

    float fauxWidth = FAUX_WIDTH;
    float16 screenGuiT = make_ortho_matrix_bottom_left_corner(fauxWidth, fauxWidth*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    // gameState->renderer->textMatrixResolution = make_float2(fauxWidth, fauxWidth*gameState->aspectRatio_y_over_x);

    float16 screenT = make_ortho_matrix_origin_center(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 textGuiT = make_ortho_matrix_origin_center(fauxWidth, fauxWidth*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);//make_ortho_matrix_top_left_corner_y_down(fauxWidth, fauxWidth*gameState->aspectRatio_y_over_x, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 cameraT = transform_getInverseX(gameState->camera.T);
    float16 cameraTWithoutTranslation = getCameraX_withoutTranslation(gameState->camera.T);

    float16 rot = eulerAnglesToTransform(gameState->camera.T.rotation.y, gameState->camera.T.rotation.x, gameState->camera.T.rotation.z);
    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    checkFileDrop(gameState);

   CanvasTab *t = getActiveCanvasTab(gameState);
   if(t) {
        checkAndSaveBackupFile(gameState);
        drawCanvasGridBackground(gameState, getActiveCanvas(gameState), getActiveCanvasTab(gameState));
        sanityCheckCanvasSize(t);
        Frame *f = getActiveFrame(gameState);
        Canvas *c = f->layers + f->activeLayer;
        
        drawLinedGrid(gameState, getActiveCanvas(gameState));
        {
        
            if(t && getArrayLength(t->frames) > 0) {
                
                for(int i = t->onionSkinningFrames; i >= 0; --i) {
                    int frameIndex = t->activeFrame - i;
                    if(frameIndex < 0) {
                        frameIndex = getArrayLength(t->frames) + frameIndex;
                    }
                    
                    assert(frameIndex >= 0 && frameIndex < getArrayLength(t->frames));

                    Frame *f = t->frames + frameIndex;
                    assert(f);

                    float onionSkinOpacity = 1.0f / MathMax(i + 1, 1);
                    drawCanvas(gameState, f, t, onionSkinOpacity);
                }
            }
        }

        updateUndoState(gameState);

        updateCanvasZoom(gameState, isInteractingWithIMGUI(), 0.8f*gameState->scrollSpeed*(gameState->inverseZoom ? -1 : 1));
        //NOTE: Update interaction with the canvas
        if(!isInteractingWithIMGUI()) {
            if(gameState->keys.keys[KEY_SPACE] == MOUSE_BUTTON_DOWN) {
                //NOTE: First check if it's a SPACE shortcut interaction - like space/drag
                if(gameState->mouseBtn[MOUSE_BUTTON_RIGHT_CLICK] != MOUSE_BUTTON_NONE) {
                    float2 diff = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP);
                    float speedFactor = 0.3f;
                    updateCanvasZoom(gameState, isInteractingWithIMGUI(), speedFactor*diff.y*(gameState->inverseZoom ? -1 : 1));
                } else {
                    updateCanvasMove(gameState);
                }
            } else if(gameState->keys.keys[gameState->hotkeyActionKeys[HOTKEY_BRUSH_SIZE]] == MOUSE_BUTTON_DOWN) {
                if(!(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_DOWN || gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED))
                 {
                    float2 diff = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP);
                    float speedFactor = 1;
                    t->eraserSize += speedFactor*diff.x;
                    t->eraserSize = clamp(1, 100, t->eraserSize);
                    updateCanvasDraw(gameState, getActiveCanvas(gameState));
                    drawPaintCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                }
            } else if(gameState->keys.keys[gameState->hotkeyActionKeys[HOTKEY_OPACITY]] == MOUSE_BUTTON_DOWN) {
                if(!(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_DOWN || gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED))
                 {
                    float2 diff = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP);
                    float opacity = t->opacity*100.0f;
                    opacity += diff.x;
                    opacity = clamp(0.0f, 100.0f, opacity);
                    t->opacity = 0.01f*opacity;
                    updateCanvasDraw(gameState, getActiveCanvas(gameState));
                    drawPaintCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } 
            } else if(isKeyPressedOrDown(gameState, gameState->hotkeyActionKeys[HOTKEY_COLOR_DROPPER])) {
                updateColorDropper(gameState, getActiveCanvas(gameState));
                hideOrShowArrowIfOnCanvas(gameState, t);
                gameState->overideDrawModeState = CANVAS_COLOR_DROPPER;
            } else if(isKeyPressedOrDown(gameState, gameState->hotkeyActionKeys[HOTKEY_ERASER]) && !isKeyPressedOrDown(gameState, KEY_COMMAND)) {
                updateEraser(gameState, getActiveCanvas(gameState));
                drawPaintCursor(gameState);
                hideOrShowArrowIfOnCanvas(gameState, t);
                gameState->overideDrawModeState = CANVAS_ERASE_MODE;
            } else {
                if(gameState->interactionMode == CANVAS_DRAW_RECTANGLE_MODE || gameState->interactionMode == CANVAS_DRAW_CIRCLE_MODE || gameState->interactionMode == CANVAS_DRAW_LINE_MODE) {
                    updateDrawShape(gameState, getActiveCanvas(gameState));
                    drawSinglePixelCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_ERASE_MODE) {
                    updateEraser(gameState, getActiveCanvas(gameState));
                    drawPaintCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_FILL_MODE) {
                    updateBucket(gameState, getActiveCanvas(gameState), gameState->selectMode);
                    drawSinglePixelCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_DRAW_MODE) {
                    updateCanvasDraw(gameState, getActiveCanvas(gameState));
                    drawPaintCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_MOVE_MODE) {
                    updateCanvasMove(gameState);
                    drawCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_SELECT_RECTANGLE_MODE) {
                    updateCanvasSelect(gameState, getActiveCanvasTab(gameState));
                    drawCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_SPRAY_CAN) {
                    updateSprayCan(gameState, getActiveCanvas(gameState));
                    drawCursor(gameState);
                    hideOrShowArrowIfOnCanvas(gameState, t);
                } else if(gameState->interactionMode == CANVAS_COLOR_DROPPER || gameState->interactionMode == CANVAS_COLOR_DROPPER_REPLACE_DEST || gameState->interactionMode == CANVAS_COLOR_DROPPER_REPLACE_SRC) {
                    updateColorDropper(gameState, getActiveCanvas(gameState));
                    hideOrShowArrowIfOnCanvas(gameState, t);
                }
            }
        }

        if(gameState->canvasMirrorFlags != 0) 
        {
            drawGuidlines(gameState, getActiveCanvas(gameState));
        }

        updateGpuCanvasTextures(gameState);

        updateSelectObject(gameState, getActiveCanvas(gameState));

        if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_NONE || gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_RELEASED || gameState->keys.keys[KEY_ESCAPE] == MOUSE_BUTTON_PRESSED) {
            gameState->paintActive = false;
            gameState->drawingShape = false;
            gameState->draggingCanvas = false;
            gameState->grabbedCornerIndex = -1;

            // if(gameState->interactionMode == CANVAS_COLOR_DROPPER_REPLACE_DEST || gameState->interactionMode == CANVAS_COLOR_DROPPER_REPLACE_SRC) {
            //     gameState->interactionMode = CANVAS_DRAW_MODE;
            // }

            getActiveCanvasTab(gameState)->currentUndoBlock = 0;
        }   
        
    }
    //NOTE: Invalidate the this array brushOutlineStencil because it was pushed on the perFrameArena
    gameState->brushOutlineStencil = 0;
    
    updateHotKeys(gameState);

    #if EASY_PROFILER_ON
    EasyProfile_DrawGraph(gameState->renderer, gameState, gameState->drawState, gameState->dt, gameState->aspectRatio_y_over_x, gameState->mouseP_01);
    #endif
    
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation);
    gameState->renderer->timeAccum += 0.1f*gameState->dt;
}   