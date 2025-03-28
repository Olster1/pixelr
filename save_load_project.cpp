#pragma pack(push, 1) 
struct ProjectFile {
    u32 version;
    u32 brushColor;
    int palletteCount;
    u32 colorsPallete[MAX_PALETTE_COUNT];
};
#pragma pack(pop) 

#define PROJECT_FILE_LATEST_VERSION 1
ProjectFile initProjectFile() {
    ProjectFile result = {};
    result.version = PROJECT_FILE_LATEST_VERSION;
    return result;

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

bool saveProjectFile_(CanvasTab *tab, char *filePath) {
    bool result = false;

    if(tab) {
        ProjectFile data = initProjectFile();
        data.brushColor = float4_to_u32_color(tab->colorPicked);

        data.palletteCount = tab->palletteCount;
        for(int i = 0; i < tab->palletteCount; ++i) {
            data.colorsPallete[i] = tab->colorsPallete[i];
        }
        
        game_file_handle file = platformBeginFileWrite((char *)filePath);
        assert(!file.HasErrors);
        
        size_t offset = platformWriteFile(&file, &data, sizeof(ProjectFile), 0);

        platformEndFile(file);

    }

    return result;

}

void savePalleteDefault_(void *data) {
    CanvasTab *tab = (CanvasTab *)data;
    char *filePath = getPlatformSaveFilePath();

    if(filePath) {
        char *strToWrite = easy_createString_printf(&globalPerFrameArena, "%sdefault.project", (char *)filePath);

        saveProjectFile_(tab, strToWrite);

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
        char *strToWrite = easy_createString_printf(&globalPerFrameArena, "%sdefault.project", (char *)filePath);
        loadProjectFile_(tab, strToWrite);
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

void loadPallete() {
    const char *filterPatterns[] = { "*.palette",};
    const char *filePath = tinyfd_openFileDialog(
        "Open palette",         // Dialog title
        "",                    // Default path
        1,                     // Number of filters
        filterPatterns,        // Filters
        ".palette files only",    // Filter description
        0                      // Allow multiple selection (0 = No, 1 = Yes)
    );    FileContents contents = getFileContentsNullTerminate((char *)filePath);
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