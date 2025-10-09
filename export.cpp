void saveFileToPNG(Frame *frame, CanvasTab *tab) {
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    size_t bytesPerPixel = sizeof(uint32_t);
    int stride_in_bytes = bytesPerPixel*tab->w;

    char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
    int writeResult = stbi_write_png(name, tab->w, tab->h, 4, getCompositePixelsForFrame_shortTerm(tab, frame), stride_in_bytes);
}

void loadPngColorPalette(CanvasTab *tab, bool clearPalette) {
    const char *filterPatterns[] = { "*.png",};
    const char *filePath = tinyfd_openFileDialog(
        "Load Color Palette",         // Dialog title
        "",                    // Default path
        1,                     // Number of filters
        filterPatterns,        // Filters
        "PNG files only",    // Filter description
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
    const char *filterPatterns[] = { "*.png",};
    const char *filePath = tinyfd_openFileDialog(
        "Open an image",         // Dialog title
        "",                    // Default path
        1,                     // Number of filters
        filterPatterns,        // Filters
        "PNG files only",    // Filter description
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

void saveSpriteSheetToPNG(CanvasTab *canvas, int columns, int rows) {
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    // columns = min getArrayLength(canvas->frames) / rows;

    int totalWidth = columns*canvas->w;
    int totalHeight = rows*canvas->h;
    u32 *totalPixels = pushArray(&globalPerFrameArena, totalWidth*totalHeight, u32);

    int x = 0;
    int y = 0;

    for(int i = 0; i < getArrayLength(canvas->frames); ++i) {
        Frame *frame = canvas->frames + i;
        u32 *compositePixels = getCompositePixelsForFrame_shortTerm(canvas, frame);

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

void exportImport_loadPng(GameState *gameState, const char *filePath) {
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
            }
            stbi_set_flip_vertically_on_load(0);
    } else {
        printf("No file selected.\n");
    }
}

void checkFileDrop(GameState *gameState) {
    if(gameState->droppedFilePath) {
        char *extension = getFileExtension(gameState->droppedFilePath);
        if(easyString_stringsMatch_nullTerminated(extension, "png") || easyString_stringsMatch_nullTerminated(extension, "PNG")) {
            exportImport_loadPng(gameState, gameState->droppedFilePath);
        } else if(easyString_stringsMatch_nullTerminated(extension, "pixelr")) {
            CanvasTab tab = loadPixelrProject(gameState->droppedFilePath);
            tab.uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;
            pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
            gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
        }
    }
}



void openPlainImage(GameState *gameState) {
    const char *filterPatterns[] = { "*.png",};
    const char *filePath = tinyfd_openFileDialog(
        "Open an image",         // Dialog title
        "",                    // Default path
        1,                     // Number of filters
        filterPatterns,        // Filters
        "PNG files only",    // Filter description
        0                      // Allow multiple selection (0 = No, 1 = Yes)
    );

    exportImport_loadPng(gameState, filePath);
}