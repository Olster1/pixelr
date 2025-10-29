
Renderer *initRenderer(Texture atlasTexture) {
    Renderer *renderer = (Renderer *)malloc(sizeof(Renderer));
    
    renderer->atlasQuadCount = 0;
    renderer->glyphCount = 0;
    renderer->checkerQuadCount = 0;
    renderer->atlasTexture = atlasTexture.handle;

    renderer->quadTextureShader = loadShader(quadVertexShader, quadTextureFragShader);
    renderer->fontTextureShader = loadShader(quadVertexShader, fontTextureFragShader);
    renderer->lineShader = loadShader(lineVertexShader, lineFragShader);

    renderer->quadShader = loadShader(quadVertexShader, quadFragShader);
    renderer->checkQuadShader = loadShader(quadVertexShader, checkQuadFragShader);
    
    renderer->pixelSelectionShader = loadShader(quadVertexShader, selectionFragShader);
    
    renderer->quadModel = generateVertexBuffer(global_quadData, 4, global_quadIndices, 6, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX);
    renderer->lineModel = generateVertexBuffer(global_lineModelData, 2, global_lineIndicies, 2, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX);

    return renderer;
}