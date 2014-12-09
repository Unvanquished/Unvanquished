/*
===========================================================================
Copyright (C) 2005 Eugene Bujak.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_mme.c -- Movie Maker's Edition client-side work

#include "../client.h"

extern cvar_t *cl_avidemo;
extern cvar_t *cl_forceavidemo;

// MME cvars
cvar_t	*mme_anykeystopsdemo;
cvar_t	*mme_saveWav;
cvar_t	*mme_gameOverride;
cvar_t	*mme_demoConvert;
cvar_t	*mme_demoSmoothen;
cvar_t	*mme_demoFileName;
cvar_t	*mme_demoStartProject;
cvar_t	*mme_demoListQuit;

void CL_MME_CheckCvarChanges(void) {
	
	if (cl_avidemo->modified) {
		cl_avidemo->modified = qfalse;
		clc.aviDemoRemain = 0;
	}
	if ( cls.state == CA_DISCONNECTED)
		CL_DemoListNext_f();
}

