/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cmdlib.h"
#include "mathlib.h"
#include "inout.h"
#include "bspfile.h"
#include "scriplib.h"

void            GetLeafNums(void);

//=============================================================================

int             numModels;
dmodel_t        dmodels[MAX_MAP_MODELS];

int             numShaders;
dshader_t       dshaders[MAX_MAP_SHADERS];

int             entDataSize;
char            dentdata[MAX_MAP_ENTSTRING];

int             numLeafs;
dleaf_t         dleafs[MAX_MAP_LEAFS];

int             numPlanes;
dplane_t        dplanes[MAX_MAP_PLANES];

int             numNodes;
dnode_t         dnodes[MAX_MAP_NODES];

int             numLeafSurfaces;
int             dleafsurfaces[MAX_MAP_LEAFFACES];

int             numLeafBrushes;
int             dleafbrushes[MAX_MAP_LEAFBRUSHES];

int             numBrushes;
dbrush_t        dbrushes[MAX_MAP_BRUSHES];

int             numBrushSides;
dbrushside_t    dbrushsides[MAX_MAP_BRUSHSIDES];

int             numLightBytes;
byte            lightBytes[MAX_MAP_LIGHTING];

int             numGridPoints;
byte            gridData[MAX_MAP_LIGHTGRID];

int             numVisBytes;
byte            visBytes[MAX_MAP_VISIBILITY];

int             numDrawVerts;
drawVert_t      drawVerts[MAX_MAP_DRAW_VERTS];

int             numDrawIndexes;
int             drawIndexes[MAX_MAP_DRAW_INDEXES];

int             numDrawSurfaces;
dsurface_t      drawSurfaces[MAX_MAP_DRAW_SURFS];

int             numFogs;
dfog_t          dfogs[MAX_MAP_FOGS];

//=============================================================================

/*
=============
SwapBlock

If all values are 32 bits, this can be used to swap everything
=============
*/
void SwapBlock(int *block, int sizeOfBlock)
{
	int             i;

	sizeOfBlock >>= 2;
	for(i = 0; i < sizeOfBlock; i++)
	{
		block[i] = LittleLong(block[i]);
	}
}

/*
=============
SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void SwapBSPFile(void)
{
	int             i;

	// models
	SwapBlock((int *)dmodels, numModels * sizeof(dmodels[0]));

	// shaders (don't swap the name)
	for(i = 0; i < numShaders; i++)
	{
		dshaders[i].contentFlags = LittleLong(dshaders[i].contentFlags);
		dshaders[i].surfaceFlags = LittleLong(dshaders[i].surfaceFlags);
	}

	// planes
	SwapBlock((int *)dplanes, numPlanes * sizeof(dplanes[0]));

	// nodes
	SwapBlock((int *)dnodes, numNodes * sizeof(dnodes[0]));

	// leafs
	SwapBlock((int *)dleafs, numLeafs * sizeof(dleafs[0]));

	// leaffaces
	SwapBlock((int *)dleafsurfaces, numLeafSurfaces * sizeof(dleafsurfaces[0]));

	// leafbrushes
	SwapBlock((int *)dleafbrushes, numLeafBrushes * sizeof(dleafbrushes[0]));

	// brushes
	SwapBlock((int *)dbrushes, numBrushes * sizeof(dbrushes[0]));

	// brushsides
	SwapBlock((int *)dbrushsides, numBrushSides * sizeof(dbrushsides[0]));

	// vis
	((int *)&visBytes)[0] = LittleLong(((int *)&visBytes)[0]);
	((int *)&visBytes)[1] = LittleLong(((int *)&visBytes)[1]);

	// drawverts
	for(i = 0; i < numDrawVerts; i++)
	{
		drawVerts[i].lightmap[0] = LittleFloat(drawVerts[i].lightmap[0]);
		drawVerts[i].lightmap[1] = LittleFloat(drawVerts[i].lightmap[1]);

		drawVerts[i].st[0] = LittleFloat(drawVerts[i].st[0]);
		drawVerts[i].st[1] = LittleFloat(drawVerts[i].st[1]);

		drawVerts[i].xyz[0] = LittleFloat(drawVerts[i].xyz[0]);
		drawVerts[i].xyz[1] = LittleFloat(drawVerts[i].xyz[1]);
		drawVerts[i].xyz[2] = LittleFloat(drawVerts[i].xyz[2]);

		drawVerts[i].normal[0] = LittleFloat(drawVerts[i].normal[0]);
		drawVerts[i].normal[1] = LittleFloat(drawVerts[i].normal[1]);
		drawVerts[i].normal[2] = LittleFloat(drawVerts[i].normal[2]);

		drawVerts[i].paintColor[0] = LittleFloat(drawVerts[i].paintColor[0]);
		drawVerts[i].paintColor[1] = LittleFloat(drawVerts[i].paintColor[1]);
		drawVerts[i].paintColor[2] = LittleFloat(drawVerts[i].paintColor[2]);
		drawVerts[i].paintColor[3] = LittleFloat(drawVerts[i].paintColor[3]);

		drawVerts[i].lightColor[0] = LittleFloat(drawVerts[i].lightColor[0]);
		drawVerts[i].lightColor[1] = LittleFloat(drawVerts[i].lightColor[1]);
		drawVerts[i].lightColor[2] = LittleFloat(drawVerts[i].lightColor[2]);
		drawVerts[i].lightColor[3] = LittleFloat(drawVerts[i].lightColor[3]);

		drawVerts[i].lightDirection[0] = LittleFloat(drawVerts[i].lightDirection[0]);
		drawVerts[i].lightDirection[1] = LittleFloat(drawVerts[i].lightDirection[1]);
		drawVerts[i].lightDirection[2] = LittleFloat(drawVerts[i].lightDirection[2]);
	}

	// drawindexes
	SwapBlock((int *)drawIndexes, numDrawIndexes * sizeof(drawIndexes[0]));

	// drawsurfs
	SwapBlock((int *)drawSurfaces, numDrawSurfaces * sizeof(drawSurfaces[0]));

	// fogs
	for(i = 0; i < numFogs; i++)
	{
		dfogs[i].brushNum = LittleLong(dfogs[i].brushNum);
		dfogs[i].visibleSide = LittleLong(dfogs[i].visibleSide);
	}
}



/*
=============
CopyLump
=============
*/
int CopyLump(dheader_t * header, int lump, void *dest, int size)
{
	int             length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;

	if(length == 0)
		return 0;

	if(length % size)
	{
		Error("LoadBSPFile: odd lump size");
	}

	memcpy(dest, (byte *) header + ofs, length);

	return length / size;
}

/*
=============
LoadBSPFile
=============
*/
void LoadBSPFile(const char *filename)
{
	dheader_t      *header;

	// load the file header
	LoadFile(filename, (void **)&header);

	// swap the header
	SwapBlock((int *)header, sizeof(*header));

	if(header->ident != BSP_IDENT)
	{
		Error("%s is not a IBSP file", filename);
	}
	if(header->version != BSP_VERSION)
	{
		Error("%s is version %i, not %i", filename, header->version, BSP_VERSION);
	}

	numShaders = CopyLump(header, LUMP_SHADERS, dshaders, sizeof(dshader_t));
	numModels = CopyLump(header, LUMP_MODELS, dmodels, sizeof(dmodel_t));
	numPlanes = CopyLump(header, LUMP_PLANES, dplanes, sizeof(dplane_t));
	numLeafs = CopyLump(header, LUMP_LEAFS, dleafs, sizeof(dleaf_t));
	numNodes = CopyLump(header, LUMP_NODES, dnodes, sizeof(dnode_t));
	numLeafSurfaces = CopyLump(header, LUMP_LEAFSURFACES, dleafsurfaces, sizeof(dleafsurfaces[0]));
	numLeafBrushes = CopyLump(header, LUMP_LEAFBRUSHES, dleafbrushes, sizeof(dleafbrushes[0]));
	numBrushes = CopyLump(header, LUMP_BRUSHES, dbrushes, sizeof(dbrush_t));
	numBrushSides = CopyLump(header, LUMP_BRUSHSIDES, dbrushsides, sizeof(dbrushside_t));
	numDrawVerts = CopyLump(header, LUMP_DRAWVERTS, drawVerts, sizeof(drawVert_t));
	numDrawSurfaces = CopyLump(header, LUMP_SURFACES, drawSurfaces, sizeof(dsurface_t));
	numFogs = CopyLump(header, LUMP_FOGS, dfogs, sizeof(dfog_t));
	numDrawIndexes = CopyLump(header, LUMP_DRAWINDEXES, drawIndexes, sizeof(drawIndexes[0]));

	numVisBytes = CopyLump(header, LUMP_VISIBILITY, visBytes, 1);
	numLightBytes = CopyLump(header, LUMP_LIGHTMAPS, lightBytes, 1);
	entDataSize = CopyLump(header, LUMP_ENTITIES, dentdata, 1);

	numGridPoints = CopyLump(header, LUMP_LIGHTGRID, gridData, 8);


	free(header);				// everything has been copied out

	// swap everything
	SwapBSPFile();
}


//============================================================================

/*
=============
AddLump
=============
*/
void AddLump(FILE * bspfile, dheader_t * header, int lumpnum, const void *data, int len)
{
	lump_t         *lump;

	lump = &header->lumps[lumpnum];

	lump->fileofs = LittleLong(ftell(bspfile));
	lump->filelen = LittleLong(len);
	SafeWrite(bspfile, data, (len + 3) & ~3);
}

/*
=============
WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void WriteBSPFile(const char *filename)
{
	dheader_t       outheader, *header;
	FILE           *bspfile;

	header = &outheader;
	memset(header, 0, sizeof(dheader_t));

	SwapBSPFile();

	header->ident = LittleLong(BSP_IDENT);
	header->version = LittleLong(BSP_VERSION);

	bspfile = SafeOpenWrite(filename);
	SafeWrite(bspfile, header, sizeof(dheader_t));	// overwritten later

	AddLump(bspfile, header, LUMP_SHADERS, dshaders, numShaders * sizeof(dshader_t));
	AddLump(bspfile, header, LUMP_PLANES, dplanes, numPlanes * sizeof(dplane_t));
	AddLump(bspfile, header, LUMP_LEAFS, dleafs, numLeafs * sizeof(dleaf_t));
	AddLump(bspfile, header, LUMP_NODES, dnodes, numNodes * sizeof(dnode_t));
	AddLump(bspfile, header, LUMP_BRUSHES, dbrushes, numBrushes * sizeof(dbrush_t));
	AddLump(bspfile, header, LUMP_BRUSHSIDES, dbrushsides, numBrushSides * sizeof(dbrushside_t));
	AddLump(bspfile, header, LUMP_LEAFSURFACES, dleafsurfaces, numLeafSurfaces * sizeof(dleafsurfaces[0]));
	AddLump(bspfile, header, LUMP_LEAFBRUSHES, dleafbrushes, numLeafBrushes * sizeof(dleafbrushes[0]));
	AddLump(bspfile, header, LUMP_MODELS, dmodels, numModels * sizeof(dmodel_t));
	AddLump(bspfile, header, LUMP_DRAWVERTS, drawVerts, numDrawVerts * sizeof(drawVert_t));
	AddLump(bspfile, header, LUMP_SURFACES, drawSurfaces, numDrawSurfaces * sizeof(dsurface_t));
	AddLump(bspfile, header, LUMP_VISIBILITY, visBytes, numVisBytes);
	AddLump(bspfile, header, LUMP_LIGHTMAPS, lightBytes, numLightBytes);
	AddLump(bspfile, header, LUMP_LIGHTGRID, gridData, 8 * numGridPoints);
	AddLump(bspfile, header, LUMP_ENTITIES, dentdata, entDataSize);
	AddLump(bspfile, header, LUMP_FOGS, dfogs, numFogs * sizeof(dfog_t));
	AddLump(bspfile, header, LUMP_DRAWINDEXES, drawIndexes, numDrawIndexes * sizeof(drawIndexes[0]));

	fseek(bspfile, 0, SEEK_SET);
	SafeWrite(bspfile, header, sizeof(dheader_t));
	fclose(bspfile);
}

//============================================================================

/*
=============
PrintBSPFileSizes

Dumps info about current file
=============
*/
void PrintBSPFileSizes(void)
{
	if(!numEntities)
	{
		ParseEntities();
	}

	Sys_Printf("%6i models       %7i\n", numModels, (int)(numModels * sizeof(dmodel_t)));
	Sys_Printf("%6i shaders      %7i\n", numShaders, (int)(numShaders * sizeof(dshader_t)));
	Sys_Printf("%6i brushes      %7i\n", numBrushes, (int)(numBrushes * sizeof(dbrush_t)));
	Sys_Printf("%6i brushsides   %7i\n", numBrushSides, (int)(numBrushSides * sizeof(dbrushside_t)));
	Sys_Printf("%6i fogs         %7i\n", numFogs, (int)(numFogs * sizeof(dfog_t)));
	Sys_Printf("%6i planes       %7i\n", numPlanes, (int)(numPlanes * sizeof(dplane_t)));
	Sys_Printf("%6i entdata      %7i\n", numEntities, entDataSize);

	Sys_Printf("\n");

	Sys_Printf("%6i nodes        %7i\n", numNodes, (int)(numNodes * sizeof(dnode_t)));
	Sys_Printf("%6i leafs        %7i\n", numLeafs, (int)(numLeafs * sizeof(dleaf_t)));
	Sys_Printf("%6i leafsurfaces %7i\n", numLeafSurfaces, (int)(numLeafSurfaces * sizeof(dleafsurfaces[0])));
	Sys_Printf("%6i leafbrushes  %7i\n", numLeafBrushes, (int)(numLeafBrushes * sizeof(dleafbrushes[0])));
	Sys_Printf("%6i drawverts    %7i\n", numDrawVerts, (int)(numDrawVerts * sizeof(drawVerts[0])));
	Sys_Printf("%6i drawindexes  %7i\n", numDrawIndexes, (int)(numDrawIndexes * sizeof(drawIndexes[0])));
	Sys_Printf("%6i drawsurfaces %7i\n", numDrawSurfaces, (int)(numDrawSurfaces * sizeof(drawSurfaces[0])));

	Sys_Printf("%6i lightmaps    %7i\n", numLightBytes / (LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 3), numLightBytes);
	Sys_Printf("       visibility   %7i\n", numVisBytes);
}


//============================================

int             numEntities;
entity_t        entities[MAX_MAP_ENTITIES];

void StripTrailing(char *e)
{
	char           *s;

	s = e + strlen(e) - 1;
	while(s >= e && *s <= 32)
	{
		*s = 0;
		s--;
	}
}

/*
=================
ParseEpair
=================
*/
epair_t        *ParseEpair(void)
{
	epair_t        *e;

	e = safe_malloc(sizeof(epair_t));
	memset(e, 0, sizeof(epair_t));

	if(strlen(token) >= MAX_KEY - 1)
	{
		Error("ParseEpar: token too long");
	}
	e->key = copystring(token);
	GetToken(qfalse);
	if(strlen(token) >= MAX_VALUE - 1)
	{
		Error("ParseEpar: token too long");
	}
	e->value = copystring(token);

	// strip trailing spaces that sometimes get accidentally
	// added in the editor
	StripTrailing(e->key);
	StripTrailing(e->value);

	return e;
}


/*
================
ParseEntity
================
*/
qboolean ParseEntity(void)
{
	epair_t        *e;
	entity_t       *mapent;

	if(!GetToken(qtrue))
	{
		return qfalse;
	}

	if(strcmp(token, "{"))
	{
		Error("ParseEntity: { not found");
	}
	if(numEntities == MAX_MAP_ENTITIES)
	{
		Error("num_entities == MAX_MAP_ENTITIES");
	}
	mapent = &entities[numEntities];
	numEntities++;

	do
	{
		if(!GetToken(qtrue))
		{
			Error("ParseEntity: EOF without closing brace");
		}
		if(!strcmp(token, "}"))
		{
			break;
		}
		e = ParseEpair();
		e->next = mapent->epairs;
		mapent->epairs = e;
	} while(1);

	return qtrue;
}

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void ParseEntities(void)
{
	numEntities = 0;
	ParseFromMemory(dentdata, entDataSize);

	while(ParseEntity())
	{
	}
}


/*
================
UnparseEntities

Generates the dentdata string from all the entities
This allows the utilities to add or remove key/value pairs
to the data created by the map editor.
================
*/
void UnparseEntities(void)
{
	char           *buf, *end;
	epair_t        *ep;
	char            line[2048];
	int             i;
	char            key[1024], value[1024];

	buf = dentdata;
	end = buf;
	*end = 0;

	for(i = 0; i < numEntities; i++)
	{
		ep = entities[i].epairs;
		if(!ep)
		{
			continue;			// ent got removed
		}

		strcat(end, "{\n");
		end += 2;

		for(ep = entities[i].epairs; ep; ep = ep->next)
		{
			strcpy(key, ep->key);
			StripTrailing(key);
			strcpy(value, ep->value);
			StripTrailing(value);

			sprintf(line, "\"%s\" \"%s\"\n", key, value);
			strcat(end, line);
			end += strlen(line);
		}
		strcat(end, "}\n");
		end += 2;

		if(end > buf + MAX_MAP_ENTSTRING)
		{
			Error("Entity text too long");
		}
	}
	entDataSize = end - buf + 1;
}

void PrintEntity(const entity_t * ent)
{
	epair_t        *ep;

	Sys_Printf("------- entity %p -------\n", ent);
	for(ep = ent->epairs; ep; ep = ep->next)
	{
		Sys_Printf("%s = %s\n", ep->key, ep->value);
	}
}

qboolean HasUniqueEntityName(const entity_t * ent, const char *name)
{
	int             i;
	entity_t       *ent2;
	const char     *name2;

	for(i = 0; i < numEntities; i++)
	{
		ent2 = &entities[i];

		if(ent == ent2)
			continue;

		name2 = ValueForKey(ent2, "name");

		if(!Q_stricmp(name, name2))
		{
			return qfalse;
		}
	}

	return qtrue;
}

const char     *UniqueEntityName(const entity_t * ent, const char *suggestion)
{
	int             i;
	const char     *classname;
	const char     *uniquename;

	classname = ValueForKey(ent, "classname");

	for(i = 0; i < 9999; i++)
	{
		uniquename = va("%s_%i", classname, i);

		if(HasUniqueEntityName(ent, uniquename))
			return uniquename;
	}

	return "";
}

void SetKeyValue(entity_t * ent, const char *key, const char *value)
{
	epair_t        *ep;

	for(ep = ent->epairs; ep; ep = ep->next)
	{
		if(!Q_stricmp(ep->key, key))
		{
			free(ep->value);
			ep->value = copystring(value);
			return;
		}
	}
	ep = safe_malloc(sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	ep->key = copystring(key);
	ep->value = copystring(value);
}

const char     *ValueForKey(const entity_t * ent, const char *key)
{
	epair_t        *ep;

	for(ep = ent->epairs; ep; ep = ep->next)
	{
		if(!Q_stricmp(ep->key, key))
		{
			return ep->value;
		}
	}
	return "";
}

qboolean HasKey(const entity_t * ent, const char *key)
{
	epair_t        *ep;

	for(ep = ent->epairs; ep; ep = ep->next)
	{
		if(!Q_stricmp(ep->key, key))
		{
			return qtrue;
		}
	}
	return qfalse;
}

void RemoveKey(entity_t * ent, const char *key)
{
	epair_t        *ep;
	epair_t        *ep_prev;

	for(ep_prev = ep = ent->epairs; ep; ep = ep->next)
	{
		if(!Q_stricmp(ep->key, key))
		{
			free(ep->key);
			free(ep->value);

			/* link scheme
			   ep->next = ent->epairs;
			   ent->epairs = ep;
			 */

			// unlink
			if(ep->next == NULL)
			{
				// last element
				ep_prev->next = NULL;
			}
			else
			{
				ep_prev->next = ep->next;
			}

			free(ep);
			return;
		}

		ep_prev = ep;
	}
}

vec_t FloatForKey(const entity_t * ent, const char *key)
{
	const char     *k;

	k = ValueForKey(ent, key);
	return atof(k);
}

void GetVectorForKey(const entity_t * ent, const char *key, vec3_t vec)
{
	const char     *k;
	double          v1, v2, v3;

	k = ValueForKey(ent, key);

	// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf(k, "%lf %lf %lf", &v1, &v2, &v3);
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
}
