#include <float.h>
#include "phys.h"
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
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btDefaultMotionState.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btIDebugDraw.h"

class p_DebugDraw : public btIDebugDraw
{
    int debug_mode;

    virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor)
    {
        vec3_t start = vec3_t_c(from[0], from[1], from[2]);
        vec3_t end = vec3_t_c(to[0], to[1], to[2]);
        vec4_t color = vec4_t_c(fromColor[0], fromColor[1], fromColor[2], 1.0);
        r_i_DrawLine(&start, &end, &color, 1.0);
    }

    virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
    {

    }

    virtual void reportErrorWarning(const char* warningString)
    {

    }

    virtual void draw3dText(const btVector3& location, const char* textString)
    {

    }

    virtual void setDebugMode(int debugMode)
    {
        debug_mode = debugMode;
    }

    virtual int getDebugMode() const
    {
        return debug_mode;
    }
};


btDiscreteDynamicsWorld *p_dynamics_world;
btDefaultCollisionConfiguration *p_collision_configuration;
btCollisionDispatcher *p_collision_dispatcher;
btSequentialImpulseConstraintSolver *p_constraint_solver;
btBroadphaseInterface *p_broadphase;
struct ds_slist_t p_colliders[P_COLLIDER_TYPE_LAST];
struct ds_slist_t p_shape_defs;
p_DebugDraw *p_debug_drawer;

extern struct r_renderer_state_t r_renderer_state;


char *p_col_shape_names[P_COL_SHAPE_TYPE_LAST] =
{
    "Capsule",
    "Cylinder",
    "Triangle mesh",
    "Indexed triangle mesh",
    "Box",
};


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void p_Init()
{
    p_colliders[P_COLLIDER_TYPE_DYNAMIC] = ds_slist_create(sizeof(struct p_dynamic_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_STATIC] = ds_slist_create(sizeof(struct p_static_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_CHARACTER] = ds_slist_create(sizeof(struct p_character_collider_t), 512);
//    p_colliders[P_COLLIDER_TYPE_CHILD] = ds_slist_create(sizeof(struct p_child_collider_t), 512);
    p_shape_defs = ds_slist_create(sizeof(struct p_shape_def_t), 512);

    p_broadphase = new btDbvtBroadphase();
    p_collision_configuration = new btDefaultCollisionConfiguration();
    p_collision_dispatcher = new btCollisionDispatcher(p_collision_configuration);
    p_constraint_solver = new btSequentialImpulseConstraintSolver();
    p_dynamics_world = new btDiscreteDynamicsWorld(p_collision_dispatcher, p_broadphase, p_constraint_solver, p_collision_configuration);
    p_debug_drawer = new p_DebugDraw();
    p_dynamics_world->setDebugDrawer(p_debug_drawer);
    p_dynamics_world->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawFrames);
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

        case P_COL_SHAPE_TYPE_ITRI_MESH:
        {
            btTriangleIndexVertexArray *indexed_mesh;
            indexed_mesh = new btTriangleIndexVertexArray(shape_def->itri_mesh.index_count / 3, (int *)shape_def->itri_mesh.indices,
                                                          sizeof(uint32_t) * 3, shape_def->itri_mesh.vert_count,
                                                          shape_def->itri_mesh.verts->comps, sizeof(vec3_t));

            shape = new btBvhTriangleMeshShape(indexed_mesh, false);
        }
        break;
    }

//    p_col_shape_count++;

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
//
//            p_col_shape_count -= shape_count;
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

//    if(collider->type != P_COLLIDER_TYPE_CHILD)
//    {
//        struct p_child_collider_t *child_collider = (struct p_child_collider_t *)collider;
//        child_collider->collision_shape = collision_shape;
//        child_collider->next = NULL;
//        child_collider->next = NULL;
//        child_collider->parent = NULL;
//    }
//    else
//    {
//        btTransform collider_transform;
//        collider_transform.setOrigin(btVector3(position->x, position->y, position->z));
//        collider_transform.setBasis(btMatrix3x3(orientation->x0, orientation->x1, orientation->x2,
//                                                orientation->y0, orientation->y1, orientation->z1,
//                                                orientation->x2, orientation->y2, orientation->z2));

        btVector3 origin = btVector3(position->x, position->y, position->z);
        btMatrix3x3 basis;
        basis[0] = btVector3(orientation->rows[0].x, orientation->rows[1].x, orientation->rows[2].x);
        basis[1] = btVector3(orientation->rows[0].y, orientation->rows[1].y, orientation->rows[2].y);
        basis[2] = btVector3(orientation->rows[0].z, orientation->rows[1].z, orientation->rows[2].z);

        btTransform collider_transform;
        collider_transform.setOrigin(origin);
        collider_transform.setBasis(basis);

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
//    }

    return collider;
}

struct p_character_collider_t *p_CreateCharacterCollider(vec3_t *position, float step_height, float height, float radius, float crouch_height)
{
    struct p_shape_def_t shape_def = {};
    struct p_col_def_t col_def = {};

    shape_def.type = P_COL_SHAPE_TYPE_CAPSULE;
    shape_def.capsule.height = (height - 2.0 * radius) - step_height;
    shape_def.capsule.radius = radius;

    col_def.mass = 1.0;
    col_def.shape = &shape_def;
    col_def.shape_count = 1;
    col_def.type = P_COLLIDER_TYPE_CHARACTER;

    mat3_t orientation = mat3_t_c_id();
    struct p_character_collider_t *collider = (struct p_character_collider_t *)p_CreateCollider(&col_def, position, &orientation);
    collider->crouch_height = crouch_height;
    collider->height = height;
    collider->step_height = step_height;
    collider->radius = radius;

    btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
    rigid_body->setAngularFactor(0.0);
    rigid_body->setCcdMotionThreshold(0.0);
    rigid_body->setActivationState(DISABLE_DEACTIVATION);
//    rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT)

    return collider;
}

void p_UpdateColliderTransform(struct p_collider_t *collider)
{
    btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
//    const btMotionState *motion_state = rigid_body->getMotionState();
//    btTransform motion_state_transform;
//    motion_state->getWorldTransform(motion_state_transform);
//    mat4_t collider_transform;
    const btTransform &body_transform = rigid_body->getCenterOfMassTransform();
    btMatrix3x3 basis = body_transform.getBasis();
    const btVector3 &origin = body_transform.getOrigin();
//    motion_state->getWorldTransform(body_transform);
//    body_transform.getOpenGLMatrix((btScalar *)collider_transform.comps);
    basis = basis.transpose();
    collider->orientation.rows[0] = vec3_t_c(basis[0][0], basis[0][1], basis[0][2]);
    collider->orientation.rows[1] = vec3_t_c(basis[1][0], basis[1][1], basis[1][2]);
    collider->orientation.rows[2] = vec3_t_c(basis[2][0], basis[2][1], basis[2][2]);
    collider->position = vec3_t_c(origin[0], origin[1], origin[2]);

//    printf("p_UpdateColliderTransform:\n[%f %f %f]\n[%f %f %f]\n[%f %f %f]\n\n", basis[0][0], basis[0][1], basis[0][2],
//                                                                                 basis[1][0], basis[1][1], basis[1][2],
//                                                                                 basis[2][0], basis[2][1], basis[2][2]);
//    collider->position = collider_transform.rows[3].xyz;
}

void p_SetColliderPosition(struct p_collider_t *collider, vec3_t *position)
{
    p_SetColliderTransform(collider, position, &collider->orientation);
}

void p_SetColliderOrientation(struct p_collider_t *collider, mat3_t *orientation)
{
//    printf("p_SetColliderOrientation:\n[%f %f %f]\n[%f %f %f]\n[%f %f %f]\n\n", orientation->rows[0].x, orientation->rows[0].y, orientation->rows[0].z,
//                                                     orientation->rows[1].x, orientation->rows[1].y, orientation->rows[1].z,
//                                                     orientation->rows[2].x, orientation->rows[2].y, orientation->rows[2].z);
    p_SetColliderTransform(collider, &collider->position, orientation);
}

void p_SetColliderTransform(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation)
{
    if(collider && collider->index != 0xffffffff)
    {

        collider->orientation = *orientation;
        collider->position = *position;

        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
//        const btTransform &src_transform = rigid_body->getWorldTransform();
        btVector3 origin = btVector3(position->x, position->y, position->z);
        btMatrix3x3 basis;
        basis[0] = btVector3(orientation->rows[0].x, orientation->rows[1].x, orientation->rows[2].x);
        basis[1] = btVector3(orientation->rows[0].y, orientation->rows[1].y, orientation->rows[2].y);
        basis[2] = btVector3(orientation->rows[0].z, orientation->rows[1].z, orientation->rows[2].z);
//         = btMatrix3x3(orientation->x0, orientation->y0, orientation->z0,
//                                        orientation->x1, orientation->y1, orientation->z1,
//                                        orientation->x2, orientation->y2, orientation->z2);
//        basis = basis.transpose();
        btTransform dst_transform;
        dst_transform.setBasis(basis);
        dst_transform.setOrigin(origin);


//        btTransform dst_transform = src_transform;
//        dst_transform.setOrigin(btVector3(position->x, position->y, position->z));
//        dst_transform.setBasis(btMatrix3x3(orientation->x0, orientation->y0, orientation->z0,
//                                           orientation->x1, orientation->y1, orientation->z1,
//                                           orientation->x2, orientation->y2, orientation->z2));
//        dst_transform.getBasis().transpose();
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

void p_TranslateCollider(struct p_collider_t *collider, vec3_t *disp)
{
    if(collider && collider->index != 0xffffffff && collider->type)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->translate(btVector3(disp->x, disp->y, disp->z));
        rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        rigid_body->setAngularVelocity(btVector3(0, 0, 0));

        vec3_t_add(&collider->position, &collider->position, disp);
    }
}

void p_RotateCollider(struct p_collider_t *collider, mat3_t *rot)
{
    if(collider && collider->index != 0xffffffff && collider->type)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        btTransform transform = rigid_body->getCenterOfMassTransform();
        btMatrix3x3 &basis = transform.getBasis();
        mat3_t *orientation = &collider->orientation;
        orientation->rows[0] = vec3_t_c(basis[0][0], basis[0][1], basis[0][2]);
        orientation->rows[1] = vec3_t_c(basis[1][0], basis[1][1], basis[1][2]);
        orientation->rows[2] = vec3_t_c(basis[2][0], basis[2][1], basis[2][2]);
        mat3_t transpose_rotation;
        mat3_t_transpose(&transpose_rotation, rot);
        mat3_t_mul(orientation, &transpose_rotation, orientation);
        basis[0] = btVector3(orientation->rows[0].x, orientation->rows[0].y, orientation->rows[0].z);
        basis[1] = btVector3(orientation->rows[1].x, orientation->rows[1].y, orientation->rows[1].z);
        basis[2] = btVector3(orientation->rows[2].x, orientation->rows[2].y, orientation->rows[2].z);
//        basis = basis.transpose();
        transform.setBasis(basis);
        rigid_body->setCenterOfMassTransform(transform);
        rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        rigid_body->setAngularVelocity(btVector3(0, 0, 0));
    }
}

//void p_SetColliderOrientation(struct p_collider_t *collider, mat3_t *orientation)
//{
//    if(collider && collider->index != 0xffffffff && collider->type)
//    {
//        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
//        btTransform transform = rigid_body->getCenterOfMassTransform();
//        btMatrix3x3 &basis = transform.getBasis();
//        collider->orientation = *orientation;
//        basis[0] = btVector3(orientation->rows[0].x, orientation->rows[0].y, orientation->rows[0].z);
//        basis[1] = btVector3(orientation->rows[1].x, orientation->rows[1].y, orientation->rows[1].z);
//        basis[2] = btVector3(orientation->rows[2].x, orientation->rows[2].y, orientation->rows[2].z);
//        transform.setBasis(basis);
//        rigid_body->setCenterOfMassTransform(transform);
//        rigid_body->setLinearVelocity(btVector3(0, 0, 0));
//        rigid_body->setAngularVelocity(btVector3(0, 0, 0));
//    }
//}

void p_MoveCharacterCollider(struct p_character_collider_t *collider, vec3_t *direction)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_CHARACTER)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->applyCentralImpulse(btVector3(direction->x, direction->y, direction->z));
    }
}

void p_JumpCharacterCollider(struct p_character_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_CHARACTER)
    {
        if(collider->flags & P_CHARACTER_COLLIDER_FLAG_ON_GROUND)
        {
            collider->flags &= ~P_CHARACTER_COLLIDER_FLAG_ON_GROUND;
            collider->flags |= P_CHARACTER_COLLIDER_FLAG_JUMPED;
            btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
            rigid_body->clearGravity();
            btVector3 linear_velocity = rigid_body->getLinearVelocity();
            linear_velocity[1] = 5.0;
            rigid_body->setLinearVelocity(linear_velocity);
        }
    }
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
    if(delta_time)
    {
        p_dynamics_world->stepSimulation(delta_time, 10);
    }

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_DYNAMIC].cursor; collider_index++)
    {
        struct p_collider_t *collider = p_GetCollider(P_COLLIDER_TYPE_DYNAMIC, collider_index);

        if(collider)
        {
            p_UpdateColliderTransform(collider);
        }
    }

    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetModelMatrix(NULL);
    r_i_SetShader(NULL);

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_CHARACTER].cursor; collider_index++)
    {
        struct p_character_collider_t *collider = (struct p_character_collider_t *)p_GetCollider(P_COLLIDER_TYPE_CHARACTER, collider_index);
        p_UpdateColliderTransform((struct p_collider_t *)collider);

        btVector3 from(collider->position.x, collider->position.y - (collider->height * 0.5 - collider->radius), collider->position.z);
        btVector3 to = from;
        to[1] -= collider->step_height * 2.0;
        btCollisionWorld::ClosestRayResultCallback raycast_result(from, to);
        p_dynamics_world->rayTest(from, to, raycast_result);
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        btVector3 linear_velocity = rigid_body->getLinearVelocity();
        collider->flags &= ~P_CHARACTER_COLLIDER_FLAG_ON_GROUND;

        if(raycast_result.m_closestHitFraction < 1.0)
        {
            uint32_t jump_flag = collider->flags & P_CHARACTER_COLLIDER_FLAG_JUMPED;
            if((raycast_result.m_closestHitFraction < 0.5 && jump_flag) || !(jump_flag))
            {
                float adjust = (0.5 - raycast_result.m_closestHitFraction);
                linear_velocity[1] = collider->step_height * adjust * 70.0;
                rigid_body->clearGravity();
                collider->flags |= P_CHARACTER_COLLIDER_FLAG_ON_GROUND;
                collider->flags &= ~P_CHARACTER_COLLIDER_FLAG_JUMPED;
            }
        }

        linear_velocity[0] *= 0.95;
        linear_velocity[2] *= 0.95;
        rigid_body->setLinearVelocity(linear_velocity);
    }

    if(r_renderer_state.draw_colliders)
    {
        p_dynamics_world->debugDrawWorld();
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus


