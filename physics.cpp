struct CollisionPoint {
    //NOTE: Data
    float2 point;
    float2 normal;
    float inverseMassNormal; //NOTE: The dividend of the J calculation. This is constant throughout the iterations
    float velocityBias; //NOTE: This is an added velocity to stop penetration along the collision normal
    float seperation; //NOTE: Negative if shapes are colliding. 
    float2 seperationVector; 
    float Pn; //NOTE: Accumulated normal impulse

    //NOTE: Used for the id
    EntityID entityId;
    int x;
    int y;

    int x1;
    int y1;
};

float2 voxelToWorldP(VoxelEntity *e, int x, int y);

struct Arbiter {
    VoxelEntity *a;
    VoxelEntity *b;
    int pointsCount;
    CollisionPoint points[MAX_CONTACT_POINTS_PER_PAIR];

    Arbiter *next;
};

struct PhysicsWorld {
    Arbiter *arbiters;
    Arbiter *arbitersFreeList;

    //TODO: This is what Box2d-lite does to improve it's stability and robustness.
    bool warmStarting;
    bool positionCorrecting; //DONE: this is how agressively the physics tries and resolves penetration
    bool accumulateImpulses;

    //NOTE: Erin Catto's talk at GDC 2006

    //NEXT: Add angular momentum in now
};

float findSeperationForShape(VoxelEntity *e, int startX, int startY, float2 startWorldP, float2 unitVector);
float2 worldPToVoxelP(VoxelEntity *e, float2 worldP);

void prestepAllArbiters(PhysicsWorld *world, float inverseDt) {
    Arbiter *arb = world->arbiters;

    float allowedPenertration = 0.01;//NOTE: Meters

    //NOTE: If positionCorrection correction is on
    // The bias factor is important because overly aggressive corrections (with a high bias factor) can cause instability or jittering in the simulation, while too small of a correction (with a low bias factor) may leave objects slightly penetrated.
	// It strikes a balance between stability and realism, ensuring that objects resolve overlaps without visibly popping or jittering in the simulation.
    float biasFactor = (world->positionCorrecting) ? 0.1f : 0.0f;

    while(arb) {
        VoxelEntity *a = arb->a;
        VoxelEntity *b = arb->b;

        float massCombined = a->inverseMass + b->inverseMass;

        for(int i = 0; i < arb->pointsCount; i++) {
            CollisionPoint *p = &arb->points[i];
            
            //NOTE: The reason Baumgarte Stabilization is wrong is that it adds kinetic energy into the system 
            //      so energy is not conserved. Instead we have a seperate impulse(momentum) that tries and seperates the 
            //      position overlap. This way the velocities aren't contributing to contact normals. 
            //      To do this we get rid of the velocityBias contributing to the velocity constraint solver and have a seperate
            //      position constraint solver that tries and fixes the positions. 

            // PGS_NGS: This is the PGS solver with a NGS position solver instead of Baumgarte Stabilization. NGS was a nice feature in Box2D because it meant that objects that start with overlap do not fly apart, instead they are pushed apart gently.
            //NOTE: This is the Baumgarte Stabilization. This was upgraded to Nonlinear Gauss-Seidel (NGS) on box2d 1.0
            p->velocityBias = -biasFactor * MathMinf((p->seperation + allowedPenertration), 0) * inverseDt;

            float2 r1 = minus_float2(p->point, a->T.pos.xy);
		    float2 r2 = minus_float2(p->point, b->T.pos.xy);

            float rn1 = float2_dot(r1, p->normal);
            float rn2 = float2_dot(r2, p->normal);
            float kNormal = massCombined;
            kNormal += (a->invI * (float2_dot(r1, r1) - rn1 * rn1)) + (b->invI * (float2_dot(r2, r2) - rn2 * rn2));
            if(kNormal == 0) {
                kNormal = 1;
            }
            p->inverseMassNormal = 1.0f / kNormal;

            // p->inverseMassNormal = 1.0f / float2_dot(p->normal, scale_float2(massCombined, p->normal));

            if (world->accumulateImpulses) {
                //NOTE: The accumulated impulse here is a convergence of the 'true' impulse 
                // Apply normal + friction impulse
                float2 Pn = scale_float2(p->Pn, p->normal);

                a->dP.xy = minus_float2(a->dP.xy, scale_float2(a->inverseMass, Pn));
                a->dA -= a->invI * float2_cross(r1, Pn);

                b->dP.xy = plus_float2(b->dP.xy, scale_float2(b->inverseMass, Pn));
                b->dA += b->invI * float2_cross(r2, Pn);
            }
        }

        arb = arb->next;
    }
}

void updateAllArbitersForPositionCorrection(PhysicsWorld *world) {
    const int iterationCount = 4;
    float slop = 0.005f;
    float maxLinearCorrection = 0.2f;
    Arbiter *arb = world->arbiters;
    
    for(int m = 0; m < iterationCount; m++) {
        while(arb) {
            VoxelEntity *a = arb->a;
            VoxelEntity *b = arb->b;
            for(int i = 0; i < arb->pointsCount; i++) {
                CollisionPoint *p = &arb->points[i];

                float2 r1 = minus_float2(p->point, a->T.pos.xy);
    		    float2 r2 = minus_float2(p->point, b->T.pos.xy);

                float2 normal = p->normal;

                // // Current separation
                // s2Vec2 d = s2Add(s2Sub(dcB, dcA), s2Sub(rB, rA));
                // float separation = s2Dot(d, normal) + cp->adjustedSeparation;

                //NOTE: Negative since incoming vector is position distance to seperation
                float2 voxelA = voxelToWorldP(a, p->x, p->y);
                float2 voxelB = voxelToWorldP(b, p->x1, p->y1);
                float separation = float2_magnitude(minus_float2(voxelB, voxelA)) - VOXEL_SIZE_IN_METERS;

                // Compute the effective mass.
                float rnA = float2_cross(r1, normal);
                float rnB = float2_cross(r2, normal);
                float effectiveMass = a->inverseMass + b->inverseMass + a->invI * rnA * rnA + b->invI * rnB * rnB;

                // // Prevent large corrections. Need to maintain a small overlap to avoid overshoot.
                // // This improves stacking stability significantly.
                float C = clamp(-maxLinearCorrection, 0.0f, separation + slop);

                float positionImpulse = effectiveMass > 0.0f ? (-0.2f * C / effectiveMass) : 0.0f;

                float2 impulseVector = scale_float2(positionImpulse, normal);

                a->T.pos.xy = minus_float2(a->T.pos.xy, scale_float2(a->inverseMass, impulseVector));
                a->T.rotation.z -= a->invI * float2_cross(r1, impulseVector);

                b->T.pos.xy = plus_float2(b->T.pos.xy, scale_float2(b->inverseMass, impulseVector));
                b->T.rotation.z += b->invI * float2_cross(r2, impulseVector);
            }
            arb = arb->next;
        }
    }
}

void updateAllArbiters(PhysicsWorld *world) {
    const int iterationCount = 2;
    
    Arbiter *arb = world->arbiters;
    
    for(int m = 0; m < iterationCount; m++) {
        while(arb) {
        VoxelEntity *a = arb->a;
        VoxelEntity *b = arb->b;
        for(int i = 0; i < arb->pointsCount; i++) {
            CollisionPoint *p = &arb->points[i];

            float e = MathMaxf(a->coefficientOfRestitution, b->coefficientOfRestitution);

            const float2 velocityA = a->dP.xy;
            const float2 velocityB = b->dP.xy;

            //NOTE: Get the distance of the point from center of mass 
            float2 r1 = minus_float2(p->point, a->T.pos.xy);
            float2 r2 = minus_float2(p->point, b->T.pos.xy);

            float2 dpPointA = plus_float2(velocityA, float2_cross_scalar(a->dA, r1));
            float2 dpPointB = plus_float2(velocityB, float2_cross_scalar(b->dA, r2));

            // Relative velocity at contact
            float2 relativeAB = minus_float2(dpPointB, dpPointA);
            // float2 relativeAB = minus_float2(velocityB, velocityA);

            if(float2_dot(relativeAB, relativeAB) < (PHYSICS_RESTITUTION_VELOCITY_THRESHOLD_SQR)) 
            {
                e = 0;
            }

            float vn = float2_dot(relativeAB, p->normal);

            //NOTE: This is the J calculation as in Chris Hecker's phsyics tutorials
            float dPn = ((-(1 + e)*vn) + p->velocityBias) * p->inverseMassNormal;

            if(world->accumulateImpulses) {
                assert(p->Pn >= 0);
                float Pn0 = p->Pn; //NOTE: Impulse magnitude previous frame
                p->Pn = MathMaxf(Pn0 + dPn, 0.0f);
                dPn = p->Pn - Pn0;
                //NOTE: This allows impulses to go towards the collision points
            } else {
                //NOTE: Moving towards the objects so remove velocities component that's contributing to them colliding more
                dPn = MathMaxf(dPn, 0.0f);
            }

            float2 Pn = scale_float2(dPn, p->normal);

            a->dP.xy = minus_float2(arb->a->dP.xy, scale_float2(a->inverseMass, Pn));
            b->dP.xy = plus_float2(arb->b->dP.xy, scale_float2(b->inverseMass, Pn));

            a->dA -= a->invI * float2_cross(r1, Pn);
            b->dA += b->invI * float2_cross(r2, Pn);
        }
        arb = arb->next;
    }
    }

}

void wakeUpEntity(VoxelEntity *e) {
    e->asleep = false;
    e->sleepTimer = 0;
} 

void mergePointsToArbiter(PhysicsWorld *world, CollisionPoint *points, int pointCount, VoxelEntity *a, VoxelEntity *b) {
    if(a > b) {
        VoxelEntity *temp = b;
        b = a;
        a = temp;
    }

    Arbiter *arb = world->arbiters;
    Arbiter **arbPrev = &world->arbiters;
    while(arb) {
        if(arb->a == a && arb->b == b) {
            if(pointCount == 0) {
                //NOTE: Remove from the list
                *arbPrev = arb->next;
                arb->next = world->arbitersFreeList;
                world->arbitersFreeList = arb;
            }
            break;
        } else {
            arbPrev = &arb->next;
            arb = arb->next;
        }
    }

    if(pointCount == 0) {
        
    } else {
        
        if(!arb) {
            if(world->arbitersFreeList) {
                arb = world->arbitersFreeList;
                world->arbitersFreeList = arb->next;
            } else {
                arb = pushStruct(&globalLongTermArena, Arbiter);
            }
            arb->next = world->arbiters;
            world->arbiters = arb;

            arb->pointsCount = 0;

            arb->a = a;
            arb->b = b;
        }

        assert(arb);
        assert(arb->a);
        assert(arb->b);

        
        for(int j = 0; j < pointCount; j++) {
            CollisionPoint *newP = &points[j];
            bool pointFound = false;
            for(int i = 0; i < arb->pointsCount && !pointFound; i++) {
                CollisionPoint *oldP = &arb->points[i];
            
                if(oldP->x == newP->x && oldP->y == newP->y &&  areEntityIdsEqual(oldP->entityId, newP->entityId)) {
                    //NOTE: Point already exists, so keep the accumulated impluses going
                    if (world->warmStarting) {
                        newP->Pn = oldP->Pn;
                        // c->Pt = cOld->Pt;
                        // c->Pnb = cOld->Pnb;
                    }
                    pointFound = true;
                }
            }
        }

        assert(pointCount < arrayCount(arb->points));
        arb->pointsCount = pointCount;

        //NOTE: Copy the new points array to the old points array
        for(int i = 0; i < pointCount; i++) {
            arb->points[i] = points[i];
        }
    }
}