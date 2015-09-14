/*
===========================================================================
Copyright (C) 2009-2010 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef TR_MODEL_SKEL_H
#define TR_MODEL_SKEL_H

#include "tr_local.h"

bool AddTriangleToVBOTriangleList( growList_t *vboTriangles, skelTriangle_t *tri, int *numBoneReferences, int boneReferences[ MAX_BONES ] );

void     AddSurfaceToVBOSurfacesList( growList_t *vboSurfaces, growList_t *vboTriangles, md5Model_t *md5, md5Surface_t *surf, int skinIndex, int boneReferences[ MAX_BONES ] );

#endif // TR_MODEL_SKEL_H
