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
    uint32_t atlasTexture;
    uint32_t fontAtlasTexture;

    int atlasQuadCount;
    InstanceDataWithRotation atlasQuads[MAX_WORLD_ITEMS_PER_INSTANCE];

    int atlasQuadHUDCount;
    InstanceDataWithRotation atlasHUDQuads[MAX_CIRCLES_PER_RENDER];

    int glyphCount; 
    InstanceDataWithRotation glyphData[MAX_GLYPHS_PER_RENDER];

    int checkerQuadCount; 
    InstanceDataWithRotation checkerQuads[1];

    int lineCount; 
    InstanceDataWithRotation lineData[MAX_LINES_PER_RENDER];

    int lineCountScreenSpace; 
    InstanceDataWithRotation lineDataScreenSpace[MAX_LINES_PER_RENDER];

    int selectionCount;
    InstanceDataWithRotation selectionQuad;

    int canvasCount; 
    InstanceDataWithRotation canvasQuads[MAX_RENDER_ITEMS_PER_INSTANCE];

    u32 canvasHandles[MAX_RENDER_ITEMS_PER_INSTANCE];

    uint32_t selectionTextureHandle;

    Shader checkQuadShader;
    Shader quadShader;
    Shader quadTextureShader;
    Shader fontTextureShader;
    Shader lineShader;
    Shader pixelSelectionShader;
    
    ModelBuffer quadModel;
    ModelBuffer lineModel;

    float timeAccum;
    float2 textMatrixResolution;
    bool underWater;
};

InstanceDataWithRotation *pushCheckerQuad(Renderer *renderer, float3 worldP, float3 scale) {
    InstanceDataWithRotation *c = 0;
    if(renderer->checkerQuadCount < arrayCount(renderer->checkerQuads)) {
        c = &renderer->checkerQuads[renderer->checkerQuadCount++];
    } else {
        assert(false);
    }
    
    if(c) {
        float16 T = eulerAnglesToTransform(0, 0, 0);
        c->M = float16_set_pos(float16_scale(T, scale), worldP);
        c->color = make_float4(1, 1, 1, 1);
        c->uv = make_float4(0, 1, 0, 1);
    }

    return c;
}


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

InstanceDataWithRotation *pushLineEndToEndScreenSpace(Renderer *renderer, float2 P1, float2 P2, float4 color) {
    InstanceDataWithRotation *c = 0;
    if(renderer->lineCountScreenSpace < arrayCount(renderer->lineDataScreenSpace)) {
        c = &renderer->lineDataScreenSpace[renderer->lineCountScreenSpace++];
    } else {
        assert(false);
    }
    
    if(c) {
        float2 offsetPosition = scale_float2(0.5f, plus_float2(P1, P2));
        float length = float2_magnitude(minus_float2(P2, P1));
        float3 scale = make_float3(length, 1.0f, 1.0f);

        TransformX T = {};
        T.pos.xy = offsetPosition;
        T.scale = scale;

        float2 diff = minus_float2(P2, offsetPosition);
        T.rotation = make_float3(0, 0, ATan2_0to360(diff.y, diff.x));

        c->M = getModelToViewSpace(T);
        c->color = color;
    }

    return c;
}

InstanceDataWithRotation *pushLineEndToEndWorldSpace(Renderer *renderer, float2 P1, float2 P2, float4 color) {
    InstanceDataWithRotation *c = 0;
    if(renderer->lineCount < arrayCount(renderer->lineData)) {
        c = &renderer->lineData[renderer->lineCount++];
    } else {
        assert(false);
    }
    
    if(c) {
        float2 offsetPosition = scale_float2(0.5f, plus_float2(P1, P2));
        float length = float2_magnitude(minus_float2(P2, P1));
        float3 scale = make_float3(length, 1.0f, 1.0f);

        TransformX T = {};
        T.pos.xy = offsetPosition;
        T.scale = scale;

        float2 diff = minus_float2(P2, offsetPosition);
        T.rotation = make_float3(0, 0, ATan2_0to360(diff.y, diff.x));

        c->M = getModelToViewSpace(T);
        c->color = color;
    }

    return c;
}

void pushGlyph(Renderer *renderer, float3 worldP, float3 scale, float4 uvs, float4 color) {
    pushGlyphQuad_(renderer, worldP, scale, uvs, color);
}
void pushRect(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 1), make_float3(0, 0, 0), make_float4(0.5f, 0.75f, 0.75, 1.0f), color, true);
}


void pushColoredQuad(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(scale.x, scale.y, 0), make_float3(0, 0, 0), make_float4(0.12f, 0.35f, 0.65, 0.75f), color, false);
}

void pushSelectionQuad(Renderer *renderer, float3 worldP, float2 scale, float4 color) {
    float16 T = eulerAnglesToTransform(0, 0, 0);
    renderer->selectionQuad.M = float16_set_pos(float16_scale(T, make_float3(scale.x, scale.y, 0)), worldP);
    renderer->selectionQuad.color = color;
    renderer->selectionQuad.uv = make_float4(0, 1, 0, 1);
    renderer->selectionCount++;
}

void pushCanvasQuad(Renderer *renderer, float3 worldP, float2 scale, float4 color, u32 handle) {
    int index = -1;
    InstanceDataWithRotation *c = 0;
    if(renderer->canvasCount < arrayCount(renderer->canvasQuads)) {
        index = renderer->canvasCount++;
        c = &renderer->canvasQuads[index];
    } else {
        assert(false);
    }
    
    if(c && index >= 0) {
        float16 T = eulerAnglesToTransform(0, 0, 0);
        c->M = float16_set_pos(float16_scale(T, make_float3(scale.x, scale.y, 0)), worldP);
        c->color = color;
        c->uv = make_float4(0, 1, 1, 0);
        
        renderer->canvasHandles[index] = handle;
    }
}

void pushCircleOutline(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float3(0, 0, 0), make_float4(0.5f, 1.0f, 0, 0.5f), color, false);
}


void pushRotationCircle(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float3(0, 0, 0), make_float4(0.75, 1.0f, 0.5f, 0.75f), color, false);
}

void pushFillCircle(Renderer *renderer, float3 worldP, float radius, float4 color) {
    pushAtlasQuad_(renderer, worldP, make_float3(radius, radius, 1), make_float3(0, 0, 0), make_float4(0, 0.5f, 0.5f, 1.0f), color, false);
}
