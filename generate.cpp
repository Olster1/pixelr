//NOTE: These functions are used Multi threaded
struct FillChunkData {
    GameState *gameState;
    Chunk *chunk;
};

struct GenerateMeshData {
    GameState *gameState;
    ChunkVertexToCreate *info;
};


void getAOMask_multiThreaded(void *data_);

void addBlock(GameState *gameState, float3 worldP, BlockType type) {
    int chunkX = roundChunkCoord(worldP.x / (float)CHUNK_DIM);
    int chunkY = roundChunkCoord(worldP.y / (float)CHUNK_DIM);
    int chunkZ = roundChunkCoord(worldP.z / (float)CHUNK_DIM);

    Chunk *c = getChunkNoGenerate(gameState, chunkX, chunkY, chunkZ);
    // assert(c);
    //NOTE: the chunk might not exist yet so trees might be cut off if the chunk hasn't been generated yet. If they were based on perlin noise this wouldn't be the case.
   
    if(c) {
        if(!c->blocks) {
            c->blocks = (Block *)easyPlatform_allocateMemory(BLOCKS_PER_CHUNK*sizeof(Block), EASY_PLATFORM_MEMORY_ZERO);
        }
        int localX = worldP.x - (CHUNK_DIM*chunkX); 
        int localY = worldP.y - (CHUNK_DIM*chunkY); 
        int localZ = worldP.z - (CHUNK_DIM*chunkZ); 

        int blockIndex = getBlockIndex(localX, localY, localZ);
        assert(blockIndex < BLOCKS_PER_CHUNK);
        if(blockIndex < BLOCKS_PER_CHUNK) {
            c->blocks[blockIndex] = spawnBlock(localX, localY, localZ, type);
        } else {
            assert(false);
        }
    } 
}

#include "./perlin_noise_values.cpp"

void generateTree_multiThread(GameState *gameState, Chunk *chunk, float3 worldP) {
    int treeHeight = (int)(3*((float)rand() / (float)RAND_MAX)) + 3;

    for(int i = 0; i < treeHeight; ++i) {
        float3 p = plus_float3(worldP, make_float3(0, i, 0));
        //NOTE: Add block
        addBlock(gameState, p, BLOCK_TREE_WOOD);
    }

    int z = 0;
    int x = 0;

    float3 p = plus_float3(worldP, make_float3(x, (treeHeight + 1), z));
    addBlock(gameState, p, BLOCK_TREE_LEAVES);

    float3 offsets[] = {make_float3(1, treeHeight, 0), make_float3(1, treeHeight, 1), 
                        make_float3(-1, treeHeight, -1), make_float3(1, treeHeight, -1), 
                        make_float3(-1, treeHeight, 1), make_float3(0, treeHeight, 1),
                        make_float3(-1, treeHeight, 0), make_float3(0, treeHeight, -1), 
                        };

    for(int j = 0; j < 2; ++j) {
        for(int i = 0; i < arrayCount(offsets); ++i) {
            float3 o = offsets[i];
            o.y -= j;
            float3 p = plus_float3(worldP, o);
            addBlock(gameState, p, BLOCK_TREE_LEAVES);
        }
    }
    treeHeight -=1;

    float3 offsets2[] = {make_float3(2, treeHeight, 0), make_float3(2, treeHeight, 1), make_float3(2, treeHeight, 2), 
                        make_float3(1, treeHeight, 2), make_float3(0, treeHeight, 2), 

                        make_float3(-1, treeHeight, 2), make_float3(-2, treeHeight, 2), make_float3(-2, treeHeight, 1), 
                        make_float3(-2, treeHeight, 0), make_float3(-2, treeHeight, -1), 

                        make_float3(-2, treeHeight, -2), make_float3(-1, treeHeight, -2), 
                        make_float3(0, treeHeight, -2), make_float3(1, treeHeight, -2),  make_float3(2, treeHeight, -2), make_float3(2, treeHeight, -1), 
                        };

    for(int i = 0; i < arrayCount(offsets2); ++i) {
        float3 p = plus_float3(worldP, offsets2[i]);
        addBlock(gameState, p, BLOCK_TREE_LEAVES);
    }

}

BlockType worldGeneration_shouldBlockExist(int worldX, int worldY, int worldZ) {
    float terrainHeight = getTerrainHeight(worldX, worldZ);

    bool underWater = worldY < WATER_ELEVATION;
    BlockType type = BLOCK_NONE;

    if(worldY < terrainHeight) {
        BlockType type = BLOCK_GRASS;
        bool isTop = false;

        if(underWater) {
            type = BLOCK_SOIL;
        } else if(worldY < (terrainHeight - 1)) {
            type = BLOCK_SOIL;
        } else {
            isTop = true;
        }

        //TODO: Handle Tree building based on perlin noise
        // if(worldY > waterElevation && isTop && isTreeLocation(worldX, worldZ)) {
            // generateTree_multiThread(gameState, chunk, make_float3(worldX, worldY + 1, worldZ));
        // } 
        
    } else if(worldY < WATER_ELEVATION) {
        type = BLOCK_WATER;
    }

    return type;
}

void fillChunk_multiThread(void *data_) {
    FillChunkData *data = (FillChunkData *)data_;

    GameState *gameState = data->gameState;
    Chunk *chunk = data->chunk;

    for(int z = 0; z < CHUNK_DIM; ++z) {
        for(int x = 0; x < CHUNK_DIM; ++x) {
            int worldX = x + chunk->x*CHUNK_DIM;
            int worldZ = z + chunk->z*CHUNK_DIM;

            float waterElevation = WATER_ELEVATION;

            float terrainHeight = getTerrainHeight(worldX, worldZ);

            for(int y = 0; y < CHUNK_DIM; ++y) {
                int worldY = y + chunk->y*CHUNK_DIM;

                bool underWater = worldY < waterElevation;

                if(worldY < terrainHeight) {
                    if(!chunk->blocks) {
                        chunk->blocks = (Block *)easyPlatform_allocateMemory(BLOCKS_PER_CHUNK*sizeof(Block), EASY_PLATFORM_MEMORY_ZERO);
                    }
                    
                    BlockType type = BLOCK_GRASS;
                    bool isTop = false;

                    if(underWater) {
                        //NOTE: Vary underwater terrain (grass can't grow underwater)
                        float value = SimplexNoise_fractal_3d(8, worldX, worldY, worldZ, 0.1f);
                        type = BLOCK_SOIL;
                        if(value < 0.5f) {
                            type = BLOCK_STONE;
                        }
                    } else if(worldY < (terrainHeight - 1)) {
                        if(worldY >= ((terrainHeight - 1) - SUB_SOIL_DEPTH)) {
                            type = BLOCK_SOIL;
                        } else {
                            if(isIronLocation(worldX, worldY, worldZ)) {
                                type = BLOCK_IRON;
                            } else if(isCoalLocation(worldX, worldY, worldZ)) {
                                type = BLOCK_COAL;
                            } else {
                                type = BLOCK_STONE;
                            }
                            
                        }
                    } else {
                        isTop = true;
                    }

                    int blockIndex = getBlockIndex(x, y, z);
                    assert(blockIndex < BLOCKS_PER_CHUNK);
                    if(blockIndex < BLOCKS_PER_CHUNK) {
                        chunk->blocks[blockIndex] = spawnBlock(x, y, z, type);
                        // assert(chunk->blocks[blockIndex].flags != 0);
                    }

                    if(worldY > waterElevation && isTop && isTreeLocation(worldX, worldZ)) {
                        generateTree_multiThread(gameState, chunk, make_float3(worldX, worldY + 1, worldZ));
                    } else if(worldY > waterElevation) {
                        if(isTop && isBushLocation(worldX, worldZ)) {
                            EntityType grassType = ENTITY_GRASS_LONG;
                            BlockType grassType1 = BLOCK_GRASS_SHORT_ENTITY;
                            bool bigBush = isBigBush(worldX, worldZ);
                            if(!bigBush) {
                                grassType = ENTITY_GRASS_SHORT;
                                grassType1 = BLOCK_GRASS_TALL_ENTITY;
                            }

                            int chunkX = roundChunkCoord((float)worldX / (float)CHUNK_DIM);
                            int chunkY = roundChunkCoord((float)(worldY + 1) / (float)CHUNK_DIM);
                            int chunkZ = roundChunkCoord((float)worldZ / (float)CHUNK_DIM);


                            int localX = x;
                            int localY = (worldY + 1) - (CHUNK_DIM*chunkY); 
                            int localZ = z;

                            int blockIndex = getBlockIndex(localX, localY, localZ);
                            assert(blockIndex < BLOCKS_PER_CHUNK);
                            if(blockIndex < BLOCKS_PER_CHUNK) {
                                chunk->blocks[blockIndex] = spawnBlock(localX, localY, localZ, grassType1);
                            }
                        }
                    }
                    
                } else if(worldY < waterElevation) {
                    if(!chunk->blocks) {
                        chunk->blocks = (Block *)easyPlatform_allocateMemory(BLOCKS_PER_CHUNK*sizeof(Block), EASY_PLATFORM_MEMORY_ZERO);
                    }
                    //NOTE: Is water so add water
                    int blockIndex = getBlockIndex(x, y, z);
                    assert(blockIndex < BLOCKS_PER_CHUNK);
                    if(blockIndex < BLOCKS_PER_CHUNK) {
                        chunk->blocks[blockIndex] = spawnBlock(x, y, z, BLOCK_WATER);
                    }
                }
            }
        }
    }

    MemoryBarrier();
    ReadWriteBarrier();

    chunk->generateState = CHUNK_GENERATED | CHUNK_MESH_DIRTY;

    free(data_);
    data_ = 0;
}


void fillChunk(GameState *gameState, Chunk *chunk) {
    assert(chunk->generateState & CHUNK_NOT_GENERATED);
    chunk->generateState = CHUNK_GENERATING;

    MemoryBarrier();
    ReadWriteBarrier();

    FillChunkData *data = (FillChunkData *)malloc(sizeof(FillChunkData));

    data->gameState = gameState;
    data->chunk = chunk;

    //NOTE: Multi-threaded version
    pushWorkOntoQueue(&gameState->threadsInfo, fillChunk_multiThread, data);
}
