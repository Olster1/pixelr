

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
    gameState->nearest = true; 

    gameState->lastMouseP = gameState->mouseP_screenSpace;
    gameState->runningAverageCount = 1;

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

    gameState->drawBlocks = false;

    initThreadQueue(&gameState->threadsInfo);
    globalThreadInfo = &gameState->threadsInfo;

    GLint maxUniformBlockSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    assert((maxUniformBlockSize / sizeof(float16)) > MAX_BONES_PER_MODEL);

    gameState->useCameraMovement = false;

    loadPalleteDefault_(&gameState->canvasTabs[0]);

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