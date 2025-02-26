#include "includes.h"

u32 float4_to_u32_color(float4 c) {
    return (((u32)(c.x*255.0f)) << 24) | (((u32)(c.y*255.0f)) << 16) | (((u32)(c.z*255.0f)) << 8) | (((u32)(c.w*255.0f)) << 0);
}

float4 u32_to_float4_color(u32 c) {
    float r = ((float)((c >> 24) & 0xFF))/255.0f;
    float g = ((float)((c >> 16) & 0xFF))/255.0f;
    float b = ((float)((c >> 8) & 0xFF))/255.0f;
    float a = ((float)((c >> 0) & 0xFF))/255.0f;
    return make_float4(r, g, b, a);
}

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

        
    for(int x = 0; x < gameState->canvasW + 1; ++x) {
        TransformX T = {};
        T.pos = make_float3(0, x*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasW*VOXEL_SIZE_IN_METERS, 0);
        T.scale.xy = make_float2(gameState->canvasH*VOXEL_SIZE_IN_METERS, 0);
        float16 A = getModelToViewSpace(T);
        pushLine(gameState->renderer, A, make_float4(1, 0, 0, 1));
    }

    for(int y = 0; y < gameState->canvasH + 1; ++y) {
        TransformX T = {};
        T.pos = make_float3(y*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasH*VOXEL_SIZE_IN_METERS, 0, 0);
        T.rotation.z = 90;
        T.scale.xy = make_float2(gameState->canvasW*VOXEL_SIZE_IN_METERS, 0);
        float16 A = getModelToViewSpace(T);
        pushLine(gameState->renderer, A, make_float4(1, 0, 0, 1));
    }

    //NOTE: Draw the canvas
    for(int y = 0; y < gameState->canvasH; ++y) {
        // pushLine(gameState->renderer, float16 T, make_float4(1, 0, 0, 1));
        for(int x = 0; x < gameState->canvasW; ++x) {
            u32 c = gameState->canvas[y*gameState->canvasW + x];
            float4 color = u32_to_float4_color(c);

            float2 p = make_float2(x*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasW*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS, y*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasH*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS);

            if(gameState->checkBackground) 
            {
                float4 c = make_float4(0.8f, 0.8f, 0.8f, 0.3);
                if(((y % 2) == 0 && (x % 2) == 1) || ((y % 2) == 1 && (x % 2) == 0)) {
                    c = make_float4(0.6f, 0.6f, 0.6f, 0.3);
                }
                pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), c);
            }

            if(color.w > 0) {
                pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), color);
            }
        }
    }

    {
        //NOTE: Zoom in & out
        gameState->camera.fov += 50*gameState->scrollSpeed*gameState->dt;
        
        float max = 0.01f;
        if(gameState->camera.fov < max) {
            gameState->camera.fov = max;
        }
    } 

    //NOTE: Update interaction with the canvas
    if(!(ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered())) {
            
        
        
        if(gameState->undoList) {
            if(gameState->keys.keys[KEY_Z] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_SHIFT] != MOUSE_BUTTON_DOWN) {
                UndoRedoBlock *block = gameState->undoList;
                if(block->next && !isUndoBlockSentinel(block)) {
                    gameState->canvas[block->y*gameState->canvasW + block->x] = block->lastColor;
                    gameState->undoList = block->next;
                }
            }

            if(gameState->keys.keys[KEY_Z] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_SHIFT] == MOUSE_BUTTON_DOWN) {
                UndoRedoBlock *block = gameState->undoList;
                if(block->prev) {
                    block = block->prev;
                    gameState->canvas[block->y*gameState->canvasW + block->x] = block->thisColor;
                    gameState->undoList = block;
                }
            }
        }

        if(gameState->interactionMode == CANVAS_DRAW_MODE) {
            if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN) {
                //NOTE: Try draw on the canvas
                const float2 plane = scale_float2(0.5f, make_float2(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x));
                
                const float x = lerp(-plane.x, plane.x, make_lerpTValue(gameState->mouseP_01.x));
                const float y = lerp(-plane.y, plane.y, make_lerpTValue(gameState->mouseP_01.y));
    
                float2 worldP = plus_float2(gameState->camera.T.pos.xy, make_float2(x, y));
                const int coordX = round(((worldP.x * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*gameState->canvasW) - 0.5f);
                const int coordY = round(((worldP.y * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*gameState->canvasH) - 0.5f);
                
                if(coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW) {
                    u32 color = float4_to_u32_color(gameState->colorPicked);
                    addUndoRedoBlock(gameState, gameState->canvas[coordY*gameState->canvasW + coordX], color, coordX, coordY);
                    gameState->canvas[coordY*gameState->canvasW + coordX] = color;

                    
                }
                
            }
        } else if(gameState->interactionMode == CANVAS_MOVE_MODE) {
             if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
                //NOTE: Move the canvas
                gameState->draggingCanvas = true;
                gameState->startDragP = gameState->mouseP_01;
            } else if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN && gameState->draggingCanvas) {
                float2 diff = scale_float2(gameState->dt*200, minus_float2(gameState->startDragP, gameState->mouseP_01));
                gameState->camera.T.pos.xy = plus_float2(gameState->camera.T.pos.xy, diff);
                gameState->startDragP = gameState->mouseP_01;
            } else {
                gameState->draggingCanvas = false;
            }
        }
    }

    //NOTE: Update interaction mode
    if(gameState->keys.keys[KEY_1] == MOUSE_BUTTON_PRESSED) {
        gameState->interactionMode = CANVAS_DRAW_MODE;
    } else if(gameState->keys.keys[KEY_2] == MOUSE_BUTTON_PRESSED) {
        gameState->interactionMode = CANVAS_MOVE_MODE;
    }

    if(gameState->keys.keys[KEY_N] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_SHIFT]) {
        gameState->showNewCanvasWindow = true;
    }

    if(gameState->showNewCanvasWindow) {
         //NOTE: Create new canvas
         ImGui::Begin("Canvas Size");       
         
         ImGui::Text("How big do you want the canvas?"); 
         ImGui::InputText("Width", gameState->dimStr0, IM_ARRAYSIZE(gameState->dimStr0));
         ImGui::InputText("Height", gameState->dimStr1, IM_ARRAYSIZE(gameState->dimStr1));
         if (ImGui::Button("Create")) {
            gameState->canvasW = atoi(gameState->dimStr0);
            gameState->canvasH = atoi(gameState->dimStr1);

            for(int i = 0; i < gameState->canvasW*gameState->canvasH; ++i) {
                gameState->canvas[i] = 0;
            }
            gameState->showNewCanvasWindow = false;
         }
 
         ImGui::End();
    }
    
    TimeOfDayValues timeOfDayValues = getTimeOfDayValues(gameState);
    // updateAndDrawDebugCode(gameState);
    rendererFinish(gameState->renderer, screenT, cameraT, screenGuiT, textGuiT, lookingAxis, cameraTWithoutTranslation, timeOfDayValues, gameState->perlinTestTexture.handle);

   
}