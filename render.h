struct Vertex {
    float3 pos;
    float2 texUV;
    float3 normal;
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