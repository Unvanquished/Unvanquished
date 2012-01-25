/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Motion Estimation for B-VOPs  -
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
 * $Id: estimation_bvop.c,v 1.25 2006/02/25 01:20:41 syskin Exp $
 *
 ****************************************************************************/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* memcpy */

#include "../encoder.h"
#include "../global.h"
#include "../image/interpolate8x8.h"
#include "estimation.h"
#include "motion.h"
#include "sad.h"
#include "motion_inlines.h"

static int32_t
ChromaSAD2(const int fx, const int fy, const int bx, const int by,
			SearchData * const data)
{
	int sad;
	const uint32_t stride = data->iEdgedWidth/2;
	uint8_t *f_refu, *f_refv, *b_refu, *b_refv;
	int offset, filter;

	const INTERPOLATE8X8_PTR interpolate8x8_halfpel[] = {
		NULL,
		interpolate8x8_halfpel_v,
		interpolate8x8_halfpel_h,
		interpolate8x8_halfpel_hv
	};
	
	if (data->chromaX == fx && data->chromaY == fy && 
		data->b_chromaX == bx && data->b_chromaY == by) 
		return data->chromaSAD;

	offset = (fx>>1) + (fy>>1)*stride;
	filter = ((fx & 1) << 1) | (fy & 1);

	if (filter != 0) {
		f_refu = data->RefQ + 64;
		f_refv = data->RefQ + 64 + 8;
		if (data->chromaX != fx || data->chromaY != fy) {
			interpolate8x8_halfpel[filter](f_refu, data->RefP[4] + offset, stride, data->rounding);
			interpolate8x8_halfpel[filter](f_refv, data->RefP[5] + offset, stride, data->rounding);
		}
	} else {
		f_refu = (uint8_t*)data->RefP[4] + offset;
		f_refv = (uint8_t*)data->RefP[5] + offset;
	}
	data->chromaX = fx; data->chromaY = fy;

	offset = (bx>>1) + (by>>1)*stride;
	filter = ((bx & 1) << 1) | (by & 1);

	if (filter != 0) {
		b_refu = data->RefQ + 64 + 16;
		b_refv = data->RefQ + 64 + 24;
		if (data->b_chromaX != bx || data->b_chromaY != by) {
			interpolate8x8_halfpel[filter](b_refu, data->b_RefP[4] + offset, stride, data->rounding);
			interpolate8x8_halfpel[filter](b_refv, data->b_RefP[5] + offset, stride, data->rounding);
		}
	} else {
		b_refu = (uint8_t*)data->b_RefP[4] + offset;
		b_refv = (uint8_t*)data->b_RefP[5] + offset;
	}
	data->b_chromaX = bx; data->b_chromaY = by;

	sad = sad8bi(data->CurU, b_refu, f_refu, stride);
	sad += sad8bi(data->CurV, b_refv, f_refv, stride);

	data->chromaSAD = sad;
	return sad;
}

static void
CheckCandidateInt(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t sad, xf, yf, xb, yb, xcf, ycf, xcb, ycb;
	uint32_t t;

	const uint8_t *ReferenceF, *ReferenceB;
	VECTOR *current;

	if ((x > data->max_dx) || (x < data->min_dx) ||
		(y > data->max_dy) || (y < data->min_dy))
		return;

	if (Direction == 1) { /* x and y mean forward vector */
		VECTOR backward = data->qpel_precision ? data->currentQMV[1] : data->currentMV[1];
		xb = backward.x;
		yb = backward.y;
		xf = x; yf = y;
	} else { /* x and y mean backward vector */
		VECTOR forward = data->qpel_precision ? data->currentQMV[0] : data->currentMV[0];
		xf = forward.x;
		yf = forward.y;
		xb = x; yb = y;
	}

	if (!data->qpel_precision) {
		ReferenceF = GetReference(xf, yf, data);
		ReferenceB = GetReferenceB(xb, yb, 1, data);
		current = data->currentMV + Direction - 1;
		xcf = xf; ycf = yf;
		xcb = xb; ycb = yb;
	} else {
		ReferenceF = xvid_me_interpolate16x16qpel(xf, yf, 0, data);
		current = data->currentQMV + Direction - 1;
		ReferenceB = xvid_me_interpolate16x16qpel(xb, yb, 1, data);
		xcf = xf/2; ycf = yf/2;
		xcb = xb/2; ycb = yb/2;
	}

	t = d_mv_bits(xf, yf, data->predMV, data->iFcode, data->qpel^data->qpel_precision)
		 + d_mv_bits(xb, yb, data->bpredMV, data->iFcode, data->qpel^data->qpel_precision);

	sad = sad16bi(data->Cur, ReferenceF, ReferenceB, data->iEdgedWidth);
	sad += (data->lambda16 * t);

	if (data->chroma && sad < *data->iMinSAD)
		sad += ChromaSAD2((xcf >> 1) + roundtab_79[xcf & 0x3],
							(ycf >> 1) + roundtab_79[ycf & 0x3],
							(xcb >> 1) + roundtab_79[xcb & 0x3],
							(ycb >> 1) + roundtab_79[ycb & 0x3], data);

	if (sad < *(data->iMinSAD)) {
		*data->iMinSAD = sad;
		current->x = x; current->y = y;
		data->dir = Direction;
	}
}

static void
CheckCandidateDirect(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t sad = 0, xcf = 0, ycf = 0, xcb = 0, ycb = 0;
	uint32_t k;
	const uint8_t *ReferenceF;
	const uint8_t *ReferenceB;
	VECTOR mvs, b_mvs;
	const int blocks[4] = {0, 8, 8*data->iEdgedWidth, 8*data->iEdgedWidth+8};

	if (( x > 31) || ( x < -32) || ( y > 31) || (y < -32)) return;

	for (k = 0; k < 4; k++) {
		mvs.x = data->directmvF[k].x + x;
		b_mvs.x = ((x == 0) ?
			data->directmvB[k].x
			: mvs.x - data->referencemv[k].x);

		mvs.y = data->directmvF[k].y + y;
		b_mvs.y = ((y == 0) ?
			data->directmvB[k].y
			: mvs.y - data->referencemv[k].y);

		if ((mvs.x > data->max_dx)   || (mvs.x < data->min_dx)   ||
			(mvs.y > data->max_dy)   || (mvs.y < data->min_dy)   ||
			(b_mvs.x > data->max_dx) || (b_mvs.x < data->min_dx) ||
			(b_mvs.y > data->max_dy) || (b_mvs.y < data->min_dy) )
			return;

		if (data->qpel) {
			xcf += mvs.x/2; ycf += mvs.y/2;
			xcb += b_mvs.x/2; ycb += b_mvs.y/2;
			if (data->qpel_precision) {
				ReferenceF = xvid_me_interpolate8x8qpel(mvs.x, mvs.y, k, 0, data);
				ReferenceB = xvid_me_interpolate8x8qpel(b_mvs.x, b_mvs.y, k, 1, data);
				goto done;
			}
			mvs.x >>=1; mvs.y >>=1; b_mvs.x >>=1; b_mvs.y >>=1; // qpel->hpel
		} else {
			xcf += mvs.x; ycf += mvs.y;
			xcb += b_mvs.x; ycb += b_mvs.y;
		}
		ReferenceF = GetReference(mvs.x, mvs.y, data) + blocks[k];
		ReferenceB = GetReferenceB(b_mvs.x, b_mvs.y, 1, data) + blocks[k];
done:
		sad += data->iMinSAD[k+1] = 
			sad8bi(data->Cur + blocks[k],
					ReferenceF, ReferenceB, data->iEdgedWidth);
		if (sad > *(data->iMinSAD)) return;
	}

	sad += (data->lambda16 * d_mv_bits(x, y, zeroMV, 1, 0));

	if (data->chroma && sad < *data->iMinSAD)
		sad += ChromaSAD2((xcf >> 3) + roundtab_76[xcf & 0xf],
							(ycf >> 3) + roundtab_76[ycf & 0xf],
							(xcb >> 3) + roundtab_76[xcb & 0xf],
							(ycb >> 3) + roundtab_76[ycb & 0xf], data);

	if (sad < *(data->iMinSAD)) {
		data->iMinSAD[0] = sad;
		data->currentMV->x = x; data->currentMV->y = y;
		data->dir = Direction;
	}
}

static void
CheckCandidateDirectno4v(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t sad, xcf, ycf, xcb, ycb;
	const uint8_t *ReferenceF;
	const uint8_t *ReferenceB;
	VECTOR mvs, b_mvs;

	if (( x > 31) || ( x < -32) || ( y > 31) || (y < -32)) return;

	mvs.x = data->directmvF[0].x + x;
	b_mvs.x = ((x == 0) ?
		data->directmvB[0].x
		: mvs.x - data->referencemv[0].x);

	mvs.y = data->directmvF[0].y + y;
	b_mvs.y = ((y == 0) ?
		data->directmvB[0].y
		: mvs.y - data->referencemv[0].y);

	if ( (mvs.x > data->max_dx) || (mvs.x < data->min_dx)
		|| (mvs.y > data->max_dy) || (mvs.y < data->min_dy)
		|| (b_mvs.x > data->max_dx) || (b_mvs.x < data->min_dx)
		|| (b_mvs.y > data->max_dy) || (b_mvs.y < data->min_dy) ) return;

	if (data->qpel) {
		xcf = 4*(mvs.x/2); ycf = 4*(mvs.y/2);
		xcb = 4*(b_mvs.x/2); ycb = 4*(b_mvs.y/2);
		if (data->qpel_precision) {
			ReferenceF = xvid_me_interpolate16x16qpel(mvs.x, mvs.y, 0, data);
			ReferenceB = xvid_me_interpolate16x16qpel(b_mvs.x, b_mvs.y, 1, data);
			goto done;
		}
		mvs.x >>=1; mvs.y >>=1; b_mvs.x >>=1; b_mvs.y >>=1; // qpel->hpel
	} else {
		xcf = 4*mvs.x; ycf = 4*mvs.y;
		xcb = 4*b_mvs.x; ycb = 4*b_mvs.y;
	}
	ReferenceF = GetReference(mvs.x, mvs.y, data);
	ReferenceB = GetReferenceB(b_mvs.x, b_mvs.y, 1, data);

done:
	sad = sad16bi(data->Cur, ReferenceF, ReferenceB, data->iEdgedWidth);
	sad += (data->lambda16 * d_mv_bits(x, y, zeroMV, 1, 0));

	if (data->chroma && sad < *data->iMinSAD)
		sad += ChromaSAD2((xcf >> 3) + roundtab_76[xcf & 0xf],
							(ycf >> 3) + roundtab_76[ycf & 0xf],
							(xcb >> 3) + roundtab_76[xcb & 0xf],
							(ycb >> 3) + roundtab_76[ycb & 0xf], data);

	if (sad < *(data->iMinSAD)) {
		*(data->iMinSAD) = sad;
		data->currentMV->x = x; data->currentMV->y = y;
		data->dir = Direction;
	}
}

void
CheckCandidate16no4v(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t sad, xc, yc;
	const uint8_t * Reference;
	uint32_t t;
	VECTOR * current;

	if ( (x > data->max_dx) || ( x < data->min_dx)
		|| (y > data->max_dy) || (y < data->min_dy) ) return;

	if (data->qpel_precision) { /* x and y are in 1/4 precision */
		Reference = xvid_me_interpolate16x16qpel(x, y, 0, data);
		current = data->currentQMV;
		xc = x/2; yc = y/2;
	} else {
		Reference = GetReference(x, y, data);
		current = data->currentMV;
		xc = x; yc = y;
	}
	t = d_mv_bits(x, y, data->predMV, data->iFcode,
					data->qpel^data->qpel_precision);

	sad = sad16(data->Cur, Reference, data->iEdgedWidth, 256*4096);
	sad += (data->lambda16 * t);

	if (data->chroma && sad < *data->iMinSAD)
		sad += xvid_me_ChromaSAD((xc >> 1) + roundtab_79[xc & 0x3],
								(yc >> 1) + roundtab_79[yc & 0x3], data);

	if (sad < *(data->iMinSAD)) {
		*(data->iMinSAD) = sad;
		current->x = x; current->y = y;
		data->dir = Direction;
	}
}


static void 
initialize_searchData(SearchData * Data_d,
					  SearchData * Data_f,
					  SearchData * Data_b,
					  SearchData * Data_i,
					  int x, int y,
					  const IMAGE * const f_Ref,
					  const uint8_t * const f_RefH,
					  const uint8_t * const f_RefV,
					  const uint8_t * const f_RefHV,
					  const IMAGE * const b_Ref,
					  const uint8_t * const b_RefH,
					  const uint8_t * const b_RefV,
					  const uint8_t * const b_RefHV,
					  const IMAGE * const pCur,
					  const MACROBLOCK * const b_mb)
{

	/* per-macroblock SearchData initialization - too many things would be repeated 4 times */
	const uint8_t * RefP[6], * b_RefP[6], * Cur[3];
	const uint32_t iEdgedWidth = Data_d->iEdgedWidth;
	unsigned int lambda;
	int i;

	/* luma */
	int offset = (x + iEdgedWidth*y) * 16;
	RefP[0] = f_Ref->y + offset;
	RefP[2] = f_RefH + offset;
	RefP[1] = f_RefV + offset;
	RefP[3] = f_RefHV + offset;
	b_RefP[0] = b_Ref->y + offset;
	b_RefP[2] = b_RefH + offset;
	b_RefP[1] = b_RefV + offset;
	b_RefP[3] = b_RefHV + offset;
	Cur[0] = pCur->y + offset;

	/* chroma */
	offset = (x + (iEdgedWidth/2)*y) * 8;
	RefP[4] = f_Ref->u + offset;
	RefP[5] = f_Ref->v + offset;
	b_RefP[4] = b_Ref->u + offset;
	b_RefP[5] = b_Ref->v + offset;
	Cur[1] = pCur->u + offset;
	Cur[2] = pCur->v + offset;

	lambda = xvid_me_lambda_vec16[b_mb->quant];

	for (i = 0; i < 6; i++) {
		Data_d->RefP[i] = Data_f->RefP[i] = Data_i->RefP[i] = RefP[i];
		Data_d->b_RefP[i] = Data_b->RefP[i] = Data_i->b_RefP[i] = b_RefP[i];
	}
	Data_d->Cur = Data_f->Cur = Data_b->Cur = Data_i->Cur = Cur[0];
	Data_d->CurU = Data_f->CurU = Data_b->CurU = Data_i->CurU = Cur[1];
	Data_d->CurV = Data_f->CurV = Data_b->CurV = Data_i->CurV = Cur[2];

	Data_d->lambda16 = Data_f->lambda16 = Data_b->lambda16 = Data_i->lambda16 = lambda;

	/* reset chroma-sad cache */
	Data_d->b_chromaX = Data_d->b_chromaY = Data_d->chromaX = Data_d->chromaY = Data_d->chromaSAD = 256*4096; 
	Data_i->b_chromaX = Data_i->b_chromaY = Data_i->chromaX = Data_i->chromaY = Data_i->chromaSAD = 256*4096; 
	Data_f->chromaX = Data_f->chromaY = Data_f->chromaSAD = 256*4096; 
	Data_b->chromaX = Data_b->chromaY = Data_b->chromaSAD = 256*4096; 

	*Data_d->iMinSAD = *Data_b->iMinSAD = *Data_f->iMinSAD = *Data_i->iMinSAD = 4096*256;
}

static __inline VECTOR
ChoosePred(const MACROBLOCK * const pMB, const uint32_t mode)
{
/* the stupidiest function ever */
	return (mode == MODE_FORWARD ? pMB->mvs[0] : pMB->b_mvs[0]);
}

static void __inline
PreparePredictionsBF(VECTOR * const pmv, const int x, const int y,
							const uint32_t iWcount,
							const MACROBLOCK * const pMB,
							const uint32_t mode_curr,
							const VECTOR hint)
{
	/* [0] is prediction */
	/* [1] is zero */
	pmv[1].x = pmv[1].y = 0; 

	pmv[2].x = hint.x; pmv[2].y = hint.y;

	if ((y != 0)&&(x != (int)(iWcount+1))) {			/* [3] top-right neighbour */
		pmv[3] = ChoosePred(pMB+1-iWcount, mode_curr);
	} else pmv[3].x = pmv[3].y = 0;

	if (y != 0) {
		pmv[4] = ChoosePred(pMB-iWcount, mode_curr);
	} else pmv[4].x = pmv[4].y = 0;

	if (x != 0) {
		pmv[5] = ChoosePred(pMB-1, mode_curr);
	} else pmv[5].x = pmv[5].y = 0;

	if (x != 0 && y != 0) {
		pmv[6] = ChoosePred(pMB-1-iWcount, mode_curr);
	} else pmv[6].x = pmv[6].y = 0;
}

/* search backward or forward */
static void
SearchBF_initial(const int x, const int y,
			const uint32_t MotionFlags,
			const uint32_t iFcode,
			const MBParam * const pParam,
			MACROBLOCK * const pMB,
			const VECTOR * const predMV,
			int32_t * const best_sad,
			const int32_t mode_current,
			SearchData * const Data,
			VECTOR hint)
{

	int i;
	VECTOR pmv[7];
	*Data->iMinSAD = MV_MAX_ERROR;
	Data->qpel_precision = 0;

	Data->predMV = *predMV;

	get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
				pParam->width, pParam->height, iFcode - Data->qpel, 1);

	pmv[0] = Data->predMV;
	if (Data->qpel) { 
		pmv[0].x /= 2; pmv[0].y /= 2; 
		hint.x /= 2; hint.y /= 2;
	}

	PreparePredictionsBF(pmv, x, y, pParam->mb_width, pMB, mode_current, hint);

	Data->currentMV->x = Data->currentMV->y = 0;

	/* main loop. checking all predictions */
	for (i = 0; i < 7; i++)
		if (!vector_repeats(pmv, i) )
			CheckCandidate16no4v(pmv[i].x, pmv[i].y, Data, i);

	if (*Data->iMinSAD > 512) {
		unsigned int mask = make_mask(pmv, 7, Data->dir);

		MainSearchFunc *MainSearchPtr;
		if (MotionFlags & XVID_ME_USESQUARES16) MainSearchPtr = xvid_me_SquareSearch;
		else if (MotionFlags & XVID_ME_ADVANCEDDIAMOND16) MainSearchPtr = xvid_me_AdvDiamondSearch;
		else MainSearchPtr = xvid_me_DiamondSearch;
		
		MainSearchPtr(Data->currentMV->x, Data->currentMV->y, Data, mask, CheckCandidate16no4v);
	}

	if (Data->iMinSAD[0] < *best_sad) *best_sad = Data->iMinSAD[0];
}

static void
SearchBF_final(const int x, const int y,
			const uint32_t MotionFlags,
			const MBParam * const pParam,
			int32_t * const best_sad,
			SearchData * const Data)
{
	if(!Data->qpel) {
		/* halfpel mode */
		if (MotionFlags & XVID_ME_HALFPELREFINE16)
				xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate16no4v, 0);
	} else {
		/* qpel mode */
		if(MotionFlags & XVID_ME_FASTREFINE16) {
			/* fast */
			get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
						pParam->width, pParam->height, Data->iFcode, 2);
			FullRefine_Fast(Data, CheckCandidate16no4v, 0);

		} else {

			Data->currentQMV->x = 2*Data->currentMV->x;
			Data->currentQMV->y = 2*Data->currentMV->y;
			if(MotionFlags & XVID_ME_QUARTERPELREFINE16) {
				/* full */
				if (MotionFlags & XVID_ME_HALFPELREFINE16) {
					xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate16no4v, 0); /* hpel part */
					Data->currentQMV->x = 2*Data->currentMV->x;
					Data->currentQMV->y = 2*Data->currentMV->y;
				}
				get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
							pParam->width, pParam->height, Data->iFcode, 2);
				Data->qpel_precision = 1;
				xvid_me_SubpelRefine(Data->currentQMV[0], Data, CheckCandidate16no4v, 0); /* qpel part */
			}
		}
	}
	if (Data->iMinSAD[0] < *best_sad) *best_sad = Data->iMinSAD[0];

}

static void
SkipDecisionB(MACROBLOCK * const pMB, const SearchData * const Data)
{
	int k;

	if (!Data->chroma) {
		int dx = 0, dy = 0, b_dx = 0, b_dy = 0;
		int32_t sum;
		const uint32_t stride = Data->iEdgedWidth/2;
		/* this is not full chroma compensation, only it's fullpel approximation. should work though */

		for (k = 0; k < 4; k++) {
			dy += Data->directmvF[k].y >> Data->qpel;
			dx += Data->directmvF[k].x >> Data->qpel;
			b_dy += Data->directmvB[k].y >> Data->qpel;
			b_dx += Data->directmvB[k].x >> Data->qpel;
		}

		dy = (dy >> 3) + roundtab_76[dy & 0xf];
		dx = (dx >> 3) + roundtab_76[dx & 0xf];
		b_dy = (b_dy >> 3) + roundtab_76[b_dy & 0xf];
		b_dx = (b_dx >> 3) + roundtab_76[b_dx & 0xf];

		sum = sad8bi(Data->CurU,
						Data->RefP[4] + (dy/2) * (int)stride + dx/2,
						Data->b_RefP[4] + (b_dy/2) * (int)stride + b_dx/2,
						stride);

		if (sum >= MAX_CHROMA_SAD_FOR_SKIP * (int)Data->iQuant) return; /* no skip */

		sum += sad8bi(Data->CurV,
						Data->RefP[5] + (dy/2) * (int)stride + dx/2,
						Data->b_RefP[5] + (b_dy/2) * (int)stride + b_dx/2,
						stride);

		if (sum >= MAX_CHROMA_SAD_FOR_SKIP * (int)Data->iQuant) return; /* no skip */
	} else {
		int sum = Data->chromaSAD; /* chroma-sad SAD caching keeps it there */

		if (sum >= MAX_CHROMA_SAD_FOR_SKIP * (int)Data->iQuant) return; /* no skip */
	}

	/* skip */
	pMB->mode = MODE_DIRECT_NONE_MV; /* skipped */
	for (k = 0; k < 4; k++) {
		pMB->qmvs[k] = pMB->mvs[k] = Data->directmvF[k];
		pMB->b_qmvs[k] = pMB->b_mvs[k] =  Data->directmvB[k];
		if (Data->qpel) {
			pMB->mvs[k].x /= 2; pMB->mvs[k].y /= 2; /* it's a hint for future searches */
			pMB->b_mvs[k].x /= 2; pMB->b_mvs[k].y /= 2;
		}
	}
}

static uint32_t
SearchDirect_initial(const int x, const int y,
				const uint32_t MotionFlags,
				const int32_t TRB, const int32_t TRD,
				const MBParam * const pParam,
				MACROBLOCK * const pMB,
				const MACROBLOCK * const b_mb,
				int32_t * const best_sad,
				SearchData * const Data)

{
	int32_t skip_sad;
	int k = (x + Data->iEdgedWidth*y) * 16;

	k = Data->qpel ? 4 : 2;
	Data->max_dx = k * (pParam->width - x * 16);
	Data->max_dy = k * (pParam->height - y * 16);
	Data->min_dx = -k * (16 + x * 16);
	Data->min_dy = -k * (16 + y * 16);

	Data->referencemv = Data->qpel ? b_mb->qmvs : b_mb->mvs;

	for (k = 0; k < 4; k++) {
		Data->directmvF[k].x = ((TRB * Data->referencemv[k].x) / TRD);
		Data->directmvB[k].x = ((TRB - TRD) * Data->referencemv[k].x) / TRD;
		Data->directmvF[k].y = ((TRB * Data->referencemv[k].y) / TRD);
		Data->directmvB[k].y = ((TRB - TRD) * Data->referencemv[k].y) / TRD;

		if ( (Data->directmvB[k].x > Data->max_dx) | (Data->directmvB[k].x < Data->min_dx)
			| (Data->directmvB[k].y > Data->max_dy) | (Data->directmvB[k].y < Data->min_dy) ) {

			Data->iMinSAD[0] = *best_sad = 256*4096; /* in that case, we won't use direct mode */
			return 256*4096;
		}
		if (b_mb->mode != MODE_INTER4V) {
			Data->directmvF[1] = Data->directmvF[2] = Data->directmvF[3] = Data->directmvF[0];
			Data->directmvB[1] = Data->directmvB[2] = Data->directmvB[3] = Data->directmvB[0];
			break;
		}
	}
	Data->qpel_precision = Data->qpel; /* this initial check is done with full precision, to find real 
											SKIP sad */

	CheckCandidateDirect(0, 0, Data, 255);	/* will also fill iMinSAD[1..4] with 8x8 SADs */

	/* initial (fast) skip decision */
	if (Data->iMinSAD[1] < (int)Data->iQuant * INITIAL_SKIP_THRESH
		&& Data->iMinSAD[2] < (int)Data->iQuant * INITIAL_SKIP_THRESH
		&& Data->iMinSAD[3] < (int)Data->iQuant * INITIAL_SKIP_THRESH
		&& Data->iMinSAD[4] < (int)Data->iQuant * INITIAL_SKIP_THRESH) {
		/* possible skip */
		SkipDecisionB(pMB, Data);
		if (pMB->mode == MODE_DIRECT_NONE_MV)
			return *Data->iMinSAD; /* skipped */
	}

	if (Data->chroma && Data->chromaSAD >= MAX_CHROMA_SAD_FOR_SKIP * (int)Data->iQuant) /* chroma doesn't allow skip */
		skip_sad = 256*4096;
	else
		skip_sad = 4*MAX(MAX(Data->iMinSAD[1],Data->iMinSAD[2]), MAX(Data->iMinSAD[3],Data->iMinSAD[4]));

	Data->currentMV[1].x = Data->directmvF[0].x + Data->currentMV->x; /* hints for forward and backward searches */
	Data->currentMV[1].y = Data->directmvF[0].y + Data->currentMV->y;

	Data->currentMV[2].x = ((Data->currentMV->x == 0) ?
			Data->directmvB[0].x
			: Data->currentMV[1].x - Data->referencemv[0].x);

	Data->currentMV[2].y = ((Data->currentMV->y == 0) ?
			Data->directmvB[0].y
			: Data->currentMV[1].y - Data->referencemv[0].y);

	*best_sad = Data->iMinSAD[0];

	return skip_sad;
}

static void
SearchDirect_final( const uint32_t MotionFlags,
					const MACROBLOCK * const b_mb,
					int32_t * const best_sad,
					SearchData * const Data)

{
	CheckFunc * CheckCandidate = b_mb->mode == MODE_INTER4V ? 
									CheckCandidateDirect : CheckCandidateDirectno4v;
	MainSearchFunc *MainSearchPtr;

	if (MotionFlags & XVID_ME_USESQUARES16) MainSearchPtr = xvid_me_SquareSearch;
	else if (MotionFlags & XVID_ME_ADVANCEDDIAMOND16) MainSearchPtr = xvid_me_AdvDiamondSearch;
	else MainSearchPtr = xvid_me_DiamondSearch;

	Data->qpel_precision = 0;
	MainSearchPtr(0, 0, Data, 255, CheckCandidate);

	Data->qpel_precision = Data->qpel;
	if(Data->qpel) {
		*Data->iMinSAD = 256*4096; /* this old SAD was not real, it was in hpel precision */
		CheckCandidate(Data->currentMV->x, Data->currentMV->y, Data, 255);
	}

	xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate, 0);

	if (Data->iMinSAD[0] < *best_sad) {
		*best_sad = Data->iMinSAD[0];
	}
		
}


static __inline void 
set_range(int * range, SearchData * Data)
{
	Data->min_dx = range[0];
	Data->max_dx = range[1];
	Data->min_dy = range[2];
	Data->max_dy = range[3];
}

static void
SearchInterpolate_initial(
				const int x, const int y,
				const uint32_t MotionFlags,
				const MBParam * const pParam,
				const VECTOR * const f_predMV,
				const VECTOR * const b_predMV,
				int32_t * const best_sad,
				SearchData * const Data,
				const VECTOR startF,
				const VECTOR startB)

{
	int b_range[4], f_range[4];

	Data->qpel_precision = 0;

	Data->predMV = *f_predMV;
	Data->bpredMV = *b_predMV;

	Data->currentMV[0] = startF;
	Data->currentMV[1] = startB;

	get_range(f_range, f_range+1, f_range+2, f_range+3, x, y, 4, pParam->width, pParam->height, Data->iFcode - Data->qpel, 1);
	get_range(b_range, b_range+1, b_range+2, b_range+3, x, y, 4, pParam->width, pParam->height, Data->bFcode - Data->qpel, 1);

	if (Data->currentMV[0].x > f_range[1]) Data->currentMV[0].x = f_range[1];
	if (Data->currentMV[0].x < f_range[0]) Data->currentMV[0].x = f_range[0];
	if (Data->currentMV[0].y > f_range[3]) Data->currentMV[0].y = f_range[3];
	if (Data->currentMV[0].y < f_range[2]) Data->currentMV[0].y = f_range[2];

	if (Data->currentMV[1].x > b_range[1]) Data->currentMV[1].x = b_range[1];
	if (Data->currentMV[1].x < b_range[0]) Data->currentMV[1].x = b_range[0];
	if (Data->currentMV[1].y > b_range[3]) Data->currentMV[1].y = b_range[3];
	if (Data->currentMV[1].y < b_range[2]) Data->currentMV[1].y = b_range[2];

	set_range(f_range, Data);

	CheckCandidateInt(Data->currentMV[0].x, Data->currentMV[0].y, Data, 1);

	if (Data->iMinSAD[0] < *best_sad) *best_sad = Data->iMinSAD[0];
}

static void
SearchInterpolate_final(const int x, const int y,
						const uint32_t MotionFlags,
						const MBParam * const pParam,
						int32_t * const best_sad,
						SearchData * const Data)
{
	int i, j;
	int b_range[4], f_range[4];

	get_range(f_range, f_range+1, f_range+2, f_range+3, x, y, 4, pParam->width, pParam->height, Data->iFcode - Data->qpel, 1);
	get_range(b_range, b_range+1, b_range+2, b_range+3, x, y, 4, pParam->width, pParam->height, Data->bFcode - Data->qpel, 1);

	/* diamond */
	do {
		Data->dir = 0;
		/* forward MV moves */
		i = Data->currentMV[0].x; j = Data->currentMV[0].y;

		CheckCandidateInt(i + 1, j, Data, 1);
		CheckCandidateInt(i, j + 1, Data, 1);
		CheckCandidateInt(i - 1, j, Data, 1);
		CheckCandidateInt(i, j - 1, Data, 1);

		/* backward MV moves */
		set_range(b_range, Data);
		i = Data->currentMV[1].x; j = Data->currentMV[1].y;

		CheckCandidateInt(i + 1, j, Data, 2);
		CheckCandidateInt(i, j + 1, Data, 2);
		CheckCandidateInt(i - 1, j, Data, 2);
		CheckCandidateInt(i, j - 1, Data, 2);

		set_range(f_range, Data);

	} while (Data->dir != 0);

	/* qpel refinement */
	if (Data->qpel) {
		Data->qpel_precision = 1;
		get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, 
			x, y, 4, pParam->width, pParam->height, Data->iFcode, 2);
		
		Data->currentQMV[0].x = 2 * Data->currentMV[0].x;
		Data->currentQMV[0].y = 2 * Data->currentMV[0].y;
		Data->currentQMV[1].x = 2 * Data->currentMV[1].x;
		Data->currentQMV[1].y = 2 * Data->currentMV[1].y;

		if (MotionFlags & XVID_ME_QUARTERPELREFINE16) {
			xvid_me_SubpelRefine(Data->currentQMV[0], Data, CheckCandidateInt, 1);

			get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, 
				x, y, 4, pParam->width, pParam->height, Data->bFcode, 2);

			xvid_me_SubpelRefine(Data->currentQMV[1], Data, CheckCandidateInt, 2);
		}
	}

	if (Data->iMinSAD[0] < *best_sad) *best_sad = Data->iMinSAD[0];
}

static void 
ModeDecision_BVOP_SAD(const SearchData * const Data_d,
					  const SearchData * const Data_b,
					  const SearchData * const Data_f,
					  const SearchData * const Data_i,
					  MACROBLOCK * const pMB,
					  const MACROBLOCK * const b_mb,
					  VECTOR * f_predMV,
					  VECTOR * b_predMV)
{
	int mode = MODE_DIRECT, k;
	int best_sad, f_sad, b_sad, i_sad;
	const int qpel = Data_d->qpel;

	/* evaluate cost of all modes - quite simple in SAD */
	best_sad = Data_d->iMinSAD[0] + 1*Data_d->lambda16;
	b_sad = Data_b->iMinSAD[0] + 3*Data_d->lambda16;
	f_sad = Data_f->iMinSAD[0] + 4*Data_d->lambda16;
	i_sad = Data_i->iMinSAD[0] + 2*Data_d->lambda16;

	if (b_sad < best_sad) {
		mode = MODE_BACKWARD;
		best_sad = b_sad;
	}

	if (f_sad < best_sad) {
		mode = MODE_FORWARD;
		best_sad = f_sad;
	}

	if (i_sad < best_sad) {
		mode = MODE_INTERPOLATE;
		best_sad = i_sad;
	}

	pMB->sad16 = best_sad;
	pMB->mode = mode;
	pMB->cbp = 63;

	switch (mode) {

	case MODE_DIRECT:
		if (!qpel && b_mb->mode != MODE_INTER4V) pMB->mode = MODE_DIRECT_NO4V; /* for faster compensation */

		pMB->pmvs[3] = Data_d->currentMV[0];

		for (k = 0; k < 4; k++) {
			pMB->mvs[k].x = Data_d->directmvF[k].x + Data_d->currentMV->x;
			pMB->b_mvs[k].x = (	(Data_d->currentMV->x == 0)
								? Data_d->directmvB[k].x
								:pMB->mvs[k].x - Data_d->referencemv[k].x);
			pMB->mvs[k].y = (Data_d->directmvF[k].y + Data_d->currentMV->y);
			pMB->b_mvs[k].y = ((Data_d->currentMV->y == 0)
								? Data_d->directmvB[k].y
								: pMB->mvs[k].y - Data_d->referencemv[k].y);
			if (qpel) {
				pMB->qmvs[k].x = pMB->mvs[k].x; pMB->mvs[k].x /= 2;
				pMB->b_qmvs[k].x = pMB->b_mvs[k].x; pMB->b_mvs[k].x /= 2;
				pMB->qmvs[k].y = pMB->mvs[k].y; pMB->mvs[k].y /= 2;
				pMB->b_qmvs[k].y = pMB->b_mvs[k].y; pMB->b_mvs[k].y /= 2;
			}

			if (b_mb->mode != MODE_INTER4V) {
				pMB->mvs[3] = pMB->mvs[2] = pMB->mvs[1] = pMB->mvs[0];
				pMB->b_mvs[3] = pMB->b_mvs[2] = pMB->b_mvs[1] = pMB->b_mvs[0];
				pMB->qmvs[3] = pMB->qmvs[2] = pMB->qmvs[1] = pMB->qmvs[0];
				pMB->b_qmvs[3] = pMB->b_qmvs[2] = pMB->b_qmvs[1] = pMB->b_qmvs[0];
				break;
			}			
		}
		break;

	case MODE_FORWARD:
		if (qpel) {
			pMB->pmvs[0].x = Data_f->currentQMV->x - f_predMV->x;
			pMB->pmvs[0].y = Data_f->currentQMV->y - f_predMV->y;
			pMB->qmvs[0] = *Data_f->currentQMV;
			*f_predMV = Data_f->currentQMV[0];
		} else {
			pMB->pmvs[0].x = Data_f->currentMV->x - f_predMV->x;
			pMB->pmvs[0].y = Data_f->currentMV->y - f_predMV->y;
			*f_predMV = Data_f->currentMV[0];
		}
		pMB->mvs[0] = *Data_f->currentMV;
		pMB->b_mvs[0] = *Data_b->currentMV; /* hint for future searches */
		break;

	case MODE_BACKWARD:
		if (qpel) {
			pMB->pmvs[0].x = Data_b->currentQMV->x - b_predMV->x;
			pMB->pmvs[0].y = Data_b->currentQMV->y - b_predMV->y;
			pMB->b_qmvs[0] = *Data_b->currentQMV;
			*b_predMV = Data_b->currentQMV[0];
		} else {
			pMB->pmvs[0].x = Data_b->currentMV->x - b_predMV->x;
			pMB->pmvs[0].y = Data_b->currentMV->y - b_predMV->y;
			*b_predMV = Data_b->currentMV[0];
		}
		pMB->b_mvs[0] = *Data_b->currentMV;
		pMB->mvs[0] = *Data_f->currentMV; /* hint for future searches */
		break;


	case MODE_INTERPOLATE:
		pMB->mvs[0] = Data_i->currentMV[0];
		pMB->b_mvs[0] = Data_i->currentMV[1];
		if (qpel) {
			pMB->qmvs[0] = Data_i->currentQMV[0];
			pMB->b_qmvs[0] = Data_i->currentQMV[1];
			pMB->pmvs[1].x = pMB->qmvs[0].x - f_predMV->x;
			pMB->pmvs[1].y = pMB->qmvs[0].y - f_predMV->y;
			pMB->pmvs[0].x = pMB->b_qmvs[0].x - b_predMV->x;
			pMB->pmvs[0].y = pMB->b_qmvs[0].y - b_predMV->y;
			*f_predMV = Data_i->currentQMV[0];
			*b_predMV = Data_i->currentQMV[1];
		} else {
			pMB->pmvs[1].x = pMB->mvs[0].x - f_predMV->x;
			pMB->pmvs[1].y = pMB->mvs[0].y - f_predMV->y;
			pMB->pmvs[0].x = pMB->b_mvs[0].x - b_predMV->x;
			pMB->pmvs[0].y = pMB->b_mvs[0].y - b_predMV->y;
			*f_predMV = Data_i->currentMV[0];
			*b_predMV = Data_i->currentMV[1];
		}
		break;
	}
}

static __inline void
maxMotionBVOP(int * const MVmaxF, int * const MVmaxB, const MACROBLOCK * const pMB, const int qpel)
{
	if (pMB->mode == MODE_FORWARD || pMB->mode == MODE_INTERPOLATE) {
		const VECTOR * const mv = qpel ? pMB->qmvs : pMB->mvs;
		int max = *MVmaxF;
		if (mv[0].x > max) max = mv[0].x;
		else if (-mv[0].x - 1 > max) max = -mv[0].x - 1;
		if (mv[0].y > max) max = mv[0].y;
		else if (-mv[0].y - 1 > max) max = -mv[0].y - 1;

		*MVmaxF = max;
	}

	if (pMB->mode == MODE_BACKWARD || pMB->mode == MODE_INTERPOLATE) {
		const VECTOR * const mv = qpel ? pMB->b_qmvs : pMB->b_mvs;
		int max = *MVmaxB;
		if (mv[0].x > max) max = mv[0].x;
		else if (-mv[0].x - 1 > max) max = -mv[0].x - 1;
		if (mv[0].y > max) max = mv[0].y;
		else if (-mv[0].y - 1 > max) max = -mv[0].y - 1;
		*MVmaxB = max;
	}
}


void
MotionEstimationBVOP(MBParam * const pParam,
					 FRAMEINFO * const frame,
					 const int32_t time_bp,
					 const int32_t time_pp,
					 /* forward (past) reference */
					 const MACROBLOCK * const f_mbs,
					 const IMAGE * const f_ref,
					 const IMAGE * const f_refH,
					 const IMAGE * const f_refV,
					 const IMAGE * const f_refHV,
					 /* backward (future) reference */
					 const FRAMEINFO * const b_reference,
					 const IMAGE * const b_ref,
					 const IMAGE * const b_refH,
					 const IMAGE * const b_refV,
					 const IMAGE * const b_refHV)
{
	uint32_t i, j;
	int32_t best_sad = 256*4096;
	uint32_t skip_sad;
	int fb_thresh;
	const MACROBLOCK * const b_mbs = b_reference->mbs;

	VECTOR f_predMV, b_predMV;

	int MVmaxF = 0, MVmaxB = 0;
	const int32_t TRB = time_pp - time_bp;
	const int32_t TRD = time_pp;
	DECLARE_ALIGNED_MATRIX(dct_space, 3, 64, int16_t, CACHE_LINE);

	/* some pre-inintialized data for the rest of the search */
	SearchData Data_d, Data_f, Data_b, Data_i;
	memset(&Data_d, 0, sizeof(SearchData));

	Data_d.iEdgedWidth = pParam->edged_width;
	Data_d.qpel = pParam->vol_flags & XVID_VOL_QUARTERPEL ? 1 : 0;
	Data_d.rounding = 0;
	Data_d.chroma = frame->motion_flags & XVID_ME_CHROMA_BVOP;
	Data_d.iQuant = frame->quant;
	Data_d.quant_sq = frame->quant*frame->quant;
	Data_d.dctSpace = dct_space;
	Data_d.quant_type = !(pParam->vol_flags & XVID_VOL_MPEGQUANT);
	Data_d.mpeg_quant_matrices = pParam->mpeg_quant_matrices;

	Data_d.RefQ = f_refV->u; /* a good place, also used in MC (for similar purpose) */

	memcpy(&Data_f, &Data_d, sizeof(SearchData));
	memcpy(&Data_b, &Data_d, sizeof(SearchData));
	memcpy(&Data_i, &Data_d, sizeof(SearchData));

	Data_f.iFcode = Data_i.iFcode = frame->fcode = b_reference->fcode;
	Data_b.iFcode = Data_i.bFcode = frame->bcode = b_reference->fcode;

	for (j = 0; j < pParam->mb_height; j++) {

		f_predMV = b_predMV = zeroMV;	/* prediction is reset at left boundary */

		for (i = 0; i < pParam->mb_width; i++) {
			MACROBLOCK * const pMB = frame->mbs + i + j * pParam->mb_width;
			const MACROBLOCK * const b_mb = b_mbs + i + j * pParam->mb_width;
			pMB->mode = -1;

			initialize_searchData(&Data_d, &Data_f, &Data_b, &Data_i,
					  i, j, f_ref, f_refH->y, f_refV->y, f_refHV->y,
					  b_ref, b_refH->y, b_refV->y, b_refHV->y,
					  &frame->image, b_mb);

/* special case, if collocated block is SKIPed in P-VOP: encoding is forward (0,0), cpb=0 without further ado */
			if (b_reference->coding_type != S_VOP)
				if (b_mb->mode == MODE_NOT_CODED) {
					pMB->mode = MODE_NOT_CODED;
					pMB->mvs[0] = pMB->b_mvs[0] = zeroMV;
					pMB->sad16 = 0;
					continue;
				}

/* direct search comes first, because it (1) checks for SKIP-mode
	and (2) sets very good predictions for forward and backward search */
			skip_sad = SearchDirect_initial(i, j, frame->motion_flags, TRB, TRD, pParam, pMB, 
											b_mb, &best_sad, &Data_d);

			if (pMB->mode == MODE_DIRECT_NONE_MV) {
				pMB->sad16 = best_sad;
				pMB->cbp = 0;
				continue;
			}

			SearchBF_initial(i, j, frame->motion_flags, frame->fcode, pParam, pMB,
						&f_predMV, &best_sad, MODE_FORWARD, &Data_f, Data_d.currentMV[1]);

			SearchBF_initial(i, j, frame->motion_flags, frame->bcode, pParam, pMB,
						&b_predMV, &best_sad, MODE_BACKWARD, &Data_b, Data_d.currentMV[2]);

			if (frame->motion_flags&XVID_ME_BFRAME_EARLYSTOP)
				fb_thresh = best_sad;
			else
				fb_thresh = best_sad + (best_sad>>1);

			if (Data_f.iMinSAD[0] <= fb_thresh)
				SearchBF_final(i, j, frame->motion_flags, pParam, &best_sad, &Data_f);

			if (Data_b.iMinSAD[0] <= fb_thresh)
				SearchBF_final(i, j, frame->motion_flags, pParam, &best_sad, &Data_b);

			SearchInterpolate_initial(i, j, frame->motion_flags, pParam, &f_predMV, &b_predMV, &best_sad,
								  &Data_i, Data_f.currentMV[0], Data_b.currentMV[0]);

			if (((Data_i.iMinSAD[0] < best_sad +(best_sad>>3)) && !(frame->motion_flags&XVID_ME_FAST_MODEINTERPOLATE))
				|| Data_i.iMinSAD[0] <= best_sad)

				SearchInterpolate_final(i, j, frame->motion_flags, pParam, &best_sad, &Data_i);
			
			if (Data_d.iMinSAD[0] <= 2*best_sad)
				if ((!(frame->motion_flags&XVID_ME_SKIP_DELTASEARCH) && (best_sad > 750))
					|| (best_sad > 1000))
				
					SearchDirect_final(frame->motion_flags, b_mb, &best_sad, &Data_d);

			/* final skip decision */
			if ( (skip_sad < 2 * Data_d.iQuant * MAX_SAD00_FOR_SKIP )
				&& ((100*best_sad)/(skip_sad+1) > FINAL_SKIP_THRESH) ) {

				Data_d.chromaSAD = 0; /* green light for chroma check */

				SkipDecisionB(pMB, &Data_d);
				
				if (pMB->mode == MODE_DIRECT_NONE_MV) { /* skipped? */
					pMB->sad16 = skip_sad;
					pMB->cbp = 0;
					continue;
				}
			}

			if (frame->vop_flags & XVID_VOP_RD_BVOP) 
				ModeDecision_BVOP_RD(&Data_d, &Data_b, &Data_f, &Data_i, 
					pMB, b_mb, &f_predMV, &b_predMV, frame->motion_flags, pParam, i, j, best_sad);
			else
				ModeDecision_BVOP_SAD(&Data_d, &Data_b, &Data_f, &Data_i, pMB, b_mb, &f_predMV, &b_predMV);

			maxMotionBVOP(&MVmaxF, &MVmaxB, pMB, Data_d.qpel);

		}
	}

	frame->fcode = getMinFcode(MVmaxF);
	frame->bcode = getMinFcode(MVmaxB);
}



void
SMPMotionEstimationBVOP(SMPmotionData * h)
{
	const MBParam * const pParam = h->pParam;
	const FRAMEINFO * const frame = h->current;
	const int32_t time_bp = h->time_bp;
	const int32_t time_pp = h->time_pp;
	/* forward (past) reference */
	const MACROBLOCK * const f_mbs = h->f_mbs;
	const IMAGE * const f_ref = h->fRef;
	const IMAGE * const f_refH = h->fRefH;
	const IMAGE * const f_refV = h->fRefV;
	const IMAGE * const f_refHV = h->fRefHV;
	/* backward (future) reference */
	const FRAMEINFO * const b_reference = h->reference;
	const IMAGE * const b_ref = h->pRef;
	const IMAGE * const b_refH = h->pRefH;
	const IMAGE * const b_refV = h->pRefV;
	const IMAGE * const b_refHV = h->pRefHV;

	int y_step = h->y_step;
	int start_y = h->start_y;
	int * complete_count_self = h->complete_count_self;
	const int * complete_count_above = h->complete_count_above;
	int max_mbs;
	int current_mb = 0;

	int32_t i, j;
	int32_t best_sad = 256*4096;
	uint32_t skip_sad;
	int fb_thresh;
	const MACROBLOCK * const b_mbs = b_reference->mbs;

	VECTOR f_predMV, b_predMV;

	int MVmaxF = 0, MVmaxB = 0;
	const int32_t TRB = time_pp - time_bp;
	const int32_t TRD = time_pp;
	DECLARE_ALIGNED_MATRIX(dct_space, 3, 64, int16_t, CACHE_LINE);

	/* some pre-inintialized data for the rest of the search */
	SearchData Data_d, Data_f, Data_b, Data_i;
	memset(&Data_d, 0, sizeof(SearchData));

	Data_d.iEdgedWidth = pParam->edged_width;
	Data_d.qpel = pParam->vol_flags & XVID_VOL_QUARTERPEL ? 1 : 0;
	Data_d.rounding = 0;
	Data_d.chroma = frame->motion_flags & XVID_ME_CHROMA_BVOP;
	Data_d.iQuant = frame->quant;
	Data_d.quant_sq = frame->quant*frame->quant;
	Data_d.dctSpace = dct_space;
	Data_d.quant_type = !(pParam->vol_flags & XVID_VOL_MPEGQUANT);
	Data_d.mpeg_quant_matrices = pParam->mpeg_quant_matrices;

	Data_d.RefQ = h->RefQ;

	memcpy(&Data_f, &Data_d, sizeof(SearchData));
	memcpy(&Data_b, &Data_d, sizeof(SearchData));
	memcpy(&Data_i, &Data_d, sizeof(SearchData));

	Data_f.iFcode = Data_i.iFcode = frame->fcode;
	Data_b.iFcode = Data_i.bFcode = frame->bcode;

	max_mbs = 0;

	for (j = start_y; j < pParam->mb_height; j += y_step) {
		if (j == 0) max_mbs = pParam->mb_width; /* we can process all blocks of the first row */

		f_predMV = b_predMV = zeroMV;	/* prediction is reset at left boundary */

		for (i = 0; i < pParam->mb_width; i++) {
			MACROBLOCK * const pMB = frame->mbs + i + j * pParam->mb_width;
			const MACROBLOCK * const b_mb = b_mbs + i + j * pParam->mb_width;
			pMB->mode = -1;

			initialize_searchData(&Data_d, &Data_f, &Data_b, &Data_i,
					  i, j, f_ref, f_refH->y, f_refV->y, f_refHV->y,
					  b_ref, b_refH->y, b_refV->y, b_refHV->y,
					  &frame->image, b_mb);

			if (current_mb >= max_mbs) {
				/* we ME-ed all macroblocks we safely could. grab next portion */
				int above_count = *complete_count_above; /* sync point */
				if (above_count == pParam->mb_width) {
					/* full line above is ready */
					above_count = pParam->mb_width+1;
					if (j < pParam->mb_height-y_step) {
						/* this is not last line, grab a portion of MBs from the next line too */
						above_count += MAX(0, complete_count_above[1] - 1);
					}
				}

				max_mbs = current_mb + above_count - i - 1;
				
				if (current_mb >= max_mbs) {
					/* current workload is zero */
					i--;
					sched_yield();
					continue;
				}
			}

/* special case, if collocated block is SKIPed in P-VOP: encoding is forward (0,0), cpb=0 without further ado */
			if (b_reference->coding_type != S_VOP)
				if (b_mb->mode == MODE_NOT_CODED) {
					pMB->mode = MODE_NOT_CODED;
					pMB->mvs[0] = pMB->b_mvs[0] = zeroMV;
					pMB->sad16 = 0;
					*complete_count_self = i+1;
					current_mb++;
					continue;
				}

/* direct search comes first, because it (1) checks for SKIP-mode
	and (2) sets very good predictions for forward and backward search */
			skip_sad = SearchDirect_initial(i, j, frame->motion_flags, TRB, TRD, pParam, pMB, 
											b_mb, &best_sad, &Data_d);

			if (pMB->mode == MODE_DIRECT_NONE_MV) {
				pMB->sad16 = best_sad;
				pMB->cbp = 0;
				*complete_count_self = i+1;
				current_mb++;
				continue;
			}

			SearchBF_initial(i, j, frame->motion_flags, frame->fcode, pParam, pMB,
						&f_predMV, &best_sad, MODE_FORWARD, &Data_f, Data_d.currentMV[1]);

			SearchBF_initial(i, j, frame->motion_flags, frame->bcode, pParam, pMB,
						&b_predMV, &best_sad, MODE_BACKWARD, &Data_b, Data_d.currentMV[2]);

			if (frame->motion_flags&XVID_ME_BFRAME_EARLYSTOP)
				fb_thresh = best_sad;
			else
				fb_thresh = best_sad + (best_sad>>1);

			if (Data_f.iMinSAD[0] <= fb_thresh)
				SearchBF_final(i, j, frame->motion_flags, pParam, &best_sad, &Data_f);

			if (Data_b.iMinSAD[0] <= fb_thresh)
				SearchBF_final(i, j, frame->motion_flags, pParam, &best_sad, &Data_b);

			SearchInterpolate_initial(i, j, frame->motion_flags, pParam, &f_predMV, &b_predMV, &best_sad,
								  &Data_i, Data_f.currentMV[0], Data_b.currentMV[0]);

			if (((Data_i.iMinSAD[0] < best_sad +(best_sad>>3)) && !(frame->motion_flags&XVID_ME_FAST_MODEINTERPOLATE))
				|| Data_i.iMinSAD[0] <= best_sad)

				SearchInterpolate_final(i, j, frame->motion_flags, pParam, &best_sad, &Data_i);
			
			if (Data_d.iMinSAD[0] <= 2*best_sad)
				if ((!(frame->motion_flags&XVID_ME_SKIP_DELTASEARCH) && (best_sad > 750))
					|| (best_sad > 1000))
				
					SearchDirect_final(frame->motion_flags, b_mb, &best_sad, &Data_d);

			/* final skip decision */
			if ( (skip_sad < 2 * Data_d.iQuant * MAX_SAD00_FOR_SKIP )
				&& ((100*best_sad)/(skip_sad+1) > FINAL_SKIP_THRESH) ) {

				Data_d.chromaSAD = 0; /* green light for chroma check */

				SkipDecisionB(pMB, &Data_d);
				
				if (pMB->mode == MODE_DIRECT_NONE_MV) { /* skipped? */
					pMB->sad16 = skip_sad;
					pMB->cbp = 0;
					*complete_count_self = i+1;
					current_mb++;
					continue;
				}
			}

			if (frame->vop_flags & XVID_VOP_RD_BVOP) 
				ModeDecision_BVOP_RD(&Data_d, &Data_b, &Data_f, &Data_i, 
					pMB, b_mb, &f_predMV, &b_predMV, frame->motion_flags, pParam, i, j, best_sad);
			else
				ModeDecision_BVOP_SAD(&Data_d, &Data_b, &Data_f, &Data_i, pMB, b_mb, &f_predMV, &b_predMV);

			*complete_count_self = i+1;
			current_mb++;
			maxMotionBVOP(&MVmaxF, &MVmaxB, pMB, Data_d.qpel);
		}

		complete_count_self++;
		complete_count_above++;
	}

	h->minfcode = getMinFcode(MVmaxF);
	h->minbcode = getMinFcode(MVmaxB);
}
