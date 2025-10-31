void updateCamera(GameState *gameState) {
    if(!gameState->camera.followingPlayer) {
        
        float speed = 10;

        if(gameState->keys.keys[KEY_SHIFT]) {
            speed = 60;
        }
        float rotSpeed = 13.0f;

        float2 mouseDelta = minus_float2(gameState->mouseP_screenSpace, gameState->lastMouseP);

        // gameState->camera.T.rotation.y += gameState->dt*rotSpeed*-mouseDelta.x;
        // gameState->camera.T.rotation.x += gameState->dt*rotSpeed*-mouseDelta.y;

        float16 rot = eulerAnglesToTransform(gameState->camera.T.rotation.y, gameState->camera.T.rotation.x, gameState->camera.T.rotation.z);

        float3 zAxis = make_float3(rot.E_[2][0], rot.E_[2][1], rot.E_[2][2]);
        float3 yAxis = make_float3(rot.E_[1][0], rot.E_[1][1], rot.E_[1][2]);
        float3 xAxis = make_float3(rot.E_[0][0], rot.E_[0][1], rot.E_[0][2]);

        // if(gameState->keys.keys[KEY_LEFT]) {
        //     gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(-gameState->dt*speed, xAxis));
        // }
        // if(gameState->keys.keys[KEY_RIGHT]) {
        //     gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(gameState->dt*speed, xAxis));
        // }
        // if(gameState->keys.keys[KEY_DOWN]) {
        //     gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(-gameState->dt*speed, zAxis));
        // }
        // if(gameState->keys.keys[KEY_UP]) {
        //     gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(gameState->dt*speed, zAxis));
        // }
        // if(gameState->keys.keys[KEY_Q]) {
        //     gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(-gameState->dt*speed, yAxis));
        // }
        // if(gameState->keys.keys[KEY_E]) {
        //     gameState->camera.T.pos = plus_float3(gameState->camera.T.pos, scale_float3(gameState->dt*speed, yAxis));
        // }

    }
}