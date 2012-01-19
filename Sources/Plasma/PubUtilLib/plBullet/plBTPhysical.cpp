/*==COPYING==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011 Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==COPYING==*/
#include "plBTPhysical.h"

#include "hsResMgr.h"
#include "hsStream.h"
#include "hsTimer.h"
#include "plProfile.h"
#include "hsQuat.h"
#include "hsSTLStream.h"

#include "plSimulationMgr.h"
#include "plPhysical/plPhysicalSDLModifier.h"
#include "plPhysical/plPhysicalSndGroup.h"
#include "plPhysical/plPhysicalProxy.h"
#include "pnSceneObject/plSimulationInterface.h"
#include "pnSceneObject/plCoordinateInterface.h"

#include "plSurface/hsGMaterial.h"
#include "plSurface/plLayerInterface.h"

#include "pnKeyedObject/plKey.h"
#include "pnMessage/plCorrectionMsg.h"
#include "pnMessage/plNodeRefMsg.h"
#include "pnMessage/plSDLModifierMsg.h"
#include "plMessage/plSimStateMsg.h"
#include "plMessage/plSimInfluenceMsg.h"
#include "plMessage/plLinearVelocityMsg.h"
#include "plMessage/plAngularVelocityMsg.h"
#include "plDrawable/plDrawableGenerator.h"
#include "plNetClient/plNetClientMgr.h"
#include "plNetTransport/plNetTransportMember.h"
#include "plStatusLog/plStatusLog.h"
#include "plBTPhysicalControllerCore.h"

#include <btBulletDynamicsCommon.h>

#define LogActivate(func) if (fActor->isSleeping()) SimLog("%s activated by %s", GetKeyName(), func);
#define kMaxNegativeZPos -2000.f

plProfile_Extern(MaySendLocation);
plProfile_Extern(LocationsSent);
plProfile_Extern(PhysicsUpdates);
plProfile_Extern(SetTransforms);

bool CompareMatrices(const hsMatrix44 &matA, const hsMatrix44 &matB, float tolerance)
{
    return 
        (fabs(matA.fMap[0][0] - matB.fMap[0][0]) < tolerance) &&
        (fabs(matA.fMap[0][1] - matB.fMap[0][1]) < tolerance) &&
        (fabs(matA.fMap[0][2] - matB.fMap[0][2]) < tolerance) &&
        (fabs(matA.fMap[0][3] - matB.fMap[0][3]) < tolerance) &&

        (fabs(matA.fMap[1][0] - matB.fMap[1][0]) < tolerance) &&
        (fabs(matA.fMap[1][1] - matB.fMap[1][1]) < tolerance) &&
        (fabs(matA.fMap[1][2] - matB.fMap[1][2]) < tolerance) &&
        (fabs(matA.fMap[1][3] - matB.fMap[1][3]) < tolerance) &&

        (fabs(matA.fMap[2][0] - matB.fMap[2][0]) < tolerance) &&
        (fabs(matA.fMap[2][1] - matB.fMap[2][1]) < tolerance) &&
        (fabs(matA.fMap[2][2] - matB.fMap[2][2]) < tolerance) &&
        (fabs(matA.fMap[2][3] - matB.fMap[2][3]) < tolerance) &&

        (fabs(matA.fMap[3][0] - matB.fMap[3][0]) < tolerance) &&
        (fabs(matA.fMap[3][1] - matB.fMap[3][1]) < tolerance) &&
        (fabs(matA.fMap[3][2] - matB.fMap[3][2]) < tolerance) &&
        (fabs(matA.fMap[3][3] - matB.fMap[3][3]) < tolerance);
}

static void ClearMatrix(hsMatrix44 &m)
{
    m.fMap[0][0] = 0.0f; m.fMap[0][1] = 0.0f; m.fMap[0][2] = 0.0f; m.fMap[0][3]  = 0.0f;
    m.fMap[1][0] = 0.0f; m.fMap[1][1] = 0.0f; m.fMap[1][2] = 0.0f; m.fMap[1][3]  = 0.0f;
    m.fMap[2][0] = 0.0f; m.fMap[2][1] = 0.0f; m.fMap[2][2] = 0.0f; m.fMap[2][3]  = 0.0f;
    m.fMap[3][0] = 0.0f; m.fMap[3][1] = 0.0f; m.fMap[3][2] = 0.0f; m.fMap[3][3]  = 0.0f;
    m.NotIdentity();
}

btVector3 toBullet(const hsVector3& vec) {
    return btVector3 (vec.fX, vec.fY, vec.fZ);
}

btVector3 toBullet(const hsPoint3& vec) {
    return btVector3 (vec.fX, vec.fY, vec.fZ);
}

btQuaternion toBullet(const hsQuat& quat) {
    return btQuaternion(quat.fX, quat.fY, quat.fZ, quat.fW);
}

btTransform toBullet(const hsMatrix44& mat) {
    hsMatrix44 ogl;
    mat.GetTranspose(&ogl);
    btTransform t;
    t.setFromOpenGLMatrix((const btScalar*)ogl.fMap);
    return t;
}

template<class T>
T toPlasma(const btVector3& vec) {
    return T(vec.x(), vec.y(), vec.z());
}

hsQuat toPlasma(const btQuaternion& quat) {
    return hsQuat(quat.x(), quat.y(), quat.z(), quat.w());
}

hsMatrix44 toPlasma(const btTransform& trans) {
    hsMatrix44 ogl, result;
    trans.getOpenGLMatrix((btScalar*)ogl.fMap);
    ogl.GetTranspose(&result);
    return result;
}

PhysRecipe::PhysRecipe()
    : mass(0.f)
    , friction(0.f)
    , restitution(0.f)
    , bounds(plSimDefs::kBoundsMax)
    , group(plSimDefs::kGroupMax)
    , reportsOn(0)
    , objectKey(nil)
    , sceneNode(nil)
    , worldKey(nil)
    , radius(0.f)
    , offset(0.f, 0.f, 0.f)
{
    l2s.Reset();
}

plBTPhysical::plBTPhysical()
    : fSDLMod(nil)
    , fBoundsType(plSimDefs::kBoundsMax)
    , fLOSDBs(plSimDefs::kLOSDBNone)
    , fGroup(plSimDefs::kGroupMax)
    , fReportsOn(0)
    , fLastSyncTime(0.0f)
    , fProxyGen(nil)
    , fSceneNode(nil)
    , fWorldKey(nil)
    , fSndGroup(nil)
    , fWorldHull(nil)
    , fHullNumberPlanes(0)
    , fMass(0.f)
    , fWeWereHit(false)
    , fHitForce(0,0,0)
    , fHitPos(0,0,0)
    , fInsideConvexHull(false)
    , fEnabled(true)
{
}

plBTPhysical::~plBTPhysical()
{
    plSimulationMgr::Log("Destroying physical %s", GetKeyName());

    if (fBody)
    {
        IEnable(true);
        BtScene* scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
        scene->world->removeRigidBody(fBody);
        delete fBody->getMotionState();
        btCollisionShape *shape = fBody->getCollisionShape();
        if(fBoundsType == plSimDefs::kProxyBounds || fBoundsType == plSimDefs::kExplicitBounds)
            delete static_cast<btBvhTriangleMeshShape*>(shape)->getMeshInterface();
        delete shape;
        delete fBody;
        plSimulationMgr::GetInstance()->ReleaseScene(fWorldKey);
    }

    if (fWorldHull)
        delete [] fWorldHull;

    delete fProxyGen;

    // remove sdl modifier
    plSceneObject* sceneObj = plSceneObject::ConvertNoRef(fObjectKey->ObjectIsLoaded());
    if (sceneObj && fSDLMod)
    {
        sceneObj->RemoveModifier(fSDLMod);
    }
    delete fSDLMod;
}

hsBool plBTPhysical::Init(PhysRecipe& recipe)
{
    fBoundsType = recipe.bounds;
    fGroup = recipe.group;
    fReportsOn = recipe.reportsOn;
    fObjectKey = recipe.objectKey;
    fSceneNode = recipe.sceneNode;
    fWorldKey = recipe.worldKey;
    fMass = recipe.mass;
    fSavedPositions = recipe.vertices;
    fSavedIndices = recipe.indices;

    for(size_t i = 0; i < fSavedIndices.size(); i++) {
        if(fSavedIndices[i] >= fSavedPositions.size())
            MessageBox(NULL, "Error in indices: too large", "Error", 0);
    }

    btCollisionShape* shape;
    switch(fBoundsType) {
    case plSimDefs::kSphereBounds:
        shape = new btSphereShape(recipe.radius);
        break;
    case plSimDefs::kHullBounds:
        shape = new btConvexHullShape;
        for (size_t i = 0; i < recipe.vertices.size(); i++) {
            static_cast<btConvexHullShape*>(shape)->addPoint(toBullet(recipe.vertices[i]));
        }
        break;
    case plSimDefs::kBoxBounds:
        shape = new btBoxShape(toBullet(recipe.bDimensions)/2);
        break;
    case plSimDefs::kExplicitBounds:
    case plSimDefs::kProxyBounds:
        {
            // Bullet does not support animated trimesh objects
            fMass = 0.0f;
            fGroup = plSimDefs::kGroupStatic;
            btTriangleMesh *mesh = new btTriangleMesh;
            for(size_t i = 0; i < recipe.indices.size(); i+=3) {
                mesh->addTriangle(toBullet(recipe.vertices[recipe.indices[i]]), toBullet(recipe.vertices[recipe.indices[i+1]]), toBullet(recipe.vertices[recipe.indices[i+2]]), true);
            }
        }
        break;
    }

    btVector3 inertia;
    if (fMass != 0.0f)
        shape->calculateLocalInertia(fMass, inertia);

    btRigidBody::btRigidBodyConstructionInfo cinfo(fMass, new btDefaultMotionState, shape, inertia);
    cinfo.m_friction = recipe.friction;
    cinfo.m_restitution = recipe.restitution;

    fBody = new btRigidBody(cinfo);
    fBody->setUserPointer(this);

    BtScene *scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
    short colgroups = 0;
    switch (fGroup) {
    case plSimDefs::kGroupAvatar:
        colgroups |= 1 << plSimDefs::kGroupAvatarBlocker;
        colgroups |= 1 << plSimDefs::kGroupDetector;
        colgroups |= 1 << plSimDefs::kGroupDynamic;
        colgroups |= 1 << plSimDefs::kGroupStatic;
        break;
    case plSimDefs::kGroupAvatarBlocker:
        colgroups |= 1 << plSimDefs::kGroupAvatar;
        break;
    case plSimDefs::kGroupDetector:
        colgroups |= 1 << plSimDefs::kGroupAvatar;
        colgroups |= 1 << plSimDefs::kGroupDynamic;
    case plSimDefs::kGroupDynamic:
        colgroups |= 1 << plSimDefs::kGroupAvatar;
        colgroups |= 1 << plSimDefs::kGroupDetector;
        colgroups |= 1 << plSimDefs::kGroupDynamicBlocker;
        colgroups |= 1 << plSimDefs::kGroupDynamic;
        colgroups |= 1 << plSimDefs::kGroupStatic;
        break;
    case plSimDefs::kGroupDynamicBlocker:
        colgroups |= 1 << plSimDefs::kGroupDynamic;
        break;
    case plSimDefs::kGroupStatic:
        colgroups |= 1 << plSimDefs::kGroupAvatar;
        colgroups |= 1 << plSimDefs::kGroupDynamic;
        break;
    default:
        // All other types don't do normal collisions
        break;
    }

    if (fGroup == plSimDefs::kGroupStatic)
        fBody->setCollisionFlags(fBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    if (fGroup == plSimDefs::kGroupDetector)
        fBody->setCollisionFlags(fBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

    scene->world->addRigidBody(fBody, 1 << fGroup, colgroups);

    if (GetProperty(plSimulationInterface::kDisable))
        IEnable(false);
    return true;
}

/////////////////////////////////////////////////////////////////////
//
// READING AND WRITING
//
/////////////////////////////////////////////////////////////////////

void plBTPhysical::IReadV0(hsStream* stream, hsResMgr* mgr, PhysRecipe& recipe)
{
    plSimulationMgr *simmgr = plSimulationMgr::GetInstance();
    simmgr->Log("Beginning compat read from PhysX data\n");
    ClearMatrix(fCachedLocal2World);

    recipe.mass = stream->ReadLEScalar();
    recipe.friction = stream->ReadLEScalar();
    recipe.restitution = stream->ReadLEScalar();
    recipe.bounds = (plSimDefs::Bounds)stream->ReadByte();
    recipe.group = (plSimDefs::Group)stream->ReadByte();
    recipe.reportsOn = stream->ReadLE32();
    fLOSDBs = stream->ReadLE16();
    //hack for swim regions currently they are labeled as static av blockers
    if(fLOSDBs==plSimDefs::kLOSDBSwimRegion)
    {
        recipe.group=plSimDefs::kGroupMax;
    }
    //
    recipe.objectKey = mgr->ReadKey(stream);
    recipe.sceneNode = mgr->ReadKey(stream);
    recipe.worldKey = mgr->ReadKey(stream);

    mgr->ReadKeyNotifyMe(stream, new plGenRefMsg(GetKey(), plRefMsg::kOnCreate, 0, kPhysRefSndGroup), plRefFlags::kActiveRef);

    hsPoint3 pos;
    hsQuat rot;
    pos.Read(stream);
    rot.Read(stream);
    rot.MakeMatrix(&recipe.l2s);
    recipe.l2s.SetTranslate(&pos);

    fProps.Read(stream);

    if (recipe.bounds == plSimDefs::kSphereBounds)
    {
        recipe.radius = stream->ReadLEScalar();
        recipe.offset.Read(stream);
    }
    else if (recipe.bounds == plSimDefs::kBoxBounds)
    {
        recipe.bDimensions.Read(stream);
        recipe.bOffset.Read(stream);
    }
    else
    {
        // These are reverse-engineered readers for PhysX-formatted data. They exist in order to keep things nicely backwards
        // compatible with Cyan's PRPs.
        //
        // They read the minimum data we need to make the physical in Bullet. They leave extra crap on the stream, but so be it.
        if (recipe.bounds == plSimDefs::kHullBounds) {
            simmgr->Log("Reading convex hull data from PhysX\n");
            char tag[4];
            stream->Read(4, tag);
            stream->Read(4, tag);
            stream->ReadLE32();
            stream->ReadLE32();

            stream->Read(4, tag);
            stream->Read(4, tag);
            stream->ReadLE32();

            stream->Read(4, tag);
            stream->Read(4, tag);

            stream->ReadLE32();
            recipe.vertices.resize(stream->ReadLE32());
            recipe.indices.resize(stream->ReadLE32() * 3);
            stream->ReadLE32();
            stream->ReadLE32();
            stream->ReadLE32();
            stream->ReadLE32();

            for (size_t i=0; i<recipe.vertices.size(); i++)
                recipe.vertices[i].Read(stream);
            stream->ReadLE32();

            for (size_t i=0; i<recipe.indices.size(); i += 3) {
                if (recipe.vertices.size() < 256) {
                    recipe.indices[i+0] = stream->ReadByte();
                    recipe.indices[i+1] = stream->ReadByte();
                    recipe.indices[i+2] = stream->ReadByte();
                } else if (recipe.vertices.size() < 65536) {
                    recipe.indices[i+0] = stream->ReadLE16();
                    recipe.indices[i+1] = stream->ReadLE16();
                    recipe.indices[i+2] = stream->ReadLE16();
                } else {
                    recipe.indices[i+0] = stream->ReadLE32();
                    recipe.indices[i+1] = stream->ReadLE32();
                    recipe.indices[i+2] = stream->ReadLE32();
                }
            }
        } else {
            simmgr->Log("Reading trimesh data from PhysX\n");
            char tag[4];
            stream->Read(4, tag);
            stream->Read(4, tag);
            stream->ReadLE32();

            unsigned int nxFlags = stream->ReadLE32();
            stream->ReadLEFloat();
            stream->ReadLE32();
            stream->ReadLEFloat();
            unsigned int nxNumVerts = stream->ReadLE32();
            unsigned int nxNumTris = stream->ReadLE32();

            recipe.vertices.resize(nxNumVerts);
            for (size_t i = 0; i < nxNumVerts; i++) {
                recipe.vertices[i].Read(stream);
            }

            recipe.indices.resize(nxNumTris*3);
            for (size_t i = 0; i < nxNumTris * 3; i += 3) {
                if (nxFlags & 0x8) {
                    recipe.indices[i+0] = stream->ReadByte();
                    recipe.indices[i+1] = stream->ReadByte();
                    recipe.indices[i+2] = stream->ReadByte();
                } else if (nxFlags & 0x10) {
                    recipe.indices[i+0] = stream->ReadLE16();
                    recipe.indices[i+1] = stream->ReadLE16();
                    recipe.indices[i+2] = stream->ReadLE16();
                } else {
                    recipe.indices[i+0] = stream->ReadLE32();
                    recipe.indices[i+1] = stream->ReadLE32();
                    recipe.indices[i+2] = stream->ReadLE32();
                }
            }
        }
    }
    simmgr->Log("Compat read complete!\n");
}

void plBTPhysical::IReadV3(hsStream* stream, hsResMgr* mgr, PhysRecipe& recipe)
{
    // BULLET STUB
}

void plBTPhysical::Read(hsStream* stream, hsResMgr* mgr)
{
    PhysRecipe recipe;
    plPhysical::Read(stream, mgr); 
    switch(mgr->GetKeyVersion(GetKey())) {
    case 0:
        IReadV0(stream, mgr, recipe);
        break;
    case 3:
        IReadV3(stream, mgr, recipe);
        break;
    };
    Init(recipe);

    hsAssert(!fProxyGen, "Already have proxy gen, double read?");

    hsColorRGBA physColor;
    float opac = 1.0f;

    if (fGroup == plSimDefs::kGroupAvatar)
    {
        // local avatar is light purple and transparent
        physColor.Set(.2f, .1f, .2f, 1.f);
        opac = 0.4f;
    }
    else if (fGroup == plSimDefs::kGroupDynamic)
    {
        // Dynamics are red
        physColor.Set(1.f,0.f,0.f,1.f);
    }
    else if (fGroup == plSimDefs::kGroupDetector)
    {
        if(!fWorldKey)
        {
            // Detectors are blue, and transparent
            physColor.Set(0.f,0.f,1.f,1.f);
            opac = 0.3f;
        }
        else
        {
            // subworld Detectors are green
            physColor.Set(0.f,1.f,0.f,1.f);
            opac = 0.3f;
        }
    }
    else if (fGroup == plSimDefs::kGroupStatic)
    {
        if (GetProperty(plSimulationInterface::kPhysAnim))
            // Statics that are animated are more reddish?
            physColor.Set(1.f,0.6f,0.2f,1.f);
        else
            // Statics are yellow
            physColor.Set(1.f,0.8f,0.2f,1.f);
        // if in a subworld... slightly transparent
        if(fWorldKey)
            opac = 0.6f;
    }
    else
    {
        // don't knows are grey
        physColor.Set(0.6f,0.6f,0.6f,1.f);
    }

    fProxyGen = new plPhysicalProxy(hsColorRGBA().Set(0,0,0,1.f), physColor, opac);
    fProxyGen->Init(this);
    plSimulationMgr::GetInstance()->Log("Finished reading and initting BTPhysical\n");
}

void plBTPhysical::Write(hsStream* stream, hsResMgr* mgr)
{
    plPhysical::Write(stream, mgr);
    hsAssert(0, "plBTPhysical can't be written yet.");
    // BULLET STUB
    /*
    hsAssert(fActor, "nil actor");  
    hsAssert(fActor->getNbShapes() == 1, "Can only write actors with one shape. Writing first only.");
    NxShape* shape = fActor->getShapes()[0];

    NxMaterialIndex matIdx = shape->getMaterial();
    NxScene* scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
    NxMaterial* mat = scene->getMaterialFromIndex(matIdx);
    float friction = mat->getStaticFriction();
    float restitution = mat->getRestitution();

    stream->WriteSwapScalar(fActor->getMass());
    stream->WriteSwapScalar(friction);
    stream->WriteSwapScalar(restitution);
    stream->WriteByte(fBoundsType);
    stream->WriteByte(fGroup);
    stream->WriteSwap32(fReportsOn);
    stream->WriteSwap16(fLOSDBs);
    mgr->WriteKey(stream, fObjectKey);
    mgr->WriteKey(stream, fSceneNode);
    mgr->WriteKey(stream, fWorldKey);
    mgr->WriteKey(stream, fSndGroup);

    hsPoint3 pos;
    hsQuat rot;
    IGetPositionSim(pos);
    IGetRotationSim(rot);
    pos.Write(stream);
    rot.Write(stream);

    fProps.Write(stream);

    if (fBoundsType == plSimDefs::kSphereBounds)
    {
        const NxSphereShape* sphereShape = shape->isSphere();
        stream->WriteSwapScalar(sphereShape->getRadius());
        hsPoint3 localPos = plPXConvert::Point(sphereShape->getLocalPosition());
        localPos.Write(stream);
        stream->WriteSwapScalar(0.f);
        hsPoint3 localPos;
        localPos.Write(stream);
    }
    else if (fBoundsType == plSimDefs::kBoxBounds)
    {
        const NxBoxShape* boxShape = shape->isBox();
        hsPoint3 dim = plPXConvert::Point(boxShape->getDimensions());
        dim.Write(stream);
        hsPoint3 localPos = plPXConvert::Point(boxShape->getLocalPosition());
        localPos.Write(stream);
        hsPoint3 dim;
        dim.Write(stream);
        dim.Write(stream);
    }
    else
    {
        if (fBoundsType == plSimDefs::kHullBounds)
        hsAssert(shape->isConvexMesh(), "Hull shape isn't a convex mesh");
        else
        hsAssert(shape->isTriangleMesh(), "Exact shape isn't a trimesh");

        // We hide the stream we used to create this mesh away in the shape user data.
        // Pull it out and write it to disk.
        hsVectorStream* vecStream = (hsVectorStream*)shape->userData;
        stream->Write(vecStream->GetEOF(), vecStream->GetData());
        delete vecStream;
    } */
}

/////////////////////////////////////////////////////////////////
//
// MESSAGE HANDLING
//
/////////////////////////////////////////////////////////////////

// MSGRECEIVE
hsBool plBTPhysical::MsgReceive( plMessage* msg )
{
    if(plGenRefMsg *refM = plGenRefMsg::ConvertNoRef(msg))
    {
        return HandleRefMsg(refM);
    }
    else if(plSimulationMsg *simM = plSimulationMsg::ConvertNoRef(msg))
    {
        plLinearVelocityMsg* velMsg = plLinearVelocityMsg::ConvertNoRef(msg);
        if(velMsg)
        {
            SetLinearVelocitySim(velMsg->Velocity());
            return true;
        }
        plAngularVelocityMsg* angvelMsg = plAngularVelocityMsg::ConvertNoRef(msg);
        if(angvelMsg)
        {
            SetAngularVelocitySim(angvelMsg->AngularVelocity());
            return true;
        }


        return false;
    }
    // couldn't find a local handler: pass to the base
    else
        return plPhysical::MsgReceive(msg);
}

// IHANDLEREFMSG
// there's two things we hold references to: subworlds
// and the simulation manager.
// right now, we're only worrying about the subworlds
hsBool plBTPhysical::HandleRefMsg(plGenRefMsg* refMsg)
{
    uint8_t refCtxt = refMsg->GetContext();
    plKey refKey = refMsg->GetRef()->GetKey();
    plKey ourKey = GetKey();
    PhysRefType refType = PhysRefType(refMsg->fType);

    const char* refKeyName = refKey ? refKey->GetName().c_str() : "MISSING";

    if (refType == kPhysRefWorld)
    {
        if (refCtxt == plRefMsg::kOnCreate || refCtxt == plRefMsg::kOnRequest)
        {
            // Cache the initial transform, since we assume the sceneobject already knows
            // that and doesn't need to be told again
            IGetTransformGlobal(fCachedLocal2World);
        }
        if (refCtxt == plRefMsg::kOnDestroy)
        {
            // our world was deleted out from under us: move to the main world
            //          hsAssert(0, "Lost world");
        }
    }
    else if (refType == kPhysRefSndGroup)
    {
        switch (refCtxt)
        {
        case plRefMsg::kOnCreate:
        case plRefMsg::kOnRequest:
            fSndGroup = plPhysicalSndGroup::ConvertNoRef( refMsg->GetRef() );
            break;

        case plRefMsg::kOnDestroy:
            fSndGroup = nil;
            break;
        }
    }
    else
    {
        hsAssert(0, "Unknown ref type, who sent us this?");
    }

    return true;
}

plPhysical& plBTPhysical::SetProperty(int prop, hsBool status)
{
    if (GetProperty(prop) == status)
    {
        const char* propName = "(unknown)";
        switch (prop)
        {
        case plSimulationInterface::kDisable:           propName = "kDisable";              break;
        case plSimulationInterface::kPinned:            propName = "kPinned";               break;
        case plSimulationInterface::kPassive:           propName = "kPassive";              break;
        case plSimulationInterface::kPhysAnim:          propName = "kPhysAnim";             break;
        case plSimulationInterface::kStartInactive:     propName = "kStartInactive";        break;
        case plSimulationInterface::kNoSynchronize:     propName = "kNoSynchronize";        break;
        }

        const char* name = "(unknown)";
        if (GetKey())
            name = GetKeyName().c_str();
        if (plSimulationMgr::fExtraProfile)
            plSimulationMgr::Log("Warning: Redundant physical property set (property %s, value %s) on %s", propName, status ? "true" : "false", name);
    }

    switch (prop)
    {
    case plSimulationInterface::kDisable:
        IEnable(!status);
        break;

    case plSimulationInterface::kPinned:
        if (! fBody->isStaticObject())
        {
            // if the body is already unpinned and you unpin it again,
            // you'll wipe out its velocity. hence the check.
            // Is this also true for Bullet?
            hsBool current = fBody->getLinearFactor().x() == 0;
            if (status != current)
            {
                if (status) {
                    fBody->setLinearFactor(btVector3(0, 0, 0));
                    fBody->setAngularFactor(btVector3(0, 0, 0));
                } else {
                    fBody->setLinearFactor(btVector3(1, 1, 1));
                    fBody->setAngularFactor(btVector3(1, 1, 1));
                    fBody->activate();
                }
            }
        }
        break;
    }

    fProps.SetBit(prop, status);

    return *this;
}

plKey plBTPhysical::GetSceneNode() const
{
    return fSceneNode;
}

void plBTPhysical::SetSceneNode(plKey newNode)
{
#ifdef HS_DEBUGGING
    plKey oldNode = GetSceneNode();
    char msg[1024];
    if (newNode)
        sprintf(msg,"Physical object %s cannot change scenes. Already in %s, trying to switch to %s.",fObjectKey->GetName(),oldNode->GetName(),newNode->GetName());
    else
        sprintf(msg,"Physical object %s cannot change scenes. Already in %s, trying to switch to <nil key>.",fObjectKey->GetName(),oldNode->GetName());
    hsAssert(oldNode == newNode, msg);
#endif  // HS_DEBUGGING
}

hsBool plBTPhysical::GetLinearVelocitySim(hsVector3& vel) const
{
    vel = toPlasma<hsVector3>(fBody->getLinearVelocity());
    return true;
}

void plBTPhysical::SetLinearVelocitySim(const hsVector3& vel)
{
    fBody->setLinearVelocity(toBullet(vel));
}

void plBTPhysical::ClearLinearVelocity()
{
    fBody->setLinearVelocity(btVector3(0, 0, 0));
}

hsBool plBTPhysical::GetAngularVelocitySim(hsVector3& vel) const
{
    vel = toPlasma<hsVector3>(fBody->getAngularVelocity());
    return true;
}

void plBTPhysical::SetAngularVelocitySim(const hsVector3& vel)
{
    fBody->setAngularVelocity(toBullet(vel));
}

void plBTPhysical::SetTransform(const hsMatrix44& l2w, const hsMatrix44& w2l, hsBool force)
{
    // make sure the physical is dynamic.
    //  also make sure there is some difference between the matrices...
    // ... but not when a subworld... because the subworld maybe animating and if the object is still then it is actually moving within the subworld
    if (force || (!(fBody->isStaticObject()) && (fWorldKey || !CompareMatrices(l2w, fCachedLocal2World, .0001f))))
    {
        ISetTransformGlobal(l2w);
        plProfile_Inc(SetTransforms);
    }
    else
    {
        if (fBody->isStaticObject()  && plSimulationMgr::fExtraProfile)
            SimLog("Setting transform on non-dynamic: %s.", GetKeyName());
    }
}

void plBTPhysical::GetTransform(hsMatrix44& l2w, hsMatrix44& w2l)
{
    IGetTransformGlobal(l2w);
    l2w.GetInverse(&w2l);
}

hsBool plBTPhysical::Should_I_Trigger(hsBool enter, hsPoint3& pos)
{
    bool trigger;
    bool inside = IsObjectInsideHull(pos);
    if (!inside) {
        trigger = true;
        fInsideConvexHull = enter;
    } else {
        if ( enter && !fInsideConvexHull) {
            trigger = true;
            fInsideConvexHull = enter;
        }
    }
    return trigger;
}

hsBool plBTPhysical::IsObjectInsideHull(const hsPoint3& pos)
{
    // BULLET STUB ???
    return false;
}

void plBTPhysical::SendNewLocation(hsBool synchTransform, hsBool isSynchUpdate)
{
    // we only send if:
    // - the body is active or forceUpdate is on
    // - the mass is non-zero
    // - the physical is not passive
    hsBool bodyActive = false; //!fActor->isSleeping();
    hsBool dynamic = false; //fActor->isDynamic();

    if ((bodyActive || isSynchUpdate) && dynamic)// && fInitialTransform)
    {
        plProfile_Inc(MaySendLocation);

        if (!GetProperty(plSimulationInterface::kPassive))
        {
            hsMatrix44 curl2w = fCachedLocal2World;
            // we're going to cache the transform before sending so we can recognize if it comes back
            IGetTransformGlobal(fCachedLocal2World);

            if (!CompareMatrices(curl2w, fCachedLocal2World, .0001f))
            {
                plProfile_Inc(LocationsSent);
                plProfile_BeginLap(PhysicsUpdates, GetKeyName().c_str());

                if (fCachedLocal2World.GetTranslate().fZ < kMaxNegativeZPos)
                {
                    SimLog("Physical %s fell to %.1f (%.1f is the max).  Suppressing.", GetKeyName(), fCachedLocal2World.GetTranslate().fZ, kMaxNegativeZPos);
                    // Since this has probably been falling for a while, and thus not getting any syncs,
                    // make sure to save it's current pos so we'll know to reset it later
                    DirtySynchState(kSDLPhysical, plSynchedObject::kBCastToClients);
                    IEnable(false);
                }

                hsMatrix44 w2l;
                fCachedLocal2World.GetInverse(&w2l);
                plCorrectionMsg *pCorrMsg = new plCorrectionMsg(GetObjectKey(), fCachedLocal2World, w2l, synchTransform);
                pCorrMsg->Send();
                if (fProxyGen)
                    fProxyGen->SetTransform(fCachedLocal2World, w2l);
                plProfile_EndLap(PhysicsUpdates, GetKeyName().c_str());
            }
        }
    }
}

void plBTPhysical::ApplyHitForce()
{
    if(fBody && fWeWereHit) {
        fBody->applyForce(toBullet(fHitForce), toBullet(fHitPos));
        fWeWereHit = false;
    }
}

//
// TESTING SDL
// Send phys sendState msg to object's plPhysicalSDLModifier
//
hsBool plBTPhysical::DirtySynchState(const char* SDLStateName, uint32_t synchFlags )
{
    if (GetObjectKey())
    {
        plSynchedObject* so=plSynchedObject::ConvertNoRef(GetObjectKey()->ObjectIsLoaded());
        if (so)
        {
            fLastSyncTime = hsTimer::GetSysSeconds();
            return so->DirtySynchState(SDLStateName, synchFlags);
        }
    }

    return false;
}

void plBTPhysical::GetSyncState(hsPoint3& pos, hsQuat& rot, hsVector3& linV, hsVector3& angV)
{
    IGetPositionSim(pos);
    IGetRotationSim(rot);
    GetLinearVelocitySim(linV);
    GetAngularVelocitySim(angV);
}

void plBTPhysical::SetSyncState(hsPoint3* pos, hsQuat* rot, hsVector3* linV, hsVector3* angV)
{
    bool initialSync =  plNetClientApp::GetInstance()->IsLoadingInitialAgeState() &&
        plNetClientApp::GetInstance()->GetJoinOrder() == 0;

    // If the physical has fallen out of the sim, and this is initial age state, and we're
    // the first person in, reset it to the original position.  (ie, prop the default state
    // we've got right now)
    // This hack is bad, and I should feel bad for leaving it in.
    if (pos && pos->fZ < kMaxNegativeZPos && initialSync)
    {
        SimLog("Physical %s loaded out of range state.  Forcing initial state to server.", GetKeyName());
        DirtySynchState(kSDLPhysical, plSynchedObject::kBCastToClients);
        return;
    }

    if (pos)
        ISetPositionSim(*pos);
    if (rot)
        ISetRotationSim(*rot);

    if (linV)
        SetLinearVelocitySim(*linV);
    if (angV)
        SetAngularVelocitySim(*angV);

    SendNewLocation(false, true);
}

void plBTPhysical::ExcludeRegionHack(hsBool cleared)
{
    // BULLET STUB
}

plDrawableSpans* plBTPhysical::CreateProxy(hsGMaterial* mat, hsTArray<uint32_t>& idx, plDrawableSpans* addTo)
{
    hsMatrix44 l2w, dummy;
    GetTransform(l2w, dummy);

    hsBool blended = ((mat->GetLayer(0)->GetBlendFlags() & hsGMatState::kBlendMask));

    switch(fBoundsType) {
    case plSimDefs::kSphereBounds:
        {
            float radius = static_cast<btSphereShape*>(fBody->getCollisionShape())->getRadius();
            addTo = plDrawableGenerator::GenerateSphericalDrawable(hsPoint3(), radius,
                mat, l2w, blended,
                nil, &idx, addTo);
        }
        break;
    case plSimDefs::kBoxBounds:
        {
            hsPoint3 dim = toPlasma<hsPoint3>(static_cast<btBoxShape*>(fBody->getCollisionShape())->getImplicitShapeDimensions());
            addTo = plDrawableGenerator::GenerateBoxDrawable(dim.fX*2.f, dim.fY*2.f, dim.fZ*2.f,
                mat,l2w,blended,
                nil,&idx,addTo);
        }
        break;
    case plSimDefs::kHullBounds:
    case plSimDefs::kProxyBounds:
    case plSimDefs::kExplicitBounds:
        {
            addTo = plDrawableGenerator::GenerateDrawable(fSavedPositions.size(), 
                &(fSavedPositions[0]),
                nil,    // normals - def to avg (smooth) norm
                nil,    // uvws
                0,      // uvws per vertex
                nil,    // colors - def to white
                true,   // do a quick fake shade
                nil,    // optional color modulation
                fSavedIndices.size(),
                &(fSavedIndices[0]),
                mat,
                l2w,
                blended,
                &idx,
                addTo);
        }
        break;
    default:
        break;
    }
    return addTo;
}

void plBTPhysical::IEnable(hsBool enable)
{
    if(enable == fEnabled)
        return;
    fEnabled = enable;
    fProps.SetBit(plSimulationInterface::kDisable, !enable);
    if(!enable) {
        BtScene *scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
        scene->world->removeRigidBody(fBody);
    } else {
        BtScene *scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
        scene->world->addRigidBody(fBody);
    }
}

void plBTPhysical::IGetRotationSim(hsQuat& quat) const
{
    quat = toPlasma(fBody->getWorldTransform().getRotation());
}

void plBTPhysical::IGetPositionSim(hsPoint3& point) const
{
    point = toPlasma<hsPoint3>(fBody->getWorldTransform().getOrigin());
}

void plBTPhysical::ISetRotationSim(const hsQuat& quat)
{
    fBody->getWorldTransform().setRotation(toBullet(quat));
}

void plBTPhysical::ISetPositionSim(const hsPoint3& point)
{
    fBody->getWorldTransform().setOrigin(toBullet(point));
}

void plBTPhysical::IGetTransformGlobal(hsMatrix44 &l2w) const
{
    l2w = toPlasma(fBody->getWorldTransform());
    if (fWorldKey)
    {
        plSceneObject* so = plSceneObject::ConvertNoRef(fWorldKey->ObjectIsLoaded());
        hsAssert(so, "Scene object not loaded while accessing subworld.");
        // We'll hit this at export time, when the ci isn't ready yet, so do a check
        if (so->GetCoordinateInterface())
        {
            const hsMatrix44& s2w = so->GetCoordinateInterface()->GetLocalToWorld();
            l2w = s2w * l2w;
        }
    }
}

void plBTPhysical::ISetTransformGlobal(const hsMatrix44& l2w)
{
    hsAssert(! fBody->isStaticObject(), "Shouldn't move a static actor");

    btTransform trans;

    if (fWorldKey)
    {
        plSceneObject* so = plSceneObject::ConvertNoRef(fWorldKey->ObjectIsLoaded());
        hsAssert(so, "Scene object not loaded while accessing subworld.");
        // physical to subworld (simulation space)
        hsMatrix44 p2s = so->GetCoordinateInterface()->GetWorldToLocal() * l2w;
        trans = toBullet(p2s);
        if (fProxyGen)
        {
            hsMatrix44 w2l;
            p2s.GetInverse(&w2l);
            fProxyGen->SetTransform(p2s, w2l);
        }
    }
    // No need to localize
    else
    {
        trans = toBullet(l2w);
        if (fProxyGen)
        {
            hsMatrix44 w2l;
            l2w.GetInverse(&w2l);
            fProxyGen->SetTransform(l2w, w2l);
        }
    }

    fBody->setWorldTransform(trans);
}
