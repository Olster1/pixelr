
bool canvasTabSaveStateHasChanged(CanvasTab *t) {
    return (((t->savePositionBackupUndoBlock != t->undoList) && !t->currentUndoBlock && t->undoList && !t->undoList->isSentintel));
}

//NOTE: This is for adding blocks to parent blocks if there are multiple undo-redo actions at once of different type
UndoRedoBlock *CanvasTab::addUndoRedoChildBlock(CanvasTab *c, Canvas *canvas, UndoRedoBlock *parentBlock, UndoRedoBlockType type) {
    DEBUG_TIME_BLOCK()
    assert(parentBlock);
    assert(parentBlock->type == UNDO_REDO_CANVAS_PARENT);

    UndoRedoBlock *block = 0;
    if(c->undoBlockFreeList) {
        block = c->undoBlockFreeList;
        c->undoBlockFreeList = block->next;
    } else {
        block = pushStruct(&globalLongTermArena, UndoRedoBlock);
    }
    assert(block);

    //NOTE: Init the undo redo block
    Canvas *activeCanvas = canvas;  
    if(!activeCanvas) {
        activeCanvas = this->getActiveCanvas();
    }

    if(activeCanvas) {
        *block = UndoRedoBlock(activeCanvas->id);
    } 
    block->isSentintel = false;
    block->type = type;
    //////

    if(parentBlock->childUndoRedoBlocks) {
        parentBlock->childUndoRedoBlocks->prev = block;
    }

    //NOTE: Add it as a child block to the parent
    block->next = parentBlock->childUndoRedoBlocks;
    block->prev = 0;
    parentBlock->childUndoRedoBlocks = block;

    //NOTE: add it as the current block
    c->currentUndoBlock = block;

    return block;
}

UndoRedoBlock *CanvasTab::addUndoRedoBlock(CanvasTab *c, bool isSentintel, Canvas *canvas) {
    DEBUG_TIME_BLOCK()
    UndoRedoBlock *block = 0;
    if(c->undoList) {
        //NOTE: Remove all of them off to start at a new undo redo point
        UndoRedoBlock *b = c->undoList->prev;
        while(b) {
            b->dispose(&c->undoBlockFreeList);
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

    Canvas *activeCanvas = canvas;  
    if(!activeCanvas) {
        activeCanvas = this->getActiveCanvas();
    }

    if(activeCanvas) {
        *block = UndoRedoBlock(activeCanvas->id);
    } 

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
    {
        DEBUG_TIME_BLOCK_NAMED("DEFAULT SETUP CANVAS")
        this->id = makeEntityId(globalRandomStartupSeed);
        this->saveId = this->id;
        this->w = w;
        this->h = h;
        this->zoomFactor = 0.3f*MathMax(w, h); //NOTE: Zoom out to fit the image in the screen
        this->colorPicked = make_float4(1, 1, 1, 1);
        this->currentUndoBlock = 0;
        this->palletteCount = 0;
        this->frames = initResizeArray(Frame);
        Frame f = Frame(w, h);
        pushArrayItem(&frames, f, Frame);
    }
    {
        DEBUG_TIME_BLOCK_NAMED("INIT PLAYBACK ANIMATION")
        this->playback = PlayBackAnimation();
        this->playback.frameTime = 0.2f;
    }

    {
        DEBUG_TIME_BLOCK_NAMED("CREATE FILE NAME CANVAS")
        char *fileName = 0;
        if(saveFilePath_) {
            this->saveFilePath = easyString_copyToHeap(saveFilePath_);
            fileName = getFileLastPortion_allocateToHeap(saveFilePath);
        } else {
            this->saveFilePath = 0;
            fileName = easyString_copyToHeap(easy_createString_printf(&globalPerFrameArena, "Untitled %d x %d", w, h));
        }
        this->fileName = fileName;
    }

    {
        DEBUG_TIME_BLOCK_NAMED("CREATE GPU TEXTURES")
        this->selectionGpuHandle = createGPUTextureRed(w, h).handle;

        // u32 *pixels = pushArray(&globalPerFrameArena, w*h, u32);

        // for(int i = 0 ; i < w*h; i++) {
        //     pixels[i] = 0xFF00FF00;
        // }

        this->checkBackgroundFrameBuffer = createFrameBuffer(w, h, 0);
        this->checkerBufferbuilt = false;

    }
    this->selected = initResizeArray(float2);

    //NOTE:sentintel
    {
        DEBUG_TIME_BLOCK_NAMED("addUndoRedoBlock FUNC")
        this->addUndoRedoBlock(this, true);
    }
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

void CanvasTab::addUndoInfo(PixelInfo info, Canvas *canvas) {
    DEBUG_TIME_BLOCK()
    if(!currentUndoBlock) {
        DEBUG_TIME_BLOCK_NAMED("CREATE NEW UNDO BLOCK")
        addUndoRedoBlock(this, false, canvas);
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

    clearUndoRedoList(this);
    
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