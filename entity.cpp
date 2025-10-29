static int globalRandomStartupSeed = 0;

struct EntityID {
    size_t stringSizeInBytes; //NOTE: Not include null terminator
    char stringID[256]; 
    uint32_t crc32Hash;
};

static int global_entityIdCreated = 0;

EntityID makeEntityId(int randomStartUpID) {
    DEBUG_TIME_BLOCK()
    EntityID result = {};

    time_t timeSinceEpoch = time(0);

    #define ENTITY_ID_PRINTF_STRING "%ld-%d-%d", timeSinceEpoch, randomStartUpID, global_entityIdCreated

    //NOTE: Allocate the string
    size_t bufsz = snprintf(NULL, 0, ENTITY_ID_PRINTF_STRING) + 1;
    assert(bufsz < arrayCount(result.stringID));
    result.stringSizeInBytes = MathMin_sizet(arrayCount(result.stringID), bufsz);
    snprintf(result.stringID, bufsz, ENTITY_ID_PRINTF_STRING);

    result.crc32Hash = get_crc32(result.stringID, result.stringSizeInBytes);

    #undef ENTITY_ID_PRINTF_STRING

    //NOTE: This would have to be locked in threaded application
    global_entityIdCreated++;

    return result;
} 

bool areEntityIdsEqual(EntityID a, EntityID b) {
    bool result = false;
    if(a.crc32Hash == b.crc32Hash && easyString_stringsMatch_nullTerminated(a.stringID, b.stringID)) {
        result = true;
    }
    return result;
}

struct Camera {
    TransformX T;
    float fov;
    float targetFov;
    float shakeTimer;
    float runShakeTimer;
    bool followingPlayer;
};
