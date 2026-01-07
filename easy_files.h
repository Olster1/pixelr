#include <dirent.h>
typedef struct {
    bool valid;
    size_t fileSize;
    unsigned char *memory;
} FileContents;

char *getFileExtension(char *fileName) {
    char *result = fileName;
    
    bool hasDot = false;
    while(*fileName) {
        if(*fileName == '.') { 
            result = fileName + 1;
            hasDot = true;
        }
        fileName++;
    }
    
    if(!hasDot) {
        result = 0;
    }
    
    return result;
}

char *getFileLastPortion_(char *buffer, int bufferLen, char *at, Arena *arena) {
    char *recent = at;
    while(*at) {
        if((*at == '/' || (*at == '\\'))&& at[1] != '\0') { 
            recent = (at + 1); //plus 1 to pass the slash
        }
        at++;
    }
    
    char *result = buffer;
    int length = (int)(at - recent) + 1; //for null termination
    if(!result) {
        if(arena) {
            // assert(false);
            result = (char *)pushArray(arena, length, char);
        } else {
            result = (char *)easyPlatform_allocateMemory(length, EASY_PLATFORM_MEMORY_ZERO);    
        }
        
    } else {
        assert(bufferLen >= length);
        buffer[length] = '\0'; //null terminate. 
    }
    
    easyPlatform_copyMemory(result, recent, length - 1);
    result[length - 1] = '\0';
    
    return result;
}
#define getFileLastPortion_allocateToHeap(at) getFileLastPortion_(0, 0, at, 0)
#define getFileLastPortionWithBuffer(buffer, bufferLen, at) getFileLastPortion_(buffer, bufferLen, at, 0)
#define getFileLastPortionWithArena(at, arena) getFileLastPortion_(0, 0, at, arena)

#define getFileLastPortionWithoutExtension_arena(name, arena) getFileLastPortionWithoutExtension_(name, arena)
#define getFileLastPortionWithoutExtension(name) getFileLastPortionWithoutExtension_(name, 0)
char *getFileLastPortionWithoutExtension_(char *name, Arena *arena) {
    char *lastPortion = 0;
    if(arena) {
        lastPortion = getFileLastPortionWithArena(name, arena);
    } else {
        lastPortion = getFileLastPortion_allocateToHeap(name);
    }
    
    char *at = lastPortion;
    while(*at) {
        if(*at == '.') { 
            break;
        }
        at++;
    }
    
    int length = (int)(at - lastPortion) + 1; //for null termination

    char *result = 0;
    if(arena) {
        // assert(false);
        result = pushArray(arena, length, char);
    } else {
        result = (char *)easyPlatform_allocateMemory(sizeof(char)*length, EASY_PLATFORM_MEMORY_ZERO);    
    }
    
    
    easyPlatform_copyMemory(result, lastPortion, length - 1);
    result[length - 1] = '\0';

    if(arena) {
        //do nothing
    } else {
        easyPlatform_freeMemory(lastPortion);    
        lastPortion = 0;
    }
    
    return result;
}

typedef struct {
    char *names[4096]; //max 4096 files
    int count;
} FileNameOfType;

bool isInCharList(char *ext, char **exts, int count) {
    bool result = false;
    for(int i = 0; i < count; i++) {
        if(easyString_stringsMatch_nullTerminated(ext, exts[i])) {
            result = true;
            break;
        }
    }
    return result;
}

typedef struct {
    void *Data;
    bool HasErrors;
}  game_file_handle;

char *getPlatformSaveFilePath() {
    char* filePath = SDL_GetPrefPath("edgeeffectmedia", DEFINED_FILE_NAME);
    if(!filePath) {
        filePath = "./";
    }
    return filePath;
}

size_t GetFileSize(SDL_RWops *FileHandle) {
    long Result = SDL_RWseek(FileHandle, 0, RW_SEEK_END);
    if(Result < 0) {
        assert(!"Seek Error");
    }
    if(SDL_RWseek(FileHandle, 0, RW_SEEK_SET) < 0) {
        assert(!"Seek Error");
    }
    return (size_t)Result;
}

game_file_handle platformBeginFileRead(char *FileName)
{
    game_file_handle Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        Result.HasErrors = true;
        printf("%s\n", SDL_GetError());
    }
    
    return Result;
}

game_file_handle platformBeginFileWrite(const char *FileName)
{
    game_file_handle Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "wb");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        const char* Error = SDL_GetError();
        Result.HasErrors = true;
    }
    
    return Result;
}

void platformEndFile(game_file_handle Handle)
{
    SDL_RWops*  FileHandle = (SDL_RWops* )Handle.Data;
    if(FileHandle) {
        SDL_RWclose(FileHandle);
    }
}

static int platformWriteFile(game_file_handle *Handle, void *Memory, size_t Size, int Offset)
{
    Handle->HasErrors = false;
    SDL_RWops *FileHandle = (SDL_RWops *)Handle->Data;
    if(!Handle->HasErrors)
    {
        Handle->HasErrors = true; 
        
        if(FileHandle)
        {
            
            if(SDL_RWseek(FileHandle, Offset, RW_SEEK_SET) >= 0)
            {
                if(SDL_RWwrite(FileHandle, Memory, 1, Size) == Size)
                {
                    Handle->HasErrors = false;
                }
                else
                {
                    assert(!"write file did not succeed");
                }
            }
        }
    }

    assert(Offset >= 0);
    assert(Size >= 0);

    return (int)(Offset + Size);
}

FileContents platformReadFile(game_file_handle Handle, void *Memory, size_t Size, int Offset)
{
    FileContents Result = {};
    
    SDL_RWops* FileHandle = (SDL_RWops*)Handle.Data;
    if(!Handle.HasErrors)
    {
        if(FileHandle)
        {
            if(SDL_RWseek(FileHandle, Offset, RW_SEEK_SET) >= 0)
            {
                if(SDL_RWread(FileHandle, Memory, 1, Size) == Size)
                {
                    Result.memory = (unsigned char *)Memory;
                    Result.fileSize = Size;
                    Result.valid = true;
                }
                else
                {
                    assert(!"Read file did not succeed");
                    Result.valid = false;
                }
            }
        }
    }    
    return Result;
    
}
size_t platformFileSize(char *FileName)
{
    size_t Result = 0;
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "rb");
    
    if(FileHandle)
    {
        Result = GetFileSize(FileHandle);
        SDL_RWclose(FileHandle);
    }
    
    return Result;
}

bool platformDoesFileExist(char *FileName) {
    assert(FileName);
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r");
        
    bool result = false;    
    if(FileHandle) { 
        SDL_RWclose(FileHandle);
        result = true; 
    }

    return result;
}

FileContents platformReadEntireFile(char *FileName, bool nullTerminate) {
    FileContents Result = {};
    assert(FileName);
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r+b");
    
    if(FileHandle)
    {
        size_t allocSize = Result.fileSize = GetFileSize(FileHandle);

        if(nullTerminate) { allocSize += 1; }

        Result.memory = (unsigned char *)calloc(allocSize, 1);
        size_t ReturnSize = SDL_RWread(FileHandle, Result.memory, 1, Result.fileSize);
        if(ReturnSize == Result.fileSize)
        {
            if(nullTerminate) {
                Result.memory[Result.fileSize] = '\0'; // put at the end of the file
                Result.fileSize += 1;
            }
            Result.valid = true;
            //NOTE(Oliver): Successfully read
        } else {
            assert(!"Couldn't read file");
            Result.valid = false;
            free(Result.memory);
            Result.memory = 0;
        }
        SDL_RWclose(FileHandle);
    } else {
        Result.valid = false;
        const char *Error = SDL_GetError();
        printf("%s\n", Error);
        // assert(!"Couldn't open file");
    }
    return Result;
}

static inline void easyFile_endFileContents(FileContents *contents) {
    free(contents->memory);
    contents->memory = 0;
} 

static inline FileContents getFileContentsNullTerminate(char *fileName) {
    FileContents result = platformReadEntireFile(fileName, true);
    return result;
}

static inline FileContents getFileContents(char *fileName) {
    FileContents result = platformReadEntireFile(fileName, false);
    return result;
}

char *platformGetUniqueDirName(char *dirName) {
    char timeStampBuffer[512] = {};
    snprintf(timeStampBuffer, arrayCount(timeStampBuffer), "%lu/", (unsigned long)time(NULL)); 
    char *newDirName = concat(dirName, timeStampBuffer);
    return newDirName;
}

