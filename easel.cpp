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
    PixelClipboardInfo *pixels;
    bool dragging = false;
    Rect2f boundsCanvasSpace;
    float2 dragOffset;
    float timeAt = 0;

    SelectObject() {
        T = initTransformX();
        pixels = initResizeArray(PixelClipboardInfo);
        isActive = false;
        startCanvasP = make_float2(0, 0);
        boundsCanvasSpace = make_rect2f(0, 0, 0, 0);
        dragOffset = make_float2(0, 0);
        timeAt = 0;
    }

    void clear() {
        if(pixels) {
            clearResizeArray(pixels);
        }
    }

    void dispose() {
        if(pixels) {
            freeResizeArray(pixels);
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


struct CanvasTab {
    Frame *frames = 0;
    int activeFrame = 0;

    PlayBackAnimation playback;
    int w = 0;
    int h = 0;

    float2 *selected = 0;//NOTE: Resize array 
    u32 selectionGpuHandle = 0;

    char *saveFilePath = 0; //NOTE: Allocated on heap - need to free on dispose
    char *fileName = 0; //NOTE: Allocated on heap - need to free on dispose

    UndoRedoBlock *undoList = 0;
    UndoRedoBlock *undoBlockFreeList = 0;
    UndoRedoBlock *currentUndoBlock = 0;

    bool isOpen = true; //NOTE: Used to close the tab

    CanvasTab(int w, int h, char *saveFilePath_) {
        this->w = w;
        this->h = h;
        currentUndoBlock = 0;
        frames = initResizeArray(Frame);
        Frame f = Frame(w, h);
        pushArrayItem(&frames, f, Frame);

        playback = PlayBackAnimation();
        playback.frameTime = 0.2f;

        saveFilePath = saveFilePath_;
        fileName = getFileLastPortion(saveFilePath);

        selectionGpuHandle = createGPUTextureRed(w, h).handle;
        selected = initResizeArray(float2);

        //NOTE:sentintel
        this->addUndoRedoBlock(this, true);
    }


    Canvas *getActiveCanvas() {
        Frame *f = this->frames + this->activeFrame;
        assert(f);

        Canvas *canvas = f->layers + f->activeLayer;
        assert(canvas);

        return canvas;

    }

    void clearSelection() {
        if(selected) {
            clearResizeArray(selected);
        }
    }

    void addUndoInfo(PixelInfo info) {
        if(!currentUndoBlock) {
            addUndoRedoBlock(this);
        }
        assert(currentUndoBlock);
        currentUndoBlock->addPixelInfo(info);
    }



void addUndoRedoBlock(CanvasTab *c, bool isSentintel = false) {
    UndoRedoBlock *block = 0;
    if(c->undoList) {
        //NOTE: Remove all of them off to start at a new undo redo point
        UndoRedoBlock *b = c->undoList->prev;
        while(b) {
            b->dispose();
            b->next = c->undoBlockFreeList;
            c->undoBlockFreeList = b;

            b = b->prev;
        }
        c->undoList->prev = 0;
    }
    
    if(c->undoBlockFreeList) {
        block = c->undoBlockFreeList;
        c->undoBlockFreeList = block->next;
    } else {
        block = pushStruct(&globalLongTermArena, UndoRedoBlock);
    }

    assert(block);

    Canvas *activeCanvas = this->getActiveCanvas();

    *block = UndoRedoBlock(activeCanvas->id);

    block->isSentintel = isSentintel;

    if(c->undoList) {
        c->undoList->prev = block;
    }
    
    block->next = c->undoList;
    block->prev = 0;

    assert(block);
    c->undoList = block;

    //NOTE: add it as the current block
    c->currentUndoBlock = block;
}

    void dispose() {
        //TODO: Clear the undo list 
        if(frames) {
            for(int i = 0; i < getArrayLength(frames); ++i) {
                Frame *f = frames + i;
                f->dispose();
            }

            freeResizeArray(frames);
            frames = 0;
        }

        if(saveFilePath) {
            easyPlatform_freeMemory(saveFilePath);
        }

        if(fileName) {
            easyPlatform_freeMemory(fileName);
        }

        if(selectionGpuHandle > 0) {
            deleteTextureHandle(selectionGpuHandle);
        }
        
        if(selected) {
            freeResizeArray(selected);
            selected = 0;
        }
    }
};