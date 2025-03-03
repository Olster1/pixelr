u32 float4_to_u32_color(float4 c) {
    return (((u32)(c.w*255.0f)) << 24) | (((u32)(c.z*255.0f)) << 16) | (((u32)(c.y*255.0f)) << 8) | (((u32)(c.x*255.0f)) << 0);
}

float4 u32_to_float4_color(u32 c) {
    float a = ((float)((c >> 24) & 0xFF))/255.0f;
    float b = ((float)((c >> 16) & 0xFF))/255.0f;
    float g = ((float)((c >> 8) & 0xFF))/255.0f;
    float r = ((float)((c >> 0) & 0xFF))/255.0f;
    return make_float4(r, g, b, a);
}

float2 getWorldPFromMouse(GameState *gameState) {
    const float2 plane = scale_float2(0.5f, make_float2(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x));
                
     const float x = lerp(-plane.x, plane.x, make_lerpTValue(gameState->mouseP_01.x));
     const float y = lerp(-plane.y, plane.y, make_lerpTValue(gameState->mouseP_01.y));

     float2 worldP = plus_float2(gameState->camera.T.pos.xy, make_float2(x, y));
     return worldP;
}

float2 getCanvasCoordFromMouse(GameState *gameState) {
     //NOTE: Try draw on the canvas
     const float2 plane = scale_float2(0.5f, make_float2(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x));
                
     const float x = lerp(-plane.x, plane.x, make_lerpTValue(gameState->mouseP_01.x));
     const float y = lerp(-plane.y, plane.y, make_lerpTValue(gameState->mouseP_01.y));

     float2 worldP = plus_float2(gameState->camera.T.pos.xy, make_float2(x, y));
     float2 result = {};
     result.x = round(((worldP.x * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*gameState->canvasW) - 0.5f);
     result.y = round(((worldP.y * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*gameState->canvasH) - 0.5f);

     return result;
}


u32 getCanvasColor(GameState *gameState, int coordX, int coordY) {
    u32 result = 0;
    if(coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW) {
        result = gameState->canvas[coordY*gameState->canvasW + coordX];    
    } 

    return result;
    
}

void setCanvasColor(GameState *gameState, int coordX, int coordY, const u32 color, bool useOpacity = true) {
    if(coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW) {
        u32 oldColor = getCanvasColor(gameState, coordX, coordY);
        float4 oldColorF = u32_to_float4_color(oldColor);
        float4 newColorF = u32_to_float4_color(color);

        if(useOpacity) {
            newColorF.w = gameState->opacity;
        } else {
            newColorF.w = 1.0f;
        }
        
        
        // Convert colors to premultiplied alpha
        float oldR = oldColorF.x * oldColorF.w;
        float oldG = oldColorF.y * oldColorF.w;
        float oldB = oldColorF.z * oldColorF.w;
        
        float newR = newColorF.x * newColorF.w;
        float newG = newColorF.y * newColorF.w;
        float newB = newColorF.z * newColorF.w;
        
        // Compute new alpha
        float alphaNew = newColorF.w + oldColorF.w * (1.0f - newColorF.w);
        
        // Blend each color channel separately
        float blendedR = (newR + oldR * (1.0f - newColorF.w)) / MathMaxf(alphaNew, 1e-6f);
        float blendedG = (newG + oldG * (1.0f - newColorF.w)) / MathMaxf(alphaNew, 1e-6f);
        float blendedB = (newB + oldB * (1.0f - newColorF.w)) / MathMaxf(alphaNew, 1e-6f);
        
        // Store final color
        float4 c = make_float4(blendedR, blendedG, blendedB, alphaNew);

       

        gameState->canvas[coordY*gameState->canvasW + coordX] = float4_to_u32_color(c);
    }
}

bool isValidCanvasRange(GameState *gameState, int coordX, int coordY) {
    return (coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW);
}

bool isInShape(int x, int y, int w, int h, CanvasInteractionMode mode) {
    bool result = false;
    if(mode == CANVAS_DRAW_RECTANGLE_MODE) {
        if(x == 0 || x == w || y == 0 || y == h) {
            result = true;
        }
    } else if(mode == CANVAS_DRAW_CIRCLE_MODE) {
        float lengthX = sqr_float(x - 0.5f*w) / (0.25f * w * w);
        float lengthY = sqr_float(y - 0.5f*h) / (0.25f * h * h);
        
        float total = lengthX + lengthY;
        
        // Compute absolute distance from the ideal ellipse boundary (1.0)
        float distance = fabs(total - 1.0f);
        
        // Use a pixel-based threshold for thickness
        float pixelThreshold = 2.0f / std::min(w, h); // Ensures 1-pixel width even for large canvases
        
        if (distance < pixelThreshold) {
            result = true;
        }
    } else if(mode == CANVAS_DRAW_LINE_MODE) {
        float2 a = normalize_float2(float2_perp(make_float2(w, h)));

        float v = float2_dot(a, make_float2(x, y));

        float tolerance = 0.4f;

        if(fabs(v) < tolerance) {
            result = true;
        }
    }

    return result;
}

void drawDragShape(GameState *gameState, CanvasInteractionMode mode, bool fill = false) {
    float2 p = getCanvasCoordFromMouse(gameState);
    float2 diff1 = minus_float2(p, gameState->drawShapeStart);
    float2 diff = diff1;
    diff.x = abs(diff1.x);
    diff.y = abs(diff1.y);
    float4 color = gameState->colorPicked;

    float startX = (gameState->drawShapeStart.x < p.x) ? gameState->drawShapeStart.x : p.x;
    float startY = (gameState->drawShapeStart.y < p.y) ? gameState->drawShapeStart.y : p.y;

    float2 middle = make_float2(startX + 0.5f*diff.x, startY + 0.5f*diff.y);
    
    int w = round(diff.x);
    int h = round(diff.y);

    for(int y = 0; y <= h; ++y) {
        for(int x = 0; x <= w; ++x) {
            float2 offset = make_float2((startX + x) - gameState->drawShapeStart.x, (startY + y) - gameState->drawShapeStart.y);
            if(isInShape((mode == CANVAS_DRAW_LINE_MODE) ? round(offset.x) : x, (mode == CANVAS_DRAW_LINE_MODE) ? round(offset.y) : y, (mode == CANVAS_DRAW_LINE_MODE) ? round(diff1.x) : w, (mode == CANVAS_DRAW_LINE_MODE) ? round(diff1.y) : h, mode)) {
                if((startX + x) >= 0 && (startY + y) >= 0 && (startX + x) < gameState->canvasW && (startY + y) < gameState->canvasH) {
                    if(fill) {
                        float2 p = make_float2(startX + x,startY + y);
                        setCanvasColor(gameState, p.x, p.y, float4_to_u32_color(color));
                    } else {
                        float2 p = make_float2((startX + x)*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasW*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS, (startY + y)*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasH*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS);
                        color.w = gameState->opacity;
                        pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), color);
                    }

                }
            }
        }
    }
}

void drawCanvas(GameState *gameState) {
     //NOTE: Draw the canvas
     for(int y = 0; y < gameState->canvasH; ++y) {
        for(int x = 0; x < gameState->canvasW; ++x) {
            u32 c = gameState->canvas[y*gameState->canvasW + x];
            float4 color = u32_to_float4_color(c);

            float2 p = make_float2(x*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasW*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS, y*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasH*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS);

            if(gameState->checkBackground) 
            {
                float4 c = make_float4(0.8f, 0.8f, 0.8f, 1);
                if(((y % 2) == 0 && (x % 2) == 1) || ((y % 2) == 1 && (x % 2) == 0)) {
                    c = make_float4(0.6f, 0.6f, 0.6f, 1);
                }
                pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), c);
            }

            if(color.w > 0) {
                pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), color);
            }
        }
    }
}

void drawLinedGrid(GameState *gameState) {
    float4 greyColor = make_float4(0.6, 0.6, 0.6, 1);
    for(int x = 0; x < gameState->canvasH + 1; ++x) {
        if(gameState->drawGrid || (x == 0 || x == gameState->canvasH)) {
            TransformX T = {};
            T.pos = make_float3(0, x*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasH*VOXEL_SIZE_IN_METERS, 0);
            T.scale.xy = make_float2(gameState->canvasW*VOXEL_SIZE_IN_METERS, 0);
            float16 A = getModelToViewSpace(T);
            pushLine(gameState->renderer, A, greyColor);
        }
    }

    for(int y = 0; y < gameState->canvasW + 1; ++y) {
        if(gameState->drawGrid || (y == 0 || y == gameState->canvasW)) {
            TransformX T = {};
            T.pos = make_float3(y*VOXEL_SIZE_IN_METERS - 0.5f*gameState->canvasW*VOXEL_SIZE_IN_METERS, 0, 0);
            T.rotation.z = 90;
            T.scale.xy = make_float2(gameState->canvasH*VOXEL_SIZE_IN_METERS, 0);
            float16 A = getModelToViewSpace(T);
            pushLine(gameState->renderer, A, greyColor);
        }
    }
}

void updateCanvasZoom(GameState *gameState) {
    //NOTE: Zoom in & out
    gameState->camera.fov += 50*gameState->scrollSpeed*gameState->dt;
    
    float max = 0.01f;
    if(gameState->camera.fov < max) {
        gameState->camera.fov = max;
    }
} 

void updateUndoState(GameState *gameState) {
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
}

void updateEraser(GameState *gameState) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN) {
        float2 canvasP = getCanvasCoordFromMouse(gameState);
        addUndoRedoBlock(gameState, getCanvasColor(gameState, canvasP.x, canvasP.y), 0, canvasP.x, canvasP.y);
        for(int y = 0; y < gameState->eraserSize; y++) {
            for(int x = 0; x < gameState->eraserSize; x++) {
                float px = (canvasP.x - 0.5f*(gameState->eraserSize == 1 ? 0 : gameState->eraserSize)) + x;
                float py = (canvasP.y - 0.5f*(gameState->eraserSize == 1 ? 0 : gameState->eraserSize)) + y;
                setCanvasColor(gameState, px, py, 0x00FFFFFF, false);
            }
        }
    }
}


void updateCanvasMove(GameState *gameState) {
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

void updateDrawShape(GameState *gameState) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
        gameState->drawShapeStart = getCanvasCoordFromMouse(gameState);
        gameState->drawingShape = true;
    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->drawingShape) {
            drawDragShape(gameState, gameState->interactionMode);
            
        }
    } 
    
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->drawingShape) {
            drawDragShape(gameState, gameState->interactionMode, true);
            gameState->drawingShape = false;
        }
    }
}

void updateCanvasDraw(GameState *gameState) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
                
               
        float2 a = getCanvasCoordFromMouse(gameState);

        const int coordX = a.x;
        const int coordY = a.y;
       
         //NOTE: So there is always a continous line, even when the user is painting fast
         if(gameState->paintActive) {
             //NOTE: Fill in from the last one
             float2 endP = make_float2(coordX, coordY);
             float2 distance = minus_float2(endP, gameState->lastPaintP);
             float2 addend = scale_float2(VOXEL_SIZE_IN_METERS, normalize_float2(distance));

             float2 startP = plus_float2(gameState->lastPaintP, addend);//NOTE: Move past the one we just did last turn
             
             u32 color = float4_to_u32_color(gameState->colorPicked);
             while(float2_dot(minus_float2(startP, endP), minus_float2(gameState->lastPaintP, endP)) >= 0 && (float2_magnitude(addend) > 0)) {
                 int x1 = startP.x;
                 int y1 = startP.y;
                 if(y1 >= 0 && x1 >= 0 && y1 < gameState->canvasH && x1 < gameState->canvasW) {
                     if(!(x1 == coordX && y1 == coordY)) 
                     { //NOTE: Don't color the one were about to do after this loop
                         addUndoRedoBlock(gameState, getCanvasColor(gameState, x1, y1), color, x1, y1);
                         setCanvasColor(gameState, x1, y1, color);
                         
                     }
                 }

                 startP = plus_float2(startP, addend);
             }
         }
           
         if(coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW) {
             u32 color = float4_to_u32_color(gameState->colorPicked);
             
             addUndoRedoBlock(gameState, getCanvasColor(gameState, coordX, coordY), color, coordX, coordY);
             setCanvasColor(gameState, coordX, coordY, color);
         }

         gameState->paintActive = true;
         gameState->lastPaintP = make_float2(coordX, coordY);
     }
}

void showNewCanvas(GameState *gameState) {
    gameState->showNewCanvasWindow = true;
    gameState->autoFocus = true;
}