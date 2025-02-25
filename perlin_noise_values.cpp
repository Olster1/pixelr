float getTerrainHeight(int worldX, int worldZ) {
    float a = SimplexNoise_fractal_2d(16, worldX, worldZ, 0.00522);
    a = mapSimplexNoiseTo01(a);

    float b = SimplexNoise_fractal_2d(16, worldX, worldZ, 0.007143);
    b = mapSimplexNoiseTo01(b);

    float c = SimplexNoise_fractal_2d(16, 0.1f*worldX, 0.1f*worldZ, 0.000043);
    c = mapSimplexNoiseTo01(c);

    float maxTerrainValue = lerp(100, 200, make_lerpTValue(c));

    float terrainAmplitude = lerp(10, maxTerrainValue, make_lerpTValue(b));
    float terrainHeight = a*terrainAmplitude; 

    return terrainHeight;
}


bool isTreeLocation(int worldX, int worldZ) {
    float t = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(16, worldX, worldZ, 0.8f));
    return t < 0.25f;
}

bool isBushLocation(int worldX, int worldZ) {
    float t = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(16, worldX, worldZ, 0.7f));
    return t < 0.4f;
}

bool isBigBush(int worldX, int worldZ) {
    float t = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(16, worldX, worldZ, 0.9f));
    return t < 0.5f;
}

bool isIronLocation(int worldX, int worldY, int worldZ) {
    //NOTE: Bigger frequency noise overlay
    float t0 = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, 0.007143);
    t0 = mapSimplexNoiseTo01(t0);
    
    //NOTE: Smaller frequency for individual iron blocks 
    float t = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, 0.128572);
    t = mapSimplexNoiseTo01(t);

    t = t * t0;
    return t < 0.135714;
}

bool isCoalLocation(int worldX, int worldY, int worldZ) {
    int coalOffset = 64;
    //NOTE: Bigger frequency noise overlay
    float t0 = SimplexNoise_fractal_3d(16, worldX + coalOffset, worldY + coalOffset, worldZ + coalOffset, 0.007143);
    t0 = mapSimplexNoiseTo01(t0);
    
    //NOTE: Smaller frequency for individual iron blocks 
    float t = SimplexNoise_fractal_3d(16, worldX, worldY, worldZ, 0.128572);
    t = mapSimplexNoiseTo01(t);

    t = t * t0;
    return t < 0.135714;
}