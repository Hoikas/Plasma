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

#include "pfMumbleLink.h"
#include "plAvatar/plArmatureMod.h"
#include "plAvatar/plAvatarMgr.h"
#include "pfCamera/plVirtualCamNeu.h"
#include "plgDispatch.h"
#include "plMessage/plAgeLoadedMsg.h"
#include "pnMessage/plTimeMsg.h"
#include "plNetClient/plNetLinkingMgr.h"
#include "pnNetCommon/plNetApp.h"
#include "pnProduct/pnProduct.h"
#include "plString.h"

#ifdef HS_BUILD_FOR_UNIX
#   include <sys/mman.h>
#   include <sys/stat.h> /* For mode constants */
#   include <fcntl.h>    /* For O_* constants */
#endif

// Mumble likes meters, but feet are customary for Plasma
static const float kFeetPerMeter = 0.3048f;

/* Begin "Hoikas is Lazy" functions */
static inline void _UpdateVec(float* vec, float x = 0.f, float y = 0.f, float z = 0.f)
{
    vec[0] = x * kFeetPerMeter;
    vec[1] = y * kFeetPerMeter;
    vec[2] = z * kFeetPerMeter;
}

static inline void _UpdateVec(float* vec, const hsScalarTriple& pos)
{
    _UpdateVec(vec, pos.fX, pos.fY, pos.fZ);
}
/* End "Hoikas is Lazy" functions */

pfMumbleLink* pfMumbleLink::fInstance = nil;
void pfMumbleLink::Init()
{
    if (!fInstance)
        fInstance = new pfMumbleLink;
    fInstance->RegisterAs(kMumbleLink_KEY);
    plgDispatch::Dispatch()->RegisterForExactType(plAgeLoadedMsg::Index(), fInstance->GetKey());
}

void pfMumbleLink::Shutdown()
{
    plgDispatch::Dispatch()->UnRegisterForExactType(plAgeLoadedMsg::Index(), fInstance->GetKey());
    plgDispatch::Dispatch()->UnRegisterForExactType(plEvalMsg::Index(), fInstance->GetKey()); // just in case
    fInstance->UnRegisterAs(kMumbleLink_KEY); // will unref us
    fInstance = nil;
}

pfMumbleLink::pfMumbleLink()
{
    if (!OpenLink())
        fLink = nil;
}

pfMumbleLink::~pfMumbleLink()
{
    if (fLink && fHandle)
        CloseLink();
}

bool pfMumbleLink::OpenLink()
{
#ifdef HS_BUILD_FOR_WIN32
    fHandle = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
    if (!fHandle)
        return false;

    fLink = (LinkedMemory*)MapViewOfFile(fHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMemory));
    if (!fLink)
    {
        CloseHandle(fHandle);
        fHandle = nil;
        return false;
    }
#else
    plString memname = plString::Format("/MumbleLink.%d", getuid());
    if (fHandle = shm_open(memname.c_str(), O_RDWR, S_IRUSR | S_IWUSR) > -1)
    {
        fLink = (LinkedMemory*)mmap(NULL, sizeof(struct LinkedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fHandle, 0);
        if (fLink == MAP_FAILED)
        {
            shm_unlink(memname.c_str());
            fHandle = nil;
            fLink = nil;
            return false;
        }
    }
#endif

    // This ought to be acceptable?
    _UpdateVec(fLink->m_avatarFront, 0.f, 1.f, 0.f);
    _UpdateVec(fLink->m_avatarTop, 0.f, 0.f, 1.f);

    // Maybe we should use something more descriptive than pnProduct and a literal...
    // TODO for localization when we have a content license, I think.
    if (fLink->m_uiVersion != 2)
    {
        wcsncpy(fLink->m_name, ProductLongName(), arrsize(fLink->m_name));
        wcsncpy(fLink->m_description, L"Positional audio for Uru", arrsize(fLink->m_description));
    }
    return true;
}

bool pfMumbleLink::EnsureLink()
{
    // No link? Try to establish it.
    if (!fLink)
        if (!OpenLink())
            return false;
    return true;
}

void pfMumbleLink::CloseLink()
{
#ifdef HS_BUILD_FOR_WIN32
    CloseHandle(fHandle);
#else
    plString memname = plString::Format("/MumbleLink.%d", getuid());
    shm_unlink(memname.c_str());
#endif

    fHandle = nil;
    fLink = nil;
}

void pfMumbleLink::IUpdate()
{
    if (!EnsureLink())
        return;
    fLink->m_uiTick++;

    // We can change our name on the fly in python, so yeah...
    plString user = plNetClientApp::GetInstance()->GetPlayerName();
    wcsncpy(fLink->m_identity, _TEMP_CONVERT_TO_WCHAR_T(user), arrsize(fLink->m_identity));

    // TODO: include ProductLongName?
    if (plAgeLinkStruct* link = plNetLinkingMgr::GetInstance()->GetAgeLink())
    {
        plString str = link->GetAgeInfo()->GetAgeInstanceGuid()->AsString();
        strncpy(fLink->m_context, str.c_str(), arrsize(fLink->m_context));
        fLink->m_contextLen = strlen(fLink->m_context);
    }

    plArmatureMod* av = plAvatarMgr::GetInstance()->GetLocalAvatar();
    {
        hsPoint3 pos;
        av->GetPositionAndRotationSim(&pos, nil);
        _UpdateVec(fLink->m_avatarPosition, pos);
    }

    plVirtualCam1* cam = plVirtualCam1::Instance();
    {
        hsVector3 view(cam->GetCameraPos() - cam->GetCameraPOA());
        view.Normalize();

        _UpdateVec(fLink->m_cameraFront, view);
        _UpdateVec(fLink->m_cameraPosition, cam->GetCameraPos());
        _UpdateVec(fLink->m_cameraTop, cam->GetCameraUp());
    }
}

void pfMumbleLink::ILinkIn()
{
    // We'll update the link on every frame (eval)
    plgDispatch::Dispatch()->RegisterForExactType(plEvalMsg::Index(), GetKey());
}

void pfMumbleLink::ILinkOut()
{
    // Don't update until we link back in
    plgDispatch::Dispatch()->UnRegisterForExactType(plEvalMsg::Index(), GetKey());

    if (!EnsureLink())
        return;
    _UpdateVec(fLink->m_avatarPosition); // zero it out
    _UpdateVec(fLink->m_cameraPosition);
    fLink->m_contextLen = 0;             // doesn't like strncpy'ing nil
}

hsBool pfMumbleLink::MsgReceive(plMessage* msg)
{
    if (plEvalMsg* pEval = plEvalMsg::ConvertNoRef(msg))
    {
        // A frame has been rendered... Go for it.
        IUpdate();
        return true;
    }

    if (plAgeLoadedMsg* pLoaded = plAgeLoadedMsg::ConvertNoRef(msg))
    {
        if (pLoaded->fLoaded)
        {
            // If it's an offline age (like StartUp), no one will hear us,
            // so it's pointless to update there
            if (!plNetClientApp::GetInstance()->GetFlagsBit(plNetClientApp::kLinkingToOfflineAge))
                ILinkIn();
        }
        else
            ILinkOut();
        return true;
    }

    return hsKeyedObject::MsgReceive(msg);
}
