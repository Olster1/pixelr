
void initBackendRenderer() {
    //TODO: Enable the back face culling
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    // glFrontFace(GL_CCW);  

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
}