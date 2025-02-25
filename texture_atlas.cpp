#include <dirent.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./libs/stb_image_write.h"

    struct TextureAtlas {
        Texture texture;
        
        //NOTE: Hash table of assets
        AtlasAsset *items[4096];
    };


    AtlasAsset *textureAtlas_addItem(TextureAtlas *atlas, char *name, float4 uv) {
        uint32_t hash = get_crc32_for_string(name);

        hash %= arrayCount(atlas->items);
        assert(hash < arrayCount(atlas->items));

        AtlasAsset **aPtr = &atlas->items[hash];

        while(*aPtr) {
            aPtr = &((*aPtr)->next);
        }

        assert((*aPtr) == 0);

        AtlasAsset *a = pushStruct(&globalLongTermArena, AtlasAsset);

        a->name = name;
        a->uv = uv;
        a->next = 0;

        *aPtr = a;

        return a;
    }

    AtlasAsset *textureAtlas_getItem(TextureAtlas *atlas, char *name) {
        AtlasAsset *result = 0;

        uint32_t hash = get_crc32_for_string(name);

        uint32_t hashIndex = hash % arrayCount(atlas->items);
        assert(hashIndex < arrayCount(atlas->items));

        AtlasAsset *a = atlas->items[hashIndex];

        while(a && !result) {
            uint32_t hashText = get_crc32_for_string(a->name);
            if(hashText == hash && easyString_stringsMatch_nullTerminated(a->name, name)) {
                result = a;
            }

            a = a->next;
        }

        return result;
    }

    // void DEBUG_runUnitTests() {
    //     TextureAtlas atlas = {};

    //     AtlasAsset *a = addItem(&atlas, "name", make_float4(1, 1, 1, 1));
    //     AtlasAsset *b = getItem(&atlas, "name", 0);

    //     AtlasAsset *c = addItem(&atlas, "name", make_float4(1, 0, 0, 1));
    //     AtlasAsset *d = getItem(&atlas, "name", 1);

    //     AtlasAsset *e = getItem(&atlas, "name1", 1);
        

    //     assert(a);
    //     assert(b);
    //     assert(c);

    //     assert(a == b);
    //     assert(c == d);
    //     assert(a != d);

    //     assert(!e);
        
    // }

    void createTextureAtlas(Renderer *renderer, char *folder) {
        char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp", "PNG"};
        FileNameOfType files = getDirectoryFilesOfType(folder, imgFileTypes, arrayCount(imgFileTypes));

        float outputW = 4096*0.5f;
        float outputH = 4096*0.5f;

        float xAt = 0;
        float yAt = 0;

        float16 screenGuiT = make_ortho_matrix_top_left_corner(outputW, outputH, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
        FrameBuffer frameBuffer = createFrameBuffer(outputW, outputH);
        rendererBindFrameBuffer(&frameBuffer);

        glViewport(0, 0, outputW, outputH);

        game_file_handle atlasJsonFile = platformBeginFileWrite("./texture_atlas.json");
        assert(!atlasJsonFile.HasErrors);
        
        size_t offset = 0;
        
        float largestY = 0;
        for(int i = 0; i < files.count; ++i) {
            char *name = files.names[i];
            
            Texture t = loadTextureToGPU(name);

            pushAtlasQuad_(renderer, make_float3(xAt + 0.5f*t.w, yAt - 0.5f*t.h, 1), make_float3(t.w, t.h, 1), make_float3(0, 0, 0), make_float4(0, 1, 0, 1), make_float4(1, 1, 1, 1), true);

            //NOTE: Draw the texture at the coord
            if(renderer->atlasQuadHUDCount > 0) {
                TimeOfDayValues timeOfDayValues;
                //NOTE: Draw circle oultines
                updateInstanceData(renderer->quadModel.instanceBufferhandle, renderer->atlasHUDQuads, renderer->atlasQuadHUDCount*sizeof(InstanceDataWithRotation));
                drawModels(&renderer->quadModel, &renderer->quadTextureShader, t.handle, renderer->atlasQuadHUDCount, screenGuiT, float16_identity(), make_float3(0, 0, 0), false, timeOfDayValues);

                renderer->atlasQuadHUDCount = 0;
            }

            char *strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"name\": \"%s\", \"uv\": %f %f %f %f}\n", getFileLastPortionWithArena(name, &globalPerFrameArena), xAt / outputW, (xAt + t.w) / outputW, -1*yAt / outputW, (-1*yAt + t.h) / outputW);

            offset = platformWriteFile(&atlasJsonFile, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

            renderer->alphaItemCount = 0;

            if(largestY < t.h) {
                largestY = t.h;
            }

            xAt += t.w;

            if(xAt >= outputW) {
                xAt = 0;
                yAt -= largestY;
                largestY = 0;
            }

            if(yAt >= outputH) {
                assert(false);
                //NOTE: Out of room
            }
        }

        platformEndFile(atlasJsonFile);

        //NOTE: Flush the openGL command calls
        glFlush();

        //NOTE: Save the buffer to a file
        size_t bytesPerPixel = sizeof(uint8_t)*4;
        size_t sizeToAlloc = outputW*outputH*bytesPerPixel;
        int stride_in_bytes = bytesPerPixel*outputW;
        
        uint8_t *pixelBuffer = (uint8_t *)calloc(sizeToAlloc, 1);
        
        glReadPixels(0, 0,
                            outputW,
                            outputH,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            pixelBuffer);
        
        
        stbi_flip_vertically_on_write(1);
        int writeResult = stbi_write_png("./texture_atlas.png", outputW, outputH, 4, pixelBuffer, stride_in_bytes);

        free(pixelBuffer);

        deleteFrameBuffer(&frameBuffer);
        frameBuffer.handle = 0;
        rendererBindFrameBuffer(&frameBuffer);

    }

    TextureAtlas readTextureAtlas(char *jsonFileName, char *textureFileName) {
        TextureAtlas result = {};

        FileContents contents = getFileContentsNullTerminate(jsonFileName);
        assert(contents.valid);
        assert(contents.fileSize > 0);
        assert(contents.memory);

        EasyTokenizer tokenizer = lexBeginParsing(contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

        bool parsing = true;
        while(parsing) {
            EasyToken t = lexGetNextToken(&tokenizer);

            if(t.type == TOKEN_NULL_TERMINATOR) {
                parsing = false;
            } else if(t.type == TOKEN_OPEN_BRACKET) {
                //NOTE: Get the item out
                float4 uv = make_float4(0, 0, 0, 0);

                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                char *assetName = easyString_copyToArena_(t.at, &globalLongTermArena, t.size);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COMMA);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.x = t.floatVal;
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.y = t.floatVal;
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.z = t.floatVal;
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.w = t.floatVal;

                textureAtlas_addItem(&result, assetName, uv);
            }
        }

        result.texture = loadTextureToGPU(textureFileName);

        return result;
    }
