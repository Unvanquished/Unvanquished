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
 * $Id: estimation_common.c,v 1.13 2005/12/09 04:39:49 syskin Exp $
 *
 ****************************************************************************/

#include "../encoder.h"
#include "../global.h"
#include "../image/interpolate8x8.h"
#include "estimation.h"
#include "motion.h"
#include "sad.h"
#include "motion_inlines.h"


/*****************************************************************************
 * Modified rounding tables
 * Original tables see ISO spec tables 7-6 -> 7-9
 ****************************************************************************/

const uint32_t roundtab[16] =
{0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };

/* K = 4 */
const uint32_t roundtab_76[16] =
{ 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 };

/* K = 2 */
const uint32_t roundtab_78[8] =
{ 0, 0, 1, 1, 0, 0, 0, 1  };

/* K = 1 */
const uint32_t roundtab_79[4] =
{ 0, 1, 0, 0 };

const int xvid_me_lambda_vec16[32] =
	{     0    ,(int)(1.0 * NEIGH_TEND_16X16 + 0.5),
	(int)(2.0*NEIGH_TEND_16X16 + 0.5), (int)(3.0*NEIGH_TEND_16X16 + 0.5),
	(int)(4.0*NEIGH_TEND_16X16 + 0.5), (int)(5.0*NEIGH_TEND_16X16 + 0.5),
	(int)(6.0*NEIGH_TEND_16X16 + 0.5), (int)(7.0*NEIGH_TEND_16X16 + 0.5),
	(int)(8.0*NEIGH_TEND_16X16 + 0.5), (int)(9.0*NEIGH_TEND_16X16 + 0.5),
	(int)(10.0*NEIGH_TEND_16X16 + 0.5), (int)(11.0*NEIGH_TEND_16X16 + 0.5),
	(int)(12.0*NEIGH_TEND_16X16 + 0.5), (int)(13.0*NEIGH_TEND_16X16 + 0.5),
	(int)(14.0*NEIGH_TEND_16X16 + 0.5), (int)(15.0*NEIGH_TEND_16X16 + 0.5),
	(int)(16.0*NEIGH_TEND_16X16 + 0.5), (int)(17.0*NEIGH_TEND_16X16 + 0.5),
	(int)(18.0*NEIGH_TEND_16X16 + 0.5), (int)(19.0*NEIGH_TEND_16X16 + 0.5),
	(int)(20.0*NEIGH_TEND_16X16 + 0.5), (int)(21.0*NEIGH_TEND_16X16 + 0.5),
	(int)(22.0*NEIGH_TEND_16X16 + 0.5), (int)(23.0*NEIGH_TEND_16X16 + 0.5),
	(int)(24.0*NEIGH_TEND_16X16 + 0.5), (int)(25.0*NEIGH_TEND_16X16 + 0.5),
	(int)(26.0*NEIGH_TEND_16X16 + 0.5), (int)(27.0*NEIGH_TEND_16X16 + 0.5),
	(int)(28.0*NEIGH_TEND_16X16 + 0.5), (int)(29.0*NEIGH_TEND_16X16 + 0.5),
	(int)(30.0*NEIGH_TEND_16X16 + 0.5), (int)(31.0*NEIGH_TEND_16X16 + 0.5)
};

/*****************************************************************************
 * Code
 ****************************************************************************/

int32_t
xvid_me_ChromaSAD(const int dx, const int dy, SearchData * const data)
{
	int sad;
	const uint32_t stride = data->iEdgedWidth/2;
	int offset = (dx>>1) + (dy>>1)*stride;
	int next = 1;

	if (dx == data->chromaX && dy == data->chromaY) 
		return data->chromaSAD; /* it has been checked recently */
	data->chromaX = dx; data->chromaY = dy; /* backup */

	switch (((dx & 1) << 1) | (dy & 1))	{
		case 0:
			sad = sad8(data->CurU, data->RefP[4] + offset, stride);
			sad += sad8(data->CurV, data->RefP[5] + offset, stride);
			break;
		case 1:
			next = stride;
		case 2:
			sad = sad8bi(data->CurU, data->RefP[4] + offset, data->RefP[4] + offset + next, stride);
			sad += sad8bi(data->CurV, data->RefP[5] + offset, data->RefP[5] + offset + next, stride);
			break;
		default:
			interpolate8x8_halfpel_hv(data->RefQ, data->RefP[4] + offset, stride, data->rounding);
			sad = sad8(data->CurU, data->RefQ, stride);

			interpolate8x8_halfpel_hv(data->RefQ, data->RefP[5] + offset, stride, data->rounding);
			sad += sad8(data->CurV, data->RefQ, stride);
			break;
	}
	data->chromaSAD = sad; /* backup, part 2 */
	return sad;
}

uint8_t *
xvid_me_interpolate8x8qpel(const int x, const int y, const uint32_t block, const uint32_t dir, const SearchData * const data)
{
	/* create or find a qpel-precision reference picture; return pointer to it */
	uint8_t * Reference = data->RefQ + 16*dir;
	const uint32_t iEdgedWidth = data->iEdgedWidth;
	const uint32_t rounding = data->rounding;
	const int halfpel_x = x/2;
	const int halfpel_y = y/2;
	const uint8_t *ref1, *ref2, *ref3, *ref4;

	ref1 = GetReferenceB(halfpel_x, halfpel_y, dir, data);
	ref1 += 8 * (block&1) + 8 * (block>>1) * iEdgedWidth;
	switch( ((x&1)<<1) + (y&1) ) {
	case 3: /* x and y in qpel resolution - the "corners" (top left/right and */
			/* bottom left/right) during qpel refinement */
		ref2 = GetReferenceB(halfpel_x, y - halfpel_y, dir, data);
		ref3 = GetReferenceB(x - halfpel_x, halfpel_y, dir, data);
		ref4 = GetReferenceB(x - halfpel_x, y - halfpel_y, dir, data);
		ref2 += 8 * (block&1) + 8 * (block>>1) * iEdgedWidth;
		ref3 += 8 * (block&1) + 8 * (block>>1) * iEdgedWidth;
		ref4 += 8 * (block&1) + 8 * (block>>1) * iEdgedWidth;
		interpolate8x8_avg4(Reference, ref1, ref2, ref3, ref4, iEdgedWidth, rounding);
		break;

	case 1: /* x halfpel, y qpel - top or bottom during qpel refinement */
		ref2 = GetReferenceB(halfpel_x, y - halfpel_y, dir, data);
		ref2 += 8 * (block&1) + 8 * (block>>1) * iEdgedWidth;
		interpolate8x8_avg2(Reference, ref1, ref2, iEdgedWidth, rounding, 8);
		break;

	case 2: /* x qpel, y halfpel - left or right during qpel refinement */
		ref2 = GetReferenceB(x - halfpel_x, halfpel_y, dir, data);
		ref2 += 8 * (block&1) + 8 * (block>>1) * iEdgedWidth;
		interpolate8x8_avg2(Reference, ref1, ref2, iEdgedWidth, rounding, 8);
		break;

	default: /* pure halfpel position */
		return (uint8_t *) ref1;

	}
	return Reference;
}

uint8_t *
xvid_me_interpolate16x16qpel(const int x, const int y, const uint32_t dir, const SearchData * const data)
{
	/* create or find a qpel-precision reference picture; return pointer to it */
	uint8_t * Reference = data->RefQ + 16*dir;
	const uint32_t iEdgedWidth = data->iEdgedWidth;
	const uint32_t rounding = data->rounding;
	const int halfpel_x = x/2;
	const int halfpel_y = y/2;
	const uint8_t *ref1, *ref2, *ref3, *ref4;

	ref1 = GetReferenceB(halfpel_x, halfpel_y, dir, data);
	switch( ((x&1)<<1) + (y&1) ) {
	case 3:
		/*
		 * x and y in qpel resolution - the "corners" (top left/right and
		 * bottom left/right) during qpel refinement
		 */
		ref2 = GetReferenceB(halfpel_x, y - halfpel_y, dir, data);
		ref3 = GetReferenceB(x - halfpel_x, halfpel_y, dir, data);
		ref4 = GetReferenceB(x - halfpel_x, y - halfpel_y, dir, data);
		interpolate8x8_avg4(Reference, ref1, ref2, ref3, ref4, iEdgedWidth, rounding);
		interpolate8x8_avg4(Reference+8, ref1+8, ref2+8, ref3+8, ref4+8, iEdgedWidth, rounding);
		interpolate8x8_avg4(Reference+8*iEdgedWidth, ref1+8*iEdgedWidth, ref2+8*iEdgedWidth, ref3+8*iEdgedWidth, ref4+8*iEdgedWidth, iEdgedWidth, rounding);
		interpolate8x8_avg4(Reference+8*iEdgedWidth+8, ref1+8*iEdgedWidth+8, ref2+8*iEdgedWidth+8, ref3+8*iEdgedWidth+8, ref4+8*iEdgedWidth+8, iEdgedWidth, rounding);
		break;

	case 1: /* x halfpel, y qpel - top or bottom during qpel refinement */
		ref2 = GetReferenceB(halfpel_x, y - halfpel_y, dir, data);
		interpolate8x8_avg2(Reference, ref1, ref2, iEdgedWidth, rounding, 8);
		interpolate8x8_avg2(Reference+8, ref1+8, ref2+8, iEdgedWidth, rounding, 8);
		interpolate8x8_avg2(Reference+8*iEdgedWidth, ref1+8*iEdgedWidth, ref2+8*iEdgedWidth, iEdgedWidth, rounding, 8);
		interpolate8x8_avg2(Reference+8*iEdgedWidth+8, ref1+8*iEdgedWidth+8, ref2+8*iEdgedWidth+8, iEdgedWidth, rounding, 8);
		break;

	case 2: /* x qpel, y halfpel - left or right during qpel refinement */
		ref2 = GetReferenceB(x - halfpel_x, halfpel_y, dir, data);
		interpolate8x8_avg2(Reference, ref1, ref2, iEdgedWidth, rounding, 8);
		interpolate8x8_avg2(Reference+8, ref1+8, ref2+8, iEdgedWidth, rounding, 8);
		interpolate8x8_avg2(Reference+8*iEdgedWidth, ref1+8*iEdgedWidth, ref2+8*iEdgedWidth, iEdgedWidth, rounding, 8);
		interpolate8x8_avg2(Reference+8*iEdgedWidth+8, ref1+8*iEdgedWidth+8, ref2+8*iEdgedWidth+8, iEdgedWidth, rounding, 8);
		break;


	default: /* pure halfpel position */
		return (uint8_t *) ref1;
	}
	return Reference;
}

void
xvid_me_AdvDiamondSearch(int x, int y, SearchData * const data,
						 int bDirection, CheckFunc * const CheckCandidate)
{

/* directions: 1 - left (x-1); 2 - right (x+1), 4 - up (y-1); 8 - down (y+1) */

	unsigned int * const iDirection = &data->dir;

	for(;;) { /* forever */
		*iDirection = 0;
		if (bDirection & 1) CHECK_CANDIDATE(x - iDiamondSize, y, 1);
		if (bDirection & 2) CHECK_CANDIDATE(x + iDiamondSize, y, 2);
		if (bDirection & 4) CHECK_CANDIDATE(x, y - iDiamondSize, 4);
		if (bDirection & 8) CHECK_CANDIDATE(x, y + iDiamondSize, 8);

		/* now we're doing diagonal checks near our candidate */

		if (*iDirection) {		/* if anything found */
			bDirection = *iDirection;
			*iDirection = 0;
			x = data->currentMV->x; y = data->currentMV->y;
			if (bDirection & 3) {	/* our candidate is left or right */
				CHECK_CANDIDATE(x, y + iDiamondSize, 8);
				CHECK_CANDIDATE(x, y - iDiamondSize, 4);
			} else {			/* what remains here is up or down */
				CHECK_CANDIDATE(x + iDiamondSize, y, 2);
				CHECK_CANDIDATE(x - iDiamondSize, y, 1);
			}

			if (*iDirection) {
				bDirection += *iDirection;
				x = data->currentMV->x; y = data->currentMV->y;
			}
		} else {				/* about to quit, eh? not so fast.... */
			switch (bDirection) {
			case 2:
				CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2 + 4);
				CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2 + 8);
				break;
			case 1:
				CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1 + 4);
				CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1 + 8);
				break;
			case 2 + 4:
				CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1 + 4);
				CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2 + 4);
				CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2 + 8);
				break;
			case 4:
				CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2 + 4);
				CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1 + 4);
				break;
			case 8:
				CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2 + 8);
				CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1 + 8);
				break;
			case 1 + 4:
				CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1 + 8);
				CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1 + 4);
				CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2 + 4);
				break;
			case 2 + 8:
				CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2 + 4);
				CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2 + 8);
				CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1 + 8);
				break;
			case 1 + 8:
				CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1 + 4);
				CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1 + 8);
				CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2 + 8);
				break;
			default:		/* 1+2+4+8 == we didn't find anything at all */
				CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1 + 4);
				CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1 + 8);
				CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2 + 4);
				CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2 + 8);
				break;
			}
			if (!*iDirection) break;		/* ok, the end. really */
			bDirection = *iDirection;
			x = data->currentMV->x; y = data->currentMV->y;
		}
	}
}

void
xvid_me_SquareSearch(int x, int y, SearchData * const data,
					 int bDirection, CheckFunc * const CheckCandidate)
{
	unsigned int * const iDirection = &data->dir;

	do {
		*iDirection = 0;
		if (bDirection & 1) CHECK_CANDIDATE(x - iDiamondSize, y, 1+16+64);
		if (bDirection & 2) CHECK_CANDIDATE(x + iDiamondSize, y, 2+32+128);
		if (bDirection & 4) CHECK_CANDIDATE(x, y - iDiamondSize, 4+16+32);
		if (bDirection & 8) CHECK_CANDIDATE(x, y + iDiamondSize, 8+64+128);
		if (bDirection & 16) CHECK_CANDIDATE(x - iDiamondSize, y - iDiamondSize, 1+4+16+32+64);
		if (bDirection & 32) CHECK_CANDIDATE(x + iDiamondSize, y - iDiamondSize, 2+4+16+32+128);
		if (bDirection & 64) CHECK_CANDIDATE(x - iDiamondSize, y + iDiamondSize, 1+8+16+64+128);
		if (bDirection & 128) CHECK_CANDIDATE(x + iDiamondSize, y + iDiamondSize, 2+8+32+64+128);

		bDirection = *iDirection;
		x = data->currentMV->x; y = data->currentMV->y;
	} while (*iDirection);
}

void
xvid_me_DiamondSearch(int x, int y, SearchData * const data,
					  int bDirection, CheckFunc * const CheckCandidate)
{

/* directions: 1 - left (x-1); 2 - right (x+1), 4 - up (y-1); 8 - down (y+1) */

	unsigned int * const iDirection = &data->dir;

	for (;;) {
		*iDirection = 0;
		if (bDirection & 1) CHECK_CANDIDATE(x - iDiamondSize, y, 1);
		if (bDirection & 2) CHECK_CANDIDATE(x + iDiamondSize, y, 2);
		if (bDirection & 4) CHECK_CANDIDATE(x, y - iDiamondSize, 4);
		if (bDirection & 8) CHECK_CANDIDATE(x, y + iDiamondSize, 8);

		if (*iDirection == 0)
			break;
		
		/* now we're doing diagonal checks near our candidate */
		bDirection = *iDirection;
		x = data->currentMV->x; y = data->currentMV->y;
		if (bDirection & 3) {	/* our candidate is left or right */
			CHECK_CANDIDATE(x, y + iDiamondSize, 8);
			CHECK_CANDIDATE(x, y - iDiamondSize, 4);
		} else {			/* what remains here is up or down */
			CHECK_CANDIDATE(x + iDiamondSize, y, 2);
			CHECK_CANDIDATE(x - iDiamondSize, y, 1);
		}
		bDirection |= *iDirection;
		x = data->currentMV->x; y = data->currentMV->y;
	}
}

void
xvid_me_SubpelRefine(VECTOR centerMV, SearchData * const data, CheckFunc * const CheckCandidate, int dir)
{
/* Do a half-pel or q-pel refinement */

	CHECK_CANDIDATE(centerMV.x, centerMV.y - 1, dir);
	CHECK_CANDIDATE(centerMV.x + 1, centerMV.y - 1, dir);
	CHECK_CANDIDATE(centerMV.x + 1, centerMV.y, dir);
	CHECK_CANDIDATE(centerMV.x + 1, centerMV.y + 1, dir);
	CHECK_CANDIDATE(centerMV.x, centerMV.y + 1, dir);
	CHECK_CANDIDATE(centerMV.x - 1, centerMV.y + 1, dir);
	CHECK_CANDIDATE(centerMV.x - 1, centerMV.y, dir);
	CHECK_CANDIDATE(centerMV.x - 1, centerMV.y - 1, dir);
}

#define CHECK_CANDIDATE_2ndBEST(X, Y, DIR) { \
	*data->iMinSAD = s_best2; \
	CheckCandidate((X),(Y), data, direction); \
	if (data->iMinSAD[0] < s_best) { \
		s_best2 = s_best; \
		s_best = data->iMinSAD[0]; \
		v_best2 = v_best; \
		v_best.x = X; v_best.y = Y; \
		dir = DIR; \
	} else if (data->iMinSAD[0] < s_best2) { \
		s_best2 = data->iMinSAD[0]; \
		v_best2.x = X; v_best2.y = Y; \
	} \
}

void
FullRefine_Fast(SearchData * data, CheckFunc * CheckCandidate, int direction)
{
/* Do a fast h-pel and then q-pel refinement */
	
	int32_t s_best = data->iMinSAD[0], s_best2 = 256*4096;
	VECTOR v_best, v_best2;
	int dir = 0, xo2, yo2, best_halfpel, b_cbp;

	int xo = 2*data->currentMV[0].x, yo = 2*data->currentMV[0].y;
	
	data->currentQMV[0].x = v_best.x = v_best2.x = xo;
	data->currentQMV[0].y = v_best.y = v_best2.y = yo;

	data->qpel_precision = 1;

	/* halfpel refinement: check 8 neighbours, but keep the second best SAD as well */
	CHECK_CANDIDATE_2ndBEST(xo - 2,	yo,		1+16+64);
	CHECK_CANDIDATE_2ndBEST(xo + 2,	yo,		2+32+128);
	CHECK_CANDIDATE_2ndBEST(xo,		yo - 2,	4+16+32);
	CHECK_CANDIDATE_2ndBEST(xo,		yo + 2,	8+64+128);
	CHECK_CANDIDATE_2ndBEST(xo - 2,	yo - 2,	1+4+16+32+64);
	CHECK_CANDIDATE_2ndBEST(xo + 2,	yo - 2,	2+4+16+32+128);
	CHECK_CANDIDATE_2ndBEST(xo - 2,	yo + 2,	1+8+16+64+128);
	CHECK_CANDIDATE_2ndBEST(xo + 2,	yo + 2,	2+8+32+64+128);

	xo = v_best.x; yo = v_best.y, b_cbp = data->cbp[0];

	/* we need all 8 neighbours *of best hpel position found above* checked for 2nd best
		let's check the missing ones */

	/* on rare occasions, 1st best and 2nd best are far away, and 2nd best is not 1st best's neighbour.
		to simplify stuff, we'll forget that evil 2nd best and make a full search for a new 2nd best */
	/* todo. we should check the missing neighbours first, maybe they'll give us 2nd best which is even better
		than the infamous one. in that case, we will not have to re-check the other neighbours */

	if (abs(v_best.x - v_best2.x) > 2 || abs(v_best.y - v_best2.y) > 2) { /* v_best2 is useless */
		data->iMinSAD[0] = 256*4096;
		dir = ~0; /* all */
	} else {
		data->iMinSAD[0] = s_best2;
		data->currentQMV[0] = v_best2;
	}

	if (dir & 1) CHECK_CANDIDATE(	xo - 2,	yo,		direction);
	if (dir & 2) CHECK_CANDIDATE(	xo + 2,	yo,		direction);
	if (dir & 4) CHECK_CANDIDATE(	xo,		yo - 2,	direction);
	if (dir & 8) CHECK_CANDIDATE(	xo,		yo + 2,	direction);
	if (dir & 16) CHECK_CANDIDATE(	xo - 2,	yo - 2,	direction);
	if (dir & 32) CHECK_CANDIDATE(	xo + 2,	yo - 2,	direction);
	if (dir & 64) CHECK_CANDIDATE(	xo - 2,	yo + 2,	direction);
	if (dir & 128) CHECK_CANDIDATE(	xo + 2,	yo + 2,	direction);

	/* read the position of 2nd best */
	v_best2 = data->currentQMV[0];

	/* after second_best has been found, go back to best vector */

	data->currentQMV[0].x = xo;
	data->currentQMV[0].y = yo;
	data->cbp[0] = b_cbp;

	data->currentMV[0].x = xo/2;
	data->currentMV[0].y = yo/2;
	data->iMinSAD[0] = best_halfpel = s_best;

	xo2 = v_best2.x;
	yo2 = v_best2.y;
	s_best2 = 256*4096;

	if (yo == yo2) {
		CHECK_CANDIDATE_2ndBEST((xo+xo2)>>1, yo, 0);
		CHECK_CANDIDATE_2ndBEST(xo, yo-1, 0);
		CHECK_CANDIDATE_2ndBEST(xo, yo+1, 0);
		data->currentQMV[0] = v_best;
		data->iMinSAD[0] = s_best;

		if(best_halfpel <= s_best2) return;

		if(data->currentQMV[0].x == v_best2.x) {
			CHECK_CANDIDATE((xo+xo2)>>1, yo-1, direction);
			CHECK_CANDIDATE((xo+xo2)>>1, yo+1, direction);
		} else {
			CHECK_CANDIDATE((xo+xo2)>>1,
				(data->currentQMV[0].x == xo) ? data->currentQMV[0].y : v_best2.y, direction);
		}
		return;
	}

	if (xo == xo2) {
		CHECK_CANDIDATE_2ndBEST(xo, (yo+yo2)>>1, 0);
		CHECK_CANDIDATE_2ndBEST(xo-1, yo, 0);
		CHECK_CANDIDATE_2ndBEST(xo+1, yo, 0);
		data->currentQMV[0] = v_best;
		data->iMinSAD[0] = s_best;

		if(best_halfpel <= s_best2) return;

		if(data->currentQMV[0].y == v_best2.y) {
			CHECK_CANDIDATE(xo-1, (yo+yo2)>>1, direction);
			CHECK_CANDIDATE(xo+1, (yo+yo2)>>1, direction);
		} else {
			CHECK_CANDIDATE((data->currentQMV[0].y == yo) ? data->currentQMV[0].x : v_best2.x, (yo+yo2)>>1, direction);
		}
		return;
	}

	CHECK_CANDIDATE_2ndBEST(xo, (yo+yo2)>>1, 0);
	CHECK_CANDIDATE_2ndBEST((xo+xo2)>>1, yo, 0);
	data->currentQMV[0] = v_best;
	data->iMinSAD[0] = s_best;

	if(best_halfpel <= s_best2) return;

	CHECK_CANDIDATE((xo+xo2)>>1, (yo+yo2)>>1, direction);

}

/* it's the positive max, so "32" needs fcode of 2, not 1 */
unsigned int
getMinFcode(const int MVmax)
{
	unsigned int fcode;
	for (fcode = 1; (16 << fcode) <= MVmax; fcode++);
	return fcode;
}
