float16 getFoxAnimation(AnimationState *state, Joint *joint) {
    assert(state->animation.animation);
    assert(joint);
    float16 result = float16_identity();

    if(joint->name && joint->hasMesh) {
        if(easyString_stringsMatch_nullTerminated(state->animation.animation->name, "animation.sdf.walk")) {
            float d = 80;
            float t = state->lifeTime*10;
            if(easyString_stringsMatch_nullTerminated(joint->name, "leg0")) {
                float r = degreesToRadians(cos(t));
                result = float16_angle_aroundX(r * d); //NOTE: Back leg
            } else if(easyString_stringsMatch_nullTerminated(joint->name, "leg1")) {
                float r = degreesToRadians(cos(t));
                result = float16_angle_aroundX(r * -d); //NOTE: Back leg
            } else if(easyString_stringsMatch_nullTerminated(joint->name, "leg2")) {
                float r = degreesToRadians(cos(t));
                result = float16_angle_aroundX(r * -d);
            } else if(easyString_stringsMatch_nullTerminated(joint->name, "leg3")) {
                float r = degreesToRadians(cos(t));
                result = float16_angle_aroundX(r * d);
            }
        }
    }

    return result;
}