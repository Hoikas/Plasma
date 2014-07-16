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

#include "plNetClientMgr.h"

#include "plMessage/plAvatarMsg.h"
#include "pnMessage/plEnableMsg.h"
#include "plMessage/plIgnorePlayerMsg.h"

#include "plAvatar/plArmatureMod.h"
#include "plAvatar/plAvatarMgr.h"
#include "plNetTransport/plNetTransportMember.h"
#include "plVault/plVault.h"

// ===================================================

/** Catches changes to the player's ignore list */
class IgnoreListCallback : public VaultCallback
{
    friend class plNetClientMgr;
    static IgnoreListCallback* fInstance;

public:
    void AddedChildNode(RelVaultNode* parent, RelVaultNode* child)
    {
        if (parent == VaultGetIgnoreListFolder()) {
            VaultPlayerInfoNode ignoree(child);
            uint32_t flags = VaultGetMutualIgnore() ? plIgnorePlayerMsg::kMutualIgnore : 0;
            plIgnorePlayerMsg* msg = new plIgnorePlayerMsg(VaultGetPlayerId(), ignoree.GetPlayerId(), flags);
            msg->Send();
        }
    }

    void RemovingChildNode(RelVaultNode* parent, RelVaultNode* child)
    {
        if (parent == VaultGetIgnoreListFolder()) {
            VaultPlayerInfoNode ignoree(child);
            uint32_t flags = plIgnorePlayerMsg::kUnIgnore | (VaultGetMutualIgnore() ? plIgnorePlayerMsg::kMutualIgnore : 0);
            plIgnorePlayerMsg* msg = new plIgnorePlayerMsg(VaultGetPlayerId(), ignoree.GetPlayerId(), flags);
            msg->Send();
        }
    }

    void ChangedNode(RelVaultNode*) { }
};
IgnoreListCallback* IgnoreListCallback::fInstance = nullptr;

void plNetClientMgr::ISetupIgnore()
{
    hsAssert(!IgnoreListCallback::fInstance, "should only init IgnoreListCallback once!");
    IgnoreListCallback::fInstance = new IgnoreListCallback();
    VaultRegisterCallback(IgnoreListCallback::fInstance);
}

void plNetClientMgr::IDestroyIgnore()
{
    hsAssert(IgnoreListCallback::fInstance, "trying to destroy uninited IgnoreListCallbacks");
    VaultUnregisterCallback(IgnoreListCallback::fInstance);
    delete IgnoreListCallback::fInstance;
    IgnoreListCallback::fInstance = nullptr;
}

// ===================================================

static void _ToggleAvatarState(plNetTransportMember* mbr, bool enable)
{
    plArmatureMod* avatar = plAvatarMgr::GetInstance()->FindAvatar(mbr->GetAvatarKey());
    if (avatar) {
        avatar->EnableDrawing(enable, plArmatureMod::kDisableReasonMutualIgnore);
        avatar->EnablePhysics(enable, plArmatureMod::kDisableReasonMutualIgnore);
    }
}

void plNetClientMgr::IHandleIgnoreMsg(plIgnorePlayerMsg* msg)
{
    // NOTE: We can be reasonable certain kNetCreatedRemotely is correct--because we set it when
    //       the message arrives in the NetClientMgr. The NonLocal flag does that crazy NetCascade...
    bool remote = hsCheckBits(msg->GetAllBCastFlags(), plMessage::kNetCreatedRemotely);
    uint32_t playerID = remote ? msg->GetIgnorerPlayerID() : msg->GetIgnoreePlayerID();

    // Our net transport member will allow us to act on this player
    int index = fTransport.FindMember(playerID);
    plNetTransportMember* mbr = nullptr;
    if (index != -1)
        mbr = fTransport.GetMember(index);

    if (msg->AmUnIgnoring())
        IHandleUnIgnore(remote, mbr, playerID);
    else
        IHandleIgnore(remote, msg->IsMutualIgnore(), mbr, playerID);
}

void plNetClientMgr::IHandleIgnore(bool remote, bool mutual, plNetTransportMember* mbr, uint32_t playerID)
{
    if (mutual && mbr) {
        uint32_t theFlag = remote ? kRemoteIgnore : kLocalIgnore;
        mbr->GetNetApp()->SetFlagsBit(theFlag);
        _ToggleAvatarState(mbr, false);
    }
}

void plNetClientMgr::IHandleUnIgnore(bool remote, plNetTransportMember* mbr, uint32_t playerID)
{
    if (mbr) {
        // Remove the appropriate flag from the NetApp
        plNetApp* theApp = mbr->GetNetApp();
        uint32_t theFlag = remote ? kRemoteIgnore : kLocalIgnore;
        theApp->SetFlagsBit(theFlag, false);

        // We only want to clear the invisibility (mutual ignore) if neither player is enforcing it
        bool anyIgnore = theApp->GetFlagsBit(kRemoteIgnore) || theApp->GetFlagsBit(kLocalIgnore);
        if (!anyIgnore)
            _ToggleAvatarState(mbr, true);
    }
}

// ===================================================

void plNetClientMgr::ICheckForIgnore(plNetTransportMember* mbr)
{
    // If this is someone I'm ignoring, I have to send out the bad news...
    if (VaultAmIgnoringPlayer(mbr->GetPlayerID())) {
        uint32_t flags = VaultGetMutualIgnore() ? plIgnorePlayerMsg::kMutualIgnore : 0;
        plIgnorePlayerMsg* msg = new plIgnorePlayerMsg(VaultGetPlayerId(), mbr->GetPlayerID(), flags);
        msg->Send();
        return;
    }

    // If this d00d is ignoring me, he's probably already told me, but his avatar wasn't loaded (D'oh)
    // So, I'll want to check for that. If he's a slacker and hasn't gotten around to it yet (unlikely),
    // I'll receive his notify later, and it will just work(TM)
    if (mbr->GetNetApp()->GetFlagsBit(kRemoteIgnore))
        _ToggleAvatarState(mbr, false);
}
