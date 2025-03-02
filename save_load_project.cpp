#pragma pack(push, 1) 
struct ProjectFile {
    u32 version;
    float brushColor[4];
};
#pragma pack(pop) 

#define PROJECT_FILE_LATEST_VERSION 1
ProjectFile initProjectFile() {
    ProjectFile result = {};
    result.version = PROJECT_FILE_LATEST_VERSION;
    return result;

}

bool saveProjectFile(GameState *gameState) {
    bool result = false;

    ProjectFile file = initProjectFile();
    file.brushColor[0] = gameState->colorPicked.E[0];
    file.brushColor[1] = gameState->colorPicked.E[1];
    file.brushColor[2] = gameState->colorPicked.E[2];
    file.brushColor[3] = gameState->colorPicked.E[3];

    return result;

}

void loadProjectFile(GameState *gameState) {

}