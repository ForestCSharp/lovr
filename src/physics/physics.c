#include "physics.h"
#include "math/quat.h"
#include <stdlib.h>

static void defaultNearCallback(void* data, dGeomID a, dGeomID b) {
  lovrWorldCollide((World*) data, dGeomGetData(a), dGeomGetData(b));
}

static void customNearCallback(void* data, dGeomID shapeA, dGeomID shapeB) {
  World* world = data;
  vec_push(&world->overlaps, dGeomGetData(shapeA));
  vec_push(&world->overlaps, dGeomGetData(shapeB));
}

void lovrPhysicsInit() {
  dInitODE();

  if (!dCheckConfiguration("ODE_single_precision")) {
    error("lovr.physics must use single precision");
  }

  atexit(lovrPhysicsDestroy);
}

void lovrPhysicsDestroy() {
  dCloseODE();
}

World* lovrWorldCreate() {
  World* world = lovrAlloc(sizeof(World), lovrWorldDestroy);
  if (!world) return NULL;

  world->id = dWorldCreate();
  world->space = dHashSpaceCreate(0);
  dHashSpaceSetLevels(world->space, -4, 8);
  world->contactGroup = dJointGroupCreate(0);
  vec_init(&world->overlaps);

  return world;
}

void lovrWorldDestroy(const Ref* ref) {
  World* world = containerof(ref, World);
  dWorldDestroy(world->id);
  vec_deinit(&world->overlaps);
  free(world);
}

void lovrWorldGetGravity(World* world, float* x, float* y, float* z) {
  dReal gravity[3];
  dWorldGetGravity(world->id, gravity);
  *x = gravity[0];
  *y = gravity[1];
  *z = gravity[2];
}

void lovrWorldSetGravity(World* world, float x, float y, float z) {
  dWorldSetGravity(world->id, x, y, z);
}

void lovrWorldGetLinearDamping(World* world, float* damping, float* threshold) {
  *damping = dWorldGetLinearDamping(world->id);
  *threshold = dWorldGetLinearDampingThreshold(world->id);
}

void lovrWorldSetLinearDamping(World* world, float damping, float threshold) {
  dWorldSetLinearDamping(world->id, damping);
  dWorldSetLinearDampingThreshold(world->id, threshold);
}

void lovrWorldGetAngularDamping(World* world, float* damping, float* threshold) {
  *damping = dWorldGetAngularDamping(world->id);
  *threshold = dWorldGetAngularDampingThreshold(world->id);
}

void lovrWorldSetAngularDamping(World* world, float damping, float threshold) {
  dWorldSetAngularDamping(world->id, damping);
  dWorldSetAngularDampingThreshold(world->id, threshold);
}

int lovrWorldIsSleepingAllowed(World* world) {
  return dWorldGetAutoDisableFlag(world->id);
}

void lovrWorldSetSleepingAllowed(World* world, int allowed) {
  dWorldSetAutoDisableFlag(world->id, allowed);
}

void lovrWorldUpdate(World* world, float dt, CollisionResolver resolver, void* userdata) {
  if (resolver) {
    resolver(world, userdata);
  } else {
    dSpaceCollide(world->space, world, defaultNearCallback);
  }

  dWorldQuickStep(world->id, dt);
  dJointGroupEmpty(world->contactGroup);
}

void lovrWorldComputeOverlaps(World* world) {
  vec_clear(&world->overlaps);
  dSpaceCollide(world->space, world, customNearCallback);
}

int lovrWorldGetNextOverlap(World* world, Shape** a, Shape** b) {
  if (world->overlaps.length == 0) {
    *a = *b = NULL;
    return 0;
  }

  *a = vec_pop(&world->overlaps);
  *b = vec_pop(&world->overlaps);
  return 1;
}

int lovrWorldCollide(World* world, Shape* a, Shape* b) {
  if (!a || !b) {
    return 0;
  }

  dContact contacts[MAX_CONTACTS];

  for (int i = 0; i < MAX_CONTACTS; i++) {
    contacts[i].surface.mode = 0;
    contacts[i].surface.mu = dInfinity;
  }

  int contactCount = dCollide(a->id, b->id, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));

  for (int i = 0; i < contactCount; i++) {
    dJointID joint = dJointCreateContact(world->id, world->contactGroup, &contacts[i]);
    dJointAttach(joint, a->body->id, b->body->id);
  }

  return contactCount;
}

Body* lovrBodyCreate(World* world) {
  if (!world) {
    error("No world specified");
  }

  Body* body = lovrAlloc(sizeof(Body), lovrBodyDestroy);
  if (!body) return NULL;

  body->id = dBodyCreate(world->id);
  body->world = world;
  dBodySetData(body->id, body);

  return body;
}

void lovrBodyDestroy(const Ref* ref) {
  Body* body = containerof(ref, Body);
  dBodyDestroy(body->id);
  free(body);
}

void lovrBodyGetPosition(Body* body, float* x, float* y, float* z) {
  const dReal* position = dBodyGetPosition(body->id);
  *x = position[0];
  *y = position[1];
  *z = position[2];
}

void lovrBodySetPosition(Body* body, float x, float y, float z) {
  dBodySetPosition(body->id, x, y, z);
}

void lovrBodyGetOrientation(Body* body, float* angle, float* x, float* y, float* z) {
  const dReal* q = dBodyGetQuaternion(body->id);
  float quaternion[4] = { q[1], q[2], q[3], q[0] };
  quat_getAngleAxis(quaternion, angle, x, y, z);
}

void lovrBodySetOrientation(Body* body, float angle, float x, float y, float z) {
  float quaternion[4];
  float axis[3] = { x, y, z };
  quat_fromAngleAxis(quaternion, angle, axis);
  float q[4] = { quaternion[3], quaternion[0], quaternion[1], quaternion[2] };
  dBodySetQuaternion(body->id, q);
}

void lovrBodyGetLinearVelocity(Body* body, float* x, float* y, float* z) {
  const dReal* velocity = dBodyGetLinearVel(body->id);
  *x = velocity[0];
  *y = velocity[1];
  *z = velocity[2];
}

void lovrBodySetLinearVelocity(Body* body, float x, float y, float z) {
  dBodySetLinearVel(body->id, x, y, z);
}

void lovrBodyGetAngularVelocity(Body* body, float* x, float* y, float* z) {
  const dReal* velocity = dBodyGetAngularVel(body->id);
  *x = velocity[0];
  *y = velocity[1];
  *z = velocity[2];
}

void lovrBodySetAngularVelocity(Body* body, float x, float y, float z) {
  dBodySetAngularVel(body->id, x, y, z);
}

void lovrBodyGetLinearDamping(Body* body, float* damping, float* threshold) {
  *damping = dBodyGetLinearDamping(body->id);
  *threshold = dBodyGetLinearDampingThreshold(body->id);
}

void lovrBodySetLinearDamping(Body* body, float damping, float threshold) {
  dBodySetLinearDamping(body->id, damping);
  dBodySetLinearDampingThreshold(body->id, threshold);
}

void lovrBodyGetAngularDamping(Body* body, float* damping, float* threshold) {
  *damping = dBodyGetAngularDamping(body->id);
  *threshold = dBodyGetAngularDampingThreshold(body->id);
}

void lovrBodySetAngularDamping(Body* body, float damping, float threshold) {
  dBodySetAngularDamping(body->id, damping);
  dBodySetAngularDampingThreshold(body->id, threshold);
}

void lovrBodyApplyForce(Body* body, float x, float y, float z) {
  dBodyAddForce(body->id, x, y, z);
}

void lovrBodyApplyForceAtPosition(Body* body, float x, float y, float z, float cx, float cy, float cz) {
  dBodyAddForceAtPos(body->id, x, y, z, cx, cy, cz);
}

void lovrBodyApplyTorque(Body* body, float x, float y, float z) {
  dBodyAddTorque(body->id, x, y, z);
}

int lovrBodyIsKinematic(Body* body) {
  return dBodyIsKinematic(body->id);
}

void lovrBodySetKinematic(Body* body, int kinematic) {
  if (kinematic) {
    dBodySetKinematic(body->id);
  } else {
    dBodySetDynamic(body->id);
  }
}

void lovrBodyGetLocalPoint(Body* body, float wx, float wy, float wz, float* x, float* y, float* z) {
  dReal local[3];
  dBodyGetPosRelPoint(body->id, wx, wy, wz, local);
  *x = local[0];
  *y = local[1];
  *z = local[2];
}

void lovrBodyGetWorldPoint(Body* body, float x, float y, float z, float* wx, float* wy, float* wz) {
  dReal world[3];
  dBodyGetRelPointPos(body->id, x, y, z, world);
  *wx = world[0];
  *wy = world[1];
  *wz = world[2];
}

void lovrBodyGetLocalVector(Body* body, float wx, float wy, float wz, float* x, float* y, float* z) {
  dReal local[3];
  dBodyVectorFromWorld(body->id, wx, wy, wz, local);
  *x = local[0];
  *y = local[1];
  *z = local[2];
}

void lovrBodyGetWorldVector(Body* body, float x, float y, float z, float* wx, float* wy, float* wz) {
  dReal world[3];
  dBodyVectorToWorld(body->id, x, y, z, world);
  *wx = world[0];
  *wy = world[1];
  *wz = world[2];
}

void lovrBodyGetLinearVelocityFromLocalPoint(Body* body, float x, float y, float z, float* vx, float* vy, float* vz) {
  dReal velocity[3];
  dBodyGetRelPointVel(body->id, x, y, z, velocity);
  *vx = velocity[0];
  *vy = velocity[1];
  *vz = velocity[2];
}

void lovrBodyGetLinearVelocityFromWorldPoint(Body* body, float wx, float wy, float wz, float* vx, float* vy, float* vz) {
  dReal velocity[3];
  dBodyGetPointVel(body->id, wx, wy, wz, velocity);
  *vx = velocity[0];
  *vy = velocity[1];
  *vz = velocity[2];
}

int lovrBodyIsSleepingAllowed(Body* body) {
  return dBodyGetAutoDisableFlag(body->id);
}

void lovrBodySetSleepingAllowed(Body* body, int allowed) {
  dBodySetAutoDisableFlag(body->id, allowed);
}

int lovrBodyIsAwake(Body* body) {
  return dBodyIsEnabled(body->id);
}

void lovrBodySetAwake(Body* body, int awake) {
  if (awake) {
    dBodyEnable(body->id);
  } else {
    dBodyDisable(body->id);
  }
}

void* lovrBodyGetUserData(Body* body) {
  return body->userdata;
}

void lovrBodySetUserData(Body* body, void* data) {
  body->userdata = data;
}

World* lovrBodyGetWorld(Body* body) {
  return body->world;
}

float lovrBodyGetMass(Body* body) {
  dMass m;
  dBodyGetMass(body->id, &m);
  return m.mass;
}

void lovrBodySetMass(Body* body, float mass) {
  dMass m;
  dBodyGetMass(body->id, &m);
  dMassAdjust(&m, mass);
  dBodySetMass(body->id, &m);
}

void lovrBodyGetMassData(Body* body, float* cx, float* cy, float* cz, float* mass, float inertia[6]) {
  dMass m;
  dBodyGetMass(body->id, &m);
  *cx = m.c[0];
  *cy = m.c[1];
  *cz = m.c[2];
  *mass = m.mass;

  // Diagonal
  inertia[0] = m.I[0];
  inertia[1] = m.I[5];
  inertia[2] = m.I[10];

  // Lower triangular
  inertia[3] = m.I[4];
  inertia[4] = m.I[8];
  inertia[5] = m.I[9];
}

void lovrBodySetMassData(Body* body, float cx, float cy, float cz, float mass, float inertia[]) {
  dMass m;
  dBodyGetMass(body->id, &m);
  dMassSetParameters(&m, mass, cx, cy, cz, inertia[0], inertia[1], inertia[2], inertia[3], inertia[4], inertia[5]);
  dBodySetMass(body->id, &m);
}

void lovrShapeDestroy(const Ref* ref) {
  Shape* shape = containerof(ref, Shape);
  dGeomDestroy(shape->id);
  free(shape);
}

ShapeType lovrShapeGetType(Shape* shape) {
  return shape->type;
}

Body* lovrShapeGetBody(Shape* shape) {
  return shape->body;
}

void lovrShapeSetBody(Shape* shape, Body* body) {
  shape->body = body;
  dGeomSetBody(shape->id, body ? body->id : 0);

  dSpaceID oldSpace = dGeomGetSpace(shape->id);
  dSpaceID newSpace = body ? body->world->space : 0;

  if (oldSpace && oldSpace != newSpace) {
    dSpaceRemove(oldSpace, shape->id);
  }

  if (newSpace && newSpace != oldSpace) {
    dSpaceAdd(newSpace, shape->id);
  }
}

int lovrShapeIsEnabled(Shape* shape) {
  return dGeomIsEnabled(shape->id);
}

void lovrShapeSetEnabled(Shape* shape, int enabled) {
  if (enabled) {
    dGeomEnable(shape->id);
  } else {
    dGeomDisable(shape->id);
  }
}

void* lovrShapeGetUserData(Shape* shape) {
  return shape->userdata;
}

void lovrShapeSetUserData(Shape* shape, void* data) {
  shape->userdata = data;
}

void lovrShapeGetPosition(Shape* shape, float* x, float* y, float* z) {
  const dReal* position = dGeomGetOffsetPosition(shape->id);
  *x = position[0];
  *y = position[1];
  *z = position[2];
}

void lovrShapeSetPosition(Shape* shape, float x, float y, float z) {
  dGeomSetOffsetPosition(shape->id, x, y, z);
}

void lovrShapeGetOrientation(Shape* shape, float* angle, float* x, float* y, float* z) {
  dReal q[4];
  dGeomGetOffsetQuaternion(shape->id, q);
  float quaternion[4] = { q[1], q[2], q[3], q[0] };
  quat_getAngleAxis(quaternion, angle, x, y, z);
}

void lovrShapeSetOrientation(Shape* shape, float angle, float x, float y, float z) {
  float quaternion[4];
  float axis[3] = { x, y, z };
  quat_fromAngleAxis(quaternion, angle, axis);
  float q[4] = { quaternion[3], quaternion[0], quaternion[1], quaternion[2] };
  dGeomSetOffsetQuaternion(shape->id, q);
}

uint32_t lovrShapeGetCategory(Shape* shape) {
  return dGeomGetCategoryBits(shape->id);
}

void lovrShapeSetCategory(Shape* shape, uint32_t category) {
  dGeomSetCategoryBits(shape->id, category);
}

uint32_t lovrShapeGetMask(Shape* shape) {
  return dGeomGetCollideBits(shape->id);
}

void lovrShapeSetMask(Shape* shape, uint32_t mask) {
  dGeomSetCollideBits(shape->id, mask);
}

void lovrShapeComputeMass(Shape* shape, float density, float* cx, float* cy, float* cz, float* mass, float inertia[6]) {
  dMass m;
  dMassSetZero(&m);
  switch (shape->type) {
    case SHAPE_SPHERE: {
      dMassSetSphere(&m, density, dGeomSphereGetRadius(shape->id));
      break;
    }

    case SHAPE_BOX: {
      dReal lengths[3];
      dGeomBoxGetLengths(shape->id, lengths);
      dMassSetBox(&m, density, lengths[0], lengths[1], lengths[2]);
      break;
    }

    case SHAPE_CAPSULE: {
      dReal radius, length;
      dGeomCapsuleGetParams(shape->id, &radius, &length);
      dMassSetCapsule(&m, density, 3, radius, length);
      break;
    }

    case SHAPE_CYLINDER: {
      dReal radius, length;
      dGeomCylinderGetParams(shape->id, &radius, &length);
      dMassSetCylinder(&m, density, 3, radius, length);
      break;
    }
  }

  const dReal* position = dGeomGetOffsetPosition(shape->id);
  dMassTranslate(&m, position[0], position[1], position[2]);
  const dReal* rotation = dGeomGetOffsetRotation(shape->id);
  dMassRotate(&m, rotation);

  *cx = m.c[0];
  *cy = m.c[1];
  *cz = m.c[2];
  *mass = m.mass;

  // Diagonal
  inertia[0] = m.I[0];
  inertia[1] = m.I[5];
  inertia[2] = m.I[10];

  // Lower triangular
  inertia[3] = m.I[4];
  inertia[4] = m.I[8];
  inertia[5] = m.I[9];
}

SphereShape* lovrSphereShapeCreate(float radius) {
  SphereShape* sphere = lovrAlloc(sizeof(SphereShape), lovrShapeDestroy);
  if (!sphere) return NULL;

  sphere->type = SHAPE_SPHERE;
  sphere->id = dCreateSphere(0, radius);
  dGeomSetData(sphere->id, sphere);

  return sphere;
}

float lovrSphereShapeGetRadius(SphereShape* sphere) {
  return dGeomSphereGetRadius(sphere->id);
}

void lovrSphereShapeSetRadius(SphereShape* sphere, float radius) {
  dGeomSphereSetRadius(sphere->id, radius);
}

BoxShape* lovrBoxShapeCreate(float x, float y, float z) {
  BoxShape* box = lovrAlloc(sizeof(BoxShape), lovrShapeDestroy);
  if (!box) return NULL;

  box->type = SHAPE_BOX;
  box->id = dCreateBox(0, x, y, z);
  dGeomSetData(box->id, box);

  return box;
}

void lovrBoxShapeGetDimensions(BoxShape* box, float* x, float* y, float* z) {
  float dimensions[3];
  dGeomBoxGetLengths(box->id, dimensions);
  *x = dimensions[0];
  *y = dimensions[1];
  *z = dimensions[2];
}

void lovrBoxShapeSetDimensions(BoxShape* box, float x, float y, float z) {
  dGeomBoxSetLengths(box->id, x, y, z);
}

CapsuleShape* lovrCapsuleShapeCreate(float radius, float length) {
  CapsuleShape* capsule = lovrAlloc(sizeof(CapsuleShape), lovrShapeDestroy);
  if (!capsule) return NULL;

  capsule->type = SHAPE_CAPSULE;
  capsule->id = dCreateCapsule(0, radius, length);
  dGeomSetData(capsule->id, capsule);

  return capsule;
}

float lovrCapsuleShapeGetRadius(CapsuleShape* capsule) {
  float radius, length;
  dGeomCapsuleGetParams(capsule->id, &radius, &length);
  return radius;
}

void lovrCapsuleShapeSetRadius(CapsuleShape* capsule, float radius) {
  dGeomCapsuleSetParams(capsule->id, radius, lovrCapsuleShapeGetLength(capsule));
}

float lovrCapsuleShapeGetLength(CapsuleShape* capsule) {
  float radius, length;
  dGeomCapsuleGetParams(capsule->id, &radius, &length);
  return length;
}

void lovrCapsuleShapeSetLength(CapsuleShape* capsule, float length) {
  dGeomCapsuleSetParams(capsule->id, lovrCapsuleShapeGetRadius(capsule), length);
}

CylinderShape* lovrCylinderShapeCreate(float radius, float length) {
  CylinderShape* cylinder = lovrAlloc(sizeof(CylinderShape), lovrShapeDestroy);
  if (!cylinder) return NULL;

  cylinder->type = SHAPE_CYLINDER;
  cylinder->id = dCreateCylinder(0, radius, length);
  dGeomSetData(cylinder->id, cylinder);

  return cylinder;
}

float lovrCylinderShapeGetRadius(CylinderShape* cylinder) {
  float radius, length;
  dGeomCylinderGetParams(cylinder->id, &radius, &length);
  return radius;
}

void lovrCylinderShapeSetRadius(CylinderShape* cylinder, float radius) {
  dGeomCylinderSetParams(cylinder->id, radius, lovrCylinderShapeGetLength(cylinder));
}

float lovrCylinderShapeGetLength(CylinderShape* cylinder) {
  float radius, length;
  dGeomCylinderGetParams(cylinder->id, &radius, &length);
  return length;
}

void lovrCylinderShapeSetLength(CylinderShape* cylinder, float length) {
  dGeomCylinderSetParams(cylinder->id, lovrCylinderShapeGetRadius(cylinder), length);
}