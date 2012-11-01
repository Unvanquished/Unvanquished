/* -------------------------------------------------------------------------------

Copyright (C) 1999-2007 id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

----------------------------------------------------------------------------------

This code has been altered significantly from its original form, to support
several games based on the Quake III Arena engine, in the form of "Q3Map2."

------------------------------------------------------------------------------- */



/* marker */
#define FACEBSP_C

//#define DEBUG_SPLITS 1



/* dependencies */
#include "q3map2.h"



int             c_faceLeafs;
int				c_faceNodes;


/*
================
AllocBspFace
================
*/
face_t         *AllocBspFace(void)
{
	face_t         *f;

	f = safe_malloc(sizeof(*f));
	memset(f, 0, sizeof(*f));

	return f;
}



/*
================
FreeBspFace
================
*/
void FreeBspFace(face_t * f)
{
	if(f->w)
	{
		FreeWinding(f->w);
	}
	free(f);
}



/*
SelectSplitPlaneNum()
finds the best split plane for this node
*/

static void SelectSplitPlaneNum(node_t * node, face_t * list, int *splitPlaneNum, int *compileFlags)
{
	face_t         *split;
	face_t         *check;
	face_t         *bestSplit;
	int             splits, facing, front, back;
	int             side;
	plane_t        *plane;
	int             value, bestValue;
	int             i;
	vec3_t          normal;
	float           dist;
	int             planenum;
	//float           sizeBias;
	//int				checks;
	int				frontC, backC, splitsC, facingC;


	/* ydnar: set some defaults */
	*splitPlaneNum = -1;		/* leaf */
	*compileFlags = 0;

	/* ydnar 2002-06-24: changed this to split on z-axis as well */
	/* ydnar 2002-09-21: changed blocksize to be a vector, so mappers can specify a 3 element value */

	/* if it is crossing a block boundary, force a split */
#if 1
	for(i = 0; i < 3; i++)
	{
		if(blockSize[i] <= 0)
			continue;

		dist = blockSize[i] * (floor(node->mins[i] / blockSize[i]) + 1);
		if(node->maxs[i] > dist)
		{
			VectorClear(normal);
			normal[i] = 1;
			planenum = FindFloatPlane(normal, dist, 0, NULL);
			*splitPlaneNum = planenum;
			return;
		}
	}
#endif

	/* pick one of the face planes */
	bestValue = -99999;
	bestSplit = list;

	//checks = 0;

#if 1
	// div0: this check causes detail/structural mixes
	//for(split = list; split; split = split->next)
	//	split->checked = qfalse;

#if defined(DEBUG_SPLITS)
	Sys_FPrintf(SYS_VRB, "split scores: [");
#endif
	for(split = list; split; split = split->next)
	{
		//if(split->checked)
		//	continue;

		//checks++;

		plane = &mapplanes[split->planenum];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;

		for(check = list; check; check = check->next)
		{
			if(check->planenum == split->planenum)
			{
				facing++;
				//check->checked = qtrue;   // won't need to test this plane again
				continue;
			}
			side = WindingOnPlaneSide(check->w, plane->normal, plane->dist);
			if(side == SIDE_CROSS)
			{
				splits++;
			}
			else if(side == SIDE_FRONT)
			{
				front++;
			}
			else if(side == SIDE_BACK)
			{
				back++;
			}
		}

		if(bspAlternateSplitWeights)
		{
			//Base score = 20000 perfectly balanced
			value = 0;//20000;
			value -= abs(front - back);	// prefer centered planes
			value -= plane->counter;	// if we've already used this plane sometime in the past try not to use it again
			value += facing * 5;		// if we're going to have alot of other surfs use this plane, we want to get it in quickly.
			value -= splits * 5;		// more splits = bad
			//value += sizeBias * 10;		// we want a huge score bias based on plane size

			if(plane->type < 3)
			{
				value += 5;		// axial is better
			}

			// we want a huge score bias based on plane size
#if 0
			{
				winding_t      *w;
				node_t         *n;
				plane_t        *plane;
				vec3_t          normal;
				vec_t           dist;

				// create temporary winding to draw the split plane
				w = CopyWinding(split->w);

				// clip by all the parents
				for(n = node->parent; n && w;)
				{
					plane = &mapplanes[n->planenum];

					if(n->children[0] == node)
					{
						// take front
						ChopWindingInPlace(&w, plane->normal, plane->dist, 0.001);	// BASE_WINDING_EPSILON
					}
					else
					{
						// take back
						VectorNegate(plane->normal, normal);
						dist = -plane->dist;
						ChopWindingInPlace(&w, normal, dist, 0.001); // BASE_WINDING_EPSILON
					}
					node = n;
					n = n->parent;
				}

				// clip by node AABB
				if(w != NULL)
					ChopWindingByBounds(&w, node->mins, node->maxs, CLIP_EPSILON);

				if(w != NULL)
					value += WindingArea(w);
			}
#endif
		}
		else
		{
			value = 5 * facing - 5 * splits;	// - abs(front-back);
			if(plane->type < 3)
			{
				value += 5;		// axial is better
			}
		}

		value += split->priority;	// prioritize hints higher

#if defined(DEBUG_SPLITS)
		Sys_FPrintf(SYS_VRB, " %d", value);
#endif

		if(value > bestValue)
		{
			bestValue = value;
			bestSplit = split;

			frontC = front;
			backC = back;
			splitsC = splits;
			facingC = facing;
		}
	}
#if defined(DEBUG_SPLITS)
	Sys_FPrintf(SYS_VRB, "]\n");
#endif

#endif

	/* nothing, we have a leaf */
	if(bestValue == -99999)
		return;

	//Sys_FPrintf(SYS_VRB, "F: %9d B:%9d S:%9d FA:%9d\n", frontC, backC, splitsC, facingC);

	/* set best split data */
	*splitPlaneNum = bestSplit->planenum;
	*compileFlags = bestSplit->compileFlags;

#if defined(DEBUG_SPLITS)
	if(bestSplit->compileFlags & C_DETAIL)
		for(split = list; split; split = split->next)
			if(!(split->compileFlags & C_DETAIL))
				Sys_FPrintf(SYS_ERR, "DON'T DO SUCH SPLITS (1)\n");
	if((node->compileFlags & C_DETAIL) && !(bestSplit->compileFlags & C_DETAIL))
		Sys_FPrintf(SYS_ERR, "DON'T DO SUCH SPLITS (2)\n");
#endif

	if(*splitPlaneNum > -1)
		mapplanes[*splitPlaneNum].counter++;
}



/*
CountFaceList()
counts bsp faces in the linked list
*/

int CountFaceList(face_t * list)
{
	int             c;


	c = 0;
	for(; list != NULL; list = list->next)
		c++;
	return c;
}

static tree_t  *drawTree = NULL;
static void DrawTreeNodes_r(node_t * node)
{
	vec4_t			nodeColor = {1, 1, 0, 0.3};
	vec4_t			leafColor = {0, 0, 1, 0.3};

	if(!node)
		return;

	if(node->planenum == PLANENUM_LEAF)
	{
		Draw_AABB(vec3_origin, node->mins, node->maxs, leafColor);
		return;
	}

	Draw_AABB(vec3_origin, node->mins, node->maxs, nodeColor);

	DrawTreeNodes_r(node->children[0]);
	DrawTreeNodes_r(node->children[1]);
}
static void DrawNodes(void)
{
	DrawTreeNodes_r(drawTree->headnode);
}

static face_t  *drawChildLists[2];
static node_t  *drawSplitNode;
static void DrawPartitions(void)
{
	face_t         *face;
	winding_t      *w;

	// create temporary winding to draw the split plane
	w = BaseWindingForNode(drawSplitNode);

	ChopWindingByBounds(&w, drawSplitNode->mins, drawSplitNode->maxs, 32);

	if(w != NULL)
	{
		Draw_Winding(w, 0, 0, 1, 0.3);
		FreeWinding(w);
	}

	for(face = drawChildLists[0]; face != NULL; face = face->next)
	{
		w = face->w;

		Draw_Winding(w, 0, 1, 0, 0.3);
	}

	for(face = drawChildLists[1]; face != NULL; face = face->next)
	{
		w = face->w;

		Draw_Winding(w, 1, 0, 0, 0.3);
	}
}

static void DrawAll(void)
{
	DrawPartitions();
	DrawNodes();
}


/*
BuildFaceTree_r()
recursively builds the bsp, splitting on face planes
*/

void BuildFaceTree_r(node_t * node, face_t * list)
{
	face_t         *split;
	face_t         *next;
	int             side;
	plane_t        *plane;
	face_t         *newFace;
	face_t         *childLists[2];
	winding_t      *frontWinding, *backWinding;
	int             i;
	int             splitPlaneNum, compileFlags;
	qboolean        isstruct = qfalse;
	int             splits, front, back;


	/* count faces left */
	i = CountFaceList(list);

#if defined(DEBUG_SPLITS)
	Sys_FPrintf(SYS_VRB, "faces left = %d\n", i);
#endif

	/* select the best split plane */
	SelectSplitPlaneNum(node, list, &splitPlaneNum, &compileFlags);

	/* if we don't have any more faces, this is a leaf */
	if(splitPlaneNum == -1)
	{
		node->planenum = PLANENUM_LEAF;
		node->has_structural_children = qfalse;
		c_faceLeafs++;
		return;
	}

	/* partition the list */
	node->planenum = splitPlaneNum;
	node->compileFlags = compileFlags;
	node->has_structural_children = !(compileFlags & C_DETAIL) && !node->opaque;
	plane = &mapplanes[splitPlaneNum];
	childLists[0] = NULL;
	childLists[1] = NULL;

	splits = front = back = 0;

	for(split = list; split; split = next)
	{
		/* set next */
		next = split->next;

		/* don't split by identical plane */
		if(split->planenum == node->planenum)
		{
			FreeBspFace(split);
			continue;
		}

		if(!(split->compileFlags & C_DETAIL))
			isstruct = 1;

		/* determine which side the face falls on */
		side = WindingOnPlaneSide(split->w, plane->normal, plane->dist);

		/* switch on side */
		if(side == SIDE_CROSS)
		{
			splits++;

			ClipWindingEpsilon(split->w, plane->normal, plane->dist, CLIP_EPSILON * 2, &frontWinding, &backWinding);
			if(frontWinding)
			{
				newFace = AllocBspFace();
				newFace->w = frontWinding;
				newFace->next = childLists[0];
				newFace->planenum = split->planenum;
				newFace->priority = split->priority;
				newFace->compileFlags = split->compileFlags;
				childLists[0] = newFace;

				front++;
			}
			if(backWinding)
			{
				newFace = AllocBspFace();
				newFace->w = backWinding;
				newFace->next = childLists[1];
				newFace->planenum = split->planenum;
				newFace->priority = split->priority;
				newFace->compileFlags = split->compileFlags;
				childLists[1] = newFace;

				back++;
			}
			FreeBspFace(split);
		}
		else if(side == SIDE_FRONT)
		{
			split->next = childLists[0];
			childLists[0] = split;

			front++;
		}
		else if(side == SIDE_BACK)
		{
			split->next = childLists[1];
			childLists[1] = split;

			back++;
		}
	}


	// recursively process children
	for(i = 0; i < 2; i++)
	{
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		VectorCopy(node->mins, node->children[i]->mins);
		VectorCopy(node->maxs, node->children[i]->maxs);

		c_faceNodes++;
	}

	for(i = 0; i < 3; i++)
	{
		if(plane->normal[i] == 1)
		{
			node->children[0]->mins[i] = plane->dist;
			node->children[1]->maxs[i] = plane->dist;
			break;
		}
	}

#if 1
	if(drawBSP && drawTree)
	{
		drawChildLists[0] = childLists[0];
		drawChildLists[1] = childLists[1];
		drawSplitNode = node;
		Draw_Scene(DrawAll);
	}
#endif

#if defined(DEBUG_SPLITS)
	if((node->compileFlags & C_DETAIL) && isstruct)
		Sys_FPrintf(SYS_ERR, "I am detail, my child is structural, this is a wtf1\n", node->has_structural_children);
#endif

	for(i = 0; i < 2; i++)
	{
		BuildFaceTree_r(node->children[i], childLists[i]);
		node->has_structural_children |= node->children[i]->has_structural_children;
	}

#if defined(DEBUG_SPLITS)
	if((node->compileFlags & C_DETAIL) && !(node->children[0]->compileFlags & C_DETAIL) &&
	   node->children[0]->planenum != PLANENUM_LEAF)
		Sys_FPrintf(SYS_ERR, "I am detail, my child is structural\n", node->has_structural_children);
	if((node->compileFlags & C_DETAIL) && isstruct)
		Sys_FPrintf(SYS_ERR, "I am detail, my child is structural, this is a wtf2\n", node->has_structural_children);
#endif
}


/*
================
FaceBSP

List will be freed before returning
================
*/
tree_t         *FaceBSP(face_t * list, qboolean drawDebug)
{
	tree_t         *tree;
	face_t         *face;
	int             i;
	int             count;

	Sys_FPrintf(SYS_VRB, "--- FaceBSP ---\n");

	tree = AllocTree();

	count = 0;
	for(face = list; face != NULL; face = face->next)
	{
		count++;
		for(i = 0; i < face->w->numpoints; i++)
		{
			AddPointToBounds(face->w->p[i], tree->mins, tree->maxs);
		}
	}
	Sys_FPrintf(SYS_VRB, "%9d faces\n", count);

	for(i = 0; i < nummapplanes; i++)
	{
		mapplanes[i].counter = 0;
	}

	tree->headnode = AllocNode();
	VectorCopy(tree->mins, tree->headnode->mins);
	VectorCopy(tree->maxs, tree->headnode->maxs);
	c_faceLeafs = 0;
	c_faceNodes = 1;

#if 1
	if(drawBSP && drawDebug)
	{
		drawTree = tree;
	}
#endif

	BuildFaceTree_r(tree->headnode, list);

	Sys_FPrintf(SYS_VRB, "%9d nodes\n", c_faceNodes);
	Sys_FPrintf(SYS_VRB, "%9d leafs\n", c_faceLeafs);
	Sys_FPrintf(SYS_VRB, "%9d depth\n", (int)(logf(c_faceNodes) / logf(2)));

	return tree;
}



/*
MakeStructuralBSPFaceList()
get structural brush faces
*/

face_t         *MakeStructuralBSPFaceList(brush_t * list)
{
	brush_t        *b;
	int             i;
	side_t         *s;
	winding_t      *w;
	face_t         *f, *flist;


	flist = NULL;
	for(b = list; b != NULL; b = b->next)
	{
		if(!deepBSP && b->detail)
			continue;

		for(i = 0; i < b->numsides; i++)
		{
			/* get side and winding */
			s = &b->sides[i];
			w = s->winding;
			if(w == NULL)
				continue;

			/* ydnar: skip certain faces */
			if(s->compileFlags & C_SKIP)
				continue;

			/* allocate a face */
			f = AllocBspFace();
			f->w = CopyWinding(w);
			f->planenum = s->planenum & ~1;
			f->compileFlags = s->compileFlags;	/* ydnar */
			if(b->detail)
				f->compileFlags |= C_DETAIL;

			/* ydnar: set priority */
			f->priority = 0;
			if(f->compileFlags & C_HINT)
				f->priority += HINT_PRIORITY;
			if(f->compileFlags & C_ANTIPORTAL)
				f->priority += ANTIPORTAL_PRIORITY;
			if(f->compileFlags & C_AREAPORTAL)
				f->priority += AREAPORTAL_PRIORITY;
			if(f->compileFlags & C_DETAIL)
				f->priority += DETAIL_PRIORITY;

			/* get next face */
			f->next = flist;
			flist = f;
		}
	}

	return flist;
}



/*
MakeVisibleBSPFaceList()
get visible brush faces
*/

face_t         *MakeVisibleBSPFaceList(brush_t * list)
{
	brush_t        *b;
	int             i;
	side_t         *s;
	winding_t      *w;
	face_t         *f, *flist;


	flist = NULL;
	for(b = list; b != NULL; b = b->next)
	{
		if(!deepBSP && b->detail)
			continue;

		for(i = 0; i < b->numsides; i++)
		{
			/* get side and winding */
			s = &b->sides[i];
			w = s->visibleHull;
			if(w == NULL)
				continue;

			/* ydnar: skip certain faces */
			if(s->compileFlags & C_SKIP)
				continue;

			/* allocate a face */
			f = AllocBspFace();
			f->w = CopyWinding(w);
			f->planenum = s->planenum & ~1;
			f->compileFlags = s->compileFlags;	/* ydnar */
			if(b->detail)
				f->compileFlags |= C_DETAIL;

			/* ydnar: set priority */
			f->priority = 0;
			if(f->compileFlags & C_HINT)
				f->priority += HINT_PRIORITY;
			if(f->compileFlags & C_ANTIPORTAL)
				f->priority += ANTIPORTAL_PRIORITY;
			if(f->compileFlags & C_AREAPORTAL)
				f->priority += AREAPORTAL_PRIORITY;
			if(f->compileFlags & C_DETAIL)
				f->priority += DETAIL_PRIORITY;

			/* get next face */
			f->next = flist;
			flist = f;
		}
	}

	return flist;
}
