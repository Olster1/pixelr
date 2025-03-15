
bool isUndoBlockSentinel(UndoRedoBlock *block) {
    return (block->isSentintel);
}

void clearUndoRedoList(CanvasTab *tab) {
    //NOTE: Clear the undo buffer
    UndoRedoBlock *b = tab->undoList;
    while(b) {
        UndoRedoBlock *b1 = b->next;
        b->dispose();
        b->next = tab->undoBlockFreeList;
        tab->undoBlockFreeList = b; 

        b = b1;
    }
    tab->undoList = 0;
    assert(!tab->undoList);
}
