clear
clear

# clang++ -std=c++17 -dynamiclib \
#     -install_name @rpath/libimgui.dylib \
#     -arch arm64 \
#     -I ~/Documents/dev/imgui \
#     ~/Documents/dev/imgui/imgui.cpp \
#     ~/Documents/dev/imgui/imgui_draw.cpp \
#     ~/Documents/dev/imgui/imgui_tables.cpp \
#     ~/Documents/dev/imgui/imgui_widgets.cpp \
#     ~/Documents/dev/imgui/imgui_demo.cpp \
#     ~/Documents/dev/imgui/backends/imgui_impl_opengl3.cpp \
#     ~/Documents/dev/imgui/backends/imgui_impl_sdl2.cpp \
#     -I/Library/Frameworks/SDL2.framework/Headers \
#     -I/Library/Frameworks/SDL2_image.framework/Headers \
#     -F/Library/Frameworks \
#     -framework OpenGL -framework SDL2 -framework SDL2_image \
#     -rpath /Library/Frameworks \
#     -o ./bin/libimgui.dylib


# -O2

g++ -g -std=c++11 -o ./bin/Pixelr \
    -I ../imgui \
    -I ../imgui/backends \
    ./platform_backends/platform_layer.cpp \
    -Wno-deprecated-declarations \
    -Wno-writable-strings \
    -Wno-c++11-compat-deprecated-writable-strings \
    -L./bin -limgui \
    -Wl,-rpath,@executable_path \
    -Wl,-rpath,/Library/Frameworks \
    -I/Library/Frameworks/SDL2.framework/Headers \
    -I/Library/Frameworks/SDL2_image.framework/Headers \
    -F/Library/Frameworks \
    -framework OpenGL \
    -framework SDL2 \
    -framework SDL2_image
