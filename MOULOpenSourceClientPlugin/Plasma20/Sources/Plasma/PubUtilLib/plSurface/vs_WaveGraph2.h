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


static const UInt32 vs_WaveGraph2ByteLen = 608;

static const UInt8 vs_WaveGraph2Codes[] = {
	0x1,	0x1,	0xfe,	0xff,
	0x1f,	0x0,	0x0,	0x0,
	0x0,	0x0,	0x0,	0x80,
	0x0,	0x0,	0xf,	0x90,
	0x1f,	0x0,	0x0,	0x0,
	0x3,	0x0,	0x0,	0x80,
	0x3,	0x0,	0xf,	0x90,
	0x1,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x0,	0x0,	0xe4,	0x90,
	0x1,	0x0,	0x0,	0x0,
	0x0,	0x0,	0x8,	0x80,
	0x0,	0x0,	0xaa,	0xa0,
	0x1,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0xc0,
	0x0,	0x0,	0xe4,	0x80,
	0x2,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x0,	0x0,	0x0,	0x90,
	0x0,	0x0,	0xaa,	0xa0,
	0x5,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x0,	0x0,	0xe4,	0x80,
	0x1,	0x0,	0xe4,	0xa0,
	0x2,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x0,	0x0,	0xe4,	0x80,
	0x2,	0x0,	0xe4,	0xa0,
	0x5,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x0,	0x0,	0xe4,	0x80,
	0x4,	0x0,	0x0,	0xa0,
	0x4e,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x2,	0x80,
	0x0,	0x0,	0x0,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x1,	0x80,
	0x1,	0x0,	0x55,	0x80,
	0x4e,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x2,	0x80,
	0x0,	0x0,	0xaa,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x4,	0x80,
	0x1,	0x0,	0x55,	0x80,
	0x4e,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x2,	0x80,
	0x0,	0x0,	0xff,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x8,	0x80,
	0x1,	0x0,	0x55,	0x80,
	0x4e,	0x0,	0x0,	0x0,
	0x1,	0x0,	0x2,	0x80,
	0x0,	0x0,	0x55,	0x80,
	0x5,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x1,	0x0,	0xe4,	0x80,
	0x4,	0x0,	0xff,	0xa0,
	0x2,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0x80,
	0x0,	0x0,	0xe4,	0x80,
	0x4,	0x0,	0xaa,	0xa1,
	0x5,	0x0,	0x0,	0x0,
	0x1,	0x0,	0xf,	0x80,
	0x0,	0x0,	0xe4,	0x80,
	0x0,	0x0,	0xe4,	0x80,
	0x5,	0x0,	0x0,	0x0,
	0x2,	0x0,	0xf,	0x80,
	0x1,	0x0,	0xe4,	0x80,
	0x1,	0x0,	0xe4,	0x80,
	0x5,	0x0,	0x0,	0x0,
	0x3,	0x0,	0xf,	0x80,
	0x1,	0x0,	0xe4,	0x80,
	0x2,	0x0,	0xe4,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x4,	0x0,	0xf,	0x80,
	0x5,	0x0,	0x0,	0xa0,
	0x4,	0x0,	0x0,	0x0,
	0x4,	0x0,	0xf,	0x80,
	0x1,	0x0,	0xe4,	0x80,
	0x5,	0x0,	0x55,	0xa0,
	0x4,	0x0,	0xe4,	0x80,
	0x4,	0x0,	0x0,	0x0,
	0x4,	0x0,	0xf,	0x80,
	0x2,	0x0,	0xe4,	0x80,
	0x5,	0x0,	0xaa,	0xa0,
	0x4,	0x0,	0xe4,	0x80,
	0x4,	0x0,	0x0,	0x0,
	0x4,	0x0,	0xf,	0x80,
	0x3,	0x0,	0xe4,	0x80,
	0x5,	0x0,	0xff,	0xa0,
	0x4,	0x0,	0xe4,	0x80,
	0x2,	0x0,	0x0,	0x0,
	0x4,	0x0,	0xf,	0x80,
	0x4,	0x0,	0xe4,	0x80,
	0x0,	0x0,	0xaa,	0xa0,
	0x5,	0x0,	0x0,	0x0,
	0x4,	0x0,	0xf,	0x80,
	0x4,	0x0,	0xe4,	0x80,
	0x3,	0x0,	0xe4,	0xa0,
	0x9,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x2,	0x80,
	0x4,	0x0,	0xe4,	0x80,
	0x0,	0x0,	0xaa,	0xa0,
	0x2,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x2,	0x80,
	0x6,	0x0,	0x0,	0xa0,
	0x5,	0x0,	0x55,	0x80,
	0x5,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x1,	0x80,
	0x0,	0x0,	0x0,	0x90,
	0x0,	0x0,	0x55,	0xa0,
	0x2,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x1,	0x80,
	0x5,	0x0,	0x0,	0x80,
	0x0,	0x0,	0x55,	0xa0,
	0x1,	0x0,	0x0,	0x0,
	0x5,	0x0,	0xc,	0x80,
	0x0,	0x0,	0xa8,	0xa0,
	0x5,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0xe0,
	0x5,	0x0,	0xe4,	0x80,
	0x3,	0x0,	0xf3,	0x90,
	0x5,	0x0,	0x0,	0x0,
	0x2,	0x0,	0xf,	0xe0,
	0x5,	0x0,	0xe4,	0x80,
	0x3,	0x0,	0xf3,	0x90,
	0x1,	0x0,	0x0,	0x0,
	0x6,	0x0,	0xf,	0x80,
	0x6,	0x0,	0xe4,	0xa0,
	0x2,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x1,	0x80,
	0x5,	0x0,	0x0,	0x80,
	0x6,	0x0,	0x55,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x2,	0x80,
	0x3,	0x0,	0x0,	0x90,
	0x5,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x3,	0x80,
	0x5,	0x0,	0xe4,	0x80,
	0x6,	0x0,	0xff,	0x80,
	0x4,	0x0,	0x0,	0x0,
	0x5,	0x0,	0x2,	0x80,
	0x6,	0x0,	0x0,	0x80,
	0x6,	0x0,	0xaa,	0x80,
	0x5,	0x0,	0x55,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x1,	0x0,	0xf,	0xe0,
	0x5,	0x0,	0xe4,	0x80,
	0x1,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0xd0,
	0x7,	0x0,	0xe4,	0xa0,
	0xff,	0xff,	0x0,	0x0
	};

static const plShaderDecl vs_WaveGraph2Decl("sha/vs_WaveGraph2.inl", vs_WaveGraph2, vs_WaveGraph2ByteLen, vs_WaveGraph2Codes);

static const plShaderRegister vs_WaveGraph2Register(&vs_WaveGraph2Decl);

