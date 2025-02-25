#define MAX_CUBES_PER_RENDER 150000000
#define MAX_CIRCLES_PER_RENDER 500000
#define MAX_RENDER_ITEMS_PER_INSTANCE 32
#define MAX_WORLD_ITEMS_PER_INSTANCE 500000
#define MAX_GLYPHS_PER_RENDER 50000
#define MAX_LINES_PER_RENDER 50000

struct AtlasAsset {
    char *name;
    float4 uv;

    AtlasAsset *next;
};

static Vertex global_quadData[] = {
    makeVertex(make_float3(0.5f, -0.5f, 0), make_float2(1, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, -0.5f, 0), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f,  0.5f, 0), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0), make_float2(1, 0), make_float3(0, 0, 1)),
};

static unsigned int global_quadIndices[] = {
    0, 1, 2, 0, 2, 3,
};

#define HALF_SIN_45 0.3535533906f

static unsigned int global_lineIndicies[] = {
    0, 1
};

static Vertex global_lineModelData[] = {
    makeVertex(make_float3(-0.5f, 0, 0), make_float2(1, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0, 0), make_float2(0, 1), make_float3(0, 0, 1)),
    
};

static VertexForChunk global_quadDataChunkData[] = {
    makeVertexForChunk(make_float3(0.5f, -0.5f, 0), make_float2(1, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-0.5f, -0.5f, 0), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-0.5f,  0.5f, 0), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(0.5f, 0.5f, 0), make_float2(1, 0), make_float3(0, 0, 1)),
};

static VertexForChunk global_quadDataChunkDataRotate90AntiClockwise[] = {
    makeVertexForChunk(make_float3(HALF_SIN_45, -0.5f, -HALF_SIN_45), make_float2(0.25f, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-HALF_SIN_45, -0.5f, HALF_SIN_45), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-HALF_SIN_45,  0.5f, HALF_SIN_45), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(HALF_SIN_45, 0.5f, -HALF_SIN_45), make_float2(0.25f, 0), make_float3(0, 0, 1)),
};

static VertexForChunk global_quadDataChunkDataRotate90Clockwise[] = {
    makeVertexForChunk(make_float3(HALF_SIN_45, -0.5f, HALF_SIN_45), make_float2(0.25f, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-HALF_SIN_45, -0.5f, -HALF_SIN_45), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-HALF_SIN_45,  0.5f, -HALF_SIN_45), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(HALF_SIN_45, 0.5f, HALF_SIN_45), make_float2(0.25f, 0), make_float3(0, 0, 1)),
};

static VertexForChunk global_cubeDataForChunk[] = {
    // Top face (y = 1.0f)
    makeVertexForChunk(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 1, 0)),
    makeVertexForChunk(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f, 0), make_float3(0, 1, 0)),
    makeVertexForChunk(make_float3(0.5f, 0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, 1, 0)),
    makeVertexForChunk(make_float3(-0.5f, 0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, 1, 0)),
    // Bottom face (y = -1.0f)
    makeVertexForChunk(make_float3(0.5f, -0.5f, -0.5f), make_float2(0.75f, 0), make_float3(0, -1, 0)),
    makeVertexForChunk(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0.75f, 1), make_float3(0, -1, 0)),
    makeVertexForChunk(make_float3(-0.5f, -0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, -1, 0)),
    makeVertexForChunk(make_float3(0.5f, -0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, -1, 0)),
    // Front face  (z = 1.0f)
    makeVertexForChunk(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(0.5f, -0.5f, 0.5f), make_float2(0.25f,1), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(0.5f, 0.5f, 0.5f), make_float2(0.25f,0), make_float3(0, 0, 1)),
    makeVertexForChunk(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(0, 0, 1)),
    // Back face (z = -1.0f)
    makeVertexForChunk(make_float3(0.5f, -0.5f, -0.5f), make_float2(0,1), make_float3(0, 0, -1)),
    makeVertexForChunk(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 0, -1)),
    makeVertexForChunk(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,0), make_float3(0, 0, -1)),
    makeVertexForChunk(make_float3(0.5f, 0.5f, -0.5f), make_float2(0,0), make_float3(0, 0, -1)),
    // Left face (x = -1.0f)
    makeVertexForChunk(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(-1, 0, 0)),
    makeVertexForChunk(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0, 1), make_float3(-1, 0, 0)),
    makeVertexForChunk(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0.25f,1), make_float3(-1, 0, 0)),
    makeVertexForChunk(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0.25f,0), make_float3(-1, 0, 0)),
    // Right face (x = 1.0f)
    makeVertexForChunk(make_float3(0.5f, -0.5f, -0.5f), make_float2(0.25f,1), make_float3(1, 0, 0)),
    makeVertexForChunk(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f,0), make_float3(1, 0, 0)),
    makeVertexForChunk(make_float3(0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(1, 0, 0)),
    makeVertexForChunk(make_float3(0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(1, 0, 0)),
};

static Vertex global_cubeData[] = {
    // Top face (y = 1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(-0.5f, 0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, 1, 0)),
    // Bottom face (y = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0.75f, 0), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0.75f, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f,  0.5f), make_float2(0.5f, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(0.5f, -0.5f,  0.5f), make_float2(0.5f, 0), make_float3(0, -1, 0)),
    // Front face  (z = 1.0f)
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0.25f,1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(0.25f,0), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(0, 0, 1)),
    // Back face (z = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0.25f,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0.25f,0), make_float3(0, 0, -1)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0,0), make_float3(0, 0, -1)),
    // Left face (x = -1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0, 1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0.25f,1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0.25f,0), make_float3(-1, 0, 0)),
    // Right face (x = 1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0.25f,1), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0.25f,0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(1, 0, 0)),
};


static Vertex global_cubeData_sameTexture[] = {
    // Top face (y = 1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0,1), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(0.5f, 0.5f,  0.5f), make_float2(1, 0), make_float3(0, 1, 0)),
    makeVertex(make_float3(-0.5f, 0.5f,  0.5f), make_float2(1, 1), make_float3(0, 1, 0)),
    // Bottom face (y = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(1, 0), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(1, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(-0.5f, -0.5f,  0.5f), make_float2(0, 1), make_float3(0, -1, 0)),
    makeVertex(make_float3(0.5f, -0.5f,  0.5f), make_float2(0, 0), make_float3(0, -1, 0)),
    // Front face  (z = 1.0f)
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(1,1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(1,0), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(0, 0, 1)),
    // Back face (z = -1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(0,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(1,1), make_float3(0, 0, -1)),
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(1,0), make_float3(0, 0, -1)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(0,0), make_float3(0, 0, -1)),
    // Left face (x = -1.0f)
    makeVertex(make_float3(-0.5f, 0.5f, -0.5f), make_float2(0, 0), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, -0.5f), make_float2(0, 1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, -0.5f, 0.5f), make_float2(1,1), make_float3(-1, 0, 0)),
    makeVertex(make_float3(-0.5f, 0.5f, 0.5f), make_float2(1,0), make_float3(-1, 0, 0)),
    // Right face (x = 1.0f)
    makeVertex(make_float3(0.5f, -0.5f, -0.5f), make_float2(1,1), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, -0.5f), make_float2(1,0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, 0.5f), make_float2(0, 0), make_float3(1, 0, 0)),
    makeVertex(make_float3(0.5f, -0.5f, 0.5f), make_float2(0, 1), make_float3(1, 0, 0)),
};

static unsigned int global_cubeIndices[] = {
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7,
    8, 9, 10, 8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};


struct Shader {
    bool valid;
    uint32_t handle;
};

struct InstanceData {
    float3 pos;
    float2 uv;
    float4 color;
    float3 scale;
    uint32_t AOMask[2];
    uint32_t samplerIndex;
};

struct InstanceDataWithRotation {
    float16 M;
    float4 color;
    float4 uv;
};

struct Renderer {
    uint32_t terrainTextureHandle;
    // uint32_t circleHandle;
    // uint32_t circleOutlineHandle;
    // uint32_t skyboxTextureHandle;
    uint32_t breakBlockTexture;
    uint32_t atlasTexture;
    uint32_t fontAtlasTexture;

    int cubeCount;
    InstanceData cubeData[MAX_CUBES_PER_RENDER];

    int atlasQuadCount;
    InstanceDataWithRotation atlasQuads[MAX_WORLD_ITEMS_PER_INSTANCE];

    int atlasQuadHUDCount;
    InstanceDataWithRotation atlasHUDQuads[MAX_CIRCLES_PER_RENDER];

    int triangleCount;
    InstanceData triangleData[MAX_RENDER_ITEMS_PER_INSTANCE];

    int blockItemsCount; //NOTE: The blocks that are rotating
    InstanceDataWithRotation blockItemsData[MAX_WORLD_ITEMS_PER_INSTANCE];

    int alphaBlockCount; //NOTE: Blocks that have an alpha component 
    InstanceData alphaBlockData[MAX_CUBES_PER_RENDER];

    int alphaItemCount; //NOTE: The blocks that are rotating
    InstanceDataWithRotation alphaItemData[MAX_WORLD_ITEMS_PER_INSTANCE];

    int modelItemCount; //NOTE: The blocks that are rotating
    InstanceDataWithRotation modelData[MAX_WORLD_ITEMS_PER_INSTANCE];

    int glyphCount; 
    InstanceDataWithRotation glyphData[MAX_GLYPHS_PER_RENDER];

    int lineCount; 
    InstanceDataWithRotation lineData[MAX_LINES_PER_RENDER];

    Shader blockShader;
    Shader blockGreedyShader;
    Shader blockPickupShader;
    Shader skeletalModelShader;
    Shader quadShader;
    Shader quadTextureShader;
    Shader fontTextureShader;
    Shader lineShader;
    Shader skyboxShader;
    Shader blockColorShader;
    Shader blockSameTextureShader;
    Shader rayCastShader;
    
    ModelBuffer blockModel;
    ModelBuffer blockModelWithInstancedT;
    ModelBuffer quadModel;
    ModelBuffer triangleModel;
    ModelBuffer avocadoModel;
    ModelBuffer blockModelSameTexture;
    ModelBuffer lineModel;

    bool underWater;
};

InstanceDataWithRotation *pushAtlasQuad_(Renderer *renderer, float3 worldP, float3 scale, float3 rotation, float4 uvs, float4 color, bool isHUD) {
    InstanceDataWithRotation *c = 0;
    if(isHUD) {
        if(renderer->atlasQuadHUDCount < arrayCount(renderer->atlasHUDQuads)) {
            c = &renderer->atlasHUDQuads[renderer->atlasQuadHUDCount++];
        } else {
            assert(false);
        }
    } else {
        if(renderer->atlasQuadCount < arrayCount(renderer->atlasQuads)) {
            c = &renderer->atlasQuads[renderer->atlasQuadCount++];
        } else {
            assert(false);
        }
    }
    
    if(c) {
        float16 T = eulerAnglesToTransform(rotation.y, rotation.x, rotation.z);
        c->M = float16_set_pos(float16_scale(T, scale), worldP);
        c->color = color;
        c->uv = uvs;
    }

    return c;
}

InstanceDataWithRotation *pushGlyphQuad_(Renderer *renderer, float3 worldP, float3 scale, float4 uvs, float4 color) {
    InstanceDataWithRotation *c = 0;
    if(renderer->glyphCount < arrayCount(renderer->glyphData)) {
        c = &renderer->glyphData[renderer->glyphCount++];
    } else {
        assert(false);
    }
    
    
    if(c) {
        float16 T = float16_identity();
        c->M = float16_set_pos(float16_scale(T, scale), worldP);
        c->color = color;
        c->uv = uvs;
    }

    return c;
}

InstanceDataWithRotation *pushLine(Renderer *renderer, float16 T, float4 color) {
    InstanceDataWithRotation *c = 0;
    if(renderer->lineCount < arrayCount(renderer->lineData)) {
        c = &renderer->lineData[renderer->lineCount++];
    } else {
        assert(false);
    }
    
    if(c) {
        c->M = T;
        c->color = color;
    }

    return c;
}


float2 getUVCoordForBlock(BlockType type) {
    float2 uv = make_float2(0, 0);
     if(type == BLOCK_GRASS) {
        uv.x = 0;
        uv.y = 0.1f;
    } else if(type == BLOCK_SOIL) {
        uv.x = 0.1f;
        uv.y = 0.2f;
    } else if(type == BLOCK_STONE) {
        uv.x = 0.2f;
        uv.y = 0.3f;
    } else if(type == BLOCK_TREE_LEAVES) {
        uv.x = 0.3f;
        uv.y = 0.4f;
    } else if(type == BLOCK_TREE_WOOD) {
        uv.x = 0.4f;
        uv.y = 0.5f;
    } else if(type == BLOCK_IRON) {
        uv.x = 0.5f;
        uv.y = 0.6f;
    } else if(type == BLOCK_COAL) {
        uv.x = 0.6f;
        uv.y = 0.7f;
    } else if(type == BLOCK_GRASS_SHORT_ENTITY || type == BLOCK_GRASS_TALL_ENTITY) {
        uv.x = 0.7f;
        uv.y = 0.8f;
    } else if(type == BLOCK_WATER) {
        uv.x = 0.8f;
        uv.y = 0.9f;
    } else if(type == BLOCK_OUTLINE) {
        uv.x = 0.9f;
        uv.y = 1.0f;
    }
    
    return uv;
}

void pushGrassQuad(Renderer *renderer, float3 worldP, float height, float4 color) {
    if(height == 2) {
        worldP.y += 0.25f;
    }
    
    pushAtlasQuad_(renderer, worldP, make_float3(1, height, 1), make_float3(0, -45, 0), make_float4(0, 0.25f, 0, 0.25f), color, false);
    pushAtlasQuad_(renderer, worldP, make_float3(1, height, 1), make_float3(0, 45, 0), make_float4(0, 0.25f, 0, 0.25f), color, false);
}

void pushWaterQuad(Renderer *renderer, float3 worldP, float4 color) {
    pushAtlasQuad_(renderer, plus_float3(worldP, make_float3(0, 0.5f, 0)), make_float3(1, 1, 1), make_float3(-90, 90, 0), make_float4(0.25f, 0.5f, 0, 0.25f), color, false);
}

void pushGlyph(Renderer *renderer, float3 worldP, float3 scale, float4 uvs, float4 color) {
    pushGlyphQuad_(renderer, worldP, scale, uvs, color);
}

void pushHUDOutline(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 1), make_float3(0, 0, 0), make_float4(0.0, 0.25f, 0.25f, 0.5f), color, true);
}

void pushSpriteForInventoryType(Renderer *renderer, float3 worldP, float2 scale, float4 color, BlockType type) {
    float4 coords = make_float4(0.0, 0.25f, 0, 0.25f);

    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 1), make_float3(0, 0, 0), coords, color, true);
}

void pushSprite(Renderer *renderer, AtlasAsset *assetItem, float3 worldP, float2 scale, float4 color) {
    float4 coords = assetItem->uv;

    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 1), make_float3(0, 0, 0), coords, color, true);
}

void pushPlainQuadHUD(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 1), make_float3(0, 0, 0), make_float4(0.25f, 0.5f, 0, 0.25f), color, true);
}

void pushColoredQuad(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 0), make_float3(0, 0, 0), make_float4(0.12f, 0.35f, 0.65, 0.75f), color, false);
}

void pushCircleOutline(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float3(0, 0, 0), make_float4(0.5f, 1.0f, 0, 0.5f), color, false);
}

void pushFillCircle(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float3(0, 0, 0), make_float4(0, 0.5f, 0.5f, 1.0f), color, true);
}

void pushCube(Renderer *renderer, float3 worldP, BlockType type, float4 color, uint64_t AOMask) {
    if(renderer->cubeCount < arrayCount(renderer->cubeData)) {
        InstanceData *cube = &renderer->cubeData[renderer->cubeCount++];

        cube->pos = worldP;
        uint32_t samplerIndex = 0;
        cube->uv = getUVCoordForBlock(type);

        cube->samplerIndex = samplerIndex;

        cube->color = color;
        
        //NOTE: Convert the AO mask into two integers since openGL doesn't support uint64 in the shader attributes
        cube->AOMask[0] = (uint32_t)(AOMask);
        cube->AOMask[1] = (uint32_t)(AOMask >> 32);
    } else {
        assert(false);
    }
}

void pushAlphaCube(Renderer *renderer, float3 worldP, BlockType type, float4 color) {
    if(renderer->alphaBlockCount < arrayCount(renderer->alphaBlockData)) {
        InstanceData *cube = &renderer->alphaBlockData[renderer->alphaBlockCount++];

        cube->pos = worldP;
        cube->color = color;
        cube->uv = getUVCoordForBlock(type);
    } else {
        assert(false);
    }
}

void pushAlphaItem(Renderer *renderer, float3 worldP, float3 scale, float4 color, float breakPercent) {
    if(renderer->alphaItemCount < arrayCount(renderer->alphaItemData)) {
        InstanceDataWithRotation *cube = &renderer->alphaItemData[renderer->alphaItemCount++];

        cube->M = float16_set_pos(float16_scale(float16_identity(), scale), worldP);
        cube->color = color;

        float numOfPics = 8;
        int indexAt = (int)(breakPercent * numOfPics);

        if(indexAt >= numOfPics) {
            indexAt = numOfPics - 1;
        }

        cube->uv.x = indexAt / numOfPics;
        cube->uv.y = (indexAt + 1) / numOfPics;
    } else {
        assert(false);
    }
}

void pushModel(Renderer *renderer, float16 T, float4 color) {
    if(renderer->modelItemCount < arrayCount(renderer->modelData)) {
        InstanceDataWithRotation *model = &renderer->modelData[renderer->modelItemCount++];

        model->M = T;
        model->uv = make_float4(0, 1, 0, 1);
        model->color = color;

    } else {
        assert(false);
    }
}


void pushBlockItem(Renderer *renderer, float16 T, BlockType type, float4 color) {
    if(renderer->blockItemsCount < arrayCount(renderer->blockItemsData)) {
        InstanceDataWithRotation *cube = &renderer->blockItemsData[renderer->blockItemsCount++];

        cube->M = T;
        float2 uvs = getUVCoordForBlock(type);
        cube->uv = make_float4(uvs.x, uvs.y, 0, 1);
        cube->color = color;

    } else {
        assert(false);
    }
}

void pushTriangle(Renderer *renderer, float3 worldP, float4 color) {
    if(renderer->triangleCount < MAX_CUBES_PER_RENDER) {
        InstanceData *t = &renderer->triangleData[renderer->triangleCount++];

        t->pos = worldP;

        t->uv.x = 0;
        t->uv.y = 1;

        t->scale = make_float3(4, 4, 4);
        
        t->color = color;
    } else {
        assert(false);
    }
}

