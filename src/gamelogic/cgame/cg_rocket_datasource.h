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

typedef struct node_s {
	struct node_s *next;
} node_t;

typedef struct server_s
{
	struct server_s *next;

	char *name;
	int clients;
	int bots;
	int ping;
} server_t;

server_t *serverListHead;
server_t *serverListTail;
int serverCount = 0;

typedef struct resolution_s
{
	struct resolution_s *next;

	int width;
	int height;
} resolution_t;

resolution_t *resolutionsListHead;
resolution_t *resolutionsListTail;
int resolutionCount = 0;

typedef struct language_s
{
	struct language_s *next;

	char *name;
	char *lang;
} language_t;

language_t *languageListHead;
language_t *languageListTail;
int languageCount = 0;

typedef struct charList_s
{
	struct charList_s *next;

	char *name;
} charList_t;

charList_t *voipInputsListHead;
charList_t *voipInputsListTail;
int voipInputsCount = 0;

charList_t *alOutputsListHead;
charList_t *alOutputsListTail;
int alOutputsCount = 0;
