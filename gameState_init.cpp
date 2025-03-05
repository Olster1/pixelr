

void startCanvas(GameState *gameState) {
    // addUndoRedoBlock(gameState, 0, 0, -1, -1, true);

    // for(int y = 0; y < gameState->canvasH; ++y) {
    //     for(int x = 0; x < gameState->canvasW; ++x) {
    //         gameState->canvas[y*gameState->canvasW + x] = 0x00FFFFFF;
    //     }
    // }
}

void initGameState(GameState *gameState) {
    srand(time(NULL));
    gameState->randomStartUpID = rand();
    gameState->voxelEntities[gameState->voxelEntityCount++] = createVoxelCircleEntity(1.0f, make_float3(0, 0, 0), 1.0f, gameState->randomStartUpID);
    gameState->voxelEntities[gameState->voxelEntityCount++] = createVoxelSquareEntity(1, 1, make_float3(0, 2, 0), 1.0f, gameState->randomStartUpID);
    gameState->voxelEntities[gameState->voxelEntityCount++] = createVoxelSquareEntity(1, 1, make_float3(0, 4, 0), 1.0f, gameState->randomStartUpID);
    gameState->voxelEntities[gameState->voxelEntityCount++] = createVoxelPlaneEntity(100.0f, make_float3(0, -3, 0), 0, gameState->randomStartUpID);
    // gameState->grabbed = &gameState->voxelEntities[2]; 
    
    gameState->scrollSpeed = 0;
    assert(BLOCK_TYPE_COUNT < 255);
    gameState->camera.fov = 5;
    gameState->camera.T.pos = make_float3(0, 0, -10);
    gameState->camera.followingPlayer = false;
    gameState->cameraOffset = CAMERA_OFFSET;
    gameState->camera.shakeTimer = -1;
    gameState->camera.runShakeTimer = -1;

    gameState->colorPicked = make_float4(1, 0, 0, 1);
    gameState->bgColor = make_float4(0.3f,0, 0.3f, 1); //make_float4(1, 1, 1, 1); //
    gameState->drawGrid = false;
    gameState->opacity = 0.5f;
    gameState->eraserSize = 1.0f;

    stbi_flip_vertically_on_write(1);

    //NOTE: The sentinel block
    startCanvas(gameState);

    gameState->currentInventoryHotIndex = 0;

    createBlockFlags(gameState);
    memset(gameState->chunks, 0, arrayCount(gameState->chunks)*sizeof(Chunk *));

    gameState->interactionMode = CANVAS_DRAW_MODE;
    gameState->checkBackground = true;
    
    gameState->entitiesToAddCount = 0;

    gameState->timeOfDay = 0.4f;
    
    initPlayer(&gameState->player, gameState->randomStartUpID);
    gameState->player.T.pos = gameState->camera.T.pos;

    gameState->physicsWorld.positionCorrecting = true;
    gameState->physicsWorld.warmStarting = true;
    gameState->physicsWorld.accumulateImpulses = true;

    loadWavFile(&gameState->cardFlipSound[0], "./sounds/cardFlip.wav", &gameState->audioSpec);
    loadWavFile(&gameState->cardFlipSound[1], "./sounds/cardFlip1.wav", &gameState->audioSpec);
    loadWavFile(&gameState->blockBreakSound, "./sounds/blockBreak.wav", &gameState->audioSpec);
    loadWavFile(&gameState->blockFinishSound, "./sounds/blockFinish.wav", &gameState->audioSpec);
    loadWavFile(&gameState->fallBigSound, "./sounds/fallbig.wav", &gameState->audioSpec);
    loadWavFile(&gameState->pickupSound, "./sounds/pop.wav", &gameState->audioSpec);
    // loadWavFile(&gameState->bgMusic, "./sounds/sweeden.wav", &gameState->audioSpec);

    gameState->lastMouseP = gameState->mouseP_screenSpace;

    gameState->grassTexture = loadTextureToGPU("./images/grass_block.png");
    Texture breakBlockTexture = loadTextureToGPU("./images/break_block.png");
    Texture atlasTexture = loadTextureToGPU("./images/atlas.png");

    {
        gameState->canvasTabs = initResizeArray(CanvasTab);
        CanvasTab tab = CanvasTab(16, 16, easyString_copyToHeap("Untitled"));
        pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
        gameState->activeCanvasTab = 0;
    }

    if(!gameState->playBackAnimation.frameTextures) {
        gameState->playBackAnimation.frameTextures = initResizeArray(unsigned int);
  
        pushArrayItem(&gameState->playBackAnimation.frameTextures, gameState->grassTexture.handle, u32);
        pushArrayItem(&gameState->playBackAnimation.frameTextures, gameState->grassTexture.handle, u32);
    }
    gameState->playBackAnimation.frameTime = 0.2f;

    gameState->currentMiningBlock = 0;

    gameState->meshesToCreate = 0;
    gameState->meshesToCreateFreeList = 0;

    gameState->renderer = initRenderer(gameState->grassTexture, breakBlockTexture, atlasTexture);

    gameState->mainFont = initFontAtlas("./fonts/Roboto-Regular.ttf");
    
    gameState->renderer->fontAtlasTexture = gameState->mainFont.textureHandle;

    gameState->placeBlockTimer = -1;
    gameState->mineBlockTimer = -1;
    gameState->showCircleTimer = -1;

    gameState->inventoryCount = 0;
    gameState->entityToDeleteCount = 0;

    playSound(&gameState->bgMusic);

    

    gameState->drawBlocks = false;

    createAOOffsets(gameState);
    createCardinalDirections(gameState);

    gameState->particlerCount = 0;

    initThreadQueue(&gameState->threadsInfo);

    gameState->spriteTextureAtlas = readTextureAtlas("./texture_atlas.json", "./texture_atlas.png");
    
    GLint maxUniformBlockSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    assert((maxUniformBlockSize / sizeof(float16)) > MAX_BONES_PER_MODEL);
    
    createSearchOffsets(gameState);

    gameState->perlinTestTexture = createGPUTexture(PERLIN_SIZE, PERLIN_SIZE, 0);

    gameState->useCameraMovement = false;
    gameState->perlinNoiseValue.x = 0.5f;
    gameState->perlinNoiseValue.y = 0.5f;
    gameState->perlinNoiseValue.z = 0.5f;

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