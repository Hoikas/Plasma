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

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/
/*****************************************************************************
*
*   $/Plasma20/Sources/Plasma/NucleusLib/pnNetDiag/Intern.h
*   
***/

#ifdef PLASMA20_SOURCES_PLASMA_NUCLEUSLIB_PNNETDIAG_INTERN_H
#error "Header $/Plasma20/Sources/Plasma/NucleusLib/pnNetDiag/Intern.h included more than once"
#endif
#define PLASMA20_SOURCES_PLASMA_NUCLEUSLIB_PNNETDIAG_INTERN_H


namespace ND {

extern HMODULE          g_lib;
extern const wchar      g_version[];


//============================================================================
enum {
    kDiagSrvAuth,
    kDiagSrvFile,
    kNumDiagSrvs
};

//============================================================================
inline unsigned NetProtocolToSrv (ENetProtocol protocol) {

    switch (protocol) {
        case kNetProtocolCli2Auth:  return kDiagSrvAuth;
        case kNetProtocolCli2File:  return kDiagSrvFile;
        default:                    return kNumDiagSrvs;
    }
}

//============================================================================
inline const wchar * SrvToString (unsigned srv) {
    
    switch (srv) {
        case kDiagSrvAuth:  return L"Auth";
        case kDiagSrvFile:  return L"File";
        DEFAULT_FATAL(srv);
    }
}

} using namespace ND;


//============================================================================
struct NetDiag : AtomicRef {

    bool            destroyed;
    CCritSect       critsect;
    wchar *         hosts[kNumDiagSrvs];
    unsigned        nodes[kNumDiagSrvs];
    
    ~NetDiag ();

    void Destroy ();
    void SetHost (unsigned srv, const wchar host[]);    
};


/*****************************************************************************
*
*   SYS
*
***/

void SysStartup ();
void SysShutdown ();


/*****************************************************************************
*
*   DNS
*
***/

void DnsStartup ();
void DnsShutdown ();


/*****************************************************************************
*
*   ICMP
*
***/

void IcmpStartup ();
void IcmpShutdown ();


/*****************************************************************************
*
*   TCP
*
***/

void TcpStartup ();
void TcpShutdown ();

