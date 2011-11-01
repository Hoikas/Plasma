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
#include "plProfile.h"
#include "plSimulationMgr.h"
#include "plLOSDispatch.h"
#include "plPhysical/plPhysicsSoundMgr.h"
#include "plStatusLog/plStatusLog.h"

#include <btBulletDynamicsCommon.h>

/////////////////////////////////////////////////////////////////
//
// DEFAULTS
//
/////////////////////////////////////////////////////////////////

#define kDefaultMaxDelta    0.1         // if the step is greater than .1 seconds, clamp to that
#define kDefaultStepSize    1.f / 60.f  // default simulation freqency is 60hz

// 
// Alloc all the sim timers here so they make a nice pretty display
//
plProfile_CreateTimer(  "Step", "Simulation", Step);
plProfile_CreateCounter("  Awake", "Simulation", Awake);
plProfile_CreateCounter("  Contacts", "Simulation", Contacts);
plProfile_CreateCounter("  DynActors", "Simulation", DynActors);
plProfile_CreateCounter("  DynShapes", "Simulation", DynShapes);
plProfile_CreateCounter("  StaticShapes", "Simulation", StaticShapes);
plProfile_CreateCounter("  Actors", "Simulation", Actors);
plProfile_CreateCounter("  PhyScenes", "Simulation", Scenes);

plProfile_CreateTimer(  "LineOfSight", "Simulation", LineOfSight);
plProfile_CreateTimer(  "ProcessSyncs", "Simulation", ProcessSyncs);
plProfile_CreateTimer(  "UpdateContexts", "Simulation", UpdateContexts);
plProfile_CreateCounter("  MaySendLocation", "Simulation", MaySendLocation);
plProfile_CreateCounter("  LocationsSent", "Simulation", LocationsSent);
plProfile_CreateTimer(  "  PhysicsUpdates","Simulation",PhysicsUpdates);
plProfile_CreateCounter("SetTransforms Accepted", "Simulation", SetTransforms);
plProfile_CreateCounter("AnimatedPhysicals", "Simulation", AnimatedPhysicals);
plProfile_CreateCounter("AnimatedActivators", "Simulation", AnimatedActivators);
plProfile_CreateCounter("Controllers", "Simulation", Controllers);
plProfile_CreateCounter("StepLength", "Simulation", StepLen);

static plSimulationMgr* gTheInstance = NULL;
bool plSimulationMgr::fExtraProfile = false;
bool plSimulationMgr::fSubworldOptimization = false;
bool plSimulationMgr::fDoClampingOnStep = true;
#ifndef PLASMA_EXTERNAL_RELEASE
bool plSimulationMgr::fDisplayAwakeActors;
#endif //PLASMA_EXTERNAL_RELEASE

plSimulationMgr::plSimulationMgr()
    : fSuspended(true)
    , fMaxDelta(kDefaultMaxDelta)
    , fStepSize(kDefaultStepSize)
    , fLOSDispatch(new plLOSDispatch())
    , fSoundMgr(new plPhysicsSoundMgr)
    , fLog(nil)
{
    fLog = plStatusLogMgr::GetInstance().CreateStatusLog(40, "Simulation.log", plStatusLog::kFilledBackground | plStatusLog::kAlignToTop);
    fLog->AddLine("Initialized simulation mgr");
}

void plSimulationMgr::Advance(float delSecs)
{
    for(SceneMap::iterator it = fScenes.begin(); it != fScenes.end(); ++it) {
        it->second->world->stepSimulation(delSecs, 5);
    }
}

hsBool plSimulationMgr::MsgReceive(plMessage* msg)
{
    return hsKeyedObject::MsgReceive(msg);
}

BtScene* plSimulationMgr::GetScene(plKey world)
{
    if(!world)
        world = GetKey();
    BtScene* scene = fScenes[world];
    if(!scene) {
        scene = new BtScene;
        scene->broadphase = new btDbvtBroadphase;
        scene->config = new btDefaultCollisionConfiguration;
        scene->dispatch = new btCollisionDispatcher(scene->config);
        scene->solver = new btSequentialImpulseConstraintSolver;
        scene->world = new btDiscreteDynamicsWorld(scene->dispatch, scene->broadphase, scene->solver, scene->config);
        scene->world->setGravity(btVector3(0, 0, -32.174049f));
        fScenes[world] = scene;
    }
    return scene;
}

void plSimulationMgr::ReleaseScene(plKey world)
{
    if(!world)
        world = GetKey();
    SceneMap::iterator it = fScenes.find(world);
    hsAssert(it != fScenes.end(), "Unknown scene");
    if (it != fScenes.end())
    {
        BtScene* scene = it->second;
        if (scene->world->getNumCollisionObjects() == 0) {
            delete scene->world;
            delete scene->solver;
            delete scene->dispatch;
            delete scene->config;
            delete scene->broadphase;
            fScenes.erase(it);
        }
    }
}

void plSimulationMgr::Log(const char * fmt, ...)
{
    if(gTheInstance)
    {
        plStatusLog* log = GetInstance()->fLog;
        if(log)
        {
            va_list args;
            va_start(args, fmt);
            log->AddLineV(fmt, args);
            va_end(args);
        }
    }
}

void plSimulationMgr::LogV(const char* formatStr, va_list args)
{
    if(gTheInstance)
    {
        plStatusLog * log = GetInstance()->fLog;
        if(log)
        {
            log->AddLineV(formatStr, args);
        }
    }
}

void plSimulationMgr::ClearLog()
{
    if(gTheInstance)
    {
        plStatusLog *log = GetInstance()->fLog;
        if(log)
        {
            log->Clear();
        }
    }
}

plSimulationMgr* plSimulationMgr::GetInstance()
{
    return gTheInstance;
}

void plSimulationMgr::Init()
{
    hsAssert(!gTheInstance, "Initializing the sim when it's already been done");
    gTheInstance = new plSimulationMgr();
    gTheInstance->RegisterAs(kSimulationMgr_KEY);
}

// when the app is going away completely
void plSimulationMgr::Shutdown()
{
    hsAssert(gTheInstance, "Simulation manager missing during shutdown.");
    if (gTheInstance)
    {
        gTheInstance->UnRegister();     // this will destroy the instance
        gTheInstance = nil;
    }
}
