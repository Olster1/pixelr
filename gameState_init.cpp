

void initGameState(GameState *gameState) {
    srand(time(NULL));
    gameState->randomStartUpID = rand();
    globalRandomStartupSeed = gameState->randomStartUpID;
    
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

    gameState->bgColor = make_float4(0.3f,0, 0.3f, 1); //u32_to_float4_color(0xFF6D5F72)
    gameState->drawGrid = false;

    stbi_flip_vertically_on_write(1);

    gameState->currentInventoryHotIndex = 0;

    createBlockFlags(gameState);
    memset(gameState->chunks, 0, arrayCount(gameState->chunks)*sizeof(Chunk *));

    gameState->interactionMode = CANVAS_DRAW_MODE;
    
    
    gameState->entitiesToAddCount = 0;

    gameState->timeOfDay = 0.4f;
    
    initPlayer(&gameState->player, gameState->randomStartUpID);
    gameState->player.T.pos = gameState->camera.T.pos;

    gameState->physicsWorld.positionCorrecting = true;
    gameState->physicsWorld.warmStarting = true;
    gameState->physicsWorld.accumulateImpulses = true;
    gameState->grabbedCornerIndex = -1;
    gameState->nearest = true; 

    gameState->lastMouseP = gameState->mouseP_screenSpace;
    gameState->runningAverageCount = 1;

    gameState->grassTexture = loadTextureToGPU("./images/grass_block.png");
    Texture breakBlockTexture = loadTextureToGPU("./images/break_block.png");
    Texture atlasTexture = loadTextureToGPU("./images/atlas.png");

    {
        gameState->canvasTabs = initResizeArray(CanvasTab);
        CanvasTab tab = CanvasTab(16, 16, 0);
        pushArrayItem(&gameState->canvasTabs, tab, CanvasTab);
        gameState->activeCanvasTab = 0;
    }

    gameState->editPaletteIndex = -1;

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


    gameState->clipboard = Clipboard();
    gameState->selectMode = false;

    gameState->selectObject = SelectObject();

    gameState->drawBlocks = false;

    createAOOffsets(gameState);
    createCardinalDirections(gameState);

    gameState->particlerCount = 0;

    initThreadQueue(&gameState->threadsInfo);
    globalThreadInfo = &gameState->threadsInfo;

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

    loadPalleteDefault_(&gameState->canvasTabs[0]);

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