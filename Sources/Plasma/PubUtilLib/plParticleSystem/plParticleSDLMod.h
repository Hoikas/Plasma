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
#ifndef plParticleSDLMod_inc
#define plParticleSDLMod_inc

#include "plModifier/plSDLModifier.h"

//
// This modifier is responsible for sending and recving 
// particle system state. Few need it. Little state is needed.
//

class plParticleSDLMod : public plSDLModifier
{
private:
    bool fAttachedToAvatar;

protected:
    void IPutCurrentStateIn(plStateDataRecord* dstState);
    void ISetCurrentStateFrom(const plStateDataRecord* srcState);
    UInt32 IApplyModFlags(UInt32 sendFlags);
public:
    // var labels 
    static char kStrNumParticles[]; 
    
    CLASSNAME_REGISTER( plParticleSDLMod );
    GETINTERFACE_ANY( plParticleSDLMod, plSDLModifier);
    
    plParticleSDLMod(bool attachedToAvatar = false): fAttachedToAvatar(attachedToAvatar) {}
    
    void PutCurrentStateIn(plStateDataRecord* dstState);
    const char* GetSDLName() const { return kSDLParticleSystem; }

    void SetAttachedToAvatar(bool attached) {fAttachedToAvatar = attached;}
};

#endif  // plParticleSDLMod_inc
