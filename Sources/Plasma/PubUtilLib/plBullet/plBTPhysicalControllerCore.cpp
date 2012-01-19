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
#include "plBTPhysicalControllerCore.h"
#include "plSimulationMgr.h"
#include "plBTPhysical.h"
#include "pnSceneObject/plSimulationInterface.h"
#include "pnSceneObject/plSceneObject.h"
#include "pnMessage/plCorrectionMsg.h"
#include "plAvatar/plArmatureMod.h"
#include "pnSceneObject/plCoordinateInterface.h"
#include "plDrawable/plDrawableGenerator.h"
#include "plPhysical/plPhysicalProxy.h"
#include "pnMessage/plSetNetGroupIDMsg.h"
#include "plMessage/plCollideMsg.h"
#include "plModifier/plDetectorLog.h"

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#define kPhysxSkinWidth 0.1f
#define kPhysZOffset ((fRadius + (fHeight / 2)) + kPhysxSkinWidth)

#define AvatarMass 200.f

static std::vector<plBTPhysicalControllerCore*> gControllers;

plPhysicalControllerCore* plPhysicalControllerCore::Create(plKey ownerSO, float height, float width)
{
    float radius = width / 2.f;
    float realHeight = height - width;
    return new plBTPhysicalControllerCore(ownerSO, realHeight,radius);
    return nil;
}

plBTPhysicalControllerCore::plBTPhysicalControllerCore(plKey ownerSO, float height, float radius)
    : plPhysicalControllerCore(ownerSO, height, radius)
{
    fLastGlobalLoc.Reset();
    ICreateController();
    Enable(false);
    gControllers.push_back(this);
}

plBTPhysicalControllerCore::~plBTPhysicalControllerCore()
{
}

void plBTPhysicalControllerCore::Move(hsVector3 displacement, unsigned int collideWith, unsigned int &collisionResults)
{
    collisionResults=0;
    fLastGlobalLoc.Translate(&displacement);
    btTransform trans = fBody->getWorldTransform();
    trans.setOrigin(trans.getOrigin()+btVector3(displacement.fX, displacement.fY, displacement.fZ));
    fBody->setWorldTransform(trans);
}

void plBTPhysicalControllerCore::Enable(bool enable)
{
    if (fEnabled != enable)
    {
        fEnabled = enable;
        if (fEnabled)
            fEnableChanged = true;
        else
        {
            fBody->setCollisionFlags(fBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
            fBody->setLinearFactor(btVector3(0.f, 0.f, 0.f));
            fBody->setAngularFactor(btVector3(0.f, 0.f, 0.f));
        }
    }
}

void plBTPhysicalControllerCore::SetSubworld(plKey world) 
{   
    if (fWorldKey != world)
    {
        bool wasEnabled = fEnabled;
        //need to inform detectors in the old world that we are leaving
        //IInformDetectors(false);
        //done informing old world
        SimLog("Changing subworlds!");
        IDeleteController();
        SimLog("Deleted old controller");
        fWorldKey = world;
        if (GetSubworldCI())
            fPrevSubworldW2L = GetSubworldCI()->GetWorldToLocal();
        // Update our subworld position and rotation
        const plCoordinateInterface* subworldCI = GetSubworldCI();
        if (subworldCI)
        {
            const hsMatrix44& w2s = fPrevSubworldW2L;
            hsMatrix44 l2s = w2s * fLastGlobalLoc;
            l2s.GetTranslate(&fLocalPosition);
            fLocalRotation.SetFromMatrix44(l2s);
        }
        else
        {
            fLastGlobalLoc.GetTranslate(&fLocalPosition);
            fLocalRotation.SetFromMatrix44(fLastGlobalLoc);
        }
        hsMatrix44 w2l;
        fLastGlobalLoc.GetInverse(&w2l);
        //        if (fProxyGen)
        //            fProxyGen->SetTransform(fLastGlobalLoc, w2l);
        // Update the physical position
        SimLog("creating new controller");
        hsPoint3 PositionPlusOffset=fLocalPosition;
        PositionPlusOffset.fZ +=kPhysZOffset;
        //placing new controller and kinematic in the appropriate location
        ICreateController(PositionPlusOffset);
        //        RebuildCache();
    }
}

const plCoordinateInterface* plBTPhysicalControllerCore::GetSubworldCI() const 
{
    if (fWorldKey)
    {
        plSceneObject* so = plSceneObject::ConvertNoRef(fWorldKey->ObjectIsLoaded());
        if (so)
            return so->GetCoordinateInterface();
    }
    return nil;
}

void plBTPhysicalControllerCore::GetState(hsPoint3& pos, float& zRot)
{   
    // Temporarily use the position point while we get the z rotation
    fLocalRotation.NormalizeIfNeeded();
    fLocalRotation.GetAngleAxis(&zRot, (hsVector3*)&pos);

    if (pos.fZ < 0)
        zRot = (2 * M_PI) - zRot; // axis is backwards, so reverse the angle too

    pos = fLocalPosition;
}

void plBTPhysicalControllerCore::SetState(const hsPoint3& pos, float zRot)
{
    plSceneObject* so = plSceneObject::ConvertNoRef(fOwner->ObjectIsLoaded());
    if (so)
    {
        hsQuat worldRot;
        hsVector3 zAxis(0.f, 0.f, 1.f);
        worldRot.SetAngleAxis(zRot, zAxis);

        hsMatrix44 l2w, w2l;
        worldRot.MakeMatrix(&l2w);
        l2w.SetTranslate(&pos);

        // Localize new position and rotation to global coords if we're in a subworld
        const plCoordinateInterface* ci = GetSubworldCI();
        if (ci)
        {
            const hsMatrix44& subworldL2W = ci->GetLocalToWorld();
            l2w = subworldL2W * l2w;
        }
        l2w.GetInverse(&w2l);
        so->SetTransform(l2w, w2l);
        so->FlushTransform();
    }
}

void plBTPhysicalControllerCore::Kinematic(bool state)
{
    if (fKinematic != state)
    {
        fKinematic = state;
    }
}

bool plBTPhysicalControllerCore::IsKinematic()
{
    return fKinematic;
}

void plBTPhysicalControllerCore::GetKinematicPosition(hsPoint3& pos)
{
    btTransform trans;
    fBody->getMotionState()->getWorldTransform(trans);
    btVector3 out = trans.getOrigin();
    pos.fX = out.x();
    pos.fY = out.y();
    pos.fZ = out.z();
}

void plBTPhysicalControllerCore::HandleEnableChanged()
{
    fEnableChanged = false;

    fBody->setCollisionFlags(fBody->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
    fBody->setLinearFactor(btVector3(1.f, 1.f, 1.f));
    fBody->setAngularFactor(btVector3(0.f, 0.f, 1.f));
}

void plBTPhysicalControllerCore::HandleKinematicChanged()
{
    fKinematicChanged = false;
}

void plBTPhysicalControllerCore::HandleKinematicEnableNextUpdate()
{
    fBody->setCollisionFlags(fBody->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
    fKinematicEnableNextUpdate = false;
}

void plBTPhysicalControllerCore::MoveKinematicToController(hsPoint3& pos)
{
    btVector3 kinPos = fBody->getWorldTransform().getOrigin();
    if ( abs(kinPos.x()-pos.fX) + abs(kinPos.y()-pos.fY) + (abs(kinPos.z()-pos.fZ+kPhysZOffset)) > 0.0001f)
    {
        btVector3 newPos(pos.fX, pos.fY, pos.fZ+kPhysZOffset);
        fBody->getWorldTransform().setOrigin(newPos);
    }
}

void plBTPhysicalControllerCore::UpdateControllerAndPhysicalRep()
{
    GetPositionSim(fLocalPosition);
}

void plBTPhysicalControllerCore::SetControllerDimensions(float radius, float height)
{
    fNeedsResize=false;
    if(fRadius!=radius)
    {
        fNeedsResize=true;
    }
    if(fHeight!=height)
    {
        fNeedsResize=true;
    }
    fPreferedRadius=radius;
    fPreferedHeight=height;
}

void plBTPhysicalControllerCore::LeaveAge()
{
    SetPushingPhysical(nil);
    if(fWorldKey) this->SetSubworld(nil);
    this->fMovementInterface->LeaveAge();
}

int plBTPhysicalControllerCore::SweepControllerPath(const hsPoint3& startPos, const hsPoint3& endPos, hsBool vsDynamics, hsBool vsStatics, 
    uint32_t& vsSimGroups, std::multiset< plControllerSweepRecord >& WhatWasHitOut)
{
/*    NxCapsule tempCap;
    tempCap.p0 =plPXConvert::Point( startPos);
    tempCap.p0.z = tempCap.p0.z + fPreferedRadius;
    tempCap.radius = fPreferedRadius ;
    tempCap.p1 = tempCap.p0;
    tempCap.p1.z = tempCap.p1.z + fPreferedHeight;

    NxVec3 vec;
    vec.x = endPos.fX - startPos.fX;
    vec.y = endPos.fY - startPos.fY;
    vec.z = endPos.fZ - startPos.fZ;

    int numberofHits = 0;
    int HitsReturned = 0;
    WhatWasHitOut.clear();
    NxScene *myscene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
    NxSweepQueryHit whatdidIhit[10];
    unsigned int flags = NX_SF_ALL_HITS;
    if(vsDynamics)
        flags |= NX_SF_DYNAMICS;
    if(vsStatics)
        flags |= NX_SF_STATICS;
    numberofHits = myscene->linearCapsuleSweep(tempCap, vec, flags, nil, 10, whatdidIhit, nil, vsSimGroups);
    if(numberofHits)
    {//we hit a dynamic object lets make sure it is not animatable
        for(int i=0; i<numberofHits; i++)
        {
            plControllerSweepRecord CurrentHit;
            CurrentHit.ObjHit=(plPhysical*)whatdidIhit[i].hitShape->getActor().userData;
            CurrentHit.Norm.fX = whatdidIhit[i].normal.x;
            CurrentHit.Norm.fY = whatdidIhit[i].normal.y;
            CurrentHit.Norm.fZ = whatdidIhit[i].normal.z;
            if(CurrentHit.ObjHit != nil)
            {
                hsPoint3 where;
                where.fX = whatdidIhit[i].point.x;
                where.fY = whatdidIhit[i].point.y;
                where.fZ = whatdidIhit[i].point.z;
                CurrentHit.locHit = where;
                CurrentHit.TimeHit = whatdidIhit[i].t ;
                WhatWasHitOut.insert(CurrentHit);
                HitsReturned++;
            }
        }
    } 

    return HitsReturned; */
    return 0; // BULLET STUB
}

void plBTPhysicalControllerCore::BehaveLikeAnimatedPhysical(hsBool actLikeAnAnimatedPhys)
{
 /*    hsAssert(fKinematicActor, "Changing behavior, but plPXPhysicalControllerCore has no Kinematic actor associated with it");
    if(fBehavingLikeAnimatedPhys!=actLikeAnAnimatedPhys)
    {
        fBehavingLikeAnimatedPhys=actLikeAnAnimatedPhys;
        if(fKinematicActor)
        {
            if(actLikeAnAnimatedPhys)
            {
                //need to set BX Kinematic if true and kill any rotation
                fController->setCollision(fEnabled);
                fKinematicActor->raiseBodyFlag(NX_BF_KINEMATIC);
                fKinematicActor->clearBodyFlag(NX_BF_FROZEN_ROT);
                fKinematicActor->raiseBodyFlag(NX_BF_DISABLE_GRAVITY);
            }
            else
            {
                //don't really use the controller now don't bother with collisions 
                fController->setCollision(false);
                fKinematicActor->clearBodyFlag(NX_BF_KINEMATIC);
                fKinematicActor->raiseBodyFlag(NX_BF_FROZEN_ROT_X);
                fKinematicActor->raiseBodyFlag(NX_BF_FROZEN_ROT_Y);
                fKinematicActor->clearBodyFlag(NX_BF_DISABLE_GRAVITY);
            }
        }
    } */
}

hsBool plBTPhysicalControllerCore::BehavingLikeAnAnimatedPhysical()
{
    //    hsAssert(fKinematicActor, "plPXPhysicalControllerCore is missing a kinematic actor");
    return fBehavingLikeAnimatedPhys;
}

const hsVector3& plBTPhysicalControllerCore::GetLinearVelocity()
{
    btVector3 vel = fBody->getLinearVelocity();
    fLinearVelocity.fX = vel.x();
    fLinearVelocity.fY = vel.y();
    fLinearVelocity.fZ = vel.z();
    if (!BehavingLikeAnAnimatedPhysical()) {

    }
    return fLinearVelocity;
}

void plBTPhysicalControllerCore::SetLinearVelocity(const hsVector3& vel)
{
    plPhysicalControllerCore::SetLinearVelocity(vel);
    fBody->setLinearVelocity(btVector3(vel.fX, vel.fY, vel.fZ));
    if(!BehavingLikeAnAnimatedPhysical()) {

    }
}

void plBTPhysicalControllerCore::SetAngularVelocity(const float val)
{
    plPhysicalControllerCore::SetAngularVelocity(val);
    fBody->setAngularVelocity(btVector3(0.f, 0.f, val));
    if(!BehavingLikeAnAnimatedPhysical()) {

    }
}
void plBTPhysicalControllerCore::SetVelocities(const hsVector3& linearVel, float angVel)
{
    SetLinearVelocity(linearVel);
    SetAngularVelocity(angVel);
}

void plBTPhysicalControllerCore::ISetKinematicLoc(const hsMatrix44& l2w)
{
    fLastGlobalLoc = l2w;
    hsPoint3 kPos;
    // Update our subworld position and rotation
    const plCoordinateInterface* subworldCI = GetSubworldCI();
    if (subworldCI)
    {
        const hsMatrix44& w2s = subworldCI->GetWorldToLocal();
        hsMatrix44 l2s = w2s * l2w;

        l2s.GetTranslate(&kPos);
    }
    else
    {
        l2w.GetTranslate(&kPos);
    }

    hsMatrix44 w2l;
    l2w.GetInverse(&w2l);
    /*    if (fProxyGen)
    fProxyGen->SetTransform(l2w, w2l); */

    // add z offset
    kPos.fZ += kPhysZOffset;
    // Update the physical position of kinematic
    fBody->getWorldTransform().setOrigin(btVector3(kPos.fX, kPos.fY, kPos.fZ));
    /*    if (fEnabled|| fKinematic)
        fKinematicActor->moveGlobalPosition(plPXConvert::Point(kPos));
    else
        fKinematicActor->setGlobalPosition(plPXConvert::Point(kPos)); */
}

void plBTPhysicalControllerCore::ISetGlobalLoc(const hsMatrix44& l2w)
{
    fLastGlobalLoc = l2w;
    // Update our subworld position and rotation
    const plCoordinateInterface* subworldCI = GetSubworldCI();
    if (subworldCI)
    {
        const hsMatrix44& w2s = fPrevSubworldW2L;
        hsMatrix44 l2s = w2s * l2w;

        l2s.GetTranslate(&fLocalPosition);
        fLocalRotation.SetFromMatrix44(l2s);
    }
    else
    {
        l2w.GetTranslate(&fLocalPosition);
        fLocalRotation.SetFromMatrix44(l2w);
    }
    hsMatrix44 w2l;
    l2w.GetInverse(&w2l);
    /*    if (fProxyGen)
    fProxyGen->SetTransform(l2w, w2l);
    // Update the physical position
    NxExtendedVec3 nxPos(fLocalPosition.fX, fLocalPosition.fY, fLocalPosition.fZ + kPhysZOffset);
    fController->setPosition(nxPos); */
    fBody->getWorldTransform().setOrigin(btVector3(fLocalPosition.fX, fLocalPosition.fY, fLocalPosition.fZ + kPhysZOffset));
    IMatchKinematicToController();
}

void plBTPhysicalControllerCore::IMatchKinematicToController()
{
/*    if ( fKinematicActor)
    {
        NxExtendedVec3 cPos = fController->getPosition();
        NxVec3 prevKinPos = fKinematicActor->getGlobalPosition();
        NxVec3 kinPos;
        kinPos.x = (NxReal)cPos.x;
        kinPos.y = (NxReal)cPos.y;
        kinPos.z = (NxReal)cPos.z;
        if (plSimulationMgr::fExtraProfile)
        SimLog("Match setting kinematic from %f,%f,%f to %f,%f,%f",prevKinPos.x,prevKinPos.y,prevKinPos.z,kinPos.x,kinPos.y,kinPos.z );
        fKinematicActor->setGlobalPosition(kinPos);
    }*/
}

void plBTPhysicalControllerCore::IGetPositionSim(hsPoint3& pos) const
{
    btVector3 btPos = fBody->getWorldTransform().getOrigin();
    pos.Set(btPos.x(), btPos.y(), btPos.z());
    if(this->fBehavingLikeAnimatedPhys)
    {
//        const NxExtendedVec3& nxPos = fController->getPosition();
//        pos.Set(float(nxPos.x), float(nxPos.y), float(nxPos.z) - kPhysZOffset);
    }
    else
    {
//        NxVec3 Pos = fKinematicActor->getGlobalPosition();
//        pos.Set(float(Pos.x), float(Pos.y), float(Pos.z) - kPhysZOffset);
    }
}

void plBTPhysicalControllerCore::ICreateController()
{
    BtScene *scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);

    btCapsuleShape *shape = new btCapsuleShape(fRadius, fHeight);
    btVector3 inertia;
    shape->calculateLocalInertia(AvatarMass, inertia);
    btRigidBody::btRigidBodyConstructionInfo cinfo(AvatarMass, new btDefaultMotionState, shape, inertia);
    cinfo.m_friction = 0;
    cinfo.m_restitution = 0;
    fBody = new btRigidBody(cinfo);
    fBody->setUserPointer(this);
    fBody->setCollisionFlags(fBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    fBody->setAngularFactor(btVector3(0.f, 0.f, 1.f));

    short colgroups = 0;
    colgroups |= 1 << plSimDefs::kGroupAvatarBlocker;
    colgroups |= 1 << plSimDefs::kGroupDetector;
    colgroups |= 1 << plSimDefs::kGroupDynamic;
    colgroups |= 1 << plSimDefs::kGroupStatic;
    scene->world->addRigidBody(fBody, 1 << plSimDefs::kGroupAvatar, colgroups);
}

void plBTPhysicalControllerCore::ICreateController(const hsPoint3& origin)
{
    BtScene *scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);

    btCapsuleShape *shape = new btCapsuleShape(fRadius, fHeight);
    btVector3 inertia;
    shape->calculateLocalInertia(AvatarMass, inertia);
    btRigidBody::btRigidBodyConstructionInfo cinfo(AvatarMass, new btDefaultMotionState, shape, inertia);
    cinfo.m_friction = 0;
    cinfo.m_restitution = 0;
    fBody = new btRigidBody(cinfo);
    fBody->setUserPointer(this);
    fBody->setCollisionFlags(fBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    fBody->setAngularFactor(btVector3(0.f, 0.f, 1.f));
    fBody->getWorldTransform().setOrigin(btVector3(origin.fX, origin.fY, origin.fZ));

    short colgroups = 0;
    colgroups |= 1 << plSimDefs::kGroupAvatarBlocker;
    colgroups |= 1 << plSimDefs::kGroupDetector;
    colgroups |= 1 << plSimDefs::kGroupDynamic;
    colgroups |= 1 << plSimDefs::kGroupStatic;
    scene->world->addRigidBody(fBody, 1 << plSimDefs::kGroupAvatar, colgroups);
}

void plBTPhysicalControllerCore::IDeleteController()
{
    BtScene* scene = plSimulationMgr::GetInstance()->GetScene(fWorldKey);
    scene->world->removeRigidBody(fBody);
    delete fBody->getMotionState();
    btCollisionShape *shape = fBody->getCollisionShape();
    delete shape;
    delete fBody;
}