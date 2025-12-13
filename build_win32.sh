clear
clear

# #!/bin/bash

echo "=== Cleaning old object files ==="
# rm -f ./*.o

echo "=== Building ImGui (libimgui.a) ==="

x86_64-w64-mingw32-g++ -std=gnu++17 -O2 -c \
    -I ~/Documents/dev/imgui \
    -I ./windows_sdl_64bit/include \
    -I ./windows_sdl_64bit/include/SDL2 \
    -I ./windows_sdl_image_64bit/include \
    -I ./windows_sdl_image_64bit/include/SDL2 \
    ~/Documents/dev/imgui/imgui.cpp \
    ~/Documents/dev/imgui/imgui_draw.cpp \
    ~/Documents/dev/imgui/imgui_tables.cpp \
    ~/Documents/dev/imgui/imgui_widgets.cpp \
    ~/Documents/dev/imgui/imgui_demo.cpp \
    ~/Documents/dev/imgui/backends/imgui_impl_opengl3.cpp \
    ~/Documents/dev/imgui/backends/imgui_impl_sdl2.cpp \
    -static -static-libgcc -static-libstdc++ 


# mkdir -p ./bin_win32

# echo "=== Archiving libimgui.a ==="
x86_64-w64-mingw32-ar rcs ./bin_win32/libimgui.a *.o

# echo "=== Cleaning object files before main build ==="
rm -f ./*.o

echo "=== Building Spixl.exe ==="

x86_64-w64-mingw32-g++ -O2 -std=gnu++17 -msse4.2 -g \
    -Wno-deprecated-declarations \
    -Wno-write-strings \
    -o ./bin_win32/Spixl.exe \
    ./platform_backends/platform_layer.cpp \
    -I ./libs/GLAD/include \
    -I ~/Documents/dev/imgui \
    -I ~/Documents/dev/imgui/backends \
    -I ./windows_sdl_64bit/include \
    -I ./windows_sdl_64bit/include/SDL2 \
    -I ./windows_sdl_image_64bit/include \
    -I ./windows_sdl_image_64bit/include/SDL2 \
    -L ./windows_sdl_64bit/lib \
    -L ./windows_sdl_image_64bit/lib \
    -L ./bin_win32 \
    -limgui \
    -lopengl32 -lmingw32 -lSDL2main -lSDL2 -lSDL2_image \
    -luser32 -lgdi32 -lwinmm -limm32 -lversion \
    -lole32 -lsetupapi -lshell32 -ladvapi32 -lcomdlg32 -ldxguid \
    -luuid -loleaut32 \
    -static -static-libgcc -static-libstdc++ 


echo "=== Build complete! Output is in ./bin_win32 ==="
