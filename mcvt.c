#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_path.h"
#include "engine/anim.h"
#include "engine/r_defs.h"
#include <stdint.h>
#include <stdio.h>
#include <float.h>

enum ARG
{
    ARG_INPUT_NAME = 1,
    ARG_PREPEND,
    ARG_PREPEND_ANIM,
    ARG_PREPEND_MODEL,
    ARG_MODEL_NAME,
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

void get_them_bones(struct aiNode *cur_node, struct ds_list_t *bones)
{
    for(uint32_t child_index = 0; child_index < cur_node->mNumChildren; child_index++)
    {
        ds_list_add_element(bones, &cur_node->mChildren[child_index]);
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
    struct aiPropertyStore *properties;
    int32_t cur_arg = ARG_NO_ARG;
    char *input_name = NULL;
    char prepend_anim[PATH_MAX] = "./";
    char prepend_model[PATH_MAX] = "./";
    char output_name[PATH_MAX] = "";
    char model_name[PATH_MAX] = "";
    uint32_t no_anim = 0;
    uint32_t no_model = 0;
    uint32_t overwrite = 0;
    if(argc > 1)
    {
        uint32_t arg_index = 1;
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
                else if(!strcmp(argv[arg_index], "-pmodel"))
                {
                    cur_arg = ARG_PREPEND_MODEL;
                }
                else if(!strcmp(argv[arg_index], "-panim"))
                {
                    cur_arg = ARG_PREPEND_ANIM;
                }
                else if(!strcmp(argv[arg_index], "-noanim"))
                {
                    no_anim = 1;
                }
                else if(!strcmp(argv[arg_index], "-nomodel"))
                {
                    no_model = 1;
                }
                else if(!strcmp(argv[arg_index], "-mname"))
                {
                    cur_arg = ARG_MODEL_NAME;
                }
                else if(!strcmp(argv[arg_index], "-f"))
                {
                    overwrite = 1;
                }
                else if(!strcmp(argv[arg_index], "-help"))
                {
                    printf("mcvt - mesh coverter\n");
                    printf("arguments:\n\n");
                    printf("-i : input file name. Since input files may contain several\n");
                    printf("models, each with potentially several animations, output names\n");
                    printf("will be those present in the files, but with .anf extension for\n");
                    printf("animations, and .mof for models\n\n");
                    printf("-p : prepend path to output file name. If not provided, output\n");
                    printf("files will be written to the current directory.\n\n");
                    printf("-pmodel : prepend path just to .mof files. Other than that, same\n");
                    printf("deal as with -p.\n\n");
                    printf("-panim : prepend path just to .anf files. Other than that, same\n");
                    printf("deal as with -p.\n\n");
                    printf("-f : force overwrite of existing files. If this is not\n");
                    printf("provided the tool will ask whether to overwrite or not the\n");
                    printf("existing file.\n\n");
                    printf("-noanim : don't output animation data.\n\n");
                    printf("-nomodel : don't output model data.\n\n");
                    printf("-help : prints this help, then exits.\n\n");
                    return 0;
                }
                else
                {
                    printf("unknown argument '%s'\n", argv[arg_index]);
                    return 0;
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
                        ds_path_format_path(argv[arg_index], prepend_anim, PATH_MAX);
                        ds_path_format_path(argv[arg_index], prepend_model, PATH_MAX);
                    break;

                    case ARG_PREPEND_ANIM:
                        ds_path_format_path(argv[arg_index], prepend_anim, PATH_MAX);
                    break;

                    case ARG_PREPEND_MODEL:
                        ds_path_format_path(argv[arg_index], prepend_model, PATH_MAX);
                    break;

                    case ARG_MODEL_NAME:
                        strncpy(model_name, argv[arg_index], sizeof(model_name));
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

                case ARG_PREPEND_MODEL:
                    printf("missing model prepend!\n");
                break;

                case ARG_PREPEND_ANIM:
                    printf("missing anim prepend!\n");
                break;
            }
        }

//        properties = aiCreatePropertyStore();
//        aiSetImportPropertyInteger(properties, AI_CONFIG_IMPORT_FBX_OPTIMIZE_EMPTY_ANIMATION_CURVES, 0);
//        scene = aiImportFileExWithProperties((const char *)input_name, aiProcess_CalcTangentSpace | aiProcess_Triangulate, NULL, properties);
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
                struct ds_list_t node_list = ds_list_create(sizeof(struct aiNode *), 512);
                struct ds_list_t bone_transform_list = ds_list_create(sizeof(struct bone_transform_t), 512);
                struct ds_list_t pair_list = ds_list_create(sizeof(struct a_transform_pair_t), 512);

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

                strcpy(output_name, prepend_anim);
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
                    if(choice != 'y' || choice != 'Y')
                    {
                        break;
                    }
                }

                while(skeleton_node->mParent != scene->mRootNode)
                {
                    skeleton_node = skeleton_node->mParent;
                }

                /* get bones in depth-first order */
                get_them_bones(skeleton_node, &node_list);
                uint32_t duration = (uint32_t)((animation->mDuration / 1000.0) * animation->mTicksPerSecond);

                for(uint32_t channel_index = 0; channel_index < animation->mNumChannels; channel_index++)
                {
                    /* each channel contains all the keyframes of a single node */
                    struct aiNodeAnim *channel = animation->mChannels[channel_index];
                    uint32_t rotation_index = 0;
                    uint32_t position_index = 0;
                    uint32_t scale_index = 0;

                    for(uint32_t node_index = 0; node_index < node_list.cursor; node_index++)
                    {
                        struct aiNode *node = *(struct aiNode **)ds_list_get_element(&node_list, node_index);

                        if(!strcmp(node->mName.data, channel->mNodeName.data))
                        {
                            struct bone_transform_t bone_transform = {};
                            bone_transform.bone_index = node_index;

                            if(channel->mNumPositionKeys == 1 && channel->mNumRotationKeys == 1)
                            {
                                /* this channel has a single keyframe, but we need a keyframe at the start and
                                end of the animation, so we duplicate it  */
                                bone_transform.transform.rot.x = channel->mRotationKeys->mValue.x;
                                bone_transform.transform.rot.y = channel->mRotationKeys->mValue.y;
                                bone_transform.transform.rot.z = channel->mRotationKeys->mValue.z;
                                bone_transform.transform.rot.w = channel->mRotationKeys->mValue.w;

                                bone_transform.transform.pos.x = channel->mPositionKeys->mValue.x;
                                bone_transform.transform.pos.y = channel->mPositionKeys->mValue.y;
                                bone_transform.transform.pos.z = channel->mPositionKeys->mValue.z;

                                bone_transform.time = 0;
                                ds_list_add_element(&bone_transform_list, &bone_transform);
                                bone_transform.time = duration;
                                ds_list_add_element(&bone_transform_list, &bone_transform);
                            }
                            else
                            {
                                while(rotation_index < channel->mNumRotationKeys || position_index < channel->mNumPositionKeys)
                                {
                                    /* this bone has more than on keyframe, so we accumulate them all. A bone might have a
                                    rotation without translation (or vice versa) in a given keyframe. If this is the case,
                                    we just duplicate the one that's missing from the previous keyframe */
                                    uint32_t lowest_time = 0xffffffff;
                                    struct bone_transform_t old_bone_transform = bone_transform;
                                    if(rotation_index < channel->mNumRotationKeys)
                                    {
                                        struct aiQuatKey *rotation = channel->mRotationKeys + rotation_index;

                                        uint32_t time = (uint32_t)((rotation->mTime / 1000.0) * animation->mTicksPerSecond);
//                                        if(time <= lowest_time)
//                                        {
                                        lowest_time = time;
                                        rotation_index++;

                                        bone_transform.time = lowest_time;
                                        bone_transform.transform.rot.x = rotation->mValue.x;
                                        bone_transform.transform.rot.y = rotation->mValue.y;
                                        bone_transform.transform.rot.z = rotation->mValue.z;
                                        bone_transform.transform.rot.w = rotation->mValue.w;
//                                        }
                                    }

                                    if(position_index < channel->mNumPositionKeys)
                                    {
                                        struct aiVectorKey *position = channel->mPositionKeys + position_index;
                                        uint32_t time = (uint32_t)((position->mTime / 1000.0) * animation->mTicksPerSecond);
                                        if(time <= lowest_time)
                                        {
                                            /* this position keyframe may be for the same frame, or for a different that comes before
                                            the rotation keyframe */
                                            position_index++;
                                            if(time < lowest_time)
                                            {
                                                /* this position keyframe comes before the rotation keyframe, which means
                                                only the position must be modified. So, we restore it to the value before
                                                the modification */
                                                bone_transform = old_bone_transform;
                                            }

                                            lowest_time = time;
                                            bone_transform.time = lowest_time;
                                            bone_transform.transform.pos.x = position->mValue.x;
                                            bone_transform.transform.pos.y = position->mValue.y;
                                            bone_transform.transform.pos.z = position->mValue.z;
                                        }
                                    }


                                    /* make sure we don't have duplicate frames, because for some reason assimp
                                    sometimes handles us that...? */
                                    struct bone_transform_t *last_bone_transform = NULL;
                                    if(bone_transform_list.cursor)
                                    {
                                        last_bone_transform = ds_list_get_element(&bone_transform_list, bone_transform_list.cursor - 1);
                                        if(bone_transform.time != last_bone_transform->time)
                                        {
                                            last_bone_transform = NULL;
                                        }
                                    }

                                    if(!last_bone_transform)
                                    {
                                        ds_list_add_element(&bone_transform_list, &bone_transform);
                                    }
                                }
                            }

                            break;
                        }
                    }
                }

                /* sort keyframes by time */
                ds_list_qsort(&bone_transform_list, compare_bone_transforms);


                /* for each frame in the animation, we need a list of pairs, sorted by bone.
                This is required to keep the skeleton in a consistent state even if frames are
                skipped. Also required for seeking. Since it's likely animations will have more
                than one frame, we'll have several of those lists, and the total number of pairs
                will be the bone_count * animation_length */

                for(uint32_t frame = 0; frame < duration; frame++)
                {
                    /* so, for every frame in the animation... */
                    for(uint32_t node_index = 0; node_index < node_list.cursor; node_index++)
                    {
                        /* we go over all the bones... */
                        struct bone_transform_t *start_transform;
                        struct bone_transform_t *end_transform;
                        struct a_transform_pair_t pair = {.start = 0xffff, .end = 0xffff};

                        for(uint32_t start_index = 0; start_index < bone_transform_list.cursor; start_index++)
                        {
                            start_transform = ds_list_get_element(&bone_transform_list, start_index);
                            if(start_transform->bone_index == node_index && start_transform->time <= frame)
                            {
                                /* we find the first transform that starts at the current frame, or before
                                it, that belongs to the current bone. We take transforms that start before
                                the current frame because we're looking for a pair of tranforms that enclose
                                the current frame, and a pair of transforms might get used by several consecutive
                                frames */

                                uint32_t end_index = start_index + 1;

                                for(; end_index < bone_transform_list.cursor; end_index++)
                                {
                                    /* we then find the next transform for this bone after the first we found. Since all
                                    transforms are sorted by time, and there's only one transform per frame for each bone,
                                    this transform has to have a frame time higher than the first */

                                    end_transform = ds_list_get_element(&bone_transform_list, end_index);
                                    if(end_transform->bone_index == node_index)
                                    {
                                        break;
                                    }
                                }

                                if(end_index < bone_transform_list.cursor && end_transform->time > frame)
                                {
                                    /* we check to see if the current frame is contained between those two
                                    transforms. If it is, we create a pair and store it. */
                                    pair.start = start_index;
                                    pair.end = end_index;
                                    pair.end_frame = end_transform->time;
                                    break;
                                }

                                /* otherwise, the current frame is outside the interval defined by the two
                                frames, and we continue the search from the second transform we found for this
                                bone, now treating it as the start transform */
                                start_index = end_index - 1;
                            }
                        }

                        ds_list_add_element(&pair_list, &pair);
                    }
                }

                printf("Exporting animation '%s'...\n", animation_name);
                printf("Duration: %d\n", duration);
                printf("Frame rate: %f\n", animation->mTicksPerSecond);
                printf("Bones: %d\n", node_list.cursor);
                printf("Transforms: %d\n", bone_transform_list.cursor);
                printf("Pairs: %d\n", pair_list.cursor);

                struct ds_section_t animation_section = {};
                strcpy(animation_section.info.name, "[animation]");

                struct a_anim_sec_header_t *header;
                header = ds_append_data(&animation_section, sizeof(*header), NULL);
                header->bone_count = node_list.cursor;
                header->transform_count = bone_transform_list.cursor;
                header->duration = duration;
                header->framerate = animation->mTicksPerSecond;
                strcpy(header->name, animation_name);

                struct a_anim_sec_data_t(header->transform_count, pair_list.cursor) *data;
                data = ds_append_data(&animation_section, sizeof(*data), NULL);

                for(uint32_t transform_index = 0; transform_index < bone_transform_list.cursor; transform_index++)
                {
                    struct bone_transform_t *transform = ds_list_get_element(&bone_transform_list, transform_index);
                    data->transforms[transform_index] = transform->transform;
                }

                for(uint32_t pair_index = 0; pair_index < pair_list.cursor; pair_index++)
                {
                    data->pairs[pair_index] = *(struct a_transform_pair_t *)ds_list_get_element(&pair_list, pair_index);
                }

                void *buffer;
                uint32_t buffer_size;

                struct ds_section_t *sections[] = {&animation_section};

                ds_serialize_sections(&buffer, &buffer_size, 1, sections);
                ds_free_section(&animation_section);
                ds_list_destroy(&node_list);
                ds_list_destroy(&bone_transform_list);
                ds_list_destroy(&pair_list);

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

                strcpy(output_name, prepend_model);
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

                    if(choice != 'y' || choice != 'Y')
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
                    struct ds_list_t bone_list = ds_list_create(sizeof(struct aiBone *), 512);
                    struct ds_list_t node_list = ds_list_create(sizeof(struct aiNode *), 512);
                    struct ds_list_t weight_list = ds_list_create(sizeof(struct vert_weight_t), 512);
                    struct ds_list_t range_list = ds_list_create(sizeof(struct a_weight_range_t), 512);

                    struct aiNode *skeleton_node = find_node(scene->mRootNode, mesh->mBones[0]->mName.data);

                    while(skeleton_node->mParent != scene->mRootNode)
                    {
                        skeleton_node = skeleton_node->mParent;
                    }
                    /* store all nodes that represent the skeleton this object uses in depth-first order */
                    get_them_bones(skeleton_node, &node_list);

                    for(uint32_t mesh_index = 0; mesh_index < object->mNumMeshes; mesh_index++)
                    {
                        struct aiMesh *mesh = scene->mMeshes[object->mMeshes[mesh_index]];

                        for(uint32_t bone_index = 0; bone_index < mesh->mNumBones; bone_index++)
                        {
                            /* store all bones all meshes of this object use */
                            ds_list_add_element(&bone_list, mesh->mBones + bone_index);
                        }
                    }

                    uint32_t bone_name_len = 0;

                    for(uint32_t bone_index = 0; bone_index < bone_list.cursor; bone_index++)
                    {
                        struct aiBone *bone = *(struct aiBone **)ds_list_get_element(&bone_list, bone_index);
                        bone_name_len += bone->mName.length + 1;
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
                                struct aiNode *node = *(struct aiNode **)ds_list_get_element(&node_list, node_index);
                                if(!strcmp(bone->mName.data, node->mName.data))
                                {
                                    /* and which node in depth-first order it belongs to */
                                    vert_weight.weight.bone_index = node_index;
                                    break;
                                }
                            }

                            /* we store in a temporary list because we'll need to sort them by vertex index */
                            ds_list_add_element(&weight_list, &vert_weight);
                        }
                    }

                    /* sort the list by vertex index, so weights of vertex 0 comes before all weights
                    of vertex 1, and so on. Here we'll export everything that assimp has got to give us,
                    even though we'll likely limit the number of weights per vertex during import (likely
                    4 weights per vertex) */
                    ds_list_qsort(&weight_list, compare_weights_vert);
                    uint32_t cur_vert = 0xffffffff;
                    uint32_t left = 0;
                    uint32_t right = 0;
                    struct vert_weight_t *vert_weight = NULL;
                    struct a_weight_range_t range = {};
                    do
                    {
                        /* we'll also sort the list by weight value, so larger weights come before
                        smaller weights. This is to have guarantee that the larger weights are considered
                        in case the importer decides to not use every weight. */


                        /* we'll be sorting the list in parts. Each part is composed by weights
                        that point to the same vertex. So, all weights of vertex 0 will be sorted
                        by weight, then all weights of vertex 1, and so on.*/

                        vert_weight = ds_list_get_element(&weight_list, right);
                        cur_vert = vert_weight->vert_index;
                        range.start = right;
                        while(vert_weight && vert_weight->vert_index == cur_vert)
                        {
                            /* count how many weights there are for the current vertex */
                            right++;
                            vert_weight = ds_list_get_element(&weight_list, right);
                        }
                        range.count = right - range.start;
                        ds_list_add_element(&range_list, &range);
                        if(left + 1 < right)
                        {
                            ds_list_qsort_rec(&weight_list, left, right - 1, compare_weights_value);
                        }
                        left = right;
                    }
                    while(vert_weight);

                    /* for some funny reason assimp doesn't seem to like giving weights that amount to
                    1.0 (or something decently close to it), so we'll scale all the weights here to make
                    sure they add up to one (or something decently close to it). Larger weights get rescaled
                    more than smaller weights, since they contribute more to the final result. */
                    cur_vert = 0xffffffff;
                    float total_weight = 0.0;
                    uint32_t first_weight = 0;
                    for(uint32_t weight_index = 0; weight_index < weight_list.cursor; weight_index++)
                    {
                        struct vert_weight_t *vert_weight = ds_list_get_element(&weight_list, weight_index);
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
                                        struct vert_weight_t *fix_weight = ds_list_get_element(&weight_list, fix_index);
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
                        struct vert_weight_t *weight = ds_list_get_element(&weight_list, weight_index);
                        weights->weights[weight_index] = weight->weight;
                    }

                    for(uint32_t range_index = 0; range_index < weights->range_count; range_index++)
                    {
                        struct a_weight_range_t *range = ds_list_get_element(&range_list, range_index);
                        weights->ranges[range_index] = *range;
                    }

                    /* now we generate the actual bone/weight data to be exported... */

                    struct a_skeleton_section_t(node_list.cursor, bone_name_len) *skeleton;
                    skeleton = ds_append_data(&skeleton_section, sizeof(*skeleton), NULL);

                    skeleton->bone_count = node_list.cursor;
                    for(uint32_t node_index = 0; node_index < node_list.cursor; node_index++)
                    {
                        struct aiNode *node = *(struct aiNode **)ds_list_get_element(&node_list, node_index);
                        struct a_bone_t *bone = skeleton->bones + node_index;
                        char *bone_name = skeleton->bone_names + skeleton->bone_names_length;
                        for(uint32_t bone_index = 0; bone_index < bone_list.cursor; bone_index++)
                        {
                            struct aiBone *ai_bone = *(struct aiBone **)ds_list_get_element(&bone_list, bone_index);
                            if(!strcmp(ai_bone->mName.data, node->mName.data))
                            {
                                strcpy(bone_name, ai_bone->mName.data);
                                skeleton->bone_names_length += ai_bone->mName.length + 1;

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

                    ds_list_destroy(&weight_list);
                    ds_list_destroy(&range_list);
                    ds_list_destroy(&node_list);
                    ds_list_destroy(&bone_list);
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

                    vertices->min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
                    vertices->max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);

                    for(uint32_t vert_index = 0; vert_index < mesh->mNumVertices; vert_index++)
                    {
                        struct r_vert_t vert = {};

                        vert.pos.x = mesh->mVertices[vert_index].x;
                        vert.pos.y = mesh->mVertices[vert_index].y;
                        vert.pos.z = mesh->mVertices[vert_index].z;

                        vert.normal.x = mesh->mNormals[vert_index].x;
                        vert.normal.y = mesh->mNormals[vert_index].y;
                        vert.normal.z = mesh->mNormals[vert_index].z;
                        vert.normal.w = 0.0;

                        vert.tangent.x = mesh->mTangents[vert_index].x;
                        vert.tangent.y = mesh->mTangents[vert_index].y;
                        vert.tangent.z = mesh->mTangents[vert_index].z;

                        vert.tex_coords.x = mesh->mTextureCoords[0][vert_index].x;
                        vert.tex_coords.y = mesh->mTextureCoords[0][vert_index].y;

                        if(vertices->min.x > vert.pos.x) vertices->min.x = vert.pos.x;
                        if(vertices->min.y > vert.pos.y) vertices->min.y = vert.pos.y;
                        if(vertices->min.z > vert.pos.z) vertices->min.z = vert.pos.z;

                        if(vertices->max.x < vert.pos.x) vertices->max.x = vert.pos.x;
                        if(vertices->max.y < vert.pos.y) vertices->max.y = vert.pos.y;
                        if(vertices->max.z < vert.pos.z) vertices->max.z = vert.pos.z;

                        ds_append_data(&vert_section, sizeof(struct r_vert_t), &vert);
                    }

                    for(uint32_t face_index = 0; face_index < mesh->mNumFaces; face_index++)
                    {
                        struct aiFace *face = mesh->mFaces + face_index;

                        for(uint32_t index = 0; index < face->mNumIndices; index++)
                        {
                            face->mIndices[index] += vertices->vert_count;
                        }

                        batch_record.count += face->mNumIndices;
                        indices->index_count += face->mNumIndices;
                        ds_append_data(&index_section, sizeof(uint32_t) * face->mNumIndices, face->mIndices);

//                        for(uint32_t index = 0; index < face->mNumIndices; index++)
//                        {
//                            face->mIndices[index] -= vertices->vert_count;
//                        }
                    }

                    vertices->vert_count += mesh->mNumVertices;

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






