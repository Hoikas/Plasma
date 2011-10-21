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
/*****************************************************************************
*
*   $/Plasma20/Sources/Plasma/PubUtilLib/plNetGameLib/Private/plNglAuth.cpp
*   
***/

#include "../Pch.h"
#pragma hdrstop

namespace Ngl { namespace Auth {
/*****************************************************************************
*
*   Private
*
***/

struct CliAuConn : AtomicRef {
    CliAuConn ();
    ~CliAuConn ();

    // Reconnection
    AsyncTimer *        reconnectTimer;
    unsigned            reconnectStartMs;
    
    // Ping
    AsyncTimer *        pingTimer;
    unsigned            pingSendTimeMs;
    unsigned            lastHeardTimeMs;

    // This function should be called during object construction
    // to initiate connection attempts to the remote host whenever
    // the socket is disconnected.
    void AutoReconnect ();
    bool AutoReconnectEnabled ();
    void StopAutoReconnect (); // call before destruction
    void StartAutoReconnect ();
    void TimerReconnect ();
    
    // ping
    void AutoPing ();
    void StopAutoPing ();
    void TimerPing ();
    
    void Send (const unsigned_ptr fields[], unsigned count);

    CCritSect       critsect;
    LINK(CliAuConn) link;
    AsyncSocket     sock;
    NetCli *        cli;
    wchar           name[MAX_PATH];
    NetAddress      addr;
    Uuid            token;
    unsigned        seq;
    unsigned        serverChallenge;
    AsyncCancelId   cancelId;
    bool            abandoned;
};

//============================================================================
// PingRequestTrans
//============================================================================
struct PingRequestTrans : NetAuthTrans {
    FNetCliAuthPingRequestCallback  m_callback;
    void *                          m_param;
    unsigned                        m_pingAtMs;
    unsigned                        m_replyAtMs;
    ARRAY(byte)                     m_payload;
    
    PingRequestTrans (
        FNetCliAuthPingRequestCallback  callback,
        void *                          param,
        unsigned                        pingAtMs,
        unsigned                        payloadBytes,
        const void *                    payload
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountExistsRequestTrans
//============================================================================
struct AccountExistsRequestTrans : NetAuthTrans {
    FNetCliAuthAccountExistsRequestCallback     m_callback;
    void *                                      m_param;

    // send    
    wchar                                       m_accountName[kMaxAccountNameLength];

    // recv
    byte                                        m_exists;
    

    
    AccountExistsRequestTrans (
        FNetCliAuthAccountExistsRequestCallback callback,
        void *                          param,
        const wchar                     accountName[]
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// LoginRequestTrans
//============================================================================
struct LoginRequestTrans : NetAuthTrans {
    FNetCliAuthLoginRequestCallback m_callback;
    void *                          m_param;

    Uuid                            m_accountId;
    unsigned                        m_accountFlags;
    unsigned                        m_billingType;
    unsigned                        m_playerCount;
    NetCliAuthPlayerInfo            m_players[kMaxPlayersPerAccount];

    LoginRequestTrans (
        FNetCliAuthLoginRequestCallback callback,
        void *                          param
    );

    void AddPlayer (
        unsigned    playerInt,
        const wchar playerName[],
        const wchar avatarShape[],
        unsigned    explorer
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AgeRequestTrans
//============================================================================
struct AgeRequestTrans : NetAuthTrans {
    FNetCliAuthAgeRequestCallback       m_callback;
    void *                              m_param;
    wchar                               m_ageName[kMaxAgeNameLength];
    unsigned                            m_ageMcpId;
    Uuid                                m_ageInstId;
    unsigned                            m_ageVaultId;
    NetAddressNode                      m_gameSrvNode;

    AgeRequestTrans (
        const wchar                         ageName[],
        const Uuid &                        ageInstId,
        FNetCliAuthAgeRequestCallback       callback,
        void *                              param
    );
    ~AgeRequestTrans ();

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountCreateRequestTrans
//============================================================================
struct AccountCreateRequestTrans : NetAuthTrans {
    FNetCliAuthAccountCreateRequestCallback m_callback;
    void *                                  m_param;

    // send    
    wchar                                   m_accountName[kMaxAccountNameLength];
    ShaDigest                               m_namePassHash;
    unsigned                                m_accountFlags;
    unsigned                                m_billingType;

    // recv
    Uuid                                    m_accountId;

    AccountCreateRequestTrans (
        const wchar                             accountName[],
        const wchar                             password[],
        unsigned                                accountFlags,
        unsigned                                billingType,
        FNetCliAuthAccountCreateRequestCallback callback,
        void *                                  param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountCreateFromKeyRequestTrans
//============================================================================
struct AccountCreateFromKeyRequestTrans : NetAuthTrans {
    FNetCliAuthAccountCreateFromKeyRequestCallback  m_callback;
    void *                                          m_param;

    // send    
    wchar                                   m_accountName[kMaxAccountNameLength];
    ShaDigest                               m_namePassHash;
    Uuid                                    m_key;
    unsigned                                m_billingType;

    // recv
    Uuid                                    m_accountId;
    Uuid                                    m_activationKey;

    AccountCreateFromKeyRequestTrans (
        const wchar                                     accountName[],
        const wchar                                     password[],
        const Uuid &                                    key,
        unsigned                                        billingType,
        FNetCliAuthAccountCreateFromKeyRequestCallback  callback,
        void *                                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// PlayerCreateRequestTrans
//============================================================================
struct PlayerCreateRequestTrans : NetAuthTrans {
    FNetCliAuthPlayerCreateRequestCallback  m_callback;
    void *                                  m_param;

    // send    
    wchar                                   m_playerName[kMaxPlayerNameLength];
    wchar                                   m_avatarShape[MAX_PATH];
    wchar                                   m_friendInvite[MAX_PATH];

    // recv
    NetCliAuthPlayerInfo                    m_playerInfo;


    PlayerCreateRequestTrans (
        const wchar                             playerName[],
        const wchar                             avatarShape[],
        const wchar                             friendInvite[],
        FNetCliAuthPlayerCreateRequestCallback  callback,
        void *                                  param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// PlayerDeleteRequestTrans
//============================================================================
struct PlayerDeleteRequestTrans : NetAuthTrans {
    FNetCliAuthPlayerDeleteRequestCallback  m_callback;
    void *                                  m_param;

    // send
    unsigned                                m_playerId;

    // recv
    NetCliAuthPlayerInfo                    m_playerInfo;


    PlayerDeleteRequestTrans (
        unsigned                                playerId,
        FNetCliAuthPlayerDeleteRequestCallback  callback,
        void *                                  param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// UpgradeVisitorRequestTrans
//============================================================================
struct UpgradeVisitorRequestTrans : NetAuthTrans {
    FNetCliAuthUpgradeVisitorRequestCallback    m_callback;
    void *                                      m_param;

    // send
    unsigned                                m_playerId;

    UpgradeVisitorRequestTrans (
        unsigned                                    playerId,
        FNetCliAuthUpgradeVisitorRequestCallback    callback,
        void *                                      param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// SetPlayerRequestTrans
//============================================================================
struct SetPlayerRequestTrans : NetAuthTrans {
    FNetCliAuthSetPlayerRequestCallback m_callback;
    void *                              m_param;
    unsigned                            m_playerInt;

    SetPlayerRequestTrans (
        unsigned                            playerInt,
        FNetCliAuthSetPlayerRequestCallback callback,
        void *                              param
    );
    
    // This transaction doesn't timeout since a client starting from a clean
    // directory can take a long time between issuing this transaction and
    // receiving a reply.
    bool TimedOut () { return false; }
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountChangePasswordRequestTrans
//============================================================================
struct AccountChangePasswordRequestTrans : NetAuthTrans {
    FNetCliAuthAccountChangePasswordRequestCallback m_callback;
    void *                                          m_param;

    // send    
    wchar                                   m_accountName[kMaxAccountNameLength];
    ShaDigest                               m_namePassHash;

    AccountChangePasswordRequestTrans (
        const wchar                                     accountName[],
        const wchar                                     password[],
        FNetCliAuthAccountChangePasswordRequestCallback callback,
        void *                                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// GetPublicAgeListTrans
//============================================================================
struct GetPublicAgeListTrans : NetAuthTrans {
    FNetCliAuthGetPublicAgeListCallback     m_callback;
    void *                                  m_param;
    
    // send
    wchar                                   m_ageName[MAX_PATH];
    
    // recv
    ARRAY(NetAgeInfo)                       m_ages;
    
    GetPublicAgeListTrans (
        const wchar                         ageName[],
        FNetCliAuthGetPublicAgeListCallback callback,
        void *                              param
    );
        
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountSetRolesRequestTrans
//============================================================================
struct AccountSetRolesRequestTrans : NetAuthTrans {
    FNetCliAuthAccountSetRolesRequestCallback   m_callback;
    void *                                      m_param;

    // send    
    wchar                                   m_accountName[kMaxAccountNameLength];
    unsigned                                m_accountFlags;

    AccountSetRolesRequestTrans (
        const wchar                                 accountName[],
        unsigned                                    accountFlags,
        FNetCliAuthAccountSetRolesRequestCallback   callback,
        void *                                      param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountSetBillingTypeRequestTrans
//============================================================================
struct AccountSetBillingTypeRequestTrans : NetAuthTrans {
    FNetCliAuthAccountSetBillingTypeRequestCallback m_callback;
    void *                                          m_param;

    // send    
    wchar                                   m_accountName[kMaxAccountNameLength];
    unsigned                                m_billingType;

    AccountSetBillingTypeRequestTrans (
        const wchar                                     accountName[],
        unsigned                                        billingType,
        FNetCliAuthAccountSetBillingTypeRequestCallback callback,
        void *                                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AccountActivateRequestTrans
//============================================================================
struct AccountActivateRequestTrans : NetAuthTrans {
    FNetCliAuthAccountActivateRequestCallback   m_callback;
    void *                                      m_param;

    // send    
    Uuid                                        m_activationKey;

    AccountActivateRequestTrans (
        const Uuid &                                activationKey,
        FNetCliAuthAccountActivateRequestCallback   callback,
        void *                                      param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// FileListRequestTrans
//============================================================================
struct FileListRequestTrans : NetAuthTrans {
    FNetCliAuthFileListRequestCallback  m_callback;
    void *                              m_param;

    wchar                               m_directory[MAX_PATH];
    wchar                               m_ext[MAX_EXT];

    ARRAY(NetCliAuthFileInfo)           m_fileInfoArray;

    FileListRequestTrans (
        FNetCliAuthFileListRequestCallback  callback,
        void *                              param,
        const wchar                         directory[],
        const wchar                         ext[]
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// FileDownloadRequestTrans
//============================================================================
struct FileDownloadRequestTrans : NetAuthTrans {
    FNetCliAuthFileRequestCallback  m_callback;
    void *                          m_param;

    wchar                           m_filename[MAX_PATH];
    hsStream *                      m_writer;

    FileDownloadRequestTrans (
        FNetCliAuthFileRequestCallback  callback,
        void *                          param,
        const wchar                     filename[],
        hsStream *                      writer
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// RcvdFileDownloadChunkTrans
//============================================================================
struct RcvdFileDownloadChunkTrans : NetNotifyTrans {

    unsigned    bytes;
    unsigned    offset;
    byte *      data;
    hsStream *  writer;

    RcvdFileDownloadChunkTrans () : NetNotifyTrans(kRcvdFileDownloadChunkTrans) {}
    ~RcvdFileDownloadChunkTrans ();
    void Post ();
};


//============================================================================
// RcvdPropagatedBufferTrans
//============================================================================
struct RcvdPropagatedBufferTrans : NetNotifyTrans {

    unsigned        bufferType;
    unsigned        bufferBytes;
    byte *          bufferData;

    RcvdPropagatedBufferTrans () : NetNotifyTrans(kRcvdPropagatedBufferTrans) {}
    ~RcvdPropagatedBufferTrans ();
    void Post ();
};

//============================================================================
// VaultNodeChangedTrans
//============================================================================
struct VaultNodeChangedTrans : NetNotifyTrans {

    unsigned        m_nodeId;
    Uuid            m_revId;

    VaultNodeChangedTrans () : NetNotifyTrans(kVaultNodeChangedTrans) {}
    void Post ();
};

//============================================================================
// VaultNodeAddedTrans
//============================================================================
struct VaultNodeAddedTrans : NetNotifyTrans {

    unsigned        m_parentId;
    unsigned        m_childId;
    unsigned        m_ownerId;

    VaultNodeAddedTrans () : NetNotifyTrans(kVaultNodeAddedTrans) {}
    void Post ();
};

//============================================================================
// VaultNodeRemovedTrans
//============================================================================
struct VaultNodeRemovedTrans : NetNotifyTrans {

    unsigned        m_parentId;
    unsigned        m_childId;

    VaultNodeRemovedTrans () : NetNotifyTrans(kVaultNodeRemovedTrans) {}
    void Post ();
};

//============================================================================
// VaultNodeDeletedTrans
//============================================================================
struct VaultNodeDeletedTrans : NetNotifyTrans {

    unsigned        m_nodeId;

    VaultNodeDeletedTrans () : NetNotifyTrans(kVaultNodeDeletedTrans) {}
    void Post ();
};

//============================================================================
// VaultFetchNodeRefsTrans
//============================================================================
struct VaultFetchNodeRefsTrans : NetAuthTrans {

    unsigned                        m_nodeId;
    FNetCliAuthVaultNodeRefsFetched m_callback;
    void *                          m_param;
    
    ARRAY(NetVaultNodeRef)          m_refs;

    VaultFetchNodeRefsTrans (
        unsigned                        nodeId,
        FNetCliAuthVaultNodeRefsFetched callback,
        void *                          param
    );
        
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultInitAgeTrans
//============================================================================
struct VaultInitAgeTrans : NetAuthTrans {
    FNetCliAuthAgeInitCallback  m_callback;
    void *                      m_param;

    Uuid                        m_ageInstId;
    Uuid                        m_parentAgeInstId;
    wchar *                     m_ageFilename;
    wchar *                     m_ageInstName;
    wchar *                     m_ageUserName;
    wchar *                     m_ageDesc;
    unsigned                    m_ageSequenceNumber;
    unsigned                    m_ageLanguage;
    
    unsigned                    m_ageId;
    unsigned                    m_ageInfoId;
    
    VaultInitAgeTrans (
        FNetCliAuthAgeInitCallback  callback,           // optional
        void *                      param,              // optional
        const Uuid &                ageInstId,          // optional. is used in match
        const Uuid &                parentAgeInstId,    // optional. is used in match
        const wchar                 ageFilename[],      // optional. is used in match
        const wchar                 ageInstName[],      // optional. not used in match
        const wchar                 ageUserName[],      // optional. not used in match
        const wchar                 ageDesc[],          // optional. not used in match
        unsigned                    ageSequenceNumber,  // optional. not used in match
        unsigned                    ageLanguage         // optional. not used in match
    );
    ~VaultInitAgeTrans ();

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultFetchNodeTrans
//============================================================================
struct VaultFetchNodeTrans : NetAuthTrans {

    unsigned                    m_nodeId;
    FNetCliAuthVaultNodeFetched m_callback;
    void *                      m_param;
    
    NetVaultNode *              m_node;
    
    VaultFetchNodeTrans (
        unsigned                    nodeId,
        FNetCliAuthVaultNodeFetched callback,
        void *                      param
    );
    
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultFindNodeTrans
//============================================================================
struct VaultFindNodeTrans : NetAuthTrans {

    ARRAY(unsigned)             m_nodeIds;
    FNetCliAuthVaultNodeFind    m_callback;
    void *                      m_param;
    
    NetVaultNode *              m_node;
    
    VaultFindNodeTrans (
        NetVaultNode *              templateNode,
        FNetCliAuthVaultNodeFind    callback,
        void *                      param
    );
    ~VaultFindNodeTrans ();

    
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultCreateNodeTrans
//============================================================================
struct VaultCreateNodeTrans : NetAuthTrans {

    NetVaultNode *                  m_templateNode;
    FNetCliAuthVaultNodeCreated     m_callback;
    void *                          m_param;
    
    unsigned                        m_nodeId;
    
    VaultCreateNodeTrans (
        NetVaultNode *                  templateNode,
        FNetCliAuthVaultNodeCreated     callback,
        void *                          param
    );
    
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultSaveNodeTrans
//============================================================================
struct VaultSaveNodeTrans : NetAuthTrans {

    unsigned                            m_nodeId;
    Uuid                                m_revisionId;
    ARRAY(byte)                         m_buffer;
    FNetCliAuthVaultNodeSaveCallback    m_callback;
    void *                              m_param;
    
    VaultSaveNodeTrans (
        unsigned                            nodeId,
        const Uuid &                        revisionId,
        unsigned                            dataCount,
        const void *                        data,
        FNetCliAuthVaultNodeSaveCallback    callback,
        void *                              param
    );
    
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultAddNodeTrans
//============================================================================
struct VaultAddNodeTrans : NetAuthTrans {

    unsigned                        m_parentId;
    unsigned                        m_childId;
    unsigned                        m_ownerId;
    FNetCliAuthVaultNodeAddCallback m_callback;
    void *                          m_param;
    
    VaultAddNodeTrans (
        unsigned                        parentId,
        unsigned                        childId,
        unsigned                        ownerId,
        FNetCliAuthVaultNodeAddCallback callback,
        void *                          param
    );
    
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// VaultRemoveNodeTrans
//============================================================================
struct VaultRemoveNodeTrans : NetAuthTrans {

    unsigned                            m_parentId;
    unsigned                            m_childId;
    FNetCliAuthVaultNodeRemoveCallback  m_callback;
    void *                              m_param;
    
    VaultRemoveNodeTrans (
        unsigned                            parentId,
        unsigned                            childId,
        FNetCliAuthVaultNodeRemoveCallback  callback,
        void *                              param
    );
    
    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// NotifyNewBuildTrans
//============================================================================
struct NotifyNewBuildTrans : NetNotifyTrans {

    NotifyNewBuildTrans () : NetNotifyTrans(kNotifyNewBuildTrans) {}
    void Post ();
};

//============================================================================
// SetPlayerBanStatusRequestTrans
//============================================================================
struct SetPlayerBanStatusRequestTrans : NetAuthTrans {
    FNetCliAuthSetPlayerBanStatusRequestCallback    m_callback;
    void *                                          m_param;

    // send    
    unsigned                                            m_playerId;
    unsigned                                            m_banned;

    SetPlayerBanStatusRequestTrans (
        unsigned                                        playerId,
        unsigned                                        banned,
        FNetCliAuthSetPlayerBanStatusRequestCallback    callback,
        void *                                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ChangePlayerNameRequestTrans
//============================================================================
struct ChangePlayerNameRequestTrans : NetAuthTrans {
    FNetCliAuthChangePlayerNameRequestCallback  m_callback;
    void *                                      m_param;

    // send    
    unsigned                                        m_playerId;
    wchar                                           m_newName[kMaxPlayerNameLength];

    ChangePlayerNameRequestTrans (
        unsigned                                    playerId,
        const wchar                                 newName[],
        FNetCliAuthChangePlayerNameRequestCallback  callback,
        void *                                      param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// SendFriendInviteTrans
//============================================================================
struct SendFriendInviteTrans : NetAuthTrans {
    FNetCliAuthSendFriendInviteCallback m_callback;
    void *                              m_param;

    // send    
    wchar                               m_emailAddress[kMaxEmailAddressLength];
    wchar                               m_toName[kMaxPlayerNameLength];
    Uuid                                m_inviteUuid;

    SendFriendInviteTrans (
        const wchar                             emailAddr[],
        const wchar                             toName[],
        const Uuid &                            inviteUuid,
        FNetCliAuthSendFriendInviteCallback     callback,
        void *                                  param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// AuthConnectedNotifyTrans
//============================================================================
struct AuthConnectedNotifyTrans : NetNotifyTrans {

    AuthConnectedNotifyTrans () : NetNotifyTrans(kAuthConnectedNotifyTrans) {}
    void Post ();
};


//============================================================================
// ScoreCreateTrans
//============================================================================
struct ScoreCreateTrans : NetAuthTrans {
    FNetCliAuthCreateScoreCallback  m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_ownerId;
    char                            m_gameName[kMaxGameScoreNameLength];
    unsigned                        m_gameType;
    int                             m_value;

    // recv
    unsigned                        m_scoreId;
    UInt32                          m_createdTime;

    ScoreCreateTrans (
        unsigned                        ownerId,
        const char*                     gameName,
        unsigned                        gameType,
        int                             value,
        FNetCliAuthCreateScoreCallback  callback,
        void *                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ScoreDeleteTrans
//============================================================================
struct ScoreDeleteTrans : NetAuthTrans {
    FNetCliAuthScoreUpdateCallback  m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_scoreId;

    ScoreDeleteTrans (
        unsigned                        scoreId,
        FNetCliAuthScoreUpdateCallback  callback,
        void *                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ScoreGetScoresTrans
//============================================================================
struct ScoreGetScoresTrans : NetAuthTrans {
    FNetCliAuthGetScoresCallback    m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_ownerId;
    char                            m_gameName[kMaxGameScoreNameLength];

    // recv
    NetGameScore *                  m_scores;
    unsigned                        m_scoreCount;

    ScoreGetScoresTrans (
        unsigned                        ownerId,
        const char*                     gameName,
        FNetCliAuthGetScoresCallback    callback,
        void *                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ScoreAddPointsTrans
//============================================================================
struct ScoreAddPointsTrans : NetAuthTrans {
    FNetCliAuthScoreUpdateCallback  m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_scoreId;
    int                             m_numPoints;

    ScoreAddPointsTrans (
        unsigned                        scoreId,
        int                             numPoints,
        FNetCliAuthScoreUpdateCallback  callback,
        void *                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ScoreTransferPointsTrans
//============================================================================
struct ScoreTransferPointsTrans : NetAuthTrans {
    FNetCliAuthScoreUpdateCallback  m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_srcScoreId;
    unsigned                        m_destScoreId;
    int                             m_numPoints;

    ScoreTransferPointsTrans (
        unsigned                        srcScoreId,
        unsigned                        destScoreId,
        int                             numPoints,
        FNetCliAuthScoreUpdateCallback  callback,
        void *                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ScoreSetPointsTrans
//============================================================================
struct ScoreSetPointsTrans : NetAuthTrans {
    FNetCliAuthScoreUpdateCallback  m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_scoreId;
    int                             m_numPoints;

    ScoreSetPointsTrans (
        unsigned                        scoreId,
        int                             numPoints,
        FNetCliAuthScoreUpdateCallback  callback,
        void *                          param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};

//============================================================================
// ScoreGetRanksTrans
//============================================================================
struct ScoreGetRanksTrans : NetAuthTrans {
    FNetCliAuthGetRanksCallback     m_callback;
    void *                          m_param;

    // send    
    unsigned                        m_ownerId;
    unsigned                        m_scoreGroup;
    unsigned                        m_parentFolderId;
    wchar                           m_gameName[kMaxGameScoreNameLength];
    unsigned                        m_timePeriod;
    unsigned                        m_numResults;
    unsigned                        m_pageNumber;
    unsigned                        m_sortDesc;

    // recv
    NetGameRank *                   m_ranks;
    unsigned                        m_rankCount;

    ScoreGetRanksTrans (
        unsigned                    ownerId,
        unsigned                    scoreGroup,
        unsigned                    parentFolderId,
        const char *                cGameName,
        unsigned                    timePeriod,
        unsigned                    numResults,
        unsigned                    pageNumber,
        bool                        sortDesc,
        FNetCliAuthGetRanksCallback callback,
        void *                      param
    );

    bool Send ();
    void Post ();
    bool Recv (
        const byte  msg[],
        unsigned    bytes
    );
};


/*****************************************************************************
*
*   Private data
*
***/

enum {
    kPerfConnCount,
    kPingDisabled,
    kAutoReconnectDisabled,
    kNumPerf
};

static bool                         s_running;
static CCritSect                    s_critsect;
static LISTDECL(CliAuConn, link)    s_conns;
static CliAuConn *                  s_active;
static wchar                        s_accountName[kMaxAccountNameLength];
static ShaDigest                    s_accountNamePassHash;
static wchar                        s_authToken[kMaxPublisherAuthKeyLength];
static wchar                        s_os[kMaxGTOSIdLength];

static long                         s_perf[kNumPerf];

static UInt32                       s_encryptionKey[4];

static FNetCliAuthRecvBufferHandler         s_bufRcvdCb;
static FNetCliAuthConnectCallback           s_connectedCb;

// Vault notification handlers
static FNetCliAuthVaultNodeChanged  s_vaultNodeChangedHandler;
static FNetCliAuthVaultNodeAdded    s_vaultNodeAddedHandler;
static FNetCliAuthVaultNodeRemoved  s_vaultNodeRemovedHandler;
static FNetCliAuthVaultNodeDeleted  s_vaultNodeDeletedHandler;

static FNotifyNewBuildHandler       s_notifyNewBuildHandler;


/*****************************************************************************
*
*   Internal functions
*
***/

//===========================================================================
static inline bool ICharIsSpace (unsigned ch) {
    return ch == ' ';
}

//===========================================================================
static ENetError FixupPlayerName (wchar * name) {
    ASSERT(name);

    // Trim leading and trailing whitespace and convert
    // multiple internal spaces into only one space
    unsigned nonSpaceChars = 0;
    wchar *dst = name;
    for (wchar *src = name; *src; ) {
        // Skip whitespace
        while (*src && ICharIsSpace(*src))
            src++;

        // If the block skipped was not at the beginning
        // of the string then add one space character
        if (*src && (dst != name))
            *dst++ = ' ';

        // Copy characters until end-of-string or next whitespace
        while (*src && !ICharIsSpace(*src)) {
            ++nonSpaceChars;
            *dst++ = *src++;
        }
    }

    // Ensure destination string is terminated
    *dst = 0;

    // Check for minimum name length
    if (nonSpaceChars < 3)
        return kNetErrPlayerNameInvalid;

    return kNetSuccess;
}

//===========================================================================
static unsigned GetNonZeroTimeMs () {
    if (unsigned ms = TimeGetMs())
        return ms;
    return 1;
}

//============================================================================
static CliAuConn * GetConnIncRef_CS (const char tag[]) {
    if (CliAuConn * conn = s_active) {
        conn->IncRef(tag);
        return conn;
    }
    return nil;
}

//============================================================================
static CliAuConn * GetConnIncRef (const char tag[]) {
    CliAuConn * conn;
    s_critsect.Enter();
    {
        conn = GetConnIncRef_CS(tag);
    }
    s_critsect.Leave();
    return conn;
}

//============================================================================
static void UnlinkAndAbandonConn_CS (CliAuConn * conn) {
    s_conns.Unlink(conn);
    conn->abandoned = true;

    conn->StopAutoReconnect();

    if (conn->cancelId) {
        AsyncSocketConnectCancel(nil, conn->cancelId);
        conn->cancelId  = 0;
    }
    else if (conn->sock) {
        AsyncSocketDisconnect(conn->sock, true);
    }
    else {
        conn->DecRef("Lifetime");
    }
}

//============================================================================
static void SendClientRegisterRequest (CliAuConn * conn) {
    const unsigned_ptr msg[] = {
        kCli2Auth_ClientRegisterRequest,
        BuildId(),
    };

    conn->Send(msg, arrsize(msg));
}

//============================================================================
static bool ConnEncrypt (ENetError error, void * param) {
    CliAuConn * conn = (CliAuConn *) param;
        
    if (IS_NET_SUCCESS(error)) {

        SendClientRegisterRequest(conn);

        if (!s_perf[kPingDisabled])
            conn->AutoPing();
            
        AuthConnectedNotifyTrans * trans = NEW(AuthConnectedNotifyTrans);
        NetTransSend(trans);
    }

    return IS_NET_SUCCESS(error);
}

//============================================================================
static void NotifyConnSocketConnect (CliAuConn * conn) {

    conn->TransferRef("Connecting", "Connected");
    conn->cli = NetCliConnectAccept(
        conn->sock,
        kNetProtocolCli2Auth,
        false,
        ConnEncrypt,
        0,
        nil,
        conn
    );
}

//============================================================================
static void CheckedReconnect (CliAuConn * conn, ENetError error) {

    unsigned disconnectedMs = GetNonZeroTimeMs() - conn->lastHeardTimeMs;

    // no auto-reconnect or haven't heard from the server in a while?
    if (!conn->AutoReconnectEnabled() || (int)disconnectedMs >= (int)kDisconnectedTimeoutMs) {
        // Cancel all transactions in progress on this connection.
        NetTransCancelByConnId(conn->seq, kNetErrTimeout);
        // conn is dead.
        conn->DecRef("Lifetime");
        ReportNetError(kNetProtocolCli2Auth, error);
    }
    else {
        if (conn->token != kNilGuid)
            // previously encrypted; reconnect quickly
            conn->reconnectStartMs = GetNonZeroTimeMs() + 500;
        else
            // never encrypted; reconnect slowly
            conn->reconnectStartMs = GetNonZeroTimeMs() + kMaxReconnectIntervalMs;

        // clean up the socket and start reconnect
        if (conn->cli)
            NetCliDelete(conn->cli, true);
        conn->cli = nil;
        conn->sock = nil;
        
        conn->StartAutoReconnect();
    }
}

//============================================================================
static void NotifyConnSocketConnectFailed (CliAuConn * conn) {

    s_critsect.Enter();
    {
        conn->cancelId = 0;
        s_conns.Unlink(conn);

        if (conn == s_active)
            s_active = nil;
    }
    s_critsect.Leave();
    
    CheckedReconnect(conn, kNetErrConnectFailed);
    
    conn->DecRef("Connecting");
}

//============================================================================
static void NotifyConnSocketDisconnect (CliAuConn * conn) {

    conn->StopAutoPing();

    s_critsect.Enter();
    {
        conn->cancelId = 0;
        s_conns.Unlink(conn);
            
        if (conn == s_active)
            s_active = nil;
    }
    s_critsect.Leave();


    CheckedReconnect(conn, kNetErrDisconnected);

    conn->DecRef("Connected");
}

//============================================================================
static bool NotifyConnSocketRead (CliAuConn * conn, AsyncNotifySocketRead * read) {
    // TODO: Only dispatch messages from the active auth server
    conn->lastHeardTimeMs = GetNonZeroTimeMs();
    bool result = NetCliDispatch(conn->cli, read->buffer, read->bytes, conn);
    read->bytesProcessed += read->bytes;
    return result;
}

//============================================================================
static bool SocketNotifyCallback (
    AsyncSocket         sock,
    EAsyncNotifySocket  code,
    AsyncNotifySocket * notify,
    void **             userState
) {
    bool result = true;
    CliAuConn * conn;

    switch (code) {
        case kNotifySocketConnectSuccess:
            conn = (CliAuConn *) notify->param;
            *userState = conn;
            bool abandoned;
            s_critsect.Enter();
            {
                conn->sock      = sock;
                conn->cancelId  = 0;
                abandoned       = conn->abandoned;
            }
            s_critsect.Leave();
            if (abandoned)
                AsyncSocketDisconnect(sock, true);
            else
                NotifyConnSocketConnect(conn);
        break;

        case kNotifySocketConnectFailed:
            conn = (CliAuConn *) notify->param;
            NotifyConnSocketConnectFailed(conn);
        break;

        case kNotifySocketDisconnect:
            conn = (CliAuConn *) *userState;
            NotifyConnSocketDisconnect(conn);
        break;

        case kNotifySocketRead:
            conn = (CliAuConn *) *userState;
            result = NotifyConnSocketRead(conn, (AsyncNotifySocketRead *) notify);
        break;
    }
    
    return result;
}

//============================================================================
static void Connect (
    CliAuConn * conn
) {
    ASSERT(s_running);

    conn->pingSendTimeMs = 0;

    s_critsect.Enter();
    {
        while (CliAuConn * oldConn = s_conns.Head()) {
            if (oldConn != conn)
                UnlinkAndAbandonConn_CS(oldConn);
            else
                s_conns.Unlink(oldConn);
        }
        s_conns.Link(conn);
    }
    s_critsect.Leave();
    
    Cli2Auth_Connect connect;
    connect.hdr.connType        = kConnTypeCliToAuth;
    connect.hdr.hdrBytes        = sizeof(connect.hdr);
    connect.hdr.buildId         = BuildId();
    connect.hdr.buildType       = BUILD_TYPE_LIVE;
    connect.hdr.branchId        = BranchId();
    connect.hdr.productId       = ProductId();
    connect.data.token          = conn->token;
    connect.data.dataBytes      = sizeof(connect.data);

    AsyncSocketConnect(
        &conn->cancelId,
        conn->addr,
        SocketNotifyCallback,
        conn,
        &connect,
        sizeof(connect),
        0,
        0
    );
}

//============================================================================
static void Connect (
    const wchar         name[],
    const NetAddress &  addr
) {
    ASSERT(s_running);
    
    CliAuConn * conn        = NEWZERO(CliAuConn);
    conn->addr              = addr;
    conn->seq               = ConnNextSequence();
    conn->lastHeardTimeMs   = GetNonZeroTimeMs();   // used in connect timeout, and ping timeout
    StrCopy(conn->name, name, arrsize(conn->name));

    conn->IncRef("Lifetime");
    conn->AutoReconnect();
}

//============================================================================
static void AsyncLookupCallback (
    void *              param,
    const wchar         name[],
    unsigned            addrCount,
    const NetAddress    addrs[]
) {
    if (!addrCount) {
        ReportNetError(kNetProtocolCli2Auth, kNetErrNameLookupFailed);
        return;
    }

    for (unsigned i = 0; i < addrCount; ++i) {
        Connect(name, addrs[i]);
    }
}


/*****************************************************************************
*
*   CliAuConn
*
***/

//===========================================================================
static unsigned CliAuConnTimerDestroyed (void * param) {
    CliAuConn * conn = (CliAuConn *) param;
    conn->DecRef("TimerDestroyed");
    return kAsyncTimeInfinite;
}

//===========================================================================
static unsigned CliAuConnReconnectTimerProc (void * param) {
    ((CliAuConn *) param)->TimerReconnect();
    return kAsyncTimeInfinite;
}

//===========================================================================
static unsigned CliAuConnPingTimerProc (void * param) {
    ((CliAuConn *) param)->TimerPing();
    return kPingIntervalMs;
}

//============================================================================
CliAuConn::CliAuConn () {
    AtomicAdd(&s_perf[kPerfConnCount], 1);
}

//============================================================================
CliAuConn::~CliAuConn () {
    if (cli)
        NetCliDelete(cli, true);
    AtomicAdd(&s_perf[kPerfConnCount], -1);
}

//===========================================================================
void CliAuConn::TimerReconnect () {
    ASSERT(!sock);
    ASSERT(!cancelId);
    
    if (!s_running) {
        s_critsect.Enter();
        UnlinkAndAbandonConn_CS(this);
        s_critsect.Leave();
    }
    else {
        IncRef("Connecting");

        // Remember the time we started the reconnect attempt, guarding against
        // TimeGetMs() returning zero (unlikely), as a value of zero indicates
        // a first-time connect condition to StartAutoReconnect()
        reconnectStartMs = GetNonZeroTimeMs();

        Connect(this);
    }
}

//===========================================================================
// This function is called when after a disconnect to start a new connection
void CliAuConn::StartAutoReconnect () {
    critsect.Enter();
    if (reconnectTimer && !s_perf[kAutoReconnectDisabled]) {
        // Make reconnect attempts at regular intervals. If the last attempt
        // took more than the specified max interval time then reconnect
        // immediately; otherwise wait until the time interval is up again
        // then reconnect.
        unsigned remainingMs = 0;
        if (reconnectStartMs) {
            remainingMs = reconnectStartMs - GetNonZeroTimeMs();
            if ((signed)remainingMs < 0)
                remainingMs = 0;
            LogMsg(kLogPerf, L"Auth auto-reconnecting in %u ms", remainingMs);
        }
        AsyncTimerUpdate(reconnectTimer, remainingMs);
    }
    critsect.Leave();
}

//===========================================================================
// This function should be called during object construction
// to initiate connection attempts to the remote host whenever
// the socket is disconnected.
void CliAuConn::AutoReconnect () {
        
    ASSERT(!reconnectTimer);
    IncRef("ReconnectTimer");
    critsect.Enter();
    {
        AsyncTimerCreate(
            &reconnectTimer,
            CliAuConnReconnectTimerProc,
            0,  // immediate callback
            this
        );
    }
    critsect.Leave();
}

//============================================================================
void CliAuConn::StopAutoReconnect () {
    critsect.Enter();
    {
        if (AsyncTimer * timer = reconnectTimer) {
            reconnectTimer = nil;
            AsyncTimerDeleteCallback(timer, CliAuConnTimerDestroyed);
        }
    }
    critsect.Leave();
}

//============================================================================
bool CliAuConn::AutoReconnectEnabled () {
    
    return (reconnectTimer != nil) && !s_perf[kAutoReconnectDisabled];
}

//============================================================================
void CliAuConn::AutoPing () {
    ASSERT(!pingTimer);
    IncRef("PingTimer");
    critsect.Enter();
    {
        AsyncTimerCreate(
            &pingTimer,
            CliAuConnPingTimerProc,
            sock ? 0 : kAsyncTimeInfinite,
            this
        );
    }
    critsect.Leave();
}

//============================================================================
void CliAuConn::StopAutoPing () {
    critsect.Enter();
    {
        if (AsyncTimer * timer = pingTimer) {
            pingTimer = nil;
            AsyncTimerDeleteCallback(timer, CliAuConnTimerDestroyed);
        }
    }
    critsect.Leave();
}

//============================================================================
void CliAuConn::TimerPing () {

#if 0
    // if the time difference between when we last sent a ping and when we last
    // heard from the server is >= 3x the ping interval, the socket is stale.
    if (pingSendTimeMs && abs(int(pingSendTimeMs - lastHeardTimeMs)) >= kPingTimeoutMs) {
        // ping timed out, disconnect the socket
        AsyncSocketDisconnect(sock, true);
    }
    else
#endif
    {
        // Send a ping request
        pingSendTimeMs = GetNonZeroTimeMs();
        
        const unsigned_ptr msg[] = {
            kCli2Auth_PingRequest,
            pingSendTimeMs,
            0,      // not a transaction
            0,      // no payload
            nil
        };
        
        Send(msg, arrsize(msg));
    }       
}

//============================================================================
void CliAuConn::Send (const unsigned_ptr fields[], unsigned count) {
    critsect.Enter();
    {
        NetCliSend(cli, fields, count);
        NetCliFlush(cli);
    }
    critsect.Leave();
}


/*****************************************************************************
*
*   Cli2Auth message handlers
*
***/

//============================================================================
static bool Recv_PingReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_PingReply & reply = *(const Auth2Cli_PingReply *)msg;

    if (reply.transId)
        NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AccountExistsReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AccountExistsReply & reply = *(const Auth2Cli_AccountExistsReply *)msg;

    if (reply.transId)
        NetTransRecv(reply.transId, msg, bytes);

    return true;
}


//============================================================================
static bool Recv_ClientRegisterReply (
    const byte      msg[],
    unsigned        ,
    void *          param
) {
    const Auth2Cli_ClientRegisterReply & reply = *(const Auth2Cli_ClientRegisterReply *)msg;

    CliAuConn * conn = (CliAuConn *) param;
    conn->serverChallenge = reply.serverChallenge;

    // Make this the active server
    s_critsect.Enter();
    {
        s_active = conn;
    }
    s_critsect.Leave();

    return true;
}

//============================================================================
static bool Recv_AcctPlayerInfo (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctPlayerInfo & reply = *(const Auth2Cli_AcctPlayerInfo *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctLoginReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctLoginReply & reply = *(const Auth2Cli_AcctLoginReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctCreateReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctCreateReply & reply = *(const Auth2Cli_AcctCreateReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctCreateFromKeyReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctCreateFromKeyReply & reply = *(const Auth2Cli_AcctCreateFromKeyReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_PlayerCreateReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_PlayerCreateReply & reply = *(const Auth2Cli_PlayerCreateReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_PlayerDeleteReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_PlayerDeleteReply & reply = *(const Auth2Cli_PlayerDeleteReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_UpgradeVisitorReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_UpgradeVisitorReply & reply = *(const Auth2Cli_UpgradeVisitorReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctSetPlayerReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctSetPlayerReply & reply = *(const Auth2Cli_AcctSetPlayerReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctChangePasswordReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctChangePasswordReply & reply = *(const Auth2Cli_AcctChangePasswordReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctSetRolesReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctSetRolesReply & reply = *(const Auth2Cli_AcctSetRolesReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctSetBillingTypeReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctSetBillingTypeReply & reply = *(const Auth2Cli_AcctSetBillingTypeReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AcctActivateReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AcctActivateReply & reply = *(const Auth2Cli_AcctActivateReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_AgeReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_AgeReply & reply = *(const Auth2Cli_AgeReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_FileListReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_FileListReply & reply = *(const Auth2Cli_FileListReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_FileDownloadChunk (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_FileDownloadChunk & reply = *(const Auth2Cli_FileDownloadChunk *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_KickedOff (
    const byte      buffer[],
    unsigned        bytes,
    void *          param
) {
    const Auth2Cli_KickedOff & msg = *(const Auth2Cli_KickedOff *)buffer;

    ReportNetError(kNetProtocolCli2Auth, msg.reason);
    NetCliAuthDisconnect();
    
    return true;
}

//============================================================================
static bool Recv_VaultNodeRefsFetched (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultNodeRefsFetched & reply = *(const Auth2Cli_VaultNodeRefsFetched *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultNodeFetched (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultNodeFetched & reply = *(const Auth2Cli_VaultNodeFetched *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultNodeCreated (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultNodeCreated & reply = *(const Auth2Cli_VaultNodeCreated *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultNodeChanged (
    const byte      msg[],
    unsigned        ,
    void *
) {
    const Auth2Cli_VaultNodeChanged & notify = *(const Auth2Cli_VaultNodeChanged *)msg;
    
    VaultNodeChangedTrans * trans = NEW(VaultNodeChangedTrans);
    trans->m_nodeId     = notify.nodeId;
    trans->m_revId      = notify.revisionId;
    NetTransSend(trans);

    return true;
}

//============================================================================
static bool Recv_VaultNodeAdded (
    const byte      msg[],
    unsigned        ,
    void *
) {
    const Auth2Cli_VaultNodeAdded & notify = *(const Auth2Cli_VaultNodeAdded *)msg;
    
    VaultNodeAddedTrans * trans = NEW(VaultNodeAddedTrans);
    trans->m_parentId   = notify.parentId;
    trans->m_childId    = notify.childId;
    trans->m_ownerId    = notify.ownerId;
    NetTransSend(trans);

    return true;
}

//============================================================================
static bool Recv_VaultNodeRemoved (
    const byte      msg[],
    unsigned        ,
    void *
) {
    const Auth2Cli_VaultNodeRemoved & notify = *(const Auth2Cli_VaultNodeRemoved *)msg;
    
    VaultNodeRemovedTrans * trans = NEW(VaultNodeRemovedTrans);
    trans->m_parentId   = notify.parentId;
    trans->m_childId    = notify.childId;
    NetTransSend(trans);

    return true;
}

//============================================================================
static bool Recv_VaultNodeDeleted (
    const byte      msg[],
    unsigned        ,
    void *
) {
    const Auth2Cli_VaultNodeDeleted & notify = *(const Auth2Cli_VaultNodeDeleted *)msg;
    
    VaultNodeDeletedTrans * trans = NEW(VaultNodeDeletedTrans);
    trans->m_nodeId     = notify.nodeId;
    NetTransSend(trans);

    return true;
}

//============================================================================
static bool Recv_VaultSaveNodeReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultSaveNodeReply & reply = *(const Auth2Cli_VaultSaveNodeReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultAddNodeReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultAddNodeReply & reply = *(const Auth2Cli_VaultAddNodeReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultRemoveNodeReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultRemoveNodeReply & reply = *(const Auth2Cli_VaultRemoveNodeReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultInitAgeReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultInitAgeReply & reply = *(const Auth2Cli_VaultInitAgeReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_VaultNodeFindReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_VaultNodeFindReply & reply = *(const Auth2Cli_VaultNodeFindReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_PublicAgeList (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_PublicAgeList & reply = *(const Auth2Cli_PublicAgeList *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ServerAddr (
    const byte  in[],
    unsigned    ,
    void *
) {
    // the auth server has given us its actual address (behind any load-balancer)
    // so that when we attempt a reconnect, we'll reconnect with our state on
    // the auth (but only if we reconnect in a short period of time!)
    const Auth2Cli_ServerAddr & msg = *(const Auth2Cli_ServerAddr *)in;
    
    s_critsect.Enter();
    {
        if (s_active) {
            s_active->token = msg.token;
            NetAddressFromNode(
                msg.srvAddr,
                NetAddressGetPort(s_active->addr),
                &s_active->addr
            );
            wchar addrStr[64];
            NetAddressNodeToString(
                msg.srvAddr,
                addrStr,
                arrsize(addrStr)
            );
            LogMsg(kLogPerf, L"SrvAuth addr: %s", addrStr);
        }
    }
    s_critsect.Leave();
    
    return true;
}

//============================================================================
static bool Recv_NotifyNewBuild (
    const byte[],
    unsigned    ,
    void *
) {
    NotifyNewBuildTrans * trans = NEW(NotifyNewBuildTrans);
    NetTransSend(trans);

    return true;
}

//============================================================================
static bool Recv_SetPlayerBanStatusReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_SetPlayerBanStatusReply & reply = *(const Auth2Cli_SetPlayerBanStatusReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ChangePlayerNameReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ChangePlayerNameReply & reply = *(const Auth2Cli_ChangePlayerNameReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_SendFriendInviteReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_SendFriendInviteReply & reply = *(const Auth2Cli_SendFriendInviteReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreCreateReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreCreateReply & reply = *(const Auth2Cli_ScoreCreateReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreDeleteReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreDeleteReply & reply = *(const Auth2Cli_ScoreDeleteReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreGetScoresReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreGetScoresReply & reply = *(const Auth2Cli_ScoreGetScoresReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreAddPointsReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreAddPointsReply & reply = *(const Auth2Cli_ScoreAddPointsReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreTransferPointsReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreTransferPointsReply & reply = *(const Auth2Cli_ScoreTransferPointsReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreSetPointsReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreSetPointsReply & reply = *(const Auth2Cli_ScoreSetPointsReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

//============================================================================
static bool Recv_ScoreGetRanksReply (
    const byte      msg[],
    unsigned        bytes,
    void *
) {
    const Auth2Cli_ScoreGetRanksReply & reply = *(const Auth2Cli_ScoreGetRanksReply *)msg;

    NetTransRecv(reply.transId, msg, bytes);

    return true;
}

/*****************************************************************************
*
*   Cli2Auth protocol
*
***/

#define MSG(s)  kNetMsg_Cli2Auth_##s
static NetMsgInitSend s_send[] = {
    { MSG(PingRequest)              },
    { MSG(ClientRegisterRequest)    },
    { MSG(AcctLoginRequest)         },
    { MSG(AcctCreateRequest)        },
    { MSG(AcctCreateFromKeyRequest) },
    { MSG(PlayerCreateRequest)      },
    { MSG(PlayerDeleteRequest)      },
    { MSG(UpgradeVisitorRequest)    },
    { MSG(AcctSetPlayerRequest)     },
    { MSG(AcctChangePasswordRequest)},
    { MSG(AcctSetRolesRequest)      },
    { MSG(AcctSetBillingTypeRequest)},
    { MSG(AcctActivateRequest)      },
    { MSG(AgeRequest)               },
    { MSG(FileListRequest)          },
    { MSG(FileDownloadRequest)      },
    { MSG(FileDownloadChunkAck)     },
    { MSG(VaultFetchNodeRefs)       },
    { MSG(VaultNodeAdd)             },
    { MSG(VaultNodeRemove)          },
    { MSG(VaultNodeFetch)           },
    { MSG(VaultNodeCreate)          },
    { MSG(VaultNodeSave)            },
    { MSG(VaultInitAgeRequest)      },
    { MSG(VaultNodeFind)            },
    { MSG(VaultSetSeen)             },
    { MSG(VaultSendNode)            },
    { MSG(GetPublicAgeList)         },
    { MSG(SetAgePublic)             },
    { MSG(PropagateBuffer)          },
    { MSG(ClientSetCCRLevel)        },
    { MSG(LogPythonTraceback)       },
    { MSG(LogStackDump)             },
    { MSG(LogClientDebuggerConnect) },
    { MSG(SetPlayerBanStatusRequest)},
    { MSG(KickPlayer)               },
    { MSG(ChangePlayerNameRequest)  },
    { MSG(SendFriendInviteRequest)  },
    { MSG(ScoreCreate)              },
    { MSG(ScoreDelete)              },
    { MSG(ScoreGetScores)           },
    { MSG(ScoreAddPoints)           },
    { MSG(ScoreTransferPoints)      },
    { MSG(ScoreSetPoints)           },
    { MSG(ScoreGetRanks)            },
    { MSG(AccountExistsRequest)     },
};
#undef MSG

#define MSG(s)  kNetMsg_Auth2Cli_##s, Recv_##s
static NetMsgInitRecv s_recv[] = {
    { MSG(PingReply)                },
    { MSG(ClientRegisterReply)      },
    { MSG(AcctPlayerInfo)           },
    { MSG(AcctLoginReply)           },
    { MSG(AcctCreateReply)          },
    { MSG(AcctCreateFromKeyReply)   },
    { MSG(PlayerCreateReply)        },
    { MSG(PlayerDeleteReply)        },
    { MSG(UpgradeVisitorReply)      },
    { MSG(AcctSetPlayerReply)       },
    { MSG(AcctChangePasswordReply)  },
    { MSG(AcctSetRolesReply)        },
    { MSG(AcctSetBillingTypeReply)  },
    { MSG(AcctActivateReply)        },
    { MSG(AgeReply)                 },
    { MSG(FileListReply)            },
    { MSG(FileDownloadChunk)        },
    { MSG(KickedOff)                },
    { MSG(VaultNodeRefsFetched)     },
    { MSG(VaultNodeFetched)         },
    { MSG(VaultNodeCreated)         },
    { MSG(VaultNodeChanged)         },
    { MSG(VaultNodeAdded)           },
    { MSG(VaultNodeRemoved)         },
    { MSG(VaultNodeDeleted)         },
    { MSG(VaultSaveNodeReply)       },
    { MSG(VaultAddNodeReply)        },
    { MSG(VaultRemoveNodeReply)     },
    { MSG(VaultInitAgeReply)        },
    { MSG(VaultNodeFindReply)       },
    { MSG(PublicAgeList)            },
    { MSG(ServerAddr)               },
    { MSG(NotifyNewBuild)           },
    { MSG(SetPlayerBanStatusReply)  },
    { MSG(ChangePlayerNameReply)    },
    { MSG(SendFriendInviteReply)    },
    { MSG(ScoreCreateReply)         },
    { MSG(ScoreDeleteReply)         },
    { MSG(ScoreGetScoresReply)      },
    { MSG(ScoreAddPointsReply)      },
    { MSG(ScoreTransferPointsReply) },
    { MSG(ScoreSetPointsReply)      },
    { MSG(ScoreGetRanksReply)       },
    { MSG(AccountExistsReply)       },
};
#undef MSG


/*****************************************************************************
*
*   PingRequestTrans
*
***/

//============================================================================
PingRequestTrans::PingRequestTrans (
    FNetCliAuthPingRequestCallback  callback,
    void *                          param,
    unsigned                        pingAtMs,
    unsigned                        payloadBytes,
    const void *                    payload
) : NetAuthTrans(kPingRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_pingAtMs(pingAtMs)
{
    m_payload.Set((const byte *)payload, payloadBytes);
}

//============================================================================
bool PingRequestTrans::Send () {

    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_PingRequest,
                        m_pingAtMs,
                        m_transId,
                        m_payload.Count(),
        (unsigned_ptr)  m_payload.Ptr(),
    };
    
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void PingRequestTrans::Post () {

    m_callback(
        m_result,
        m_param,
        m_pingAtMs,
        m_replyAtMs,
        m_payload.Count(),
        m_payload.Ptr()
    );
}

//============================================================================
bool PingRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_PingReply & reply = *(const Auth2Cli_PingReply *)msg;

    m_payload.Set(reply.payload, reply.payloadBytes);
    m_replyAtMs     = TimeGetMs();
    m_result        = kNetSuccess;
    m_state         = kTransStateComplete;

    return true;
}


/*****************************************************************************
*
*   AccountExistsRequestTrans
*
***/

//============================================================================
AccountExistsRequestTrans::AccountExistsRequestTrans (
    FNetCliAuthAccountExistsRequestCallback callback,
    void *                          param,
    const wchar                     accountName[]
) : NetAuthTrans(kPingRequestTrans)
,   m_callback(callback)
,   m_param(param)
{
    StrCopy(m_accountName, accountName, kMaxAccountNameLength);
}

//============================================================================
bool AccountExistsRequestTrans::Send () {

    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AccountExistsRequest,
                        m_transId,
                        (unsigned_ptr)m_accountName
    };
    
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void AccountExistsRequestTrans::Post () {

    m_callback(
        m_result,
        m_param,
        m_exists
    );
}

//============================================================================
bool AccountExistsRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AccountExistsReply & reply = *(const Auth2Cli_AccountExistsReply *)msg;

    m_exists        = reply.exists;
    m_result        = reply.result;
    m_state         = kTransStateComplete;

    return true;
}


/*****************************************************************************
*
*   LoginRequestTrans
*
***/

//============================================================================
LoginRequestTrans::LoginRequestTrans (
    FNetCliAuthLoginRequestCallback callback,
    void *                          param
) : NetAuthTrans(kLoginRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_accountId(kNilGuid)
,   m_accountFlags(0)
,   m_playerCount(0)
{
    ZERO(m_players);
}

//============================================================================
void LoginRequestTrans::AddPlayer (
    unsigned    playerInt,
    const wchar playerName[],
    const wchar avatarShape[],
    unsigned    explorer
) {
    unsigned index = m_playerCount++;
    ASSERT(index < kMaxPlayersPerAccount);
    m_players[index].playerInt  = playerInt;
    m_players[index].explorer   = explorer;
    StrCopy(m_players[index].playerName, playerName, arrsize(m_players[index].playerName));
    StrCopy(m_players[index].avatarShape, avatarShape, arrsize(m_players[index].avatarShape));
}

//============================================================================
bool LoginRequestTrans::Send () {

    if (!AcquireConn())
        return false;

    ShaDigest challengeHash;
    dword clientChallenge = 0;

    wchar domain[15];
    PathSplitEmail(s_accountName, nil, 0, domain, arrsize(domain), nil, 0, nil, 0, 0);

    if (StrLen(domain) == 0 || StrCmpI(domain, L"gametap") == 0) {
        challengeHash = s_accountNamePassHash;
    }
    else {
        CryptCreateRandomSeed(
            sizeof(clientChallenge),
            (byte *) &clientChallenge
        );

        CryptHashPasswordChallenge(
            clientChallenge,
            s_active->serverChallenge,
            s_accountNamePassHash,
            &challengeHash
        );
    }

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctLoginRequest,
        m_transId,
        clientChallenge,
        (unsigned_ptr) s_accountName,
        (unsigned_ptr) &challengeHash,
        (unsigned_ptr) s_authToken,
        (unsigned_ptr) s_os,
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void LoginRequestTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_accountId,
        m_accountFlags,
        m_billingType,
        m_players,
        m_playerCount
    );
}

//============================================================================
bool LoginRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    dword msgId = (dword)*msg;
    switch (msgId) {
        case kAuth2Cli_AcctPlayerInfo: {
            const Auth2Cli_AcctPlayerInfo & reply = *(const Auth2Cli_AcctPlayerInfo *) msg;
            AddPlayer(reply.playerInt, reply.playerName, reply.avatarShape, reply.explorer);
        }
        break;

        case kAuth2Cli_AcctLoginReply: {
            const Auth2Cli_AcctLoginReply & reply = *(const Auth2Cli_AcctLoginReply *) msg;
            m_result        = reply.result;
            m_state         = kTransStateComplete;
            m_accountId     = reply.accountId;
            m_accountFlags  = reply.accountFlags;
            m_billingType   = reply.billingType;

            unsigned memSize = min(arrsize(s_encryptionKey), arrsize(reply.encryptionKey));
            memSize *= sizeof(UInt32);
            MemCopy(s_encryptionKey, reply.encryptionKey, memSize);
        }
        break;

        DEFAULT_FATAL(msgId);
    };

    return true;
}


/*****************************************************************************
*
*   AgeRequestTrans
*
***/

//============================================================================
AgeRequestTrans::AgeRequestTrans (
    const wchar                         ageName[],
    const Uuid &                        ageInstId,
    FNetCliAuthAgeRequestCallback       callback,
    void *                              param
) : NetAuthTrans(kAgeRequestTrans)
,   m_ageInstId(ageInstId)
,   m_callback(callback)
,   m_param(param)
{
    StrCopy(m_ageName, ageName, arrsize(m_ageName));
}

//============================================================================
AgeRequestTrans::~AgeRequestTrans () {
}

//============================================================================
bool AgeRequestTrans::Send () {
    if (!AcquireConn())
        return true;

    const unsigned_ptr msg[] = {
        kCli2Auth_AgeRequest,
                        m_transId,
        (unsigned_ptr)  m_ageName,
        (unsigned_ptr) &m_ageInstId,
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void AgeRequestTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_ageMcpId,
        m_ageVaultId,
        m_ageInstId,
        m_gameSrvNode
    );
}

//============================================================================
bool AgeRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AgeReply & reply = *(const Auth2Cli_AgeReply *) msg;
    m_gameSrvNode   = reply.gameSrvNode;
    m_ageMcpId      = reply.ageMcpId;
    m_ageVaultId    = reply.ageVaultId;
    m_ageInstId     = reply.ageInstId;
    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   AccountCreateRequestTrans
*
***/

//============================================================================
AccountCreateRequestTrans::AccountCreateRequestTrans (
    const wchar                             accountName[],
    const wchar                             password[],
    unsigned                                accountFlags,
    unsigned                                billingType,
    FNetCliAuthAccountCreateRequestCallback callback,
    void *                                  param
) : NetAuthTrans(kAccountCreateRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_accountFlags(accountFlags)
,   m_billingType(billingType)
{
    StrCopy(m_accountName, accountName, arrsize(m_accountName));

    CryptHashPassword(
        m_accountName,
        password,
        &m_namePassHash
    );
}

//============================================================================
bool AccountCreateRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctCreateRequest,
                        m_transId,
        (unsigned_ptr)  m_accountName,
        (unsigned_ptr)  &m_namePassHash,
                        m_accountFlags,
                        m_billingType,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void AccountCreateRequestTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_accountId
    );
}

//============================================================================
bool AccountCreateRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctCreateReply & reply = *(const Auth2Cli_AcctCreateReply *) msg;

    m_result    = reply.result;
    m_accountId = reply.accountId;
    m_state     = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   AccountCreateFromKeyRequestTrans
*
***/

//============================================================================
AccountCreateFromKeyRequestTrans::AccountCreateFromKeyRequestTrans (
    const wchar                                     accountName[],
    const wchar                                     password[],
    const Uuid &                                    key,
    unsigned                                        billingType,
    FNetCliAuthAccountCreateFromKeyRequestCallback  callback,
    void *                                          param
) : NetAuthTrans(kAccountCreateFromKeyRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_billingType(billingType)
,   m_key(key)
{
    StrCopy(m_accountName, accountName, arrsize(m_accountName));

    CryptHashPassword(
        m_accountName,
        password,
        &m_namePassHash
    );
}

//============================================================================
bool AccountCreateFromKeyRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctCreateFromKeyRequest,
                        m_transId,
        (unsigned_ptr)  m_accountName,
        (unsigned_ptr)  &m_namePassHash,
        (unsigned_ptr)  &m_key,
                        m_billingType,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void AccountCreateFromKeyRequestTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_accountId,
        m_activationKey
    );
}

//============================================================================
bool AccountCreateFromKeyRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctCreateFromKeyReply & reply = *(const Auth2Cli_AcctCreateFromKeyReply *) msg;

    m_result        = reply.result;
    m_accountId     = reply.accountId;
    m_activationKey = reply.activationKey;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   PlayerCreateRequestTrans
*
***/

//============================================================================
PlayerCreateRequestTrans::PlayerCreateRequestTrans (
    const wchar                             playerName[],
    const wchar                             avatarShape[],
    const wchar                             friendInvite[],
    FNetCliAuthPlayerCreateRequestCallback  callback,
    void *                                  param
) : NetAuthTrans(kPlayerCreateRequestTrans)
,   m_callback(callback)
,   m_param(param)
{
    StrCopy(m_playerName, playerName, arrsize(m_playerName));
    StrCopy(m_avatarShape, avatarShape, arrsize(m_avatarShape));
    if (friendInvite)
        StrCopy(m_friendInvite, friendInvite, arrsize(m_friendInvite));
    else
        m_friendInvite[0] = 0;
    ZERO(m_playerInfo);
}

//============================================================================
bool PlayerCreateRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_PlayerCreateRequest,
                        m_transId,
        (unsigned_ptr)  m_playerName,
        (unsigned_ptr)  m_avatarShape,
        (unsigned_ptr)  m_friendInvite,
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void PlayerCreateRequestTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_playerInfo
    );
}

//============================================================================
bool PlayerCreateRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_PlayerCreateReply & reply = *(const Auth2Cli_PlayerCreateReply *) msg;
    if (!IS_NET_ERROR(reply.result)) {
        m_playerInfo.playerInt  = reply.playerInt;
        m_playerInfo.explorer   = reply.explorer;
        StrCopy(m_playerInfo.playerName, reply.playerName, arrsize(m_playerInfo.playerName));
        StrCopy(m_playerInfo.avatarShape, reply.avatarShape, arrsize(m_playerInfo.avatarShape));
    }
    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   PlayerDeleteRequestTrans
*
***/

//============================================================================
PlayerDeleteRequestTrans::PlayerDeleteRequestTrans (
    unsigned                                playerId,
    FNetCliAuthPlayerDeleteRequestCallback  callback,
    void *                                  param
) : NetAuthTrans(kPlayerDeleteRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_playerId(playerId)
{
}

//============================================================================
bool PlayerDeleteRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_PlayerDeleteRequest,
                        m_transId,
                        m_playerId
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void PlayerDeleteRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool PlayerDeleteRequestTrans::Recv (
    const byte  msg[],
    unsigned    
) {
    const Auth2Cli_PlayerDeleteReply & reply = *(const Auth2Cli_PlayerDeleteReply *) msg;
    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   UpgradeVisitorRequestTrans
*
***/

//============================================================================
UpgradeVisitorRequestTrans::UpgradeVisitorRequestTrans (
    unsigned                                    playerId,
    FNetCliAuthUpgradeVisitorRequestCallback    callback,
    void *                                      param
) : NetAuthTrans(kUpgradeVisitorRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_playerId(playerId)
{
}

//============================================================================
bool UpgradeVisitorRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_UpgradeVisitorRequest,
                        m_transId,
                        m_playerId
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void UpgradeVisitorRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool UpgradeVisitorRequestTrans::Recv (
    const byte  msg[],
    unsigned    
) {
    const Auth2Cli_UpgradeVisitorReply & reply = *(const Auth2Cli_UpgradeVisitorReply *) msg;
    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   SetPlayerRequestTrans
*
***/

//============================================================================
SetPlayerRequestTrans::SetPlayerRequestTrans (
    unsigned                            playerInt,
    FNetCliAuthSetPlayerRequestCallback callback,
    void *                              param

) : NetAuthTrans(kSetPlayerRequestTrans)
,   m_playerInt(playerInt)
,   m_callback(callback)
,   m_param(param)
{
}

//============================================================================
bool SetPlayerRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctSetPlayerRequest,
        m_transId,
        m_playerInt,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void SetPlayerRequestTrans::Post () {
    m_callback(m_result, m_param);
}

//============================================================================
bool SetPlayerRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctSetPlayerReply & reply = *(const Auth2Cli_AcctSetPlayerReply *) msg;
    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   AccountChangePasswordRequestTrans
*
***/

//============================================================================
AccountChangePasswordRequestTrans::AccountChangePasswordRequestTrans (
    const wchar                                     accountName[],
    const wchar                                     password[],
    FNetCliAuthAccountChangePasswordRequestCallback callback,
    void *                                          param
) : NetAuthTrans(kAccountChangePasswordRequestTrans)
,   m_callback(callback)
,   m_param(param)
{
    StrCopy(m_accountName, accountName, arrsize(m_accountName));
    
    CryptHashPassword(
        m_accountName,
        password,
        &m_namePassHash
    );
}

//============================================================================
bool AccountChangePasswordRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctChangePasswordRequest,
                        m_transId,
        (unsigned_ptr)  m_accountName,
        (unsigned_ptr)  &m_namePassHash,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void AccountChangePasswordRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool AccountChangePasswordRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctChangePasswordReply & reply = *(const Auth2Cli_AcctChangePasswordReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   GetPublicAgeListTrans
*
***/

//============================================================================
GetPublicAgeListTrans::GetPublicAgeListTrans (
    const wchar                         ageName[],
    FNetCliAuthGetPublicAgeListCallback callback,
    void *                              param
) : NetAuthTrans(kGetPublicAgeListTrans)
,   m_callback(callback)
,   m_param(param)
{
    StrCopy(m_ageName, ageName, arrsize(m_ageName));
}

//============================================================================
bool GetPublicAgeListTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_GetPublicAgeList,
                        m_transId,
        (unsigned_ptr)  &m_ageName,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void GetPublicAgeListTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_ages
    );
}

//============================================================================
bool GetPublicAgeListTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_PublicAgeList & reply = *(const Auth2Cli_PublicAgeList *) msg;
    
    if (IS_NET_SUCCESS(reply.result))
        m_ages.Set(reply.ages, reply.ageCount);

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   AccountSetRolesRequestTrans
*
***/

//============================================================================
AccountSetRolesRequestTrans::AccountSetRolesRequestTrans (
    const wchar                                 accountName[],
    unsigned                                    accountFlags,
    FNetCliAuthAccountSetRolesRequestCallback   callback,
    void *                                      param
) : NetAuthTrans(kAccountSetRolesRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_accountFlags(accountFlags)
{
    StrCopy(m_accountName, accountName, arrsize(m_accountName));
}

//============================================================================
bool AccountSetRolesRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctSetRolesRequest,
                        m_transId,
        (unsigned_ptr)  m_accountName,
                        m_accountFlags,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void AccountSetRolesRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool AccountSetRolesRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctSetRolesReply & reply = *(const Auth2Cli_AcctSetRolesReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   AccountSetBillingTypeRequestTrans
*
***/

//============================================================================
AccountSetBillingTypeRequestTrans::AccountSetBillingTypeRequestTrans (
    const wchar                                     accountName[],
    unsigned                                        billingType,
    FNetCliAuthAccountSetBillingTypeRequestCallback callback,
    void *                                          param
) : NetAuthTrans(kAccountSetBillingTypeRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_billingType(billingType)
{
    StrCopy(m_accountName, accountName, arrsize(m_accountName));
}

//============================================================================
bool AccountSetBillingTypeRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctSetBillingTypeRequest,
                        m_transId,
        (unsigned_ptr)  m_accountName,
                        m_billingType,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void AccountSetBillingTypeRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool AccountSetBillingTypeRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctSetBillingTypeReply & reply = *(const Auth2Cli_AcctSetBillingTypeReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   AccountActivateRequestTrans
*
***/

//============================================================================
AccountActivateRequestTrans::AccountActivateRequestTrans (
    const Uuid &                                activationKey,
    FNetCliAuthAccountActivateRequestCallback   callback,
    void *                                      param
) : NetAuthTrans(kAccountActivateRequestTrans)
,   m_callback(callback)
,   m_param(param)
{
    m_activationKey = activationKey;
}

//============================================================================
bool AccountActivateRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_AcctActivateRequest,
                        m_transId,
        (unsigned_ptr)  &m_activationKey,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void AccountActivateRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool AccountActivateRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_AcctActivateReply & reply = *(const Auth2Cli_AcctActivateReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   FileListRequestTrans
*
***/

//============================================================================
FileListRequestTrans::FileListRequestTrans (
    FNetCliAuthFileListRequestCallback  callback,
    void *                              param,
    const wchar                         directory[],
    const wchar                         ext[]
) : NetAuthTrans(kFileListRequestTrans)
,   m_callback(callback)
,   m_param(param)
{
    StrCopy(m_directory, directory, arrsize(m_directory));
    StrCopy(m_ext, ext, arrsize(m_ext));
}

//============================================================================
bool FileListRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_FileListRequest,
        m_transId,
        (unsigned_ptr) m_directory,
        (unsigned_ptr) m_ext,
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void FileListRequestTrans::Post () {
    m_callback(m_result, m_param, m_fileInfoArray.Ptr(), m_fileInfoArray.Count());
}

//============================================================================
bool FileListRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_FileListReply & reply = *(const Auth2Cli_FileListReply *) msg;

    dword wcharCount = reply.wcharCount;
    const wchar* curChar = reply.fileData;
    // if wcharCount is 2, the data only contains the terminator "\0\0" and we
    // don't need to convert anything
    if (wcharCount == 2)
        m_fileInfoArray.Clear();
    else
    {
        // fileData format: "filename\0size\0filename\0size\0...\0\0"
        bool done = false;
        while (!done) {
            if (wcharCount == 0)
            {
                done = true;
                break;
            }

            // read in the filename
            wchar filename[MAX_PATH];
            StrCopy(filename, curChar, arrsize(filename));
            filename[arrsize(filename) - 1] = L'\0'; // make sure it's terminated

            unsigned filenameLen = StrLen(filename);
            curChar += filenameLen; // advance the pointer
            wcharCount -= filenameLen; // keep track of the amount remaining
            if ((*curChar != L'\0') || (wcharCount <= 0))
                return false; // something is screwy, abort and disconnect

            curChar++; // point it at the first part of the size value (format: 0xHHHHLLLL)
            wcharCount--;
            if (wcharCount < 4) // we have to have 2 chars for the size, and 2 for terminator at least
                return false; // screwy data
            unsigned size = ((*curChar) << 16) + (*(curChar + 1));
            curChar += 2;
            wcharCount -= 2;
            if ((*curChar != L'\0') || (wcharCount <= 0))
                return false; // screwy data

            // save the data in our array
            NetCliAuthFileInfo* info = m_fileInfoArray.New();
            StrCopy(info->filename, filename, arrsize(info->filename));
            info->filesize = size;

            // point it at either the second part of the terminator, or the next filename
            curChar++;
            wcharCount--;
            if (*curChar == L'\0')
            {
                // we hit the terminator
                if (wcharCount != 1)
                    return false; // invalid data, we shouldn't have any more
                done = true; // we're done
            }
            else if (wcharCount < 6) // we must have at least a 1 char string, '\0', size, and "\0\0" terminator left
                return false; // screwy data
        }
    }

    m_result    = reply.result;
    m_state     = kTransStateComplete;

    return true;
}

/*****************************************************************************
*
*   FileDownloadRequestTrans
*
***/

//============================================================================
FileDownloadRequestTrans::FileDownloadRequestTrans (
    FNetCliAuthFileRequestCallback  callback,
    void *                          param,
    const wchar                     filename[],
    hsStream *                      writer
) : NetAuthTrans(kFileDownloadRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_writer(writer)
{
    StrCopy(m_filename, filename, arrsize(m_filename));
    // This transaction issues "sub transactions" which must complete
    // before this one even though they were issued after us.
    m_hasSubTrans = true;
}

//============================================================================
bool FileDownloadRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_FileDownloadRequest,
        m_transId,
        (unsigned_ptr) m_filename,
    };

    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void FileDownloadRequestTrans::Post () {
    m_callback(m_result, m_param, m_filename, m_writer);
}

//============================================================================
bool FileDownloadRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_FileDownloadChunk & reply = *(const Auth2Cli_FileDownloadChunk *) msg;

    if (IS_NET_ERROR(reply.result)) {
        // we have a problem... indicate we are done and abort
        m_result    = reply.result;
        m_state     = kTransStateComplete;
        return true;
    }

    // we have data to write, so queue it for write in the main thread (we're
    // currently in a net recv thread)
    if (reply.chunkSize) {
        RcvdFileDownloadChunkTrans * writeTrans = NEW(RcvdFileDownloadChunkTrans);
        writeTrans->writer  = m_writer;
        writeTrans->bytes   = reply.chunkSize;
        writeTrans->offset  = reply.chunkOffset;
        writeTrans->data    = (byte *)ALLOC(reply.chunkSize);
        MemCopy(writeTrans->data, reply.chunkData, reply.chunkSize);
        NetTransSend(writeTrans);
    }

    if (reply.chunkOffset + reply.chunkSize >= reply.fileSize) {
        // all bytes received, mark as complete
        m_result    = reply.result;
        m_state     = kTransStateComplete;
    }
    
    // Ack the data
    const unsigned_ptr ack[] = {
        kCli2Auth_FileDownloadChunkAck,
        m_transId
    };
    m_conn->Send(ack, arrsize(ack));
    
    return true;
}


/*****************************************************************************
*
*   RcvdFileDownloadChunkTrans
*
***/

//============================================================================
RcvdFileDownloadChunkTrans::~RcvdFileDownloadChunkTrans () {
    FREE(data);
}

//============================================================================
void RcvdFileDownloadChunkTrans::Post () {
    
    if (writer->GetPosition() != offset)
        writer->SetPosition(offset);
    
    writer->Write(bytes, data);
    m_result = kNetSuccess;
    m_state  = kTransStateComplete;
}


/*****************************************************************************
*
*   RcvdPropagatedBufferTrans
*
***/

//============================================================================
RcvdPropagatedBufferTrans::~RcvdPropagatedBufferTrans () {
    FREE(bufferData);
}

//============================================================================
void RcvdPropagatedBufferTrans::Post () {
    if (s_bufRcvdCb)
        s_bufRcvdCb(bufferType, bufferBytes, bufferData);
}


/*****************************************************************************
*
*   VaultNodeChangedTrans
*
***/

//============================================================================
void VaultNodeChangedTrans::Post () {
    if (s_vaultNodeChangedHandler)
        s_vaultNodeChangedHandler(m_nodeId, m_revId);
}


/*****************************************************************************
*
*   VaultNodeAddedTrans
*
***/

//============================================================================
void VaultNodeAddedTrans::Post () {
    if (s_vaultNodeAddedHandler)
        s_vaultNodeAddedHandler(m_parentId, m_childId, m_ownerId);
}


/*****************************************************************************
*
*   VaultNodeRemovedTrans
*
***/

//============================================================================
void VaultNodeRemovedTrans::Post () {
    if (s_vaultNodeRemovedHandler)
        s_vaultNodeRemovedHandler(m_parentId, m_childId);
}


/*****************************************************************************
*
*   VaultNodeDeletedTrans
*
***/

//============================================================================
void VaultNodeDeletedTrans::Post () {
    if (s_vaultNodeDeletedHandler)
        s_vaultNodeDeletedHandler(m_nodeId);
}


/*****************************************************************************
*
*   VaultFetchNodeRefsTrans
*
***/

//============================================================================
VaultFetchNodeRefsTrans::VaultFetchNodeRefsTrans (
    unsigned                        nodeId,
    FNetCliAuthVaultNodeRefsFetched callback,
    void *                          param
) : NetAuthTrans(kVaultFetchNodeRefsTrans)
,   m_nodeId(nodeId)
,   m_callback(callback)
,   m_param(param)
{
}
    
//============================================================================
bool VaultFetchNodeRefsTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultFetchNodeRefs,
        m_transId,
        m_nodeId,
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultFetchNodeRefsTrans::Post () {
    if (m_callback)
        m_callback(
            m_result,
            m_param,
            m_refs.Ptr(),
            m_refs.Count()
        );
}

//============================================================================
bool VaultFetchNodeRefsTrans::Recv (
    const byte  msg[],
    unsigned
) {
    const Auth2Cli_VaultNodeRefsFetched & reply = *(const Auth2Cli_VaultNodeRefsFetched *) msg;

    if (IS_NET_SUCCESS(reply.result))
        m_refs.Set(reply.refs, reply.refCount); 

    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}


/*****************************************************************************
*
*   VaultInitAgeTrans
*
***/

//============================================================================
VaultInitAgeTrans::VaultInitAgeTrans (
    FNetCliAuthAgeInitCallback  callback,           // optional
    void *                      param,              // optional
    const Uuid &                ageInstId,          // optional. is used in match
    const Uuid &                parentAgeInstId,    // optional. is used in match
    const wchar                 ageFilename[],      // optional. is used in match
    const wchar                 ageInstName[],      // optional. not used in match
    const wchar                 ageUserName[],      // optional. not used in match
    const wchar                 ageDesc[],          // optional. not used in match
    unsigned                    ageSequenceNumber,  // optional. not used in match
    unsigned                    ageLanguage         // optional. not used in match
) : NetAuthTrans(kVaultInitAgeTrans)
,   m_callback(callback)
,   m_param(param)
,   m_ageInstId(ageInstId)
,   m_parentAgeInstId(parentAgeInstId)
,   m_ageFilename(StrDup(ageFilename ? ageFilename : L""))
,   m_ageInstName(StrDup(ageInstName ? ageInstName : L""))
,   m_ageUserName(StrDup(ageUserName ? ageUserName : L""))
,   m_ageDesc(StrDup(ageDesc ? ageDesc : L""))
,   m_ageSequenceNumber(ageSequenceNumber)
,   m_ageLanguage(ageLanguage)
{
}

//============================================================================
VaultInitAgeTrans::~VaultInitAgeTrans () {
    FREE(m_ageFilename);
    FREE(m_ageInstName);
    FREE(m_ageUserName);
    FREE(m_ageDesc);
}

//============================================================================
bool VaultInitAgeTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultInitAgeRequest,
                        m_transId,
        (unsigned_ptr) &m_ageInstId,
        (unsigned_ptr) &m_parentAgeInstId,
        (unsigned_ptr)  m_ageFilename,
        (unsigned_ptr)  m_ageInstName,
        (unsigned_ptr)  m_ageUserName,
        (unsigned_ptr)  m_ageDesc,
                        m_ageSequenceNumber,
                        m_ageLanguage,
    };
    
    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void VaultInitAgeTrans::Post () {
    if (m_callback)
        m_callback(
            m_result,
            m_param,
            m_ageId,
            m_ageInfoId
        );
}

//============================================================================
bool VaultInitAgeTrans::Recv (
    const byte  msg[],
    unsigned    
) {
    const Auth2Cli_VaultInitAgeReply & reply = *(const Auth2Cli_VaultInitAgeReply *) msg;
    
    m_ageId     = reply.ageVaultId;
    m_ageInfoId = reply.ageInfoVaultId;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    
    return true;
}


/*****************************************************************************
*
*   VaultFetchNodeTrans
*
***/

//============================================================================
VaultFetchNodeTrans::VaultFetchNodeTrans (
    unsigned                    nodeId,
    FNetCliAuthVaultNodeFetched callback,
    void *                      param
) : NetAuthTrans(kVaultFetchNodeTrans)
,   m_nodeId(nodeId)
,   m_callback(callback)
,   m_param(param)
{
}

//============================================================================
bool VaultFetchNodeTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultNodeFetch,
        m_transId,
        m_nodeId,
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultFetchNodeTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_node
    );
    if (m_node)
        m_node->DecRef("Recv");
}

//============================================================================
bool VaultFetchNodeTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_VaultNodeFetched & reply = *(const Auth2Cli_VaultNodeFetched *) msg;
    
    if (IS_NET_SUCCESS(reply.result)) {
        m_node = NEWZERO(NetVaultNode);
        m_node->Read_LCS(reply.nodeBuffer, reply.nodeBytes, 0);
        m_node->IncRef("Recv");
    }

    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}


/*****************************************************************************
*
*   VaultFindNodeTrans
*
***/

//============================================================================
VaultFindNodeTrans::VaultFindNodeTrans (
    NetVaultNode *              templateNode,
    FNetCliAuthVaultNodeFind    callback,
    void *                      param
) : NetAuthTrans(kVaultFindNodeTrans)
,   m_node(templateNode)
,   m_callback(callback)
,   m_param(param)
{
    m_node->IncRef();
}

//============================================================================
VaultFindNodeTrans::~VaultFindNodeTrans () {
    m_node->DecRef();
}

//============================================================================
bool VaultFindNodeTrans::Send () {
    if (!AcquireConn())
        return false;
        
    ARRAY(byte) buffer;
    m_node->critsect.Enter();
    m_node->Write_LCS(&buffer, 0);
    m_node->critsect.Leave();

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultNodeFind,
                        m_transId,
                        buffer.Count(),
        (unsigned_ptr)  buffer.Ptr(),
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultFindNodeTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_nodeIds.Count(),
        m_nodeIds.Ptr()
    );
}

//============================================================================
bool VaultFindNodeTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_VaultNodeFindReply & reply = *(const Auth2Cli_VaultNodeFindReply *) msg;

    if (IS_NET_SUCCESS(reply.result)) {
        COMPILER_ASSERT(sizeof(unsigned) == sizeof(dword));
        m_nodeIds.Set((unsigned *)reply.nodeIds, reply.nodeIdCount);
    }

    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}


/*****************************************************************************
*
*   VaultCreateNodeTrans
*
***/

//============================================================================
VaultCreateNodeTrans::VaultCreateNodeTrans (
    NetVaultNode *                  templateNode,
    FNetCliAuthVaultNodeCreated     callback,
    void *                          param
) : NetAuthTrans(kVaultCreateNodeTrans)
,   m_templateNode(templateNode)
,   m_callback(callback)
,   m_param(param)
{
    m_templateNode->IncRef();
}

//============================================================================
bool VaultCreateNodeTrans::Send () {
    if (!AcquireConn())
        return false;
        
    ARRAY(byte) buffer;
    m_templateNode->Write_LCS(&buffer, 0);

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultNodeCreate,
                        m_transId,
                        buffer.Count(),
        (unsigned_ptr)  buffer.Ptr()
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultCreateNodeTrans::Post () {
    m_callback(
        m_result,
        m_param,
        m_nodeId
    );
    m_templateNode->DecRef();
}

//============================================================================
bool VaultCreateNodeTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_VaultNodeCreated & reply = *(const Auth2Cli_VaultNodeCreated *) msg;
    
    if (IS_NET_SUCCESS(reply.result))
        m_nodeId = reply.nodeId;

    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}

/*****************************************************************************
*
*   VaultSaveNodeTrans
*
***/

//============================================================================
VaultSaveNodeTrans::VaultSaveNodeTrans (
    unsigned                            nodeId,
    const Uuid &                        revisionId,
    unsigned                            dataCount,
    const void *                        data,
    FNetCliAuthVaultNodeSaveCallback    callback,
    void *                              param
) : NetAuthTrans(kVaultSaveNodeTrans)
,   m_nodeId(nodeId)
,   m_revisionId(revisionId)
,   m_callback(callback)
,   m_param(param)
{
    m_buffer.Set((const byte *)data, dataCount);
}

//============================================================================
bool VaultSaveNodeTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultNodeSave,
                        m_transId,
                        m_nodeId,
        (unsigned_ptr)  &m_revisionId,
                        m_buffer.Count(),
        (unsigned_ptr)  m_buffer.Ptr(),
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultSaveNodeTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool VaultSaveNodeTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_VaultSaveNodeReply & reply = *(const Auth2Cli_VaultSaveNodeReply *) msg;
    
    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}

/*****************************************************************************
*
*   VaultAddNodeTrans
*
***/

//============================================================================
VaultAddNodeTrans::VaultAddNodeTrans (
    unsigned                        parentId,
    unsigned                        childId,
    unsigned                        ownerId,
    FNetCliAuthVaultNodeAddCallback callback,
    void *                          param
) : NetAuthTrans(kVaultAddNodeTrans)
,   m_parentId(parentId)
,   m_childId(childId)
,   m_ownerId(ownerId)
,   m_callback(callback)
,   m_param(param)
{
}

//============================================================================
bool VaultAddNodeTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultNodeAdd,
                    m_transId,
                    m_parentId,
                    m_childId,
                    m_ownerId,
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultAddNodeTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool VaultAddNodeTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_VaultAddNodeReply & reply = *(const Auth2Cli_VaultAddNodeReply *) msg;
    
    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}

/*****************************************************************************
*
*   VaultRemoveNodeTrans
*
***/

//============================================================================
VaultRemoveNodeTrans::VaultRemoveNodeTrans (
    unsigned                            parentId,
    unsigned                            childId,
    FNetCliAuthVaultNodeRemoveCallback  callback,
    void *                              param
) : NetAuthTrans(kVaultRemoveNodeTrans)
,   m_parentId(parentId)
,   m_childId(childId)
,   m_callback(callback)
,   m_param(param)
{
}

//============================================================================
bool VaultRemoveNodeTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultNodeRemove,
                    m_transId,
                    m_parentId,
                    m_childId,
    };
            
    m_conn->Send(msg, arrsize(msg));
    
    return true;
}

//============================================================================
void VaultRemoveNodeTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool VaultRemoveNodeTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_VaultRemoveNodeReply & reply = *(const Auth2Cli_VaultRemoveNodeReply *) msg;
    
    m_result = reply.result;
    m_state  = kTransStateComplete;
    
    return true;
}

/*****************************************************************************
*
*   NotifyNewBuildTrans
*
***/

//============================================================================
void NotifyNewBuildTrans::Post () {

    if (s_notifyNewBuildHandler)
        s_notifyNewBuildHandler();
}

/*****************************************************************************
*
*   SetPlayerBanStatusRequestTrans
*
***/

//============================================================================
SetPlayerBanStatusRequestTrans::SetPlayerBanStatusRequestTrans (
    unsigned                                        playerId,
    unsigned                                        banned,
    FNetCliAuthSetPlayerBanStatusRequestCallback    callback,
    void *                                          param
) : NetAuthTrans(kSetPlayerBanStatusRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_playerId(playerId)
,   m_banned(banned)
{
}

//============================================================================
bool SetPlayerBanStatusRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_SetPlayerBanStatusRequest,
                        m_transId,
                        m_playerId,
                        m_banned,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void SetPlayerBanStatusRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool SetPlayerBanStatusRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_SetPlayerBanStatusReply & reply = *(const Auth2Cli_SetPlayerBanStatusReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   ChangePlayerNameRequestTrans
*
***/

//============================================================================
ChangePlayerNameRequestTrans::ChangePlayerNameRequestTrans (
    unsigned                                    playerId,
    const wchar                                 newName[],
    FNetCliAuthChangePlayerNameRequestCallback  callback,
    void *                                      param
) : NetAuthTrans(kChangePlayerNameRequestTrans)
,   m_callback(callback)
,   m_param(param)
,   m_playerId(playerId)
{
    StrCopy(m_newName, newName, arrsize(m_newName));
}

//============================================================================
bool ChangePlayerNameRequestTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_ChangePlayerNameRequest,
                        m_transId,
                        m_playerId,
        (unsigned_ptr)  m_newName,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ChangePlayerNameRequestTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool ChangePlayerNameRequestTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ChangePlayerNameReply & reply = *(const Auth2Cli_ChangePlayerNameReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   SendFriendInviteTrans
*
***/

//============================================================================
SendFriendInviteTrans::SendFriendInviteTrans (
    const wchar                             emailAddr[],
    const wchar                             toName[],
    const Uuid &                            inviteUuid,
    FNetCliAuthSendFriendInviteCallback     callback,
    void *                                  param
) : NetAuthTrans(kSendFriendInviteTrans)
,   m_callback(callback)
,   m_param(param)
,   m_inviteUuid(inviteUuid)
{
    StrCopy(m_emailAddress, emailAddr, arrsize(m_emailAddress));
    StrCopy(m_toName, toName, arrsize(m_toName));
}

//============================================================================
bool SendFriendInviteTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_SendFriendInviteRequest,
                        m_transId,
        (unsigned_ptr) &m_inviteUuid,
        (unsigned_ptr)  m_emailAddress,
        (unsigned_ptr)  m_toName,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void SendFriendInviteTrans::Post () {
    m_callback(
        m_result,
        m_param
    );
}

//============================================================================
bool SendFriendInviteTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_SendFriendInviteReply & reply = *(const Auth2Cli_SendFriendInviteReply *) msg;

    m_result    = reply.result;
    m_state     = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   AuthConnectedNotifyTrans
*
***/

//============================================================================
void AuthConnectedNotifyTrans::Post() {
    if (s_connectedCb != nil)
        s_connectedCb();
}


/*****************************************************************************
*
*   ScoreCreateTrans
*
***/

//============================================================================
ScoreCreateTrans::ScoreCreateTrans (
    unsigned                        ownerId,
    const char*                     gameName,
    unsigned                        gameType,
    int                             value,
    FNetCliAuthCreateScoreCallback  callback,
    void *                          param
) : NetAuthTrans(kScoreCreateTrans)
,   m_callback(callback)
,   m_param(param)
,   m_ownerId(ownerId)
,   m_gameType(gameType)
,   m_value(value)
,   m_scoreId(0)
,   m_createdTime(0)
{
    StrCopy(m_gameName, gameName, arrsize(m_gameName));
}

//============================================================================
bool ScoreCreateTrans::Send () {
    if (!AcquireConn())
        return false;

    wchar wgameName[kMaxGameScoreNameLength];
    StrToUnicode(wgameName, m_gameName, arrsize(wgameName));

    const unsigned_ptr msg[] = {
            kCli2Auth_ScoreCreate,
                        m_transId,
                        m_ownerId,
        (unsigned_ptr)  wgameName,
                        m_gameType,
                        m_value
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreCreateTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param,
            m_scoreId,
            m_createdTime,
            m_ownerId,
            m_gameName,
            m_gameType,
            m_value
        );
    }
}

//============================================================================
bool ScoreCreateTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreCreateReply & reply = *(const Auth2Cli_ScoreCreateReply *) msg;

    m_scoreId       = reply.scoreId;
    m_createdTime   = reply.createdTime;

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   ScoreDeleteTrans
*
***/

//============================================================================
ScoreDeleteTrans::ScoreDeleteTrans (
    unsigned                        scoreId,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) : NetAuthTrans(kScoreDeleteTrans)
,   m_callback(callback)
,   m_param(param)
,   m_scoreId(scoreId)
{
}

//============================================================================
bool ScoreDeleteTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_ScoreDelete,
                        m_transId,
                        m_scoreId,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreDeleteTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool ScoreDeleteTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreDeleteReply & reply = *(const Auth2Cli_ScoreDeleteReply *) msg;

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   ScoreGetScoresTrans
*
***/

//============================================================================
ScoreGetScoresTrans::ScoreGetScoresTrans (
    unsigned                        ownerId,
    const char*                     gameName,
    FNetCliAuthGetScoresCallback    callback,
    void *                          param
) : NetAuthTrans(kScoreGetScoresTrans)
,   m_callback(callback)
,   m_param(param)
,   m_ownerId(ownerId)
,   m_scores(nil)
,   m_scoreCount(0)
{
    StrCopy(m_gameName, gameName, arrsize(m_gameName));
}

//============================================================================
bool ScoreGetScoresTrans::Send () {
    if (!AcquireConn())
        return false;

    wchar wgameName[kMaxGameScoreNameLength];
    StrToUnicode(wgameName, m_gameName, arrsize(wgameName));

    const unsigned_ptr msg[] = {
        kCli2Auth_ScoreGetScores,
                        m_transId,
                        m_ownerId,
        (unsigned_ptr)  wgameName
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreGetScoresTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param,
            m_scores,
            m_scoreCount
        );
    }
}

//============================================================================
bool ScoreGetScoresTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreGetScoresReply & reply = *(const Auth2Cli_ScoreGetScoresReply *) msg;

    if (reply.scoreCount > 0) {
        m_scoreCount    = reply.scoreCount;
        m_scores        = TRACKED_NEW NetGameScore[m_scoreCount];

        byte*       bufferPos = const_cast<byte*>(reply.buffer);
        unsigned    bufferLength = reply.byteCount;

        for (unsigned i = 0; i < m_scoreCount; ++i) {
            bufferLength -= m_scores[i].Read(bufferPos, bufferLength, &bufferPos);
        }
    }
    else {
        m_scoreCount    = 0;
        m_scores        = nil;
    }

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   ScoreAddPointsTrans
*
***/

//============================================================================
ScoreAddPointsTrans::ScoreAddPointsTrans (
    unsigned                        scoreId,
    int                             numPoints,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) : NetAuthTrans(kScoreAddPointsTrans)
,   m_callback(callback)
,   m_param(param)
,   m_scoreId(scoreId)
,   m_numPoints(numPoints)
{
}

//============================================================================
bool ScoreAddPointsTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_ScoreAddPoints,
                        m_transId,
                        m_scoreId,
                        m_numPoints,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreAddPointsTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool ScoreAddPointsTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreAddPointsReply & reply = *(const Auth2Cli_ScoreAddPointsReply *) msg;

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   ScoreTransferPointsTrans
*
***/

//============================================================================
ScoreTransferPointsTrans::ScoreTransferPointsTrans (
    unsigned                        srcScoreId,
    unsigned                        destScoreId,
    int                             numPoints,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) : NetAuthTrans(kScoreTransferPointsTrans)
,   m_callback(callback)
,   m_param(param)
,   m_srcScoreId(srcScoreId)
,   m_destScoreId(destScoreId)
,   m_numPoints(numPoints)
{
}

//============================================================================
bool ScoreTransferPointsTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_ScoreTransferPoints,
                        m_transId,
                        m_srcScoreId,
                        m_destScoreId,
                        m_numPoints,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreTransferPointsTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool ScoreTransferPointsTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreTransferPointsReply & reply = *(const Auth2Cli_ScoreTransferPointsReply *) msg;

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

/*****************************************************************************
*
*   ScoreSetPointsTrans
*
***/

//============================================================================
ScoreSetPointsTrans::ScoreSetPointsTrans (
    unsigned                        scoreId,
    int                             numPoints,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) : NetAuthTrans(kScoreSetPointsTrans)
,   m_callback(callback)
,   m_param(param)
,   m_scoreId(scoreId)
,   m_numPoints(numPoints)
{
}

//============================================================================
bool ScoreSetPointsTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_ScoreSetPoints,
                        m_transId,
                        m_scoreId,
                        m_numPoints,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreSetPointsTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param
        );
    }
}

//============================================================================
bool ScoreSetPointsTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreSetPointsReply & reply = *(const Auth2Cli_ScoreSetPointsReply *) msg;

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}


/*****************************************************************************
*
*   ScoreGetRanksTrans
*
***/

//============================================================================
ScoreGetRanksTrans::ScoreGetRanksTrans (
    unsigned                    ownerId,
    unsigned                    scoreGroup,
    unsigned                    parentFolderId,
    const char *                cGameName,
    unsigned                    timePeriod,
    unsigned                    numResults,
    unsigned                    pageNumber,
    bool                        sortDesc,
    FNetCliAuthGetRanksCallback callback,
    void *                      param
) : NetAuthTrans(kScoreGetRanksTrans)
,   m_callback(callback)
,   m_param(param)
,   m_ownerId(ownerId)
,   m_scoreGroup(scoreGroup)
,   m_parentFolderId(parentFolderId)
,   m_timePeriod(timePeriod)
,   m_numResults(numResults)
,   m_pageNumber(pageNumber)
,   m_sortDesc(sortDesc)
{
    StrToUnicode(m_gameName, cGameName, arrsize(m_gameName));
}

//============================================================================
bool ScoreGetRanksTrans::Send () {
    if (!AcquireConn())
        return false;

    const unsigned_ptr msg[] = {
        kCli2Auth_ScoreGetRanks,
                        m_transId,
                        m_ownerId,
                        m_scoreGroup,
                        m_parentFolderId,
        (unsigned_ptr)  m_gameName,
                        m_timePeriod,
                        m_numResults,
                        m_pageNumber,
                        m_sortDesc,
    };

    m_conn->Send(msg, arrsize(msg));

    return true;
}

//============================================================================
void ScoreGetRanksTrans::Post () {
    if (m_callback) {
        m_callback(
            m_result,
            m_param,
            m_ranks,
            m_rankCount
        );
    }
}

//============================================================================
bool ScoreGetRanksTrans::Recv (
    const byte  msg[],
    unsigned    bytes
) {
    const Auth2Cli_ScoreGetRanksReply & reply = *(const Auth2Cli_ScoreGetRanksReply *) msg;

    if (reply.rankCount > 0) {
        m_rankCount = reply.rankCount;
        m_ranks     = TRACKED_NEW NetGameRank[m_rankCount];

        byte*       bufferPos = const_cast<byte*>(reply.buffer);
        unsigned    bufferLength = reply.byteCount;

        for (unsigned i = 0; i < m_rankCount; ++i) {
            bufferLength -= m_ranks[i].Read(bufferPos, bufferLength, &bufferPos);
        }
    }
    else {
        m_rankCount = 0;
        m_ranks     = nil;
    }

    m_result        = reply.result;
    m_state         = kTransStateComplete;
    return true;
}

} using namespace Auth;


/*****************************************************************************
*
*   NetAuthTrans
*
***/

//============================================================================
NetAuthTrans::NetAuthTrans (ETransType transType)
:   NetTrans(kNetProtocolCli2Auth, transType)
,   m_conn(nil)
{
}

//============================================================================
NetAuthTrans::~NetAuthTrans () {
    ReleaseConn();
}

//============================================================================
bool NetAuthTrans::AcquireConn () {
    if (!m_conn)
        m_conn = GetConnIncRef("AcquireConn");
    return m_conn != nil;
}

//============================================================================
void NetAuthTrans::ReleaseConn () {
    if (m_conn) {
        m_conn->DecRef("AcquireConn");
        m_conn = nil;
    }
}


/*****************************************************************************
*
*   Protected functions
*
***/

//============================================================================
void AuthInitialize () {
    s_running = true;
    NetMsgProtocolRegister(
        kNetProtocolCli2Auth,
        false,
        s_send, arrsize(s_send),
        s_recv, arrsize(s_recv),
        kAuthDhGValue,
        BigNum(sizeof(kAuthDhXData), kAuthDhXData),
        BigNum(sizeof(kAuthDhNData), kAuthDhNData)
    );
}

//============================================================================
void AuthDestroy (bool wait) {
    s_running = false;
    
    s_bufRcvdCb = nil;
    s_connectedCb = nil;
    s_vaultNodeChangedHandler   = nil;
    s_vaultNodeAddedHandler     = nil;
    s_vaultNodeRemovedHandler   = nil;
    s_vaultNodeDeletedHandler   = nil;
    s_notifyNewBuildHandler     = nil;
    

    NetTransCancelByProtocol(
        kNetProtocolCli2Auth,
        kNetErrRemoteShutdown
    );    
    NetMsgProtocolDestroy(
        kNetProtocolCli2Auth,
        false
    );

    s_critsect.Enter();
    {
        while (CliAuConn * conn = s_conns.Head())
            UnlinkAndAbandonConn_CS(conn);
        s_active = nil;
    }
    s_critsect.Leave();

    if (!wait)
        return;
        
    while (s_perf[kPerfConnCount]) {
        NetTransUpdate();
        AsyncSleep(10);
    }
}

//============================================================================
bool AuthQueryConnected () {
    bool result;
    s_critsect.Enter();
    {
        if (nil != (result = s_active))
            result &= (nil != s_active->cli);
    }
    s_critsect.Leave();
    return result;
}

//============================================================================
unsigned AuthGetConnId () {
    unsigned connId;
    s_critsect.Enter();
    connId = (s_active) ? s_active->seq : 0;
    s_critsect.Leave();
    return connId;
}

//============================================================================
void AuthPingEnable (bool enable) {
    s_perf[kPingDisabled] = !enable;
    s_critsect.Enter();
    for (;;) {
        if (!s_active)
            break;
        if (enable)
            s_active->AutoPing();
        else
            s_active->StopAutoPing();
        break;
    }
    s_critsect.Leave();
}


} using namespace Ngl;


/*****************************************************************************
*
*   Exported functions
*
***/

//============================================================================
void NetCliAuthStartConnect (
    const wchar *   authAddrList[],
    unsigned        authAddrCount
) {
    // TEMP: Only connect to one auth server until we fill out this module
    // to choose the "best" auth connection.
    authAddrCount = min(authAddrCount, 1);

    for (unsigned i = 0; i < authAddrCount; ++i) {
        // Do we need to lookup the address?
        const wchar * name = authAddrList[i];
        while (unsigned ch = *name) {
            ++name;
            if (!(isdigit(ch) || ch == L'.' || ch == L':')) {
                AsyncCancelId cancelId;
                AsyncAddressLookupName(
                    &cancelId,
                    AsyncLookupCallback,
                    authAddrList[i],
                    kNetDefaultClientPort,
                    nil
                );
                break;
            }
        }
        if (!name[0]) {
            NetAddress addr;
            NetAddressFromString(&addr, authAddrList[i], kNetDefaultClientPort);
            Connect(authAddrList[i], addr);
        }
    }
}

//============================================================================
bool NetCliAuthQueryConnected () {

    return AuthQueryConnected();
}

//============================================================================
void NetCliAuthAutoReconnectEnable (bool enable) {

    s_perf[kAutoReconnectDisabled] = !enable;
}

//============================================================================
void NetCliAuthDisconnect () {

    s_critsect.Enter();
    {
        while (CliAuConn * conn = s_conns.Head())
            UnlinkAndAbandonConn_CS(conn);
        s_active = nil;
    }
    s_critsect.Leave();
}

//============================================================================
void NetCliAuthUnexpectedDisconnect () {
    s_critsect.Enter();
    {
        if (s_active && s_active->sock)
            AsyncSocketDisconnect(s_active->sock, true);
    }
    s_critsect.Leave();
}

//============================================================================
void NetCliAuthSetConnectCallback (
    FNetCliAuthConnectCallback  callback
) {
    s_connectedCb = callback;
}

//============================================================================
void NetCliAuthPingRequest (
    unsigned                        pingTimeMs,
    unsigned                        payloadBytes,
    const void *                    payload,
    FNetCliAuthPingRequestCallback  callback,
    void *                          param
) {
    PingRequestTrans * trans = NEW(PingRequestTrans)(
        callback,
        param,
        pingTimeMs,
        payloadBytes,
        payload
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAccountExistsRequest (
    const wchar                                 accountName[],
    FNetCliAuthAccountExistsRequestCallback     callback,
    void *                                      param
) {
    AccountExistsRequestTrans * trans = NEW(AccountExistsRequestTrans)(
        callback,
        param,
        accountName
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthLoginRequest (
    const wchar                     accountName[],
    const ShaDigest *               accountNamePassHash,
    const wchar                     authToken[],
    const wchar                     os[],
    FNetCliAuthLoginRequestCallback callback,
    void *                          param
) {
    // Cache updated login info if provided.
    if (accountName)
        StrCopy(s_accountName, accountName, arrsize(s_accountName));
    if (accountNamePassHash)
        s_accountNamePassHash = *accountNamePassHash;
    if (authToken)
        StrCopy(s_authToken, authToken, arrsize(s_authToken));
    if (os)
        StrCopy(s_os, os, arrsize(s_os));

    LoginRequestTrans * trans = NEW(LoginRequestTrans)(callback, param);
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAgeRequest (
    const wchar                         ageName[],
    const Uuid &                        ageInstId,
    FNetCliAuthAgeRequestCallback       callback,
    void *                              param
) {
    AgeRequestTrans * trans = NEW(AgeRequestTrans)(
        ageName,
        ageInstId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthGetEncryptionKey (
    UInt32      key[],
    unsigned    size
) {
    unsigned memSize = min(arrsize(s_encryptionKey), size);
    memSize *= sizeof(UInt32);
    MemCopy(key, s_encryptionKey, memSize);
}

//============================================================================
void NetCliAuthAccountCreateRequest (
    const wchar                             accountName[],
    const wchar                             password[],
    unsigned                                accountFlags,
    unsigned                                billingType,
    FNetCliAuthAccountCreateRequestCallback callback,
    void *                                  param
) {
    AccountCreateRequestTrans * trans = NEW(AccountCreateRequestTrans)(
        accountName,
        password,
        accountFlags,
        billingType,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAccountCreateFromKeyRequest (
    const wchar                                     accountName[],
    const wchar                                     password[],
    Uuid                                            key,
    unsigned                                        billingType,
    FNetCliAuthAccountCreateFromKeyRequestCallback  callback,
    void *                                          param
) {
    AccountCreateFromKeyRequestTrans * trans = NEW(AccountCreateFromKeyRequestTrans)(
        accountName,
        password,
        key,
        billingType,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthPlayerCreateRequest (
    const wchar                             playerName[],
    const wchar                             avatarShape[],
    const wchar                             friendInvite[],
    FNetCliAuthPlayerCreateRequestCallback  callback,
    void *                                  param
) {
    wchar name[kMaxPlayerNameLength];
    StrCopy(name, playerName, arrsize(name));
    ENetError error = FixupPlayerName(name);
    if (IS_NET_ERROR(error)) {
        NetCliAuthPlayerInfo playerInfo;
        callback(error, param, playerInfo);
    }
    else {
        PlayerCreateRequestTrans * trans = NEW(PlayerCreateRequestTrans)(
            name,
            avatarShape,
            friendInvite,
            callback,
            param
        );
        NetTransSend(trans);
    }
}

//============================================================================
void NetCliAuthPlayerDeleteRequest (
    unsigned                                playerId,
    FNetCliAuthPlayerDeleteRequestCallback  callback,
    void *                                  param
) {
    PlayerDeleteRequestTrans * trans = NEW(PlayerDeleteRequestTrans)(
        playerId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthUpgradeVisitorRequest (
    unsigned                                    playerId,
    FNetCliAuthUpgradeVisitorRequestCallback    callback,
    void *                                      param
) {
    UpgradeVisitorRequestTrans * trans = NEW(UpgradeVisitorRequestTrans)(
        playerId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthSetCCRLevel (
    unsigned        ccrLevel
) {
    CliAuConn * conn = GetConnIncRef("SetCCRLevel");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
        kCli2Auth_ClientSetCCRLevel,
                    ccrLevel,
    };
    
    conn->Send(msg, arrsize(msg));
    conn->DecRef("SetCCRLevel");
}

//============================================================================
void NetCliAuthSetPlayerRequest (
    unsigned                            playerInt,
    FNetCliAuthSetPlayerRequestCallback callback,
    void *                              param
) {
    SetPlayerRequestTrans * trans = NEW(SetPlayerRequestTrans)(
        playerInt,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthSetAgePublic (
    unsigned                    ageInfoId,
    bool                        publicOrNot
) {
    CliAuConn * conn = GetConnIncRef("SetAgePublic");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
        kCli2Auth_SetAgePublic,
                    ageInfoId,
                    publicOrNot,
    };
    
    conn->Send(msg, arrsize(msg));

    conn->DecRef("SetAgePublic");
}

//============================================================================
void NetCliAuthGetPublicAgeList (
    const wchar                         ageName[],
    FNetCliAuthGetPublicAgeListCallback callback,
    void *                              param
) {
    GetPublicAgeListTrans * trans = NEW(GetPublicAgeListTrans)(
        ageName,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAccountChangePasswordRequest (
    const wchar                                     accountName[],
    const wchar                                     password[],
    FNetCliAuthAccountChangePasswordRequestCallback callback,
    void *                                          param
) {
    AccountChangePasswordRequestTrans * trans = NEW(AccountChangePasswordRequestTrans)(
        accountName,
        password,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAccountSetRolesRequest (
    const wchar                                 accountName[],
    unsigned                                    accountFlags,
    FNetCliAuthAccountSetRolesRequestCallback   callback,
    void *                                      param
) {
    AccountSetRolesRequestTrans * trans = NEW(AccountSetRolesRequestTrans)(
        accountName,
        accountFlags,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAccountSetBillingTypeRequest (
    const wchar                                     accountName[],
    unsigned                                        billingType,
    FNetCliAuthAccountSetBillingTypeRequestCallback callback,
    void *                                          param
) {
    AccountSetBillingTypeRequestTrans * trans = NEW(AccountSetBillingTypeRequestTrans)(
        accountName,
        billingType,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthAccountActivateRequest (
    const Uuid &                                activationKey,
    FNetCliAuthAccountActivateRequestCallback   callback,
    void *                                      param
) {
    AccountActivateRequestTrans * trans = NEW(AccountActivateRequestTrans)(
        activationKey,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthFileListRequest (
    const wchar                         dir[],
    const wchar                         ext[],
    FNetCliAuthFileListRequestCallback  callback,
    void *                              param
) {
    FileListRequestTrans * trans = NEW(FileListRequestTrans)(
        callback,
        param,
        dir,
        ext
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthFileRequest (
    const wchar                     filename[],
    hsStream *                      writer,
    FNetCliAuthFileRequestCallback  callback,
    void *                          param
) {
    FileDownloadRequestTrans * trans = NEW(FileDownloadRequestTrans)(
        callback,
        param,
        filename,
        writer
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthVaultSetRecvNodeChangedHandler (
    FNetCliAuthVaultNodeChanged handler
) {
    s_vaultNodeChangedHandler   = handler;
}

//============================================================================
void NetCliAuthVaultSetRecvNodeAddedHandler (
    FNetCliAuthVaultNodeAdded   handler
) {
    s_vaultNodeAddedHandler     = handler;
}

//============================================================================
void NetCliAuthVaultSetRecvNodeRemovedHandler (
    FNetCliAuthVaultNodeRemoved handler
) {
    s_vaultNodeRemovedHandler   = handler;
}

//============================================================================
void NetCliAuthVaultSetRecvNodeDeletedHandler (
    FNetCliAuthVaultNodeDeleted handler
) {
    s_vaultNodeDeletedHandler   = handler;
}

//============================================================================
void NetCliAuthVaultNodeCreate (
    NetVaultNode *              templateNode,
    FNetCliAuthVaultNodeCreated callback,
    void *                      param
) {
    VaultCreateNodeTrans * trans = NEWZERO(VaultCreateNodeTrans)(
        templateNode,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthVaultNodeFetch (
    unsigned                    nodeId,
    FNetCliAuthVaultNodeFetched callback,
    void *                      param
) {
    VaultFetchNodeTrans * trans = NEWZERO(VaultFetchNodeTrans)(
        nodeId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthVaultNodeFind (
    NetVaultNode *              templateNode,
    FNetCliAuthVaultNodeFind    callback,
    void *                      param
) {
    VaultFindNodeTrans * trans = NEWZERO(VaultFindNodeTrans)(
        templateNode,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
unsigned NetCliAuthVaultNodeSave (
    NetVaultNode *                      node,
    FNetCliAuthVaultNodeSaveCallback    callback,
    void *                              param
) {
    ASSERTMSG(!(node->dirtyFlags & NetVaultNode::kNodeType), "Node type may not be changed");
    
    // Clear dirty bits of read-only fields before we write the node to the msg buffer
    node->dirtyFlags &= ~(
        NetVaultNode::kNodeId |
        NetVaultNode::kNodeType |
        NetVaultNode::kCreatorAcct |
        NetVaultNode::kCreatorId |
        NetVaultNode::kCreateTime
    );
    
    if (!node->dirtyFlags)
        return 0;
        
    if (!node->nodeId)
        return 0;
        
    // force sending of the nodeType value, since the auth needs it.
    // auth will clear the field before sending it on to the vault.
    node->dirtyFlags |= NetVaultNode::kNodeType;

    // We're definitely saving this node, so assign a revisionId
    node->revisionId = GuidGenerate();

    ARRAY(byte) buffer;
    unsigned bytes = node->Write_LCS(&buffer, NetVaultNode::kRwDirtyOnly | NetVaultNode::kRwUpdateDirty);
    
    VaultSaveNodeTrans * trans = NEWZERO(VaultSaveNodeTrans)(
        node->nodeId,
        node->revisionId,
        buffer.Count(),
        buffer.Ptr(),
        callback,
        param
    );
    NetTransSend(trans);
    return bytes;
}

//============================================================================
void NetCliAuthVaultNodeDelete (
    unsigned                    nodeId
) {
    hsAssert(false, "eric, implement me");
}

//============================================================================
void NetCliAuthVaultNodeAdd (
    unsigned                        parentId,
    unsigned                        childId,
    unsigned                        ownerId,
    FNetCliAuthVaultNodeAddCallback callback,
    void *                          param
) {
    VaultAddNodeTrans * trans = NEWZERO(VaultAddNodeTrans)(
        parentId,
        childId,
        ownerId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthVaultNodeRemove (
    unsigned                            parentId,
    unsigned                            childId,
    FNetCliAuthVaultNodeRemoveCallback  callback,
    void *                              param
) {
    VaultRemoveNodeTrans * trans = NEWZERO(VaultRemoveNodeTrans)(
        parentId,
        childId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthVaultFetchNodeRefs (
    unsigned                        nodeId,
    FNetCliAuthVaultNodeRefsFetched callback,
    void *                          param
) {
    VaultFetchNodeRefsTrans * trans = NEWZERO(VaultFetchNodeRefsTrans)(
        nodeId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthVaultSetSeen (
    unsigned    parentId,
    unsigned    childId,
    bool        seen
) {
    CliAuConn * conn = GetConnIncRef("SetSeen");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultSetSeen,
                    parentId,
                    childId,
                    seen,
    };
    
    conn->Send(msg, arrsize(msg));

    conn->DecRef("SetSeen");
}

//============================================================================
void NetCliAuthVaultSendNode (
    unsigned    srcNodeId,
    unsigned    dstPlayerId
) {
    CliAuConn * conn = GetConnIncRef("SendNode");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
        kCli2Auth_VaultSendNode,
                    srcNodeId,
                    dstPlayerId,
    };
    
    conn->Send(msg, arrsize(msg));

    conn->DecRef("SendNode");
}

//============================================================================
void NetCliAuthVaultInitAge (
    const Uuid &                ageInstId,          // optional. is used in match
    const Uuid &                parentAgeInstId,    // optional. is used in match
    const wchar                 ageFilename[],      // optional. is used in match
    const wchar                 ageInstName[],      // optional. not used in match
    const wchar                 ageUserName[],      // optional. not used in match
    const wchar                 ageDesc[],          // optional. not used in match
    unsigned                    ageSequenceNumber,  // optional. not used in match
    unsigned                    ageLanguage,        // optional. not used in match
    FNetCliAuthAgeInitCallback  callback,           // optional
    void *                      param               // optional
) {
    VaultInitAgeTrans * trans = NEWZERO(VaultInitAgeTrans)(
        callback,
        param,
        ageInstId,
        parentAgeInstId,
        ageFilename,
        ageInstName,
        ageUserName,
        ageDesc,
        ageSequenceNumber,
        ageLanguage
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthSetRecvBufferHandler (
    FNetCliAuthRecvBufferHandler    handler
) {
    s_bufRcvdCb = handler;
}

//============================================================================
void NetCliAuthSendCCRPetition (
    const wchar *       petitionText
) {
    hsAssert(false, "eric, implement me.");
}

//============================================================================
void NetCliAuthPropagateBuffer (
    unsigned                        type,
    unsigned                        bytes,
    const byte                      buffer[]
) {
    CliAuConn * conn = GetConnIncRef("PropBuffer");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
        kCli2Auth_PropagateBuffer,
        type,
        bytes,
        (unsigned_ptr) buffer,
    };

    conn->Send(msg, arrsize(msg));

    conn->DecRef("PropBuffer");

}

//============================================================================
void NetCliAuthLogPythonTraceback (const wchar traceback[]) {
    CliAuConn * conn = GetConnIncRef("LogTraceback");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
       kCli2Auth_LogPythonTraceback,
       (unsigned_ptr) traceback
    };

    conn->Send(msg, arrsize(msg));

    conn->DecRef("LogTraceback");
}


//============================================================================
void NetCliAuthLogStackDump (const wchar stackdump[]) {
    CliAuConn * conn = GetConnIncRef("LogStackDump");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
       kCli2Auth_LogStackDump,
       (unsigned_ptr) stackdump
    };

    conn->Send(msg, arrsize(msg));

    conn->DecRef("LogStackDump");
}

//============================================================================
void NetCliAuthLogClientDebuggerConnect () {
    CliAuConn * conn = GetConnIncRef("");
    if (!conn)
        return;

    unsigned nothing = 0;

    const unsigned_ptr msg[] = {
       kCli2Auth_LogClientDebuggerConnect,
        nothing
    };

    conn->Send(msg, arrsize(msg));

    conn->DecRef();
}

//============================================================================
void NetCliAuthSetNotifyNewBuildHandler (FNotifyNewBuildHandler handler) {

    s_notifyNewBuildHandler = handler;
}

//============================================================================
void NetCliAuthSetPlayerBanStatusRequest (
    unsigned                                        playerId,
    unsigned                                        banned,
    FNetCliAuthSetPlayerBanStatusRequestCallback    callback,
    void *                                          param
) {
    SetPlayerBanStatusRequestTrans * trans = NEW(SetPlayerBanStatusRequestTrans)(
        playerId,
        banned,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthKickPlayer (
    unsigned                                playerId
) {
    CliAuConn * conn = GetConnIncRef("KickPlayer");
    if (!conn)
        return;

    const unsigned_ptr msg[] = {
       kCli2Auth_KickPlayer,
                    playerId
    };

    conn->Send(msg, arrsize(msg));
    conn->DecRef("KickPlayer");
}

//============================================================================
void NetCliAuthChangePlayerNameRequest (
    unsigned                                    playerId,
    const wchar                                 newName[],
    FNetCliAuthChangePlayerNameRequestCallback  callback,
    void *                                      param
) {
    ChangePlayerNameRequestTrans * trans = NEW(ChangePlayerNameRequestTrans)(
        playerId,
        newName,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthSendFriendInvite (
    const wchar                         emailAddress[],
    const wchar                         toName[],
    const Uuid&                         inviteUuid,
    FNetCliAuthSendFriendInviteCallback callback,
    void *                              param
) {
    SendFriendInviteTrans * trans = NEW(SendFriendInviteTrans)(
        emailAddress,
        toName,
        inviteUuid,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreCreate (
    unsigned                        ownerId,
    const char*                     gameName,
    unsigned                        gameType,
    int                             value,
    FNetCliAuthCreateScoreCallback  callback,
    void *                          param
) {
    ScoreCreateTrans * trans = NEW(ScoreCreateTrans)(
        ownerId,
        gameName,
        gameType,
        value,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreDelete(
    unsigned                        scoreId,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) {
    ScoreDeleteTrans * trans = NEW(ScoreDeleteTrans)(
        scoreId,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreGetScores(
    unsigned                        ownerId,
    const char*                     gameName,
    FNetCliAuthGetScoresCallback    callback,
    void *                          param
) {
    ScoreGetScoresTrans * trans = NEW(ScoreGetScoresTrans)(
        ownerId,
        gameName,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreAddPoints(
    unsigned                        scoreId,
    int                             numPoints,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) {
    ScoreAddPointsTrans * trans = NEW(ScoreAddPointsTrans)(
        scoreId,
        numPoints,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreTransferPoints(
    unsigned                        srcScoreId,
    unsigned                        destScoreId,
    int                             numPoints,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) {
    ScoreTransferPointsTrans * trans = NEW(ScoreTransferPointsTrans)(
        srcScoreId,
        destScoreId,
        numPoints,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreSetPoints(
    unsigned                        scoreId,
    int                             numPoints,
    FNetCliAuthScoreUpdateCallback  callback,
    void *                          param
) {
    ScoreSetPointsTrans * trans = NEW(ScoreSetPointsTrans)(
        scoreId,
        numPoints,
        callback,
        param
    );
    NetTransSend(trans);
}

//============================================================================
void NetCliAuthScoreGetRankList(
    unsigned                    ownerId,
    unsigned                    scoreGroup,
    unsigned                    parentFolderId,
    const char *                gameName,
    unsigned                    timePeriod,
    unsigned                    numResults,
    unsigned                    pageNumber,
    bool                        sortDesc,
    FNetCliAuthGetRanksCallback callback,
    void *                      param
) {
    ScoreGetRanksTrans * trans = NEW(ScoreGetRanksTrans)(
        ownerId,
        scoreGroup,
        parentFolderId,
        gameName,
        timePeriod,
        numResults,
        pageNumber,
        sortDesc,
        callback,
        param
    );
    NetTransSend(trans);
}
