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

    
   drawLinedGrid(gameState, getActiveCanvas(gameState));
   drawCanvas(gameState, getActiveCanvas(gameState), getActiveCanvasTab(gameState));
   

    //NOTE: Update interaction with the canvas
    if(!isInteractingWithIMGUI()) {
        updateCanvasZoom(gameState);
        updateUndoState(gameState);
        
        if(gameState->interactionMode == CANVAS_DRAW_RECTANGLE_MODE || gameState->interactionMode == CANVAS_DRAW_CIRCLE_MODE || gameState->interactionMode == CANVAS_DRAW_LINE_MODE) {
           updateDrawShape(gameState, getActiveCanvas(gameState));
        } else if(gameState->interactionMode == CANVAS_ERASE_MODE) {
           updateEraser(gameState, getActiveCanvas(gameState));
        } else if(gameState->interactionMode == CANVAS_FILL_MODE) {
           updateBucket(gameState, getActiveCanvas(gameState));
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

    if(gameState->keys.keys[KEY_N] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND]) {
        showNewCanvas(gameState);
    }

    if(gameState->keys.keys[KEY_E] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND]) {
        saveFileToPNG(getActiveCanvas(gameState));
    }

    
    
    TimeOfDayValues timeOfDayValues = getTimeOfDayValues(gameState);
    // updateAndDrawDebugCode(gameState);
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation, timeOfDayValues, gameState->perlinTestTexture.handle);
    gameState->renderer->timeAccum += 0.1f*gameState->dt;
}   