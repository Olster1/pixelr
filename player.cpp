float3 getBlockWorldPos(BlockChunkPartner b) {
    float3 p = {};

    p.x = b.block->x + (CHUNK_DIM*b.chunk->x);
    p.y = b.block->y + (CHUNK_DIM*b.chunk->y);
    p.z = b.block->z + (CHUNK_DIM*b.chunk->z);

    return p;
}



BlockChunkPartner castRayAgainstBlock(GameState *gameState, float3 dir, float length, float3 start, BlockFlags blockFlags = BLOCK_EXISTS) {
    int blockRadius = length;
    float shortestT = FLT_MAX;
    BlockChunkPartner block = {};
    block.block = 0;

    for(int z = -blockRadius; z <= blockRadius; ++z) {
        for(int x = -blockRadius; x <= blockRadius; ++x) {
            for(int y = -blockRadius; y <= blockRadius; ++y) {
                float3 pos = start;
                //NOTE: Get the block pos in world
                float3 blockPos = convertRealWorldToBlockCoords(pos);
                int worldx = (int)blockPos.x + x;
                int worldy = (int)blockPos.y + y;
                int worldz = (int)blockPos.z + z;

                BlockChunkPartner blockTemp = blockExists(gameState, worldx, worldy, worldz, blockFlags);

                if(blockTemp.block) {
                    //NOTE: Ray Cast against the block
                    Rect3f b = make_rect3f_center_dim(make_float3(worldx, worldy, worldz), make_float3(1, 1, 1));
                    float3 hitPoint;
                    float3 normalVector;
                    float tAt;
                    if(easyMath_rayVsAABB3f(pos, dir, b, &hitPoint, &tAt, &normalVector)) {
                        if(tAt <= length && tAt < shortestT) {
                            shortestT = tAt;
                            block = blockTemp;
                            block.sideNormal = normalVector;
                        }
                    }
                }
            }
        }
    }

    return block;
}

void updatePlayerPhysics(GameState *gameState, Entity *e, float3 movementForFrame) {
    int physicsIterations = 4;
    bool didHit = false;

    //NOTE: Start the frame not grounded
    e->grounded = false;

    //NOTE: GRAVITY
    if(gameState->camera.followingPlayer) {
        e->dP.y -= GRAVITY_POWER*gameState->dt;
    } else {
        e->dP = make_float3(0, 0, 0);
    }

    //NOTE: Integrate velocity
    // e->dP = plus_float3(e->dP, accelForFrame); //NOTE: Already * by dt 

    //NOTE: Apply drag
    // e->dP = scale_float3(0.95f, e->dP);

    //NOTE: Get the movement vector for this frame
    float3 dpVector = plus_float3(movementForFrame, scale_float3(gameState->dt, e->dP));
    // float lengthLeft = float3_magnitude(dpVector);

    updateRecoverMovement(gameState, e);

    for(int i = 0; i < physicsIterations; ++i) {
        // dpVector = normalize_float3(dpVector);
        BlockChunkPartner blockHit = {};
        blockHit.block = 0;
        
        float radius = float3_magnitude(dpVector);

        int blockRadius = maxi(ceil(radius + 1.5f), 5);

        float shortestT = FLT_MAX;
        bool hit = false;
        float3 shortestNormalVector = make_float3(0, 0, 0);

        for(int z = -blockRadius; z <= blockRadius; ++z) {
            for(int x = -blockRadius; x <= blockRadius; ++x) {
                for(int y = -blockRadius; y <= blockRadius; ++y) {
                    float3 roundedPos = convertRealWorldToBlockCoords(e->T.pos);
                    //NOTE: Get the block pos in world
                    int worldx = ((int)roundedPos.x) + x;
                    int worldy = ((int)roundedPos.y) + y;
                    int worldz = ((int)roundedPos.z) + z;
                    
                    BlockChunkPartner blockPtr = blockExists(gameState, worldx, worldy, worldz, BLOCK_EXISTS_COLLISION);
                    if(blockPtr.block) {
                        //NOTE: Ray Cast against the block
                        Rect3f block = make_rect3f_center_dim(make_float3(worldx, worldy, worldz), make_float3(1, 1, 1));
                        Rect3f playerB = make_rect3f_center_dim(make_float3(0, 0, 0), e->T.scale);
                        Rect3f b = rect3f_minowski_plus(playerB, block, make_float3(worldx, worldy, worldz));
                        float3 hitPoint;
                        float3 normalVector;
                        float tAt;
                        if(easyMath_rayVsAABB3f(e->T.pos, dpVector, b, &hitPoint, &tAt, &normalVector)) {
                            
                            float slopeFactor = 0.3f;
                            float distToGround = 0.1f;
                            if(tAt < distToGround && float3_dot(make_float3(0, 1, 0), normalVector) > slopeFactor) {
                                e->grounded = true;
                            }

                            if(tAt <= float3_magnitude(dpVector) && tAt < shortestT) {
                                shortestT = tAt;
                                shortestNormalVector = normalVector;
                                hit = true;
                                blockHit = blockPtr;
                            }
                        }
                    }
                }
            }
        }

        //NOTE: Update player movement
        float3 moveVector = make_float3(0, 0, 0);
        if(hit) {
            float epsilonValue = EPSILON_VALUE;
            float moveFactor = fmax(0, shortestT - epsilonValue);
            moveVector = scale_float3(moveFactor, normalize_float3(dpVector));
        } else {
            moveVector = dpVector;
        }
        
        e->T.pos = plus_float3(e->T.pos, moveVector);

        if(hit) {
            didHit = true;

            if((e->dP.y < -15 && float3_dot(make_float3(0, 1, 0), shortestNormalVector) > 0.0f) && gameState->camera.followingPlayer) {
                //NOTE: Big fall so shake the camera
                gameState->camera.shakeTimer = MAX_SHAKE_TIMER;
                playSound(&gameState->fallBigSound);
            }
            

            //NOTE: Auto jump code - see if hit the sides 
            if(float3_dot(make_float3(1, 0, 0), shortestNormalVector) > 0.0f 
            || float3_dot(make_float3(-1, 0, 0), shortestNormalVector) > 0.0f
            || float3_dot(make_float3(0, 0, -1), shortestNormalVector) > 0.0f
            || float3_dot(make_float3(0, 0, 1), shortestNormalVector) > 0.0f) {

                if(blockHit.block && blockHit.chunk) {
                    if(!blockExistsReadOnly(gameState, blockHit.block->x + CHUNK_DIM*blockHit.chunk->x, blockHit.block->y + CHUNK_DIM*blockHit.chunk->y + 1, blockHit.block->z + CHUNK_DIM*blockHit.chunk->z, BLOCK_EXISTS_COLLISION)) {
                        e->tryJump = true;
                    }
                }
            }

            //NOTE: Update dpVector & total move time
            float3 d = scale_float3(float3_dot(e->dP, shortestNormalVector), shortestNormalVector);
            e->dP = minus_float3(e->dP, d);

            d = scale_float3(float3_dot(dpVector, shortestNormalVector), shortestNormalVector);
            dpVector = minus_float3(dpVector, d);

            if(shortestNormalVector.x == 0 && shortestNormalVector.y == 0 && shortestNormalVector.z == 0) {
                //NOTE: Inside the block
                float3 moveDir = findClosestFreePosition(gameState, e->T.pos, make_float3(0, 1, 0), gameState->searchOffsets, arrayCount(gameState->searchOffsets), arrayCount(gameState->searchOffsets));
                moveDir = normalize_float3(moveDir);
                e->recoverDP = plus_float3(e->recoverDP, scale_float3(1.0f, moveDir));
            } else {
                //NOTE: Not in a block so reset the recover dP
                e->recoverDP = make_float3(0, 0, 0);
            }   
        }  
    }
}

void cancelMiningInteraction(GameState *gameState) {
    gameState->currentMiningBlock->timeLeft = getBlockTime((BlockType)gameState->currentMiningBlock->type);
    gameState->currentMiningBlock = 0;

    if(gameState->miningSoundPlaying) {
        gameState->miningSoundPlaying->shouldEnd = true;
        gameState->miningSoundPlaying = 0;
    }
}

void invalidateSurroundingAoValues(GameState *gs, int worldX, int worldY, int worldZ) {
    for(int z = -1; z <= 1; z++) {
        for(int y = -1; y <= 1; y++) {
            for(int x = -1; x <= 1; x++) {
                int chunkX = roundChunkCoord((float)(worldX + x) / (float)CHUNK_DIM);
                int chunkY = roundChunkCoord((float)(worldY + y) / (float)CHUNK_DIM);
                int chunkZ = roundChunkCoord((float)(worldZ + z) / (float)CHUNK_DIM);

                Chunk *c = getChunkReadOnly(gs, chunkX, chunkY, chunkZ);

                if(c) {
                    c->generateState &= ~(CHUNK_MESH_DIRTY | CHUNK_MESH_BUILDING);
                    c->generateState |= CHUNK_MESH_DIRTY;
                }
            }
        }
    }
}

bool placeBlock(GameState *gameState, float3 lookingAxis, Entity *e, BlockType blockType) {
    bool placed = false;
    BlockChunkPartner b = castRayAgainstBlock(gameState, lookingAxis, DISTANCE_CAN_PLACE_BLOCK, plus_float3(gameState->cameraOffset, e->T.pos), BLOCK_EXISTS);

    if(b.block) {
        // b.block->hitBlock = true;
        int worldX = b.sideNormal.x + b.block->x + (CHUNK_DIM*b.chunk->x);
        int worldY = b.sideNormal.y + b.block->y + (CHUNK_DIM*b.chunk->y);
        int worldZ = b.sideNormal.z + b.block->z + (CHUNK_DIM*b.chunk->z);

        //NOTE: check if stacking ontop of a block
        if(b.sideNormal.y < 1 || (getBlockFlags(gameState, b.block->type) & BLOCK_FLAG_STACKABLE)){
            BlockChunkPartner nxtBlock = blockExists(gameState, worldX, worldY, worldZ, BLOCK_EXISTS_COLLISION);
            
            //NOTE: There's no block there
            if(!nxtBlock.block) {
                float c = 1 + EPSILON_VALUE;
                Rect3f blockBounds = make_rect3f_center_dim(make_float3(0, 0, 0), make_float3(c, c, c));
                blockBounds = rect3f_minowski_plus(blockBounds, make_rect3f_center_dim(make_float3(0, 0, 0), e->T.scale), make_float3(worldX, worldY, worldZ));
                if(!in_rect3f_bounds(blockBounds, e->T.pos)) {
                    //NOTE: place block
                    if(nxtBlock.chunk) {
                        if(!nxtBlock.chunk->blocks) {
                            nxtBlock.chunk->blocks = (Block *)easyPlatform_allocateMemory(BLOCKS_PER_CHUNK*sizeof(Block), EASY_PLATFORM_MEMORY_ZERO);
                        }

                        int localX = worldX - (CHUNK_DIM*nxtBlock.chunk->x); 
                        int localY = worldY - (CHUNK_DIM*nxtBlock.chunk->y); 
                        int localZ = worldZ - (CHUNK_DIM*nxtBlock.chunk->z); 

                        int blockIndex = getBlockIndex(localX, localY, localZ);
                        if(blockIndex < BLOCKS_PER_CHUNK) {
                            nxtBlock.chunk->blocks[blockIndex] = spawnBlock(localX, localY, localZ, blockType);
                            placed = true;
                            invalidateSurroundingAoValues(gameState, worldX, worldY, worldZ);
                            playSound(&gameState->blockFinishSound);
                        } else {
                            assert(false);
                        }
                    }
                }
            }
        }
    }
    return placed;
}



Particler *findParticler(GameState *gameState, ParticlerId id) {
    Particler *result = 0;
    for(int i = 0; i < gameState->particlerCount && !result; ++i) {
        Particler *p = &gameState->particlers[i];

        if(particlerIdsMatch(p->id, id)) {
            result = p;
            break;
        }
    }

    return result;

}

void highlightBlockLookingAt(GameState *gameState, float3 lookingAxis, Entity *e) {
    BlockChunkPartner b = castRayAgainstBlock(gameState, lookingAxis, DISTANCE_CAN_PLACE_BLOCK, plus_float3(gameState->cameraOffset, e->T.pos), BLOCK_FLAGS_MINEABLE);

    if(b.block) {
        float3 blockWorldP = getBlockWorldPos(b);
        pushAlphaCube(gameState->renderer, blockWorldP, BLOCK_OUTLINE, make_float4(0, 0, 0, 0.1f));
        gameState->DEBUG_BlocksDrawnForFrame++;
    }
}

Particler *getBlockMiningParticler(GameState *gameState, Block *block, float3 blockWorldP) {
    Particler *p = 0;
    //NOTE: Get reference to the particler
    assert(gameState->particlerCount < arrayCount(gameState->particlers));
    if(gameState->particlerCount < arrayCount(gameState->particlers)) {
        float lifespan = 0.5f;
        float spawnRatePerSecond = 10;
        gameState->particlers[gameState->particlerCount++] = initParticler(lifespan, spawnRatePerSecond, make_rect3f_center_dim(plus_float3(blockWorldP, make_float3(0, 0.5f, 0)), make_float3(1, 0.1f, 1)), make_float4(0.5f, 0.75f, 0.5f, 0.75f), makeParticlerId(block));
        p = &gameState->particlers[gameState->particlerCount - 1];
    }
    return p;
}


void mineBlock(GameState *gameState, float3 lookingAxis, Entity *e) {
    float3 cameraPos = plus_float3(gameState->cameraOffset, e->T.pos);
    BlockChunkPartner b = castRayAgainstBlock(gameState, lookingAxis, DISTANCE_CAN_PLACE_BLOCK, cameraPos, BLOCK_FLAGS_MINEABLE);

        if(b.block) {
            //NOTE: Play sound
            if(!gameState->miningSoundPlaying) {
                gameState->miningSoundPlaying = playSound(&gameState->blockBreakSound);
                gameState->miningSoundPlaying->nextSound = gameState->miningSoundPlaying;
            }

            // b.block->hitBlock = true;
            b.block->timeLeft -= gameState->dt;

            if(!gameState->camera.followingPlayer) {
                // b.block->timeLeft = 0;
            }

            //NOTE: Show progress on mining the block
            float percent = 1.0f - fmax(0, b.block->timeLeft / getBlockTime((BlockType)b.block->type));

            float3 blockWorldP = getBlockWorldPos(b);
            if(!(getBlockFlags(gameState, b.block->type) & BLOCK_FLAGS_NO_MINE_OUTLINE)) {
                //NOTE: the destruction overlay
                pushAlphaItem(gameState->renderer, blockWorldP, make_float3(1.001f, 1.001f, 1.001f), make_float4(1, 1, 1, 0.7f), percent);
            }
            Particler *p;
            if(gameState->camera.followingPlayer) {
                p = findParticler(gameState, makeParticlerId(b.block));
                if(!p) {
                    p = getBlockMiningParticler(gameState, b.block, blockWorldP);
                }
                assert(p);
                if(p) {
                    p->lifeAt = 0; //NOTE: doesn't die while were still mining
                }
            }

            gameState->showCircleTimer = 0;
            pushFillCircle(gameState->renderer, make_float3(0, 0, 0), CIRCLE_RADIUS_MAX*percent, make_float4(1, 1, 1, 1));
            
            //NOTE: Check if block was successfully mined
            if(b.block->timeLeft <= 0) {
                //NOTE: Add block to pickup 
                if(!(getBlockFlags(gameState, b.block->type) & BLOCK_NOT_PICKABLE) && gameState->camera.followingPlayer) {
                    initPickupItem(gameState, b.chunk, blockWorldP, (BlockType)b.block->type, gameState->randomStartUpID);
                }
                
                //NOTE: Destory the block
                b.chunk->blocks[b.blockIndex].exists = false;

                if(gameState->camera.followingPlayer) {
                    playSound(&gameState->blockFinishSound);
                }

                float3 worldP = blockWorldP;
                invalidateSurroundingAoValues(gameState, worldP.x, worldP.y, worldP.z);

                //NOTE: Check if we should destroy an above block like grass
                Block *aboveBlock = blockExistsReadOnly_withBlock(gameState, worldP.x, worldP.y + 1 , worldP.z, BLOCK_FLAGS_UNSAFE_UNDER);
                if(aboveBlock) {
                    //NOTE: Destory the block
                    aboveBlock->exists = false;
                    float3 blockWorldPAbove = blockWorldP;
                    blockWorldPAbove.y++;
                    getBlockMiningParticler(gameState, b.block, blockWorldPAbove);
                    
                }

                //NOTE: Clear the block
                b.block = 0;
            }
        }

        if(gameState->currentMiningBlock && gameState->currentMiningBlock != b.block) {
            //NOTE: Reset if we were mining another block
            cancelMiningInteraction(gameState);
        }

        gameState->currentMiningBlock = b.block;
}

void updatePlayer(GameState *gameState, Entity *e) {
    if(gameState->camera.followingPlayer) {
        float rotSpeed = 13.0f;

        float2 mouseDelta = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP);

        e->T.rotation.y += gameState->dt*rotSpeed*-mouseDelta.x;
        e->T.rotation.x += gameState->dt*rotSpeed*-mouseDelta.y;

        if(e->T.rotation.x > 89) {
            e->T.rotation.x = 89;
        }
        if(e->T.rotation.x < -89) {
            e->T.rotation.x = -89;
        }
    }

    float16 rot = eulerAnglesToTransform(e->T.rotation.y, e->T.rotation.x, e->T.rotation.z);
    float3 movement = make_float3(0, 0, 0);

    if(gameState->camera.followingPlayer) {

        float3 zAxis = make_float3(rot.E_[2][0], 0, rot.E_[2][2]); //NOTE: rot.E_[2][1] in y if we want to fly
        zAxis = normalize_float3(zAxis);
        float3 xAxis = make_float3(rot.E_[0][0], rot.E_[0][1], rot.E_[0][2]);

        float3 playerSpeed = make_float3(1, 1, 1);
        float magnitude = gameState->dt*WALK_SPEED;

        if(gameState->keys.keys[KEY_LEFT]) {
            movement = plus_float3(movement, float3_negate(xAxis));
        }
        if(gameState->keys.keys[KEY_RIGHT]) {
            movement = plus_float3(movement, xAxis);
        }
        if(gameState->keys.keys[KEY_DOWN]) {
            movement = plus_float3(movement, float3_negate(zAxis));
        }
        if(gameState->keys.keys[KEY_UP]) {
            movement = plus_float3(movement, zAxis);

            if(gameState->keys.keys[KEY_SHIFT] && e->grounded) {
                //NOTE: Player is running
                magnitude *= 2.0f;

                if(!e->running) {
                    e->running = true;
                    
                    gameState->camera.runShakeTimer = 0;
                }
                
            }
        }

        if(e->running) {
            gameState->camera.targetFov = 80;
            e->stamina -= gameState->dt*STAMINA_DRAIN_SPEED;

            if(e->stamina < 0) {
                e->stamina = 0;
            }
        } else {
            e->stamina += gameState->dt*STAMINA_RECHARGE_SPEED;

            if(e->stamina >= 1) {
                e->stamina = 1;
            }
        }

        movement = normalize_float3(movement);
        movement = scale_float3(magnitude, movement);

        if(!gameState->keys.keys[KEY_SHIFT] || e->stamina <= 0) {
            e->running = false;
            gameState->camera.runShakeTimer = -1;
        }
        
        if((gameState->keys.keys[KEY_SPACE] == MOUSE_BUTTON_PRESSED || e->tryJump) && e->grounded) {
            //NOTE: JUMP
            e->dP.y += JUMP_POWER;
        }
        e->tryJump = false;
    } 

    updatePlayerPhysics(gameState, e, movement);
    float3 lookingAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
        gameState->placeBlockTimer = 0;
        gameState->mineBlockTimer = 0;
    }
    
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN) {
        gameState->showCircleTimer = 0;
        
       if(gameState->placeBlockTimer >= 0) {
            gameState->placeBlockTimer += gameState->dt;
        }

        if(gameState->mineBlockTimer >= 0) {
            gameState->mineBlockTimer += gameState->dt;
        }

        if(gameState->mineBlockTimer > 0.45f) {
            //NOTE: Mining a block
            mineBlock(gameState, lookingAxis, e);
        }
    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED) {
        if(gameState->placeBlockTimer < 0.2f) {
            bool placed = placeBlock(gameState, lookingAxis, e, BLOCK_GRASS);
            // //NOTE: Check if user has any blocks in their inventory hotspot they can use
            // if(gameState->playerInventory[gameState->currentInventoryHotIndex].count > 0) 
            // {
            //     //NOTE: Placing a block down
            //     bool placed = placeBlock(gameState, lookingAxis, e, gameState->playerInventory[gameState->currentInventoryHotIndex].type);
            //     if(placed) {
            //         //NOTE: Decrement their inventory count
            //         gameState->playerInventory[gameState->currentInventoryHotIndex].count--;
            //     }
            // }
        }

        if(gameState->currentMiningBlock) {
            cancelMiningInteraction(gameState);
        }

        gameState->placeBlockTimer = -1;
        gameState->mineBlockTimer = -1;
    }
    highlightBlockLookingAt(gameState, lookingAxis, e);
    
    if(gameState->showCircleTimer >= 0) {
        
        pushCircleOutline(gameState->renderer, make_float3(0, 0, 0), CIRCLE_RADIUS_MAX, make_float4(1, 1, 1, 1));
        
        if(gameState->mouseLeftBtn == MOUSE_BUTTON_NONE) {
            gameState->showCircleTimer += gameState->dt;
            if(gameState->showCircleTimer >= SHOW_CIRCLE_DELAY) {
                //NOTE: Finish the timer
                gameState->showCircleTimer = -1;
            }
        }
    }
}