/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

*==LICENSE==*/

#ifndef _pfMumbleLink_h_
#define _pfMumbleLink_h_

#include "HeadSpin.h"
#include "pnKeyedObject/hsKeyedObject.h"

class pfMumbleLink : hsKeyedObject
{
#ifdef HS_BUILD_FOR_WIN32
    typedef HANDLE LinkHandle;
#else
    typedef int32_t LinkHandle;
#endif

    struct LinkedMemory
    {
	    uint32_t      m_uiVersion;
	    uint32_t      m_uiTick;
	    float         m_avatarPosition[3];
	    float         m_avatarFront[3];
	    float         m_avatarTop[3];
	    wchar_t       m_name[256];
	    float         m_cameraPosition[3];
	    float         m_cameraFront[3];
	    float         m_cameraTop[3];
	    wchar_t       m_identity[256];
	    uint32_t      m_contextLen;
	    char          m_context[256];
	    wchar_t       m_description[2048];
    };

    static pfMumbleLink* fInstance;
    LinkedMemory*        fLink;
    LinkHandle           fHandle;

    bool OpenLink();
    bool EnsureLink();
    void IUpdate();
    void CloseLink();

    void ILinkIn();
    void ILinkOut();

public:

    pfMumbleLink();
    ~pfMumbleLink();

    CLASSNAME_REGISTER(pfMumbleLink);
    GETINTERFACE_ANY(pfMumbleLink, hsKeyedObject);

    virtual hsBool MsgReceive(plMessage* msg);

    static void Init();
    static pfMumbleLink* GetInstance() { return fInstance; }
    static void Shutdown();
};

#endif // _pfMumbleLink_h_
