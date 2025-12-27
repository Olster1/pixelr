
u32 float4_to_u32_color(float4 c) {
    DEBUG_TIME_BLOCK()
    return (((u32)(c.w*255.0f)) << 24) | (((u32)(c.z*255.0f)) << 16) | (((u32)(c.y*255.0f)) << 8) | (((u32)(c.x*255.0f)) << 0);
}

float4 u32_to_float4_color(u32 c) {
    DEBUG_TIME_BLOCK()
    float a = ((float)((c >> 24) & 0xFF))/255.0f;
    float b = ((float)((c >> 16) & 0xFF))/255.0f;
    float g = ((float)((c >> 8) & 0xFF))/255.0f;
    float r = ((float)((c >> 0) & 0xFF))/255.0f;
    return make_float4(r, g, b, a);
}

enum UndoRedoBlockType {
    UNDO_REDO_PIXELS,
    UNDO_REDO_CANVAS_DELETE,
    UNDO_REDO_FRAME_DELETE,
    UNDO_REDO_CANVAS_CREATE,
    UNDO_REDO_FRAME_CREATE,
    UNDO_REDO_CANVAS_SWAP
};

struct FrameInfo {
    UndoRedoBlockType canvasType;
    int frameIndex;
    int canvasIndex; //NOTE: Only relevant if the type is CANVAS_TYPE_CANVAS
    int canvasIndexB; //NOTE: Only relevant if the type is CANVAS_TYPE_CANVAS_SWAP
    int afterActiveLayer;
    int beforeActiveLayer;
};

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

struct EaselSelectObject {
    Toast *toast;
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

    EaselSelectObject() {
        toast = 0;
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
        if(toast) {
            endImguiToast(toast);
            toast = 0;
        }
        if(pixels) {
            easyPlatform_freeMemory(pixels);
            pixels = 0;
        }
    }

    void dispose() {
        toast = 0;
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
        DEBUG_TIME_BLOCK()
        if(!pixels) {
            DEBUG_TIME_BLOCK_NAMED("INIT RESIZE ARRAY")
            pixels = initResizeArray(PixelClipboardInfo);
        }

        {
            DEBUG_TIME_BLOCK_NAMED("PUSH PIXEL INFO TO ARRAY")
            PixelClipboardInfo p = PixelClipboardInfo(x, y, color);
            pushArrayItem(&pixels, p, PixelClipboardInfo);
        }
    }
};

struct UndoRedoBlock {
    //NOTE: The canvas the block is talking about
    EntityID canvasId; //NOTE: Lives on the long term storeage
    UndoRedoBlockType type = UNDO_REDO_PIXELS;

    PixelInfo *pixelInfos = 0; //NOTE: Resize array 
    FrameInfo frameInfo; //NOTE: If the undo type is a frame or canvas - determined if the pixelInfos == 0

    bool isSentintel = false;

    UndoRedoBlock *next = 0;
    UndoRedoBlock *prev = 0;

    UndoRedoBlock(EntityID canvasId) {
        this->canvasId = canvasId;
    }

    void addFrameInfo(FrameInfo info) {
        assert(!pixelInfos);
        type = info.canvasType;
        this->frameInfo = info;
    }

    void addPixelInfo(PixelInfo info) {
        DEBUG_TIME_BLOCK()
        type = UNDO_REDO_PIXELS;
        if(!pixelInfos) {
            pixelInfos = initResizeArray(PixelInfo);
        }

        pushArrayItem(&pixelInfos, info, PixelInfo);
    }

    void dispose() {
        if(pixelInfos) {
            freeResizeArray(pixelInfos);
            pixelInfos = 0;
        }
    }
};

struct PlayBackAnimation {
    bool playing = false;
    float frameTime = 0.2f;
    float elapsedTime = 0;

    PlayBackAnimation() {

    }
};

struct Canvas {
    EntityID id;

    bool deleted = false; //NOTE: For the undo system we keep all the canvases active, we just say it's deleted

    u32 gpuHandle = 0;
    u32 *pixels = 0;
    int w = 0;
    int h = 0;
    bool visible; //NOTE: Users can turn layers on and off. When off we don't show this layer.
    float opacity = 1.0; //NOTE: Layers can have an overall opacity that can be changed

    Canvas(int w_, int h_) {
        DEBUG_TIME_BLOCK()
        id = makeEntityId(globalRandomStartupSeed);
        {
            DEBUG_TIME_BLOCK_NAMED("ALLOCATE MEMORY")
            w = w_;
            h = h_;
            pixels = (u32 *)easyPlatform_allocateMemory(sizeof(u32)*w_*h_);
        }

        visible = true;
        {
            DEBUG_TIME_BLOCK_NAMED("FILL CANVAS TRANSPARENT")
            fill_pixels_fast(pixels, w*h, 0x00FFFFFF);
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

    bool deleted = false; //NOTE: For the undo system we keep all the canvases active, we just say it's deleted

    FrameBuffer frameBufferHandle = {};

    Frame(int w, int h) {
        DEBUG_TIME_BLOCK()
        layers = initResizeArray(Canvas);
        Canvas f = Canvas(w, h);
        pushArrayItem(&layers, f, Canvas);

        frameBufferHandle = createFrameBuffer(w, h, f.pixels);
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

        if(frameBufferHandle.textureHandle > 0) {
            deleteTextureHandle(frameBufferHandle.textureHandle);
        }

          if(frameBufferHandle.handle > 0) {
            deleteFrameBuffer(&frameBufferHandle);
        }
    }

};

enum BrushShapeType {
    BRUSH_SHAPE_RECTANGLE,
    BRUSH_SHAPE_CIRCLE
};

#define MAX_PALETTE_COUNT 1028
struct CanvasTab {
    EntityID id; //NOTE: Lives on the long term storeage
    EntityID saveId; //NOTE: This is conistent across runs of the program so in backups folder
    Frame *frames = 0;
    int activeFrame = 0;
    bool isInSaveState = true; //NOTE: This is because we save and load the files in the app directory no matter what, so when they reload, even though they're technically saved (in the app dir), it wasn't at a save point when it was saved to the actual user's file. 
    bool valid = true; //NOTE: Whether when the canvasTab is a valid one - used when we load project files and for some reason can't load them

    float zoomFactor = 5.0f;
    float2 cameraP = {};

    PlayBackAnimation playback;
    int w = 0;
    int h = 0;

    float2 *selected = 0;//NOTE: Resize array 
    u32 selectionGpuHandle = 0;

    char *saveFilePath = 0; //NOTE: Allocated on heap - need to free on dispose
    char *fileName = 0; //NOTE: Allocated on heap - need to free on dispose

    UndoRedoBlock *savePositionUndoBlock = 0;
    UndoRedoBlock *savePositionBackupUndoBlock = 0;
    UndoRedoBlock *undoList = 0;
    UndoRedoBlock *undoBlockFreeList = 0;
    UndoRedoBlock *currentUndoBlock = 0;
    int palletteCount = 0;
    u32 colorsPallete[MAX_PALETTE_COUNT];
    float4 colorPicked = make_float4(1, 1, 1, 1);
    float opacity = 1;
    float savedOpacity = 1;//NOTE: When we make a select shape we save the opactiy to reset it to once they finished the select shape editing, so it isn't annoying having to keep chaning the opacity value.
    float eraserSize = DEFAULT_ERASER_SIZE;
    BrushShapeType brushShape = BRUSH_SHAPE_CIRCLE;
    int onionSkinningFrames = 0;
    bool copyFrameOnAdd;;

    bool isOpen = true; //NOTE: Used to close the tab
    u32 uiTabSelectedFlag = 0;

    CanvasTab(int w, int h, char *saveFilePath_);

    Canvas *getActiveCanvas();

    Canvas *addEmptyCanvas();

    void clearSelection();

    void addColorToPalette(u32 color);

    void addUndoInfo(PixelInfo info);
    void addUndoInfo(FrameInfo info);



UndoRedoBlock *addUndoRedoBlock(CanvasTab *c, bool isSentintel = false);
 
    void dispose();
};