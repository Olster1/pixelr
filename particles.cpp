struct Particle {
    TransformX T;
    float3 dP;
    float lifeTime;
    float4 uvCoords;
    bool alive;
};

struct ParticlerId {
    void *id; 
};

ParticlerId makeParticlerId(void *id) {
    ParticlerId result = {};
    result.id = id;
    return result;
}

bool particlerIdsMatch(ParticlerId id, ParticlerId id1) {
    return id.id == id1.id;
}

struct Particler {
    ParticlerId id;
    Particle particles[255];
    int count;
    int indexAt;

    Rect3f spawnBox;
    float4 uvCoords;

    float lastTimeCreation;
    float lifespan;
    float lifeAt;
    float tAt;
    float spawnRate; //NOTE: seconds per particle
};

Particler initParticler(float lifespan, float spawnRate, Rect3f spawnBox, float4 uvCoords, ParticlerId id) {
    Particler p = {};

    p.count = 0;
    p.lifespan = lifespan;
    p.spawnRate = 1.0f / spawnRate;
    p.lastTimeCreation = 0;
    p.tAt = 0;
    p.lifeAt = 0;
    p.indexAt = 0;
    p.spawnBox = spawnBox;
    p.uvCoords = uvCoords;
    p.id = id;

    return p;   
}

bool updateParticler(Renderer *renderer, Particler *particler, float3 cameraPos, float dt) {

    particler->tAt += dt;
    particler->lifeAt += dt;

    bool isDead = (particler->lifeAt >= particler->lifespan);

    float diff = (particler->tAt - particler->lastTimeCreation);
    if(!isDead && diff >= particler->spawnRate) {
        int numberOfParticles = diff / particler->spawnRate;

        assert(numberOfParticles > 0);

        for(int i = 0; i < numberOfParticles; ++i) {
            Particle *p = 0;

            if(particler->count < arrayCount(particler->particles)) {
                p = &particler->particles[particler->count++];
                
            } else {
                //NOTE: Already full so use the ring buffer index
                assert(particler->indexAt < arrayCount(particler->particles));
                p = &particler->particles[particler->indexAt++];

                if(particler->indexAt >= arrayCount(particler->particles)) {
                    particler->indexAt = 0;
                }
            }

            assert(p);

            float x = lerp(particler->spawnBox.minX, particler->spawnBox.maxX, make_lerpTValue((float)rand() / RAND_MAX));
            float y = lerp(particler->spawnBox.minY, particler->spawnBox.maxY, make_lerpTValue((float)rand() / RAND_MAX));
            float z = lerp(particler->spawnBox.minZ, particler->spawnBox.maxZ, make_lerpTValue((float)rand() / RAND_MAX));
            p->T.pos = make_float3(x, y, z);
            p->T.scale = make_float3(0.1f, 0.1f, 0.1f);

            float dpMargin = 0.8f;

            p->dP = make_float3(randomBetween(-dpMargin, dpMargin), 2, randomBetween(-dpMargin, dpMargin)); //NOTE: Straight up
            p->lifeTime = 1; //NOTE: Live for 2 seconds
            p->alive = true;

            float step = 0.05f;
            float minX = randomBetween(particler->uvCoords.x, particler->uvCoords.y - step);
            float minY = randomBetween(particler->uvCoords.z, particler->uvCoords.w - step);
            p->uvCoords = make_float4(minX, minX + step, minY, minY + step);
            
        }

        particler->lastTimeCreation = particler->tAt;
    }
    
    int deadCount = 0;
    for(int i = 0; i < particler->count; ++i) {
        Particle *p = &particler->particles[i];

        if(p->alive) {
            float3 accelForFrame = scale_float3(dt, make_float3(0, -10, 0));

            //NOTE: Integrate velocity
            p->dP = plus_float3(p->dP, accelForFrame); //NOTE: Already * by dt 

            //NOTE: Apply drag
            // p->dP = scale_float3(0.95f, p->dP);

            //NOTE: Get the movement vector for this frame
            p->T.pos = plus_float3(p->T.pos, scale_float3(dt, p->dP));

            //NOTE: make them facing the player
            float3 diffVec = minus_float3(cameraPos, p->T.pos);
            float d = radiansToDegrees((atan2(diffVec.z, diffVec.x) + 0.5f*PI32));
            float3 rotation = make_float3(0, d, 0);

            //NOTE: Draw the particle
            pushAtlasQuad_(renderer, p->T.pos, p->T.scale, rotation, p->uvCoords, make_float4(1, 1, 1, 1), false);

            //NOTE Update whether the particle should still be alive
            p->lifeTime -= dt;

            if(p->lifeTime <= 0) {
                p->alive = false;
            }
        } else {
            deadCount++;
        }
    }

    bool shouldRemove = false;

    if(isDead && deadCount == particler->count){
        //NOTE: Particle system should be removed
        shouldRemove = true;
    }

    return shouldRemove;

}
