
static char *blockPickupVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"

   " uv_frag = vec2(texUV.x, mix(uvAtlas.x, uvAtlas.y, texUV.y));"
"}";

static char *lineVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"
"}";


static char *lineFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"out vec4 color;"
"void main() {"
    "color = color_frag;"
"}";


static char *skeletalVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"
"in ivec4 jointIndexes;"
"in vec4 jointWeights;"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"
"uniform samplerBuffer boneMatrixBuffer;"

"mat4 getBoneMatrix(int index) {"
    "return mat4(texelFetch(boneMatrixBuffer, index * 4),"
                "texelFetch(boneMatrixBuffer, index * 4 + 1),"
                "texelFetch(boneMatrixBuffer, index * 4 + 2),"
                "texelFetch(boneMatrixBuffer, index * 4 + 3));"
"}"

"vec4 getSkinnedPosition(vec4 p) {"
    "vec4 skinnedPosition = vec4(0.0);"

    "float total = jointWeights[0] + jointWeights[1] + jointWeights[2] + jointWeights[3];"

    "if(total != 1) {"
        "skinnedPosition = p;"
    "} else {"
        "skinnedPosition += (getBoneMatrix(jointIndexes[0]) * p) * jointWeights[0];"
        "skinnedPosition += (getBoneMatrix(jointIndexes[1]) * p) * jointWeights[1];"
        "skinnedPosition += (getBoneMatrix(jointIndexes[2]) * p) * jointWeights[2];"
        "skinnedPosition += (getBoneMatrix(jointIndexes[3]) * p) * jointWeights[3];"
    "}"

    "return skinnedPosition;"
"}"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * getSkinnedPosition(vec4(vertex, 1));"
    "color_frag = color;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"

   " uv_frag = vec2(texUV.x, mix(uvAtlas.x, uvAtlas.y, texUV.y));"
"}";

static char *skeletalFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normal_frag_view_space;"//viewspace
"in vec2 uv_frag; "
"in vec3 sunAngle;"
"in vec3 fragPosInViewSpace;" //view space
"uniform sampler2D diffuse;"
"uniform vec3 lookingAxis;"

"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "vec4 c = color_frag;"
    "color = diffSample*c;"
"}";

static char *blockSameTextureVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"

   " uv_frag = vec2(mix(uvAtlas.x, uvAtlas.y, texUV.x), texUV.y);"
"}";

static char *blockPickupFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normal_frag_view_space;"//viewspace
"in vec2 uv_frag; "
"in vec3 sunAngle;"
"in vec3 fragPosInViewSpace;" //view space
"uniform sampler2D diffuse;"
"uniform vec3 lookingAxis;"

"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "vec4 c = color_frag;"
    "color = diffSample*c;"
"}";

static char *blockVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

"in uvec2 AOMask;"

//per instanced variables
"in vec3 pos;"
"in vec2 uvAtlas;"
"in vec4 color;"
"in uint samplerIndex;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"
"out vec3 normalInModelSpace;"
"out float distanceFromEye;"
"out float AOValue;"

"float aoFactors[4] = float[4](1, 0.7, 0.5, 0.3);"

"void main() {"
    "mat4 MV = V;"
    "vec4 cameraSpace = MV * vec4((vertex + pos), 1);"
    "distanceFromEye = cameraSpace.z;"
    "gl_Position = projection * cameraSpace;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"
    "normalInModelSpace = normal;"
    "uint aoIndex = uint(3);"
    "uint aoMask = AOMask.x >> uint(gl_VertexID*2);"
    "if(gl_VertexID >= 16) {"
        "aoMask = AOMask.y >> (uint(2*(gl_VertexID - 16)));"
    "}"
    "AOValue = aoFactors[aoMask & aoIndex];"
    // "color_frag = vec4(AOValue, 0, 0, 1);"//color;"
    "color_frag = color;"

   "uv_frag = vec2(texUV.x, mix(uvAtlas.x, uvAtlas.y, texUV.y));"
//    "samplerIndexOut = float(samplerIndex);"
"}";

static char *blockGreedyVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"
"in int AOMask;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec3 normal_frag_view_space;"
"out vec2 uv_frag;"
"out vec3 fragPosInViewSpace;"
"out vec3 sunAngle;"
"out vec3 normalInModelSpace;"
"out float distanceFromEye;"
"out float AOValue;"

"float aoFactors[4] = float[4](1, 0.7, 0.5, 0.3);"

"void main() {"
    "mat4 MV = V;"
    "vec4 cameraSpace = MV * vec4(vertex, 1);"
    "distanceFromEye = cameraSpace.z;"
    "gl_Position = projection * cameraSpace;"
    "normal_frag_view_space = mat3(transpose(inverse(MV))) * normal;"
    "sunAngle = mat3(transpose(inverse(MV))) * vec3(0.7071, 0, 0.7071);"
    "fragPosInViewSpace = vec3(MV * vec4(vertex, 1));"
    "normalInModelSpace = normal;"
    "AOValue = aoFactors[AOMask];"
    "color_frag = vec4(0, 0, 0, 1);"
    "uv_frag = texUV;"
"}";

static char *blockFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normal_frag_view_space;"//viewspace
"in vec2 uv_frag; "
"in vec3 sunAngle;"
"in vec3 fragPosInViewSpace;" //view space
"in float AOValue;"
"in float distanceFromEye;"
"uniform sampler2D diffuse;"
"uniform vec3 lookingAxis;"
"uniform vec4 fogColor;"
"uniform float fogSeeDistance;"
"uniform float fogFadeDistance;"

"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "if(diffSample.w == 0) {"
        "discard;"
    "}"
    "float mixValue = max(dot(normal_frag_view_space, sunAngle), 0);"
    "vec4 c = color_frag;"
    "float darkness = 0.95;"
    "if(color_frag.x == 1) {"
        // "c = color_frag*1.5;"
        "c = vec4(1.5f, 1.5f, 1.5f, 1.5f);"
    "} else {"
        "c = vec4(1, 1, 1, 1);"
        // "c = mix(vec4(darkness, darkness, darkness, 1), vec4(1, 1, 1, 1), mixValue);"
    "}"
    "c = vec4(AOValue*c.xyz, c.w);"
    "vec4 fogFactor = mix(diffSample*c, vec4(fogColor), clamp(((distanceFromEye - fogSeeDistance) / fogFadeDistance), 0.0, 1.0));"
    // "vec4 fogFactor = diffSample*c;"
    
    "color = vec4((fogFactor).xyz, 1);"//fogFactor
"}";

static char *blockColorShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec3 normalInModelSpace;"
"in float distanceFromEye;"
"out vec4 color;"
"float factors[2] = float[](1, 0.7f);"
"void main() {"
    "float d = dot(vec3(0, -1, 0), normalInModelSpace);"
    "int index = int(clamp(d, 0, 1));"
    "float factor = factors[index];"
    "vec4 c = vec4(color_frag.x*factor, color_frag.y*factor, color_frag.z*factor, color_frag.w);"
    "color = mix(c, vec4(0.8, 0.8, 0.8, 1), (distanceFromEye - 10) / 50);"
"}";

static char *fullScreenVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"

//outgoing variables
"out vec3 rayDir;"

"void main() {"
    "rayDir = normal;"
    "gl_Position = vec4(vertex, 1);"
"}";

static char *rayCastFragShader = 
"#version 330\n"
"in vec3 rayDir; "
"out vec4 color;"
"void main() {"
    "color = vec4(1, 0, 0, 1);"
"}";

static char *quadVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"

   "uv_frag = vec2(mix(uvAtlas.x, uvAtlas.y, texUV.x), mix(uvAtlas.z, uvAtlas.w, texUV.y));"
"}";

static char *quadTextureFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "if(diffSample.w == 0) {"
        "discard;"
    "}"
    "color = diffSample*color_frag;"
"}";

static char *fontTextureFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "color = vec4(diffSample.r);"
"}";

static char *quadFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"out vec4 color;"
"void main() {"
    "color = color_frag;"
"}";

static char *skyboxVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec3 uv_frag;"

"void main() {"
    "uv_frag = vertex;"
    "vec4 pos = projection * V * vec4(vertex, 1);"
    "gl_Position = pos.xyww;"
"}";

static char *skyboxFragShader = 
"#version 330\n"
"in vec3 uv_frag; "
"uniform samplerCube diffuse;"
"uniform vec4 skyColorA;"
"uniform vec4 skyColorB;"
"out vec4 color;"
"void main() {"
"float value = dot(normalize(uv_frag), vec3(0, 1, 0));"
"color = mix(skyColorA, skyColorB, value);"
"}";
