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

#include "con_common.h"

namespace Color {

/*
 * Returns a 4 bit integer with the bits following this pattern:
 *  1 red
 *  2 green
 *  4 blue
 *  8 bright
 */
int To4bit(const Color& c) NOEXCEPT
{
    int color = 0;

    float cmax = std::max({c.Red(),c.Green(),c.Blue()});
    float cmin = std::min({c.Red(),c.Green(),c.Blue()});
    float delta = cmax-cmin;

    if ( delta > 0 )
    {
        float hue = 0;
        if ( c.Red() == cmax )
            hue = (c.Green()-c.Blue())/delta;
        else if ( c.Green() == cmax )
            hue = (c.Blue()-c.Red())/delta + 2;
        else if ( c.Blue() == cmax )
            hue = (c.Red()-c.Green())/delta + 4;

        float sat = delta / cmax;
        if ( sat >= 0.3 )
        {
            if ( hue < 0 )
                hue += 6;

            if ( hue <= 0.5 )      color = 1; // red
            else if ( hue <= 1.5 ) color = 3; // yellow
            else if ( hue <= 2.5 ) color = 2; // green
            else if ( hue <= 3.5 ) color = 6; // cyan
            else if ( hue <= 4.5 ) color = 4; // blue
            else if ( hue <= 5.5 ) color = 5; // magenta
            else                   color = 1; // red
        }
        else if ( cmax >= 0.5 )
            color = 7;

        if ( ( cmax + cmin ) / 2 >= 0.64 )
            color |= 8; // bright
    }
    else
    {
        if ( cmax > 0.8 )
            color = 15; // white
        else if ( cmax > 0.53 )
            color = 7; // silver
        else if ( cmax > 0.27 )
            color = 8; // gray
        else
            color = 0; // black
    }

    return color;
}

} // namespace Color
