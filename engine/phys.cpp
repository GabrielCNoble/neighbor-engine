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
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletDynamics/ConstraintSolver/btHingeConstraint.h"
#include "BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h"
#include "BulletDynamics/ConstraintSolver/btSliderConstraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btDefaultMotionState.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btIDebugDraw.h"
#include "log.h"

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
struct ds_slist_t p_constraints;
p_DebugDraw *p_debug_drawer;
uint32_t p_physics_frozen;

extern struct r_renderer_state_t r_renderer_state;


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
    "Point2Point",
    "Slider"
};


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void p_Init()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Initializing physics...");
    p_colliders[P_COLLIDER_TYPE_DYNAMIC] = ds_slist_create(sizeof(struct p_dynamic_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_STATIC] = ds_slist_create(sizeof(struct p_static_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_CHARACTER] = ds_slist_create(sizeof(struct p_character_collider_t), 512);
//    p_colliders[P_COLLIDER_TYPE_CHILD] = ds_slist_create(sizeof(struct p_child_collider_t), 512);
    p_shape_defs = ds_slist_create(sizeof(struct p_shape_def_t), 512);
    p_constraints = ds_slist_create(sizeof(struct p_constraint_t), 512);

    p_broadphase = new btDbvtBroadphase();
    p_collision_configuration = new btDefaultCollisionConfiguration();
    p_collision_dispatcher = new btCollisionDispatcher(p_collision_configuration);
    p_constraint_solver = new btSequentialImpulseConstraintSolver();
    p_dynamics_world = new btDiscreteDynamicsWorld(p_collision_dispatcher, p_broadphase, p_constraint_solver, p_collision_configuration);
    p_debug_drawer = new p_DebugDraw();
    p_dynamics_world->setDebugDrawer(p_debug_drawer);
    p_dynamics_world->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits);

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

        case P_COL_SHAPE_TYPE_CYLINDER:
            shape = new btCylinderShape(btVector3(shape_def->cylinder.radius,
                                                  shape_def->cylinder.height * 0.5,
                                                  shape_def->cylinder.radius));
        break;

        case P_COL_SHAPE_TYPE_ITRI_MESH:
        {
            btTriangleIndexVertexArray *indexed_mesh;
            indexed_mesh = new btTriangleIndexVertexArray(shape_def->itri_mesh.index_count / 3,
                                                          (int *)shape_def->itri_mesh.indices,
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

    if(collider_def->type == P_COLLIDER_TYPE_CHARACTER)
    {
        float height = collider_def->character.height;
        float radius = collider_def->character.radius;
        float step_height = collider_def->character.step_height;

        struct p_shape_def_t shape_def = {};
        shape_def.type = P_COL_SHAPE_TYPE_CAPSULE;
        shape_def.capsule.height = (height - 2.0 * radius) - step_height;
        shape_def.capsule.radius = radius;

        shape = (btCollisionShape *)p_CreateCollisionShape(&shape_def);
    }
    else
    {
        if(collider_def->passive.shape_count > 1)
        {
            btCompoundShape *compound_shape = new btCompoundShape(true, collider_def->passive.shape_count);
            struct p_shape_def_t *shape_def = collider_def->passive.shape;
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
            shape = (btCollisionShape *)p_CreateCollisionShape(collider_def->passive.shape);
        }
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
    float mass;

    collider_index = ds_slist_add_element(&p_colliders[col_def->type], NULL);
    collider = (struct p_collider_t *)ds_slist_get_element(&p_colliders[col_def->type], collider_index);

    collider->index = collider_index;
    collider->type = col_def->type;
    collider->position = *position;
    collider->constraints = NULL;

    btCollisionShape *collision_shape = (btCollisionShape *)p_CreateColliderCollisionShape(col_def);
    btVector3 origin = btVector3(position->x, position->y, position->z);
    btMatrix3x3 basis;

    if(col_def->type == P_COLLIDER_TYPE_CHARACTER)
    {
        basis.setIdentity();
        collider->orientation = mat3_t_c_id();
        mass = 1.0;
    }
    else
    {
        basis[0] = btVector3(orientation->rows[0].x, orientation->rows[1].x, orientation->rows[2].x);
        basis[1] = btVector3(orientation->rows[0].y, orientation->rows[1].y, orientation->rows[2].y);
        basis[2] = btVector3(orientation->rows[0].z, orientation->rows[1].z, orientation->rows[2].z);
        collider->orientation = *orientation;
        mass = col_def->passive.mass;
    }

    btTransform collider_transform;
    collider_transform.setOrigin(origin);
    collider_transform.setBasis(basis);

    btDefaultMotionState *motion_state = new btDefaultMotionState(collider_transform);
    btVector3 inertia_tensor = btVector3(0, 0, 0);
    collision_shape->calculateLocalInertia(mass, inertia_tensor);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motion_state, collision_shape, inertia_tensor);
    btRigidBody *rigid_body = new btRigidBody(info);
    collider->rigid_body = rigid_body;
    rigid_body->setUserPointer(collider);
    p_dynamics_world->addRigidBody(rigid_body);

    switch(col_def->type)
    {
        case P_COLLIDER_TYPE_DYNAMIC:
        {
            struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
            dynamic_collider->mass = mass;

            if(mass == 0.0)
            {
                rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                rigid_body->setActivationState(DISABLE_DEACTIVATION);
            }
        }
        break;

        case P_COLLIDER_TYPE_CHARACTER:
        {
            struct p_character_collider_t *character_collider = (struct p_character_collider_t *)collider;

            character_collider->height = col_def->character.height;
            character_collider->crouch_height = col_def->character.crouch_height;
            character_collider->step_height = col_def->character.step_height;
            character_collider->radius = col_def->character.radius;

            rigid_body = (btRigidBody *)collider->rigid_body;
            rigid_body->setAngularFactor(0.0);
            rigid_body->setCcdMotionThreshold(0.0);
            rigid_body->setActivationState(DISABLE_DEACTIVATION);
        }
        break;
    }
//
//    if(col_def->type == P_COLLIDER_TYPE_DYNAMIC)
//    {
//        struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
//        dynamic_collider->mass = mass;
//
//        if(mass == 0.0)
//        {
//            rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
//            rigid_body->setActivationState(DISABLE_DEACTIVATION);
//        }
//    }

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

void p_UpdateColliderTransform(struct p_collider_t *collider)
{
    btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;

    const btTransform &body_transform = rigid_body->getCenterOfMassTransform();
    btMatrix3x3 basis = body_transform.getBasis();
    const btVector3 &origin = body_transform.getOrigin();

    basis = basis.transpose();
    collider->orientation.rows[0] = vec3_t_c(basis[0][0], basis[0][1], basis[0][2]);
    collider->orientation.rows[1] = vec3_t_c(basis[1][0], basis[1][1], basis[1][2]);
    collider->orientation.rows[2] = vec3_t_c(basis[2][0], basis[2][1], basis[2][2]);
    collider->position = vec3_t_c(origin[0], origin[1], origin[2]);
}

void p_SetColliderTransformRecursive(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation, struct p_collider_t *src_collider)
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
                p_SetColliderTransformRecursive(linked_collider, position, orientation, src_collider);
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
    }
    else
    {
        mat3_t linked_orientation;
        vec3_t linked_translation;

        vec3_t_sub(&linked_translation, position, &src_collider->position);

        mat3_t_transpose(&linked_orientation, &src_collider->orientation);
        mat3_t_mul(&linked_orientation, &linked_orientation, orientation);
        vec3_t_sub(&collider->position, &collider->position, &src_collider->position);

        mat3_t_vec3_t_mul(&collider->position, &collider->position, &linked_orientation);
        mat3_t_mul(&collider->orientation, &collider->orientation, &linked_orientation);
        vec3_t_add(&collider->position, &collider->position, &src_collider->position);
        vec3_t_add(&collider->position, &collider->position, &linked_translation);
    }

    btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
    btVector3 origin = btVector3(collider->position.x, collider->position.y, collider->position.z);
    btMatrix3x3 basis;
    basis[0] = btVector3(collider->orientation.rows[0].x, collider->orientation.rows[1].x, collider->orientation.rows[2].x);
    basis[1] = btVector3(collider->orientation.rows[0].y, collider->orientation.rows[1].y, collider->orientation.rows[2].y);
    basis[2] = btVector3(collider->orientation.rows[0].z, collider->orientation.rows[1].z, collider->orientation.rows[2].z);

    btTransform dst_transform;
    dst_transform.setBasis(basis);
    dst_transform.setOrigin(origin);

    rigid_body->setWorldTransform(dst_transform);
}

void p_SetColliderTransform(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation)
{
    p_SetColliderTransformRecursive(collider, position, orientation, collider);
}

void p_TransformCollider(struct p_collider_t *collider, vec3_t *translation, mat3_t *rotation)
{
    if(collider && collider->index != 0xffffffff)
    {
        vec3_t position;
        mat3_t orientation;
        vec3_t_add(&position, translation, &collider->position);
        mat3_t_mul(&orientation, &collider->orientation, rotation);
        p_SetColliderTransform(collider, &position, &orientation);
    }
}

void p_SetColliderVelocity(struct p_collider_t *collider, vec3_t *linear_velocity, vec3_t *angular_velocity)
{
    if(collider)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->activate();

        if(linear_velocity)
        {
            rigid_body->setLinearVelocity(btVector3(linear_velocity->x, linear_velocity->y, linear_velocity->z));
        }

        if(angular_velocity)
        {
            rigid_body->setAngularVelocity(btVector3(angular_velocity->x, angular_velocity->y, angular_velocity->z));
        }
    }
}

void p_SetColliderMass(struct p_collider_t *collider, float mass)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        btCollisionShape *collision_shape = rigid_body->getCollisionShape();
        btVector3 inertia_tensor;
        collision_shape->calculateLocalInertia(mass, inertia_tensor);
        rigid_body->setMassProps(mass, inertia_tensor);
    }
}

void p_DisableColliderGravity(struct p_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff && collider->type == P_COLLIDER_TYPE_DYNAMIC)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->setGravity(btVector3(0.0, 0.0, 0.0));
    }
}

void p_ApplyForce(struct p_collider_t *collider, vec3_t *force, vec3_t *relative_pos)
{
    if(collider)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->activate();
        rigid_body->applyForce(btVector3(force->x, force->x, force->z),
                               btVector3(relative_pos->x, relative_pos->y, relative_pos->z));
    }
}

void p_ApplyImpulse(struct p_collider_t *collider, vec3_t *impulse, vec3_t *relative_pos)
{
    if(collider)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->activate();
        rigid_body->applyImpulse(btVector3(impulse->x, impulse->x, impulse->z),
                               btVector3(relative_pos->x, relative_pos->y, relative_pos->z));
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

void p_FreezeCollider(struct p_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->setLinearFactor(btVector3(0, 0, 0));
        rigid_body->setAngularFactor(btVector3(0, 0, 0));
    }
}

void p_UnfreezeCollider(struct p_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff)
    {
        btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
        rigid_body->setLinearFactor(btVector3(1, 1, 1));
        rigid_body->setAngularFactor(btVector3(1, 1, 1));
    }
}


struct p_constraint_t *p_CreateConstraint(struct p_constraint_def_t *constraint_def, struct p_collider_t *collider_a, struct p_collider_t *collider_b)
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
    constraint->type = constraint_def->type;

    mat3_t a_to_b_transform = collider_b->orientation;
    mat3_t_transpose(&a_to_b_transform, &a_to_b_transform);
    mat3_t_mul(&a_to_b_transform, &collider_a->orientation, &a_to_b_transform);
    btRigidBody *rigid_body_a = (btRigidBody *)collider_a->rigid_body;
    btRigidBody *rigid_body_b = (btRigidBody *)collider_b->rigid_body;

    switch(constraint->type)
    {
        case P_CONSTRAINT_TYPE_HINGE:
        {
            btVector3 pivot_a = btVector3(constraint_def->fields.hinge.pivot_a.x, constraint_def->fields.hinge.pivot_a.y, constraint_def->fields.hinge.pivot_a.z);
            btVector3 pivot_b = btVector3(constraint_def->fields.hinge.pivot_b.x, constraint_def->fields.hinge.pivot_b.y, constraint_def->fields.hinge.pivot_b.z);
            vec3_t *axis_a = &constraint_def->fields.hinge.axis;
            vec3_t axis_b;
            mat3_t_vec3_t_mul(&axis_b, axis_a, &a_to_b_transform);
            btHingeConstraint *hinge_constraint;
            hinge_constraint = new btHingeConstraint(*rigid_body_a, *rigid_body_b, pivot_a, pivot_b,
                                                     btVector3(axis_a->x, axis_a->y, axis_a->z), btVector3(axis_b.x, axis_b.y, axis_b.z));

            constraint->constraint = hinge_constraint;
            constraint->fields.hinge = constraint_def->fields.hinge;
            hinge_constraint->setLimit(constraint_def->fields.hinge.limit_low, constraint_def->fields.hinge.limit_high);
        }
        break;
    }

    p_dynamics_world->addConstraint((btTypedConstraint *)constraint->constraint);

    return constraint;
}

void p_DestroyConstraint(struct p_constraint_t *constraint)
{
    if(constraint && constraint->index != 0xffffffff)
    {
        btTypedConstraint *bt_constraint = (btTypedConstraint *)constraint->constraint;
        p_dynamics_world->removeConstraint(bt_constraint);
        delete bt_constraint;

        for(uint32_t index = 0; index < 2; index++)
        {
            struct p_collider_constraint_t *col_constraint = constraint->colliders + index;

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

struct p_collider_t *p_Raycast(vec3_t *from, vec3_t *to, float *time)
{
    btVector3 ray_from(from->x, from->y, from->z);
    btVector3 ray_to(to->x, to->y, to->z);
    btCollisionWorld::ClosestRayResultCallback ray_test(ray_from, ray_to);

    p_dynamics_world->rayTest(ray_from, ray_to, ray_test);
    if(ray_test.m_closestHitFraction < 1.0)
    {
        *time = ray_test.m_closestHitFraction;
        btRigidBody *rigid_body = (btRigidBody *)ray_test.m_collisionObject;
        return (struct p_collider_t *)rigid_body->getUserPointer();
    }

    return NULL;
}

void p_StepPhysics(float delta_time)
{
    if(!p_physics_frozen)
    {
        p_dynamics_world->stepSimulation(delta_time, 10);
    }

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_DYNAMIC].cursor; collider_index++)
    {
        struct p_collider_t *collider = p_GetCollider(P_COLLIDER_TYPE_DYNAMIC, collider_index);

        if(collider)
        {
            p_UpdateColliderTransform(collider);
            struct p_dynamic_collider_t *dynamic_collider = (struct p_dynamic_collider_t *)collider;
            btRigidBody *rigid_body = (btRigidBody *)collider->rigid_body;
            btVector3 linear_velocity = rigid_body->getLinearVelocity();
            dynamic_collider->linear_velocity = vec3_t_c(linear_velocity[0], linear_velocity[1], linear_velocity[2]);
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

void p_FreezePhysics()
{
    p_physics_frozen = 1;
}

void p_UnfreezePhysics()
{
    p_physics_frozen = 0;
}

#ifdef __cplusplus
}
#endif // __cplusplus


