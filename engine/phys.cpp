#include <float.h>
#include "phys.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_matrix.h"
#include "r_draw.h"
#include "r_main.h"
#include "log.h"

#include "newton/include/dgNewton/Newton.h"
#include "newton/include/dgNewton/NewtonStdAfx.h"
#include "newton/include/dCustomJoints/dCustomHinge.h"
#include "newton/include/dCustomJoints/dCustomSlider.h"
#include "newton/include/dCustomJoints/dCustomCorkscrew.h"
#include "newton/include/dCustomJoints/dCustomBallAndSocket.h"

//#include "newton/include/dgPhysics/dgCollision.h"

struct ds_slist_t p_colliders[P_COLLIDER_TYPE_LAST];
struct ds_list_t p_active_colliders;
struct ds_slist_t p_shape_defs;
struct ds_slist_t p_constraints;
NewtonWorld *p_physics_world;
vec3_t p_gravity = vec3_t_c(0.0, -9.8, 0.0);
uint32_t p_physics_frozen;

extern struct r_renderer_state_t r_renderer_state;
extern struct r_shader_t *r_immediate_shader;
extern mat4_t r_view_projection_matrix;



char *p_col_type_names[P_COLLIDER_TYPE_LAST] =
{
    "Static",
    "Kinematic",
    "Dynamic",
    "Trigger",
    "Character"
};




char *p_col_shape_names[P_COL_SHAPE_TYPE_LAST] =
{
    "Capsule",
    "Cylinder",
    "Sphere",
    "Box",
    "Triangle mesh",
    "Indexed triangle mesh",
};

char *p_constraint_names[P_CONSTRAINT_TYPE_LAST] =
{
    "Hinge",
    "Double hinge",
    "Corkscrew",
    "Slider",
    "Ball and socket"
};


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


void p_SetForceTorqueCallback(const NewtonBody *body, float timestep, int thread_index)
{
    float mass;
    float ixx;
    float iyy;
    float izz;
    struct p_collider_t *collider = (struct p_collider_t *)NewtonBodyGetUserData(body);

    NewtonBodyGetMass(body, &mass, &ixx, &iyy, &izz);
    vec3_t force = vec3_t_c(0.0, p_gravity.y * mass, 0.0);

    if(collider->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
        vec3_t_add(&force, &force, &dynamic_collider->accumulated_force);
        dynamic_collider->accumulated_force = vec3_t_c(0.0, 0.0, 0.0);
        dynamic_collider->linear_velocity = vec3_t_c(0.0, 0.0, 0.0);
        dynamic_collider->active_index = 0xffffffff;
    }

    NewtonBodyAddForce(body, force.comps);
}

void p_GetTransformCallback(const NewtonBody *body, const float * const matrix, int thread_index)
{
    struct p_collider_t *collider = (struct p_collider_t *)NewtonBodyGetUserData(body);

    collider->orientation.rows[0] = vec3_t_c(matrix[0], matrix[1], matrix[2]);
    collider->orientation.rows[1] = vec3_t_c(matrix[4], matrix[5], matrix[6]);
    collider->orientation.rows[2] = vec3_t_c(matrix[8], matrix[9], matrix[10]);
    collider->position = vec3_t_c(matrix[12], matrix[13], matrix[14]);

    if(collider->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
        dynamic_collider->active_index = ds_list_add_element(&p_active_colliders, &collider);
        NewtonBodyGetVelocity(body, dynamic_collider->linear_velocity.comps);
    }
}

struct p_closest_hit_raycast_data_t
{
    NewtonBody *ignore;
    NewtonBody *body;
    NewtonCollision *shape;
    vec3_t point;
    vec3_t normal;
    float time;
};

unsigned int p_ClosestHitRaycastPrefilter(const NewtonBody * const body, const NewtonCollision * const shape, void * const data)
{
    return 1;
}

float p_ClosestHitRaycastFilter(const NewtonBody * const body, const NewtonCollision * const shape, const float * const point, const float * const normal, dLong collision_id, void * const data, float time)
{
    struct p_closest_hit_raycast_data_t *hit_data = (struct p_closest_hit_raycast_data_t *)data;

    if(time < hit_data->time&& hit_data->ignore != body)
    {
        hit_data->body = (NewtonBody *)body;
        hit_data->shape = (NewtonCollision *)shape;
        hit_data->normal = vec3_t_c(normal[0], normal[1], normal[2]);
        hit_data->point = vec3_t_c(point[0], point[1], point[2]);
        hit_data->time = time;
    }

    return time;
}

void p_Init()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Initializing physics...");
    p_colliders[P_COLLIDER_TYPE_DYNAMIC] = ds_slist_create(sizeof(struct p_dynamic_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_STATIC] = ds_slist_create(sizeof(struct p_static_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_CHARACTER] = ds_slist_create(sizeof(struct p_character_collider_t), 512);
    p_active_colliders = ds_list_create(sizeof(struct p_collider_t *), 512);
//    p_colliders[P_COLLIDER_TYPE_CHILD] = ds_slist_create(sizeof(struct p_child_collider_t), 512);
    p_shape_defs = ds_slist_create(sizeof(struct p_shape_def_t), 512);
    p_constraints = ds_slist_create(sizeof(struct p_constraint_t), 512);
    p_physics_world = NewtonCreate();
    NewtonSetThreadsCount(p_physics_world, 4);

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Physics initialized!");
}

void p_Shutdown()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Shutting down physics...");
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Physics shut down!");
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

void *p_CreateSimpleCollisionShape(struct p_shape_def_t *shape_def)
{
    NewtonCollision *shape = NULL;

    switch(shape_def->type)
    {
        case P_COL_SHAPE_TYPE_BOX:
        {
            vec3_t size = shape_def->box.size;
            shape = NewtonCreateBox(p_physics_world, size.x * 2.0, size.y * 2.0, size.z * 2.0, 0, NULL);
        }
        break;

        case P_COL_SHAPE_TYPE_CAPSULE:
        {
            mat4_t offset = mat4_t_c_id();
            mat4_t_rotate_z(&offset, 0.5);
            float radius = shape_def->capsule.radius;
            float height = shape_def->capsule.height;
            shape = NewtonCreateCapsule(p_physics_world, radius, radius, height, 0, (const float *)offset.comps);
        }
        break;

        case P_COL_SHAPE_TYPE_CYLINDER:
            shape = NewtonCreateCylinder(p_physics_world, shape_def->cylinder.radius, shape_def->cylinder.radius, shape_def->cylinder.height, 0, NULL);
        break;

        case P_COL_SHAPE_TYPE_ITRI_MESH:
        {
            shape = NewtonCreateTreeCollision(p_physics_world, 0);
            NewtonTreeCollisionBeginBuild(shape);

            for(uint32_t index = 0; index < shape_def->itri_mesh.index_count;)
            {
                vec3_t verts[3];

                verts[0] = shape_def->itri_mesh.verts[shape_def->itri_mesh.indices[index]];
                index++;
                verts[1] = shape_def->itri_mesh.verts[shape_def->itri_mesh.indices[index]];
                index++;
                verts[2] = shape_def->itri_mesh.verts[shape_def->itri_mesh.indices[index]];
                index++;

                NewtonTreeCollisionAddFace(shape, 3, verts[0].comps, sizeof(vec3_t), 0);
            }

            NewtonTreeCollisionEndBuild(shape, 1);
        }
        break;
    }

    return shape;
}

void *p_CreateCollisionShape(struct p_col_def_t *collider_def, vec3_t *shape_scale)
{
//    btCollisionShape *shape;

    NewtonCollision *shape;

    if(collider_def->type == P_COLLIDER_TYPE_CHARACTER)
    {
        float height = collider_def->character.height;
        float radius = collider_def->character.radius;
        float step_height = collider_def->character.step_height;

        struct p_shape_def_t shape_def = {};
        shape_def.type = P_COL_SHAPE_TYPE_CAPSULE;
        shape_def.capsule.height = (height - 2.0 * radius) - step_height;
//        shape_def.capsule.height = height;
        shape_def.capsule.radius = radius;

        shape = (NewtonCollision *)p_CreateSimpleCollisionShape(&shape_def);
        NewtonCollisionSetScale(shape, 1, 1, 1);
    }
    else
    {
        if(collider_def->passive.shape_count > 1)
        {
            NewtonCollision *compound_shape = NewtonCreateCompoundCollision(p_physics_world, 0);
            NewtonCompoundCollisionBeginAddRemove(compound_shape);

            struct p_shape_def_t *shape_def = collider_def->passive.shape;
            while(shape_def)
            {
                mat4_t transform;
                mat4_t_comp(&transform, &shape_def->orientation, &shape_def->position);

                NewtonCollision *child_shape = (NewtonCollision *)p_CreateSimpleCollisionShape(shape_def);
                void *collision_node = NewtonCompoundCollisionAddSubCollision(compound_shape, child_shape);
                NewtonCompoundCollisionSetSubCollisionMatrix(compound_shape, collision_node, (const float *)transform.comps);
                shape_def = shape_def->next;
            }

            NewtonCompoundCollisionEndAddRemove(compound_shape);
            NewtonCollisionSetScale(compound_shape, shape_scale->x, shape_scale->y, shape_scale->z);
            shape = compound_shape;
        }
        else
        {
            mat4_t transform;
            mat4_t_comp(&transform, &collider_def->passive.shape->orientation, &collider_def->passive.shape->position);
            shape = (NewtonCollision *)p_CreateSimpleCollisionShape(collider_def->passive.shape);
            dgCollisionInstance *instance = (dgCollisionInstance *)shape;
            NewtonCollisionSetMatrix(shape, (const float *)transform.comps);
            instance->SetGlobalScale(dgVector(shape_scale->x, shape_scale->y, shape_scale->z, 1.0));
        }
    }

    return shape;
}

//void p_DestroyCollisionShape(void *collision_shape)
//{
//    if(collision_shape)
//    {
////        btCollisionShape *shape = (btCollisionShape *)collision_shape;
////
////        if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
////        {
////            btCompoundShape *compound_shape = (btCompoundShape *)shape;
////            uint32_t shape_count = compound_shape->getNumChildShapes();
////            for(uint32_t shape_index = 0; shape_index < shape_count; shape_index++)
////            {
////                btCollisionShape *child_shape = compound_shape->getChildShape(shape_index);
////                delete child_shape;
////            }
//////
//////            p_col_shape_count -= shape_count;
////        }
//
////        delete shape;
//    }
//}

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

struct p_collider_t *p_CreateCollider(struct p_col_def_t *col_def, vec3_t *scale, vec3_t *position, mat3_t *orientation)
{
    uint32_t collider_index;
    struct p_collider_t *collider;
    float mass;

    collider_index = ds_slist_add_element(&p_colliders[col_def->type], NULL);
    collider = (struct p_collider_t *)ds_slist_get_element(&p_colliders[col_def->type], collider_index);

    collider->index = collider_index;
    collider->type = col_def->type;
    collider->position = *position;
    collider->scale = *scale;
    collider->constraints = NULL;

    NewtonCollision *scratch_collision_shape = (NewtonCollision *)p_CreateCollisionShape(col_def, scale);
    NewtonCollisionSetScale(scratch_collision_shape, scale->x, scale->y, scale->z);

    mat4_t transform;
    mat4_t_comp(&transform, orientation, position);

    NewtonBody *rigid_body = NULL;
    vec3_t inertia_tensor;
    vec3_t shape_origin = vec3_t_c(0.0, 0.0, 0.0);

    if(col_def->type == P_COLLIDER_TYPE_CHARACTER)
    {
        collider->orientation = mat3_t_c_id();
        mass = 1.0;
    }
    else
    {
        collider->orientation = *orientation;
        mass = col_def->passive.mass;

    }

    rigid_body = NewtonCreateDynamicBody(p_physics_world, scratch_collision_shape, (const float *)transform.comps);
    NewtonDestroyCollision(scratch_collision_shape);
    NewtonCollision *collision_shape = NewtonBodyGetCollision(rigid_body);
//    NewtonCollisionSetScale(collision_shape, 1.0, 1.0, 1.0);

    NewtonConvexCollisionCalculateInertialMatrix(collision_shape, inertia_tensor.comps, shape_origin.comps);

    NewtonBodySetMatrix(rigid_body, (const float *)transform.comps);
    NewtonBodySetCollidable(rigid_body, 1);
    NewtonBodySetMassProperties(rigid_body, mass, collision_shape);

    NewtonBodySetMassMatrix(rigid_body, mass, inertia_tensor.x * mass, inertia_tensor.y * mass, inertia_tensor.z * mass);
    NewtonBodySetCentreOfMass(rigid_body, (const float *)shape_origin.comps);
    NewtonBodySetForceAndTorqueCallback(rigid_body, p_SetForceTorqueCallback);
    NewtonBodySetTransformCallback(rigid_body, p_GetTransformCallback);
    NewtonBodySetDestructorCallback(rigid_body, NULL);
    NewtonBodySetUserData(rigid_body, collider);

    collider->rigid_body = rigid_body;

    switch(col_def->type)
    {
        case P_COLLIDER_TYPE_DYNAMIC:
        {
            struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
            dynamic_collider->mass = mass;
            dynamic_collider->center_offset = shape_origin;
//            if(mass == 0.0)
//            {
//                rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
//                rigid_body->setActivationState(DISABLE_DEACTIVATION);
//            }
        }
        break;

        case P_COLLIDER_TYPE_CHARACTER:
        {
            struct p_character_collider_t *character_collider = (struct p_character_collider_t *)collider;

            character_collider->height = col_def->character.height;
            character_collider->crouch_height = col_def->character.crouch_height;
            character_collider->step_height = col_def->character.step_height;
            character_collider->radius = col_def->character.radius;
//            NewtonCollisionSetMode(collision_shape, 1);

//            NewtonBodySetContinuousCollisionMode(rigid_body, 1);

//            vec3_t damping = {-1.0, -1.0, -1.0};
//            NewtonBodySetAngularDamping(rigid_body, damping.comps);
//            NewtonBodySetSleepState(rigid_body, 0);

//            rigid_body = (btRigidBody *)collider->rigid_body;
//            rigid_body->setAngularFactor(0.0);
//            rigid_body->setCcdMotionThreshold(0.0);
//            rigid_body->setActivationState(DISABLE_DEACTIVATION);
        }
        break;
    }

    return collider;
}

//struct p_character_collider_t *p_CreateCharacterCollider(vec3_t *position, float step_height, float height, float radius, float crouch_height)
//{
//    struct p_shape_def_t shape_def = {};
//    struct p_col_def_t col_def = {};
//
//    shape_def.type = P_COL_SHAPE_TYPE_CAPSULE;
//    shape_def.capsule.height = (height - 2.0 * radius) - step_height;
//    shape_def.capsule.radius = radius;
//
//    col_def.mass = 1.0;
//    col_def.shape = &shape_def;
//    col_def.shape_count = 1;
//    col_def.type = P_COLLIDER_TYPE_CHARACTER;
//
//    mat3_t orientation = mat3_t_c_id();
//    struct p_character_collider_t *collider = (struct p_character_collider_t *)p_CreateCollider(&col_def, position, &orientation);
//    collider->crouch_height = crouch_height;
//    collider->height = height;
//    collider->step_height = step_height;
//    collider->radius = radius;
//
//    btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
//    rigid_body->setAngularFactor(0.0);
//    rigid_body->setCcdMotionThreshold(0.0);
//    rigid_body->setActivationState(DISABLE_DEACTIVATION);
//
//    return collider;
//}

//void p_UpdateColliderTransform(struct p_collider_t *collider)
//{
////    btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
////
////    const btTransform &body_transform = rigid_body->getCenterOfMassTransform();
////    btMatrix3x3 basis = body_transform.getBasis();
////    const btVector3 &origin = body_transform.getOrigin();
////
////    basis = basis.transpose();
////    collider->orientation.rows[0] = vec3_t_c(basis[0][0], basis[0][1], basis[0][2]);
////    collider->orientation.rows[1] = vec3_t_c(basis[1][0], basis[1][1], basis[1][2]);
////    collider->orientation.rows[2] = vec3_t_c(basis[2][0], basis[2][1], basis[2][2]);
////    collider->position = vec3_t_c(origin[0], origin[1], origin[2]);
//}

/*
    src_collider is the collider passed to p_SetColliderTransform, and is only meaningful if there are constraints involved.
    Colliders linked to src_collider through constraints are treated like child objects of src_constraint, and are transformed
    accordingly.
*/

void p_SetColliderTransformRecursive(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation, vec3_t *scale, struct p_collider_t *src_collider)
{
    if(collider->constraints)
    {
        struct p_constraint_t *constraint = collider->constraints;

        while(constraint)
        {
            uint32_t collider_side = constraint->colliders[1].collider == collider;
            constraint->colliders[collider_side].transform_set = 1;

            if(!constraint->colliders[!collider_side].transform_set)
            {
                struct p_collider_t *linked_collider = constraint->colliders[!collider_side].collider;
                constraint->colliders[!collider_side].transform_set = 1;
                p_SetColliderTransformRecursive(linked_collider, position, orientation, scale, src_collider);
            }
            else
            {
                constraint->colliders[0].transform_set = 0;
                constraint->colliders[1].transform_set = 0;
            }

            constraint = constraint->colliders[collider_side].next;
        }
    }

    if(src_collider == collider)
    {
        collider->orientation = *orientation;
        collider->position = *position;
        collider->scale = *scale;
    }
    else
    {
        mat3_t linked_orientation;
        vec3_t linked_translation;

        vec3_t_sub(&linked_translation, position, &src_collider->position);

        /* transform this collider's rotation into the src_collider local space. This gives
        us the relative rotation between this collider and src_collider */
        mat3_t_transpose(&linked_orientation, &src_collider->orientation);
        /* apply the incoming rotation matrix to this relative rotation */
        mat3_t_mul(&linked_orientation, &linked_orientation, orientation);

        /* transform this collider's translation into the src_collider local space */
        vec3_t_sub(&collider->position, &collider->position, &src_collider->position);
        mat3_t_vec3_t_mul(&collider->position, &collider->position, &linked_orientation);

        mat3_t_mul(&collider->orientation, &collider->orientation, &linked_orientation);
        vec3_t_add(&collider->position, &collider->position, &src_collider->position);
        vec3_t_add(&collider->position, &collider->position, &linked_translation);
    }

    mat4_t transform;
    mat4_t_comp(&transform, &collider->orientation, &collider->position);

    NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
    NewtonBodySetMatrix(rigid_body, (const float *)transform.comps);
    NewtonCollision *collision_shape = NewtonBodyGetCollision(rigid_body);
    NewtonCollisionSetScale(collision_shape, collider->scale.x, collider->scale.y, collider->scale.z);
}

void p_SetColliderTransform(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation, vec3_t *scale)
{
    p_SetColliderTransformRecursive(collider, position, orientation, scale, collider);
}

void p_TransformCollider(struct p_collider_t *collider, vec3_t *translation, mat3_t *rotation, vec3_t *scale)
{
    if(collider && collider->index != 0xffffffff)
    {
        vec3_t position;
        mat3_t orientation;
        vec3_t_add(&position, translation, &collider->position);
        mat3_t_mul(&orientation, &collider->orientation, rotation);
        p_SetColliderTransform(collider, &position, &orientation, scale);
    }
}

void p_SetColliderVelocity(struct p_collider_t *collider, vec3_t *linear_velocity, vec3_t *angular_velocity)
{
    if(collider)
    {
        NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;

        if(linear_velocity)
        {
            NewtonBodySetVelocity(rigid_body, linear_velocity->comps);
        }

        if(angular_velocity)
        {
            NewtonBodySetOmega(rigid_body, angular_velocity->comps);
        }
    }
}

//void p_SetColliderMass(struct p_collider_t *collider, float mass)
//{
//    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_DYNAMIC)
//    {
////        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
////        btCollisionShape *collision_shape = rigid_body->getCollisionShape();
////        btVector3 inertia_tensor;
////        collision_shape->calculateLocalInertia(mass, inertia_tensor);
////        rigid_body->setMassProps(mass, inertia_tensor);
//    }
//}

//void p_DisableColliderGravity(struct p_collider_t *collider)
//{
//    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_DYNAMIC)
//    {
////        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
////        rigid_body->setGravity(btVector3(0.0, 0.0, 0.0));
//    }
//}

void p_ApplyForce(struct p_collider_t *collider, vec3_t *force, vec3_t *relative_pos)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
        vec3_t_add(&dynamic_collider->accumulated_force, &dynamic_collider->accumulated_force, force);
    }
}

void p_ApplyImpulse(struct p_collider_t *collider, vec3_t *impulse, vec3_t *relative_pos, float delta_time)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
        vec3_t center_offset = dynamic_collider->center_offset;
        vec3_t collider_center = collider->position;

        mat3_t_vec3_t_mul(&center_offset, &center_offset, &collider->orientation);
        vec3_t_add(&collider_center, &collider_center, &center_offset);

        NewtonBody *body = (NewtonBody *)collider->rigid_body;
        NewtonBodyAddImpulse(body, impulse->comps, collider_center.comps, delta_time);
    }
}

void p_DestroyCollider(struct p_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff)
    {
        ds_slist_remove_element(&p_colliders[collider->type], collider->index);
        collider->index = 0xffffffff;

        struct p_constraint_t *constraint = collider->constraints;

        while(constraint)
        {
            uint32_t collider_side = constraint->colliders[1].collider == collider;
            struct p_constraint_t *next_constraint = constraint->colliders[collider_side].next;
            p_DestroyConstraint(constraint);
            constraint = next_constraint;
        }

        NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
        NewtonDestroyBody(rigid_body);
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

struct ds_list_t *p_GetActiveColliders()
{
    return &p_active_colliders;
}

//struct p_dynamic_collider_t *p_GetDynamicCollider(uint32_t index)
//{
//    return(struct p_dynamic_collider_t *)p_GetCollider(P_COLLIDER_TYPE_DYNAMIC, index);
//}

//void p_FreezeCollider(struct p_collider_t *collider)
//{
////    if(collider && collider->index != 0xffffffff)
////    {
////        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
////        rigid_body->setLinearFactor(btVector3(0, 0, 0));
////        rigid_body->setAngularFactor(btVector3(0, 0, 0));
////    }
//}
//
//void p_UnfreezeCollider(struct p_collider_t *collider)
//{
////    if(collider && collider->index != 0xffffffff)
////    {
////        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
////        rigid_body->setLinearFactor(btVector3(1, 1, 1));
////        rigid_body->setAngularFactor(btVector3(1, 1, 1));
////    }
//}


struct p_constraint_t *p_CreateConstraint(struct p_constraint_def_t *def, struct p_collider_t *collider_a, struct p_collider_t *collider_b)
{
    uint32_t index = ds_slist_add_element(&p_constraints, NULL);
    struct p_constraint_t *constraint = (struct p_constraint_t *)ds_slist_get_element(&p_constraints, index);

    constraint->index = index;
    constraint->type = P_CONSTRAINT_TYPE_LAST;

    constraint->colliders[0].next = collider_a->constraints;
    constraint->colliders[0].prev = NULL;
    constraint->colliders[0].collider = collider_a;
    if(collider_a->constraints)
    {
        uint32_t collider_side = collider_a->constraints->colliders[1].collider == collider_a;
        collider_a->constraints->colliders[collider_side].prev = constraint;
    }
    collider_a->constraints = constraint;

    constraint->colliders[1].next = collider_b->constraints;
    constraint->colliders[1].prev = NULL;
    constraint->colliders[1].collider = collider_b;
    if(collider_b->constraints)
    {
        uint32_t collider_side = collider_b->constraints->colliders[1].collider == collider_b;
        collider_b->constraints->colliders[collider_side].prev = constraint;
    }
    collider_b->constraints = constraint;
//    constraint->type = constraint_def->type;

    NewtonBody *rigid_body_a = (NewtonBody *)collider_a->rigid_body;
    NewtonBody *rigid_body_b = (NewtonBody *)collider_b->rigid_body;

    vec3_t pivot_a = def->pivot_a;
    mat3_t axis_a = def->axis;
    mat4_t transform_a;

    mat3_t_mul(&axis_a, &axis_a, &collider_a->orientation);
    mat3_t_vec3_t_mul(&pivot_a, &pivot_a, &collider_a->orientation);
    vec3_t_add(&pivot_a, &pivot_a, &collider_a->position);
    mat4_t_comp(&transform_a, &axis_a, &pivot_a);

    vec3_t pivot_b = def->pivot_b;
    mat3_t axis_b = def->axis;
    mat4_t transform_b;

    mat3_t_mul(&axis_b, &axis_b, &collider_b->orientation);
    mat3_t_vec3_t_mul(&pivot_b, &pivot_b, &collider_b->orientation);
    vec3_t_add(&pivot_b, &pivot_b, &collider_b->position);
    mat4_t_comp(&transform_b, &axis_b, &pivot_b);

    dMatrix pivot_axis_a((const float *)transform_a.comps);
    dMatrix pivot_axis_b((const float *)transform_b.comps);

    switch(def->type)
    {
        case P_CONSTRAINT_TYPE_HINGE:
        {
            dCustomHinge *hinge_constraint = new dCustomHinge(pivot_axis_a, pivot_axis_b, rigid_body_a, rigid_body_b);
            hinge_constraint->SetLimits(def->hinge.min_angle, def->hinge.max_angle);
            hinge_constraint->EnableLimits(def->hinge.use_limits);
            constraint->constraint = hinge_constraint;
        }
        break;

        case P_CONSTRAINT_TYPE_SLIDER:
        {
            dCustomSlider *slider_constraint = new dCustomSlider(pivot_axis_a, pivot_axis_b, rigid_body_a, rigid_body_b);
            slider_constraint->SetLimits(def->slider.min_dist, def->slider.max_dist);
            slider_constraint->EnableLimits(def->slider.use_limits);
            slider_constraint->SetFriction(def->slider.friction);
            constraint->constraint = slider_constraint;
        }
        break;

        case P_CONSTRAINT_TYPE_CORKSCREW:
        {
            dCustomCorkScrew *corkscrew_constraint = new dCustomCorkScrew(pivot_axis_a, pivot_axis_b, rigid_body_a, rigid_body_b);
            corkscrew_constraint->SetLimits(def->corkscrew.min_dist, def->corkscrew.max_dist);
            corkscrew_constraint->EnableLimits(def->corkscrew.use_lin_limits);
            corkscrew_constraint->SetFriction(def->corkscrew.lin_friction);
            corkscrew_constraint->SetAngularLimits(def->corkscrew.min_angle, def->corkscrew.max_angle);
            corkscrew_constraint->EnableAngularLimits(def->corkscrew.use_ang_limits);
            corkscrew_constraint->SetAngularFriction(def->corkscrew.ang_friction);
            constraint->constraint = corkscrew_constraint;
        }
        break;

        case P_CONSTRAINT_TYPE_BALL_SOCKET:
        {
            dCustomBallAndSocket *ball_and_socket_constraint = new dCustomBallAndSocket(pivot_axis_a, pivot_axis_b, rigid_body_a, rigid_body_b);
            ball_and_socket_constraint->SetConeLimits(def->ball_socket.cone_limit);
            ball_and_socket_constraint->SetConeFriction(def->ball_socket.cone_friction);
            ball_and_socket_constraint->EnableCone(def->ball_socket.use_cone);
            ball_and_socket_constraint->SetTwistLimits(def->ball_socket.min_twist, def->ball_socket.max_twist);
            ball_and_socket_constraint->SetTwistFriction(def->ball_socket.twist_friction);
            ball_and_socket_constraint->EnableTwist(def->ball_socket.use_twist);
            constraint->constraint = ball_and_socket_constraint;
        }
        break;
    }

    constraint->def = *def;

    return constraint;
}

void p_DestroyConstraint(struct p_constraint_t *constraint)
{
    if(constraint && constraint->index != 0xffffffff)
    {
        dCustomJoint *joint = (dCustomJoint *)constraint->constraint;
        delete joint;

        for(uint32_t index = 0; index < 2; index++)
        {
            struct p_col_constraint_t *col_constraint = constraint->colliders + index;

            if(col_constraint->prev)
            {
                uint32_t collider_side = col_constraint->prev->colliders[1].collider == col_constraint->collider;
                col_constraint->prev->colliders[collider_side].next = col_constraint->next;
            }
            else
            {
                col_constraint->collider->constraints = col_constraint->next;
            }

            col_constraint->prev = NULL;

            if(col_constraint->next)
            {
                uint32_t collider_side = col_constraint->next->colliders[1].collider == col_constraint->collider;
                col_constraint->next->colliders[collider_side].prev = col_constraint->prev;
            }
        }

        ds_slist_remove_element(&p_constraints, constraint->index);
        constraint->index = 0xffffffff;
    }
}

void p_SetConstraintLinearLimits(struct p_constraint_t *constraint, float min, float max)
{
    if(constraint && constraint->index != 0xffffffff)
    {
        switch(constraint->type)
        {
            case P_CONSTRAINT_TYPE_SLIDER:
            {
                dCustomSlider *slider = (dCustomSlider *)constraint->constraint;
                slider->SetLimits(min, max);
            }
            break;

            case P_CONSTRAINT_TYPE_CORKSCREW:
            {
                dCustomCorkScrew *corkscrew = (dCustomCorkScrew *)constraint->constraint;
                corkscrew->SetLimits(min, max);
            }
            break;
        }
    }
}

void p_SetConstraintAngularLimits(struct p_constraint_t *constraint, float min, float max)
{
    if(constraint && constraint->index != 0xffffffff)
    {
        switch(constraint->type)
        {
            case P_CONSTRAINT_TYPE_HINGE:
            {
                dCustomHinge *hinge = (dCustomHinge *)constraint->constraint;
                hinge->SetLimits(min, max);
            }
            break;

            case P_CONSTRAINT_TYPE_CORKSCREW:
            {
                dCustomCorkScrew *corkscrew = (dCustomCorkScrew *)constraint->constraint;
                corkscrew->SetAngularLimits(min, max);
            }
            break;
        }
    }
}

void p_MoveCharacterCollider(struct p_character_collider_t *collider, vec3_t *direction)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_CHARACTER)
    {
        NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
        vec3_t linear_velocity;
        NewtonBodyGetVelocity(rigid_body, linear_velocity.comps);
        linear_velocity.x += direction->x;
        linear_velocity.z += direction->z;
        NewtonBodySetVelocity(rigid_body, linear_velocity.comps);
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
            NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
            vec3_t linear_velocity;
            NewtonBodyGetVelocity(rigid_body, linear_velocity.comps);
            linear_velocity.y = 5.0;
            NewtonBodySetVelocity(rigid_body, linear_velocity.comps);
        }
    }
}

struct p_collider_t *p_Raycast(vec3_t *from, vec3_t *to, float *time, struct p_collider_t *ignore)
{
    struct p_closest_hit_raycast_data_t hit_data = {};

    hit_data.time = 1.0;
    if(ignore)
    {
        hit_data.ignore = (NewtonBody *)ignore->rigid_body;
    }
    NewtonWorldRayCast(p_physics_world, from->comps, to->comps, p_ClosestHitRaycastFilter, &hit_data, p_ClosestHitRaycastPrefilter, 0);

    if(hit_data.time < 1.0)
    {
        *time = hit_data.time;
        NewtonBody *rigid_body = hit_data.body;
        return (struct p_collider_t *)NewtonBodyGetUserData(rigid_body);
    }

    return NULL;
}


void p_StepPhysics(float delta_time)
{
    p_active_colliders.cursor = 0;

    NewtonUpdateAsync(p_physics_world, delta_time);

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_CHARACTER].cursor; collider_index++)
    {
        struct p_character_collider_t *collider = (struct p_character_collider_t *)p_GetCollider(P_COLLIDER_TYPE_CHARACTER, collider_index);

        if(collider)
        {
            vec3_t from = collider->position;
            from.y -= (collider->height * 0.5);

            vec3_t to = from;
            to.y -= collider->step_height * 2.0;

            float time = 1.0;
            p_Raycast(&from, &to, &time, (struct p_collider_t *)collider);
            NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
            NewtonBodyIntegrateVelocity(rigid_body, delta_time);

            vec3_t linear_velocity;
            NewtonBodyGetVelocity(rigid_body, linear_velocity.comps);
            collider->flags &= ~P_CHARACTER_COLLIDER_FLAG_ON_GROUND;

            if(time < 1.0)
            {
                uint32_t jump_flag = collider->flags & P_CHARACTER_COLLIDER_FLAG_JUMPED;
                if((time < 0.5 && jump_flag) || !(jump_flag))
                {
                    float adjust = (0.5 - time);
                    linear_velocity.y = collider->step_height * adjust * 70.0;
                    collider->flags |= P_CHARACTER_COLLIDER_FLAG_ON_GROUND;
                    collider->flags &= ~P_CHARACTER_COLLIDER_FLAG_JUMPED;
                }
            }

            mat4_t transform = mat4_t_c_id();
            transform.rows[3].xyz = collider->position;
            collider->orientation = mat3_t_c_id();
            NewtonBodySetMatrix(rigid_body, (const float *)transform.comps);

            linear_velocity.x *= 0.95;
            linear_velocity.z *= 0.95;
            NewtonBodySetVelocity(rigid_body, linear_velocity.comps);

            vec3_t angular_velocity = vec3_t_c(0.0, 0.0, 0.0);
            NewtonBodySetOmega(rigid_body, angular_velocity.comps);
        }
    }

//    if(r_renderer_state.draw_colliders)
//    {
//        p_DebugDrawPhysics();
//    }
}

void p_DrawCollisionShape(mat4_t *base_transform, NewtonCollision *shape)
{
    NewtonCollisionInfoRecord collision_info;
    vec3_t scale;
    NewtonCollisionGetInfo(shape, &collision_info);
    NewtonCollisionGetScale(shape, &scale.x, &scale.y, &scale.z);


    mat4_t shape_transform;
    mat4_t scale_transform = mat4_t_c_id();
    mat4_t model_transform;
    memcpy(shape_transform.comps, collision_info.m_offsetMatrix, sizeof(mat4_t));
    mat4_t_mul(&model_transform, &shape_transform, base_transform);

    scale_transform.rows[0].x = scale.x;
    scale_transform.rows[1].y = scale.y;
    scale_transform.rows[2].z = scale.z;
    mat4_t_mul(&model_transform, &scale_transform, &model_transform);

    if(collision_info.m_collisionType == SERIALIZE_ID_COMPOUND)
    {
        void *collision_node = NewtonCompoundCollisionGetFirstNode(shape);

        while(collision_node)
        {
            NewtonCollision *child_shape = NewtonCompoundCollisionGetCollisionFromNode(shape, collision_node);
            p_DrawCollisionShape(&model_transform, child_shape);
            collision_node = NewtonCompoundCollisionGetNextNode(shape, collision_node);
        }
    }
    else
    {
        mat4_t_mul(&model_transform, &model_transform, &r_view_projection_matrix);
        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_transform);

        switch(collision_info.m_collisionType)
        {
            case SERIALIZE_ID_BOX:
            {
                vec3_t size;
                vec4_t color = vec4_t_c(1.0, 1.0, 1.0, 1.0);
                size.x = collision_info.m_box.m_x * 0.5;
                size.y = collision_info.m_box.m_y * 0.5;
                size.z = collision_info.m_box.m_z * 0.5;
                r_DrawBox(&size, &color);
            }
            break;

            case SERIALIZE_ID_TREE:
            {

            }
            break;
        }
    }
}

void p_DrawConstraint(struct p_constraint_t *constraint)
{
    dCustomJoint *joint = (dCustomJoint *)constraint->constraint;
    dMatrix matrix0;
    dMatrix matrix1;
    mat4_t transform0;
    mat4_t transform1;

    joint->CalculateGlobalMatrix(matrix0, matrix1);
    NewtonBody *body0 = joint->GetBody0();
    NewtonBody *body1 = joint->GetBody1();

    NewtonBodyGetMatrix(body0, (float *)transform0.comps);
    NewtonBodyGetMatrix(body1, (float *)transform1.comps);

    switch(constraint->type)
    {
        case P_CONSTRAINT_TYPE_HINGE:
        {
            dCustomHinge *hinge_constraint = (dCustomHinge *)joint;
            vec3_t hinge_pivot = vec3_t_c(matrix0[3][0], matrix0[3][1], matrix0[3][2]);
            vec3_t hinge_axis = vec3_t_c(matrix0[0][0], matrix0[0][1], matrix0[0][2]);

            vec3_t point0;
            vec3_t_fmadd(&point0, &hinge_pivot, &hinge_axis, 0.3);
            vec3_t point1;
            vec3_t_fmadd(&point1, &hinge_pivot, &hinge_axis, -0.3);

            vec4_t color = vec4_t_c(0.7, 0.7, 1.0, 0.0);
            r_DrawLine(&point0, &point1, &color);
            r_DrawLine(&transform0.rows[3].xyz, &hinge_pivot, &color);

            hinge_pivot = vec3_t_c(matrix1[3][0], matrix1[3][1], matrix1[3][2]);
            hinge_axis = vec3_t_c(matrix1[0][0], matrix1[0][1], matrix1[0][2]);

            vec3_t_fmadd(&point0, &hinge_pivot, &hinge_axis, 0.3);
            vec3_t_fmadd(&point1, &hinge_pivot, &hinge_axis, -0.3);

            r_DrawLine(&point0, &point1, &color);
            r_DrawLine(&transform1.rows[3].xyz, &hinge_pivot, &color);
        }
        break;

        case P_CONSTRAINT_TYPE_SLIDER:
        {
            dCustomSlider *slider_constraint = (dCustomSlider *)joint;
            vec3_t slider_center = vec3_t_c(matrix0[3][0], matrix0[3][1], matrix0[3][2]);
            vec3_t slider_axis = vec3_t_c(matrix0[0][0], matrix0[0][1], matrix0[0][2]);
            vec3_t axis1 = vec3_t_c(matrix0[1][0], matrix0[1][1], matrix0[1][2]);
            vec3_t axis2 = vec3_t_c(matrix0[2][0], matrix0[2][1], matrix0[2][2]);

            vec3_t point0;
            vec3_t point1;

            vec3_t_fmadd(&point0, &slider_center, &slider_axis, -constraint->slider.min_dist);
            vec3_t_fmadd(&point1, &slider_center, &slider_axis, -constraint->slider.max_dist);

            vec4_t color = vec4_t_c(0.7, 0.7, 1.0, 0.0);
            r_DrawLine(&point0, &point1, &color);

            slider_center = vec3_t_c(matrix1[3][0], matrix1[3][1], matrix1[3][2]);
            slider_axis = vec3_t_c(matrix1[0][0], matrix1[0][1], matrix1[0][2]);

            vec3_t_fmadd(&point0, &slider_center, &slider_axis, -constraint->slider.min_dist);
            vec3_t_fmadd(&point1, &slider_center, &slider_axis, -constraint->slider.max_dist);

            r_DrawLine(&point0, &point1, &color);
        }
        break;
    }
}

void p_DebugDrawPhysics()
{
//    r_i_SetViewProjectionMatrix(NULL);
//    r_i_SetModelMatrix(NULL);
//    r_i_SetShader(NULL);

//    r_BindShader(r_immediate_shader);

/*     r_SetImmediateModeDefaults();
    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
    glLineWidth(1.0); */
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);

    /* for(uint32_t constraint_index = 0; constraint_index < p_constraints.cursor; constraint_index++)
    {
        struct p_constraint_t *constraint = (struct p_constraint_t *)ds_slist_get_element(&p_constraints, constraint_index);
        if(constraint && constraint->index != 0xffffffff)
        {
            p_DrawConstraint(constraint);
        }
    }

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_DYNAMIC].cursor; collider_index++)
    {
        struct p_dynamic_collider_t *collider = (struct p_dynamic_collider_t *)p_GetCollider(P_COLLIDER_TYPE_DYNAMIC, collider_index);

        if(collider)
        {
            mat4_t base_transform;
            mat4_t model_view_projection_matrix;
            mat4_t_comp(&base_transform, &collider->orientation, &collider->position);
            vec3_t origin = {};
            vec4_t axes[3] =
            {
                vec4_t_c(1.0, 0.0, 0.0, 1.0),
                vec4_t_c(0.0, 1.0, 0.0, 1.0),
                vec4_t_c(0.0, 0.0, 1.0, 1.0),
            };

            NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
            NewtonCollision *collision_shape = NewtonBodyGetCollision(rigid_body);
            p_DrawCollisionShape(&base_transform, collision_shape);
            mat4_t_mul(&base_transform, &base_transform, &r_view_projection_matrix);

            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &base_transform);
            for(uint32_t axis = 0; axis < 3; axis++)
            {
                r_DrawLine(&origin, &axes[axis].xyz, &axes[axis]);
            }
        }
    } */

//    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_STATIC].cursor; collider_index++)
//    {
//        struct p_static_collider_t *collider = (struct p_static_collider_t *)p_GetCollider(P_COLLIDER_TYPE_STATIC, collider_index);
//
//        if(collider)
//        {
//
//            mat4_t base_transform;
//            mat4_t_comp(&base_transform, &collider->orientation, &collider->position);
//
//            NewtonBody *rigid_body = (NewtonBody *)collider->rigid_body;
//            NewtonCollision *collision_shape = NewtonBodyGetCollision(rigid_body);
//            p_DrawCollisionShape(&base_transform, collision_shape);
//        }
//    }
}

#ifdef __cplusplus
}
#endif // __cplusplus


