void resizeCanvasImages() {
    char *inputFolderPath =
      "/Users/olivermarsh/Documents/dev/studytimer/images/dark_study_images/";
    char *outputFolderPath = "./dark_study_images";
    char *exts[] = {"png"};

    stbi_set_flip_vertically_on_load(1);
    FileNameOfType files = getDirectoryFilesOfType(inputFolderPath, exts, arrayCount(exts));

    for(int i = 0; i < files.count; ++i) {
        int width = 0;
        int  height = 0;
        int nrChannels = 0;
        int padding = 450;

        printf("%s\n", files.names[i]);

        unsigned char *data = stbi_load(files.names[i], &width, &height, &nrChannels, STBI_rgb_alpha);

        int newWidth = width + 2*padding;
        int newHeight = height + 2*padding;

        if (data)
        {
            u32 *image = (u32 *)pushArray(&globalPerFrameArena, newWidth*newHeight, u32);

            for(int y = 0; y < height; ++y) {
                for(int x = 0; x < width; ++x) {
                    int xP = x + padding;
                    int yP = y + padding;

                    image[yP*newWidth + xP] = ((u32 *)data)[y*width + x];
                }
            }

            //NOTE: Write the image
            size_t bytesPerPixel = sizeof(u32);
            int stride_in_bytes = bytesPerPixel*newWidth;

            char *name = easy_createString_printf(&globalPerFrameArena, "%s/%s", outputFolderPath, getFileLastPortionWithArena(files.names[i], &globalPerFrameArena));
            printf("%s\n", name);
            int writeResult = stbi_write_png(name, newWidth, newHeight, 4, image, stride_in_bytes);

            assert(writeResult == 1);

            stbi_image_free(data);
        }
    }
    stbi_set_flip_vertically_on_load(0);
}