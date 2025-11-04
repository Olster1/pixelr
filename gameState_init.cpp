
void loadDefaultProjectSettings(GameState *gameState) {
    char *fileName = easy_createString_printf(&globalPerFrameArena, "%sglobalProjectSettings", gameState->appDataFolderName);
    if(platformDoesFileExist(fileName)) {
         FileContents contents = getFileContentsNullTerminate((char *)fileName);
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
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                

                if(easyString_stringsMatch_null_and_count("checkBackground", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    gameState->checkBackground = t.intVal;
                }

                if(easyString_stringsMatch_null_and_count("smoothStrokeCount", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    gameState->runningAverageCount = t.intVal;
                }

                if(easyString_stringsMatch_null_and_count("drawGrid", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    gameState->drawGrid = t.intVal;
                }

                if(easyString_stringsMatch_null_and_count("nearest", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    gameState->nearest = t.intVal;
                }

                if(easyString_stringsMatch_null_and_count("bgColor", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_FLOAT);
                    gameState->bgColor.x = t.floatVal;

                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_FLOAT);
                    gameState->bgColor.y = t.floatVal;

                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_FLOAT);
                    gameState->bgColor.z = t.floatVal;

                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_FLOAT);
                    gameState->bgColor.w = t.floatVal;
                    
                }
            }
        }
    }
}

void initGameState(GameState *gameState) {
    srand(time(NULL));
    gameState->randomStartUpID = rand();
    globalRandomStartupSeed = gameState->randomStartUpID;
    
    gameState->scrollSpeed = 0;
    gameState->camera.fov = 5;
    gameState->camera.T.pos = make_float3(0, 0, -10);
    gameState->camera.followingPlayer = false;
    gameState->cameraOffset = CAMERA_OFFSET;
    gameState->camera.shakeTimer = -1;
    gameState->camera.runShakeTimer = -1;

    gameState->bgColor = make_float4(0.3f,0, 0.3f, 1); //u32_to_float4_color(0xFF6D5F72)
    gameState->drawGrid = false;

    stbi_flip_vertically_on_write(1);

    gameState->lastInteractionMode = gameState->interactionMode = CANVAS_DRAW_MODE;
    
    gameState->grabbedCornerIndex = -1;
    gameState->nearest = false; 

    gameState->lastMouseP = gameState->mouseP_screenSpace;
    gameState->runningAverageCount = 1;
    gameState->versionString = "0.0.1";

    Texture atlasTexture = loadTextureToGPU("./images/atlas.png");

    {
        gameState->canvasTabs = initResizeArray(CanvasTab);
        CanvasTab tab = CanvasTab(DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT, 0);
        pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
        gameState->activeCanvasTab = 0;
    }

    gameState->editPaletteIndex = -1;

    gameState->renderer = initRenderer(atlasTexture);

    gameState->mainFont = initFontAtlas("./fonts/Roboto-Regular.ttf");
    
    gameState->renderer->fontAtlasTexture = gameState->mainFont.textureHandle;

    gameState->clipboard = Clipboard();
    gameState->selectMode = false;

    gameState->selectObject = SelectObject();

    initThreadQueue(&gameState->threadsInfo);
    globalThreadInfo = &gameState->threadsInfo;

    GLint maxUniformBlockSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    assert((maxUniformBlockSize / sizeof(float16)) > MAX_BONES_PER_MODEL);

    loadPalleteDefault_(&gameState->canvasTabs[0]);
    loadDefaultProjectSettings(gameState);

    gameState->drawState = EasyProfiler_initProfilerDrawState();

    gameState->inited = true;

}

void checkInitGameState(GameState *gameState) {
    if(!gameState->inited) {
        globalLongTermArena = createArena(Kilobytes(200));
        globalPerFrameArena = createArena(Kilobytes(100));
        perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
        initGameState(gameState);
    } else { 
        releaseMemoryMark(&perFrameArenaMark);
        perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
    }
}