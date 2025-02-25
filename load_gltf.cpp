#define CGLTF_IMPLEMENTATION
#include "./libs/cgltf.h"

/*
structs of interest: 

typedef struct cgltf_mesh {
	char* name;
	cgltf_primitive* primitives;
	cgltf_size primitives_count;
	cgltf_float* weights;
	cgltf_size weights_count;
	char** target_names;
	cgltf_size target_names_count;
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_mesh;


typedef enum cgltf_primitive_type
{
	cgltf_primitive_type_invalid,
	cgltf_primitive_type_points,
	cgltf_primitive_type_lines,
	cgltf_primitive_type_line_loop,
	cgltf_primitive_type_line_strip,
	cgltf_primitive_type_triangles,
	cgltf_primitive_type_triangle_strip,
	cgltf_primitive_type_triangle_fan,
	cgltf_primitive_type_max_enum
} cgltf_primitive_type;


typedef struct cgltf_primitive {
	cgltf_primitive_type type;
	cgltf_accessor* indices;  //IMPORTANT: The indicies array
	cgltf_material* material;
	cgltf_attribute* attributes;  //IMPORTANT: The attribute arrays
	cgltf_size attributes_count;
	cgltf_morph_target* targets;
	cgltf_size targets_count;
	cgltf_extras extras;
	cgltf_bool has_draco_mesh_compression;
	cgltf_draco_mesh_compression draco_mesh_compression;
	cgltf_material_mapping* mappings;
	cgltf_size mappings_count;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_primitive;

typedef enum cgltf_attribute_type
{
	cgltf_attribute_type_invalid,
	cgltf_attribute_type_position,
	cgltf_attribute_type_normal,
	cgltf_attribute_type_tangent,
	cgltf_attribute_type_texcoord,
	cgltf_attribute_type_color,
	cgltf_attribute_type_joints, //IMPORTANT: For Skeletal Animation
	cgltf_attribute_type_weights, //IMPORTANT: For Skeletal Animation
	cgltf_attribute_type_custom,
	cgltf_attribute_type_max_enum
} cgltf_attribute_type;

typedef struct cgltf_attribute  //IMPORTANT: Points to the buffer view
{
	char* name;
	cgltf_attribute_type type;
	cgltf_int index;
	cgltf_accessor* data;
} cgltf_attribute;

typedef enum cgltf_type
{
	cgltf_type_invalid,
	cgltf_type_scalar,
	cgltf_type_vec2,
	cgltf_type_vec3,
	cgltf_type_vec4,
	cgltf_type_mat2,
	cgltf_type_mat3,
	cgltf_type_mat4,
	cgltf_type_max_enum
} cgltf_type;

typedef enum cgltf_component_type
{
	cgltf_component_type_invalid,
	cgltf_component_type_r_8,  BYTE 
	cgltf_component_type_r_8u, UNSIGNED_BYTE 
	cgltf_component_type_r_16,  SHORT 
	cgltf_component_type_r_16u,  UNSIGNED_SHORT 
	cgltf_component_type_r_32u,  UNSIGNED_INT 
	cgltf_component_type_r_32f, FLOAT 
    cgltf_component_type_max_enum
} cgltf_component_type;

typedef struct cgltf_accessor_sparse
{
	cgltf_size count;
	cgltf_buffer_view* indices_buffer_view;
	cgltf_size indices_byte_offset;
	cgltf_component_type indices_component_type;
	cgltf_buffer_view* values_buffer_view;
	cgltf_size values_byte_offset;
} cgltf_accessor_sparse;


typedef struct cgltf_accessor
{
	char* name;
	cgltf_component_type component_type;
	cgltf_bool normalized;
	cgltf_type type;
	cgltf_size offset;
	cgltf_size count;
	cgltf_size stride;
	cgltf_buffer_view* buffer_view;
	cgltf_bool has_min;
	cgltf_float min[16];
	cgltf_bool has_max;
	cgltf_float max[16];
	cgltf_bool is_sparse; //IMPORTANT: This is if we should replace some of the data with this ones
	cgltf_accessor_sparse sparse; //IMPORTANT: This is the data to replace with
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_accessor;

typedef enum cgltf_buffer_view_type
{
	cgltf_buffer_view_type_invalid,
	cgltf_buffer_view_type_indices,
	cgltf_buffer_view_type_vertices,
	cgltf_buffer_view_type_max_enum
} cgltf_buffer_view_type;

typedef struct cgltf_buffer_view
{
	char *name;
	cgltf_buffer* buffer;
	cgltf_size offset;
	cgltf_size size;
	cgltf_size stride;  0 == automatically determined by accessor 
	cgltf_buffer_view_type type;
	void* data; // overrides buffer->data if present, filled by extensions 
	cgltf_bool has_meshopt_compression;
	cgltf_meshopt_compression meshopt_compression;
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_buffer_view;

typedef struct cgltf_buffer
{
	char* name;
	cgltf_size size;
	char* uri;
	void* data; //: IMPORTANT: loaded by cgltf_load_buffers
	cgltf_data_free_method data_free_method;
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_buffer;

*/



void copyBufferToMemory(void *bufferToCopyTo, uint8_t *ptr, cgltf_buffer_view *buffer_view, cgltf_accessor *accessor, bool isShort, int floatCount) {
	uint8_t *from = ptr + buffer_view->offset + accessor->offset;
	uint8_t *to = (uint8_t *)bufferToCopyTo;

	size_t sizeOfElm = (isShort) ? sizeof(u16) : sizeof(float);

	int buffer_view_stride = buffer_view->stride;
	if(buffer_view_stride == 0) {
		buffer_view_stride = accessor->stride;
		if(buffer_view_stride == 0) {
			buffer_view_stride = sizeOfElm * floatCount;
		}
	}
 
	//NOTE: This is to account if the data is interleaved, we extract out into it's own SOA instead of AOS
	assert(sizeOfElm*floatCount <= buffer_view_stride);
	for(int i = 0; i < buffer_view->size; i += buffer_view_stride) {
		memcpy(to, from + i, sizeOfElm*floatCount);

		to += sizeOfElm*floatCount;
	}
}

void copyDataFromAccessor(void *bufferToCopyTo, uint8_t *ptr, cgltf_buffer_view *buffer_view, cgltf_accessor *accessor, bool isShort, int floatCount) {
	size_t sizeOfElm = (isShort) ? sizeof(u16) : sizeof(float);
	copyBufferToMemory(bufferToCopyTo, ptr, buffer_view, accessor, isShort, floatCount);
	
	//NOTE: This is if an attribute is sparse 
	if(accessor->is_sparse) {
		
		cgltf_accessor_sparse sparse = accessor->sparse;

		if(sparse.count > 0) {

			//NOTE: Get the indicies out
			cgltf_buffer_view* indices_buffer_view = sparse.indices_buffer_view;
			assert(indices_buffer_view);
			
			cgltf_buffer* indicies_buffer = indices_buffer_view->buffer;
			uint8_t *indicies_ptr = (uint8_t *)((uint8_t *)indicies_buffer->data) + indices_buffer_view->offset;
			assert(cgltf_component_type_r_16u == sparse.indices_component_type);

			//NOTE: Now get the data out 
			cgltf_buffer_view* values_buffer_view = sparse.values_buffer_view;
			
			cgltf_buffer* values_buffer = values_buffer_view->buffer;
			uint8_t *values_ptr = (uint8_t *)((uint8_t *)values_buffer->data) + values_buffer_view->offset;

			size_t indices_stride = sizeof(unsigned short);
			size_t value_stride = sizeOfElm*floatCount;
			
			for(int i = 0; i < sparse.count; ++i) {
				int index = (int)(*(indicies_ptr + i*indices_stride));

				void *to = ((uint8_t *)bufferToCopyTo) + sizeOfElm*floatCount*index;
				void *from = values_ptr + i*value_stride;

				memcpy(to, from, floatCount*sizeOfElm);
			}
		}
	}
}

SkeletalModel loadGLTF(char *fileName) {
    SkeletalModel model = {};
    model.valid = false;

    cgltf_options options = {};
    cgltf_data* data = NULL;

	int vertexCount = 0;
	VertexWithJoints *vertexes = 0;

	int indexCount = 0;
	unsigned int *indiciesToSend = 0;

    //NOTE: Parse the file
    cgltf_result result = cgltf_parse_file(&options, fileName, &data);
    if (result == cgltf_result_success) {
        //NOTE: Load the buffers - .bin files or imbeded in the file
        result = cgltf_load_buffers(&options, data, fileName);

        if (result == cgltf_result_success) {
            result = cgltf_validate(data);

            if (result == cgltf_result_success) {

                //NOTE: Each file will have a list of meshes 
                //      We're interested in the primitives of this (i.e. triangles, triangle strips, lines) AND the indices
                //  
				float *poss = 0;
				float *uvs = 0;
				float *normals = 0;
				u16 *joints = 0;
				float *weights = 0;
				int *meshIndexes = 0;

				int normalCount = 0;
				int uvCount = 0;
				int jointCount = 0;
				int weightsCount = 0;

				size_t weightsOldSize = 0;
				size_t jointsOldSize = 0;
				size_t texOldSize = 0;
				size_t normalOldSize = 0;
				size_t possOldSize = 0;
				size_t indicesOldSize = 0;
				size_t meshIndexOldSize = 0;
				
				if(data->animations_count > 0) {
					model.animationCount = data->animations_count;
					model.animations = (Animation3d *)easyPlatform_allocateMemory(sizeof(Animation3d)*data->animations_count, EASY_PLATFORM_MEMORY_ZERO);

					cgltf_skin *skin = 0;
					
					// assert(data->skins_count == 1);
					if(data->skins_count == 1) {
						skin = &data->skins[0];
					}
		
					for(int i = 0; i < data->animations_count; ++i) {
						cgltf_animation* animation = &data->animations[i];
						Animation3d *animation3d = &model.animations[i];
						
						animation3d->name = easyString_copyToHeap(animation->name);
						animation3d->boneCount = animation->channels_count;

						animation3d->boneAnimations = (BoneAnimation *)easyPlatform_allocateMemory(sizeof(BoneAnimation)*animation3d->boneCount, EASY_PLATFORM_MEMORY_ZERO);

						for(int k = 0; k < animation->channels_count; ++k) {
							cgltf_animation_channel *channel = &animation->channels[k];
							BoneAnimation *boneAnimation = &animation3d->boneAnimations[k];
							if(channel->target_path == cgltf_animation_path_type_rotation) {
								boneAnimation->type = BONE_ANIMATION_ROTATION;
							} else if(channel->target_path == cgltf_animation_path_type_translation) {
								boneAnimation->type = BONE_ANIMATION_TRANSLATION;
							} else if(channel->target_path == cgltf_animation_path_type_scale) {
								boneAnimation->type = BONE_ANIMATION_SCALE;
							} else {
								//NOTE: Don't support morph target animations
								assert(false);
							}

							int boneIndex = -1;
							// NOTE: Find the index of the bone
							if(skin) {
								for(int j = 0; j < skin->joints_count; ++j) {
									cgltf_node *joint = skin->joints[j];

									if(joint == channel->target_node) {
										boneIndex = j;
										break;
									}
								}
								assert(boneIndex >= 0);
							} else {
								//NOTE: Look through the meshes
								for(int j = 0; j < data->nodes_count; ++j) {
									cgltf_node *node = &data->nodes[j];

									if(node == channel->target_node) {
										boneIndex = j;
										break;
									}
								}
								assert(boneIndex >= 0);
							}

							

							boneAnimation->boneIndex = boneIndex;

							cgltf_animation_sampler *sampler = channel->sampler;
							assert(sampler);
							//NOTE: Input is the seconds values of the keyframes
							cgltf_accessor *inputAccessor = sampler->input;
							float *inputBuffer = 0;
							{
								cgltf_buffer_view *buffer_view = inputAccessor->buffer_view;
								cgltf_buffer* buffer = buffer_view->buffer;
								uint8_t *ptr = (uint8_t *)buffer->data;
								int floatCount = 1;
								inputBuffer = (float *)easyPlatform_allocateMemory(buffer_view->size);

								animation3d->maxTime = inputAccessor->max[0];

								assert(inputAccessor->component_type == cgltf_component_type_r_32f);
								assert(inputAccessor->type == cgltf_type_scalar);

								copyDataFromAccessor(inputBuffer, ptr, buffer_view, inputAccessor, false, floatCount);
							}

							float *outputBuffer;

							//NOTE: Output is the transform values for the keyframes
							cgltf_accessor *outputAccessor = sampler->output;
							{
								cgltf_buffer_view *buffer_view = outputAccessor->buffer_view;
								cgltf_buffer* buffer = buffer_view->buffer;
								uint8_t *ptr = (uint8_t *)buffer->data;
								int floatCount = 4;
								outputBuffer = (float *)easyPlatform_allocateMemory(buffer_view->size);

								if(channel->target_path == cgltf_animation_path_type_rotation) {
									assert(outputAccessor->component_type == cgltf_component_type_r_32f);
									assert(outputAccessor->type == cgltf_type_vec4);
									floatCount = 4;
								} else if(channel->target_path == cgltf_animation_path_type_translation || channel->target_path == cgltf_animation_path_type_scale) {
									assert(outputAccessor->component_type == cgltf_component_type_r_32f);
									assert(outputAccessor->type == cgltf_type_vec3);
									floatCount = 3;
								} else {
									//NOTE: Don't support morph target animations
									assert(false);
								}
								assert(outputBuffer);
								copyDataFromAccessor(outputBuffer, ptr, buffer_view, outputAccessor, false, floatCount);

								assert(outputAccessor->count == inputAccessor->count);

								boneAnimation->keyFrameCount = outputAccessor->count;
								boneAnimation->keyFrames = (KeyFrame *)easyPlatform_allocateMemory(sizeof(KeyFrame)*outputAccessor->count);
								for(int l = 0; l < outputAccessor->count; l++) {
									KeyFrame *frame = &boneAnimation->keyFrames[l];
									frame->time = inputBuffer[l];

									assert(floatCount <= 4);
									for(int m = 0; m < floatCount; m++) {
										frame->transform.E[m] = outputBuffer[floatCount*l + m];
									}

									// printf("type: %d\n", boneAnimation->type);
									// printf("bone index: %d\n", boneAnimation->boneIndex);
									// printf("time: %f\n", frame->time);
									// printf("transform: %f %f %f %f\n\n", frame->transform.E[0], frame->transform.E[1], frame->transform.E[2], frame->transform.E[3]);
									
								}
							}

							free(outputBuffer);
							outputBuffer = 0;
							free(inputBuffer);
							inputBuffer = 0;
						}
					}
				}
				
				assert(data->skins_count < 2);
				{
					cgltf_node **joints = 0;
					cgltf_node *nodes = 0;
					if(data->skins_count > 0) {
						cgltf_skin skin = data->skins[0];
						cgltf_accessor *inverse_bind_matrices = skin.inverse_bind_matrices;
						assert(inverse_bind_matrices->count == skin.joints_count);
						cgltf_buffer_view *buffer_view = inverse_bind_matrices->buffer_view;
						cgltf_buffer* buffer = buffer_view->buffer;
						uint8_t *ptr = (uint8_t *)buffer->data;

						assert(inverse_bind_matrices->component_type == cgltf_component_type_r_32f);
						assert(inverse_bind_matrices->type == cgltf_type_mat4);
						
						model.inverseBindMatrices = (float16 *)easyPlatform_allocateMemory(sizeof(float16)*inverse_bind_matrices->count, EASY_PLATFORM_MEMORY_ZERO);
						copyBufferToMemory(model.inverseBindMatrices, ptr, buffer_view, inverse_bind_matrices, false, 16);
						model.inverseBindMatrixCount = inverse_bind_matrices->count;

						assert(skin.joints_count < MAX_BONES_PER_MODEL);

						model.joints = (Joint *)easyPlatform_allocateMemory(sizeof(Joint)*skin.joints_count, EASY_PLATFORM_MEMORY_ZERO);
						model.jointCount = skin.joints_count;

						joints = skin.joints;

						assert(model.jointCount == model.inverseBindMatrixCount);
					} else {
						model.joints = (Joint *)easyPlatform_allocateMemory(sizeof(Joint)*data->nodes_count, EASY_PLATFORM_MEMORY_ZERO);
						model.jointCount = data->nodes_count;

						model.inverseBindMatrices = (float16 *)easyPlatform_allocateMemory(sizeof(float16)*model.jointCount, EASY_PLATFORM_MEMORY_ZERO);
						model.inverseBindMatrixCount = model.jointCount;

						for(int i = 0; i < model.inverseBindMatrixCount; ++i) {
							model.inverseBindMatrices[i] = float16_identity();
						}

						nodes = data->nodes;
					}

					for(int i = 0; i < model.jointCount; ++i) {
						cgltf_node *joint;
						if(joints) {
							joint = joints[i];
						} else {
							assert(nodes);
							joint = &nodes[i];
						}
						
						if(joint) {
							model.joints[i].parentIndex = -1;
							model.joints[i].T = SQT_identity();
							if(joint->name) {
								model.joints[i].name = easyString_copyToHeap(joint->name);
							}
							if(joint->has_rotation) {
								model.joints[i].T.rotation = inverseQuaternion(make_float4(joint->rotation[0], joint->rotation[1], joint->rotation[2], joint->rotation[3]));
							}
							if(joint->has_scale) {
								model.joints[i].T.scale = make_float3(joint->scale[0], joint->scale[1], joint->scale[2]);
							}
							if(joint->has_translation) {
								model.joints[i].T.translate = make_float3(joint->translation[0], joint->translation[1], joint->translation[2]);
							}

							//NOTE: Find the parent index
							for(int k = 0; k < model.jointCount; ++k) {
								cgltf_node *testJoint;
								if(joints) {
									testJoint = joints[k];
								} else {
									assert(nodes);
									testJoint = &nodes[k];
								}

								if(testJoint == joint->parent) {
									assert(k != i);
									model.joints[i].parentIndex = k;
									model.joints[i].hasMesh = joint->mesh != 0;
									break;
								}

							}
						}
					}
				}

				for(int m = 0; m < data->meshes_count; ++m) {
					assert(data->meshes[m].primitives_count == 1);
					int boneIndex = -1;

					for(int j = 0; j < data->nodes_count && boneIndex < 0; ++j) {
						cgltf_node *n = &data->nodes[j];

						if(n->mesh == &data->meshes[m]) {
							boneIndex = j;
						}
					}
					assert(boneIndex >= 0);
					int prevVertexCount = vertexCount;

					int thisVertexCount = 0;
					for(int i = 0; i < data->meshes[m].primitives[0].attributes_count; ++i) {

						cgltf_attribute attrib = data->meshes[m].primitives[0].attributes[i];
						cgltf_accessor *accessor = attrib.data;
						cgltf_buffer_view *buffer_view = accessor->buffer_view;
						cgltf_buffer* buffer = buffer_view->buffer;
						uint8_t *ptr = (uint8_t *)buffer->data;

						void *bufferToCopyTo = 0;
						int floatCount = 3;

						bool isShort = false;

						size_t possSize = 0;
						size_t normalSize = 0;
						size_t jointsSize = 0;
						size_t weightsSize = 0;
						size_t texSize = 0;

						
						
						if(attrib.type == cgltf_attribute_type_position) {
							//NOTE: Make sure it has floats
							assert(accessor->component_type == cgltf_component_type_r_32f);
							assert(accessor->type == cgltf_type_vec3);
							bufferToCopyTo = (float *)easyPlatform_allocateMemory(buffer_view->size);
							possSize = buffer_view->size;
							vertexCount += accessor->count;
							thisVertexCount = accessor->count;
						}
						if(attrib.type == cgltf_attribute_type_normal) {
							assert(accessor->component_type == cgltf_component_type_r_32f);
							assert(accessor->type == cgltf_type_vec3);
							bufferToCopyTo = (float *)easyPlatform_allocateMemory(buffer_view->size);
							normalSize = buffer_view->size;
							normalCount += accessor->count;
						}
						if(attrib.type == cgltf_attribute_type_joints) {
							assert(accessor->component_type == cgltf_component_type_r_16u);
							assert(accessor->type == cgltf_type_vec4);
							bufferToCopyTo = (u16 *)easyPlatform_allocateMemory(buffer_view->size);
							jointCount += accessor->count;
							floatCount = 4;
							jointsSize = buffer_view->size;
							isShort = true;
						}

						if(attrib.type == cgltf_attribute_type_weights) {
							assert(accessor->component_type == cgltf_component_type_r_32f);
							assert(accessor->type == cgltf_type_vec4);
							bufferToCopyTo = (float *)easyPlatform_allocateMemory(buffer_view->size, EASY_PLATFORM_MEMORY_ZERO);
							weightsCount += accessor->count;
							weightsSize = buffer_view->size;
							floatCount = 4;
						}

						if(attrib.type == cgltf_attribute_type_texcoord) {
							assert(accessor->component_type == cgltf_component_type_r_32f);
							assert(accessor->type == cgltf_type_vec2);
							bufferToCopyTo = (float *)easyPlatform_allocateMemory(buffer_view->size, EASY_PLATFORM_MEMORY_ZERO);
							uvCount += accessor->count;
							texSize = buffer_view->size;
							floatCount = 2;
						}
						
						//NOTE: Is an actual attribute we want to retrieve
						if(bufferToCopyTo) {
							copyDataFromAccessor(bufferToCopyTo, ptr, buffer_view, accessor, isShort, floatCount);
						}

						if(possSize > 0) {
							poss = (float *)easyPlatform_reallocMemory(poss, possOldSize, possOldSize + possSize);
							easyPlatform_copyMemory(((u8 *)poss) + possOldSize, bufferToCopyTo, possSize);
							possOldSize += possSize;
						}
						if(normalSize > 0) {
							normals = (float *)easyPlatform_reallocMemory(normals, normalOldSize, normalOldSize + normalSize);
							easyPlatform_copyMemory(((u8 *)normals) + normalOldSize, bufferToCopyTo, normalSize);
							normalOldSize += normalSize;
						}
						if(texSize > 0) {
							uvs = (float *)easyPlatform_reallocMemory(uvs, texOldSize, texOldSize + texSize);
							easyPlatform_copyMemory(((u8 *)uvs) + texOldSize, bufferToCopyTo, texSize);
							texOldSize += texSize;
						}
						if(jointsSize > 0) {
							joints = (u16 *)easyPlatform_reallocMemory(joints, jointsOldSize, jointsOldSize + jointsSize);
							easyPlatform_copyMemory(((u8 *)joints) + jointsOldSize, bufferToCopyTo, jointsSize);
							jointsOldSize += jointsSize;
						}
						if(weightsSize > 0) {
							weights = (float *)easyPlatform_reallocMemory(weights, weightsOldSize, weightsOldSize + weightsSize);
							easyPlatform_copyMemory(((u8 *)weights) + weightsOldSize, bufferToCopyTo, weightsSize);
							weightsOldSize += weightsSize;
						}
						
						free(bufferToCopyTo);
						bufferToCopyTo = 0;
					}

					if(data->meshes_count > 1) {
						size_t indexesSize = sizeof(int)*thisVertexCount;
						meshIndexes = (int *)easyPlatform_reallocMemory(meshIndexes, meshIndexOldSize, meshIndexOldSize + indexesSize);

						int *ptr = (int *)(((u8 *)meshIndexes) + meshIndexOldSize);
						for(int p = 0; p < thisVertexCount; ++p) {
							ptr[p] = boneIndex;
						}
						meshIndexOldSize += indexesSize;
					}

					{	
						unsigned int *indiciesTemp = 0;
						cgltf_accessor *indiciesAccessor = data->meshes[m].primitives[0].indices;
						int thisIndicesSize = 0;
						//NOTE: Get the index data 
						if(indiciesAccessor) {
							cgltf_buffer_view *buffer_view = indiciesAccessor->buffer_view;
							cgltf_buffer* buffer = buffer_view->buffer;
							uint8_t *ptr = (uint8_t *)buffer->data;

							assert(indiciesAccessor->component_type == cgltf_component_type_r_16u);
							// assert(buffer_view->type == cgltf_buffer_view_type_indices);

							unsigned short *indicies = (unsigned short *)easyPlatform_allocateMemory(buffer_view->size, EASY_PLATFORM_MEMORY_ZERO);

							uint8_t *p = ptr + buffer_view->offset + indiciesAccessor->offset;
							memcpy(indicies, p, buffer_view->size);

							indexCount += indiciesAccessor->count;

							thisIndicesSize = sizeof(unsigned int)*indiciesAccessor->count;
							//NOTE: Upscale the indices buffer from a unsigned SHORT to a unsigned INT
							indiciesTemp = (unsigned int *)easyPlatform_allocateMemory(thisIndicesSize, EASY_PLATFORM_MEMORY_ZERO);

							//NOTE: Create the vertex data now
							int reverseWindingIndex = indiciesAccessor->count - 1;
							//NOTE: The winding is reverse of our renderer - gltf uses a CCW winding order for the front face.
							for(int i = 0; i < indiciesAccessor->count; ++i) {
								unsigned short v = indicies[reverseWindingIndex];
								indiciesTemp[i] = ((unsigned int)v) + prevVertexCount;
								reverseWindingIndex--;
							}

							free(indicies);
							indicies = 0;
							
						} else {
							indexCount += thisVertexCount;
							thisIndicesSize = sizeof(unsigned int)*indexCount;
							indiciesTemp = (unsigned int *)easyPlatform_allocateMemory(thisIndicesSize, EASY_PLATFORM_MEMORY_ZERO);

							for(int i = 0; i < thisVertexCount; ++i) {
								indiciesTemp[i] = i + prevVertexCount;
							}
						}

						indiciesToSend = (unsigned int *)easyPlatform_reallocMemory(indiciesToSend, indicesOldSize, indicesOldSize + thisIndicesSize);
						easyPlatform_copyMemory(((u8 *)indiciesToSend) + indicesOldSize, indiciesTemp, thisIndicesSize);
						indicesOldSize += thisIndicesSize;

						free(indiciesTemp);
						indiciesTemp = 0;
					}
				}

				assert(jointCount == 0 || jointCount == vertexCount);
				assert(weightsCount == 0 || weightsCount == vertexCount);
				assert(weightsCount == jointCount);
				assert(uvCount == vertexCount);

				vertexes = (VertexWithJoints *)easyPlatform_allocateMemory(sizeof(VertexWithJoints)*vertexCount, EASY_PLATFORM_MEMORY_ZERO);

				if(vertexes) {
					//NOTE: Create the vertex data now
					for(int i = 0; i < vertexCount; ++i) {
						VertexWithJoints *v = &vertexes[i];

						v->pos = make_float3((float)poss[i*3 + 0], (float)poss[i*3 + 1], (float)poss[i*3 + 2]);

						if(normals) {
							v->normal = make_float3((float)normals[i*3 + 0], (float)normals[i*3 + 1], (float)normals[i*3 + 2]);
						} 
						if(uvs) {
							v->texUV = make_float2((float)uvs[i*2 + 0], (float)uvs[i*2 + 1]);
						}

						if(joints) {
							v->jointIndexes[0] = (int)joints[i*4 + 0];
							v->jointIndexes[1] = (int)joints[i*4 + 1];
							v->jointIndexes[2] = (int)joints[i*4 + 2];
							v->jointIndexes[3] = (int)joints[i*4 + 3];
							// printf("%f, %f, %f, %f,\n", v->jointIndexes.x, v->jointIndexes.y, v->jointIndexes.z, v->jointIndexes.w);
							// v->jointIndexes = make_float4(1, 1, 1, 0);
							assert((int)v->jointIndexes[0] < jointCount);
							assert((int)v->jointIndexes[1] < jointCount);
							assert((int)v->jointIndexes[2] < jointCount);
							assert((int)v->jointIndexes[3] < jointCount);
						} else {
							v->jointIndexes[0] = meshIndexes[i];
							v->jointIndexes[1] = 0;
							v->jointIndexes[2] = 0;
							v->jointIndexes[3] = 0;
						}

						if(weights) {
							v->jointWeights = make_float4(weights[i*4 + 0], weights[i*4 + 1], weights[i*4 + 2], weights[i*4 + 3]);
							float totalJointWeight = (v->jointWeights.x + v->jointWeights.y + v->jointWeights.z + v->jointWeights.w);
							//NOTE: Make sure weights add up to zero if the vertex is weighted
							assert(totalJointWeight == 1.0f || totalJointWeight == 0.0f);
						} else {
							v->jointWeights.x = 1;
							v->jointWeights.y = 0;
							v->jointWeights.z = 0;
							v->jointWeights.w = 0;
						}
					}
				}

				free(poss);
				poss = 0;
				free(uvs);
				uvs = 0;
				free(normals);
				normals = 0;
				free(joints);
				joints = 0;
				free(weights);
				weights = 0;
				free(meshIndexes);
				meshIndexes = 0;


            }
        } 
                
        cgltf_free(data);
		data = 0;
    } else {
		printf("didn't Parsed files successfully\n");
	}

    if(result == cgltf_result_success) {
        model.valid = true;
    } 

	assert(model.valid);	

	if(vertexes && indiciesToSend) {
		model.modelBuffer = generateVertexBuffer(vertexes, vertexCount, indiciesToSend, indexCount, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL);
	}

	free(vertexes);
	vertexes = 0;
	free(indiciesToSend);
	indiciesToSend = 0;

	return model;
}

