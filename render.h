struct Vertex {
    float3 pos;
    float2 texUV;
    float3 normal;
};

struct VertexForChunk {
    float3 pos;
    float2 texUV;
    float3 normal;
    int aoMask;
};

struct VertexWithJoints {
    float3 pos;
    float2 texUV;
    float3 normal;
    float4 jointWeights;
    int jointIndexes[4];
};

struct VertexWithMeshIDs {
    float3 pos;
    float2 texUV;
    float3 normal;
    int meshIndex;
};

struct VertexFullScreen {
    float3 pos;
    float3 normal;
};

Vertex makeVertex(float3 pos, float2 texUV, float3 normal) {
    Vertex v = {};
    
    v.pos = pos;
    v.texUV = texUV;
    v.normal = normal;

    return v;
}

VertexForChunk makeVertexForChunk(float3 pos, float2 texUV, float3 normal) {
    VertexForChunk v = {};
    
    v.pos = pos;
    v.texUV = texUV;
    v.normal = normal;
    v.aoMask = 0;

    return v;
}


struct ChunkModelBuffer {
    uint32_t handle;
    int indexCount;
};