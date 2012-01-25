/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Motion Estimation shared functions -
 *
 *  Copyright(C) 2002 Christoph Lampert <gruel@web.de>
 *               2002 Michael Militzer <michael@xvid.org>
 *               2002-2003 Radoslaw Czyz <xvid@syskin.cjb.net>
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
 * $Id: motion_inlines.h,v 1.5 2007/04/28 16:30:20 syskin Exp $
 *
 ****************************************************************************/

#ifndef _MOTION_INLINES_
#define _MOTION_INLINES_

#include <stdlib.h>

/*
 * Calculate the min/max range
 * relative to the _MACROBLOCK_ position
 */
static void __inline
get_range(int32_t * const min_dx,
		  int32_t * const max_dx,
		  int32_t * const min_dy,
		  int32_t * const max_dy,
		  const uint32_t x,
		  const uint32_t y,
		  uint32_t block_sz, /* block dimension, 3(8) or 4(16) */
		  const uint32_t width,
		  const uint32_t height,
		  const int fcode,
		  const int precision) /* 2 for qpel, 1 for halfpel */
{
	int k;
	const int search_range = 1 << (4+fcode);
	int high = search_range - 1;
	int low = -search_range;

	k = (int)(width - (x<<block_sz))<<precision;
	*max_dx = MIN(high, k);
	k = (int)(height -  (y<<block_sz))<<precision;
	*max_dy = MIN(high, k);

	k = (-(int)((x+1)<<block_sz))<<precision;
	*min_dx = MAX(low, k);
	k = (-(int)((y+1)<<block_sz))<<precision;
	*min_dy = MAX(low, k);
}

/* reversed mv.length table */
static const int r_mvtab[65] = {
	11, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12,
	12, 11, 11, 11, 11, 11, 11, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 9, 9, 9,
	7, 7, 7, 6, 4, 3, 2, 1
};

static __inline uint32_t
d_mv_bits(int x, int y, const VECTOR pred, const uint32_t iFcode, const int qpel)
{
	unsigned int bits;

	x <<= qpel;
	y <<= qpel;

	x -= pred.x;
	bits = (x != 0 ? iFcode:0);
	x = -abs(x);
	x >>= (iFcode - 1);
	bits += r_mvtab[x+64];

	y -= pred.y;
	bits += (y != 0 ? iFcode:0);
	y = -abs(y);
	y >>= (iFcode - 1);
	bits += r_mvtab[y+64];

	return bits;
}

static __inline const uint8_t *
GetReference(const int x, const int y, const SearchData * const data)
{
	const int picture = ((x&1)<<1) | (y&1);
	const int offset = (x>>1) + (y>>1)*data->iEdgedWidth;
	return data->RefP[picture] + offset;
}

static __inline const uint8_t *
GetReferenceB(const int x, const int y, const uint32_t dir, const SearchData * const data)
{
	/* dir : 0 = forward, 1 = backward */
	const uint8_t *const *const direction = ( dir == 0 ? data->RefP : data->b_RefP );
	const int picture = ((x&1)<<1) | (y&1);
	const int offset = (x>>1) + (y>>1)*data->iEdgedWidth;
	return direction[picture] + offset;
}

static __inline void
ZeroMacroblockP(MACROBLOCK *pMB, const int32_t sad)
{
	pMB->mode = MODE_INTER;
	pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] = pMB->mvs[3] = zeroMV;
	pMB->qmvs[0] = pMB->qmvs[1] = pMB->qmvs[2] = pMB->qmvs[3] = zeroMV;
	pMB->sad16 = pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] = sad;
	pMB->mcsel = 0;
	pMB->cbp = 0;
}

/* check if given vector is equal to any vector checked before */
static __inline int
vector_repeats(const VECTOR * const pmv, const unsigned int i)
{
	unsigned int j;
	for (j = 0; j < i; j++)
		if (MVequal(pmv[i], pmv[j])) return 1; /* same vector has been checked already */
	return 0;
}

/*	make a binary mask that prevents diamonds/squares
	from checking a vector which has been checked as a prediction */
static __inline int
make_mask(const VECTOR * const pmv, const unsigned int i, const unsigned int current)
{
	unsigned int mask = 255, j;
	for (j = 0; j < i; j++) {
		if (pmv[current].x == pmv[j].x) {
			if (pmv[current].y == pmv[j].y + iDiamondSize) mask &= ~4;
			else if (pmv[current].y == pmv[j].y - iDiamondSize) mask &= ~8;
		} else
			if (pmv[current].y == pmv[j].y) {
				if (pmv[current].x == pmv[j].x + iDiamondSize) mask &= ~1;
				else if (pmv[current].x == pmv[j].x - iDiamondSize) mask &= ~2;
			}
	}
	return mask;
}

#endif /* _MOTION_INLINES_ */
