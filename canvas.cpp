

Canvas *getActiveCanvas(GameState *gameState) {
    CanvasTab *t = gameState->canvasTabs + gameState->activeCanvasTab;
    assert(t);

    Frame *f = t->frames + t->activeFrame;
    assert(f);

    Canvas *canvas = f->layers + f->activeLayer;
    assert(canvas);

    return canvas;

}

CanvasTab *getActiveCanvasTab(GameState *gameState) {
    CanvasTab *t = gameState->canvasTabs + gameState->activeCanvasTab;
    return t;

}

Frame *getActiveFrame(GameState *gameState) {
    CanvasTab *t = gameState->canvasTabs + gameState->activeCanvasTab;
    Frame *f = t->frames + t->activeFrame;
    return f;

}

bool isInteractingWithIMGUI() {
    return (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered());
}

float2 getMouseP01(GameState *gameState) {
    //NOTE: Running average of the mouse
    float2 result = make_float2(0, 0);

    int startAt = gameState->mouseIndexAt - 1;
    int totalCount = MathMin(gameState->runningAverageCount, gameState->mouseCountAt);

    if(totalCount > 0 && gameState->runningAverageCount > 1) {
        int index = startAt;
        for(int i = 0; i < totalCount; ++i) {
            index = ((startAt - i) + gameState->mouseCountAt) % gameState->mouseCountAt;

            assert(index >= 0 && index < gameState->mouseCountAt);
            result = plus_float2(result, gameState->mouseP_01_array[index]);
        }

        result.x *= (1.0f / totalCount);
        result.y *= (1.0f / totalCount);
    } else {
        result = gameState->mouseP_01;
    }
    
    return result;
}
  
float2 getWorldPFromMouse(GameState *gameState) {
    const float2 plane = scale_float2(0.5f, make_float2(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x));
                
     const float x = lerp(-plane.x, plane.x, make_lerpTValue(getMouseP01(gameState).x));
     const float y = lerp(-plane.y, plane.y, make_lerpTValue(getMouseP01(gameState).y));

     float2 worldP = plus_float2(gameState->camera.T.pos.xy, make_float2(x, y));
     return worldP;
}

float2 getCanvasCoordFromMouse(GameState *gameState, int w, int h) {
     //NOTE: Try draw on the canvas
     const float2 plane = scale_float2(0.5f, make_float2(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x));
                
     const float x = lerp(-plane.x, plane.x, make_lerpTValue(getMouseP01(gameState).x));
     const float y = lerp(-plane.y, plane.y, make_lerpTValue(getMouseP01(gameState).y));

     float2 worldP = plus_float2(gameState->camera.T.pos.xy, make_float2(x, y));
     float2 result = {};
     result.x = round(((worldP.x * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*w) - 0.5f);
     result.y = round(((worldP.y * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*h) - 0.5f);

     return result;
}


u32 getCanvasColor(Canvas *canvas, int coordX, int coordY) {
    u32 result = 0;
    if(coordY >= 0 && coordX >= 0 && coordY < canvas->h && coordX < canvas->w) {
        result = canvas->pixels[coordY*canvas->w + coordX];    
    } 

    return result;
    
}

void setCanvasColor(CanvasTab *tab, Canvas *canvas, int coordX, int coordY, const u32 color, float opacity, bool useOpacity = true) {
    if(coordY >= 0 && coordX >= 0 && coordY < canvas->h && coordX < canvas->w) {
        u32 oldColor = getCanvasColor(canvas, coordX, coordY);
        float4 oldColorF = u32_to_float4_color(oldColor);
        float4 newColorF = u32_to_float4_color(color);

        float4 c;
        if(useOpacity) {
            newColorF.w = opacity;

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
            c = make_float4(blendedR, blendedG, blendedB, alphaNew);

        } else {
            c = newColorF;
        }

        tab->addUndoInfo(PixelInfo(coordX, coordY, oldColor, float4_to_u32_color(c)));
        canvas->pixels[coordY*canvas->w + coordX] = float4_to_u32_color(c);
    }
}

bool isValidCanvasRange(Canvas *canvas, int coordX, int coordY) {
    return (coordY >= 0 && coordX >= 0 && coordY < canvas->h && coordX < canvas->w);
}

bool isValidCanvasTabRange(CanvasTab *canvas, int coordX, int coordY) {
    return (coordY >= 0 && coordX >= 0 && coordY < canvas->h && coordX < canvas->w);
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

void clearSelection(CanvasTab *canvas, GameState *gameState = 0) {
    bool shouldClear = true;
    if(gameState) {
        shouldClear = !(gameState->keys.keys[KEY_SHIFT] == MOUSE_BUTTON_DOWN);
    }
    if(shouldClear) {
        clearResizeArray(canvas->selected);
    }
}

void drawSelectShape(GameState *gameState, CanvasTab *canvas, CanvasInteractionMode mode) {
    float2 p = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
    float2 diff1 = minus_float2(p, gameState->drawShapeStart);
    float2 diff = diff1;
    diff.x = abs(diff1.x);
    diff.y = abs(diff1.y);
    float4 color = gameState->colorPicked;

    float startX = (gameState->drawShapeStart.x < p.x) ? gameState->drawShapeStart.x : p.x;
    float startY = (gameState->drawShapeStart.y < p.y) ? gameState->drawShapeStart.y : p.y;

    int w = round(diff.x);
    int h = round(diff.y);

    clearSelection(canvas, gameState);

    for(int y = 0; y <= h; ++y) {
        for(int x = 0; x <= w; ++x) {
            float2 f = make_float2(round(x + startX), round(y + startY));
            pushArrayItem(&canvas->selected, f, float2);
        }
    }
    
}

void drawDragShape(GameState *gameState, Canvas *canvas, CanvasInteractionMode mode, bool fill = false) {
    float2 p = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
    float2 diff1 = minus_float2(p, gameState->drawShapeStart);
    float2 diff = diff1;
    diff.x = abs(diff1.x);
    diff.y = abs(diff1.y);
    float4 color = gameState->colorPicked;

    CanvasTab *tab = getActiveCanvasTab(gameState);

    float startX = (gameState->drawShapeStart.x < p.x) ? gameState->drawShapeStart.x : p.x;
    float startY = (gameState->drawShapeStart.y < p.y) ? gameState->drawShapeStart.y : p.y;

    float2 middle = make_float2(startX + 0.5f*diff.x, startY + 0.5f*diff.y);
    
    int w = round(diff.x);
    int h = round(diff.y);

    for(int y = 0; y <= h; ++y) {
        for(int x = 0; x <= w; ++x) {
            float2 offset = make_float2((startX + x) - gameState->drawShapeStart.x, (startY + y) - gameState->drawShapeStart.y);
            if(isInShape((mode == CANVAS_DRAW_LINE_MODE) ? round(offset.x) : x, (mode == CANVAS_DRAW_LINE_MODE) ? round(offset.y) : y, (mode == CANVAS_DRAW_LINE_MODE) ? round(diff1.x) : w, (mode == CANVAS_DRAW_LINE_MODE) ? round(diff1.y) : h, mode)) {
                if((startX + x) >= 0 && (startY + y) >= 0 && (startX + x) < canvas->w && (startY + y) < canvas->h) {
                    if(fill) {
                        float2 p = make_float2(startX + x,startY + y);
                        setCanvasColor(tab, canvas, p.x, p.y, float4_to_u32_color(color), gameState->opacity);
                    } else {
                        float2 p = make_float2((startX + x)*VOXEL_SIZE_IN_METERS - 0.5f*canvas->w*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS, (startY + y)*VOXEL_SIZE_IN_METERS - 0.5f*canvas->h*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS);
                        color.w = gameState->opacity;
                        pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), color);
                    }

                }
            }
        }
    }
}

float2 canvasCoordToWorldSpace(Canvas *canvas, float x, float y, bool offsetP = true) {
    float2 p = make_float2(x*VOXEL_SIZE_IN_METERS - 0.5f*canvas->w*VOXEL_SIZE_IN_METERS, y*VOXEL_SIZE_IN_METERS - 0.5f*canvas->h*VOXEL_SIZE_IN_METERS);

    if(offsetP) {
        p.x += 0.5f*VOXEL_SIZE_IN_METERS;
        p.y += 0.5f*VOXEL_SIZE_IN_METERS;
    }
    return p;
}

void updateCanvasSelectionTexture(Renderer *renderer, CanvasTab *t) {
    renderer->selectionTextureHandle = t->selectionGpuHandle;
    glBindTexture(GL_TEXTURE_2D, t->selectionGpuHandle);
    renderCheckError();

    u8 *pixels = (u8 *)pushArray(&globalPerFrameArena, t->w*t->h, u8);
            
    for(int i = 0; i < getArrayLength(t->selected); ++i) {
        float2 p = t->selected[i];
        int y = (t->h - 1) - p.y;
        if(isValidCanvasTabRange(t, p.x, y)) 
        {
            pixels[(int)(y*t->w + p.x)] = 0xFF;
        }
    } 
            
    glTexSubImage2D(
        GL_TEXTURE_2D,  // Target
        0,              // Mipmap level (0 for base level)
        0, 0,           // X and Y offset (update from the top-left corner)
        t->w, t->h,  // Width and height of the new data
        GL_RED,        // Format of the pixel data
        GL_UNSIGNED_BYTE, // Data type
        pixels    // Pointer to new pixel data
    );

    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();
}

void drawCanvasGridBackground(GameState *gameState, Canvas *canvas, CanvasTab *canvasTab) {
    if(gameState->checkBackground) {
        //NOTE: Draw the canvas
        // for(int y = 0; y < canvas->h; ++y) {
        //     for(int x = 0; x < canvas->w; ++x) {
        //         u32 c = canvas->pixels[y*canvas->w + x];
        //         float4 color = u32_to_float4_color(c);

        //         float2 p = make_float2(x*VOXEL_SIZE_IN_METERS - 0.5f*canvas->w*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS, y*VOXEL_SIZE_IN_METERS - 0.5f*canvas->h*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS);

        //         {
        //             float4 c = make_float4(0.8f, 0.8f, 0.8f, 1);
        //             if(((y % 2) == 0 && (x % 2) == 1) || ((y % 2) == 1 && (x % 2) == 0)) {
        //                 c = make_float4(0.6f, 0.6f, 0.6f, 1);
        //             }
        //             pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), c);
        //         }
        //     }
        // }

        pushCanvasQuad(gameState->renderer, make_float3(0, 0, 0), make_float2(canvasTab->w*VOXEL_SIZE_IN_METERS, canvasTab->h*VOXEL_SIZE_IN_METERS), make_float4(1, 1, 1, 1), canvasTab->checkBackgroundHandle);
    }
}

void drawCanvas(GameState *gameState, Frame *frame, CanvasTab *canvasTab, float onionSkinOpacity) {
     //NOTE: Draw the canvas
    //  for(int y = 0; y < canvas->h; ++y) {
    //     for(int x = 0; x < canvas->w; ++x) {
    //         u32 c = canvas->pixels[y*canvas->w + x];
    //         float4 color = u32_to_float4_color(c);

    //         float2 p = make_float2(x*VOXEL_SIZE_IN_METERS - 0.5f*canvas->w*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS, y*VOXEL_SIZE_IN_METERS - 0.5f*canvas->h*VOXEL_SIZE_IN_METERS + 0.5f*VOXEL_SIZE_IN_METERS);

    //         color.w *= onionSkinOpacity;

    //         if(color.w > 0) {
    //             pushColoredQuad(gameState->renderer, make_float3(p.x, p.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), color);
    //         }
    //     }
    // }

    pushCanvasQuad(gameState->renderer, make_float3(0, 0, 0), make_float2(canvasTab->w*VOXEL_SIZE_IN_METERS, canvasTab->h*VOXEL_SIZE_IN_METERS), make_float4(1, 1, 1, onionSkinOpacity), frame->gpuHandle);

    //NOTE: Draw selected
    if(getArrayLength(canvasTab->selected) > 0) {
        updateCanvasSelectionTexture(gameState->renderer, canvasTab);
        pushSelectionQuad(gameState->renderer, make_float3(0, 0, 0), make_float2(canvasTab->w*VOXEL_SIZE_IN_METERS, canvasTab->h*VOXEL_SIZE_IN_METERS), make_float4(1, 1, 1, 0.7f));
    }
}

void updateSelectObject(GameState *gameState, Canvas *canvas) {
    if(gameState->selectObject.isActive && gameState->selectObject.pixels && getArrayLength(gameState->selectObject.pixels)) {
        gameState->selectObject.timeAt += gameState->dt;
        bool submit = false;
        if(gameState->keys.keys[KEY_ENTER] == MOUSE_BUTTON_PRESSED) {
            submit = true;
        }

        CanvasTab *tab = getActiveCanvasTab(gameState);

       
        TransformX TX = gameState->selectObject.T;
        float16 T = getModelToViewSpace(TX);

        float2 bounds[4] = {make_float2(gameState->selectObject.boundsCanvasSpace.minX, gameState->selectObject.boundsCanvasSpace.minY),
                            make_float2(gameState->selectObject.boundsCanvasSpace.minX, gameState->selectObject.boundsCanvasSpace.maxY),
                            make_float2(gameState->selectObject.boundsCanvasSpace.maxX, gameState->selectObject.boundsCanvasSpace.maxY),
                            make_float2(gameState->selectObject.boundsCanvasSpace.maxX, gameState->selectObject.boundsCanvasSpace.minY)
                        };

        float2 bounds1[4] = {};

                    
        for(int i = 0; i < arrayCount(bounds); ++i) {
            float2 p = bounds[i];
            
            float4 canvasP = float16_transform(T, make_float4(p.x - gameState->selectObject.startCanvasP.x, p.y - gameState->selectObject.startCanvasP.y, 0, 1));

            canvasP.x += gameState->selectObject.startCanvasP.x;
            canvasP.y += gameState->selectObject.startCanvasP.y;

            bounds1[i] = make_float2(canvasP.x, canvasP.y);

            bounds[i] = canvasCoordToWorldSpace(canvas, canvasP.x, canvasP.y, false);
        }

        if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
            float2 mouseP = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
            gameState->selectObject.dragging = true;
            float2 canvasP = plus_float2(gameState->selectObject.T.pos.xy, gameState->selectObject.startCanvasP);
            gameState->selectObject.dragOffset = minus_float2(canvasP, mouseP);
        }
        if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN && gameState->selectObject.dragging && !isInteractingWithIMGUI()) {
            float2 mouseP = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
            gameState->selectObject.T.pos.xy = minus_float2(plus_float2(mouseP, gameState->selectObject.dragOffset), gameState->selectObject.startCanvasP);
        }

        //NOTE: Now draw the box
        for(int i = 0; i < arrayCount(bounds); ++i) {

            int j = i + 1;
            if(j >= arrayCount(bounds)) {
                j = 0;
            }

            pushLineEndToEndWorldSpace(gameState->renderer, bounds[i], bounds[j], lerp_float4(make_float4(0, 1, 0, 1), make_float4(0, 1, 1, 1), easeInEaseOut(3*gameState->selectObject.timeAt)));
        }

        for(int i = 0; i < getArrayLength(gameState->selectObject.pixels); ++i) {
            PixelClipboardInfo pixel = gameState->selectObject.pixels[i];
            float4 canvasP = float16_transform(T, make_float4(pixel.x - gameState->selectObject.startCanvasP.x, pixel.y - gameState->selectObject.startCanvasP.y, 0, 1));

            canvasP.x += gameState->selectObject.startCanvasP.x;
            canvasP.y += gameState->selectObject.startCanvasP.y;

            int minX = round(canvasP.x - 0.5f*TX.scale.x);
            int maxX = round(canvasP.x + 0.5f*TX.scale.x);
            int minY = round(canvasP.y - 0.5f*TX.scale.y);
            int maxY = round(canvasP.y + 0.5f*TX.scale.y);

            for(int y = minY; y < maxY; y++) {
                for(int x = minX; x < maxX; x++) {
                    if(isValidCanvasRange(canvas, x, y)) {
                        float4 color4 = u32_to_float4_color(pixel.color);

                        if(color4.w > 0) {
                            float2 pixelP = canvasCoordToWorldSpace(canvas, round(x), round(y));
                            pushColoredQuad(gameState->renderer, make_float3(pixelP.x, pixelP.y, 0), make_float2(VOXEL_SIZE_IN_METERS, VOXEL_SIZE_IN_METERS), color4);

                            if(submit) {
                                setCanvasColor(tab, canvas, round(x), round(y), pixel.color, gameState->opacity);
                            }
                        }
                    }
                }
            }
        }

        if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED || gameState->mouseLeftBtn == MOUSE_BUTTON_NONE) {
            gameState->selectObject.dragging = false;
        }

        if(submit) {
            gameState->selectObject.isActive = false;
            gameState->selectObject.clear();
        }
    }
}

void drawLinedGrid(GameState *gameState, Canvas *canvas) {
    float4 greyColor = make_float4(0.6, 0.6, 0.6, 1);
    for(int x = 0; x < canvas->h + 1; ++x) {
        if(gameState->drawGrid || (x == 0 || x == canvas->h)) {
            TransformX T = {};
            T.pos = make_float3(0, x*VOXEL_SIZE_IN_METERS - 0.5f*canvas->h*VOXEL_SIZE_IN_METERS, 0);
            T.scale.xy = make_float2(canvas->w*VOXEL_SIZE_IN_METERS, 0);
            float16 A = getModelToViewSpace(T);
            pushLine(gameState->renderer, A, greyColor);
        }
    }

    for(int y = 0; y < canvas->w + 1; ++y) {
        if(gameState->drawGrid || (y == 0 || y == canvas->w)) {
            TransformX T = {};
            T.pos = make_float3(y*VOXEL_SIZE_IN_METERS - 0.5f*canvas->w*VOXEL_SIZE_IN_METERS, 0, 0);
            T.rotation.z = 90;
            T.scale.xy = make_float2(canvas->h*VOXEL_SIZE_IN_METERS, 0);
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

Canvas *getCanvasForUndoBlock(CanvasTab *tab, UndoRedoBlock *block) {
    Canvas *canvas = tab->getActiveCanvas();
    bool found = false;
    
    if(canvas && !areEntityIdsEqual(canvas->id, block->canvasId)) {
        //NOTE: Canvas isn't the active one so find the active one
        for (int i = 0; i < getArrayLength(tab->frames) && !found; i++) {
            Frame *frame = tab->frames + i;
            for (int j = 0; j < getArrayLength(frame->layers) && !found; j++) {
                Canvas *c = frame->layers + j;
                if(areEntityIdsEqual(c->id, block->canvasId)) {
                    canvas = c;
                    tab->activeFrame = i;
                    frame->activeLayer = j;
                    found = true;
                    break;
                }
            }
        }
    }

    assert(canvas);

    return canvas;
}

void updateUndoState(GameState *gameState, bool undo = false, bool redo = false) {
    CanvasTab *tab = getActiveCanvasTab(gameState);

    if(tab) {
        if(tab->undoList) {
            if(undo || (gameState->keys.keys[KEY_Z] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_SHIFT] != MOUSE_BUTTON_DOWN)) {
                UndoRedoBlock *block = tab->undoList;
                if(block->next && !isUndoBlockSentinel(block)) {
                    Canvas *canvas = getCanvasForUndoBlock(tab, block);
                    if(canvas) {
                        for(int i = 0; block->pixelInfos && i < getArrayLength(block->pixelInfos); ++i) {
                            PixelInfo info = block->pixelInfos[i];
                            if(info.y >= 0 && info.x >= 0 && info.y < canvas->h && info.x < canvas->w) {
                                canvas->pixels[info.y*canvas->w + info.x] = info.lastColor;
                            }
                        }
                        tab->undoList = block->next;
                    }
                }
            }

            if(redo || (gameState->keys.keys[KEY_Z] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN && gameState->keys.keys[KEY_SHIFT] == MOUSE_BUTTON_DOWN)) {
                UndoRedoBlock *block = tab->undoList;
                if(block->prev) {
                    block = block->prev;
                    Canvas *canvas = getCanvasForUndoBlock(tab, block);
                    {
                        for(int i = 0; block->pixelInfos && i < getArrayLength(block->pixelInfos); ++i) {
                            PixelInfo info = block->pixelInfos[i];
                            if(info.y >= 0 && info.x >= 0 && info.y < canvas->h && info.x < canvas->w) {
                                canvas->pixels[info.y*canvas->w + info.x] = info.thisColor;
                            }
                        }
                    }
                    tab->undoList = block;
                }
            }
        }
    }
}

void updateEraser(GameState *gameState, Canvas *canvas) {
    CanvasTab *tab = getActiveCanvasTab(gameState);

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN) {
        float2 canvasP = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
        
        
        for(int y = 0; y < gameState->eraserSize; y++) {
            for(int x = 0; x < gameState->eraserSize; x++) {
                float px = (canvasP.x - 0.5f*(gameState->eraserSize == 1 ? 0 : gameState->eraserSize)) + x;
                float py = (canvasP.y - 0.5f*(gameState->eraserSize == 1 ? 0 : gameState->eraserSize)) + y;
                setCanvasColor(tab, canvas, px, py, 0x00FFFFFF, gameState->opacity, false);
            }
        }
    }
}


void updateCanvasMove(GameState *gameState) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
        //NOTE: Move the canvas
        gameState->draggingCanvas = true;
        gameState->startDragP = getMouseP01(gameState);
    } else if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN && gameState->draggingCanvas) {
        float2 diff = scale_float2(gameState->dt*200, minus_float2(gameState->startDragP, getMouseP01(gameState)));
        gameState->camera.T.pos.xy = plus_float2(gameState->camera.T.pos.xy, diff);
        gameState->startDragP = getMouseP01(gameState);
    } else {
        gameState->draggingCanvas = false;
    }
}

void updateDrawShape(GameState *gameState, Canvas *canvas) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
        gameState->drawShapeStart = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
        gameState->drawingShape = true;

    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->drawingShape) {
            drawDragShape(gameState, canvas, gameState->interactionMode);
            
        }
    } 
    
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->drawingShape) {
            drawDragShape(gameState, canvas, gameState->interactionMode, true);
            gameState->drawingShape = false;
        }
    }
}

void updateCanvasSelect(GameState *gameState, CanvasTab *canvas) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
        gameState->drawShapeStart = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
        gameState->drawingShape = true;
    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->drawingShape) {
            drawSelectShape(gameState, canvas, gameState->interactionMode);
        }
    } 
    
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->drawingShape) {
            gameState->drawingShape = false;
        }
    }
}

void updateCanvasDraw(GameState *gameState, Canvas *canvas) {
    CanvasTab *tab = getActiveCanvasTab(gameState);
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
               
        float2 a = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);

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
                 if(y1 >= 0 && x1 >= 0 && y1 < canvas->h && x1 < canvas->w) {
                     if(!(x1 == coordX && y1 == coordY)) 
                     { //NOTE: Don't color the one were about to do after this loop
                        setCanvasColor(tab, canvas, x1, y1, color, gameState->opacity);
                     }
                 }

                 startP = plus_float2(startP, addend);
             }
         }
           
         if(coordY >= 0 && coordX >= 0 && coordY < canvas->h && coordX < canvas->w) {
             u32 color = float4_to_u32_color(gameState->colorPicked);
             
             setCanvasColor(tab, canvas, coordX, coordY, color, gameState->opacity);
         }

         gameState->paintActive = true;
         gameState->lastPaintP = make_float2(coordX, coordY);
     }
}

void showNewCanvas(GameState *gameState) {
    gameState->showNewCanvasWindow = true;
    gameState->autoFocus = true;
}

void updateFrameGPUHandles(Frame *f, CanvasTab *t) {
   
    for (int j = 0; j < getArrayLength(f->layers); j++) {
        Canvas *c = f->layers + j;
        assert(c->gpuHandle > 0);

        glBindTexture(GL_TEXTURE_2D, c->gpuHandle);
        renderCheckError();

        glTexSubImage2D(
            GL_TEXTURE_2D,  // Target
            0,              // Mipmap level (0 for base level)
            0, 0,           // X and Y offset (update from the top-left corner)
            t->w, t->h,  // Width and height of the new data
            GL_RGBA,        // Format of the pixel data
            GL_UNSIGNED_BYTE, // Data type
            c->pixels    // Pointer to new pixel data
        );

        renderCheckError();
        glBindTexture(GL_TEXTURE_2D, 0);
        renderCheckError();
    }
}

void updateGpuCanvasTextures(GameState *gameState) {
    CanvasTab *t = getActiveCanvasTab(gameState);

    for (int i = 0; i < getArrayLength(t->frames); i++) {
        Frame *f = t->frames + i;
        
        updateFrameGPUHandles(f, t);

        //NOTE: Draw to a frame buffer all the images 
        glBindTexture(GL_TEXTURE_2D, f->gpuHandle);
        renderCheckError();

        glTexSubImage2D(
            GL_TEXTURE_2D,  // Target
            0,              // Mipmap level (0 for base level)
            0, 0,           // X and Y offset (update from the top-left corner)
            t->w, t->h,  // Width and height of the new data
            GL_RGBA,        // Format of the pixel data
            GL_UNSIGNED_BYTE, // Data type
            f->layers[0].pixels    // Pointer to new pixel data
        );
        
        renderCheckError();

        glBindTexture(GL_TEXTURE_2D, 0);
        renderCheckError();
    }
}