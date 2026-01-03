#include "../libs/GLAD/include/glad/glad.h"
#include "../libs/GLAD/src/glad.c"
#ifndef __APPLE__
#include <GL/gl.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"
#include "../shaders/shaders_opengl.cpp"

// NOTE: Each location index in a vertex attribute index - i.e. 4 floats. that's why for matrix we skip 4 values
#define VERTEX_ATTRIB_LOCATION 0
#define NORMAL_ATTRIB_LOCATION 1
#define UV_ATTRIB_LOCATION 2
#define POS_ATTRIB_LOCATION 3
#define UVATLAS_ATTRIB_LOCATION 4
#define COLOR_ATTRIB_LOCATION 5
#define SCALE_ATTRIB_LOCATION 6
#define MODEL_TRANSFORM_ATTRIB_LOCATION 7

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

void backendRenderer_bindTexture2D(uint32_t handle) {
    glBindTexture(GL_TEXTURE_2D, handle);
}

struct FrameBuffer {
    uint32_t handle;
    uint32_t textureHandle;
};

void backendRenderer_clearDefaultFrameBuffer(float4 clearColor) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
}

void backendRenderer_BindFrameBuffer(uint32_t handle) {
    glBindFramebuffer(GL_FRAMEBUFFER, handle);  
}

FrameBuffer createFrameBuffer(int width, int height, void *data = 0) {
    DEBUG_TIME_BLOCK()
    FrameBuffer result;
    glGenFramebuffers(1, &result.handle);
    glBindFramebuffer(GL_FRAMEBUFFER, result.handle);  

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    result.textureHandle = texture;
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderCheckError();

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
    glBindAttribLocation(result.handle, MODEL_TRANSFORM_ATTRIB_LOCATION, "M");
    renderCheckError();

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

void addInstancingAttribsForShader() {
    size_t offsetForStruct = sizeof(InstanceDataWithRotation); 

    unsigned int uvOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->uv));
    addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 4, offsetForStruct, uvOffset);
    unsigned int colorOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->color));
    addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
    renderCheckError();
    unsigned int modelOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->M));
    addInstancingAttrib (MODEL_TRANSFORM_ATTRIB_LOCATION, 16, offsetForStruct, modelOffset);
    renderCheckError();
}

void deleteVao(GLuint handle) {
    glDeleteVertexArrays(1, &handle);
}

ModelBuffer generateVertexBuffer(void *triangleData, int vertexCount, unsigned int *indicesData, int indexCount) {
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

    size_t sizeOfVertex = sizeof(Vertex);
    
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeOfVertex, triangleData, GL_STATIC_DRAW);
    renderCheckError();
    
    glGenBuffers(1, &indices);
    renderCheckError();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    renderCheckError();
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
    renderCheckError();
    
    result.indexCount = indexCount;

    
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

    // vbo instance buffer
    {
        glGenBuffers(1, &result.instanceBufferhandle);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, result.instanceBufferhandle);
        renderCheckError();

        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_DYNAMIC_DRAW);
        
        addInstancingAttribsForShader();
    }

    glBindVertexArray(0);
        
    //we can delete these buffers since they are still referenced by the VAO 
    glDeleteBuffers(1, &vertices);
    glDeleteBuffers(1, &indices);

    return result;
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
    renderCheckError();
}

void deleteTextureHandle(u32 handle) {
    glDeleteTextures(1, &handle);
    renderCheckError();
}

Texture createGPUTextureRed(int width, int height, void *data = 0) {
    Texture result;
    result.w = width;
    result.h = height;

    unsigned int texture;
    glGenTextures(1, &texture);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, texture);
    renderCheckError();

    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderCheckError();

    glBindTexture(GL_TEXTURE_2D, 0); 
    renderCheckError();

    result.handle = texture;   

    return result;
}

Texture createGPUTexture(int width, int height, void *data = 0) {
    Texture result;
    result.w = width;
    result.h = height;

    unsigned int texture;
    glGenTextures(1, &texture);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, texture);
    renderCheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderCheckError();

    glBindTexture(GL_TEXTURE_2D, 0); 
    renderCheckError();

    result.handle = texture;   

    return result;
}

u32 *backendRenderer_getFrameBufferPixels_shortTerm(int w, int h) {
    //NOTE: Flush the openGL command calls
    glFlush();

    u32 *pixelBuffer = pushArray(&globalPerFrameArena, w*h, u32);
    
    glReadPixels(0, 0,
                        w,
                        h,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        pixelBuffer);

    return pixelBuffer;
}

Texture loadTextureToGPU(char *fileName, unsigned char* rawData = 0, int bufferLen = 0) {
    Texture t = {};
    // stbi_set_flip_vertically_on_load(true);
    unsigned char *imageData = 0;
    if(rawData) {
        imageData = (unsigned char *)stbi_load_from_memory(rawData, bufferLen, &t.w, &t.h, 0, STBI_rgb_alpha);
    } else {
        imageData = (unsigned char *)stbi_load(fileName, &t.w, &t.h, 0, STBI_rgb_alpha);
    }
    
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
    
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_STREAM_DRAW); 
    renderCheckError();
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    renderCheckError();
}

void bindTexture(char *uniformName, int slotId, GLint textureId, Shader *shader) {
    GLint texUniform = glGetUniformLocation(shader->handle, uniformName); 
    renderCheckError();
    
    glUniform1i(texUniform, slotId);
    renderCheckError();
    
    glActiveTexture(GL_TEXTURE0 + slotId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, textureId); 
    renderCheckError();
}

void drawModels(ModelBuffer *model, Shader *shader, uint32_t textureId, int instanceCount, float16 projectionTransform, float16 modelViewTransform, float3 lookingAxis, GLenum primitive = GL_TRIANGLES, float time = 0) {
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

    glUniform1f(glGetUniformLocation(shader->handle, "u_time"), time);
    renderCheckError();

    bindTexture("diffuse", 1, textureId, shader);
    renderCheckError();

    glDrawElementsInstanced(primitive, model->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();    

    glUseProgram(0);
    renderCheckError();
    
}

void rendererFinish(Renderer *renderer, float16 projectionTransform, float16 modelViewTransform, float16 projectionScreenTransform, float16 textScreenTransform, float3 lookingAxis, float16 cameraTransformWithoutTranslation) {
   
    if(renderer->checkerQuadCount > 0) {
        u32 canvasTextureHandle = renderer->canvasHandles[0];
        if(canvasTextureHandle > 0) {
            updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->checkerQuads, renderer->checkerQuadCount*sizeof(InstanceDataWithRotation));
            drawModels(&renderer->quadModel, &renderer->checkQuadShader, canvasTextureHandle, renderer->checkerQuadCount, projectionTransform, modelViewTransform, lookingAxis);
        }

        renderer->checkerQuadCount = 0;
    }
    renderCheckError();
    for(int i = 0; i < renderer->canvasCount; ++i) {
        u32 handle = renderer->canvasHandles[i];
        renderCheckError();
        updateInstanceData(renderer->quadModel.instanceBufferhandle, &renderer->canvasQuads[i], sizeof(InstanceDataWithRotation));
        renderCheckError();
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, handle, 1, projectionTransform, modelViewTransform, lookingAxis, GL_TRIANGLES, renderer->timeAccum);
        renderCheckError();
    }

    renderer->canvasCount = 0;

    if(renderer->atlasQuadCount > 0) {
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->atlasQuads, renderer->atlasQuadCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->atlasTexture, renderer->atlasQuadCount, projectionTransform, modelViewTransform, lookingAxis);

        renderer->atlasQuadCount = 0;
    }

    if(renderer->atlasQuadHUDCount > 0) {
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->atlasHUDQuads, renderer->atlasQuadHUDCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->quadTextureShader, renderer->atlasTexture, renderer->atlasQuadHUDCount, projectionScreenTransform, float16_identity(), lookingAxis);

        renderer->atlasQuadHUDCount = 0;
    }


    if(renderer->glyphCount > 0) {
        updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->glyphData, renderer->glyphCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->fontTextureShader, renderer->fontAtlasTexture, renderer->glyphCount, textScreenTransform, float16_identity(), lookingAxis);

        renderer->glyphCount = 0;
    }

    

    if(renderer->selectionCount > 0) {
        updateInstanceData(renderer->quadModel.instanceBufferhandle, &renderer->selectionQuad, sizeof(InstanceDataWithRotation));
        drawModels(&renderer->quadModel, &renderer->pixelSelectionShader, renderer->selectionTextureHandle, 1, projectionTransform, modelViewTransform, lookingAxis, GL_TRIANGLES, renderer->timeAccum);

        renderer->selectionCount = 0;
    }

    if(renderer->lineCount > 0) {
        updateInstanceData(renderer->lineModel.instanceBufferhandle, renderer->lineData, renderer->lineCount*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->lineModel, &renderer->lineShader, renderer->fontAtlasTexture, renderer->lineCount, projectionTransform, modelViewTransform, lookingAxis, GL_LINES);

        renderer->lineCount = 0;
    }

    if(renderer->lineCountScreenSpace > 0) {
        updateInstanceData(renderer->lineModel.instanceBufferhandle, renderer->lineDataScreenSpace, renderer->lineCountScreenSpace*sizeof(InstanceDataWithRotation));
        drawModels(&renderer->lineModel, &renderer->lineShader, renderer->fontAtlasTexture, renderer->lineCountScreenSpace, textScreenTransform, modelViewTransform, lookingAxis, GL_LINES);

        renderer->lineCountScreenSpace = 0;
    }

    renderCheckError();
}