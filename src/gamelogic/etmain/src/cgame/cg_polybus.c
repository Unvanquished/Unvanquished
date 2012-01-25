/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/



#include "cg_local.h"

#define MAX_PB_BUFFERS  128

polyBuffer_t    cg_polyBuffers[MAX_PB_BUFFERS];
qboolean        cg_polyBuffersInuse[MAX_PB_BUFFERS];

polyBuffer_t   *CG_PB_FindFreePolyBuffer(qhandle_t shader, int numVerts, int numIndicies)
{
	int             i;

	// Gordon: first find one with the same shader if possible
	for(i = 0; i < MAX_PB_BUFFERS; i++)
	{
		if(cg_polyBuffers[i].shader != shader)
		{
			continue;
		}

		if(!cg_polyBuffersInuse[i])
		{
			continue;
		}

		if(cg_polyBuffers[i].numIndicies + numIndicies >= MAX_PB_INDICIES)
		{
			continue;
		}

		if(cg_polyBuffers[i].numVerts + numVerts >= MAX_PB_VERTS)
		{
			continue;
		}

		cg_polyBuffersInuse[i] = qtrue;
		cg_polyBuffers[i].shader = shader;

		return &cg_polyBuffers[i];
	}

	// Gordon: or just find a free one
	for(i = 0; i < MAX_PB_BUFFERS; i++)
	{
		if(!cg_polyBuffersInuse[i])
		{
			cg_polyBuffersInuse[i] = qtrue;
			cg_polyBuffers[i].shader = shader;
			cg_polyBuffers[i].numIndicies = 0;
			cg_polyBuffers[i].numVerts = 0;

			return &cg_polyBuffers[i];
		}
	}

	return NULL;
}

void CG_PB_ClearPolyBuffers(void)
{
	// Gordon: changed numIndicies and numVerts to be reset in CG_PB_FindFreePolyBuffer, not here (should save the cache misses we were prolly getting)
	memset(cg_polyBuffersInuse, 0, sizeof(cg_polyBuffersInuse));
}

void CG_PB_RenderPolyBuffers(void)
{
	int             i;

	for(i = 0; i < MAX_PB_BUFFERS; i++)
	{
		if(cg_polyBuffersInuse[i])
		{
			trap_R_AddPolyBufferToScene(&cg_polyBuffers[i]);
		}
	}
}
