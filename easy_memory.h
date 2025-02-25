#ifndef EASY_PLATFORM_H
/*
Code for platform depedent functions 
*/

void easyMemory_zeroSize(void *memory, size_t bytes) {
    char *at = (char *)memory;
    for(int i = 0; i < bytes; i++) {
        *at = 0;
        at++;
    }
}

typedef enum {
    EASY_PLATFORM_MEMORY_NONE,
    EASY_PLATFORM_MEMORY_ZERO,
} EasyPlatform_MemoryFlag;

static void *easyPlatform_allocateMemory(size_t sizeInBytes, EasyPlatform_MemoryFlag flags = EASY_PLATFORM_MEMORY_ZERO) {
    
    void *result = 0;
#if 0//_WIN32
    
    ///////////////////////************ Win32 *************////////////////////
    
    result = HeapAlloc(GetProcessHeap(), 0, sizeInBytes);
    
    ////////////////////////////////////////////////////////////////////
    
#else
    ///////////////////////************ C runtime *************//////////////////// 
    
    result = malloc(sizeInBytes);
    
    ////////////////////////////////////////////////////////////////////
#endif
    
    if(!result) {
        assert(false);
    }
    
    if(flags & EASY_PLATFORM_MEMORY_ZERO) {
        easyMemory_zeroSize(result, sizeInBytes);
    }
    
    return result;
}

static void easyPlatform_freeMemory(void *memory) {
    
#if 0//_WIN32
    HeapFree(GetProcessHeap(), 0, memory);
#else 
    free(memory);
    memory = 0;
#endif
}


static inline void easyPlatform_copyMemory(void *to, void *from, size_t sizeInBytes) {
    memcpy(to, from, sizeInBytes);
}

static inline char * easyPlatform_reallocMemory(void *from, size_t oldSize, size_t newSize) {
    char *result = (char *)easyPlatform_allocateMemory(newSize, EASY_PLATFORM_MEMORY_ZERO);

    if(from) {
        easyPlatform_copyMemory(result, from, oldSize);
        easyPlatform_freeMemory(from);
        from = 0;
    }
    return result;
}

#define EASY_PLATFORM_H 1
#endif