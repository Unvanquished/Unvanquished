/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Daemon GPL Source Code (Daemon Source Code).  

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the Daemon 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

#ifndef _DL_LOCAL_H_
#define _DL_LOCAL_H_

//bani
#if defined __GNUC__ || defined __clang__
#define _attribute( x ) __attribute__( x )
#else
#define _attribute( x )
#endif

// system API
// only the restricted subset we need

int Com_VPrintf(const char *fmt, va_list pArgs) _attribute((format(printf, 1, 0)));
	 int Com_DPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
	 int Com_Printf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
	 void Com_Error(int code, const char *fmt, ...) _attribute((format(printf, 2, 3)));	// watch out, we don't define ERR_FATAL and stuff
	 void Cvar_SetValue(const char *var_name, float value);
	 void Cvar_Set(const char *var_name, const char *value);
	 char           *va(char *format, ...) _attribute((format(printf, 1, 2)));

#ifdef WIN32
#define Q_stricmp stricmp
#else
#define Q_stricmp strcasecmp
#endif

	 extern int com_errorEntered;

#endif
