/*
===========================================================================

Daemon GPL Source Code

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

#include "color.h"

vec4_t   colorBlack = { 0, 0, 0, 1 };
vec4_t   colorRed = { 1, 0, 0, 1 };
vec4_t   colorGreen = { 0, 1, 0, 1 };
vec4_t   colorBlue = { 0, 0, 1, 1 };
vec4_t   colorYellow = { 1, 1, 0, 1 };
vec4_t   colorOrange = { 1, 0.5, 0, 1 };
vec4_t   colorMagenta = { 1, 0, 1, 1 };
vec4_t   colorCyan = { 0, 1, 1, 1 };
vec4_t   colorWhite = { 1, 1, 1, 1 };
vec4_t   colorLtGrey = { 0.75, 0.75, 0.75, 1 };
vec4_t   colorMdGrey = { 0.5, 0.5, 0.5, 1 };
vec4_t   colorDkGrey = { 0.25, 0.25, 0.25, 1 };
vec4_t   colorMdRed = { 0.5, 0, 0, 1 };
vec4_t   colorMdGreen = { 0, 0.5, 0, 1 };
vec4_t   colorDkGreen = { 0, 0.20, 0, 1 };
vec4_t   colorMdCyan = { 0, 0.5, 0.5, 1 };
vec4_t   colorMdYellow = { 0.5, 0.5, 0, 1 };
vec4_t   colorMdOrange = { 0.5, 0.25, 0, 1 };
vec4_t   colorMdBlue = { 0, 0, 0.5, 1 };

vec4_t   g_color_table[ 32 ] =
{
	{ 0.2,  0.2,   0.2,     1.0    }, // 0 - black    0
	{ 1.0,  0.0,   0.0,     1.0    }, // 1 - red      1
	{ 0.0,  1.0,   0.0,     1.0    }, // 2 - green    2
	{ 1.0,  1.0,   0.0,     1.0    }, // 3 - yellow   3
	{ 0.0,  0.0,   1.0,     1.0    }, // 4 - blue     4
	{ 0.0,  1.0,   1.0,     1.0    }, // 5 - cyan     5
	{ 1.0,  0.0,   1.0,     1.0    }, // 6 - purple   6
	{ 1.0,  1.0,   1.0,     1.0    }, // 7 - white    7
	{ 1.0,  0.5,   0.0,     1.0    }, // 8 - orange   8
	{ 0.5,  0.5,   0.5,     1.0    }, // 9 - md.grey    9
	{ 0.75, 0.75,  0.75,    1.0    }, // : - lt.grey    10    // lt grey for names
	{ 0.75, 0.75,  0.75,    1.0    }, // ; - lt.grey    11
	{ 0.0,  0.5,   0.0,     1.0    }, // < - md.green   12
	{ 0.5,  0.5,   0.0,     1.0    }, // = - md.yellow  13
	{ 0.0,  0.0,   0.5,     1.0    }, // > - md.blue    14
	{ 0.5,  0.0,   0.0,     1.0    }, // ? - md.red   15
	{ 0.5,  0.25,  0.0,     1.0    }, // @ - md.orange  16
	{ 1.0,  0.6f,  0.1f,    1.0    }, // A - lt.orange  17
	{ 0.0,  0.5,   0.5,     1.0    }, // B - md.cyan    18
	{ 0.5,  0.0,   0.5,     1.0    }, // C - md.purple  19
	{ 0.0,  0.5,   1.0,     1.0    }, // D        20
	{ 0.5,  0.0,   1.0,     1.0    }, // E        21
	{ 0.2f, 0.6f,  0.8f,    1.0    }, // F        22
	{ 0.8f, 1.0,   0.8f,    1.0    }, // G        23
	{ 0.0,  0.4,   0.2f,    1.0    }, // H        24
	{ 1.0,  0.0,   0.2f,    1.0    }, // I        25
	{ 0.7f, 0.1f,  0.1f,    1.0    }, // J        26
	{ 0.6f, 0.2f,  0.0,     1.0    }, // K        27
	{ 0.8f, 0.6f,  0.2f,    1.0    }, // L        28
	{ 0.6f, 0.6f,  0.2f,    1.0    }, // M        29
	{ 1.0,  1.0,   0.75,    1.0    }, // N        30
	{ 1.0,  1.0,   0.5,     1.0    }, // O        31
};
