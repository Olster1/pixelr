void saveFileToPNG(GameState *gameState) {
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    size_t bytesPerPixel = sizeof(uint32_t);
    int stride_in_bytes = bytesPerPixel*gameState->canvasW;

    char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
    int writeResult = stbi_write_png(name, gameState->canvasW, gameState->canvasH, 4, gameState->canvas, stride_in_bytes);
}