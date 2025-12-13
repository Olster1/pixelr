void saveFileToPNG(Renderer *renderer, Frame *frame, CanvasTab *tab) {
    DEBUG_TIME_BLOCK()
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    if(result) {
        size_t bytesPerPixel = sizeof(uint32_t);
        int stride_in_bytes = bytesPerPixel*tab->w;

        updateFrameGPUHandles(frame, tab);

        char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
        int writeResult = stbi_write_png(name, tab->w, tab->h, 4, getPixelsForFrame_shortTerm(tab, frame), stride_in_bytes);
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
    char const * result = tinyfd_saveFileDialog (
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


        char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
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
        if(easyString_stringsMatch_nullTerminated(extension, DEFINED_APP_NAME)) {
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

void checkAndSaveBackupFile(GameState *gameState, CanvasTab *tab) {
    DEBUG_TIME_BLOCK()
    tab->secondsSinceLastBackup += gameState->dt; 
    if(tab->secondsSinceLastBackup >= BACKUP_FILE_TIME_SECONDS && canvasTabSaveStateHasChanged(tab)) {
        //NOTE: Update the save position for the backup
        tab->savePositionBackupUndoBlock = tab->undoList;
        saveProjectToFileBackup_multiThreaded(gameState->appDataFolderName, tab, &gameState->threadsInfo);    
        tab->secondsSinceLastBackup = 0;
    }
}


void saveGlobalProjectSettings(GameState *gameState, char **filePaths = 0, int filePathsCount = 0) {
     char *fileName = easy_createString_printf(&globalPerFrameArena, "%sglobalProjectSettings", gameState->appDataFolderName);
    game_file_handle fileHandle = platformBeginFileWrite((char *)fileName);
    assert(!fileHandle.HasErrors);
    size_t offset = 0;
    char *strToWrite = "";
    
    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"drawGrid\": %d}\n", gameState->drawGrid);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"nearest\": %d}\n", gameState->nearest);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"checkBackground\": %d}\n", gameState->checkBackground);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"bgColor\": %f %f %f %f}\n", gameState->bgColor.x, gameState->bgColor.y, gameState->bgColor.z, gameState->bgColor.w);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"smoothStrokeCount\": %d}\n", gameState->runningAverageCount);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"filesReopenFilePaths\": [", gameState->drawGrid);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    for(int i = 0; i < filePathsCount; ++i) {
        strToWrite = easy_createString_printf(&globalPerFrameArena, "\"%s\" ", filePaths[i]);
        offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);
    }

    strToWrite = easy_createString_printf(&globalPerFrameArena, "]}\n", gameState->drawGrid);
    offset = platformWriteFile(&fileHandle, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    platformEndFile(fileHandle);

}