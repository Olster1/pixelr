void saveFileToPNG(Renderer *renderer, Frame *frame, CanvasTab *tab) {
    DEBUG_TIME_BLOCK()
    char *result = (char *)tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    if(result) {
        size_t bytesPerPixel = sizeof(uint32_t);
        int stride_in_bytes = bytesPerPixel*tab->w;
        
        char *ending = ".png";
        char *extension = getFileExtension((char *)result);

        if(extension && easyString_stringsMatch_nullTerminated(extension, "png")) {
            ending = "";
        }

        updateFrameGPUHandles(frame, tab);

        char *name = easy_createString_printf(&globalPerFrameArena, "%s%s", (char *)result, ending);
        int writeResult = stbi_write_png(name, tab->w, tab->h, 4, getPixelsForFrame_shortTerm(tab, frame), stride_in_bytes);

        addIMGUIToast("Image Saved.", 2);
    }
}

void exportFramesToGif(Renderer *renderer, CanvasTab *tab) {
    DEBUG_TIME_BLOCK()
    char *result = (char *)tinyfd_saveFileDialog (
    "Save Gif",
    "",
    0,
    NULL,
    NULL);

    if(result) {
        size_t bytesPerPixel = sizeof(uint32_t);
        int stride_in_bytes = bytesPerPixel*tab->w;

        char *ending = ".gif";
        char *extension = getFileExtension((char *)result);

        if(extension && easyString_stringsMatch_nullTerminated(extension, "gif")) {
            ending = "";
        }
        
        char *fileName = easy_createString_printf(&globalPerFrameArena, "%s%s", (char *)result, ending);

        int pitch = tab->w * 4 * -1; //NOTE: * -1 since we can flip the image vertically by passing a negative pitch, so the imgae is the correct way up

        int centisecondsPerFrame = (tab->playback.frameTime / 100.0f); 

        int quality = 16;
        MsfGifState gifState = {};
        // msf_gif_bgra_flag = true; //optionally, set this flag if your pixels are in BGRA format instead of RGBA
        msf_gif_alpha_threshold = 128; //optionally, enable transparency (see function documentation below for details)
        msf_gif_begin(&gifState, tab->w, tab->h);

        for(int i = 0; i < getArrayLength(tab->frames); i++) {
            Frame *frame = tab->frames + i;
            if(!frame->deleted) {
                updateFrameGPUHandles(frame, tab);
                msf_gif_frame(&gifState, (u8 *)getPixelsForFrame_shortTerm(tab, frame), centisecondsPerFrame, quality, pitch);
            }
        }

        MsfGifResult result = msf_gif_end(&gifState);
        if (result.data) {
            game_file_handle fileHandle = platformBeginFileWrite((char *)fileName);
            assert(!fileHandle.HasErrors);
            platformWriteFile(&fileHandle, result.data, result.dataSize, 0);
            platformEndFile(fileHandle);
        } else {
            addIMGUIToast("Couldn't save GIF. Please try again.", 2);
        }
        msf_gif_free(result);
    }
}

void loadPngColorPalette(CanvasTab *tab, bool clearPalette) {
    DEBUG_TIME_BLOCK()
    const char *filterPatterns[] = { };
    const char *filePath = tinyfd_openFileDialog(
        "Load Color Palette",         // Dialog title
        "",                    // Default path
        0,                     // Number of filters
        NULL,        // Filters
        NULL,    // Filter description
        0                      // Allow multiple selection (0 = No, 1 = Yes)
    );
    
    if (filePath) {
            
        stbi_set_flip_vertically_on_load(1);
        int w = 0;
        int h = 0;
        int nrChannels = 0;
        u32 *data = (u32 *)stbi_load(filePath, &w, &h, &nrChannels, STBI_rgb_alpha);

        if(data) {
            if(clearPalette) {
                tab->palletteCount = 0;
            }

            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    tab->addColorToPalette(data[y*w + x]);
                }
            }
            stbi_image_free(data);
        }
        stbi_set_flip_vertically_on_load(0);
    }
    

}

void openSpriteSheet(GameState *gameState, int w, int h) {
    DEBUG_TIME_BLOCK()
    const char *filePath = tinyfd_openFileDialog(
        "Open an image",         // Dialog title
        "",                    // Default path
        0,                     // Number of filters
        NULL,        // Filters
        NULL,    // Filter description
        0                      // Allow multiple selection (0 = No, 1 = Yes)
    );
    
    if (filePath) {
            
            stbi_set_flip_vertically_on_load(1);
            int totalW = 0;
            int totalH = 0;
            int nrChannels = 0;
            u32 *data = (u32 *)stbi_load(filePath, &totalW, &totalH, &nrChannels, STBI_rgb_alpha);

            if(data) {
                char *name = getFileLastPortionWithoutExtension((char *)filePath);
                CanvasTab tab_ = CanvasTab(w, h, name);
                CanvasTab *tab = pushArrayItem(&gameState->canvasTabs, tab_, CanvasTab);
                gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
                tab->uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;

                int wCount = totalW / w;
                int hCount = totalH / h;

                int canvasIndex = 0;

                for(int y = 0; y < hCount; ++y) {
                    for(int x = 0; x < wCount; ++x) {
                        Canvas *canvas = 0;
                        if(canvasIndex == 0) {
                            //NOTE: Resuse the canvas that got allocated when we create the canvas tab
                            canvas = getActiveCanvas(gameState);
                        } else {
                            canvas = tab->addEmptyCanvas();
                        }
                        
                        if(canvas) {
                            for(int y1 = 0; y1 < h; ++y1) {
                                for(int x1 = 0; x1 < w; ++x1) {
                                    int xTemp = x1 + x*w;
                                    int yTemp = y1 + y*h;
                                    int index = yTemp*totalW + xTemp;
                                    assert(index < (totalW*totalH));
                                    u32 color = data[index];
                                    canvas->pixels[y1*w + x1] = color;

                                }
                            }
                        }

                        canvasIndex++;
                    }
                }
                updateGpuCanvasTextures(gameState);
                stbi_image_free(data);
            }
            stbi_set_flip_vertically_on_load(0);

            gameState->showNewCanvasWindow = false;
    } else {
        printf("No file selected.\n");
    }
}

void saveSpriteSheetToPNG(Renderer *renderer, CanvasTab *canvas, int columns, int rows) {
    DEBUG_TIME_BLOCK()
    char *result = (char *)tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    if(result) {
        // columns = min getArrayLength(canvas->frames) / rows;

        int totalWidth = columns*canvas->w;
        int totalHeight = rows*canvas->h;
        u32 *totalPixels = pushArray(&globalPerFrameArena, totalWidth*totalHeight, u32);

        int x = 0;
        int y = 0;

        for(int i = 0; i < getArrayLength(canvas->frames); ++i) {
            Frame *frame = canvas->frames + i;
            u32 *compositePixels = getPixelsForFrame_shortTerm(canvas, frame);

            for(int k = 0; k < canvas->h; ++k) {
                for(int j = 0; j < canvas->w; ++j) {
                    
                    int tempX = x + j;
                    int tempY = y + k;

                    if (tempX < totalWidth && tempY < totalHeight) {
                        totalPixels[tempY * totalWidth + tempX] = compositePixels[k * canvas->w + j];
                    }
                }
            }
            
            x += canvas->w;

            if(((i + 1) % columns) == 0) {
                y += canvas->h;
                x = 0;
            }
            
        }

        size_t bytesPerPixel = sizeof(uint32_t);
        int stride_in_bytes = bytesPerPixel*totalWidth;

        char *ending = ".png";
        char *extension = getFileExtension((char *)result);

        if(extension && easyString_stringsMatch_nullTerminated(extension, "png")) {
            ending = "";
        }

        char *name = easy_createString_printf(&globalPerFrameArena, "%s%s", (char *)result, ending);
        int writeResult = stbi_write_png(name, totalWidth, totalHeight, 4, totalPixels, stride_in_bytes);
    }
}

void exportImport_loadImageFile(GameState *gameState, const char *filePath) {
    DEBUG_TIME_BLOCK()
    if (filePath) {
            stbi_set_flip_vertically_on_load(1);
            int width = 0;
            int height = 0;
            int nrChannels = 0;
            unsigned char *data = stbi_load(filePath, &width, &height, &nrChannels, STBI_rgb_alpha);

            if(data) {
                char *name = getFileLastPortionWithoutExtension((char *)filePath);
                CanvasTab tab = CanvasTab(width, height, name);
                tab.uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;
                pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
                gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
                

                Canvas *canvas = getActiveCanvas(gameState);

                assert(canvas);
                if(canvas) {
                    easyPlatform_copyMemory(canvas->pixels, data, sizeof(u32)*width*height);
                    updateGpuCanvasTextures(gameState);
                }

                stbi_image_free(data);
            } else {
                addIMGUIToast("Couldn't load image", 2);
            }
            stbi_set_flip_vertically_on_load(0);
    } else {
        printf("No file selected.\n");
    }
}

void checkFileDrop(GameState *gameState) {
    DEBUG_TIME_BLOCK()
    for(int i = 0; i < gameState->droppedFileCount; ++i) {
        char *filePath = gameState->droppedFilePaths[i];
        char *extension = getFileExtension(filePath);
        if(extension && easyString_stringsMatch_nullTerminated(extension, DEFINED_FILE_NAME)) {
            CanvasTab tab = loadPixelrProject(filePath);
            if(tab.valid) {
                tab.uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;
                pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
                gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
            }
        } else {
            exportImport_loadImageFile(gameState, filePath);
        }
    }
}



void openPlainImage(GameState *gameState) {
    DEBUG_TIME_BLOCK()
    const char *filePath = tinyfd_openFileDialog(
        "Open an image",         // Dialog title
        "",                    // Default path
        0,                     // Number of filters
        NULL,        // Filters
        NULL,    // Filter description
        1                      // Allow multiple selection (0 = No, 1 = Yes)
    );

    if(filePath) {

        char *at = (char *)filePath;
        char *start = (char *)filePath;

        bool parsing = true;
        while(parsing) {
            if(!(*at) || *at == '|') {
                char *str = nullTerminateArena(start, (int)(at - start), &globalPerFrameArena);
                exportImport_loadImageFile(gameState, str);

                if(!(*at)) {
                    parsing = false;
                }
                start = at + 1;
            } 
            at++;
        }
    }
    
}

char **getCanvasTabsFileNameOpen(GameState *state, int *len, bool saveFiles, ThreadsInfo *threadInfo = 0) {
    char **fileNames = pushArray(&globalPerFrameArena, getArrayLength(state->canvasTabs), char *);
    int length = 0;
    for(int i = 0; i < getArrayLength(state->canvasTabs); ++i) {
        CanvasTab *t = state->canvasTabs + i;
        // if(!t->undoList->isSentintel && ) 
        {
            char *c = getBackupFileNameForTab((threadInfo) ? 0 : &globalPerFrameArena, t, state->appDataFolderName);
            if(saveFiles) {
                saveProjectToFile(t, c, threadInfo, false);   
            }
            fileNames[length++] = c; 
        }
    }
    *len = length;
    return fileNames;
}

void saveGlobalProjectSettings(void *gameState_) {
    GameState *gameState = (GameState *)gameState_;

    char *fileName = easy_createString_printf(0, "%sglobalProjectSettings", gameState->appDataFolderName);

    game_file_handle fileHandle = platformBeginFileWrite((char *)fileName);
    assert(!fileHandle.HasErrors);
    size_t offset = 0;
    char *strToWrite = "";
    
    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"drawGrid\": %d}\n", gameState->drawGrid);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"inverseZoom\": %d}\n", gameState->inverseZoom);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"activeCanvasTab\": %d}\n", gameState->activeCanvasTab);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"nearest\": %d}\n", gameState->nearest);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"checkBackground\": %d}\n", gameState->checkBackground);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"bgColor\": %f %f %f %f}\n", gameState->bgColor.x, gameState->bgColor.y, gameState->bgColor.z, gameState->bgColor.w);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    float2 windowSize = platform_getWindowSize(); 
    float2 windowPos = platform_getWindowPosition();

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"windowSize\": %d %d %d %d}\n", (int)windowSize.x, (int)windowSize.y, (int)windowPos.x, (int)windowPos.y);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"smoothStrokeCount\": %d}\n", gameState->runningAverageCount);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    platformEndFile(fileHandle);

    easyPlatform_freeMemory(fileName);

}

// void saveGlobalProjectSettings_multiThreaded(GameState *gameState) {
//     //IMPORTANT: NOT SAFE -> it uses the per frame arena
//     pushWorkOntoQueue(&gameState->threadsInfo, saveGlobalProjectSettings, gameState);
// }

void checkAndSaveBackupFile(GameState *gameState) {
    DEBUG_TIME_BLOCK()
    platformThread_assertMainThread();

    time_t now = time(NULL);
    time_t diff = now - gameState->timeOfLastBackup;
    
    if(diff >= BACKUP_FILE_TIME_SECONDS) 
    {
        MemoryPiece *piece = getCurrentMemoryPiece(&globalPerFrameArena);
        MemoryArenaMark mark = takeMemoryMark(&globalPerFrameArena);

        waitForWorkToFinish(&gameState->threadsInfo);
        char **fileNames = pushArray(&globalPerFrameArena, getArrayLength(gameState->canvasTabs), char *);
        int filesCount = 0;
    
        for(int i = 0; i < getArrayLength(gameState->canvasTabs); ++i) {
            CanvasTab *tab = gameState->canvasTabs + i;
            if(canvasTabSaveStateHasChanged(tab)) {
                //NOTE: Update the save position for the backup
                tab->savePositionBackupUndoBlock = tab->undoList;
                char *mallocedFileName = getBackupFileNameForTab(0, tab, gameState->appDataFolderName);
                saveProjectToFile(tab, mallocedFileName, &gameState->threadsInfo, false);
            }

            char *c = getBackupFileNameForTab(&globalPerFrameArena, tab, gameState->appDataFolderName);
            fileNames[filesCount++] = c; 
        }

        
        char *stringAllocated = easy_createString_printf(&globalPerFrameArena, "%sfilesOpen", gameState->appDataFolderName);
        saveGlobalProjectSettings_withFileNames(stringAllocated, fileNames, filesCount, gameState->appDataFolderName);
        
        gameState->canvasTabsOpened = getArrayLength(gameState->canvasTabs);
        gameState->timeOfLastBackup = time(NULL);
        releaseMemoryMark(&mark);
        memoryArena_sameSpot(&globalPerFrameArena, piece);
    } 
}


