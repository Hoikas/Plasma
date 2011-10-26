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
#include "plStreamLogger.h"
#include "hsExceptions.h"

#ifdef STREAM_LOGGER

//
// Base Stream Logger
//

void plStreamLogger::LogEntry(plGenericType::Types type, unsigned int size, void* value, const char* desc)
{
    if (fList)
    {
        plGenericVar var(desc);
        var.Value().SetVar(type,size,value);
        Event e(Event::kValue,size,var);
        fList->push_back(e);
        fEntryWaiting = false;
    }
}

void plStreamLogger::ILogEntryWaiting()
{
    fEntryWaiting = true;
}

bool plStreamLogger::IsLogEntryWaiting()
{
    return fEntryWaiting;
}

//
// Read-Only Logging Stream
//

void hsReadOnlyLoggingStream::LogStringString(const char* s)
{
    if (fList)
    {
        plGenericVar var;

        var.SetName(s);
        fList->push_back(Event(Event::kString,0,var));
    }
}

void hsReadOnlyLoggingStream::LogSubStreamStart(const char* desc)
{
    if (fList)
    {
        plGenericVar var;
        if (!fDescStack.empty())
        {
            var.SetName(fDescStack.front().c_str());
            fDescStack.pop_front();
        }
        else
            var.SetName(desc);
        fList->push_back(Event(Event::kSubStart,0,var));
    }
}

void hsReadOnlyLoggingStream::LogSubStreamEnd()
{
    if (fList)
    {
        plGenericVar var;
        fList->push_back(Event(Event::kSubEnd,0,var));
    }
}

void hsReadOnlyLoggingStream::LogSubStreamPushDesc(const char* desc)
{
    fDescStack.push_back(std::string(desc));
}

void hsReadOnlyLoggingStream::Rewind()
{
    hsThrow( "can't rewind a logging stream");
}

void hsReadOnlyLoggingStream::FastFwd()
{
    hsThrow( "can't fast forward a logging stream");
}

void hsReadOnlyLoggingStream::SetPosition(UInt32 position)
{
    hsThrow( "can't set position on a logging stream");
}

void hsReadOnlyLoggingStream::Skip(UInt32 deltaByteCount)
{
    hsReadOnlyStream::Skip(deltaByteCount);
    if (deltaByteCount > 0 && !IsLogEntryWaiting())
    {
        LogEntry(plGenericType::kNone,deltaByteCount,nil,"Unknown Skip");
    }
}

UInt32 hsReadOnlyLoggingStream::Read(UInt32 byteCount, void * buffer)
{
    UInt32 ret = hsReadOnlyStream::Read(byteCount,buffer);
    if (ret > 0 && !IsLogEntryWaiting())
    {
        LogEntry(plGenericType::kNone,byteCount,nil,"Unknown Read");
    }

    return ret;
}


void hsReadOnlyLoggingStream::LogSkip(UInt32 deltaByteCount, const char* desc)
{
    ILogEntryWaiting();
    Skip(deltaByteCount);
    if (deltaByteCount > 0)
    {
        LogEntry(plGenericType::kNone,deltaByteCount,nil,desc);
    }
}

UInt32 hsReadOnlyLoggingStream::LogRead(UInt32 byteCount, void * buffer, const char* desc)
{
    ILogEntryWaiting();
    UInt32 ret = Read(byteCount,buffer);
    if (ret > 0)
    {
        LogEntry(plGenericType::kNone,byteCount,nil,desc);
    }

    return ret;
}
char *hsReadOnlyLoggingStream::LogReadSafeString()
{
    LogSubStreamStart("push me");
    UInt16 numChars; 
    LogReadLE(&numChars,"NumChars");

    numChars &= ~0xf000; // XXX: remove when hsStream no longer does this.
    if (numChars > 0)
    {       
        char *name = TRACKED_NEW char[numChars+1];
        ILogEntryWaiting();
        UInt32 ret = Read(numChars, name);
        name[numChars] = '\0';
        if (ret > 0)
        {
            LogEntry(plGenericType::kString,ret,name,"Value");
        }
        LogSubStreamEnd();
        return name;
    }
    LogSubStreamEnd();
    return nil;
}

char *hsReadOnlyLoggingStream::LogReadSafeStringLong()
{
    LogSubStreamStart("push me");
    UInt32 numChars; 
    LogReadLE(&numChars,"NumChars");
    if (numChars > 0)
    {
        char *name = TRACKED_NEW char[numChars+1];
        ILogEntryWaiting();
        UInt32 ret = Read(numChars, name);
        name[numChars] = '\0';
        if (ret > 0)
        {
            LogEntry(plGenericType::kString,ret,name,"Value");
        }
        LogSubStreamEnd();
        return name;
    }
    LogSubStreamEnd();
    return nil;
}
#endif
