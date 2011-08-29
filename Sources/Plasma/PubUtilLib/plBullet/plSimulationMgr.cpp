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
#include "plSimulationMgr.h"
#include "plStatusLog/plStatusLog.h"

static plSimulationMgr* gTheInstance = NULL;
bool plSimulationMgr::fExtraProfile = false;
bool plSimulationMgr::fSubworldOptimization = false;
bool plSimulationMgr::fDoClampingOnStep = true;
#ifndef PLASMA_EXTERNAL_RELEASE
    bool plSimulationMgr::fDisplayAwakeActors;
#endif //PLASMA_EXTERNAL_RELEASE

plSimulationMgr::plSimulationMgr()
    : fSuspended(true)
/*    , fMaxDelta(kDefaultMaxDelta)
    , fStepSize(kDefaultStepSize)
    , fLOSDispatch(new plLOSDispatch())
    , fSoundMgr(new plPhysicsSoundMgr)
    , fLog(nil)
*/
{

}

bool plSimulationMgr::InitSimulation()
{
    fLog = plStatusLogMgr::GetInstance().CreateStatusLog(40, "Simulation.log", plStatusLog::kFilledBackground | plStatusLog::kAlignToTop);
    fLog->AddLine("Initialized simulation mgr");
    return true;
}

void plSimulationMgr::Init()
{
    hsAssert(!gTheInstance, "Initializing the sim when it's already been done");
    gTheInstance = new plSimulationMgr();
    if (gTheInstance->InitSimulation())
    {
        gTheInstance->RegisterAs(kSimulationMgr_KEY);
        gTheInstance->GetKey()->RefObject();
    }
    else
    {
        // There was an error when creating the Bullet simulation
        // ...then get rid of the simulation instance
        DEL(gTheInstance); // clean up the memory we allocated
        gTheInstance = nil;
    }
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

plSimulationMgr* plSimulationMgr::GetInstance()
{
    return gTheInstance;
}

hsBool plSimulationMgr::MsgReceive(plMessage *msg)
{
    return hsKeyedObject::MsgReceive(msg);
}

void plSimulationMgr::Advance(float)
{
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