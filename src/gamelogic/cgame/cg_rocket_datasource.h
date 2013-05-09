/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include "cg_local.h"

#define MAX_SERVERS 2048
#define MAX_RESOLUTIONS 32
#define MAX_LANGUAGES 64
#define MAX_INPUTS 16
#define MAX_OUTPUTS 16
#define MAX_MODS 64
#define MAX_DEMOS 256


typedef struct server_s
{
	char *name;
	char *label;
	int clients;
	int bots;
	int ping;
	int maxClients;
	char *addr;
} server_t;

server_t servers[ MAX_SERVERS ];
int serverCount = 0;
int serverIndex;

typedef struct resolution_s
{
	int width;
	int height;
} resolution_t;

resolution_t resolutions[ MAX_RESOLUTIONS ];
int resolutionCount = 0;
int resolutionIndex;

typedef struct language_s
{
	char *name;
	char *lang;
} language_t;

language_t languages[ MAX_LANGUAGES ];
int languageCount = 0;
int languageIndex;

typedef struct modInfo_s
{
	char *name;
	char *description;
} modInfo_t;

modInfo_t modList[ MAX_MODS ];
int modCount;
int modIndex;

char *voipInputs[ MAX_INPUTS ];
int voipInputsCount = 0;
int voipInputIndex;

char *alOutputs[ MAX_OUTPUTS ];
int alOutputsCount = 0;
int alOutputIndex;

char *demoList[ MAX_DEMOS ];
int demoCount = 0;
int demoIndex;

