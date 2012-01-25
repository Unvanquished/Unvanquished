/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - ZigZag order matrices  -
 *
 *  Copyright(C) 2001-2002 Michael Militzer <isibaar@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: zigzag.h,v 1.7 2004/03/22 22:36:23 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _ZIGZAG_H_
#define _ZIGZAG_H_

static const uint16_t scan_tables[3][64] = {
	/* zig_zag_scan */
	{ 0,  1,  8, 16,  9,  2,  3, 10,
	 17, 24, 32, 25, 18, 11,  4,  5,
	 12, 19, 26, 33, 40, 48, 41, 34,
	 27, 20, 13,  6,  7, 14, 21, 28,
	 35, 42, 49, 56, 57, 50, 43, 36,
	 29, 22, 15, 23, 30, 37, 44, 51,
	 58, 59, 52, 45, 38, 31, 39, 46,
	 53, 60, 61, 54, 47, 55, 62, 63},

	/* horizontal_scan */
	{ 0,  1,  2,  3,  8,  9, 16, 17,
	 10, 11,  4,  5,  6,  7, 15, 14,
	 13, 12, 19, 18, 24, 25, 32, 33,
	 26, 27, 20, 21, 22, 23, 28, 29,
	 30, 31, 34, 35, 40, 41, 48, 49,
	 42, 43, 36, 37, 38, 39, 44, 45,
	 46, 47, 50, 51, 56, 57, 58, 59,
	 52, 53, 54, 55, 60, 61, 62, 63},

	/* vertical_scan */
	{ 0,  8, 16, 24,  1,  9,  2, 10,
	 17, 25, 32, 40, 48, 56, 57, 49,
	 41, 33, 26, 18,  3, 11,  4, 12,
	 19, 27, 34, 42, 50, 58, 35, 43,
	 51, 59, 20, 28,  5, 13,  6, 14,
	 21, 29, 36, 44, 52, 60, 37, 45,
	 53, 61, 22, 30,  7, 15, 23, 31,
	 38, 46, 54, 62, 39, 47, 55, 63}
};

#endif							/* _ZIGZAG_H_ */
