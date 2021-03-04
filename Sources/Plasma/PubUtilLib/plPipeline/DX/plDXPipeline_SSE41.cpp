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

#include "plDXPipeline.h"

#include <smmintrin.h>

static inline void ISkinDpSSE41(const float* src, float* dst, const __m128& mc0,
                                const __m128& mc1, const __m128& mc2, const __m128& mwt)
{
    enum { DP_F4_X = 0xF1, DP_F4_Y = 0xF2, DP_F4_Z = 0xF4 };

    __m128 msr = _mm_load_ps(src);
    __m128 _r =        _mm_dp_ps(msr, mc0, DP_F4_X);
    _r = _mm_or_ps(_r, _mm_dp_ps(msr, mc1, DP_F4_Y));
    _r = _mm_or_ps(_r, _mm_dp_ps(msr, mc2, DP_F4_Z));

    __m128 _dst = _mm_load_ps(dst);
    _dst = _mm_add_ps(_dst, _mm_mul_ps(_r, mwt));
    _mm_store_ps(dst, _dst);
}

void plDXPipeline::ISkinVertexSSE41(const hsMatrix44& xfm, float wgt,
                                    const float* pt_src, float* pt_dst,
                                    const float* vec_src, float* vec_dst)
{
    __m128 mc0 = _mm_load_ps(xfm.fMap[0]);
    __m128 mc1 = _mm_load_ps(xfm.fMap[1]);
    __m128 mc2 = _mm_load_ps(xfm.fMap[2]);
    __m128 mwt = _mm_set_ps1(wgt);

    ISkinDpSSE41(pt_src, pt_dst, mc0, mc1, mc2, mwt);
    ISkinDpSSE41(vec_src, vec_dst, mc0, mc1, mc2, mwt);
}