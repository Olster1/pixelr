
struct PixelInfo {
    int x; 
    int y;
    u32 lastColor;
    u32 thisColor;
};

struct UndoRedoBlock {
    int x; 
    int y;
    u32 lastColor;
    u32 thisColor;
    
    PixelInfo *pixelInfos; //NOTE: Resize array 

    bool isSentintel;

    UndoRedoBlock *next;
    UndoRedoBlock *prev;

    void onDispose() {
        if(pixelInfos) {
            freeResizeArray(pixelInfos);
            pixelInfos = 0;
        }
    }
};

struct PlayBackAnimation {
    unsigned int *frameTextures;
    int currentFrame;
    bool playing;
    float frameTime;
    float elapsedTime;
};


struct Canvas {
    u32 gpuHandle;
    u32 *pixels;
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
    }

    void dispose() {
        if(pixels) {
            easyPlatform_freeMemory(pixels);
            pixels = 0;

        }
    }
};

struct Frame {
    Canvas *layers;
    int activeLayer = 0;

    Frame(int w, int h) {
        layers = initResizeArray(Canvas);
        Canvas f = Canvas(w, h);
        pushArrayItem(&layers, f, Canvas);
    }

    void dispose() {
        if(layers) {
            freeResizeArray(layers);
            layers = 0;
        }
    }

};


struct CanvasTab {
    Frame *frames;
    int activeFrame = 0;

    PlayBackAnimation playback;
    int w;
    int h;

    char *saveFilePath; //NOTE: Allocated on heap - need to free on dispose
    char *fileName; //NOTE: Allocated on heap - need to free on dispose

    UndoRedoBlock *undoList;

    CanvasTab(int w, int h, char *saveFilePath_) {
        frames = initResizeArray(Frame);
        Frame f = Frame(w, h);
        pushArrayItem(&frames, f, Frame);
        playback.frameTime = 0.2f;

        saveFilePath = saveFilePath_;
        fileName = getFileLastPortion(saveFilePath);


        //TODO:Complete
        // addUndoRedoBlock(gameState, 0, 0, -1, -1, true);
    }

    void dispose() {
        //TODO: Clear the undo list 
        if(frames) {
            freeResizeArray(frames);
            frames = 0;
        }

        if(saveFilePath) {
            easyPlatform_freeMemory(saveFilePath);
        }

        if(fileName) {
            easyPlatform_freeMemory(saveFilePath);
        }
    }
};