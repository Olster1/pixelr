
void addUndoRedoBlock(GameState *gameState, u32 lastColor, u32 thisColor, int x, int y) {
    UndoRedoBlock *block = 0;
    bool repeat = false;
    
    if(gameState->undoList) {
        UndoRedoBlock *b = gameState->undoList;
        if(b->x == x && b->y == y && lastColor == thisColor) {
            repeat = true;
        }
    }

    if(!repeat) {

        if(gameState->undoList) {
            //NOTE: Remove all of them off to start at a new undo redo point
            UndoRedoBlock *b = gameState->undoList->prev;
            while(b) {
                b->next = gameState->undoBlockFreeList;
                gameState->undoBlockFreeList = b;

                b = b->prev;
            }
            gameState->undoList->prev = 0;
        }
        
        if(gameState->undoBlockFreeList) {
            block = gameState->undoBlockFreeList;
            gameState->undoBlockFreeList = block->next;
        } else {
            block = pushStruct(&globalLongTermArena, UndoRedoBlock);
        }

        assert(block);

        block->lastColor = lastColor;
        block->thisColor = thisColor;
        block->x = x;
        block->y = y;

        if(gameState->undoList) {
            gameState->undoList->prev = block;
        }
        
        block->next = gameState->undoList;
        block->prev = 0;

        assert(block);
        gameState->undoList = block;
    }
}

bool isUndoBlockSentinel(UndoRedoBlock *block) {
    return (block->x < 0 || block->y < 0);
}

void clearUndoRedoList(GameState *gameState) {
    //NOTE: Clear the undo buffer
    UndoRedoBlock *b = gameState->undoList;
    while(b) {
        UndoRedoBlock *b1 = b->next;
        b->next = gameState->undoBlockFreeList;
        gameState->undoBlockFreeList = b; 

        b = b1;
    }
    gameState->undoList = 0;
    assert(!gameState->undoList);
}
