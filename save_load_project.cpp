  /*NOTE: Things to save:
            - color picked
            - palette colors 
            - opacity value (float)
            - checked background (bool)
            - eraser size (float)
            - onion skinnng (int)
            - time per frame (float)
            - copy on frame (bool)
            - active frame (int) 
            - canvas w (int) 
            - canvas h (int)
            - All the frames -> all the canvas per frame (pixels)
            
        */
#define MAX_LAYER_COUNT 256
#define MAX_FRAME_COUNT 256
#pragma pack(push, 1) 
struct ProjectFile {
    u32 version;
    u32 brushColor;
    int palletteCount;
    u32 colorsPallete[MAX_PALETTE_COUNT];
    float opacity;
    bool checkedBackground;
    float eraserSize;
    int onionSkinning;
    float timerPerFrame;
    bool copyOnFrame;
    int canvasW;
    int canvasH;
    int activeFrame;
    int frameCount;
    u32 framesActiveLayer[MAX_FRAME_COUNT];
    u32 canvasCountPerFrame[MAX_FRAME_COUNT];
    size_t canvasesFileOffset;
};
#pragma pack(pop) 

#define PROJECT_FILE_LATEST_VERSION 1
ProjectFile initProjectFile() {
    ProjectFile result = {};
    result.version = PROJECT_FILE_LATEST_VERSION;
    return result;

}

CanvasTab loadPixelrProject(const char *filePath) {
    FileContents file = platformReadEntireFile((char *)filePath, false);
    if(file.valid && file.memory) {
        ProjectFile *data = (ProjectFile *)file.memory;

        assert(data->version == PROJECT_FILE_LATEST_VERSION);

        CanvasTab tab =  CanvasTab(data->canvasW, data->canvasH, (char *)filePath);

        tab.colorPicked = u32_to_float4_color(data->brushColor);
        tab.palletteCount = data->palletteCount;
        for(int i = 0; i < data->palletteCount; ++i) {
            tab.colorsPallete[i] = data->colorsPallete[i];
        }

        PlayBackAnimation *anim = &tab.playback;

        tab.opacity = data->opacity;
        tab.eraserSize = data->eraserSize;
        tab.onionSkinningFrames = data->onionSkinning;
        anim->frameTime = data->timerPerFrame;
        tab.copyFrameOnAdd = data->copyOnFrame;
        tab.activeFrame = data->activeFrame;
        assert(tab.activeFrame < data->frameCount);

        u8 *startOfData = ((u8 *)file.memory) + data->canvasesFileOffset;
        int totalCavases = 0;
        for(int i = 0; i < data->frameCount; ++i) {
            Frame *f = 0;
            if(i == 0) {
                //NOTE: Use the one that's added by defdault when you create a canvasTab
                assert(getArrayLength(tab.frames) == 1);
                f = tab.frames + 0;
            } else {
                Frame f_ = Frame(tab.w, tab.h);
                f = pushArrayItem(&tab.frames, f_, Frame);
            }
            assert(f);
            f->activeLayer = data->framesActiveLayer[i];
            u32 canvasCount = data->canvasCountPerFrame[i];
            for(int j = 0; j < canvasCount; ++j) {
                Canvas *c = 0;
                if(j == 0) {
                    //NOTE: Use the one that's added by defdault when you create a canvasTab
                    assert(getArrayLength(f->layers) == 1);
                    c = f->layers + 0;
                } else {
                    Canvas c_ = Canvas(tab.w, tab.h);
                    c = pushArrayItem(&f->layers, c_, Canvas);
                }
                assert(c);
                size_t dataSize = sizeof(u32)*tab.w*tab.h;
                u8 *pixelData = startOfData + totalCavases*dataSize;                
                easyPlatform_copyMemory(c->pixels, pixelData, dataSize);
                totalCavases++;
            }
        }

        // data->canvasesFileOffset;
        // int frameCount;
        // u32 framesActiveLayer[MAX_FRAME_COUNT];
        // u32 canvasCountPerFrame[MAX_FRAME_COUNT];
        // size_t canvasesFileOffset;

        easyPlatform_freeMemory(file.memory);
        return tab;
    } else {
        //NOTE: Return an empty one
        return CanvasTab(16, 16, 0);
    }
}

CanvasTab loadProjectFromFile(bool *valid) {
    const char *filterPatterns[] = { "*.pixelr",};
    const char *filePath = tinyfd_openFileDialog(
        "Open Project",         // Dialog title
        "",                    // Default path
        1,                     // Number of filters
        filterPatterns,        // Filters
        ".pixelr files only",    // Filter description
        0                      // Allow multiple selection (0 = No, 1 = Yes)
    );    

    if(filePath) {
        CanvasTab tab = loadPixelrProject(filePath);
        return tab;
    } else {
        *valid = false;
        return CanvasTab(32, 32, 0);
    }
    
}

bool loadProjectFile_(CanvasTab *tab, char *filePath) {
    bool result = false;

    if(tab) {
        FileContents file = platformReadEntireFile(filePath, false);
        assert(file.valid);
        if(file.valid && file.memory) {
            ProjectFile *project = (ProjectFile *)file.memory;

            assert(project->version == PROJECT_FILE_LATEST_VERSION);
            tab->colorPicked = u32_to_float4_color(project->brushColor);

            tab->palletteCount = project->palletteCount;
            for(int i = 0; i < project->palletteCount; ++i) {
                tab->colorsPallete[i] = project->colorsPallete[i];
            }

            easyPlatform_freeMemory(file.memory);
        }
    }
    return result;
}

struct SaveProjectFileThreadData {
    CanvasTab *tab; 
    char *filePath;
    bool replaceSaveFilePath;
};

bool saveProjectFile_(CanvasTab *tab, char *filePath, bool replaceSaveFilePath) {
    bool result = false;

    if(tab) {
        /*NOTE: Things to save:
            - color picked
            - palette colors 
            - opacity value (float)
            - checked background (bool)
            - eraser size (float)
            - onion skinnng (int)
            - time per frame (float)
            - copy on frame (bool)
            - active frame (int) 
            - canvas w (int) 
            - canvas h (int)
            - All the frames -> all the canvas per frame (pixels)
            
        */
        ProjectFile data = initProjectFile();
        data.brushColor = float4_to_u32_color(tab->colorPicked);

        data.palletteCount = tab->palletteCount;
        assert(data.palletteCount < MAX_PALETTE_COUNT);
        for(int i = 0; i < tab->palletteCount; ++i) {
            data.colorsPallete[i] = tab->colorsPallete[i];
        }

        PlayBackAnimation *anim = &tab->playback;

        data.opacity = tab->opacity;
        data.eraserSize = tab->eraserSize;
        data.onionSkinning = tab->onionSkinningFrames;
        data.timerPerFrame = anim->frameTime;
        data.copyOnFrame = tab->copyFrameOnAdd;
        data.canvasW = tab->w;
        data.canvasH = tab->h;
        data.activeFrame = tab->activeFrame;
        data.canvasesFileOffset = sizeof(ProjectFile);

        data.frameCount = getArrayLength(tab->frames);
        for(int i = 0; i < getArrayLength(tab->frames); ++i) {
            Frame *f = tab->frames + i;
            if(f) {
                assert(i < MAX_FRAME_COUNT);
                data.framesActiveLayer[i] = f->activeLayer;
                data.canvasCountPerFrame[i] = getArrayLength(f->layers);
            }
        }

        game_file_handle file = platformBeginFileWrite((char *)filePath);
        assert(!file.HasErrors);
        
        size_t offset = platformWriteFile(&file, &data, sizeof(ProjectFile), 0);
        for(int i = 0; i < getArrayLength(tab->frames); ++i) {
            Frame *f = tab->frames + i;
            if(f) {
                for(int j = 0; j < getArrayLength(f->layers); ++j) {
                    Canvas *c = f->layers + j;
                    if(c) {
                        size_t dataSize = tab->w*tab->h*sizeof(u32);
                        offset = platformWriteFile(&file, c->pixels, dataSize, offset);
                    }
                }
            }
        }

        platformEndFile(file);

        if(replaceSaveFilePath) {
            //NOTE: Update the save state to say the project is now saved
            tab->savePositionUndoBlock = tab->undoList;
            
            //NOTE: If there is no save file path, now add it to save in the future
            if(!tab->saveFilePath) {
                tab->saveFilePath = easyString_copyToHeap(filePath);
                tab->fileName = getFileLastPortion_allocateToHeap(tab->saveFilePath);
            }
        }
    }

    return result;

}

void saveProjectFile_threadData(void *data_) {
    SaveProjectFileThreadData *data = (SaveProjectFileThreadData *)data_;

    CanvasTab *tab = data->tab;
    char *filePath = data->filePath;
    bool replaceFilePath = data->replaceSaveFilePath;

    saveProjectFile_(tab, filePath, replaceFilePath);

    easyPlatform_freeMemory(data);
}


void saveProjectToFile(CanvasTab *tab, char *optionalFilePath = 0, ThreadsInfo *threadsInfo = 0, bool replaceSaveFilePath = true) {
    char *strToWrite = 0;

    if(optionalFilePath) {
        strToWrite = optionalFilePath;
    } else if(tab->saveFilePath) {
        strToWrite = tab->saveFilePath;
    } else {
        char const *fileName = tinyfd_saveFileDialog (
        "Save File",
        "",
        0,
        NULL,
        NULL);

        if(fileName) {
            strToWrite = easy_createString_printf(&globalPerFrameArena, "%s.pixelr", (char *)fileName);
        }
    }
    
    if(strToWrite) {
        if(threadsInfo) {
            MemoryBarrier();
            ReadWriteBarrier();

            SaveProjectFileThreadData *data = (SaveProjectFileThreadData *)easyPlatform_allocateMemory(sizeof(SaveProjectFileThreadData));
            data->tab = tab; 
            data->filePath = strToWrite;
            data->replaceSaveFilePath = replaceSaveFilePath;

            //NOTE: Multi-threaded
            pushWorkOntoQueue(threadsInfo, saveProjectFile_threadData, data);
        } else {
            saveProjectFile_(tab, strToWrite, replaceSaveFilePath);
        }

        tab->uiTabSelectedFlag = ImGuiTabItemFlags_SetSelected;
    }
}

void saveProjectToFileBackup_multiThreaded(char *appDataFolder, CanvasTab *tab, ThreadsInfo *threadsInfo) {
#if _WIN32
//TODO: Add time function
#else 
    time_t t = time(NULL);
#endif
    char *saveFileName = "Untitled";

    if(tab->fileName) {
        saveFileName = tab->fileName;
    }
    
    char *uniqueFilePath = easy_createString_printf(&globalPerFrameArena, "%s%ld_%s_backup.pixelr", appDataFolder, t, saveFileName);
    
    saveProjectToFile(tab, uniqueFilePath, threadsInfo, false);
}

void savePalleteDefault_(void *data) {
    CanvasTab *tab = (CanvasTab *)data;
    char *filePath = getPlatformSaveFilePath();

    if(filePath) {
        char *strToWrite = easy_createString_printf(&globalPerFrameArena, "%sdefault.project", (char *)filePath);

        saveProjectFile_(tab, strToWrite, false);

        easyPlatform_freeMemory(filePath);
    }
}

void savePalleteDefaultThreaded(ThreadsInfo *threadsInfo, CanvasTab *tab) {
    MemoryBarrier();
    ReadWriteBarrier();

    //NOTE: Multi-threaded
    pushWorkOntoQueue(threadsInfo, savePalleteDefault_, tab);
}

void loadPalleteDefault_(void *data) {
    CanvasTab *tab = (CanvasTab *)data;
    char *filePath = getPlatformSaveFilePath();

    if(filePath) {
        char *strToLoad = easy_createString_printf(&globalPerFrameArena, "%sdefault.project", (char *)filePath);
        if(platformDoesFileExist(strToLoad)) {
            loadProjectFile_(tab, strToLoad);
        }
        easyPlatform_freeMemory(filePath);
    }
}

void loadPalleteDefaultThreaded(ThreadsInfo *threadsInfo, CanvasTab *tab) {
    MemoryBarrier();
    ReadWriteBarrier();

    //NOTE: Multi-threaded
    pushWorkOntoQueue(threadsInfo, loadPalleteDefault_, tab);
}

void savePallete(CanvasTab *tab) {
    if(tab) {
        char const *fileName = tinyfd_saveFileDialog (
            "Save File",
            "",
            0,
            NULL,
            NULL);

        if(fileName) {

            char *strToWrite = easy_createString_printf(&globalPerFrameArena, "%s.palette", (char *)fileName);
            game_file_handle atlasJsonFile = platformBeginFileWrite((char *)strToWrite);
            assert(!atlasJsonFile.HasErrors);
            size_t offset = 0;
            
            for(int i = 0 ; i < tab->palletteCount; i++) {
                u32 c = tab->colorsPallete[i];

                strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"color\": %u}\n", c);
                offset = platformWriteFile(&atlasJsonFile, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);
            }

            strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"palletteCount\": %d}\n", tab->palletteCount);
            offset = platformWriteFile(&atlasJsonFile, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

            platformEndFile(atlasJsonFile);
        }
    }
}

void loadPallete() {
    const char *filterPatterns[] = { "*.palette",};
    const char *filePath = tinyfd_openFileDialog(
        "Open palette",         // Dialog title
        "",                    // Default path
        1,                     // Number of filters
        filterPatterns,        // Filters
        ".palette files only",    // Filter description
        0                      // Allow multiple selection (0 = No, 1 = Yes)
    );    
    if(filePath) {
    
        FileContents contents = getFileContentsNullTerminate((char *)filePath);
        assert(contents.valid);
        assert(contents.fileSize > 0);
        assert(contents.memory);

        EasyTokenizer tokenizer = lexBeginParsing(contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

        bool parsing = true;
        while(parsing) {
            EasyToken t = lexGetNextToken(&tokenizer);

            if(t.type == TOKEN_NULL_TERMINATOR) {
                parsing = false;
            } else if(t.type == TOKEN_OPEN_BRACKET) {
                //NOTE: Get the item out
                u32 color = 0;

                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);

                if(easyString_stringsMatch_null_and_count("color", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    color = t.intVal;

                    //NOTE: Add the color
                }
            }
        }
    }
}