void saveFileToPNG(Canvas *canvas) {
    char const * result = tinyfd_saveFileDialog (
    "Save File",
    "",
    0,
    NULL,
    NULL);

    size_t bytesPerPixel = sizeof(uint32_t);
    int stride_in_bytes = bytesPerPixel*canvas->w;

    char *name = easy_createString_printf(&globalPerFrameArena, "%s.png", (char *)result);
    int writeResult = stbi_write_png(name, canvas->w, canvas->h, 4, canvas->pixels, stride_in_bytes);
}