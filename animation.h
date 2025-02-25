
struct ModelBuffer {
    uint32_t handle;
    uint32_t instanceBufferhandle;
    uint32_t tboHandle; //NOTE: For Skinning Matrix
    uint32_t textureHandle; //NOTE: For Skinning Matrix

    int indexCount;
};

struct Joint {
	char *name;
	int parentIndex;
	bool hasMesh;
	SQT T; //NOTE: Transforms from child space to this space
};

struct KeyFrame {
	float time;
	float4 transform;
};

enum BoneAnimationType {
	BONE_ANIMATION_ROTATION,
	BONE_ANIMATION_TRANSLATION,
	BONE_ANIMATION_SCALE,
};

struct BoneAnimation {
	int keyFrameCount;
	KeyFrame *keyFrames;
	
	int boneIndex;
	BoneAnimationType type;
};

struct Animation3d {
	char *name; //NOTE: Lives on Heap

	float maxTime;

	int boneCount;
	BoneAnimation *boneAnimations;
};

struct SkeletalModel {
    bool valid;
    ModelBuffer modelBuffer;

	int inverseBindMatrixCount;
	float16 *inverseBindMatrices;

	int jointCount;
	Joint *joints;

	int animationCount;
	Animation3d *animations;

	int parentJointIndex;
};

struct AnimationStateNode {
	Animation3d *animation;
	Animation3d *next;
};

struct AnimationState {
	AnimationStateNode animation;
	float timeAt;
	float lifeTime; //NOTE: Total time the animation has been running
};