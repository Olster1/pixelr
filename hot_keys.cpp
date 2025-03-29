void updateHotKeys(GameState *gameState) {
    if(!isInteractingWithIMGUI()) {
        //NOTE: Update interaction mode
        if(gameState->keys.keys[KEY_1] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_MOVE_MODE;
        } else if(gameState->keys.keys[KEY_2] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_DRAW_MODE;
        } else if(gameState->keys.keys[KEY_3] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_FILL_MODE;
        } else if(gameState->keys.keys[KEY_4] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_DRAW_CIRCLE_MODE;
        } else if(gameState->keys.keys[KEY_5] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_MOVE_SELECT_MODE;
        } else if(gameState->keys.keys[KEY_6] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_DRAW_RECTANGLE_MODE;
        } else if(gameState->keys.keys[KEY_7] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_ERASE_MODE;
        } else if(gameState->keys.keys[KEY_8] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_DRAW_LINE_MODE;
        } else if(gameState->keys.keys[KEY_9] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_SELECT_RECTANGLE_MODE;
        } else if(gameState->keys.keys[KEY_0] == MOUSE_BUTTON_PRESSED) {
            gameState->interactionMode = CANVAS_COLOR_DROPPER;
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
                gameState->clipboard.clear();
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

        if(gameState->keys.keys[KEY_X] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
            CanvasTab *t = getActiveCanvasTab(gameState);
            if(t) {
                gameState->clipboard.clear();
                for(int i = 0; i < getArrayLength(t->selected); i++) {
                    float2 p = t->selected[i];
                    Canvas *c = getActiveCanvas(gameState);
                    if(c) {
                        u32 color = getCanvasColor(c, (int)p.x, (int)p.y);
                        gameState->clipboard.addPixelInfo((int)p.x, (int)p.y, color);
                        setCanvasColor(t, c, p.x, p.y, 0x00FFFFFF, t->opacity, false);
                    }
                }
            }
        }

        if(gameState->keys.keys[KEY_V] == MOUSE_BUTTON_PRESSED && gameState->keys.keys[KEY_COMMAND] == MOUSE_BUTTON_DOWN) {
            Canvas *c = getActiveCanvas(gameState);
            CanvasTab *t = getActiveCanvasTab(gameState);
            if(t && c && gameState->clipboard.hasCopy()) {

                //NOTE: Inititate the Select Object
                gameState->selectObject.isActive = true;
                t->savedOpacity = t->opacity;
                t->opacity = 1; //NOTE: Reset the opacity
                gameState->selectObject.timeAt = 0;
                gameState->selectObject.T = initTransformX();
                gameState->selectObject.clear();
                gameState->interactionMode = CANVAS_MOVE_SELECT_MODE;

                t->clearSelection();

                float minX = FLT_MAX;
                float maxX = -FLT_MAX;
                float minY = FLT_MAX;
                float maxY = -FLT_MAX;
                
                for(int i = 0; i < getArrayLength(gameState->clipboard.pixels); i++) {
                    PixelClipboardInfo info = gameState->clipboard.pixels[i];
                    if(u32_to_float4_color(info.color).w > 0) {
                        // pushArrayItem(&gameState->selectObject.pixels, info, PixelClipboardInfo);

                        if(info.x < minX) {
                            minX = info.x;
                        }
                        if(info.x > maxX) {
                            maxX = info.x;
                        }
                        if(info.y < minY) {
                            minY = info.y;
                        }
                        if(info.y > maxY) {
                            maxY = info.y;
                        }
                    }
                }

                //NOTE: Account for the max borders are on the outer edge of the pixel, so essentially the next pixel over
                maxX++;
                maxY++;

                int w = maxX - minX;
                int h = maxY - minY;
                gameState->selectObject.clear();
                gameState->selectObject.pixels = (u32 *)easyPlatform_allocateMemory(sizeof(u32)*w*h);

                for(int i = 0; i < getArrayLength(gameState->clipboard.pixels); i++) {
                    PixelClipboardInfo info = gameState->clipboard.pixels[i];
                    if(u32_to_float4_color(info.color).w > 0) {
                        int x = info.x - minX;
                        int y = info.y - minY;
                        
                        assert(x >= 0 && x < w);
                        assert(y >= 0 && y < h);

                        gameState->selectObject.pixels[y*w + x] = info.color;
                    }
                }

                gameState->selectObject.boundsCanvasSpace = make_rect2f(minX, minY, maxX, maxY);
                gameState->selectObject.startCanvasP.x = lerp(minX, maxX, make_lerpTValue(0.5f));
                gameState->selectObject.startCanvasP.y = lerp(minY, maxY, make_lerpTValue(0.5f));
                gameState->selectObject.origin = make_float2(minX, minY);
                gameState->selectObject.wh = make_float2(w, h);
                
            }
        }

        if(gameState->keys.keys[KEY_DELETE] == MOUSE_BUTTON_PRESSED || gameState->keys.keys[KEY_BACKSPACE] == MOUSE_BUTTON_PRESSED) {
            Canvas *c = getActiveCanvas(gameState);
            CanvasTab *t = getActiveCanvasTab(gameState);
            if(t && c) {
                for(int i = 0; i < getArrayLength(t->selected); i++) {
                    float2 p = t->selected[i];
                    setCanvasColor(t, c, p.x, p.y, 0x00FFFFFF, t->opacity, false);
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
    }
}