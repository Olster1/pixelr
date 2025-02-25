
#define concat_withLength(a, aLength, b, bLength) concat_(a, aLength, b, bLength, 0)
#define concat(a, b) concat_(a, strlen(a), b, strlen(b), 0)
#define concatInArena(a, b, arena) concat_(a, strlen(a), b, strlen(b), arena)
char *concat_(char *a, int lengthA, char *b, int lengthB, Arena *arena) {
    int aLen = lengthA;
    int bLen = lengthB;
    
    int newStrLen = aLen + bLen + 1; // +1 for null terminator
    char *newString = 0;
    if(arena) {
        assert(false);
        // newString = (char *)pushArray(arena, newStrLen, char);
    } else {
        newString = (char *)calloc(newStrLen, 1); 
    }
    assert(newString);
    
    newString[newStrLen - 1] = '\0';
    
    char *at = newString;
    for (int i = 0; i < aLen; ++i)
    {
        *at++ = a[i];
    }
    
    for (int i = 0; i < bLen; ++i)
    {
        *at++ = b[i];
    }
    assert(at == &newString[newStrLen - 1]);
    assert(newString[newStrLen - 1 ] == '\0');
    
    return newString;
}

static char *easyString_copyToHeap(char *at) {
    size_t length = easyString_getSizeInBytes_utf8(at);
    //NOTE(ollie): Get memory from heap
    char *result = (char *)easyPlatform_allocateMemory(sizeof(char)*(length + 1), EASY_PLATFORM_MEMORY_NONE);
    //NOTE(ollie): Copy the string
    easyPlatform_copyMemory(result, at, sizeof(char)*length);
    //NOTE(ollie): Null terminate the string
    result[length] = '\0'; //Null terminate

    return result;
}

static char *easyString_copyToBuffer(char *at, char *buffer, size_t bufferLen) {
    
    assert(easyString_getSizeInBytes_utf8(at) < bufferLen); //NOTE(ollie): Accounting for the null terminator 
    //NOTE(ollie): Copy the string
    easyPlatform_copyMemory(buffer, at, sizeof(char)*bufferLen);
    //NOTE(ollie): Null terminate the string
    buffer[bufferLen - 1] = '\0'; //Null terminate

    return buffer;
}


#define easyString_copyToArena(a, arena) easyString_copyToArena_(a, arena, easyString_getSizeInBytes_utf8(a))
char *easyString_copyToArena_(char *a, Arena *arena, int newStrLen_) {
    int newStrLen = newStrLen_ + 1; //for null terminator
    
    char *newString = (char *)pushArray(arena, newStrLen, char);

    assert(newString);
    
    newString[newStrLen - 1] = '\0';
    
    char *at = newString;
    for (int i = 0; i < (newStrLen - 1); ++i)
    {
        *at++ = a[i];
    }
    assert(newString[newStrLen - 1 ] == '\0');

    return newString;
}


inline char *easy_createString_printf(Arena *arena, char *formatString, ...) {

    va_list args;
    va_start(args, formatString);

    char bogus[4];
    int stringLengthToAlloc = vsnprintf(bogus, 1, formatString, args) + 1; //for null terminator, just to be sure
    
    char *strArray = pushArray(arena, stringLengthToAlloc, char);

    vsnprintf(strArray, stringLengthToAlloc, formatString, args); 

    va_end(args);

    return strArray;
}