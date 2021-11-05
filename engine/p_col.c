#include "p_col.h"

extern struct ds_slist_t p_col_shapes[P_COL_SHAPE_TYPE_LAST];

struct p_col_shape_t *p_CreateCollisionShape(uint32_t type)
{
    uint32_t index = ds_slist_add_element(&p_col_shapes[type], NULL);
    struct p_col_shape_t *shape = ds_slist_get_element(&p_col_shapes[type], index);

    shape->index = index;
    shape->type = type;

    return shape;
}

void p_DestroyCollisionShape(struct p_col_shape_t *shape)
{
    if(shape && shape->index != 0xffffffff)
    {
        if(shape->type == P_COL_SHAPE_TYPE_TMESH)
        {
            struct p_tmesh_shape_t *mesh_shape = (struct p_tmesh_shape_t *)shape;
            ds_dbvt_destroy(&mesh_shape->dbvh);
            mem_Free(mesh_shape->tris);
        }

        ds_slist_remove_element(&p_col_shapes[shape->type], shape->index);
        shape->index = 0xffffffff;
    }
}

struct p_capsule_shape_t *p_CreateCapsuleCollisionShape(float radius, float height)
{
    struct p_capsule_shape_t *capsule_shape = (struct p_capsule_shape_t *)p_CreateCollisionShape(P_COL_SHAPE_TYPE_CAPSULE);

    capsule_shape->height = height;
    capsule_shape->radius = radius;

    return capsule_shape;
}

struct p_tmesh_shape_t *p_CreateTriMeshCollisionShape(vec3_t *verts, uint32_t *indices, uint32_t vert_count)
{
    struct p_tmesh_shape_t *mesh_shape = NULL;

    if(verts && vert_count)
    {
        mesh_shape = (struct p_tmesh_shape_t *)p_CreateCollisionShape(P_COL_SHAPE_TYPE_TMESH);
        mesh_shape->tris = mem_Calloc(vert_count, sizeof(struct p_col_tri_t));
        mesh_shape->tri_count = 0;
        mesh_shape->dbvh = ds_dbvt_create(0);

        if(indices)
        {
            for(uint32_t vert_index = 0; vert_index < vert_count; )
            {
                struct p_col_tri_t *col_tri = mesh_shape->tris + mesh_shape->tri_count;
                mesh_shape->tri_count++;

                col_tri->verts[0] = verts[indices[vert_index]];
                vert_index++;
                col_tri->verts[1] = verts[indices[vert_index]];
                vert_index++;
                col_tri->verts[2] = verts[indices[vert_index]];
                vert_index++;

                vec3_t edge0;
                vec3_t edge1;

                vec3_t_sub(&edge0, &col_tri->verts[1], &col_tri->verts[0]);
                vec3_t_sub(&edge1, &col_tri->verts[2], &col_tri->verts[1]);
                vec3_t_cross(&col_tri->normal, &edge1, &edge0);
                vec3_t_normalize(&col_tri->normal, &col_tri->normal);
            }
        }
//        else
//        {
//            for(uint32_t vert_index = 0; vert_index < vert_count; )
//            {
//                struct p_col_tri_t *col_tri = mesh_shape->tris + mesh_shape->tri_count;
//                mesh_shape->tri_count++;
//
//                col_tri->verts[0] = verts[vert_index];
//                vert_index++;
//                col_tri->verts[1] = verts[vert_index];
//                vert_index++;
//                col_tri->verts[2] = verts[vert_index];
//                vert_index++;
//
//                vec3_t edge0;
//                vec3_t edge1;
//
//                vec3_t_sub(&edge0, &col_tri->verts[1], &col_tri->verts[0]);
//                vec3_t_sub(&edge1, &col_tri->verts[2], &col_tri->verts[1]);
//                vec3_t_cross(&col_tri->normal, &edge1, &edge0);
//                vec3_t_normalize(&col_tri->normal, &col_tri->normal);
//            }
//        }

        for(uint32_t tri_index = 0; tri_index < mesh_shape->tri_count; tri_index++)
        {
            struct p_col_tri_t *col_tri = mesh_shape->tris + tri_index;
            uint32_t node_index = ds_dbvt_alloc_node(&mesh_shape->dbvh);
            struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&mesh_shape->dbvh, node_index);

            node->contents = col_tri;
            node->max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            node->min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);

            for(uint32_t vert_index = 0; vert_index < 3; vert_index++)
            {
                if(node->max.x < col_tri->verts[vert_index].x) node->max.x = col_tri->verts[vert_index].x;
                if(node->max.y < col_tri->verts[vert_index].y) node->max.y = col_tri->verts[vert_index].y;
                if(node->max.z < col_tri->verts[vert_index].z) node->max.z = col_tri->verts[vert_index].z;

                if(node->min.x > col_tri->verts[vert_index].x) node->min.x = col_tri->verts[vert_index].x;
                if(node->min.y > col_tri->verts[vert_index].y) node->min.y = col_tri->verts[vert_index].y;
                if(node->min.z > col_tri->verts[vert_index].z) node->min.z = col_tri->verts[vert_index].z;
            }

            if(node->max.x - node->min.x < 0.001)
            {
                node->max.x += 0.0005;
                node->min.x -= 0.0005;
            }

            if(node->max.y - node->min.y < 0.001)
            {
                node->max.y += 0.0005;
                node->min.y -= 0.0005;
            }

            if(node->max.z - node->min.z < 0.001)
            {
                node->max.z += 0.0005;
                node->min.z -= 0.0005;
            }

            ds_dbvt_insert_node(&mesh_shape->dbvh, node_index);
            struct ds_dbvn_t *root = ds_dbvt_get_node_pointer(&mesh_shape->dbvh, mesh_shape->dbvh.root);
        }
    }

    return mesh_shape;
}

void p_CollisionShapeBounds(struct p_col_shape_t *shape, vec3_t *max, vec3_t *min)
{
    switch(shape->type)
    {
        case P_COL_SHAPE_TYPE_CAPSULE:
        {
            struct p_capsule_shape_t *capsule_shape = (struct p_capsule_shape_t *)shape;
            *max = vec3_t_c(capsule_shape->radius, capsule_shape->height * 0.5, capsule_shape->radius);
            *min = vec3_t_c(-capsule_shape->radius, -capsule_shape->height * 0.5, -capsule_shape->radius);
        }
        break;

        case P_COL_SHAPE_TYPE_TMESH:
        {
            struct p_tmesh_shape_t *mesh_shape = (struct p_tmesh_shape_t *)shape;
            struct ds_dbvn_t *root_node = ds_dbvt_get_node_pointer(&mesh_shape->dbvh, mesh_shape->dbvh.root);
            *max = root_node->max;
            *min = root_node->min;
        }
        break;
    }
}
