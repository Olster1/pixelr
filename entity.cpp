static int globalRandomStartupSeed = 0;

struct EntityID {
    char *stringID; //NOTE: Null terminated string
    uint32_t crc32Hash;
};

static int global_entityIdCreated = 0;

EntityID makeEntityId(int randomStartUpID) {
    DEBUG_TIME_BLOCK()
    EntityID result = {};

    time_t timeSinceEpoch = time(0);

    #define ENTITY_ID_PRINTF_STRING "%ld-%d-%d", timeSinceEpoch, randomStartUpID, global_entityIdCreated

    //NOTE: Allocate the string
    size_t stringLengthInBytes = snprintf(NULL, 0, ENTITY_ID_PRINTF_STRING);
    result.stringID = pushArray(&globalLongTermArena, stringLengthInBytes + 1, char);//NOTE: +1 for null terminator
    snprintf(result.stringID, stringLengthInBytes + 1, ENTITY_ID_PRINTF_STRING);

    result.crc32Hash = get_crc32(result.stringID, stringLengthInBytes);

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
