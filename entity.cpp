enum EntityType {
    ENTITY_NONE,
    ENTITY_PLAYER,
    ENTITY_PICKUP_ITEM,
    ENTITY_GRASS_SHORT,
    ENTITY_GRASS_LONG,
    ENTITY_FOX,
    ENTITY_HORSE,
};

enum BlockType {
    BLOCK_NONE,
    BLOCK_GRASS,
    BLOCK_SOIL,
    BLOCK_STONE,
    BLOCK_CLOUD,
    BLOCK_TREE_WOOD,
    BLOCK_TREE_LEAVES,
    BLOCK_WATER,
    BLOCK_GRASS_SHORT_ENTITY,
    BLOCK_GRASS_TALL_ENTITY,
    BLOCK_COAL,
    BLOCK_IRON,
    BLOCK_OUTLINE,


    //NOTHING PAST HERE
    BLOCK_TYPE_COUNT
};

struct TimeOfDayValues {
    float4 skyColorA;
    float4 skyColorB;
};

enum VoxelPhysicsFlag {
    VOXEL_NONE = 0,
    VOXEL_OCCUPIED = 1 << 0,
    VOXEL_CORNER = 1 << 1,
    VOXEL_EDGE = 1 << 2,
    VOXEL_INSIDE = 1 << 3,
    VOXEL_COLLIDING = 1 << 4,
};

struct EntityID {
    size_t stringSizeInBytes; //NOTE: Not include null terminator
    char stringID[256]; 
    uint32_t crc32Hash;
};

static int global_entityIdCreated = 0;

EntityID makeEntityId(int randomStartUpID) {
    EntityID result = {};

    time_t timeSinceEpoch = time(0);

    #define ENTITY_ID_PRINTF_STRING "%ld-%d-%d", timeSinceEpoch, randomStartUpID, global_entityIdCreated

    //NOTE: Allocate the string
    size_t bufsz = snprintf(NULL, 0, ENTITY_ID_PRINTF_STRING) + 1;
    assert(bufsz < arrayCount(result.stringID));
    result.stringSizeInBytes = MathMin_sizet(arrayCount(result.stringID), bufsz);
    snprintf(result.stringID, bufsz, ENTITY_ID_PRINTF_STRING);

    result.crc32Hash = get_crc32(result.stringID, result.stringSizeInBytes);

    #undef ENTITY_ID_PRINTF_STRING

    //NOTE: This would have to be locked in threaded application
    global_entityIdCreated++;

    return result;
} 

bool areEntityIdsEqual(EntityID a, EntityID b) {
    bool result = false;
    if(a.crc32Hash == b.crc32Hash && easyString_stringsMatch_nullTerminated(a.stringID, b.stringID)) {
        result = true;
    }
    return result;
}

struct VoxelEntity {
    EntityID id;
    TransformX T;

    bool inBounds;

    //NOTE: Physics details
    float3 dP;
    float3 ddPForFrame;

    float dA; //NOTE: Positive is clockwise
    float ddAForFrame;//NOTE: Torque for frame

    float inverseMass;
    float invI;
    float coefficientOfRestitution;
    float friction;

    //NOTE: Voxel data
    float2 worldBounds;

    float sleepTimer;
    bool asleep;

    float2 *corners;

    int occupiedCount;
    u8 *data;
    int stride;
    int pitch;
};

#include "./physics.cpp"

u8 getByteFromVoxelEntity(VoxelEntity *e, int x, int y) {
    return e->data[y*e->stride + x];
}

bool isVoxelOccupied(VoxelEntity *e, int x, int y, u32 flags = VOXEL_OCCUPIED) {
    bool result = false;
    if(x >= 0 && x < e->stride && y >= 0 && y < e->pitch) {
        result = (e->data[y*e->stride + x] & flags);
    }
    return result;
}

float2 getVoxelPositionInModelSpace(float2 pos) {
    pos.x *= VOXEL_SIZE_IN_METERS;
    pos.y *= VOXEL_SIZE_IN_METERS;

    //NOTE: To account for a voxel posiiton being in the center of a voxel
    pos.x += 0.5f*VOXEL_SIZE_IN_METERS;
    pos.y += 0.5f*VOXEL_SIZE_IN_METERS;
    return pos;
}


float2 getVoxelPositionInModelSpaceFromCenter(VoxelEntity *e, float2 pos) {
    pos = getVoxelPositionInModelSpace(pos);
    float2 center = make_float2(0.5f*e->worldBounds.x, 0.5f*e->worldBounds.y);
    pos = minus_float2(pos, center);

    return pos;
}

struct FloodFillVoxel {
    int x; 
    int y;
    FloodFillVoxel *next;
    FloodFillVoxel *prev;
};

void pushOnVoxelQueue(VoxelEntity *e, FloodFillVoxel *queue, bool *visited, int x, int y) {

	if(!visited[e->stride*y + x]) {
		FloodFillVoxel *node = pushStruct(&globalPerFrameArena, FloodFillVoxel);
		node->x = x;
        node->y = y;

		queue->next->prev = node;
		node->next = queue->next;

		queue->next = node;
		node->prev = queue;

		//push node to say you visited it
        visited[e->stride*y + x] = true;
		
	}
		
}

FloodFillVoxel *popOffVoxelQueue(FloodFillVoxel *queue) {
	FloodFillVoxel *result = 0;

	if(queue->prev != queue) { //something on the queue
		result = queue->prev;

		queue->prev = result->prev;
		queue->prev->next = queue;
	} 

	return result;
}

float2 voxelToWorldP(VoxelEntity *e, int x, int y) {
    float2 modelSpace = getVoxelPositionInModelSpaceFromCenter(e, make_float2(x, y));
    TransformX TTemp = e->T;
    TTemp.rotation.z = radiansToDegrees(TTemp.rotation.z);
    float2 p = float16_transform(getModelToViewSpaceWithoutScale(TTemp), make_float4(modelSpace.x, modelSpace.y, 0, 1)).xy; 
    return p;
}

float2 worldPToVoxelP(VoxelEntity *e, float2 worldP) {
    TransformX TTemp = e->T;
    TTemp.rotation.z = radiansToDegrees(TTemp.rotation.z);
    float16 T = transform_getInverseX(TTemp);
    float2 p = float16_transform(T, make_float4(worldP.x, worldP.y, 0, 1)).xy;
    p = plus_float2(p, make_float2(0.5f*e->worldBounds.x, 0.5f*e->worldBounds.y));

    //NOTE: convert to voxel space
    p.x *= VOXELS_PER_METER;
    p.y *= VOXELS_PER_METER;

    p.x = (int)p.x;
    p.y = (int)p.y;

    return p;
}


float findSeperationForShape(VoxelEntity *e, int startX, int startY, float2 startWorldP, float2 unitVector) {
    float x = startX;
    float y = startY;

    while(isVoxelOccupied(e, (int)x, (int)y)) {
        x += unitVector.x;
        y += unitVector.y;
    }

    float2 v = minus_float2(voxelToWorldP(e, x, y), startWorldP);

    return float2_magnitude(v) * signOf(float2_dot(v, unitVector));
}

float2 getSeperationViaFloodFill(VoxelEntity *e, int startX, int startY, float2 startWorldP) {
    float2 result = {};
    perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);

    bool *visited = pushArray(&globalPerFrameArena, e->stride*e->pitch, bool);
    easyMemory_zeroSize(visited, sizeof(bool)*e->stride*e->pitch);

    FloodFillVoxel queue = {};
    queue.next = queue.prev = &queue; 
	pushOnVoxelQueue(e, &queue, visited, startX, startY);

	bool searching = true;
	while(searching) {	
		FloodFillVoxel *node = popOffVoxelQueue(&queue);
		if(node) {
			int x = node->x;
			int y = node->y;

			if(!isVoxelOccupied(e, x, y)) {
                result = minus_float2(voxelToWorldP(e, x, y), startWorldP);
				searching = false;
				break;
			} else {
				//push on more directions   
				pushOnVoxelQueue(e, &queue, visited, x + 1, y);
				pushOnVoxelQueue(e, &queue, visited, x - 1, y);
				pushOnVoxelQueue(e, &queue, visited, x, y + 1);
				pushOnVoxelQueue(e, &queue, visited, x, y - 1);

				//Diagonal movements
                pushOnVoxelQueue(e, &queue, visited, x + 1, y + 1);
				pushOnVoxelQueue(e, &queue, visited, x + 1, y - 1);
				pushOnVoxelQueue(e, &queue, visited, x - 1, y - 1);
				pushOnVoxelQueue(e, &queue, visited, x - 1, y + 1);
			}
		} else {
			searching = false;
			break;
		}
	}

    releaseMemoryMark(&perFrameArenaMark);
    return result;
}

int doesVoxelCollide(PhysicsWorld *physicsWorld, float2 worldP, VoxelEntity *e, int idX, int idY, bool swap, CollisionPoint *points, VoxelEntity *otherE) {
    float2 p = worldPToVoxelP(e, worldP);

    int x = (int)(p.x);
    int y = (int)(p.y);

    float2 voxels[9] = {
        make_float2(0, 0), 
        make_float2(1, 0), 
        make_float2(-1, 0), 
        make_float2(0, 1), 
        make_float2(0, -1), 
        make_float2(1, 1), 
        make_float2(-1, -1), 
        make_float2(-1, 1), 
        make_float2(1, -1), 
    };  

    int pointCount = 0;

    for(int i = 0; i < arrayCount(voxels); ++i) {
        float2 voxelSpace = plus_float2(make_float2(x, y), voxels[i]);

        int testX = (int)voxelSpace.x;
        int testY = (int)voxelSpace.y;

        if(isVoxelOccupied(e, testX, testY)) {
            float2 voxelWorldP = voxelToWorldP(e, testX, testY);
            float2 diff = minus_float2(worldP, voxelWorldP);

            if(swap) {
                diff = minus_float2(voxelWorldP, worldP);
            }

            float distanceSqr = float2_dot(diff, diff);

            if(distanceSqr <= VOXEL_SIZE_IN_METERS_SQR) {
                CollisionPoint result = {};

                result.entityId = e->id;
                result.x = idX;
                result.y = idY;
                result.x1 = testX;
                result.y1 = testY;
                e->data[testY*e->stride + testX] |= VOXEL_COLLIDING;
                result.point = lerp_float2(worldP, voxelWorldP, 0.5f);
                
                result.normal = normalize_float2(diff);//TODO: I think this is causing an issue since the normal value shouldn't 
                result.seperation = sqrt(distanceSqr) - VOXEL_SIZE_IN_METERS; //NOTE: Just resolves the individual voxel
                // result.seperation = findSeperationForShape(e, testX, testY, result.point, result.normal); //NOTE: Just resolves the individual voxel
                result.Pn = 0;
                result.inverseMassNormal = 0;
                result.velocityBias = 0;

                assert(pointCount < MAX_CONTACT_POINTS_PER_PAIR);
                if(pointCount < MAX_CONTACT_POINTS_PER_PAIR) {
                    points[pointCount++] = result;
                }


            }
        }
    }

    return pointCount;
}

gjk_v2 modelSpaceToWorldSpaceGJK(VoxelEntity *e, float x, float y) {
    float16 T = float16_angle_aroundZ(e->T.rotation.z);
    float2 p = plus_float2(float16_transform(T, make_float4(x, y, 0, 1)).xy, e->T.pos.xy); 
    return gjk_V2(p.x, p.y);
}

Gjk_EPA_Info boundingBoxOverlapWithMargin(VoxelEntity *a, VoxelEntity *b, float margin) {
    Rect2f aH = make_rect2f_center_dim(make_float2(0, 0), plus_float2(make_float2(margin, margin), a->worldBounds));
    Rect2f bH = make_rect2f_center_dim(make_float2(0, 0), plus_float2(make_float2(margin, margin), b->worldBounds));

    assert(a != b);

    gjk_v2 pointsA[4] = { 
        modelSpaceToWorldSpaceGJK(a, aH.minX, aH.minY), 
        modelSpaceToWorldSpaceGJK(a, aH.minX, aH.maxY), 
        modelSpaceToWorldSpaceGJK(a, aH.maxX, aH.maxY),  
        modelSpaceToWorldSpaceGJK(a, aH.maxX, aH.minY), 
    };

    gjk_v2 pointsB[4] = { 
        modelSpaceToWorldSpaceGJK(b, bH.minX, bH.minY), 
        modelSpaceToWorldSpaceGJK(b, bH.minX, bH.maxY), 
        modelSpaceToWorldSpaceGJK(b, bH.maxX, bH.maxY),  
        modelSpaceToWorldSpaceGJK(b, bH.maxX, bH.minY), 
    };

    return gjk_objectsCollide_withEPA(pointsA, 4, pointsB, 4);
}


void collideVoxelEntities(PhysicsWorld *physicsWorld, VoxelEntity *a, VoxelEntity *b) {
    int pointCount = 0;
    CollisionPoint points[MAX_CONTACT_POINTS_PER_PAIR];

    Gjk_EPA_Info r = boundingBoxOverlapWithMargin(a, b, BOUNDING_BOX_MARGIN);

    if(r.collided) 
    {
        a->inBounds = true;
        b->inBounds = true;

        //NOTE: Keep the order consistent with the order in the arbiter
        if(a > b) {
            VoxelEntity *temp = b;
            b = a;
            a = temp;
        }
        
        //NOTE: Check corners with corners & edges first
        for(int i = 0; i < getArrayLength(a->corners); i++) {
            float2 corner = a->corners[i];
            int x = corner.x;
            int y = corner.y;
            u8 byte = getByteFromVoxelEntity(a, x, y);
            
            assert(byte & VOXEL_CORNER || byte & VOXEL_EDGE);
            CollisionPoint pointsFound[MAX_CONTACT_POINTS_PER_PAIR];
            int numPointsFound = doesVoxelCollide(physicsWorld, voxelToWorldP(a, x, y), b, x, y, true, pointsFound, a);

            if(numPointsFound > 0) {
                //NOTE: Found a point
                a->data[y*a->stride + x] |= VOXEL_COLLIDING;

                for(int j = 0; j < numPointsFound; ++j) {
                    assert(pointCount < arrayCount(points));
                    if(pointCount < arrayCount(points)) {
                        points[pointCount++] = pointsFound[j];
                    }
                }
            }
        }

        //NOTE: Check corners with corners & edges first
        for(int i = 0; i < getArrayLength(b->corners); i++) {
            float2 corner = b->corners[i];
            int x = corner.x;
            int y = corner.y;
            u8 byte = getByteFromVoxelEntity(b, x, y);
            
            assert(byte & VOXEL_CORNER || byte & VOXEL_EDGE);
            CollisionPoint pointsFound[MAX_CONTACT_POINTS_PER_PAIR];
            int numPointsFound = doesVoxelCollide(physicsWorld, voxelToWorldP(b, x, y), a, x, y, false, pointsFound, b);

            if(numPointsFound > 0) {
                //NOTE: Found a point
                b->data[y*b->stride + x] |= VOXEL_COLLIDING;

                for(int j = 0; j < numPointsFound; ++j) {
                    assert(pointCount < arrayCount(points));
                    if(pointCount < arrayCount(points)) {
                        points[pointCount++] = pointsFound[j];
                    }
                }
            }
        }

        mergePointsToArbiter(physicsWorld, points, pointCount, a, b);
    }
}


void classifyPhysicsShapeAndIntertia(VoxelEntity *e) {
    float inertia = 0;
    float massPerVoxel = (1.0f / e->inverseMass) / (float)e->occupiedCount;
    if(e->corners) {
        freeResizeArray(e->corners);
    }
    e->corners = initResizeArray(float2);

    for(int y = 0; y < e->pitch; y++) {
        for(int x = 0; x < e->stride; x++) {
            u8 flags = e->data[y*e->stride + x];

            if(flags & VOXEL_OCCUPIED) {
                //NOTE: Clear flags
                flags &= ~(VOXEL_CORNER | VOXEL_EDGE | VOXEL_INSIDE);

                float2 modelP = getVoxelPositionInModelSpaceFromCenter(e, make_float2(x, y));
                inertia += massPerVoxel*(modelP.x*modelP.x + modelP.y*modelP.y);

                bool found = false;
                //NOTE: Check whether corner
                float2 corners[4] = {make_float2(1, 1), make_float2(-1, 1), make_float2(1, -1), make_float2(-1, -1)};
                for(int i = 0; i < arrayCount(corners) && !found; ++i) {
                    float2 corner = corners[i];
                    if(!isVoxelOccupied(e, x + corner.x, y + corner.y) && !isVoxelOccupied(e, x + corner.x, y) && !isVoxelOccupied(e, x, y + corner.y)) {
                        flags |= VOXEL_CORNER;
                        float2 a = make_float2(x, y);
                        pushArrayItem(&e->corners, a, float2);
                        found = true;
                    }
                }   
                
                if(!found) {
                    //NOTE: Check whether edge
                    if(!isVoxelOccupied(e, x + 1, y) || !isVoxelOccupied(e, x - 1 , y) || !isVoxelOccupied(e, x, y + 1) || !isVoxelOccupied(e, x, y - 1)) {
                        flags |= VOXEL_EDGE;
                        float2 a = make_float2(x, y);
                        #if USE_EDGES_FOR_COLLISION  
                        pushArrayItem(&e->corners, a, float2);
                        #endif
                        found = true;
                    }
                }

                if(!found) {
                    flags |= VOXEL_INSIDE;
                }

                e->data[y*e->stride + x] = flags;
            }
        }
    }

    if(e->inverseMass != 0) {
        e->invI = 1.0f / inertia;
    } else {
        e->invI = 0;
    }
    
}

VoxelEntity createVoxelCircleEntity(float radius, float3 pos, float inverseMass, int randomStartUpID) {
    VoxelEntity result = {};

    result.id = makeEntityId(randomStartUpID);

    result.sleepTimer = 0;
    result.asleep = false;
    
    result.friction = 0.2f;
    result.T.pos = pos;
    
    result.inverseMass = inverseMass;
    result.coefficientOfRestitution = 0.2f;

    float diameter = 2*radius;
    result.worldBounds = make_float2(diameter, diameter);

    result.T.scale = make_float3(diameter, diameter, 0);
    // result.T.rotation.z = 0.25f*PI32;
    
    int diameterInVoxels = round(diameter*VOXELS_PER_METER);
    int t = (int)(diameterInVoxels*diameterInVoxels);
    result.data = (u8 *)easyPlatform_allocateMemory(sizeof(u8)*t, EASY_PLATFORM_MEMORY_ZERO);

    result.stride = diameterInVoxels;
    result.pitch = diameterInVoxels;

    result.occupiedCount = 0;

    float2 center = make_float2(radius, radius);
    for(int y = 0; y < result.pitch; y++) {
        for(int x = 0; x < result.stride; x++) {
            float2 pos = getVoxelPositionInModelSpace(make_float2(x, y));

            float2 diff = minus_float2(pos, center);

            u8 flags = VOXEL_NONE;

            if(float2_magnitude(diff) <= radius) {
                flags |= VOXEL_OCCUPIED;
                result.occupiedCount++;
            } 

            result.data[y*result.stride + x] = flags;
        }
    }

    classifyPhysicsShapeAndIntertia(&result);

    return result;
}

VoxelEntity createVoxelPlaneEntity(float length, float3 pos, float inverseMass, int randomStartUpID) {
    VoxelEntity result = {};

    result.id = makeEntityId(randomStartUpID);

    result.T.pos = pos;
    result.inverseMass = inverseMass;
    result.coefficientOfRestitution = 0;
    result.friction = 0.2f;

    result.sleepTimer = 0;
    result.asleep = false;

    result.worldBounds = make_float2(length, 30*VOXEL_SIZE_IN_METERS);

    result.T.scale = make_float3(result.worldBounds.x, result.worldBounds.y, 0);

    result.stride = result.worldBounds.x*VOXELS_PER_METER;
    result.pitch = result.worldBounds.y*VOXELS_PER_METER;

    int areaInVoxels = result.stride*result.pitch;
    
    result.data = (u8 *)easyPlatform_allocateMemory(sizeof(u8)*areaInVoxels, EASY_PLATFORM_MEMORY_ZERO);
    result.occupiedCount = 0;

    for(int y = 0; y < result.pitch; y++) {
        for(int x = 0; x < result.stride; x++) {
            result.data[y*result.stride + x] = VOXEL_OCCUPIED;
            result.occupiedCount++;
        }
    }

    classifyPhysicsShapeAndIntertia(&result);

    return result;
}

VoxelEntity createVoxelSquareEntity(float w, float h, float3 pos, float inverseMass, int randomStartUpID) {
    VoxelEntity result = {};

    result.id = makeEntityId(randomStartUpID);

    result.T.pos = pos;
    result.inverseMass = inverseMass;
    result.coefficientOfRestitution = 0.2f;
    result.friction = 0.2f;

    result.sleepTimer = 0;
    result.asleep = false;
    // result.T.rotation.z = 0.25f*PI32;

    result.worldBounds = make_float2(w, h);
    result.T.scale = make_float3(result.worldBounds.x, result.worldBounds.y, 0);

    result.stride = result.worldBounds.x*VOXELS_PER_METER;
    result.pitch = result.worldBounds.y*VOXELS_PER_METER;

    int areaInVoxels = result.stride*result.pitch;

    result.occupiedCount = 0;
    result.data = (u8 *)easyPlatform_allocateMemory(sizeof(u8)*areaInVoxels, EASY_PLATFORM_MEMORY_ZERO);

    for(int y = 0; y < result.pitch; y++) {
        for(int x = 0; x < result.stride; x++) {
            result.data[y*result.stride + x] = VOXEL_OCCUPIED;
            result.occupiedCount++;
        }
    }

    classifyPhysicsShapeAndIntertia(&result);

    return result;
}

struct Block {
    u8 type;

    //NOTE: Local to the Chunk they're in
    int x;
    int y;
    int z;
    volatile uint64_t aoMask; //NOTE: Multiple threads can change this
    //NOTE: Instead of storing this 
    bool exists;

    float timeLeft;
};

struct CloudBlock {
    //NOTE: Local to the Chunk they're in
    int x;
    int z;
};

enum EntityFlags {
    SHOULD_ROTATE = 1 << 0,
    ENTITY_DESTRUCTIBLE = 1 << 1,
    ENTITY_DELETED = 1 << 2,
};

struct Entity {
    EntityID id;

    float3 offset;
    float floatTime;

    float stamina;
    
    TransformX T;
    float3 dP;
    float3 recoverDP; //NOTE: This is a seperate dP that gets applied no matter what i.e. it doesn't get stopped by collisions. It's so if an entity gets stuck in a block it can move out of it.
    EntityType type;

    Rect3f collisionBox;
    bool grounded;
    bool tryJump;
    bool running;

    AnimationState animationState;

    uint32_t flags;
    BlockType itemType;
};

#define CHUNK_DIM 16
#define BLOCKS_PER_CHUNK CHUNK_DIM*CHUNK_DIM*CHUNK_DIM

struct CloudChunk {
    int x;
    int z;

    int cloudCount;
    CloudBlock clouds[CHUNK_DIM*CHUNK_DIM];

    CloudChunk *next;
};


enum ChunkGenerationState {
    CHUNK_NOT_GENERATED = 1 << 0, 
    CHUNK_GENERATING = 1 << 1, 
    CHUNK_GENERATED = 1 << 2, 
    CHUNK_MESH_DIRTY = 1 << 3, 
    CHUNK_MESH_BUILDING = 1 << 4, 
};

struct Chunk {
    int x;
    int y;
    int z;

    volatile int64_t generateState; //NOTE: Chunk might not be generated, so check first when you get one

    //NOTE: 16 x 16 x 16
    //NOTE: Z Y X
    Block *blocks; //NOTE: Could be null

    Entity *entities;

    ChunkModelBuffer modelBuffer;
    ChunkModelBuffer alphaModelBuffer;

    Chunk *next;
};

struct ChunkInfo {
    Chunk *chunk;

    ChunkInfo *next;
};

struct BlockChunkPartner {
    Block *block;
    Chunk *chunk;

    int blockIndex;

    float3 sideNormal;
};

struct Camera {
    TransformX T;
    float fov;
    float targetFov;
    float shakeTimer;
    float runShakeTimer;
    bool followingPlayer;
};

void initBaseEntity(Entity *e, int randomStartUpID) {
    e->id = makeEntityId(randomStartUpID);
    e->T.rotation = make_float3(0, 0, 0);
    e->recoverDP = e->dP = make_float3(0, 0, 0);
    
}

Entity *initPlayer(Entity *e, int randomStartUpID) {
    initBaseEntity(e, randomStartUpID);
    e->T.pos = make_float3(0, 0, 0);
    float playerWidth = 0.7f;
    e->T.scale = make_float3(playerWidth, 1.7f, playerWidth);
    // e->T.scale = make_float3(1, 1, 1);
    
    e->type = ENTITY_PLAYER;
    e->offset = make_float3(0, 0, 0);
    e->grounded = false;
    e->flags = 0;
    e->stamina = 1;
    return e;
}