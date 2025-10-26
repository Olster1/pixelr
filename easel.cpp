

bool isCanvasTabSaved(CanvasTab *t) {
    return ((t->savePositionUndoBlock == t->undoList) && t->saveFilePath);
}

bool canvasTabSaveStateHasChanged(CanvasTab *t) {
    return ((t->savePositionBackupUndoBlock != t->undoList) && !t->currentUndoBlock && t->undoList && !t->undoList->isSentintel);
}

UndoRedoBlock *CanvasTab::addUndoRedoBlock(CanvasTab *c, bool isSentintel) {
    DEBUG_TIME_BLOCK()
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

    return block;
}

CanvasTab::CanvasTab(int w, int h, char *saveFilePath_) {
    DEBUG_TIME_BLOCK()
    this->w = w;
    this->h = h;
    this->zoomFactor = 0.3f*MathMax(w, h); //NOTE: Zoom out to fit the image in the screen
    this->colorPicked = make_float4(1, 1, 1, 1);
    this->currentUndoBlock = 0;
    this->palletteCount = 0;
    this->frames = initResizeArray(Frame);
    Frame f = Frame(w, h);
    pushArrayItem(&frames, f, Frame);

    this->playback = PlayBackAnimation();
    this->playback.frameTime = 0.2f;
    
    char *fileName = 0;
    if(saveFilePath_) {
        this->saveFilePath = easyString_copyToHeap(saveFilePath_);
        fileName = getFileLastPortion_allocateToHeap(saveFilePath);
    } else {
        this->saveFilePath = 0;
        fileName = easyString_copyToHeap(easy_createString_printf(&globalPerFrameArena, "Untitled %d x %d", w, h));
    }
    this->fileName = fileName;

    u32 *checkData = (u32 *)pushArray(&globalPerFrameArena, w*h, u32);

    int checkerWidth = w / 16;
    //TODO: This should just be a shader that does this
    for(int y = 0; y < h; ++y) {
        for(int x = 0; x < w; ++x) {
            float4 c = make_float4(0.8f, 0.8f, 0.8f, 1);
            if((((y / checkerWidth) % 2) == 0 && ((x / checkerWidth) % 2) == 1) || (((y/ checkerWidth) % 2) == 1 && ((x/ checkerWidth) % 2) == 0)) {
                c = make_float4(0.6f, 0.6f, 0.6f, 1);
            }
            checkData[y*w + x] = float4_to_u32_color(c);
        }
    }

    this->checkBackgroundHandle = createGPUTexture(w, h, checkData).handle;
    this->selectionGpuHandle = createGPUTextureRed(w, h).handle;
    this->selected = initResizeArray(float2);

    //NOTE:sentintel
    this->addUndoRedoBlock(this, true);
    this->savePositionUndoBlock = this->undoList;
}


Canvas *CanvasTab::getActiveCanvas() {
    DEBUG_TIME_BLOCK()
    Frame *f = this->frames + this->activeFrame;
    assert(f);

    Canvas *canvas = f->layers + f->activeLayer;
    assert(canvas);

    return canvas;

}

Canvas *CanvasTab::addEmptyCanvas() {
    DEBUG_TIME_BLOCK()
    Frame f_ = Frame(w, h);
    Frame *f = pushArrayItem(&frames, f_, Frame);

    return &f->layers[0];
}

void CanvasTab::clearSelection() {
    DEBUG_TIME_BLOCK()
    if(selected) {
        clearResizeArray(selected);
    }
}

void CanvasTab::addColorToPalette(u32 color) {
    DEBUG_TIME_BLOCK()
    if(this->palletteCount < arrayCount(this->colorsPallete)) {
        bool isNew = true;
        for(int i = 0; i < this->palletteCount && isNew; ++i) {
            if(this->colorsPallete[i] == color) {
                isNew = false;
                break;
            }
        }
        if(isNew) {
            this->colorsPallete[this->palletteCount++] = color;
            savePalleteDefaultThreaded(globalThreadInfo, this);
        }
        
    }
}

void CanvasTab::addUndoInfo(PixelInfo info) {
    DEBUG_TIME_BLOCK()
    if(!currentUndoBlock) {
        DEBUG_TIME_BLOCK_NAMED("CREATE NEW UNDO BLOCK")
        addUndoRedoBlock(this);
        addColorToPalette(float4_to_u32_color(colorPicked));
    }
    {
        DEBUG_TIME_BLOCK_NAMED("ADD PIXEL INFO TO BLOCK")
        assert(currentUndoBlock);
        currentUndoBlock->addPixelInfo(info);
    }
    
}

void CanvasTab::addUndoInfo(FrameInfo info) {
    DEBUG_TIME_BLOCK()
    if(!currentUndoBlock) {
        addUndoRedoBlock(this);
    }
    assert(currentUndoBlock);
    currentUndoBlock->addFrameInfo(info);

    //NOTE: Clear this undo block straight away
    this->currentUndoBlock = 0;
}

void CanvasTab::dispose() {
    DEBUG_TIME_BLOCK()
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
    if(checkBackgroundHandle > 0) {
        deleteTextureHandle(checkBackgroundHandle);
    }
    
    if(selected) {
        freeResizeArray(selected);
        selected = 0;
    }
}