#define STB_IMAGE_IMPLEMENTATION
#include "./libs/stb_image.h"
#include <OpenGL/gl3.h>
#include "./shaders/shaders_opengl.cpp"

// NOTE: Each location index in a vertex attribute index - i.e. 4 floats. that's why for matrix we skip 4 values
#define VERTEX_ATTRIB_LOCATION 0
#define NORMAL_ATTRIB_LOCATION 1
#define UV_ATTRIB_LOCATION 2
#define POS_ATTRIB_LOCATION 3
#define UVATLAS_ATTRIB_LOCATION 4
#define COLOR_ATTRIB_LOCATION 5
#define SCALE_ATTRIB_LOCATION 6
#define AO_MASK_ATTRIB_LOCATION 7
#define SAMPLER_INDEX_ATTRIB_LOCATION 8
#define JOINT_WEIGHTS 9
#define JOINT_INDEXES 10
#define MODEL_TRANSFORM_ATTRIB_LOCATION 11


#define renderCheckError() renderCheckError_(__LINE__, (char *)__FILE__)
void renderCheckError_(int lineNumber, char *fileName) {
    #define RENDER_CHECK_ERRORS 1
    #if RENDER_CHECK_ERRORS
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        assert(!err);
    }
    #endif
}

struct FrameBuffer {
    uint32_t handle;
    uint32_t textureHandle;
};

void rendererBindFrameBuffer(FrameBuffer *b) {
    glBindFramebuffer(GL_FRAMEBUFFER, b->handle);  
}

FrameBuffer createFrameBuffer(int width, int height, void *data = 0) {
    FrameBuffer result;
    glGenFramebuffers(1, &result.handle);
    glBindFramebuffer(GL_FRAMEBUFFER, result.handle);  

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    result.textureHandle = texture;
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0); 

     glBindTexture(GL_TEXTURE_2D, 0);    

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        assert(false);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);  

    return result;
}

void deleteFrameBuffer(FrameBuffer *buffer) {
    glDeleteFramebuffers(1, &buffer->handle);
}

Shader loadShader(char *vertexShader, char *fragShader) {
    Shader result = {};
    
    result.valid = true;
    
    GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    renderCheckError();
    GLuint fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    renderCheckError();
    
    glShaderSource(vertexShaderHandle, 1, (const GLchar **)(&vertexShader), 0);
    renderCheckError();
    glShaderSource(fragShaderHandle, 1, (const GLchar **)(&fragShader), 0);
    renderCheckError();
    
    glCompileShader(vertexShaderHandle);
    renderCheckError();
    glCompileShader(fragShaderHandle);
    renderCheckError();
    result.handle = glCreateProgram();
    renderCheckError();
    glAttachShader(result.handle, vertexShaderHandle);
    renderCheckError();
    glAttachShader(result.handle, fragShaderHandle);
    renderCheckError();

    int max_attribs;
    glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &max_attribs);

    glBindAttribLocation(result.handle, VERTEX_ATTRIB_LOCATION, "vertex");
    renderCheckError();
    glBindAttribLocation(result.handle, NORMAL_ATTRIB_LOCATION, "normal");
    renderCheckError();
    glBindAttribLocation(result.handle, UV_ATTRIB_LOCATION, "texUV");
    renderCheckError();
    glBindAttribLocation(result.handle, POS_ATTRIB_LOCATION, "pos");
    renderCheckError();
    glBindAttribLocation(result.handle, UVATLAS_ATTRIB_LOCATION, "uvAtlas");
    renderCheckError();
    glBindAttribLocation(result.handle, COLOR_ATTRIB_LOCATION, "color");
    renderCheckError();
    glBindAttribLocation(result.handle, SCALE_ATTRIB_LOCATION, "scale");
    renderCheckError();
    glBindAttribLocation(result.handle, AO_MASK_ATTRIB_LOCATION, "AOMask");
    renderCheckError();
    glBindAttribLocation(result.handle, SAMPLER_INDEX_ATTRIB_LOCATION, "samplerIndex");
    renderCheckError();
    glBindAttribLocation(result.handle, MODEL_TRANSFORM_ATTRIB_LOCATION, "M");
    renderCheckError();
    glBindAttribLocation(result.handle, JOINT_WEIGHTS, "jointWeights");
    renderCheckError();
    glBindAttribLocation(result.handle, JOINT_INDEXES, "jointIndexes");
    renderCheckError();

    assert(JOINT_INDEXES < (max_attribs - 1));

    glLinkProgram(result.handle);
    renderCheckError();
    glUseProgram(result.handle);
    
    GLint success = 0;
    glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);
    
    GLint success1 = 0;
    glGetShaderiv(fragShaderHandle, GL_COMPILE_STATUS, &success1); 
    
    if(success == GL_FALSE || success1 == GL_FALSE) {
        result.valid = false;
        int  vlength,    flength,    plength;
        char vlog[2048];
        char flog[2048];
        char plog[2048];
        glGetShaderInfoLog(vertexShaderHandle, 2048, &vlength, vlog);
        glGetShaderInfoLog(fragShaderHandle, 2048, &flength, flog);
        glGetProgramInfoLog(result.handle, 2048, &plength, plog);
        
        
        if(vlength || flength || plength) {
            printf("%s\n", vertexShader);
            printf("%s\n", fragShader);
            printf("%s\n", vlog);
            printf("%s\n", flog);
            printf("%s\n", plog);
            
        }
    }

    renderCheckError();
    
    assert(result.valid);

    return result;
}

static inline void addInstanceAttribForMatrix(int index, GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    glEnableVertexAttribArray(attribLoc + index);  
    renderCheckError();
    
    glVertexAttribPointer(attribLoc + index, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct + (4*sizeof(float)*index));
    renderCheckError();
    glVertexAttribDivisor(attribLoc + index, 1);
    renderCheckError();
}

static inline void addInstancingAttrib (GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    assert(offsetForStruct > 0);
    if(numOfFloats == 16) {
        addInstanceAttribForMatrix(0, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(1, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(2, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(3, attribLoc, 4, offsetForStruct, offsetInStruct);
    } else {
        glEnableVertexAttribArray(attribLoc);  
        renderCheckError();
        
        assert(numOfFloats <= 4);
        glVertexAttribPointer(attribLoc, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct);
        renderCheckError();
        
        glVertexAttribDivisor(attribLoc, 1);
        renderCheckError();
    }
}

void addInstancingAttrib_int32(GLuint attribLoc, int numOfInt32s, size_t offsetForStruct, size_t offsetInStruct) {
    glEnableVertexAttribArray(attribLoc);  
    renderCheckError();
    
    glVertexAttribIPointer(attribLoc, numOfInt32s, GL_UNSIGNED_INT, offsetForStruct, ((char *)0) + offsetInStruct);
    renderCheckError();

    glVertexAttribDivisor(attribLoc, 1);
    renderCheckError();
}

enum AttribInstancingType {
    ATTRIB_INSTANCE_TYPE_NONE,
    ATTRIB_INSTANCE_TYPE_DEFAULT,
    ATTRIB_INSTANCE_TYPE_MODEL_MATRIX,
    ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL,
};

void addInstancingAttribsForShader(AttribInstancingType type) {
    if(type == ATTRIB_INSTANCE_TYPE_DEFAULT) {
        size_t offsetForStruct = sizeof(InstanceData); 

        addInstancingAttrib (POS_ATTRIB_LOCATION, 3, offsetForStruct, 0);
        unsigned int uvOffset = (intptr_t)(&(((InstanceData *)0)->uv));
        addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 2, offsetForStruct, uvOffset);
        unsigned int colorOffset = (intptr_t)(&(((InstanceData *)0)->color));
        addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
        renderCheckError();
        unsigned int scaleOffset = (intptr_t)(&(((InstanceData *)0)->scale));
        addInstancingAttrib (SCALE_ATTRIB_LOCATION, 3, offsetForStruct, scaleOffset);
        renderCheckError();
        unsigned int maskOffset = (intptr_t)(&(((InstanceData *)0)->AOMask));
        addInstancingAttrib_int32(AO_MASK_ATTRIB_LOCATION, 2, offsetForStruct, maskOffset);
        renderCheckError();
        unsigned int samplerIndexOffset = (intptr_t)(&(((InstanceData *)0)->samplerIndex));
        addInstancingAttrib_int32(SAMPLER_INDEX_ATTRIB_LOCATION, 1, offsetForStruct, samplerIndexOffset);
        renderCheckError();
    } else if(type == ATTRIB_INSTANCE_TYPE_MODEL_MATRIX || type == ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL) {
        size_t offsetForStruct = sizeof(InstanceDataWithRotation); 

        unsigned int uvOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->uv));
        addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 4, offsetForStruct, uvOffset);
        unsigned int colorOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->color));
        addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
        renderCheckError();
        unsigned int modelOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->M));
        addInstancingAttrib (MODEL_TRANSFORM_ATTRIB_LOCATION, 16, offsetForStruct, modelOffset);
        renderCheckError();
    } else {
        assert(false);
    }
    
}

void deleteVao(GLuint handle) {
    glDeleteVertexArrays(1, &handle);
}

ChunkModelBuffer generateChunkVertexBuffer(void *triangleData, int vertexCount, unsigned int *indicesData, int indexCount) {
    ChunkModelBuffer result = {};
    glGenVertexArrays(1, &result.handle);
    renderCheckError();
    glBindVertexArray(result.handle);
    renderCheckError();
    
    GLuint vertices;
    GLuint indices;
    
    glGenBuffers(1, &vertices);
    renderCheckError();
    
    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    renderCheckError();

    size_t sizeOfVertex = sizeof(VertexForChunk);
    
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeOfVertex, triangleData, GL_DYNAMIC_DRAW);
    renderCheckError();
    
    glGenBuffers(1, &indices);
    renderCheckError();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    renderCheckError();
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_DYNAMIC_DRAW);
    renderCheckError();
    
    result.indexCount = indexCount;

    //NOTE: Assign the attribute locations with the data offsets & types
    GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
    renderCheckError();
    glEnableVertexAttribArray(vertexAttrib);  
    renderCheckError();
    glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(VertexForChunk), 0);
    renderCheckError();
    
    GLint texUVAttrib = UV_ATTRIB_LOCATION;
    glEnableVertexAttribArray(texUVAttrib);  
    renderCheckError();
    unsigned int uvByteOffset = (intptr_t)(&(((VertexForChunk *)0)->texUV));
    glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(VertexForChunk), ((char *)0) + uvByteOffset);
    renderCheckError();

    GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
    glEnableVertexAttribArray(normalsAttrib);  
    renderCheckError();
    unsigned int normalOffset = (intptr_t)(&(((VertexForChunk *)0)->normal));
    glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(VertexForChunk), ((char *)0) + normalOffset);
    renderCheckError();

    GLint aoAttrib = AO_MASK_ATTRIB_LOCATION;
    glEnableVertexAttribArray(aoAttrib);  
    renderCheckError();
    unsigned int aoOffset = (intptr_t)(&(((VertexForChunk *)0)->aoMask));
    glVertexAttribPointer(aoAttrib, 1, GL_INT, GL_FALSE, sizeof(VertexForChunk), ((char *)0) + aoOffset);
    renderCheckError();
    
    glBindVertexArray(0);
        
    //we can delete these buffers since they are still referenced by the VAO 
    glDeleteBuffers(1, &vertices);
    glDeleteBuffers(1, &indices);

    return result;
}

ModelBuffer generateVertexBuffer(void *triangleData, int vertexCount, unsigned int *indicesData, int indexCount, AttribInstancingType attribInstancingType = ATTRIB_INSTANCE_TYPE_DEFAULT) {
    ModelBuffer result = {};
    glGenVertexArrays(1, &result.handle);
    renderCheckError();
    glBindVertexArray(result.handle);
    renderCheckError();
    
    GLuint vertices;
    GLuint indices;
    
    glGenBuffers(1, &vertices);
    renderCheckError();
    
    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    renderCheckError();

    size_t sizeOfVertex = (attribInstancingType != ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL) ? sizeof(Vertex) : sizeof(VertexWithJoints);
    
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeOfVertex, triangleData, GL_STATIC_DRAW);
    renderCheckError();
    
    glGenBuffers(1, &indices);
    renderCheckError();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    renderCheckError();
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
    renderCheckError();
    
    result.indexCount = indexCount;

    if(attribInstancingType != ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL) {
    
        //NOTE: Assign the attribute locations with the data offsets & types
        GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
        renderCheckError();
        glEnableVertexAttribArray(vertexAttrib);  
        renderCheckError();
        glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        renderCheckError();
        
        GLint texUVAttrib = UV_ATTRIB_LOCATION;
        glEnableVertexAttribArray(texUVAttrib);  
        renderCheckError();
        unsigned int uvByteOffset = (intptr_t)(&(((Vertex *)0)->texUV));
        glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + uvByteOffset);
        renderCheckError();

        GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
        glEnableVertexAttribArray(normalsAttrib);  
        renderCheckError();
        unsigned int normalOffset = (intptr_t)(&(((Vertex *)0)->normal));
        glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + normalOffset);
        renderCheckError();

    } else {   
         //NOTE: Assign the attribute locations with the data offsets & types
        GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
        renderCheckError();
        glEnableVertexAttribArray(vertexAttrib);  
        renderCheckError();
        glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(VertexWithJoints), 0);
        renderCheckError();
        
        GLint texUVAttrib = UV_ATTRIB_LOCATION;
        glEnableVertexAttribArray(texUVAttrib);  
        renderCheckError();
        unsigned int uvByteOffset = (intptr_t)(&(((VertexWithJoints *)0)->texUV));
        glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(VertexWithJoints), ((char *)0) + uvByteOffset);
        renderCheckError();

        GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
        glEnableVertexAttribArray(normalsAttrib);  
        renderCheckError();
        unsigned int normalOffset = (intptr_t)(&(((VertexWithJoints *)0)->normal));
        glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(VertexWithJoints), ((char *)0) + normalOffset);
        renderCheckError();

        GLint weightAttrib = JOINT_WEIGHTS;
        glEnableVertexAttribArray(weightAttrib);  
        renderCheckError();
        unsigned int weigthOffset = (intptr_t)(&(((VertexWithJoints *)0)->jointWeights));
        glVertexAttribPointer(weightAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(VertexWithJoints), ((char *)0) + weigthOffset);
        renderCheckError();

        GLint jointAttrib = JOINT_INDEXES;
        glEnableVertexAttribArray(jointAttrib);  
        renderCheckError();
        unsigned int jointIndexOffset = (intptr_t)(&(((VertexWithJoints *)0)->jointIndexes));
        glVertexAttribPointer(jointAttrib, 4, GL_INT, GL_FALSE, sizeof(VertexWithJoints), ((char *)0) + jointIndexOffset);
        renderCheckError();
        
    }

    // vbo instance buffer
    {
        glGenBuffers(1, &result.instanceBufferhandle);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, result.instanceBufferhandle);
        renderCheckError();

        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_DYNAMIC_DRAW);
        
        addInstancingAttribsForShader(attribInstancingType);
    }

    if(attribInstancingType == ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL) {
        //NOTE: Generate a UBO to store Joint Transforms i.e. skinning matrix
        GLuint tbo;
        glGenBuffers(1, &tbo);
        renderCheckError();
        glBindBuffer(GL_TEXTURE_BUFFER, tbo);
        renderCheckError();
        //NOTE: No data yet
        glBufferData(GL_TEXTURE_BUFFER, 0, 0, GL_DYNAMIC_DRAW);
        renderCheckError();

        GLuint texture;
        glGenTextures(1, &texture);
        renderCheckError();
        glBindTexture(GL_TEXTURE_BUFFER, texture);
        renderCheckError();
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tbo);  // Create a texture buffer using the TBO
        renderCheckError();

        result.tboHandle = tbo;
        result.textureHandle = texture;

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        renderCheckError();
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        renderCheckError();
    }
    
    glBindVertexArray(0);
        
    //we can delete these buffers since they are still referenced by the VAO 
    glDeleteBuffers(1, &vertices);
    glDeleteBuffers(1, &indices);

    return result;
}

void updateSkinningTexture(ModelBuffer *modelBuffer, float16 *skinningMatrix, int jointCount) {
    glBindBuffer(GL_TEXTURE_BUFFER, modelBuffer->tboHandle);
    renderCheckError();

    size_t sizeInBytes = sizeof(float16)*jointCount;

    glBufferData(GL_TEXTURE_BUFFER, sizeInBytes, skinningMatrix, GL_STREAM_DRAW); 
    renderCheckError();
    
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    renderCheckError();
}

void initBackendRenderer() {
    //TODO: Enable the back face culling
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    // glFrontFace(GL_CCW);  

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
}

struct Texture {
    int w;
    int h;

    uint32_t handle;
};


void deleteTexture(Texture *t) {
    glDeleteTextures(1, &t->handle);
}

void deleteTextureHandle(u32 handle) {
    glDeleteTextures(1, &handle);
}

Texture createGPUTextureRed(int width, int height, void *data = 0) {
    Texture result;
    result.w = width;
    result.h = height;

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0); 

    result.handle = texture;   

    return result;
}

Texture createGPUTexture(int width, int height, void *data = 0) {
    Texture result;
    result.w = width;
    result.h = height;

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0); 

    result.handle = texture;   

    return result;
}

Texture loadCubeMapTextureToGPU(char *folderName) {
    Texture result = {};

    glGenTextures(1, &result.handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, result.handle);

    char *faces[] = {
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "front.jpg",
    "back.jpg"};
    
    int width, height, nrChannels;
    for (unsigned int i = 0; i < arrayCount(faces); i++)
    {
        size_t bytesToAlloc = snprintf(NULL, 0, "%s%s", folderName, faces[i]);
        char *concatFileName = (char *)malloc(bytesToAlloc + 1);
        snprintf(concatFileName, bytesToAlloc + 1, "%s%s", folderName, faces[i]);
        unsigned char *data = stbi_load(concatFileName, &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        } else {
            printf("Cubemap tex failed to load\n");
            stbi_image_free(data);
        }
        free(concatFileName);
        concatFileName = 0;
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return result;
}

Texture loadTextureArrayToGPU(char *fileName, int fileNameCount) {
    Texture t = {};

    unsigned char **imageDatas = (unsigned char **)malloc(sizeof(unsigned char *)*fileNameCount);

    for(int i = 0; i < fileNameCount; ++i) {
        imageDatas[i] = (unsigned char *)stbi_load(fileName, &t.w, &t.h, 0, STBI_rgb_alpha);

        if(imageDatas[i]) {
        // assert(result.comp == 4);
        } else {
            printf("%s\n", fileName);
            assert(!"no image found");
        }
    }

    GLuint resultId;
    glGenTextures(1, &resultId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, resultId);
    renderCheckError();
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    renderCheckError();

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, t.w, t.h, fileNameCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    renderCheckError();

    for(int i = 0; i < fileNameCount; ++i) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, t.w, t.h, 1, GL_RGB, GL_UNSIGNED_BYTE, imageDatas[i]);
        stbi_image_free(imageDatas[i]);
        imageDatas[i] = 0;
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();

    t.handle = resultId;

    return t;
}

Texture loadTextureToGPU(char *fileName) {
    Texture t = {};
    // stbi_set_flip_vertically_on_load(true);
    unsigned char *imageData = (unsigned char *)stbi_load(fileName, &t.w, &t.h, 0, STBI_rgb_alpha);
    
    if(imageData) {
        // assert(result.comp == 4);
    } else {
        printf("%s\n", fileName);
        assert(!"no image found");
    }

    GLuint resultId;
    glGenTextures(1, &resultId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, resultId);
    renderCheckError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    renderCheckError();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    renderCheckError();

    glGenerateMipmap(GL_TEXTURE_2D);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();

    t.handle = resultId;

    stbi_image_free(imageData);
    imageData = 0;

    return t;
    
}

void updateInstanceData(uint32_t bufferHandle, void *data, size_t sizeInBytes) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandle);
    renderCheckError();
    
    //send the data to GPU. glBufferData deletes the old one
    //NOTE(ollie): We were using glBufferData which deletes the old buffer and resends the create a new buffer, but 
    //NOTE(ollie): I saw on Dungeoneer code using glsubbufferdata is faster because it doesn't have to delete it.
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeInBytes, data);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_STREAM_DRAW); 
    renderCheckError();

    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    renderCheckError();
}

enum ShaderFlags {
    SHADER_CUBE_MAP = 1 << 0,
    SHADER_TEXTURE_BUFFER = 1 << 1,
};

void bindTexture(char *uniformName, int slotId, GLint textureId, Shader *shader, uint32_t flags) {
    GLint texUniform = glGetUniformLocation(shader->handle, uniformName); 
    renderCheckError();
    
    glUniform1i(texUniform, slotId);
    renderCheckError();
    
    glActiveTexture(GL_TEXTURE0 + slotId);
    renderCheckError();
    
    if(flags & SHADER_CUBE_MAP) {
        // glBindTexture(GL_TEXTURE_CUBE_MAP, textureId); 
        // renderCheckError();
    } else if(flags & SHADER_TEXTURE_BUFFER) { 
        glBindTexture(GL_TEXTURE_BUFFER, textureId); 
        renderCheckError();
    } else {
        glBindTexture(GL_TEXTURE_2D, textureId); 
        renderCheckError();
    }
}


void bindTextureArray(char *uniformName, GLint textureId, Shader *shader, uint32_t flags) {

}

void prepareChunkRender(ChunkModelBuffer *model, Shader *shader, uint32_t textureId, float16 projectionTransform, float16 modelViewTransform, float3 lookingAxis, bool underWater) {
     glUseProgram(shader->handle);
    renderCheckError();
    
    glBindVertexArray(model->handle);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "V"), 1, GL_FALSE, modelViewTransform.E);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "projection"), 1, GL_FALSE, projectionTransform.E);
    renderCheckError();

    glUniform3f(glGetUniformLocation(shader->handle, "lookingAxis"), lookingAxis.x, lookingAxis.y, lookingAxis.z);
    renderCheckError();

    float4 fogColor = make_float4(0.9, 0.9, 0.9, 1);

    if(underWater) {
        fogColor = make_float4(0.163, 0.551, 0.776, 1);
    }

    glUniform4f(glGetUniformLocation(shader->handle, "fogColor"), fogColor.x, fogColor.y, fogColor.z, fogColor.w);
    renderCheckError();

    glUniform1f(glGetUniformLocation(shader->handle, "fogSeeDistance"), (underWater ? 0 : 100)); //global_fogSeeDistance*1000
    renderCheckError();

    glUniform1f(glGetUniformLocation(shader->handle, "fogFadeDistance"), (underWater ? 60 : 250)); //global_fogFarDistance*1000
    renderCheckError();

    bindTexture("diffuse", 1, textureId, shader, 0);
    renderCheckError();
   
}

void drawModels(ModelBuffer *model, Shader *shader, uint32_t textureId, int instanceCount, float16 projectionTransform, float16 modelViewTransform, float3 lookingAxis, bool underWater, TimeOfDayValues timeOfDayValues, uint32_t flags = 0, int skinningTextureId = -1, GLenum primitive = GL_TRIANGLES, float time = 0) {
    // printf("%d\n", instanceCount);
    glUseProgram(shader->handle);
    renderCheckError();
    
    glBindVertexArray(model->handle);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "V"), 1, GL_FALSE, modelViewTransform.E);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "projection"), 1, GL_FALSE, projectionTransform.E);
    renderCheckError();

    glUniform4f(glGetUniformLocation(shader->handle, "skyColorA"), timeOfDayValues.skyColorA.x, timeOfDayValues.skyColorA.y, timeOfDayValues.skyColorA.z, 1);
    renderCheckError();

    glUniform4f(glGetUniformLocation(shader->handle, "skyColorB"), timeOfDayValues.skyColorB.x, timeOfDayValues.skyColorB.y, timeOfDayValues.skyColorB.z, 1);
    renderCheckError();

    glUniform3f(glGetUniformLocation(shader->handle, "lookingAxis"), lookingAxis.x, lookingAxis.y, lookingAxis.z);
    renderCheckError();

    glUniform1f(glGetUniformLocation(shader->handle, "u_time"), time); //global_fogSeeDistance*1000
    renderCheckError();

    float4 fogColor = make_float4(0.9, 0.9, 0.9, 1);

    if(underWater) {
        fogColor = make_float4(0.163, 0.551, 0.776, 1);
    }

    glUniform4f(glGetUniformLocation(shader->handle, "fogColor"), fogColor.x, fogColor.y, fogColor.z, fogColor.w);
    renderCheckError();

    glUniform1f(glGetUniformLocation(shader->handle, "fogSeeDistance"), (underWater ? 0 : 100)); //global_fogSeeDistance*1000
    renderCheckError();

    glUniform1f(glGetUniformLocation(shader->handle, "fogFadeDistance"), (underWater ? 60 : 250)); //global_fogFarDistance*1000
    renderCheckError();

    bindTexture("diffuse", 1, textureId, shader, flags);
    renderCheckError();

    if(skinningTextureId >= 0) {
        bindTexture("boneMatrixBuffer", 2, skinningTextureId, shader, SHADER_TEXTURE_BUFFER);
        renderCheckError(); 
    }

    glDrawElementsInstanced(primitive, model->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();    

    glUseProgram(0);
    renderCheckError();
    
}

void endChunkRender() {
    glBindVertexArray(0);
    renderCheckError();    

    glUseProgram(0);
    renderCheckError();
}

// void draw() {
//     updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->atlasQuads, renderer->atlasQuadCount*sizeof(InstanceDataWithRotation));
//     drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->atlasTexture, renderer->atlasQuadCount, projectionTransform, modelViewTransform, lookingAxis, false, getTimeOfDayValues());
// }

void rendererFinish(Renderer *renderer, float16 projectionTransform, float16 modelViewTransform, float16 projectionScreenTransform, float16 textScreenTransform, float3 lookingAxis, float16 cameraTransformWithoutTranslation, TimeOfDayValues timeOfDay, uint32_t perlinNoiseHandle) {
    if(renderer->cubeCount > 0) {
        //NOTE: Draw Cubes
        // printf("cube size: %lu\n", renderer->cubeCount*sizeof(InstanceData));
        updateInstanceData(renderer->blockModel.instanceBufferhandle, renderer->cubeData, renderer->cubeCount*sizeof(InstanceData));
        drawModels(&renderer->blockModel, &renderer->blockShader, renderer->terrainTextureHandle, renderer->cubeCount, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay);

        renderer->cubeCount = 0;
    }


   
    if(renderer->blockItemsCount > 0) {
        updateInstanceData(renderer->blockModelWithInstancedT.instanceBufferhandle, renderer->blockItemsData, renderer->blockItemsCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->blockModelWithInstancedT, &renderer->blockPickupShader, renderer->terrainTextureHandle, renderer->blockItemsCount, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay);

        renderer->blockItemsCount = 0;
    }
    
    // //NOTE: Draw the skybox here
    // glDepthMask(GL_FALSE); //NOTE: Disable WRITING to the depth buffer
    // drawModels(&renderer->blockModel, &renderer->skyboxShader, renderer->terrainTextureHandle, 1, projectionTransform, cameraTransformWithoutTranslation, lookingAxis, renderer->underWater, timeOfDay, SHADER_CUBE_MAP);
    // glDepthMask(GL_TRUE);

    if(renderer->alphaBlockCount > 0) {
        //NOTE: Draw Cubes
        updateInstanceData(renderer->blockModel.instanceBufferhandle, renderer->alphaBlockData, renderer->alphaBlockCount*sizeof(InstanceData));
        drawModels(&renderer->blockModel, &renderer->blockColorShader, renderer->terrainTextureHandle, renderer->alphaBlockCount, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay);

        renderer->alphaBlockCount = 0;
    }

    if(renderer->alphaItemCount > 0) {
        //NOTE: Draw Cubes
        updateInstanceData(renderer->blockModelSameTexture.instanceBufferhandle, renderer->alphaItemData, renderer->alphaItemCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->blockModelSameTexture, &renderer->blockSameTextureShader, renderer->breakBlockTexture, renderer->alphaItemCount, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay);

        renderer->alphaItemCount = 0;
    }

    if(renderer->atlasQuadCount > 0) {
        //NOTE: Draw circle oultines
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->atlasQuads, renderer->atlasQuadCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->atlasTexture, renderer->atlasQuadCount, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay);

        renderer->atlasQuadCount = 0;
    }

    if(renderer->atlasQuadHUDCount > 0) {
        //NOTE: Draw circle oultines
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->atlasHUDQuads, renderer->atlasQuadHUDCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->atlasTexture, renderer->atlasQuadHUDCount, projectionScreenTransform, float16_identity(), lookingAxis, renderer->underWater, timeOfDay);

        renderer->atlasQuadHUDCount = 0;
    }

    for(int i = 0; i < renderer->canvasCount; ++i) {
        u32 handle = renderer->canvasHandles[i];
        updateInstanceData(renderer->quadModel.instanceBufferhandle, &renderer->canvasQuads[i], sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, handle, 1, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay, 0, -1, GL_TRIANGLES, renderer->timeAccum);
    }

    renderer->canvasCount = 0;


    if(renderer->glyphCount > 0) {
        //NOTE: Draw circle oultines
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->glyphData, renderer->glyphCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->fontTextureShader, renderer->fontAtlasTexture, renderer->glyphCount, textScreenTransform, float16_identity(), lookingAxis, renderer->underWater, timeOfDay);

        renderer->glyphCount = 0;
    }

    if(renderer->lineCount > 0) {
        //NOTE: Draw circle oultines
        updateInstanceData(renderer->lineModel.instanceBufferhandle, renderer->lineData, renderer->lineCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->lineModel, &renderer->lineShader, renderer->fontAtlasTexture, renderer->lineCount, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay, 0, -1, GL_LINES);

        renderer->lineCount = 0;
    }

    

    if(renderer->selectionCount > 0) {
        //NOTE: Draw circle oultines
        updateInstanceData(renderer->quadModel.instanceBufferhandle, &renderer->selectionQuad, sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->pixelSelectionShader, renderer->selectionTextureHandle, 1, projectionTransform, modelViewTransform, lookingAxis, renderer->underWater, timeOfDay, 0, -1, GL_TRIANGLES, renderer->timeAccum);

        renderer->selectionCount = 0;
    }


    

    
}