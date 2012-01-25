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

#ifndef __INOUT__
#define __INOUT__

#include "cmdlib.h"
#include "mathlib.h"

#if defined(USE_XML)
// inout is the only stuff relying on xml, include the headers there
#include "libxml/tree.h"

// some useful xml routines
xmlNodePtr      xml_NodeForVec(vec3_t v);
void            xml_SendNode(xmlNodePtr node);

// print a message in xmap output and send the corresponding select information down the xml stream
// bError: do we end with an error on this one or do we go ahead?
void            xml_Select(char *msg, int entitynum, int brushnum, qboolean bError);

// end xmap with an error message and send a point information in the xml stream
// note: we might want to add a boolean to use this as a warning or an error thing..
void            xml_Winding(char *msg, vec3_t p[], int numpoints, qboolean die);
void            xml_Point(char *msg, vec3_t pt);

#ifdef _DEBUG
#define DBG_XML 1
#endif

#ifdef DBG_XML
void            DumpXML();
#endif

#endif // USE_XML

void            Broadcast_Setup(const char *dest);
void            Broadcast_Shutdown();

#define SYS_VRB 0				// verbose support (on/off)
#define SYS_STD 1				// standard print level
#define SYS_WRN 2				// warnings
#define SYS_ERR 3				// error
#define SYS_NOXML 4				// don't send that down the XML stream

extern qboolean verbose;
void            Sys_Printf(const char *text, ...);
void            Sys_FPrintf(int flag, const char *text, ...);



#endif
