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

#define LogActivate(func) if (fActor->isSleeping()) SimLog("%s activated by %s", GetKeyName(), func);
#define kMaxNegativeZPos -2000.f

plProfile_Extern(MaySendLocation);
plProfile_Extern(LocationsSent);
plProfile_Extern(PhysicsUpdates);

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
    //, convexMesh(nil)
    //, triMesh(nil)
    , radius(0.f)
    , offset(0.f, 0.f, 0.f)
    , meshStream(nil)
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
    , fSaveTriangles(nil)
    , fHullNumberPlanes(0)
    , fMass(0.f)
    , fWeWereHit(false)
    , fHitForce(0,0,0)
    , fHitPos(0,0,0)
    , fInsideConvexHull(false)
{
}

plBTPhysical::~plBTPhysical()
{
    plSimulationMgr::Log("Destroying physical %s", GetKeyName());

/*    if (fActor)
    {
    // Grab any mesh we may have (they need to be released manually)
    NxConvexMesh* convexMesh = nil;
    NxTriangleMesh* triMesh = nil;
    NxShape* shape = fActor->getShapes()[0];
    if (NxConvexShape* convexShape = shape->isConvexMesh())
    convexMesh = &convexShape->getConvexMesh();
    else if (NxTriangleMeshShape* trimeshShape = shape->isTriangleMesh())
    triMesh = &trimeshShape->getTriangleMesh();

    if (!fActor->isDynamic())
    plPXPhysicalControllerCore::RebuildCache();

    if (fActor->isDynamic() && fActor->readBodyFlag(NX_BF_KINEMATIC))
    {
    if (fGroup == plSimDefs::kGroupDynamic)
    fNumberAnimatedPhysicals--;
    else
    fNumberAnimatedActivators--;
    }

    // Release the actor
    NxScene* scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
    scene->releaseActor(*fActor);
    fActor = nil;

    // Now that the actor is freed, release the mesh
    if (convexMesh)
    plSimulationMgr::GetInstance()->GetSDK()->releaseConvexMesh(*convexMesh);
    if (triMesh)
    plSimulationMgr::GetInstance()->GetSDK()->releaseTriangleMesh(*triMesh);

    // Release the scene, so it can be cleaned up if no one else is using it
    plSimulationMgr::GetInstance()->ReleaseScene(fWorldKey);
    } */

    if (fWorldHull)
        delete [] fWorldHull;
    if (fSaveTriangles)
        delete [] fSaveTriangles;

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

    return true;
}

/////////////////////////////////////////////////////////////////////
//
// READING AND WRITING
//
/////////////////////////////////////////////////////////////////////

void plBTPhysical::Read(hsStream* stream, hsResMgr* mgr)
{
    plPhysical::Read(stream, mgr);  
    ClearMatrix(fCachedLocal2World);

    PhysRecipe recipe;
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
/*        // Read in the cooked mesh
        plPXStream pxs(stream);
        if (recipe.bounds == plSimDefs::kHullBounds)
        recipe.convexMesh = plSimulationMgr::GetInstance()->GetSDK()->createConvexMesh(pxs);
        else
        recipe.triMesh = plSimulationMgr::GetInstance()->GetSDK()->createTriangleMesh(pxs);
        */
    }

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
}

void plBTPhysical::Write(hsStream* stream, hsResMgr* mgr)
{
    plPhysical::Write(stream, mgr);
    hsAssert(0, "plBTPhysical can't be written yet.");
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
/*        if (fActor->isDynamic())
        {
            // if the body is already unpinned and you unpin it again,
            // you'll wipe out its velocity. hence the check.
            hsBool current = fActor->readBodyFlag(NX_BF_FROZEN);
            if (status != current)
            {
                if (status)
                    fActor->raiseBodyFlag(NX_BF_FROZEN);
                else
                {
                    fActor->clearBodyFlag(NX_BF_FROZEN);
                    LogActivate("SetProperty");
                    fActor->wakeUp();
                }
            }
        }*/
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
    vel = hsVector3(0.f, 0.f, 0.f);
    return false;
    // BULLET STUB
}

void plBTPhysical::SetLinearVelocitySim(const hsVector3& vel)
{
    // BULLET STUB
}

void plBTPhysical::ClearLinearVelocity()
{
    // BULLET STUB
}

hsBool plBTPhysical::GetAngularVelocitySim(hsVector3& vel) const
{
    vel = hsVector3(0.f, 0.f, 0.f);
    return false;
    // BULLET STUB
}

void plBTPhysical::SetAngularVelocitySim(const hsVector3& vel)
{
    // BULLET STUB
}

void plBTPhysical::SetTransform(const hsMatrix44& l2w, const hsMatrix44& w2l, hsBool force)
{
    // make sure the physical is dynamic.
    //  also make sure there is some difference between the matrices...
    // ... but not when a subworld... because the subworld maybe animating and if the object is still then it is actually moving within the subworld
    if (force /*|| (fActor->isDynamic() && (fWorldKey || !CompareMatrices(l2w, fCachedLocal2World, .0001f))) */)
    {
        ISetTransformGlobal(l2w);
//      plProfile_Inc(SetTransforms);
    }
    else
    {
        /*if (!fActor->isDynamic()  && plSimulationMgr::fExtraProfile) */
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
    return false;
    // BULLET STUB
}

hsBool plBTPhysical::IsObjectInsideHull(const hsPoint3& pos)
{
    return false;
    // BULLET STUB
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

                // quick peek at the translation...last time it was corrupted because we applied a non-unit quaternion
//              hsAssert(real_finite(fCachedLocal2World.fMap[0][3]) &&
//                       real_finite(fCachedLocal2World.fMap[1][3]) &&
//                       real_finite(fCachedLocal2World.fMap[2][3]), "Bad transform outgoing");

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
    // BULLET STUB
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
    return addTo;
    // BULLET STUB
}

void plBTPhysical::IEnable(hsBool enable)
{
    //BULLET STUB
}

void plBTPhysical::IGetRotationSim(hsQuat& quat) const
{
    quat = hsQuat(0.f, 0.f, 0.f, 1.f);
    // BULLET STUB
}

void plBTPhysical::IGetPositionSim(hsPoint3& point) const
{
    point = hsPoint3(0.f, 0.f, 0.f);
    // BULLET STUB
}

void plBTPhysical::ISetRotationSim(const hsQuat&)
{
    // BULLET STUB
}

void plBTPhysical::ISetPositionSim(const hsPoint3&)
{
    // BULLET STUB
}

void plBTPhysical::IGetTransformGlobal(hsMatrix44 &l2w) const
{
    l2w = hsMatrix44::IdentityMatrix();
    // BULLET STUB
}

void plBTPhysical::ISetTransformGlobal(const hsMatrix44& l2w)
{
}
