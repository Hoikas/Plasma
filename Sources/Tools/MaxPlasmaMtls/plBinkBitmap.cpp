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
#include "hsTypes.h"
#include "plBinkBitmap.h"
#include "iparamb2.h"

class BinkClassDesc : public ClassDesc2
{
public:
    int             IsPublic()                      { return 1; }
    void*           Create(BOOL loading=FALSE)      { return TRACKED_NEW plBinkBitmapIO; }
    const TCHAR*    ClassName()                     { return "Bink"; }
    SClass_ID       SuperClassID()                  { return BMM_IO_CLASS_ID; }
    Class_ID        ClassID()                       { return Class_ID(0x71c75c3c, 0x206f480e); }
    const TCHAR*    Category()                      { return "Bitmap I/O"; }
};

static BinkClassDesc BinkDesc;
ClassDesc2* GetBinkClassDesc()
{
    return &BinkDesc;
}

plBinkBitmapIO::plBinkBitmapIO()
{
    fHBink = NULL;
    fName[0] = '\0';
}

plBinkBitmapIO::~plBinkBitmapIO()
{
    CloseBink();
}

BOOL plBinkBitmapIO::OpenBink(BitmapInfo* fbi)
{
    // If Bink file is already open, do nothing
    if (!strcmp(fbi->Name(), fName))
        return TRUE;

    // Otherwise, close any open Bink and load the new one
    CloseBink();

    strcpy(fName, fbi->Name());
    fHBink = BinkOpen(fbi->Name(), BINKNOSKIP | BINKALPHA);
    if (!fHBink)
    {
        fName[0] = '\0';
        return FALSE;
    }

    return TRUE;
}

void plBinkBitmapIO::CloseBink()
{
    if (fHBink)
    {
        BinkClose(fHBink);
        fHBink = NULL;
        fName[0] = '\0';
    }
}

BMMRES plBinkBitmapIO::GetImageInfo(BitmapInfo* fbi)
{
    //-- Get File Info -------------------------
    if (!OpenBink(fbi))
        return (ProcessImageIOError(fbi));
    
    //-- Update Bitmap Info ------------------------------
    fbi->SetWidth((WORD)fHBink->Width);
    fbi->SetHeight((WORD)fHBink->Height);
    fbi->SetType(BMM_TRUE_32);
    fbi->SetAspect(1.0f);
    fbi->SetFirstFrame(0);
    fbi->SetLastFrame(fHBink->Frames-1);
    fbi->SetFlags(MAP_HAS_ALPHA);

    return BMMRES_SUCCESS;
}

BitmapStorage* plBinkBitmapIO::Load(BitmapInfo* fbi, Bitmap* map, BMMRES* status)
{
    BitmapStorage* s = NULL;

    //-- Make sure nothing weird is going on
    if (openMode != BMM_NOT_OPEN)
    {
        *status = ProcessImageIOError(fbi,BMMRES_INTERNALERROR);
        return NULL;
    }

    // Update BitmapInfo with Bink file stats
    *status = GetImageInfo(fbi);
    if (*status != BMMRES_SUCCESS)
        return NULL;

    //-- Open Bink File -----------------------------------
    if (!OpenBink(fbi))
    {
        *status = ProcessImageIOError(fbi);
        return NULL;
    }

    // Get frame number //////////////////////////////////
    int frame;
    *status = GetFrame(fbi, &frame);
    if (*status != BMMRES_SUCCESS)
        return NULL;
    frame++;    // Bink frames start at 1

    //-- Create Image Storage ---------------------------- 
    s = BMMCreateStorage(map->Manager(), BMM_TRUE_32);
    if (!s)
    {
        *status = ProcessImageIOError(fbi, BMMRES_INTERNALERROR);
        return NULL;
    }

    //-- Allocate Image Storage --------------------------
    if (s->Allocate(fbi, map->Manager(), BMM_OPEN_R) == 0)
    {
        *status = ProcessImageIOError(fbi,BMMRES_MEMORYERROR);
        delete s;
        return NULL;
    }

    s->bi.CopyImageInfo(fbi);
    map->SetStorage(s);

    // Allocate space for decompressed image
    BYTE *Buffer = (BYTE*)LocalAlloc(LPTR, fbi->Width() * fbi->Height() * 4);
    if (!Buffer)
    {
        *status = ProcessImageIOError(fbi, BMMRES_MEMORYERROR);
        delete s;
        return NULL;
    }

    // Decompress the specified frame
    BinkGoto(fHBink, frame, NULL);
    BinkDoFrame(fHBink);
    // Copy frame to buffer
    BinkCopyToBuffer(fHBink, Buffer, fHBink->Width*4, fHBink->Height, 0, 0, BINKSURFACE32A | BINKCOPYALL);

    // Allocate space for one scanline, for copying
    BMM_Color_64 *line = (BMM_Color_64*)LocalAlloc(LPTR,fbi->Width()*sizeof(BMM_Color_64));
    if (!line)
    {
        *status = ProcessImageIOError(fbi, BMMRES_MEMORYERROR);
        delete s;
        LocalFree(Buffer);
        return NULL;
    }

    // Copy image
    BYTE *bf = Buffer;
    for (int y = 0; y < fbi->Height(); y++)
    {
        for (int x = 0; x < fbi->Width(); x++)
        {
            line[x].b = *bf << 8; bf++;
            line[x].g = *bf << 8; bf++;
            line[x].r = *bf << 8; bf++;
            line[x].a = *bf << 8; bf++;
        }

        map->PutPixels(0, y, fbi->Width(), line);

        if (fbi->GetUpdateWindow())
            SendMessage(fbi->GetUpdateWindow(), BMM_PROGRESS, y, fbi->Width());
    }

    LocalFree(line);
    LocalFree(Buffer);
    
    return s;
}

void plBinkBitmapIO::ShowAbout(HWND hWnd)
{
}