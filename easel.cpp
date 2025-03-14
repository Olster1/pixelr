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
    float timeAt;

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
    PixelInfo *pixelInfos; //NOTE: Resize array 

    bool isSentintel;

    UndoRedoBlock *next;
    UndoRedoBlock *prev;

    void addPixelInfo(PixelInfo info) {
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
    int currentFrame = 0;
    bool playing = false;
    float frameTime = 0.2f;
    float elapsedTime = 0;

    PlayBackAnimation() {

    }
};


struct Canvas {
    u32 gpuHandle = 0;
    u32 *pixels = 0;
    int w;
    int h;

    

    Canvas(int w_, int h_) {
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
    int w;
    int h;

    float2 *selected = 0;//NOTE: Resize array 
    u32 selectionGpuHandle = 0;

    char *saveFilePath; //NOTE: Allocated on heap - need to free on dispose
    char *fileName; //NOTE: Allocated on heap - need to free on dispose

    UndoRedoBlock *undoList;
    UndoRedoBlock *undoBlockFreeList;
    UndoRedoBlock *currentUndoBlock;

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
        addUndoRedoBlock(this, true);
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