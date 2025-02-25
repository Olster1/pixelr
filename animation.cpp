// struct Joint {
// 	int *childIndexes;
// };

// struct KeyFrame {
// 	float time;
// 	float4 transform;
// };

// enum BoneAnimationType {
// 	BONE_ANIMATION_ROTATION,
// 	BONE_ANIMATION_TRANSLATION,
// 	BONE_ANIMATION_SCALE,
// };

// struct BoneAnimation {
// 	int keyFrameCount;
// 	KeyFrame *keyFrames;
	
// 	int boneIndex;
// 	BoneAnimationType type;
// };

// struct Animation3d {
// 	char *name; //NOTE: Lives on Heap

// 	int boneCount;
// 	BoneAnimation *boneAnimations;
// };

// struct SkeletalModel {
//     bool valid;
//     ModelBuffer modelBuffer;

// 	int inverseBindMatrixCount;
// 	float16 *inverseBindMatrices;

// 	int jointCount;
// 	Joint *joints;

// 	int animationCount;
// 	Animation3d *animations;

// 	int parentJointIndex;
	
// };


void updateAnimationState(GameState *gameState, AnimationState *animationState) {
	animationState->timeAt += gameState->dt;
	animationState->lifeTime += gameState->dt;

	Animation3d *a = animationState->animation.animation;

	while(animationState->timeAt > a->maxTime) {
		if(animationState->animation.next) {
			animationState->animation.animation = animationState->animation.next;
			animationState->animation.next = 0;
			animationState->timeAt = 0;	
		} else {
			animationState->timeAt -= a->maxTime;

			if(a->maxTime == 0.0) {
				animationState->timeAt = 0;
			}
		}
	}
}

// void recursivelyWalkJointChildren(Joint *j, float16 *skinningMatrix, SkeletalModel *model, float16 *perJointTransforms) {
// 	assert(j);
// 	if(j) {
// 		for(int i = 0; i < getArrayLength(j->childIndexes); ++i) {
// 			int boneIndex = j->childIndexes[i];
// 			float16 thisJointT = perJointTransforms[boneIndex];
// 			skinningMatrix[boneIndex] = float16_multiply(skinningMatrix[boneIndex], thisJointT);
// 			Joint *childJoint = &model->joints[boneIndex];
// 			recursivelyWalkJointChildren(childJoint, skinningMatrix, model, perJointTransforms);
// 		}
// 	}
// }

void buildSkinningMatrix(GameState *gameState, SkeletalModel *model, AnimationState *animationState, EntityType entityType) {
	updateAnimationState(gameState, animationState);

	float16 *skinningMatrix = (float16 *)pushArray(&globalPerFrameArena, model->jointCount, float16);
	SQT *perJointTransform = (SQT *)pushArray(&globalPerFrameArena, model->jointCount, SQT);
	float16 *perJointTransforms = (float16 *)pushArray(&globalPerFrameArena, model->jointCount, float16);

	for(int i = 0; i < model->jointCount; ++i) {
		perJointTransform[i] = model->joints[i].T;
	}

	Animation3d *animationOn = animationState->animation.animation;

	assert(animationOn);

	for(int i = 0; i < animationOn->boneCount; ++i) {
		BoneAnimation *bone = &animationOn->boneAnimations[i];

		SQT *T = &perJointTransform[bone->boneIndex];

		float4 a;
		float4 b;
		float t = 0;

		for(int k = 0; k < bone->keyFrameCount - 1; ++k) {
			KeyFrame *frame = &bone->keyFrames[k];
			KeyFrame *frame2 = &bone->keyFrames[k + 1];

			if(animationState->timeAt >= frame->time && animationState->timeAt <= frame2->time) {
				assert(frame->time <= frame2->time);

				t = (animationState->timeAt - frame->time) / (frame2->time - frame->time);
				a = frame->transform;
				b = frame2->transform;
				break;
			}
		}

		if(bone->keyFrameCount == 1) {
			a = bone->keyFrames[0].transform;
			b = bone->keyFrames[0].transform;
		}

		assert(t >= 0 && t <= 1.0f);

		if(bone->type == BONE_ANIMATION_ROTATION) {
			a = slerp(a, b, t);
			// a = lerp_float4(a, b, t);
			if(entityType == ENTITY_NONE) {
				T->rotation = inverseQuaternion(a);
			}
			
		} else if(bone->type == BONE_ANIMATION_SCALE) {
			a = lerp_float4(a, b, t);
			T->scale = a.xyz;
		} else if(bone->type == BONE_ANIMATION_TRANSLATION) {
			a = lerp_float4(a, b, t);
			T->translate = a.xyz;
		}
	}

	for(int i = 0; i < model->jointCount; ++i) {
		SQT sqt = perJointTransform[i];
		perJointTransforms[i] = sqt_to_float16(sqt.rotation, sqt.scale, sqt.translate);

		if(entityType == ENTITY_FOX) {
			perJointTransforms[i] = float16_multiply(getFoxAnimation(animationState, &model->joints[i]), perJointTransforms[i]);
		}
		
	}

	assert(model->parentJointIndex >= 0 && model->parentJointIndex < model->jointCount);

	for(int i = 0; i < model->jointCount; ++i) {
		Joint *joint = &model->joints[i];
		int parentIndex = i;

		skinningMatrix[i] = model->inverseBindMatrices[i];

		while(parentIndex >= 0) {
			// assert(parentIndex <= i);
			skinningMatrix[i] = float16_multiply(perJointTransforms[parentIndex], skinningMatrix[i]);

			Joint *parentJoint = &model->joints[parentIndex];
			parentIndex = parentJoint->parentIndex;
		}

		assert(skinningMatrix[i].E[15] == 1);
	}

	assert(model->jointCount == model->inverseBindMatrixCount);

	updateSkinningTexture(&model->modelBuffer, skinningMatrix, model->jointCount);
	
}