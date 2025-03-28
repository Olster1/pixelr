#pragma pack(push, 1) 
struct ProjectFile {
    u32 version;
    u32 brushColor;
};
#pragma pack(pop) 

#define PROJECT_FILE_LATEST_VERSION 1
ProjectFile initProjectFile() {
    ProjectFile result = {};
    result.version = PROJECT_FILE_LATEST_VERSION;
    return result;

}


bool saveProjectFile(GameState *gameState) {
    bool result = false;

    ProjectFile file = initProjectFile();
    // file.brushColor[0] = gameState->colorPicked.E[0];
    // file.brushColor[1] = gameState->colorPicked.E[1];
    // file.brushColor[2] = gameState->colorPicked.E[2];
    // file.brushColor[3] = gameState->colorPicked.E[3];

    return result;

}

void loadProjectFile(GameState *gameState) {

}

void savePallete(GameState *state) {
    CanvasTab *tab = getActiveCanvasTab(state);
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

void loadPallete(GameState *gameState) {
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