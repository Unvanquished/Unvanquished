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

#include "Color.h"

#include <algorithm>

namespace Color {

namespace Named {
Color   Black   = {   0,   0,   0, 255 };
Color   Red     = { 255,   0,   0, 255 };
Color   Green   = {   0, 255,   0, 255 };
Color   Blue    = {   0,   0, 255, 255 };
Color   Yellow  = { 255, 255,   0, 255 };
Color   Orange  = { 255, 128,   0, 255 };
Color   Magenta = { 255,   0, 255, 255 };
Color   Cyan    = {   0, 255, 255, 255 };
Color   White   = { 255, 255, 255, 255 };
Color   LtGrey  = { 191, 191, 191, 255 };
Color   MdGrey  = { 128, 128, 128, 255 };
Color   DkGrey  = {  64,  64,  64, 255 };
Color   MdRed   = { 128,   0,   0, 255 };
Color   MdGreen = {   0, 128,   0, 255 };
Color   DkGreen = {   0,  51,   0, 255 };
Color   MdCyan  = {   0, 128, 128, 255 };
Color   MdYellow= { 128, 128,   0, 255 };
Color   MdOrange= { 128,  64,   0, 255 };
Color   MdBlue  = {   0,   0, 128, 255 };
} // namespace Named


namespace NamedString {
const char* Black = "^0";
const char* Red = "^1";
const char* Green = "^2";
const char* Blue = "^4";
const char* Yellow = "^3";
const char* Orange = "^8";
const char* Magenta = "^6";
const char* Cyan = "^5";
const char* White = "^7";
const char* LtGrey = "^:";
const char* MdGrey = "^9";
const char* MdRed = "^?";
const char* MdGreen = "^<";
const char* MdCyan = "^B";
const char* MdYellow = "^=";
const char* LtOrange = "^A";
const char* MdBlue = "^>";
const char* Null = "^*";
} // namespace Named

namespace NamedFloat {
vec4_t Black = { 0, 0, 0, 1 };
vec4_t Red = { 1, 0, 0, 1 };
vec4_t Green = { 0, 1, 0, 1 };
vec4_t Blue = { 0, 0, 1, 1 };
vec4_t Yellow = { 1, 1, 0, 1 };
vec4_t Orange = { 1, 0.5, 0, 1 };
vec4_t Magenta = { 1, 0, 1, 1 };
vec4_t Cyan = { 0, 1, 1, 1 };
vec4_t White = { 1, 1, 1, 1 };
vec4_t LtGrey = { 0.75, 0.75, 0.75, 1 };
vec4_t MdGrey = { 0.5, 0.5, 0.5, 1 };
vec4_t DkGrey = { 0.25, 0.25, 0.25, 1 };
vec4_t MdRed = { 0.5, 0, 0, 1 };
vec4_t MdGreen = { 0, 0.5, 0, 1 };
vec4_t DkGreen = { 0, 0.20, 0, 1 };
vec4_t MdCyan = { 0, 0.5, 0.5, 1 };
vec4_t MdYellow = { 0.5, 0.5, 0, 1 };
vec4_t MdOrange = { 0.5, 0.25, 0, 1 };
vec4_t MdBlue = { 0, 0, 0.5, 1 };
} // namespace NamedFloat

static Color g_color_table[ 32 ] =
{
	{  51,  51,  51, 255 }, // 0 - black       0
	{ 255,   0,   0, 255 }, // 1 - red         1
	{   0, 255,   0, 255 }, // 2 - green       2
	{ 255, 255,   0, 255 }, // 3 - yellow      3
	{   0,   0, 255, 255 }, // 4 - blue        4
	{   0, 255, 255, 255 }, // 5 - cyan        5
	{ 255,   0, 255, 255 }, // 6 - purple      6
	{ 255, 255, 255, 255 }, // 7 - white       7
	{ 255, 128,   0, 255 }, // 8 - orange      8
	{ 128, 128, 128, 255 }, // 9 - md.grey     9
	{ 191, 191, 191, 255 }, // : - lt.grey    10    // lt grey for names
	{ 191, 191, 191, 255 }, // ; - lt.grey    11
	{   0, 128,   0, 255 }, // < - md.green   12
	{ 128, 128,   0, 255 }, // = - md.yellow  13
	{   0,   0, 128, 255 }, // > - md.blue    14
	{ 128,   0,   0, 255 }, // ? - md.red     15
	{ 128,  64,   0, 255 }, // @ - md.orange  16
	{ 255, 153,  26, 255 }, // A - lt.orange  17
	{   0, 128, 128, 255 }, // B - md.cyan    18
	{ 128,   0, 128, 255 }, // C - md.purple  19
	{   0, 128, 255, 255 }, // D              20
	{ 128,   0, 255, 255 }, // E              21
	{  51, 153, 204, 255 }, // F              22
	{ 204, 255, 204, 255 }, // G              23
	{   0, 102,  51, 255 }, // H              24
	{ 255,   0,  51, 255 }, // I              25
	{ 179,  26,  26, 255 }, // J              26
	{ 153,  51,   0, 255 }, // K              27
	{ 204, 153,  51, 255 }, // L              28
	{ 153, 153,  51, 255 }, // M              29
	{ 255, 255, 191, 255 }, // N              30
	{ 255, 255, 128, 255 }, // O              31
};

Color::Color::Color (int index)
{
	if ( index < 0 )
		index *= -1;
	*this = g_color_table[index%32];
}

int Color::to_4bit() const
{
    int color = 0;

    component_type cmax = std::max({r,g,b});
    component_type cmin = std::min({r,g,b});
    component_type delta = cmax-cmin;

    if ( delta > 0 )
    {
        float hue = 0;
        if ( r == cmax )
            hue = float(g-b)/delta;
        else if ( g == cmax )
            hue = float(b-r)/delta + 2;
        else if ( b == cmax )
            hue = float(r-g)/delta + 4;

        float sat = float(delta)/cmax;
        if ( sat >= 0.3 )
        {
            if ( hue < 0 )
                hue += 6;

            if ( hue <= 0.5 )      color = 0b001; // red
            else if ( hue <= 1.5 ) color = 0b011; // yellow
            else if ( hue <= 2.5 ) color = 0b010; // green
            else if ( hue <= 3.5 ) color = 0b110; // cyan
            else if ( hue <= 4.5 ) color = 0b100; // blue
            else if ( hue <= 5.5 ) color = 0b101; // magenta
            else                   color = 0b001; // red
        }
        else if ( cmax >= 120 )
            color = 7;

        if ( ( cmax + cmin ) / 2 >= 164 )
            color |= 0b1000; // bright
    }
    else
    {
        if ( cmax > 204 )
            color = 0b1111; // white
        else if ( cmax > 136 )
            color = 0b0111; // silver
        else if ( cmax > 68 )
            color = 0b1000; // gray
        else
            color = 0b0000; // black
    }

    return color;
}

} // namespace Color
