u32 float4_to_u32_color(float4 c) {
    return (((u32)(c.w*255.0f)) << 24) | (((u32)(c.z*255.0f)) << 16) | (((u32)(c.y*255.0f)) << 8) | (((u32)(c.x*255.0f)) << 0);
}

float4 u32_to_float4_color(u32 c) {
    float a = ((float)((c >> 24) & 0xFF))/255.0f;
    float b = ((float)((c >> 16) & 0xFF))/255.0f;
    float g = ((float)((c >> 8) & 0xFF))/255.0f;
    float r = ((float)((c >> 0) & 0xFF))/255.0f;
    return make_float4(r, g, b, a);
}

float2 getCanvasCoordFromMouse(GameState *gameState) {
     //NOTE: Try draw on the canvas
     const float2 plane = scale_float2(0.5f, make_float2(gameState->camera.fov, gameState->camera.fov*gameState->aspectRatio_y_over_x));
                
     const float x = lerp(-plane.x, plane.x, make_lerpTValue(gameState->mouseP_01.x));
     const float y = lerp(-plane.y, plane.y, make_lerpTValue(gameState->mouseP_01.y));

     float2 worldP = plus_float2(gameState->camera.T.pos.xy, make_float2(x, y));
     float2 result = {};
     result.x = round(((worldP.x * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*gameState->canvasW) - 0.5f);
     result.y = round(((worldP.y * INVERSE_VOXEL_SIZE_IN_METERS) + 0.5f*gameState->canvasH) - 0.5f);

     return result;
}


u32 getCanvasColor(GameState *gameState, int coordX, int coordY) {
    u32 result = 0;
    if(coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW) {
        result = gameState->canvas[coordY*gameState->canvasW + coordX];    
    } 

    return result;
    
}

void setCanvasColor(GameState *gameState, int coordX, int coordY, u32 color, bool useOpacity = true) {
    if(coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW) {
        u32 oldColor = getCanvasColor(gameState, coordX, coordY);
        const float4 c = lerp_float4(u32_to_float4_color(oldColor), u32_to_float4_color(color), gameState->opacity);

        gameState->canvas[coordY*gameState->canvasW + coordX] = float4_to_u32_color(c);
    }
}

bool isValidCanvasRange(GameState *gameState, int coordX, int coordY) {
    return (coordY >= 0 && coordX >= 0 && coordY < gameState->canvasH && coordX < gameState->canvasW);
}