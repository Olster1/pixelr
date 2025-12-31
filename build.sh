clear
clear
# codesign --force --deep --sign - Spixl.app

# clang++ -std=c++17 -dynamiclib \
#     -install_name @rpath/libimgui.dylib \
#     -arch x86_64 -arch arm64 \
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


ARM="-arch arm64"
x86="-arch x86_64  -mcrc32"
RELEASE="-DNDEBUG -O2"
DEBUG="-g"

g++ -std=c++11 -o ./bin/Spixl \
    $DEBUG \
    $ARM \
    -I ../imgui \
    -I ../imgui/backends \
    -I ./libs/GLAD/include \
    ./platform_backends/platform_layer.cpp \
    -Wno-deprecated-declarations \
    -Wno-writable-strings \
    -Wno-c++11-compat-deprecated-writable-strings \
    -L./bin -limgui \
    -Wl,-rpath,@executable_path \
    -Wl,-rpath,/Library/Frameworks \
    -Wl,-rpath,@executable_path/../Frameworks \
    -I/Library/Frameworks/SDL2.framework/Headers \
    -I/Library/Frameworks/SDL2_image.framework/Headers \
    -F/Library/Frameworks \
    -framework OpenGL \
    -framework SDL2 \
    -framework SDL2_image
