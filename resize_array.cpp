struct ResizeArrayHeader {
    size_t sizeOfElement;
    int elementsCount;
    int maxCount;
};

#define initResizeArray(type) (type *)initResizeArray_(sizeof(type))

u8 *initResizeArray_(size_t sizeOfElement) {
    ResizeArrayHeader *header =(ResizeArrayHeader *)easyPlatform_allocateMemory(sizeOfElement + sizeof(ResizeArrayHeader), EASY_PLATFORM_MEMORY_ZERO);
    u8 *array = ((u8 *)header) + sizeof(ResizeArrayHeader);

    header->sizeOfElement = sizeOfElement;
    header->elementsCount = 0;
    header->maxCount = 1;

    return array;
}

ResizeArrayHeader *getResizeArrayHeader(u8 *array) {
    ResizeArrayHeader *header = (ResizeArrayHeader *)(((u8 *)array) - sizeof(ResizeArrayHeader));
    return header;
}

void freeResizeArray(void *array_) {
    u8 *array = (u8 *)array_;
    ResizeArrayHeader *header = getResizeArrayHeader(array);
    easyPlatform_freeMemory(header);
}

u8 *getResizeArrayContents(ResizeArrayHeader *header) {
    u8 *array = ((u8 *)header) + sizeof(ResizeArrayHeader);
    return array;
}

int getArrayLength(void *array) {
    if(!array) {
        return 0;
    }
    ResizeArrayHeader *header = getResizeArrayHeader((u8 *)array);
    assert(header->elementsCount <= header->maxCount);
    int result = header->elementsCount;
    return result;
}

// bool removeArrayIndex(void *array, int index) {
//     ResizeArrayHeader *header = getResizeArrayHeader((u8 *)array);
//     assert(header->elementsCount <= header->maxCount);
//     assert(index < header->elementsCount);
//     return result;
// }

#define pushArrayItem(array_, data, type)  (type *)pushArrayItem_((void **)array_, &data)
void *pushArrayItem_(void **array_, void *data) {
    u8 *array = *((u8 **)array_);
    u8 *newPos = 0;
    if(array) {
        ResizeArrayHeader *header = getResizeArrayHeader(array);

        if(header->elementsCount == header->maxCount) {
            //NOTE: Resize array
            size_t oldSize = header->maxCount*header->sizeOfElement + sizeof(ResizeArrayHeader);
            float resizeFactor = 1.5; //NOTE: Same as MSVC C++ Vector. x2 on GCC c++ Vector
            header->maxCount = round(header->maxCount*resizeFactor); 
            size_t newSize = header->maxCount*header->sizeOfElement + sizeof(ResizeArrayHeader);
            header = (ResizeArrayHeader *)easyPlatform_reallocMemory(header, oldSize, newSize);

            array = getResizeArrayContents(header);
        } 

        newPos = array + (header->elementsCount * header->sizeOfElement);
        header->elementsCount++;

        easyPlatform_copyMemory(newPos, data, header->sizeOfElement);
    }

    *array_ = array;

    return newPos;
}

struct TestStruct {
    int x;
    int y; 
    int z;
};

void DEBUG_ArrayTests() {
    TestStruct *blocks = initResizeArray(TestStruct);
    TestStruct b;
    pushArrayItem(&blocks, b, TestStruct);
    assert(getArrayLength(blocks) == 1);

    pushArrayItem(&blocks, b, TestStruct);
    assert(getArrayLength(blocks) == 2);        

    b.y = 2;

    TestStruct *t = pushArrayItem(&blocks, b, TestStruct);
    assert(getArrayLength(blocks) == 3);
    assert(t->y == 2);


}