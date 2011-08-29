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
#ifndef plSimulationMgr_H
#define plSimulationMgr_H

#include "pnKeyedObject/hsKeyedObject.h"

class plStatusLog;

class plSimulationMgr : public hsKeyedObject {
public:
    CLASSNAME_REGISTER(plSimulationMgr);
    GETINTERFACE_ANY(plSimulationMgr, hsKeyedObject);

    plSimulationMgr();

    // initialiation of the PhysX simulation
    virtual bool InitSimulation();

    // Advance the simulation by the given number of seconds
    void Advance(float delSecs);

    hsBool MsgReceive(plMessage* msg);

    // The simulation won't run at all if it is suspended
    void Suspend() { fSuspended = true; }
    void Resume() { fSuspended = false; }
    bool IsSuspended() { return fSuspended; }

    // Output the given debug text to the simulation log.
    static void Log(const char* formatStr, ...);
    static void LogV(const char* formatStr, va_list args);
    static void ClearLog();

    static plSimulationMgr* GetInstance();
    static void Init();
    static void Shutdown();

    static bool fExtraProfile;
    static bool fSubworldOptimization;
    static bool fDoClampingOnStep;

#ifndef PLASMA_EXTERNAL_RELEASE
    static bool fDisplayAwakeActors;
#endif //PLASMA_EXTERNAL_RELEASE
protected:
    bool fSuspended;
    plStatusLog *fLog;
};

#define SIM_VERBOSE

#ifdef SIM_VERBOSE
#include <stdarg.h>     // only include when we need to call plSimulationMgr::Log

inline void SimLog(const char *str, ...)
{
    va_list args;
    va_start(args, str);
    plSimulationMgr::LogV(str, args);
    va_end(args);
}

#else

inline void SimLog(const char *str, ...)
{
    // will get stripped out
}

#endif

#endif
