
bool isUndoBlockSentinel(UndoRedoBlock *block) {
    return (block->isSentintel);
}

void clearUndoRedoList(CanvasTab *tab) {
    //TODO: The undoRedo free list should be on the GameState otherwise a memory leak
    //NOTE: Clear the undo buffer
    UndoRedoBlock *b = tab->undoList;
    while(b) {
        UndoRedoBlock *b1 = b->next;
        b->dispose(&tab->undoBlockFreeList);
        b->next = tab->undoBlockFreeList;
        tab->undoBlockFreeList = b; 

        b = b1;
    }
    tab->undoList = 0;
    assert(!tab->undoList);
}

void undoRedo_reinstateParentBlockAsCurrent(CanvasTab *tab, UndoRedoBlock *parentBlock) {
    assert(parentBlock->type == UNDO_REDO_CANVAS_PARENT);
    tab->currentUndoBlock = parentBlock;
}

void undoRedo_beginUndoParentBlock(CanvasTab *tab) {
    UndoRedoBlock *block = tab->addUndoRedoBlock(tab);
    block->type = UNDO_REDO_CANVAS_PARENT;

}

void undoRedo_endUndoParentBlock(CanvasTab *tab) {
    tab->currentUndoBlock = 0;
}
    
