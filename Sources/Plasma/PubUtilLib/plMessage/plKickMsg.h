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

#ifndef plKickMsg_h_inc
#define plKickMsg_h_inc

#include "HeadSpin.h"
#include "hsStream.h"
#include "plString.h"
#include "pnMessage/plMessage.h"
#include "pnNetCommon/plNetApp.h"

/**
 * Message sent to kick unruly clients from owned ages
 *
 * This message is dispatched only to clients that are being kicked from the age.
 * The kickee is responsible for linking himself out of the age in a nice manner. Naturually,
 * this can be circumvented by hacked clients, but if the kickee knows how to hack their client
 * to disable processing of this message, then we have much larger problems on our hands than
 * simple griefing...
 */
class plKickMsg : public plMessage
{
    plString fReason;

    void Init()
    {
        if (plNetClientApp* nc = plNetClientApp::GetInstance())
            fSender = nc->GetLocalPlayerKey();
        SetBCastFlag(kBCastByExactType, true);
        SetBCastFlag(kLocalPropagate, false);
        SetBCastFlag(kNetPropagate, true);
    }

public:
    plKickMsg()
    {
        Init();
    }

    plKickMsg(const plString& reason)
        : fReason(reason)
    {
        Init();
    }

    CLASSNAME_REGISTER(plKickMsg);
    GETINTERFACE_ANY(plKickMsg, plMessage);

    void Read(hsStream* S, hsResMgr* mgr)
    {
        plMessage::IMsgRead(S, mgr);
        fReason = S->ReadSafeWString_TEMP();
    }

    void Write(hsStream* S, hsResMgr* mgr)
    {
        hsAssert(fSender, "nil sender? you must like wasting your time...");
        plMessage::IMsgWrite(S, mgr);
        S->WriteSafeWString(fReason);
    }

    plString GetReason() const { return fReason; }
};

#endif // plKickMsg_h_inc
