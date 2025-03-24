struct EntityChunkInfo {
    EntityID entityID;
    Chunk *chunk;
};

struct InventoryItem {
    BlockType type;
    int count;
};

struct AOOffset {
    float3 offsets[3];
};

struct ChunkVertexToCreate {
    VertexForChunk *triangleData;
    u32 *indicesData;

    VertexForChunk *alphaTriangleData;
    u32 *alphaIndicesData;

    Chunk *chunk;

    ChunkVertexToCreate *next;
    bool ready;
};

enum BlockFlags {
    BLOCK_FLAGS_NONE = 0,
    BLOCK_EXISTS_COLLISION = 1 << 0,
    BLOCK_EXISTS = 1 << 1, //NOTE: All blocks have this
    BLOCK_FLAG_STACKABLE = 1 << 2, //NOTE: Whether you can put a block ontop of this one
    BLOCK_NOT_PICKABLE = 1 << 3, //NOTE: Whether it destroys without dropping itself to be picked up i.e. grass just gets destroyed.
    BLOCK_FLAGS_NO_MINE_OUTLINE = 1 << 4, //NOTE: Whether it shows the mining outline
    BLOCK_FLAGS_AO = 1 << 5, //NOTE: Whether it shows the mining outline
    BLOCK_FLAGS_UNSAFE_UNDER = 1 << 6, //NOTE: Whether the block should be destroyed if underneath block destroyed
    BLOCK_FLAGS_MINEABLE = 1 << 7, 

};

enum CanvasInteractionMode {
    CANVAS_DRAW_MODE,
    CANVAS_MOVE_MODE,
    CANVAS_FILL_MODE,
    CANVAS_DRAW_CIRCLE_MODE,
    CANVAS_DRAW_LINE_MODE,
    CANVAS_DRAW_RECTANGLE_MODE,
    CANVAS_ERASE_MODE,
    CANVAS_SELECT_RECTANGLE_MODE,
    CANVAS_MOVE_SELECT_MODE,
    CANVAS_SPRAY_CAN,


};

#define MAX_CANVAS_DIM 4000
#define CHUNK_LIST_SIZE 4096*4


struct GameState {
    bool inited;
    float dt;
    float screenWidth;
    float aspectRatio_y_over_x;

    TextureAtlas spriteTextureAtlas;

    Interaction currentInteraction;

    float2 mouseP_screenSpace;
    float2 mouseP_01;

    int mouseIndexAt;
    int mouseCountAt;
    float2 mouseP_01_array[100];
    float2 lastMouseP;

    int particlerCount;
    Particler particlers[512];
    
    Font mainFont;

    CanvasInteractionMode interactionMode;

    Texture grassTexture;
    Texture soilTexture;
    Texture circleTexture;
    Texture circleOutlineTexture;

    ChunkInfo *chunksList;
    
    MouseKeyState mouseLeftBtn;

    Block *currentMiningBlock;
    float placeBlockTimer;
    float mineBlockTimer;
    float showCircleTimer;

    WavFile cardFlipSound[2];

    int currentInventoryHotIndex;

    //NOTE: First 8 are what player as equipped in the hotbar
    int inventoryCount;
    InventoryItem playerInventory[64];

    float sprayTimeAt;

    float3 cameraOffset;

    ChunkVertexToCreate *meshesToCreate;
    ChunkVertexToCreate *meshesToCreateFreeList;

    WavFile blockBreakSound;
    WavFile blockFinishSound;
    WavFile bgMusic;
    WavFile fallBigSound;
    WavFile pickupSound;

    float16 cameraRotation;

    float3 startP;
    int grabbedCornerIndex;

    float scrollDp; //NOTE: velcoity to intergrate for the scroll speed
    float2 canvasMoveDp; //NOTE: velcoity to intergrate for the move canvas speed
    bool nearest;

    ThreadsInfo threadsInfo;

    PlayingSound *miningSoundPlaying;

    //NOTE: linked hash maps
    Chunk *chunks[CHUNK_LIST_SIZE];
    CloudChunk *cloudChunks[CHUNK_LIST_SIZE];

    Renderer *renderer;

    SDL_AudioSpec audioSpec;
    bool checkBackground;

    Camera camera;

    float timeOfDay; //NOTE: 0 - 1 is one day

    int randomStartUpID;

    Entity player;

    #define MAX_ENTITY_COUNT 1024
    int entityCount;
    Entity *entitiesForFrame[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk
    EntityChunkInfo entitiesForFrameChunkInfo[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk

    int entitiesToAddCount;
    Entity entitiesToAddAfterFrame[MAX_ENTITY_COUNT]; //NOTE: Point to the entities stored on the chunk

    int entityToDeleteCount;
    int entitiesToDelete[MAX_ENTITY_COUNT];

    KeyStates keys;

    AOOffset aoOffsets[24];
    float3 cardinalOffsets[6];

    int DEBUG_BlocksDrawnForFrame;

    bool drawBlocks;

    PhysicsWorld physicsWorld;

    float3 searchOffsets[26];
    float3 searchOffsetsSmall[6];

    Texture perlinTestTexture;

    bool useCameraMovement;
    float3 perlinNoiseValue;

    int voxelEntityCount;
    VoxelEntity voxelEntities[64];
    float physicsAccum;

    VoxelEntity *grabbed;

    uint64_t blockFlags[BLOCK_TYPE_COUNT];

    SkeletalModel foxModel;
    float3 modelLocation;


    float4 colorPicked;
    float4 bgColor;
    char dimStr0[256];
    char dimStr1[256];
    // u32 canvas[MAX_CANVAS_DIM*MAX_CANVAS_DIM]; 
    // int canvasW;
    // int canvasH;
    bool showNewCanvasWindow;
    bool showExportWindow;
    bool maxColumnsExport;
    float scrollSpeed;
    bool draggingCanvas;
    float2 startDragP;
    bool paintActive;
    float2 lastPaintP;
    bool drawGrid;
    float opacity;
    float2 drawShapeStart;
    bool drawingShape;
    float eraserSize;
    bool autoFocus;
    bool selectMode;
    int onionSkinningFrames;
    bool blueRedFlippedInUI;
    bool copyFrameOnAdd;
    int runningAverageCount;

    SelectObject selectObject;

    CanvasTab *canvasTabs; //NOTE: resize array
    int activeCanvasTab;

    int palletteCount;
    int palletteIndexAt;
    int colorsPalletePicked;
    float4 colorsPallete[256];
    bool showColorPalleteEnter;

    char colorsPalleteBuffer[10000];

    Clipboard clipboard;

    bool quit;
};

void createBlockFlags(GameState *gameState) {
    for(int i = 0; i < arrayCount(gameState->blockFlags); ++i) {
        uint64_t flags = BLOCK_EXISTS | BLOCK_FLAGS_AO | BLOCK_FLAG_STACKABLE | BLOCK_EXISTS_COLLISION | BLOCK_FLAGS_MINEABLE;
        BlockType t = (BlockType)i;
        switch(t) {
            case BLOCK_WATER: {
                flags = flags | BLOCK_FLAGS_NONE | BLOCK_FLAGS_NO_MINE_OUTLINE;
                flags &= ~(BLOCK_FLAGS_AO | BLOCK_EXISTS_COLLISION | BLOCK_FLAGS_MINEABLE);
            } break;
            case BLOCK_GRASS_SHORT_ENTITY:
            case BLOCK_GRASS_TALL_ENTITY: {
                flags = (BlockFlags)(flags | BLOCK_NOT_PICKABLE | BLOCK_FLAGS_NO_MINE_OUTLINE | BLOCK_FLAGS_UNSAFE_UNDER);
                flags &= ~(BLOCK_FLAGS_AO | BLOCK_FLAG_STACKABLE | BLOCK_EXISTS_COLLISION);
            } break;
            case BLOCK_TREE_LEAVES: {
                flags = (BlockFlags)(flags | BLOCK_EXISTS_COLLISION | BLOCK_FLAGS_NO_MINE_OUTLINE);
                flags &= ~(BLOCK_FLAGS_AO);
            } break;
            case BLOCK_NONE: {
                flags = 0;
            } break;
            default: {
                
            };
        }
        gameState->blockFlags[i] = flags;
    }
}

void createCardinalDirections(GameState *gameState) {
    gameState->cardinalOffsets[0] = make_float3(0, 1, 0);
    gameState->cardinalOffsets[1] = make_float3(0, -1, 0);
    gameState->cardinalOffsets[2] = make_float3(0, 0, 1);
    gameState->cardinalOffsets[3] = make_float3(0, 0, -1);
    gameState->cardinalOffsets[4] = make_float3(-1, 0, 0);
    gameState->cardinalOffsets[5] = make_float3(1, 0, 0);

}

void createAOOffsets(GameState *gameState) {
    for(int i = 0; i < arrayCount(global_cubeData); ++i) {
        assert(i < arrayCount(gameState->aoOffsets));

        Vertex v = global_cubeData[i];
        float3 normal = v.normal;
        float3 sizedOffset = scale_float3(2, v.pos);

        float3 masks[2] = {};
        int maskCount = 0;

        for(int k = 0; k < 3; k++) {
            if(normal.E[k] == 0) {
                assert(maskCount < arrayCount(masks));
                float3 m = make_float3(0, 0, 0);
                m.E[k] = -sizedOffset.E[k];

                masks[maskCount++] = m;
            }
        }

        gameState->aoOffsets[i].offsets[0] = plus_float3(sizedOffset, masks[0]);
        gameState->aoOffsets[i].offsets[1] = sizedOffset; 
        gameState->aoOffsets[i].offsets[2] = plus_float3(sizedOffset, masks[1]);
    }
}

void createSearchOffsets(GameState *gameState) {
    int index = 0;
    for(int z = -1; z <= 1; z++) {
        for(int y = -1; y <= 1; y++) {
            for(int x = -1; x <= 1; x++) {
                if(x == 0 && y == 0 && z == 0) {
                    continue;
                } else {
                    gameState->searchOffsets[index++] = make_float3(x, y, z);
                }

            }
        }
    }
    assert(index == 26);

    gameState->searchOffsetsSmall[0] = make_float3(1, 0, 0); 
    gameState->searchOffsetsSmall[0] = make_float3(0, 0, 1); 
    gameState->searchOffsetsSmall[0] = make_float3(-1, 0, 0); 
    gameState->searchOffsetsSmall[0] = make_float3(0, 0, -1); 

    gameState->searchOffsetsSmall[0] = make_float3(0, 1, 0); 
    gameState->searchOffsetsSmall[0] = make_float3(0, -1, 0); 
}
