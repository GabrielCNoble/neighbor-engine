#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_path.h"
#include "anim.h"
#include "r_com.h"
#include <stdint.h>
#include <stdio.h>
#include <float.h>

enum ARG
{
    ARG_INPUT_NAME = 1,
    ARG_PREPEND,
    ARG_ANIM_ONLY,
    ARG_NO_ARG
};

struct bone_transform_t
{
    uint32_t time;
    uint32_t bone_index;
    struct a_transform_t transform;
};

struct vert_weight_t
{
    uint32_t vert_index;
    struct a_weight_t weight;
};

struct aiNode* find_node(struct aiNode *cur_node, char *name)
{
    if(!strcmp(cur_node->mName.data, name))
    {
        return cur_node;
    }
    
    for(uint32_t child_index = 0; child_index < cur_node->mNumChildren; child_index++)
    {
        if(find_node(cur_node->mChildren[child_index], name))
        {
            return cur_node->mChildren[child_index];
        }
    }
    
    return NULL;
}

void get_them_bones(struct aiNode *cur_node, struct list_t *bones)
{
    for(uint32_t child_index = 0; child_index < cur_node->mNumChildren; child_index++)
    {
        add_list_element(bones, &cur_node->mChildren[child_index]);
        get_them_bones(cur_node->mChildren[child_index], bones);
    }
}

int32_t compare_weights_vert(void *a, void *b)
{
    struct vert_weight_t *weight_a = (struct vert_weight_t *)a;
    struct vert_weight_t *weight_b = (struct vert_weight_t *)b;
    return (int32_t)weight_a->vert_index - (int32_t)weight_b->vert_index;
}

int32_t compare_weights_value(void *a, void *b)
{
    struct a_weight_record_t *weight_a = (struct a_weight_record_t *)a;
    struct a_weight_record_t *weight_b = (struct a_weight_record_t *)b;
    if(weight_a->weight.weight > weight_b->weight.weight) return -1;
    else if(weight_a->weight.weight < weight_b->weight.weight) return 1;
    return 0;
}

int32_t compare_bone_transforms(void *a, void *b)
{
    struct bone_transform_t *keyframe_a = (struct bone_transform_t *)a;
    struct bone_transform_t *keyframe_b = (struct bone_transform_t *)b;
    
    if(keyframe_a->time > keyframe_b->time) return 1;
    else if(keyframe_a->time < keyframe_b->time)return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    struct aiScene *scene;  
    int32_t cur_arg = ARG_NO_ARG;
    char *input_name = NULL;
    char prepend[PATH_MAX] = "";
    char output_name[PATH_MAX] = "";
    uint32_t no_anim = 0;
    uint32_t no_model = 0;
    uint32_t overwrite = 0;
    if(argc > 1)
    {
        uint32_t arg_index = 0;
        while(arg_index < argc)
        {
            if(cur_arg == ARG_NO_ARG)
            {
                if(!strcmp(argv[arg_index], "-i"))
                {
                    cur_arg = ARG_INPUT_NAME;
                }
                else if(!strcmp(argv[arg_index], "-p"))
                {
                    cur_arg = ARG_PREPEND;
                }
                else if(!strcmp(argv[arg_index], "-noanim"))
                {
                    no_anim = 1;
                }
                else if(!strcmp(argv[arg_index], "-nomodel"))
                {
                    no_model = 1;
                }
                else if(!strcmp(argv[arg_index], "-f"))
                {
                    overwrite = 1;
                }
            }
            else
            {
                switch(cur_arg)
                {
                    case ARG_INPUT_NAME:
                        input_name = argv[arg_index];
                    break;
                    
                    case ARG_PREPEND:
                        strcpy(prepend, ds_path_FormatPath(argv[arg_index]));
                    break;
                }
                
                cur_arg = ARG_NO_ARG;
            }
            
            arg_index++;
        }
        
        if(cur_arg != ARG_NO_ARG)
        {
            switch(cur_arg)
            {
                case ARG_INPUT_NAME:
                    printf("missing input file name!\n");
                    return -1;
                break;
                
                case ARG_PREPEND:
                    printf("missing prepend!\n");
                break;
            }
        }
        
        scene = aiImportFile((const char *)input_name, aiProcess_CalcTangentSpace | aiProcess_Triangulate);
        
        if(!scene)
        {
            printf("couldn't load file %s\n", input_name);
            return -1;
        }
        
        if(!no_anim)
        {
            for(uint32_t animation_index = 0; animation_index < scene->mNumAnimations; animation_index++)
            {
                struct aiAnimation *animation = scene->mAnimations[animation_index];
                struct aiNode *skeleton_node = find_node(scene->mRootNode, animation->mChannels[0]->mNodeName.data);
                struct list_t node_list = create_list(sizeof(struct aiNode *), 512);
                struct list_t bone_transform_list = create_list(sizeof(struct bone_transform_t), 512);
                struct list_t pair_list = create_list(sizeof(struct a_bone_transform_pair_t), 512);
                struct list_t range_list = create_list(sizeof(struct a_transform_range_t), 512);
                
                char *animation_name = animation->mName.data;
                
                while(*animation_name != '|' && *animation_name) animation_name++;
                if(!(*animation_name))
                {
                    animation_name = animation->mName.data;
                }
                else
                {
                    animation_name++;
                }
                
                strcpy(output_name, prepend);
                strcat(output_name, "/");
                strcat(output_name, animation_name);
                strcat(output_name, ".anf");
                
                if(file_exists(output_name) && !overwrite)
                {
                    printf("An animation file named '%s' already exists in the output directory.\n", output_name);
                    printf("Do you want to overwrite it? (y\\n)   ");
                    char choice;
                    do
                    {
                        choice = 0;
                        scanf("%c", &choice);
                    }
                    while(choice == '\n');
                    if(choice == 'n' || choice == 'N')
                    {
                        break;
                    }
                }
                
                while(skeleton_node->mParent != scene->mRootNode)
                {
                    skeleton_node = skeleton_node->mParent;
                }
                
                get_them_bones(skeleton_node, &node_list);
                
                for(uint32_t channel_index = 0; channel_index < animation->mNumChannels; channel_index++)
                {
                    struct aiNodeAnim *channel = animation->mChannels[channel_index];
                    uint32_t rotation_index = 0;
                    uint32_t position_index = 0;
                    uint32_t scale_index = 0;
                    
                    for(uint32_t node_index = 0; node_index < node_list.cursor; node_index++)
                    {
                        struct aiNode *node = *(struct aiNode **)get_list_element(&node_list, node_index);
                        
                        if(!strcmp(node->mName.data, channel->mNodeName.data))
                        {
                            struct bone_transform_t bone_transform = {};
                            bone_transform.bone_index = node_index;
                            
//                            for(uint32_t index = 0; index < channel->mNumRotationKeys; index++)
//                            {
//                                struct aiQuatKey *key = channel->mRotationKeys + index;
//                                printf("[%f %f %f %f] -- [%s]\n", key->mValue.x, key->mValue.y, key->mValue.z, key->mValue.w, channel->mNodeName.data);
//                            }
                            
                            while(rotation_index < channel->mNumRotationKeys || position_index < channel->mNumPositionKeys)
                            {
                                uint32_t lowest_time = 0xffffffff;
                                struct bone_transform_t old_bone_transform = bone_transform;
                                if(rotation_index < channel->mNumRotationKeys)
                                {
                                    struct aiQuatKey *rotation = channel->mRotationKeys + rotation_index;
                                    
                                    if((uint32_t)rotation->mTime <= lowest_time)
                                    {
                                        lowest_time = rotation->mTime;
                                        rotation_index++;
                                        
                                        bone_transform.time = lowest_time;
                                        bone_transform.transform.rot.x = rotation->mValue.x;
                                        bone_transform.transform.rot.y = rotation->mValue.y;
                                        bone_transform.transform.rot.z = rotation->mValue.z;
                                        bone_transform.transform.rot.w = rotation->mValue.w;
                                    }
                                }
                                
                                if(position_index < channel->mNumPositionKeys)
                                {
                                    struct aiVectorKey *position = channel->mPositionKeys + position_index;
                                    
                                    if((uint32_t)position->mTime <= lowest_time)
                                    {
                                        position_index++;
                                        if((uint32_t)position->mTime < lowest_time)
                                        {
                                            /* this position keyframe comes before the rotation keyframe, which means 
                                            only the position must be modified. So, we restore it to the value before 
                                            the modification */
                                            bone_transform = old_bone_transform;
                                        }
                                        
                                        lowest_time = position->mTime;
                                        bone_transform.time = lowest_time;
                                        bone_transform.transform.pos.x = position->mValue.x;
                                        bone_transform.transform.pos.y = position->mValue.y;
                                        bone_transform.transform.pos.z = position->mValue.z;
                                    }
                                }
                                struct bone_transform_t *last_bone_transform = NULL;
                                if(bone_transform_list.cursor)
                                {
                                    last_bone_transform = get_list_element(&bone_transform_list, bone_transform_list.cursor - 1);
                                    if(bone_transform.time != last_bone_transform->time)
                                    {
                                        last_bone_transform = NULL;
                                    }
                                }
                                
                                if(!last_bone_transform)
                                {
//                                    printf("%d %d %s --- ", bone_transform.bone_index, bone_transform.time, channel->mNodeName.data);
//                                    printf("[%f %f %f %f] -- [%f %f %f]\n", bone_transform.transform.rot.x, 
//                                                                            bone_transform.transform.rot.y, 
//                                                                            bone_transform.transform.rot.z, 
//                                                                            bone_transform.transform.rot.w, 
//                                                                            bone_transform.transform.pos.x, 
//                                                                            bone_transform.transform.pos.y, 
//                                                                            bone_transform.transform.pos.z);
                                                                            
                                    add_list_element(&bone_transform_list, &bone_transform);
                                }
                            }
                            
                            break;
                        }
                    }
                }
                
                qsort_list(&bone_transform_list, compare_bone_transforms);
                uint32_t start_index = 0;
                for(uint32_t frame = 0; frame < (uint32_t)animation->mDuration; frame++)
                {
                    struct a_transform_range_t range = {};
                    range.start = pair_list.cursor;
                    
                    for(; start_index < bone_transform_list.cursor; start_index++)
                    {
                        struct bone_transform_t *start = get_list_element(&bone_transform_list, start_index);
                        struct a_bone_transform_pair_t pair = {};
                        if(start->time > frame)
                        {
                            break;
                        }
                        
                        pair.bone_index = start->bone_index;
                        pair.pair.start = start_index;
                        
                        if(start_index + 1 < bone_transform_list.cursor)
                        {
                            for(uint32_t end_index = start_index + 1; end_index < bone_transform_list.cursor; end_index++)
                            {
                                struct bone_transform_t *end = get_list_element(&bone_transform_list, end_index);
                                if(end->bone_index == start->bone_index)
                                {
                                    pair.pair.end = end_index;
                                    pair.pair.end_time = end->time;
                                    break;
                                }
                            }
                            
                            add_list_element(&pair_list, &pair);
                            range.count++;
                        }
                    }
                    
                    add_list_element(&range_list, &range);
                }
                
//                printf("\n\n");
//                
//                for(uint32_t keyframe_index = 0; keyframe_index < keyframe_list.cursor; keyframe_index++)
//                {
//                    struct a_keyframe_t *keyframe = get_list_element(&keyframe_list, keyframe_index);
//                    printf("%f - %d\n", keyframe->time, keyframe->bone_index);
//                }
                
                printf("Exporting animation '%s'...\n", animation_name);
                printf("Duration: %f\n", animation->mDuration);
                printf("Frame rate: %f\n", animation->mTicksPerSecond);
                printf("Bones: %d\n", node_list.cursor);
                printf("Transforms: %d\n", bone_transform_list.cursor);
                printf("Pairs: %d\n", pair_list.cursor);
                printf("Ranges: %d\n", range_list.cursor);
//                printf("Keyframes: %d\n", keyframe_list.cursor);
                
                struct ds_section_t animation_section = {};
                strcpy(animation_section.info.name, "[animation]");

                struct a_anim_sec_header_t *header;
                header = ds_append_data(&animation_section, sizeof(*header), NULL); 
                header->bone_count = node_list.cursor;
                header->pair_count = pair_list.cursor;
                header->range_count = range_list.cursor;
                header->transform_count = bone_transform_list.cursor;
                header->duration = (uint32_t)animation->mDuration;
                header->framerate = animation->mTicksPerSecond;
                strcpy(header->name, animation_name);
                
                struct a_anim_sec_data_t(header->transform_count, header->pair_count, header->range_count) *data;
                data = ds_append_data(&animation_section, sizeof(*data), NULL);
                
                for(uint32_t transform_index = 0; transform_index < bone_transform_list.cursor; transform_index++)
                {
                    struct bone_transform_t *transform = get_list_element(&bone_transform_list, transform_index);
                    data->transforms[transform_index] = transform->transform;
                }
                
                for(uint32_t pair_index = 0; pair_index < pair_list.cursor; pair_index++)
                {
                    data->pairs[pair_index] = *(struct a_bone_transform_pair_t *)get_list_element(&pair_list, pair_index);
                }
                
                for(uint32_t range_index = 0; range_index < range_list.cursor; range_index++)
                {
                    data->ranges[range_index] = *(struct a_transform_range_t *)get_list_element(&range_list, range_index);
                }
                
//                for(uint32_t keyframe_index = 0; keyframe_index < header->keyframe_count; keyframe_index++)
//                {
//                    /* first, we count how many keyframes per bone we have */
//                    struct a_keyframe_t *keyframe = get_list_element(&keyframe_list, keyframe_index);
//                    struct a_channel_t *channel = &data->channels[keyframe->bone_index];
//                    channel->count++;
//                }
                
//                for(uint32_t channel_index = 0; channel_index < header->bone_count - 1; channel_index++)
//                {
//                    struct a_channel_t *cur_channel = data->channels + channel_index;
//                    /* second, we set the start of each channel, which defines a range into the keyframe_indices
//                    array */
//                    data->channels[channel_index + 1].start = cur_channel->start + cur_channel->count;
//                    cur_channel->count = 0;
//                }
//                
//                for(uint32_t keyframe_index = 0; keyframe_index < header->keyframe_count; keyframe_index++)
//                {
//                    /* third, we go the keyframes once more, and fill in the keyframe_indices array, using
//                    the channels to know where to store the current index. */
//                    struct a_keyframe_t *keyframe = get_list_element(&keyframe_list, keyframe_index);
//                    struct a_channel_t *channel = data->channels + keyframe->bone_index;
//                    data->keyframes[keyframe_index] = *keyframe;
//                    data->keyframe_indices[channel->start + channel->count] = keyframe_index;
//                    channel->count++;
//                }
                
                
                void *buffer;
                uint32_t buffer_size;
                
                struct ds_section_t *sections[] = {&animation_section};
                
                ds_serialize_sections(&buffer, &buffer_size, 1, sections);
                ds_free_section(&animation_section);
                destroy_list(&node_list);
                destroy_list(&bone_transform_list);
                destroy_list(&pair_list);
                destroy_list(&range_list);
                
                FILE *file = fopen(output_name, "wb");
                fwrite(buffer, buffer_size, 1, file);
                fclose(file);
                mem_Free(buffer);
                
                printf("Animation '%s' exported!\n\n", animation_name);
            }
        }
        
        if(!no_model)
        {
            for(uint32_t object_index = 0; object_index < scene->mRootNode->mNumChildren; object_index++)
            {
                struct aiNode *object = scene->mRootNode->mChildren[object_index];
                
                if(!object->mMeshes)
                {
                    continue;
                }
                
                strcpy(output_name, prepend);
                strcat(output_name, "/");
                strcat(output_name, object->mName.data);
                strcat(output_name, ".mof");
                
                if(file_exists(output_name) && !overwrite)
                {
                    printf("A model file named '%s' already exists in the output directory.\n", output_name);
                    printf("Do you want to overwrite it? (y\\n)   ");
                    char choice;
                    do
                    {
                        choice = 0;
                        scanf("%c", &choice);
                    }
                    while(choice == '\n');
                    
                    if(choice == 'n' || choice == 'N')
                    {
                        break;
                    }
                }
                
                struct ds_section_t skeleton_section = {};
                strcpy(skeleton_section.info.name, "[skeleton]");
                struct ds_section_t weight_section = {};
                strcpy(weight_section.info.name, "[weights]");
                struct ds_section_t material_section = {};
                strcpy(material_section.info.name, "[materials]");
                struct ds_section_t batch_section = {};
                strcpy(batch_section.info.name, "[batches]");
                struct ds_section_t vert_section = {};
                strcpy(vert_section.info.name, "[vertices]");
                struct ds_section_t index_section = {};
                strcpy(index_section.info.name, "[indices]");
                
                struct aiMesh *mesh = scene->mMeshes[object->mMeshes[0]];
                uint32_t bone_count = 0;
                uint32_t weight_count = 0;
                uint32_t range_count = 0;
                
                if(mesh->mBones)
                {
                    struct list_t bone_list = create_list(sizeof(struct aiBone *), 512);
                    struct list_t node_list = create_list(sizeof(struct aiNode *), 512);
                    struct list_t weight_list = create_list(sizeof(struct vert_weight_t), 512);
                    struct list_t range_list = create_list(sizeof(struct a_weight_range_t), 512);
                
                    struct aiNode *skeleton_node = find_node(scene->mRootNode, mesh->mBones[0]->mName.data);
                    
                    while(skeleton_node->mParent != scene->mRootNode)
                    {
                        skeleton_node = skeleton_node->mParent;
                    }
                    /* store all nodes that represent the skeleton this object uses in depth-first order */
                    get_them_bones(skeleton_node, &node_list);
                    
                    struct a_skeleton_section_t *skeleton;
                    skeleton = ds_append_data(&skeleton_section, sizeof(struct a_skeleton_section_t), NULL);
                    
                    for(uint32_t mesh_index = 0; mesh_index < object->mNumMeshes; mesh_index++)
                    {
                        struct aiMesh *mesh = scene->mMeshes[object->mMeshes[mesh_index]];
                        
                        for(uint32_t bone_index = 0; bone_index < mesh->mNumBones; bone_index++)
                        {
                            /* store all bones all meshes of this object use */
                            add_list_element(&bone_list, mesh->mBones + bone_index);
                        }
                    }
                    
                    
                    
                    for(uint32_t bone_index = 0; bone_index < bone_list.cursor; bone_index++)
                    {
                        struct aiBone *bone = *(struct aiBone **)get_list_element(&bone_list, bone_index);
                        
                        for(uint32_t weight_index = 0; weight_index < bone->mNumWeights; weight_index++)
                        {
                            /* for every weight of every bone... */
                            struct aiVertexWeight *weight = bone->mWeights + weight_index;
                            struct vert_weight_t vert_weight = {};
                            
                            /* keep which vertex it belongs to... */
                            vert_weight.vert_index = weight->mVertexId;
                            vert_weight.weight.weight = weight->mWeight;
                            
                            for(uint32_t node_index = 0; node_index < node_list.cursor; node_index++)
                            {
                                struct aiNode *node = *(struct aiNode **)get_list_element(&node_list, node_index);
                                if(!strcmp(bone->mName.data, node->mName.data))
                                {
                                    /* and which node in depth-first order it belongs to */
                                    vert_weight.weight.bone_index = node_index;
                                    break;
                                }
                            }
                            
                            /* we store in a temporary list because we'll need to sort them by vertex index */
                            add_list_element(&weight_list, &vert_weight);
                        }
                    }
                    
                    /* sort the list by vertex index, so weights of vertex 0 comes before all weights
                    of vertex 1, and so on. Here we'll export everything that assimp has got to give us,
                    even though we'll likely limit the number of weights per vertex during import (likely
                    4 weights per vertex) */
                    qsort_list(&weight_list, compare_weights_vert);
                    uint32_t cur_vert = 0xffffffff;
                    uint32_t left = 0;
                    uint32_t right = 0;
                    struct vert_weight_t *vert_weight = NULL;
                    struct a_weight_range_t range = {};
                    do
                    {
                        vert_weight = get_list_element(&weight_list, right);
                        cur_vert = vert_weight->vert_index;
                        range.start = right;
                        while(vert_weight && vert_weight->vert_index == cur_vert)
                        {
                            right++;
                            vert_weight = get_list_element(&weight_list, right);
                        }
                        range.count = right - range.start;
                        add_list_element(&range_list, &range);
                        if(left + 1 < right)
                        {
                            qsort_list_rec(&weight_list, left, right - 1, compare_weights_value);
                        }
                        left = right;
                    }
                    while(vert_weight);
                    
                    cur_vert = 0xffffffff;
                    float total_weight = 0.0;
                    uint32_t first_weight = 0;
                    for(uint32_t weight_index = 0; weight_index < weight_list.cursor; weight_index++)
                    {
                        struct vert_weight_t *vert_weight = get_list_element(&weight_list, weight_index);
                        if(vert_weight->vert_index != cur_vert)
                        {
                            if(cur_vert != 0xffffffff)
                            {
                                float missing = 1.0 - total_weight;
                                if(missing != 0.0)
                                {
                                    /* very likely to happen... */
                                    for(uint32_t fix_index = first_weight; fix_index < weight_index; fix_index++)
                                    {
                                        struct vert_weight_t *fix_weight = get_list_element(&weight_list, fix_index);
                                        float proportion = fix_weight->weight.weight / total_weight;
                                        fix_weight->weight.weight += missing * proportion;
                                    }
                                }
                                total_weight = 0.0;
                            }
                            first_weight = weight_index;
                            cur_vert = vert_weight->vert_index;
                        }
                        
                        total_weight += vert_weight->weight.weight;
                    }
                     
                    struct a_weight_section_t(weight_list.cursor, range_list.cursor) *weights;
                    weights = ds_append_data(&weight_section, sizeof(*weights), NULL);
                    
                    weights->weight_count = weight_list.cursor;
                    weights->range_count = range_list.cursor;
                    
                    for(uint32_t weight_index = 0; weight_index < weights->weight_count; weight_index++)
                    {
                        struct vert_weight_t *weight = get_list_element(&weight_list, weight_index);
                        weights->weights[weight_index] = weight->weight;
                    }
                    
                    for(uint32_t range_index = 0; range_index < weights->range_count; range_index++)
                    {
                        struct a_weight_range_t *range = get_list_element(&range_list, range_index);
                        weights->ranges[range_index] = *range;
                    }

                    /* now we generate the actual bone/weight data to be exported... */                    
                    
                    skeleton->bone_count = node_list.cursor;
                    for(uint32_t node_index = 0; node_index < node_list.cursor; node_index++)
                    {
                        struct aiNode *node = *(struct aiNode **)get_list_element(&node_list, node_index);
                        struct a_bone_t *bone = ds_append_data(&skeleton_section, sizeof(struct a_bone_t), NULL);
                        for(uint32_t bone_index = 0; bone_index < bone_list.cursor; bone_index++)
                        {
                            struct aiBone *ai_bone = *(struct aiBone **)get_list_element(&bone_list, bone_index);
                            if(!strcmp(ai_bone->mName.data, node->mName.data))
                            {
                                bone->inv_bind_matrix.rows[0].x = ai_bone->mOffsetMatrix.a1;
                                bone->inv_bind_matrix.rows[0].y = ai_bone->mOffsetMatrix.b1;
                                bone->inv_bind_matrix.rows[0].z = ai_bone->mOffsetMatrix.c1;
                                bone->inv_bind_matrix.rows[0].w = ai_bone->mOffsetMatrix.d1;
                                bone->inv_bind_matrix.rows[1].x = ai_bone->mOffsetMatrix.a2;
                                bone->inv_bind_matrix.rows[1].y = ai_bone->mOffsetMatrix.b2;
                                bone->inv_bind_matrix.rows[1].z = ai_bone->mOffsetMatrix.c2;
                                bone->inv_bind_matrix.rows[1].w = ai_bone->mOffsetMatrix.d2;
                                bone->inv_bind_matrix.rows[2].x = ai_bone->mOffsetMatrix.a3;
                                bone->inv_bind_matrix.rows[2].y = ai_bone->mOffsetMatrix.b3;
                                bone->inv_bind_matrix.rows[2].z = ai_bone->mOffsetMatrix.c3;
                                bone->inv_bind_matrix.rows[2].w = ai_bone->mOffsetMatrix.d3;
                                bone->inv_bind_matrix.rows[3].x = ai_bone->mOffsetMatrix.a4;
                                bone->inv_bind_matrix.rows[3].y = ai_bone->mOffsetMatrix.b4;
                                bone->inv_bind_matrix.rows[3].z = ai_bone->mOffsetMatrix.c4;
                                bone->inv_bind_matrix.rows[3].w = ai_bone->mOffsetMatrix.d4;
                                break;
                            }
                        }
                        
                        bone->transform.rows[0].x = node->mTransformation.a1;
                        bone->transform.rows[0].y = node->mTransformation.b1;
                        bone->transform.rows[0].z = node->mTransformation.c1;
                        bone->transform.rows[0].w = node->mTransformation.d1;
                        bone->transform.rows[1].x = node->mTransformation.a2;
                        bone->transform.rows[1].y = node->mTransformation.b2;
                        bone->transform.rows[1].z = node->mTransformation.c2;
                        bone->transform.rows[1].w = node->mTransformation.d2;
                        bone->transform.rows[2].x = node->mTransformation.a3;
                        bone->transform.rows[2].y = node->mTransformation.b3;
                        bone->transform.rows[2].z = node->mTransformation.c3;
                        bone->transform.rows[2].w = node->mTransformation.d3;
                        bone->transform.rows[3].x = node->mTransformation.a4;
                        bone->transform.rows[3].y = node->mTransformation.b4;
                        bone->transform.rows[3].z = node->mTransformation.c4;
                        bone->transform.rows[3].w = node->mTransformation.d4;
                        bone->child_count = node->mNumChildren;
                    }
                    
                    bone_count = node_list.cursor;
                    weight_count = weight_list.cursor;
                    range_count = range_list.cursor;
                    
                    destroy_list(&weight_list);
                    destroy_list(&range_list);
                    destroy_list(&node_list);
                    destroy_list(&bone_list);
                }
                
                struct r_material_section_t *materials = ds_append_data(&material_section, sizeof(struct r_material_section_t), NULL);
                struct r_batch_section_t *batches = ds_append_data(&batch_section, sizeof(struct r_batch_section_t), NULL);
                struct r_index_section_t *indices = ds_append_data(&index_section, sizeof(struct r_index_section_t), NULL);
                struct r_vert_section_t *vertices = ds_append_data(&vert_section, sizeof(struct r_vert_section_t), NULL);
                
                batches->batch_count = object->mNumMeshes;
                materials->material_count = object->mNumMeshes;
                
                for(uint32_t batch_index = 0; batch_index < object->mNumMeshes; batch_index++)
                {
                    struct aiMesh *mesh = scene->mMeshes[object->mMeshes[batch_index]];
                    struct r_batch_record_t batch_record = {};
                    
                    struct r_material_record_t material_record = {};
                    struct aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
                    struct aiString name;
                    
                    aiGetMaterialString(material, AI_MATKEY_NAME, &name);
                    strcpy(material_record.name, name.data);
                    
                    if(aiGetMaterialString(material, AI_MATKEY_TEXTURE_DIFFUSE(0), &name) == aiReturn_SUCCESS)
                    {
                        strcpy(material_record.diffuse_texture, name.data);
                    }
                    
                    if(aiGetMaterialString(material, AI_MATKEY_TEXTURE_NORMALS(0), &name) == aiReturn_SUCCESS)
                    {
                        strcpy(material_record.normal_texture, name.data);
                    }
                    
                    ds_append_data(&material_section, sizeof(struct r_material_record_t), &material_record);
                    
                    
                    strcpy(batch_record.material, material_record.name);
                    batch_record.start = indices->index_count;
                    
                    for(uint32_t vert_index = 0; vert_index < mesh->mNumVertices; vert_index++)
                    {
                        struct r_vert_t vert = {};
                        
                        vert.pos.x = mesh->mVertices[vert_index].x;
                        vert.pos.y = mesh->mVertices[vert_index].y;
                        vert.pos.z = mesh->mVertices[vert_index].z;
                        
                        vert.normal.x = mesh->mNormals[vert_index].x;
                        vert.normal.y = mesh->mNormals[vert_index].y;
                        vert.normal.z = mesh->mNormals[vert_index].z;
                        
                        vert.tangent.x = mesh->mTangents[vert_index].x;
                        vert.tangent.y = mesh->mTangents[vert_index].y;
                        vert.tangent.z = mesh->mTangents[vert_index].z;
                        
                        vert.tex_coords.x = mesh->mTextureCoords[0][vert_index].x;
                        vert.tex_coords.y = mesh->mTextureCoords[0][vert_index].y;
                        
                        ds_append_data(&vert_section, sizeof(struct r_vert_t), &vert);
                    }
                    
                    vertices->vert_count += mesh->mNumVertices;
                    
                    for(uint32_t face_index = 0; face_index < mesh->mNumFaces; face_index++)
                    {
                        struct aiFace *face = mesh->mFaces + face_index;
                        batch_record.count += face->mNumIndices;
                        indices->index_count += face->mNumIndices;
                        ds_append_data(&index_section, sizeof(uint32_t) * face->mNumIndices, face->mIndices);
                    }
                    
                    ds_append_data(&batch_section, sizeof(struct r_batch_record_t), &batch_record);
                }
                
                printf("Exporting model '%s'...\n", object->mName.data);
                printf("Vertices: %d\n", vertices->vert_count);
                printf("Indices: %d\n", indices->index_count);
                printf("Batches: %d\n", batches->batch_count);
                
                void *buffer;
                uint32_t buffer_size;
                uint32_t section_count = 4;
                struct ds_section_t *sections[] = 
                {
                    &material_section,
                    &batch_section,
                    &vert_section,
                    &index_section, 
                    NULL,
                    NULL,
                };
                
                if(bone_count)
                {
                    printf("Bones: %d\n", bone_count);
                    printf("Weights: %d\n", weight_count);
                    printf("Ranges: %d\n", range_count);
                    sections[4] = &skeleton_section;
                    sections[5] = &weight_section;
                    section_count = 6;
                }
                
                ds_serialize_sections(&buffer, &buffer_size, section_count, sections);
                
                for(uint32_t section_index = 0; section_index < section_count; section_index++)
                {
                    ds_free_section(sections[section_index]);
                }
                
                FILE *file = fopen(output_name, "wb");
                fwrite(buffer, buffer_size, 1, file);
                fclose(file);
                mem_Free(buffer);
                
                printf("Model '%s' exported!\n\n", object->mName.data);
            }
        }
    }
    
    return 0;
}






