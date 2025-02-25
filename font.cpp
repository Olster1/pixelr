#define STB_TRUETYPE_IMPLEMENTATION
#include "./libs/stb_truetype.h"

struct Font {
    unsigned int textureHandle; //NOTE: Handle to the texture on the GPU 
    stbtt_bakedchar glyphData[255]; //NOTE: Meta data for each glyph like width and height and the uv coords in the texture atlas. In this case it's limited to be 255 characters per font. 
    float fontHeight;
    int startOffset;
    float2 fontAtlasDim;
};

Font initFontAtlas(char *fontFile) {
    Font result = {};

    //NOTE: Open & get size of ttf file
    FILE *ttfFile = fopen(fontFile, "rb");

    if(ttfFile){
        fseek(ttfFile, 0, SEEK_END);
        size_t fileSize = ftell(ttfFile);
        fseek(ttfFile, 0L, SEEK_SET);

        //NOTE: Allocate the memory to store the file contents. Plus 1 for zero terminated contents.
        unsigned char *ttfBuffer = (unsigned char *)malloc(fileSize + 1);

        size_t bytesRead = fread(ttfBuffer, 1, fileSize, ttfFile);
        //NOTE: Read the ttf file contents
        if(bytesRead == fileSize) {

            int tempBitmapWidth = 512;
            int tempBitmapHeight = 512;

            result.fontAtlasDim.x = tempBitmapWidth;
            result.fontAtlasDim.y = tempBitmapHeight;

            //NOTE: Allocate the CPU side image to render the glyphs to. We will then upload this to the GPU to use by our game renderer.
            unsigned char *tempBitmap = (unsigned char *)calloc(tempBitmapWidth*tempBitmapHeight*sizeof(unsigned char), 1);
            assert(tempBitmap);

            result.fontHeight = 32.0;
            result.startOffset = 32;
            //NOTE:  32, 96 values denote the main ASCI alphabet - starting at [SPACE] and going to the end of the asci table. The space is important because it tells us the width of a space.
            int r = stbtt_BakeFontBitmap(ttfBuffer, 0, result.fontHeight, tempBitmap, tempBitmapWidth, tempBitmapHeight, 32, 96, result.glyphData);

            if(r == 0) {
                printf("ERROR: Couldn't render font atlas\n");
                assert(false);
            } 

            //NOTE: Upload to the GPU now. You can change this to however you upload textures in your game engine. 
            glGenTextures(1, &result.textureHandle);
            renderCheckError();
            glBindTexture(GL_TEXTURE_2D, result.textureHandle);
            renderCheckError();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tempBitmapWidth, tempBitmapHeight, 0, GL_RED, GL_UNSIGNED_BYTE, tempBitmap);
            renderCheckError();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            renderCheckError();
            glBindTexture(GL_TEXTURE_2D, 0); //NOTE: Unbind the texture 
            renderCheckError();

            //NOTE: Free our CPU side image data now
            free(tempBitmap);  
            tempBitmap = 0;
        } else {
            printf("bytesRead: %ld\n", bytesRead);
            assert(false);
        }

        free(ttfBuffer);
        ttfBuffer = 0;
    } else {
        assert(false);
    }

   return result;

}

void renderText(Renderer *renderer, Font *font, char *nullTerminatedString, float2 start, float scale) {
    float x = start.x;
    float y = start.y;

    while (*nullTerminatedString) {
        if(*nullTerminatedString != '\n') {
            if (*nullTerminatedString >= 32 && *nullTerminatedString < 126) {

                stbtt_aligned_quad q  = {};

                int index = *nullTerminatedString - font->startOffset;
                stbtt_GetBakedQuad(font->glyphData, font->fontAtlasDim.x, font->fontAtlasDim.y, index, &x, &y, &q, 1);

                float width = q.x1 - q.x0;
                float height = q.y1 - q.y0;

                float x1 = 0.5f*width + q.x0;
                float y1 = 0.5f*height + q.y0;

                x1 = (scale*(x1 - start.x)) + start.x;
                y1 = (scale*(y1 - start.y)) + start.y;

                float4 uvCoords = make_float4(q.s0, q.s1, q.t1, q.t0);

                pushGlyph(renderer, make_float3(x1, y1, 1), make_float3(scale*width, scale*height, 1), uvCoords, make_float4(1, 1, 1, 1));
            }
        } else {
            //NOTE: Move down a line
            x = start.x;
            y += font->fontHeight;
        }
        nullTerminatedString++;
    }
}