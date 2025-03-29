
u32 float4_to_u32_color(float4 c) {
    return (((u32)(c.w*255.0f)) << 24) | (((u32)(c.z*255.0f)) << 16) | (((u32)(c.y*255.0f)) << 8) | (((u32)(c.x*255.0f)) << 0);
}

float4 u32_to_float4_color(u32 c) {
    float a = ((float)((c >> 24) & 0xFF))/255.0f;
    float b = ((float)((c >> 16) & 0xFF))/255.0f;
    float g = ((float)((c >> 8) & 0xFF))/255.0f;
    float r = ((float)((c >> 0) & 0xFF))/255.0f;
    return make_float4(r, g, b, a);
}


struct PixelInfo {
    int x; 
    int y;
    u32 lastColor;
    u32 thisColor;

    PixelInfo(int x, int y, u32 lastColor, u32 thisColor) {
        this->x = x;
        this->y = y;
        this->lastColor = lastColor;
        this->thisColor = thisColor; 
    }
};

struct PixelClipboardInfo {
    int x; 
    int y;
    u32 color;

    PixelClipboardInfo(float x, float y, u32 color) {
        this->x = x;
        this->y = y;
        this->color = color;
    }
};

struct SelectObject {
    bool isActive = false;
    TransformX T;
    float2 startCanvasP;
    float2 origin; //NOTE: Bottom left corner
    float2 wh;
    u32 *pixels;
    bool dragging = false;
    Rect2f boundsCanvasSpace;
    float2 dragOffset;
    float timeAt = 0;

    SelectObject() {
        T = initTransformX();
        pixels = 0;
        isActive = false;
        startCanvasP = make_float2(0, 0);
        boundsCanvasSpace = make_rect2f(0, 0, 0, 0);
        dragOffset = make_float2(0, 0);
        origin = make_float2(0, 0);
        timeAt = 0;
        wh = make_float2(0, 0);
    }

    float4 getColor(float2 xy) {
        int x = xy.x;
        int y = xy.y;
        float4 result = make_float4(1, 1, 1, 0);
        if(pixels && x >= 0 && x < wh.x && y >= 0 && y < wh.y) {
            float4 a = u32_to_float4_color(pixels[y*(int)wh.x + x]);
            if(a.w > 0) {
                result = a;
            }
        }
        return result;
    }

    void clear() {
        if(pixels) {
            easyPlatform_freeMemory(pixels);
            pixels = 0;
        }
    }

    void dispose() {
        if(pixels) {
            easyPlatform_freeMemory(pixels);
            pixels = 0;
        }
    }
};

struct Clipboard {
    PixelClipboardInfo *pixels = 0;
    Clipboard() {

    }

    void clear() {
        if(pixels) {
            freeResizeArray(pixels);
            pixels = 0;
        }
    }

    bool hasCopy() {
        return pixels != 0 && getArrayLength(pixels);
    }

    void addPixelInfo(float x, float y, u32 color) {
        if(!pixels) {
            pixels = initResizeArray(PixelClipboardInfo);
        }

        PixelClipboardInfo p = PixelClipboardInfo(x, y, color);
        pushArrayItem(&pixels, p, PixelClipboardInfo);
    }
};

struct UndoRedoBlock {
    //NOTE: The canvas the block is talking about
    EntityID canvasId; //NOTE: Lives on the long term storeage

    PixelInfo *pixelInfos = 0; //NOTE: Resize array 

    bool isSentintel = false;

    UndoRedoBlock *next = 0;
    UndoRedoBlock *prev = 0;

    UndoRedoBlock(EntityID canvasId) {
        this->canvasId = canvasId;
    }

    void addPixelInfo(PixelInfo info) {
        if(!pixelInfos) {
            pixelInfos = initResizeArray(PixelInfo);
        }

        bool found = false;
        //NOTE: Check if we already have this pixel info
        for(int i = 0; i < getArrayLength(pixelInfos) && !found; ++i) {
            PixelInfo *infoCheck = &pixelInfos[i];

            if(infoCheck->x == info.x && infoCheck->y == info.y) {
                //NOTE: Update just the next color - we want to keep the last color as the first one to recreate the board the same
                infoCheck->thisColor = info.thisColor;
                found = true;
                break;
            }
        }
        

        if(!found) {
            pushArrayItem(&pixelInfos, info, PixelInfo);
        }
    }

    void dispose() {
        if(pixelInfos) {
            freeResizeArray(pixelInfos);
            pixelInfos = 0;
        }
    }
};

struct PlayBackAnimation {
    int currentFrame = 0;
    bool playing = false;
    float frameTime = 0.2f;
    float elapsedTime = 0;

    PlayBackAnimation() {

    }
};


struct Canvas {
    EntityID id;

    u32 gpuHandle = 0;
    u32 *pixels = 0;
    int w = 0;
    int h = 0;

    Canvas(int w_, int h_) {
        id = makeEntityId(globalRandomStartupSeed);
        w = w_;
        h = h_;
        pixels = (u32 *)easyPlatform_allocateMemory(sizeof(u32)*w_*h_);

        for(int y = 0; y < h; ++y) {
            for(int x = 0; x < w; ++x) {
                pixels[y*w + x] = 0x00FFFFFF;
            }
        }

        gpuHandle = createGPUTexture(w, h, pixels).handle;

        
    }

    void dispose() {
        if(pixels) {
            easyPlatform_freeMemory(pixels);
            pixels = 0;
        }
        
        if(gpuHandle > 0) {
            deleteTextureHandle(gpuHandle);
        }
      
    }
};

struct Frame {
    Canvas *layers = 0;
    int activeLayer = 0;

    u32 gpuHandle = 0;

    Frame(int w, int h) {
        layers = initResizeArray(Canvas);
        Canvas f = Canvas(w, h);
        pushArrayItem(&layers, f, Canvas);

        gpuHandle = createGPUTexture(w, h, f.pixels).handle;
    }

    void dispose() {
        if(layers) {
            for(int i = 0; i < getArrayLength(layers); ++i) {
                Canvas *c = layers + i;
                c->dispose();
            }
            freeResizeArray(layers);
            layers = 0;
        }

        if(gpuHandle > 0) {
            deleteTextureHandle(gpuHandle);
        }
    }

};

#define MAX_PALETTE_COUNT 1028
struct CanvasTab {
    Frame *frames = 0;
    int activeFrame = 0;

    PlayBackAnimation playback;
    int w = 0;
    int h = 0;

    float2 *selected = 0;//NOTE: Resize array 
    u32 selectionGpuHandle = 0;
    u32 checkBackgroundHandle = 0; 

    char *saveFilePath = 0; //NOTE: Allocated on heap - need to free on dispose
    char *fileName = 0; //NOTE: Allocated on heap - need to free on dispose

    UndoRedoBlock *undoList = 0;
    UndoRedoBlock *undoBlockFreeList = 0;
    UndoRedoBlock *currentUndoBlock = 0;
    int palletteCount = 0;
    u32 colorsPallete[MAX_PALETTE_COUNT];
    float4 colorPicked = make_float4(1, 1, 1, 1);
    float opacity = 1;
    float savedOpacity = 1;//NOTE: When we make a select shape we save the opactiy to reset it to once they finished the select shape editing, so it isn't annoying having to keep chaning the opacity value.

    bool isOpen = true; //NOTE: Used to close the tab

    CanvasTab(int w, int h, char *saveFilePath_);


    Canvas *getActiveCanvas();

    Canvas *addEmptyCanvas();

    void clearSelection();

    void addColorToPalette(u32 color);

    void addUndoInfo(PixelInfo info);



void addUndoRedoBlock(CanvasTab *c, bool isSentintel = false);
 
    void dispose();
};