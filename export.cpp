void saveFileToPNG(Canvas *canvas) {
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    size_t bytesPerPixel = sizeof(uint32_t);
    int stride_in_bytes = bytesPerPixel*canvas->w;

    char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
    int writeResult = stbi_write_png(name, canvas->w, canvas->h, 4, canvas->pixels, stride_in_bytes);
}

void saveFileToPNG() {
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    stbi_set_flip_vertically_on_load(1);

    int totalWidth = 896;
    int totalHeight = 512;

    size_t bytesPerPixel = sizeof(uint32_t);
    int stride_in_bytes = bytesPerPixel*totalWidth;

    u32 *data = (u32 *)pushArray(&globalPerFrameArena, totalWidth*totalHeight, u32);

    int widthA;
    int heightA;
    int nrChannelsA;
    int widthB;
    int heightB;
    int nrChannelsB;

    u32 *a = (u32 *)stbi_load("./images/tileMapFlat.png", &widthA, &heightA, &nrChannelsA, STBI_rgb_alpha);
    u32 *b = (u32 *)stbi_load("./images/tileMapElevation.png", &widthB, &heightB, &nrChannelsB, STBI_rgb_alpha);

    for(int y = 0; y < heightA; ++y) {
        for(int x = 0; x < widthA; ++x) {
            data[(totalHeight - y)*totalWidth + x] = a[(heightA - y)*widthA + x];
        }
    }

    for(int y = 0; y < heightB; ++y) {
        for(int x = 0; x < widthB; ++x) {
            int index = y*totalWidth + (x + widthA);
            data[index] = b[y*widthB + x];
        }
    }

    char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
    int writeResult = stbi_write_png(name, totalWidth, totalHeight, 4, data, stride_in_bytes);

    stbi_set_flip_vertically_on_load(0);
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
        Canvas *c = &frame->layers[0];

        for(int k = 0; k < canvas->h; ++k) {
            for(int j = 0; j < canvas->w; ++j) {
                
                int tempX = x + j;
                int tempY = y + k;

                if (tempX < totalWidth && tempY < totalHeight) {
                    totalPixels[tempY * totalWidth + tempX] = c->pixels[k * canvas->w + j];
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

    if (filePath) {
            stbi_set_flip_vertically_on_load(1);
            int width = 0;
            int height = 0;
            int nrChannels = 0;
            unsigned char *data = stbi_load(filePath, &width, &height, &nrChannels, STBI_rgb_alpha);

            if(data) {
                char *name = getFileLastPortionWithoutExtension((char *)filePath);
                CanvasTab tab = CanvasTab(width, height, name);
                pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
                gameState->activeCanvasTab = getArrayLength(gameState->canvasTabs) - 1;
                printf("%d\n", gameState->activeCanvasTab);

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

    // size_t bytesPerPixel = sizeof(uint32_t);
    // int stride_in_bytes = bytesPerPixel*canvas->w;

    // char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
}