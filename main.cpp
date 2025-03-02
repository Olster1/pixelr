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

    
   drawLinedGrid(gameState);
   drawCanvas(gameState);
   updateCanvasZoom(gameState);

    //NOTE: Update interaction with the canvas
    if(!isInteractingWithIMGUI()) {
        updateUndoState(gameState);
        
        if(gameState->interactionMode == CANVAS_DRAW_RECTANGLE_MODE || gameState->interactionMode == CANVAS_DRAW_CIRCLE_MODE || gameState->interactionMode == CANVAS_DRAW_LINE_MODE) {
           updateDrawShape(gameState);
        } else if(gameState->interactionMode == CANVAS_ERASE_MODE) {
           updateEraser(gameState);
        } else if(gameState->interactionMode == CANVAS_FILL_MODE) {
           updateBucket(gameState);
        } else if(gameState->interactionMode == CANVAS_DRAW_MODE) {
           updateCanvasDraw(gameState);
        } else if(gameState->interactionMode == CANVAS_MOVE_MODE) {
            updateCanvasMove(gameState);
        }
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
        gameState->showNewCanvasWindow = true;
        gameState->autoFocus = true;
    }

    if(gameState->keys.keys[KEY_S] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND]) {
        saveFileToPNG(gameState);
    }

    if(gameState->showNewCanvasWindow) {
         //NOTE: Create new canvas
         ImGui::Begin("Canvas Size");       
         
         ImGui::Text("How big do you want the canvas?"); 
         if(gameState->autoFocus) {
            ImGui::SetKeyboardFocusHere();
            gameState->autoFocus = false;
         }
         ImGui::InputText("Width", gameState->dimStr0, IM_ARRAYSIZE(gameState->dimStr0));
         ImGui::InputText("Height", gameState->dimStr1, IM_ARRAYSIZE(gameState->dimStr1));
         if (ImGui::Button("Create")) {
            gameState->canvasW = atoi(gameState->dimStr0);
            gameState->canvasH = atoi(gameState->dimStr1);

            clearUndoRedoList(gameState);
            startCanvas(gameState);
            
            gameState->showNewCanvasWindow = false;
         }
 
         ImGui::End();
    }
    
    TimeOfDayValues timeOfDayValues = getTimeOfDayValues(gameState);
    // updateAndDrawDebugCode(gameState);
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation, timeOfDayValues, gameState->perlinTestTexture.handle);

}