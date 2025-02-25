
void perlinNoiseDrawTests(GameState *gameState, int originWorldX, int originWorldY, int originWorldZ) {
    deleteTexture(&gameState->perlinTestTexture);

    //NOTE: Shows the distribution for 128 block by 128 blocks or 8 x 8 chunks.
    uint32_t perlinBuffer[PERLIN_SIZE*PERLIN_SIZE];
    for(int i = 0; i < PERLIN_SIZE; ++i) {
        for(int j = 0; j < PERLIN_SIZE; ++j) {
            
                int worldX = originWorldX + i;
                int worldY = originWorldY;
                int worldZ = originWorldZ + j;

                // float t = perlin2d(worldX, worldZ, gameState->perlinNoiseValue.x, 16);
                // float t = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, gameState->perlinNoiseValue.x);

                float t0 = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, gameState->perlinNoiseValue.x);
                float t = mapSimplexNoiseTo01(t0);

                // float t = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, gameState->perlinNoiseValue.y);
                // t = mapSimplexNoiseTo01(t);

                // t = t * t0;


                if(t > gameState->perlinNoiseValue.z) {  
                // if(isIronLocation(worldX, worldY, worldZ)) {
                    t = 1;
                } else {
                    t = 0;
                }

                uint8_t color = (255 * t);
                uint32_t v = (0xFF << 24) | (color << 16) | (color << 8) | (color << 0);
                perlinBuffer[(i*PERLIN_SIZE) + j] = v;
            }
    }

    gameState->perlinTestTexture = createGPUTexture(PERLIN_SIZE, PERLIN_SIZE, perlinBuffer);
}