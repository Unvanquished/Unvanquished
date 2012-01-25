/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - GMC interpolation module header -
 *
 *  Copyright(C) 2002-2003 Pascal Massimino <skal@planet-d.net>
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
 * $Id: gmc.h,v 1.3 2006/06/14 21:44:07 Skal Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "../global.h"

/* *************************************************************
 * will initialize internal pointers
 */

void init_GMC(const unsigned int cpu_flags);

/* *************************************************************
 * Warning! It's Accuracy being passed, not 'resolution'!
 */

void generate_GMCparameters(int nb_pts, const int accuracy,
							const WARPPOINTS *const pts,
							const int width, const int height,
							NEW_GMC_DATA *const gmc);

/* *******************************************************************
 */

void
generate_GMCimage(	const NEW_GMC_DATA *const gmc_data, /* [input] precalculated data */
					const IMAGE *const pRef,		/* [input] */
					const int mb_width,
					const int mb_height,
					const int stride,
					const int stride2,
					const int fcode, 				/* [input] some parameters... */
  					const int32_t quarterpel,		/* [input] for rounding avgMV */
					const int reduced_resolution,	/* [input] ignored */
					const int32_t rounding,			/* [input] for rounding image data */
					MACROBLOCK *const pMBs, 		/* [output] average motion vectors */
					IMAGE *const pGMC);				/* [output] full warped image */

/* *************************************************************
 * utils
 */

/* This is borrowed from	decoder.c   */
static __inline int
gmc_sanitize(int value, int quarterpel, int fcode)
{
	int length = 1 << (fcode+4);

#if 0
	if (quarterpel) value *= 2;
#endif

	if (value < -length)
		return -length;
	else if (value >= length)
		return length-1;
	else return value;
}

/* *************************************************************/
