/*
===========================================================================
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* lighttile_vp.glsl */

IN vec2 attr_Position;
IN vec2 attr_TexCoord0;

OUT(smooth) vec2 vPosition;
OUT(smooth) vec2 vTexCoord;

void main() {
  gl_Position = vec4(attr_Position, 0.0, 1.0);
  gl_PointSize = 1.0;

  vPosition = attr_Position;
  vTexCoord = attr_TexCoord0;
}
