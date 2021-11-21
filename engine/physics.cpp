#include <float.h>
#include "physics.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_matrix.h"
//#include "dstuff/ds_dbvt.h"
#include "r_draw.h"

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseInterface.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btDefaultMotionState.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"


btDiscreteDynamicsWorld *p_dynamics_world;
btDefaultCollisionConfiguration *p_collision_configuration;
btCollisionDispatcher *p_collision_dispatcher;
btSequentialImpulseConstraintSolver *p_constraint_solver;
btBroadphaseInterface *p_broadphase;
struct ds_slist_t p_colliders[P_COLLIDER_TYPE_LAST];
struct ds_slist_t p_shape_defs;
uint32_t p_col_shape_count;


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void p_Init()
{
    p_colliders[P_COLLIDER_TYPE_DYNAMIC] = ds_slist_create(sizeof(struct p_dynamic_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_STATIC] = ds_slist_create(sizeof(struct p_static_collider_t), 512);
    p_shape_defs = ds_slist_create(sizeof(struct p_shape_def_t), 512);

    p_broadphase = new btDbvtBroadphase();
    p_collision_configuration = new btDefaultCollisionConfiguration();
    p_collision_dispatcher = new btCollisionDispatcher(p_collision_configuration);
    p_constraint_solver = new btSequentialImpulseConstraintSolver();
    p_dynamics_world = new btDiscreteDynamicsWorld(p_collision_dispatcher, p_broadphase, p_constraint_solver, p_collision_configuration);
}

void p_Shutdown()
{

}

struct p_shape_def_t *p_AllocShapeDef()
{
    uint32_t index;
    struct p_shape_def_t *shape_def;

    index = ds_slist_add_element(&p_shape_defs, NULL);
    shape_def = (struct p_shape_def_t *)ds_slist_get_element(&p_shape_defs, index);

    shape_def->index = index;
    shape_def->type = P_COL_SHAPE_TYPE_LAST;
    shape_def->next = NULL;
//    shape_def->prev = NULL;

    return shape_def;
}

void p_FreeShapeDef(struct p_shape_def_t *shape_def)
{
    if(shape_def && shape_def->index != 0xffffffff)
    {
        ds_slist_remove_element(&p_shape_defs, shape_def->index);
        shape_def->index = 0xffffffff;
    }
}

void *p_CreateCollisionShape(struct p_shape_def_t *shape_def)
{
    btCollisionShape *shape = NULL;

    switch(shape_def->type)
    {
        case P_COL_SHAPE_TYPE_BOX:
        {
            btVector3 size(shape_def->box.size.x, shape_def->box.size.y, shape_def->box.size.z);
            shape = new btBoxShape(size);
        }
        break;

        case P_COL_SHAPE_TYPE_CAPSULE:
            shape = new btCapsuleShape(shape_def->capsule.radius, shape_def->capsule.radius);
        break;
    }

    p_col_shape_count++;

    return shape;
}

void *p_CreateColliderCollisionShape(struct p_col_def_t *collider_def)
{
    btCollisionShape *shape;

    if(collider_def->shape_count > 1)
    {
        btCompoundShape *compound_shape = new btCompoundShape(true, collider_def->shape_count);
        struct p_shape_def_t *shape_def = collider_def->shape;
        while(shape_def)
        {
            btCollisionShape *child_shape = (btCollisionShape *)p_CreateCollisionShape(shape_def);
            btTransform child_transform;
            child_transform.setOrigin(btVector3(shape_def->position.x, shape_def->position.y, shape_def->position.z));
            child_transform.setBasis(btMatrix3x3(shape_def->orientation.x0, shape_def->orientation.x1, shape_def->orientation.x2,
                                                 shape_def->orientation.y0, shape_def->orientation.y1, shape_def->orientation.y2,
                                                 shape_def->orientation.z0, shape_def->orientation.z1, shape_def->orientation.z2));

            compound_shape->addChildShape(child_transform, child_shape);
            shape_def = shape_def->next;
        }

        shape = compound_shape;
    }
    else
    {
        shape = (btCollisionShape *)p_CreateCollisionShape(collider_def->shape);
    }

    return shape;
}

void p_DestroyCollisionShape(void *collision_shape)
{
    if(collision_shape)
    {
        btCollisionShape *shape = (btCollisionShape *)collision_shape;

        if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
        {
            btCompoundShape *compound_shape = (btCompoundShape *)shape;
            uint32_t shape_count = compound_shape->getNumChildShapes();
            for(uint32_t shape_index = 0; shape_index < shape_count; shape_index++)
            {
                btCollisionShape *child_shape = compound_shape->getChildShape(shape_index);
                delete child_shape;
            }

            p_col_shape_count -= shape_count;
        }

        delete shape;
    }
}

//struct p_dynamic_collider_t *p_CreateDynamicCollider(vec3_t *position, mat3_t *orientation, struct p_col_shape_def_t *shape_def, float mass)
//{
//    struct p_dynamic_collider_t *collider;
//    collider = (struct p_dynamic_collider_t *)p_CreateCollider(P_COLLIDER_TYPE_DYNAMIC, position, orientation, shape_def, mass);
//
//    if(mass == 0.0)
//    {
//        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
//        rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
//        rigid_body->setActivationState(DISABLE_DEACTIVATION);
//    }
//
//    return collider;
//}
//
//struct p_static_collider_t *p_CreateStaticCollider(vec3_t *position, mat3_t *orientation, struct p_col_shape_def_t *shape_def)
//{
//    struct p_static_collider_t *collider;
//    collider = (struct p_static_collider_t *)p_CreateCollider(P_COLLIDER_TYPE_DYNAMIC, position, orientation, shape_def, 0.0);
//    return collider;
//}

struct p_collider_t *p_CreateCollider(struct p_col_def_t *col_def, vec3_t *position, mat3_t *orientation)
{
    uint32_t collider_index;
    struct p_collider_t *collider;

    collider_index = ds_slist_add_element(&p_colliders[col_def->type], NULL);
    collider = (struct p_collider_t *)ds_slist_get_element(&p_colliders[col_def->type], collider_index);

    collider->index = collider_index;
    collider->type = col_def->type;
    collider->position = *position;
    collider->orientation = *orientation;

    btCollisionShape *collision_shape = (btCollisionShape *)p_CreateColliderCollisionShape(col_def);
    btTransform collider_transform;

    collider_transform.setOrigin(btVector3(position->x, position->y, position->z));
    collider_transform.setBasis(btMatrix3x3(orientation->x0, orientation->x1, orientation->x2,
                                            orientation->y0, orientation->y1, orientation->y2,
                                            orientation->z0, orientation->z1, orientation->z2));

    btDefaultMotionState *motion_state = new btDefaultMotionState(collider_transform);
    btVector3 inertia_tensor = btVector3(0, 0, 0);
    collision_shape->calculateLocalInertia(col_def->mass, inertia_tensor);
    btRigidBody::btRigidBodyConstructionInfo info(col_def->mass, motion_state, collision_shape, inertia_tensor);
    btRigidBody *rigid_body = new btRigidBody(info);
    collider->rigid_body = rigid_body;

    p_dynamics_world->addRigidBody(rigid_body);

    if(col_def->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
        dynamic_collider->mass = col_def->mass;

        if(col_def->mass == 0.0)
        {
            rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            rigid_body->setActivationState(DISABLE_DEACTIVATION);
        }
    }

    return collider;
}

void p_SetColliderPosition(struct p_collider_t *collider, vec3_t *position)
{
    p_SetColliderTransform(collider, position, &collider->orientation);
}

void p_SetColliderOrientation(struct p_collider_t *collider, mat3_t *orientation)
{
    p_SetColliderTransform(collider, &collider->position, orientation);
}

void p_SetColliderTransform(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation)
{
    if(collider && collider->index != 0xffffffff)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        const btTransform &src_transform = rigid_body->getWorldTransform();
        btTransform dst_transform = src_transform;
        dst_transform.setOrigin(btVector3(position->x, position->y, position->z));
        dst_transform.setBasis(btMatrix3x3(orientation->x0, orientation->x1, orientation->x2,
                                           orientation->y0, orientation->y1, orientation->y2,
                                           orientation->z0, orientation->z1, orientation->z2));
        rigid_body->setWorldTransform(dst_transform);
    }
}

void p_DestroyCollider(struct p_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff)
    {
        ds_slist_remove_element(&p_colliders[collider->type], collider->index);
        collider->index = 0xffffffff;

        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        p_dynamics_world->removeRigidBody(rigid_body);
        btCollisionShape *collision_shape = rigid_body->getCollisionShape();
        p_DestroyCollisionShape(collision_shape);
    }
}

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index)
{
    struct p_collider_t *collider;

    collider = (struct p_collider_t *)ds_slist_get_element(&p_colliders[type], index);

    if(collider && collider->index == 0xffffffff)
    {
        collider = NULL;
    }

    return collider;
}

struct p_dynamic_collider_t *p_GetDynamicCollider(uint32_t index)
{
    return(struct p_dynamic_collider_t *)p_GetCollider(P_COLLIDER_TYPE_DYNAMIC, index);
}

void p_DisplaceCollider(struct p_collider_t *collider, vec3_t *disp)
{
//    struct p_col_plane_t *planes;
//
//    switch(collider->type)
//    {
//        case P_COLLIDER_TYPE_STATIC:
//        {
//            vec3_t_add(&collider->position, &collider->position, disp);
//            struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&p_main_dbvt, collider->node_index);
//            vec3_t_add(&node->max, &node->max, disp);
//            vec3_t_add(&node->min, &node->min, disp);
//            uint32_t node_index = ds_dbvt_nodes_smallest_volume(&p_main_dbvt, collider->node_index);
//            ds_dbvt_pair_nodes(&p_main_dbvt, collider->node_index, node_index);
//
//            planes = ds_slist_get_element(&p_col_planes, collider->planes_index);
//
//            for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
//            {
//                struct p_col_plane_t *plane = planes + plane_index;
//                vec3_t_add(&plane->point, &plane->point, disp);
//            }
//        }
//        break;
//
//        case P_COLLIDER_TYPE_MOVABLE:
//        {
//            struct p_movable_collider_t *movable_collider = (struct p_movable_collider_t *)collider;
//            vec3_t_add(&movable_collider->disp, &movable_collider->disp, disp);
//        }
//        break;
//    }
}

void p_RotateCollider(struct p_collider_t *collider, mat3_t *rot)
{
//    mat3_t_mul(&collider->orientation, &collider->orientation, rot);
//    p_UpdateColliderNode(collider);
}

//void p_RotateColliderX(struct p_collider_t *collider, float angle)
//{
//    mat3_t_rotate_x(&collider->orientation, angle);
//    p_UpdateColliderNode(collider);
////    p_GenColPlanes(collider);
//}
//
//void p_RotateColliderY(struct p_collider_t *collider, float angle)
//{
//    mat3_t_rotate_y(&collider->orientation, angle);
//    p_UpdateColliderNode(collider);
////    p_GenColPlanes(collider);
//}
//
//void p_RotateColliderZ(struct p_collider_t *collider, float angle)
//{
//    mat3_t_rotate_z(&collider->orientation, angle);
//    p_UpdateColliderNode(collider);
////    p_GenColPlanes(collider);
//}

//uint32_t p_BoxIntersect(vec3_t *box_a0, vec3_t *box_a1, vec3_t *box_b0, vec3_t *box_b1)
//{
////    return box_a0->x < box_b1->x && box_a1->x > box_b0->x &&
////           box_a0->y < box_b1->y && box_a1->y > box_b0->y &&
////           box_a0->z < box_b1->z && box_a1->z > box_b0->z;
//}

void p_UpdateColliders(float delta_time)
{
    p_dynamics_world->stepSimulation(delta_time, 10);

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_DYNAMIC].cursor; collider_index++)
    {
        struct p_dynamic_collider_t *collider = p_GetDynamicCollider(collider_index);

        if(collider)
        {
            btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
            const btMotionState *motion_state = rigid_body->getMotionState();
            btTransform body_transform;
            mat4_t collider_transform;
            motion_state->getWorldTransform(body_transform);
            body_transform.getOpenGLMatrix((btScalar *)collider_transform.comps);

            collider->orientation.rows[0] = collider_transform.rows[0].xyz;
            collider->orientation.rows[1] = collider_transform.rows[1].xyz;
            collider->orientation.rows[2] = collider_transform.rows[2].xyz;
            collider->position = collider_transform.rows[3].xyz;
        }
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus


