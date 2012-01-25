/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Motion Estimation for P- and S- VOPs  -
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
 * $Id: estimation_pvop.c,v 1.22 2006/04/19 15:42:19 syskin Exp $
 *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* memcpy */

#include "../encoder.h"
#include "../prediction/mbprediction.h"
#include "../global.h"
#include "../utils/timer.h"
#include "../image/interpolate8x8.h"
#include "estimation.h"
#include "motion.h"
#include "sad.h"
#include "motion_inlines.h"
#include "motion_smp.h"


static const int xvid_me_lambda_vec8[32] =
	{     0    ,(int)(1.0 * NEIGH_TEND_8X8 + 0.5),
	(int)(2.0*NEIGH_TEND_8X8 + 0.5), (int)(3.0*NEIGH_TEND_8X8 + 0.5),
	(int)(4.0*NEIGH_TEND_8X8 + 0.5), (int)(5.0*NEIGH_TEND_8X8 + 0.5),
	(int)(6.0*NEIGH_TEND_8X8 + 0.5), (int)(7.0*NEIGH_TEND_8X8 + 0.5),
	(int)(8.0*NEIGH_TEND_8X8 + 0.5), (int)(9.0*NEIGH_TEND_8X8 + 0.5),
	(int)(10.0*NEIGH_TEND_8X8 + 0.5), (int)(11.0*NEIGH_TEND_8X8 + 0.5),
	(int)(12.0*NEIGH_TEND_8X8 + 0.5), (int)(13.0*NEIGH_TEND_8X8 + 0.5),
	(int)(14.0*NEIGH_TEND_8X8 + 0.5), (int)(15.0*NEIGH_TEND_8X8 + 0.5),
	(int)(16.0*NEIGH_TEND_8X8 + 0.5), (int)(17.0*NEIGH_TEND_8X8 + 0.5),
	(int)(18.0*NEIGH_TEND_8X8 + 0.5), (int)(19.0*NEIGH_TEND_8X8 + 0.5),
	(int)(20.0*NEIGH_TEND_8X8 + 0.5), (int)(21.0*NEIGH_TEND_8X8 + 0.5),
	(int)(22.0*NEIGH_TEND_8X8 + 0.5), (int)(23.0*NEIGH_TEND_8X8 + 0.5),
	(int)(24.0*NEIGH_TEND_8X8 + 0.5), (int)(25.0*NEIGH_TEND_8X8 + 0.5),
	(int)(26.0*NEIGH_TEND_8X8 + 0.5), (int)(27.0*NEIGH_TEND_8X8 + 0.5),
	(int)(28.0*NEIGH_TEND_8X8 + 0.5), (int)(29.0*NEIGH_TEND_8X8 + 0.5),
	(int)(30.0*NEIGH_TEND_8X8 + 0.5), (int)(31.0*NEIGH_TEND_8X8 + 0.5)
};

static void
CheckCandidate16(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	const uint8_t * Reference;
	int32_t sad, xc, yc; uint32_t t;
	VECTOR * current;

	if ( (x > data->max_dx) || (x < data->min_dx)
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

	sad = sad16v(data->Cur, Reference, data->iEdgedWidth, data->temp);
	t = d_mv_bits(x, y, data->predMV, data->iFcode, data->qpel^data->qpel_precision);

	sad += (data->lambda16 * t);
	data->temp[0] += (data->lambda8 * t);

	if (data->chroma) {
		if (sad >= data->iMinSAD[0]) goto no16;
		sad += xvid_me_ChromaSAD((xc >> 1) + roundtab_79[xc & 0x3],
								(yc >> 1) + roundtab_79[yc & 0x3], data);
	}

	if (sad < data->iMinSAD[0]) {
		data->iMinSAD[0] = sad;
		current[0].x = x; current[0].y = y;
		data->dir = Direction;
	}

no16:
	if (data->temp[0] < data->iMinSAD[1]) {
		data->iMinSAD[1] = data->temp[0]; current[1].x = x; current[1].y = y; }
	if (data->temp[1] < data->iMinSAD[2]) {
		data->iMinSAD[2] = data->temp[1]; current[2].x = x; current[2].y = y; }
	if (data->temp[2] < data->iMinSAD[3]) {
		data->iMinSAD[3] = data->temp[2]; current[3].x = x; current[3].y = y; }
	if (data->temp[3] < data->iMinSAD[4]) {
		data->iMinSAD[4] = data->temp[3]; current[4].x = x; current[4].y = y; }
}

static void
CheckCandidate8(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t sad; uint32_t t;
	const uint8_t * Reference;
	VECTOR * current;

	if ( (x > data->max_dx) || (x < data->min_dx)
		|| (y > data->max_dy) || (y < data->min_dy) ) return;

	if (!data->qpel_precision) {
		Reference = GetReference(x, y, data);
		current = data->currentMV;
	} else { /* x and y are in 1/4 precision */
		Reference = xvid_me_interpolate8x8qpel(x, y, 0, 0, data);
		current = data->currentQMV;
	}

	sad = sad8(data->Cur, Reference, data->iEdgedWidth);
	t = d_mv_bits(x, y, data->predMV, data->iFcode, data->qpel^data->qpel_precision);

	sad += (data->lambda8 * t);

	if (sad < *(data->iMinSAD)) {
		*(data->iMinSAD) = sad;
		current->x = x; current->y = y;
		data->dir = Direction;
	}
}

int
xvid_me_SkipDecisionP(const IMAGE * current, const IMAGE * reference,
							const int x, const int y,
							const uint32_t stride, const uint32_t iQuant)
{
	int offset = (x + y*stride)*8;
	uint32_t sadC = sad8(current->u + offset,
					reference->u + offset, stride);
	if (sadC > iQuant * MAX_CHROMA_SAD_FOR_SKIP) return 0;
	sadC += sad8(current->v + offset,
					reference->v + offset, stride);
	if (sadC > iQuant * MAX_CHROMA_SAD_FOR_SKIP) return 0;
	return 1;
}

	/*
	 * pmv are filled with:
	 *  [0]: Median (or whatever is correct in a special case)
	 *  [1]: left neighbour
	 *  [2]: top neighbour
	 *  [3]: topright neighbour
	 * psad are filled with:
	 *  [0]: minimum of [1] to [3]
	 *  [1]: left neighbour's SAD (NB:[1] to [3] are actually not needed)
	 *  [2]: top neighbour's SAD
	 *  [3]: topright neighbour's SAD
	 */

static __inline void
get_pmvdata2(const MACROBLOCK * const mbs,
		const int mb_width,
		const int bound,
		const int x,
		const int y,
		VECTOR * const pmv,
		int32_t * const psad)
{
	int lx, ly, lz;		/* left */
	int tx, ty, tz;		/* top */
	int rx, ry, rz;		/* top-right */
	int lpos, tpos, rpos;
	int num_cand = 0, last_cand = 1;

	lx = x - 1;	ly = y;		lz = 1;
	tx = x;		ty = y - 1;	tz = 2;
	rx = x + 1;	ry = y - 1;	rz = 2;

	lpos = lx + ly * mb_width;
	rpos = rx + ry * mb_width;
	tpos = tx + ty * mb_width;

	if (lpos >= bound && lx >= 0) {
		num_cand++;
		last_cand = 1;
		pmv[1] = mbs[lpos].mvs[lz];
		psad[1] = mbs[lpos].sad8[lz];
	} else {
		pmv[1] = zeroMV;
		psad[1] = MV_MAX_ERROR;
	}

	if (tpos >= bound) {
		num_cand++;
		last_cand = 2;
		pmv[2]= mbs[tpos].mvs[tz];
		psad[2] = mbs[tpos].sad8[tz];
	} else {
		pmv[2] = zeroMV;
		psad[2] = MV_MAX_ERROR;
	}

	if (rpos >= bound && rx < mb_width) {
		num_cand++;
		last_cand = 3;
		pmv[3] = mbs[rpos].mvs[rz];
		psad[3] = mbs[rpos].sad8[rz];
	} else {
		pmv[3] = zeroMV;
		psad[3] = MV_MAX_ERROR;
	}

	/* original pmvdata() compatibility hack */
	if (x == 0 && y == 0) {
		pmv[0] = pmv[1] = pmv[2] = pmv[3] = zeroMV;
		psad[0] = 0;
		psad[1] = psad[2] = psad[3] = MV_MAX_ERROR;
		return;
	}

	/* if only one valid candidate preictor, the invalid candiates are set to the canidate */
	if (num_cand == 1) {
		pmv[0] = pmv[last_cand];
		psad[0] = psad[last_cand];
		return;
	}

	if ((MVequal(pmv[1], pmv[2])) && (MVequal(pmv[1], pmv[3]))) {
		pmv[0] = pmv[1];
		psad[0] = MIN(MIN(psad[1], psad[2]), psad[3]);
		return;
	}

	/* set median, minimum */

	pmv[0].x =
		MIN(MAX(pmv[1].x, pmv[2].x),
			MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
	pmv[0].y =
		MIN(MAX(pmv[1].y, pmv[2].y),
			MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));

	psad[0] = MIN(MIN(psad[1], psad[2]), psad[3]);

}


static void
ModeDecision_SAD(SearchData * const Data,
				MACROBLOCK * const pMB,
				const MACROBLOCK * const pMBs,
				const int x, const int y,
				const MBParam * const pParam,
				const uint32_t MotionFlags,
				const uint32_t VopFlags,
				const uint32_t VolFlags,
				const IMAGE * const pCurrent,
				const IMAGE * const pRef,
				const IMAGE * const vGMC,
				const int coding_type,
				const int skip_sad)
{
	int mode = MODE_INTER;
	int mcsel = 0;
	int inter4v = (VopFlags & XVID_VOP_INTER4V) && (pMB->dquant == 0);
	const uint32_t iQuant = pMB->quant;

	const int skip_possible = (coding_type == P_VOP) && (pMB->dquant == 0);

	int sad;
	int InterBias = MV16_INTER_BIAS;

	pMB->mcsel = 0;

	if (inter4v == 0 || Data->iMinSAD[0] < Data->iMinSAD[1] + Data->iMinSAD[2] +
		Data->iMinSAD[3] + Data->iMinSAD[4] + IMV16X16 * (int32_t)iQuant) {
		mode = MODE_INTER;
		sad = Data->iMinSAD[0];
	} else {
		mode = MODE_INTER4V;
		sad = Data->iMinSAD[1] + Data->iMinSAD[2] +
					Data->iMinSAD[3] + Data->iMinSAD[4] + IMV16X16 * (int32_t)iQuant;
		Data->iMinSAD[0] = sad;
	}

	/* final skip decision, a.k.a. "the vector you found, really that good?" */
	if (skip_possible && (skip_sad < (int)iQuant * MAX_SAD00_FOR_SKIP))
		if ( (100*skip_sad)/(pMB->sad16+1) < FINAL_SKIP_THRESH)
			if (Data->chroma || xvid_me_SkipDecisionP(pCurrent, pRef, x, y, Data->iEdgedWidth/2, iQuant)) {
				mode = MODE_NOT_CODED;
				sad = 0;
			}

	/* mcsel */
	if (coding_type == S_VOP) {

		int32_t iSAD = sad16(Data->Cur,
			vGMC->y + 16*y*Data->iEdgedWidth + 16*x, Data->iEdgedWidth, 65536);

		if (Data->chroma) {
			iSAD += sad8(Data->CurU, vGMC->u + 8*y*(Data->iEdgedWidth/2) + 8*x, Data->iEdgedWidth/2);
			iSAD += sad8(Data->CurV, vGMC->v + 8*y*(Data->iEdgedWidth/2) + 8*x, Data->iEdgedWidth/2);
		}

		if (iSAD <= sad) {		/* mode decision GMC */
			mode = MODE_INTER;
			mcsel = 1;
			sad = iSAD;
		}
	}

	/* intra decision */

	if (iQuant > 10) InterBias += 60 * (iQuant - 10); /* to make high quants work */
	if (y != 0)
		if ((pMB - pParam->mb_width)->mode == MODE_INTRA ) InterBias -= 80;
	if (x != 0)
		if ((pMB - 1)->mode == MODE_INTRA ) InterBias -= 80;

	if (Data->chroma) InterBias += 50; /* dev8(chroma) ??? <-- yes, we need dev8 (no big difference though) */

	if (InterBias < sad) {
		int32_t deviation = dev16(Data->Cur, Data->iEdgedWidth);
		if (deviation < (sad - InterBias)) mode = MODE_INTRA;
	}

	pMB->cbp = 63;
	pMB->sad16 = pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] = sad;

	if (mode == MODE_INTER && mcsel == 0) {
		pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] = pMB->mvs[3] = Data->currentMV[0];

		if(Data->qpel) {
			pMB->qmvs[0] = pMB->qmvs[1]
				= pMB->qmvs[2] = pMB->qmvs[3] = Data->currentQMV[0];
			pMB->pmvs[0].x = Data->currentQMV[0].x - Data->predMV.x;
			pMB->pmvs[0].y = Data->currentQMV[0].y - Data->predMV.y;
		} else {
			pMB->pmvs[0].x = Data->currentMV[0].x - Data->predMV.x;
			pMB->pmvs[0].y = Data->currentMV[0].y - Data->predMV.y;
		}

	} else if (mode == MODE_INTER ) { /* but mcsel == 1 */

		pMB->mcsel = 1;
		if (Data->qpel) {
			pMB->qmvs[0] = pMB->qmvs[1] = pMB->qmvs[2] = pMB->qmvs[3] = pMB->amv;
			pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = pMB->amv.x/2;
			pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = pMB->amv.y/2;
		} else
			pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] = pMB->mvs[3] = pMB->amv;

	} else
		if (mode == MODE_INTER4V) ; /* anything here? */
	else	/* INTRA, NOT_CODED */
		ZeroMacroblockP(pMB, 0);

	pMB->mode = mode;
}

static __inline void
PreparePredictionsP(VECTOR * const pmv, int x, int y, int iWcount,
			int iHcount, const MACROBLOCK * const prevMB)
{

	if ( (y != 0) && (x < (iWcount-1)) ) {		/* [5] top-right neighbour */
		pmv[5].x = EVEN(pmv[3].x);
		pmv[5].y = EVEN(pmv[3].y);
	} else pmv[5].x = pmv[5].y = 0;

	if (x != 0) { pmv[3].x = EVEN(pmv[1].x); pmv[3].y = EVEN(pmv[1].y); }/* pmv[3] is left neighbour */
	else pmv[3].x = pmv[3].y = 0;

	if (y != 0) { pmv[4].x = EVEN(pmv[2].x); pmv[4].y = EVEN(pmv[2].y); }/* [4] top neighbour */
	else pmv[4].x = pmv[4].y = 0;

	/* [1] median prediction */
	pmv[1].x = EVEN(pmv[0].x); pmv[1].y = EVEN(pmv[0].y);

	pmv[0].x = pmv[0].y = 0; /* [0] is zero; not used in the loop (checked before) but needed here for make_mask */

	pmv[2].x = EVEN(prevMB->mvs[0].x); /* [2] is last frame */
	pmv[2].y = EVEN(prevMB->mvs[0].y);

	if ((x < iWcount-1) && (y < iHcount-1)) {
		pmv[6].x = EVEN((prevMB+1+iWcount)->mvs[0].x); /* [6] right-down neighbour in last frame */
		pmv[6].y = EVEN((prevMB+1+iWcount)->mvs[0].y);
	} else pmv[6].x = pmv[6].y = 0;
}

static void
Search8(SearchData * const OldData,
		const int x, const int y,
		const uint32_t MotionFlags,
		const MBParam * const pParam,
		MACROBLOCK * const pMB,
		const MACROBLOCK * const pMBs,
		const int block,
		SearchData * const Data)
{
	int i = 0;
	VECTOR vbest_q; int32_t sbest_q;
	*Data->iMinSAD = *(OldData->iMinSAD + 1 + block);
	*Data->currentMV = *(OldData->currentMV + 1 + block);
	*Data->currentQMV = *(OldData->currentQMV + 1 + block);

	if(Data->qpel) {
		Data->predMV = get_qpmv2(pMBs, pParam->mb_width, 0, x/2, y/2, block);
		if (block != 0)	i = d_mv_bits(	Data->currentQMV->x, Data->currentQMV->y,
										Data->predMV, Data->iFcode, 0);
	} else {
		Data->predMV = get_pmv2(pMBs, pParam->mb_width, 0, x/2, y/2, block);
		if (block != 0)	i = d_mv_bits(	Data->currentMV->x, Data->currentMV->y,
										Data->predMV, Data->iFcode, 0);
	}

	*(Data->iMinSAD) += (Data->lambda8 * i);

	if (MotionFlags & (XVID_ME_EXTSEARCH8|XVID_ME_HALFPELREFINE8|XVID_ME_QUARTERPELREFINE8)) {

		vbest_q = Data->currentQMV[0];
		sbest_q = Data->iMinSAD[0];


		Data->RefP[0] = OldData->RefP[0] + 8 * ((block&1) + Data->iEdgedWidth*(block>>1));
		Data->RefP[1] = OldData->RefP[1] + 8 * ((block&1) + Data->iEdgedWidth*(block>>1));
		Data->RefP[2] = OldData->RefP[2] + 8 * ((block&1) + Data->iEdgedWidth*(block>>1));
		Data->RefP[3] = OldData->RefP[3] + 8 * ((block&1) + Data->iEdgedWidth*(block>>1));

		Data->Cur = OldData->Cur + 8 * ((block&1) + Data->iEdgedWidth*(block>>1));
		Data->qpel_precision = 0;

		get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 3,
					pParam->width, pParam->height, Data->iFcode - Data->qpel, 1);

		if (MotionFlags & XVID_ME_EXTSEARCH8 && (!(MotionFlags & XVID_ME_EXTSEARCH_RD))) {

			MainSearchFunc *MainSearchPtr;
			if (MotionFlags & XVID_ME_USESQUARES8) MainSearchPtr = xvid_me_SquareSearch;
				else if (MotionFlags & XVID_ME_ADVANCEDDIAMOND8) MainSearchPtr = xvid_me_AdvDiamondSearch;
					else MainSearchPtr = xvid_me_DiamondSearch;

			MainSearchPtr(Data->currentMV->x, Data->currentMV->y, Data, 255, CheckCandidate8);
		}

		if(!Data->qpel) {
			/* halfpel mode */
			if (MotionFlags & XVID_ME_HALFPELREFINE8)
				/* perform halfpel refine of current best vector */
				xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate8, 0);
		} else {
			/* qpel mode */
			Data->currentQMV->x = 2*Data->currentMV->x;
			Data->currentQMV->y = 2*Data->currentMV->y;
			
			if(MotionFlags & XVID_ME_FASTREFINE8) {
				/* fast */
				get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 3,
					pParam->width, pParam->height, Data->iFcode, 2);
				FullRefine_Fast(Data, CheckCandidate8, 0);
			} else if(MotionFlags & XVID_ME_QUARTERPELREFINE8) {
				/* full */
				if (MotionFlags & XVID_ME_HALFPELREFINE8) {
					xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate8, 0); /* hpel part */
					Data->currentQMV->x = 2*Data->currentMV->x;
					Data->currentQMV->y = 2*Data->currentMV->y;
				}

				get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 3,
					pParam->width, pParam->height, Data->iFcode, 2);
				Data->qpel_precision = 1;

				xvid_me_SubpelRefine(Data->currentQMV[0], Data, CheckCandidate8, 0); /* qpel part */
			}
		}

		if (sbest_q <= Data->iMinSAD[0]) /* we have not found a better match */
			Data->currentQMV[0] = vbest_q;

	}

	if(Data->qpel) {		
		pMB->pmvs[block].x = Data->currentQMV->x - Data->predMV.x;
		pMB->pmvs[block].y = Data->currentQMV->y - Data->predMV.y;
		pMB->qmvs[block] = *Data->currentQMV;
	} else {
		pMB->pmvs[block].x = Data->currentMV->x - Data->predMV.x;
		pMB->pmvs[block].y = Data->currentMV->y - Data->predMV.y;
	}

	*(OldData->iMinSAD + 1 + block) = *Data->iMinSAD;
	*(OldData->currentMV + 1 + block) = *Data->currentMV;
	*(OldData->currentQMV + 1 + block) = *Data->currentQMV;

	pMB->mvs[block] = *Data->currentMV;
	pMB->sad8[block] = 4 * *Data->iMinSAD;
}



static void
SearchP(const IMAGE * const pRef,
		const uint8_t * const pRefH,
		const uint8_t * const pRefV,
		const uint8_t * const pRefHV,
		const IMAGE * const pCur,
		const int x,
		const int y,
		const uint32_t MotionFlags,
		const uint32_t VopFlags,
		SearchData * const Data,
		const MBParam * const pParam,
		const MACROBLOCK * const pMBs,
		const MACROBLOCK * const prevMBs,
		MACROBLOCK * const pMB)
{

	int i, threshA;
	VECTOR pmv[7];
	int inter4v = (VopFlags & XVID_VOP_INTER4V) && (pMB->dquant == 0);
	CheckFunc * CheckCandidate;

	get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
						pParam->width, pParam->height, Data->iFcode - Data->qpel, 1);

	get_pmvdata2(pMBs, pParam->mb_width, 0, x, y, pmv, Data->temp);

	Data->chromaX = Data->chromaY = 0; /* chroma-sad cache */
	Data->Cur = pCur->y + (x + y * Data->iEdgedWidth) * 16;
	Data->CurV = pCur->v + (x + y * (Data->iEdgedWidth/2)) * 8;
	Data->CurU = pCur->u + (x + y * (Data->iEdgedWidth/2)) * 8;

	Data->RefP[0] = pRef->y + (x + Data->iEdgedWidth*y) * 16;
	Data->RefP[2] = pRefH + (x + Data->iEdgedWidth*y) * 16;
	Data->RefP[1] = pRefV + (x + Data->iEdgedWidth*y) * 16;
	Data->RefP[3] = pRefHV + (x + Data->iEdgedWidth*y) * 16;
	Data->RefP[4] = pRef->u + (x + y * (Data->iEdgedWidth/2)) * 8;
	Data->RefP[5] = pRef->v + (x + y * (Data->iEdgedWidth/2)) * 8;

	Data->lambda16 = xvid_me_lambda_vec16[pMB->quant];
	Data->lambda8 = xvid_me_lambda_vec8[pMB->quant];
	Data->qpel_precision = 0;
	Data->dir = 0;

	memset(Data->currentMV, 0, 5*sizeof(VECTOR));

	if (Data->qpel) Data->predMV = get_qpmv2(pMBs, pParam->mb_width, 0, x, y, 0);
	else Data->predMV = pmv[0];

	i = d_mv_bits(0, 0, Data->predMV, Data->iFcode, 0);
	Data->iMinSAD[0] = pMB->sad16 + (Data->lambda16 * i);
	Data->iMinSAD[1] = pMB->sad8[0] + (Data->lambda8 * i);
	Data->iMinSAD[2] = pMB->sad8[1];
	Data->iMinSAD[3] = pMB->sad8[2];
	Data->iMinSAD[4] = pMB->sad8[3];

	if ((!(VopFlags & XVID_VOP_MODEDECISION_RD)) && (x | y)) {
		threshA = Data->temp[0]; /* that's where we keep this SAD atm */
		if (threshA < 512) threshA = 512;
		else if (threshA > 1024) threshA = 1024;
	} else
		threshA = 512;

	PreparePredictionsP(pmv, x, y, pParam->mb_width, pParam->mb_height,
					prevMBs + x + y * pParam->mb_width);

	if (inter4v) CheckCandidate = CheckCandidate16;
	else CheckCandidate = CheckCandidate16no4v; /* for extra speed */

/* main loop. checking all predictions (but first, which is 0,0 and has been checked in MotionEstimation())*/

	for (i = 1; i < 7; i++)
		if (!vector_repeats(pmv, i)) {
			CheckCandidate(pmv[i].x, pmv[i].y, Data, i);
			if (Data->iMinSAD[0] <= threshA) { i++; break; }
		}

	if ((Data->iMinSAD[0] <= threshA) ||
			(MVequal(Data->currentMV[0], (prevMBs+x+y*pParam->mb_width)->mvs[0]) &&
			(Data->iMinSAD[0] < (prevMBs+x+y*pParam->mb_width)->sad16)))
		inter4v = 0;
	else {

		MainSearchFunc * MainSearchPtr;
		int mask = make_mask(pmv, i, Data->dir); /* all vectors pmv[0..i-1] have been checked */

		if (MotionFlags & XVID_ME_USESQUARES16) MainSearchPtr = xvid_me_SquareSearch;
		else if (MotionFlags & XVID_ME_ADVANCEDDIAMOND16) MainSearchPtr = xvid_me_AdvDiamondSearch;
			else MainSearchPtr = xvid_me_DiamondSearch;

		MainSearchPtr(Data->currentMV->x, Data->currentMV->y, Data, mask, CheckCandidate);

/* extended search, diamond starting in 0,0 and in prediction.
	note that this search is/might be done in halfpel positions,
	which makes it more different than the diamond above */

		if (MotionFlags & XVID_ME_EXTSEARCH16) {
			int32_t bSAD;
			VECTOR startMV = Data->predMV, backupMV = Data->currentMV[0];
			if (Data->qpel) {
				startMV.x /= 2;
				startMV.y /= 2;
			}
			if (!(MVequal(startMV, backupMV))) {
				bSAD = Data->iMinSAD[0]; Data->iMinSAD[0] = MV_MAX_ERROR;

				CheckCandidate(startMV.x, startMV.y, Data, 255);
				xvid_me_DiamondSearch(startMV.x, startMV.y, Data, 255, CheckCandidate);
				if (bSAD < Data->iMinSAD[0]) {
					Data->currentMV[0] = backupMV;
					Data->iMinSAD[0] = bSAD; 
				}
			}

			backupMV = Data->currentMV[0];
			startMV.x = startMV.y = 1;
			if (!(MVequal(startMV, backupMV))) {
				bSAD = Data->iMinSAD[0]; Data->iMinSAD[0] = MV_MAX_ERROR;

				CheckCandidate(startMV.x, startMV.y, Data, 255);
				xvid_me_DiamondSearch(startMV.x, startMV.y, Data, 255, CheckCandidate);
				if (bSAD < Data->iMinSAD[0]) {
					Data->currentMV[0] = backupMV;
					Data->iMinSAD[0] = bSAD;
				}
			}
		}
	}


	if(!Data->qpel) {
		/* halfpel mode */
		if (MotionFlags & XVID_ME_HALFPELREFINE16)
				xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate, 0);	
	} else {
		/* qpel mode */
		
		for(i = 0; i < 5; i++) {
			Data->currentQMV[i].x = 2 * Data->currentMV[i].x; /* initialize qpel vectors */
			Data->currentQMV[i].y = 2 * Data->currentMV[i].y;
		}			
		if(MotionFlags & XVID_ME_FASTREFINE16 && MotionFlags & XVID_ME_QUARTERPELREFINE16) {
			/* fast */
			get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
						pParam->width, pParam->height, Data->iFcode, 2);
			FullRefine_Fast(Data, CheckCandidate, 0);
		} else {
			if(MotionFlags & (XVID_ME_QUARTERPELREFINE16 | XVID_ME_QUARTERPELREFINE16_RD)) {
				/* full */
				if (MotionFlags & XVID_ME_HALFPELREFINE16) {
					xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate, 0); /* hpel part */
					for(i = 0; i < 5; i++) {
						Data->currentQMV[i].x = 2 * Data->currentMV[i].x;
						Data->currentQMV[i].y = 2 * Data->currentMV[i].y;
					}
				}
				get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
							pParam->width, pParam->height, Data->iFcode, 2);
				Data->qpel_precision = 1;
				if(MotionFlags & XVID_ME_QUARTERPELREFINE16)
					xvid_me_SubpelRefine(Data->currentQMV[0], Data, CheckCandidate, 0); /* qpel part */
			}
		}
	}

	if (Data->iMinSAD[0] < (int32_t)pMB->quant * 30 * ((MotionFlags & XVID_ME_FASTREFINE16) ? 8 : 1))
		inter4v = 0;

	if (inter4v) {
		SearchData Data8;
		memcpy(&Data8, Data, sizeof(SearchData)); /* quick copy of common data */

		Search8(Data, 2*x, 2*y, MotionFlags, pParam, pMB, pMBs, 0, &Data8);
		Search8(Data, 2*x + 1, 2*y, MotionFlags, pParam, pMB, pMBs, 1, &Data8);
		Search8(Data, 2*x, 2*y + 1, MotionFlags, pParam, pMB, pMBs, 2, &Data8);
		Search8(Data, 2*x + 1, 2*y + 1, MotionFlags, pParam, pMB, pMBs, 3, &Data8);

		if ((Data->chroma) && (!(VopFlags & XVID_VOP_MODEDECISION_RD))) {
			/* chroma is only used for comparison to INTER. if the comparison will be done in RD domain, it will not be used */
			int sumx = 0, sumy = 0;

			if (Data->qpel)
				for (i = 1; i < 5; i++) {
					sumx += Data->currentQMV[i].x/2;
					sumy += Data->currentQMV[i].y/2;
				}
			else
				for (i = 1; i < 5; i++) {
					sumx += Data->currentMV[i].x;
					sumy += Data->currentMV[i].y;
				}

			Data->iMinSAD[1] += xvid_me_ChromaSAD((sumx >> 3) + roundtab_76[sumx & 0xf],
												(sumy >> 3) + roundtab_76[sumy & 0xf], Data);
		}
	} else Data->iMinSAD[1] = 4096*256;
}

static int
InitialSkipDecisionP(int sad00,
					 const MBParam * pParam,
					 const FRAMEINFO * current,
					 MACROBLOCK * pMB,
					 const MACROBLOCK * prevMB,
					 int x, int y,
					 const SearchData * Data,
					 const IMAGE * const pGMC,
					 const IMAGE * const pCurrent,
					 const IMAGE * const pRef,
					 const uint32_t MotionFlags)
{
	const unsigned int iEdgedWidth = pParam->edged_width;

	int skip_thresh = INITIAL_SKIP_THRESH * \
		(current->vop_flags & XVID_VOP_MODEDECISION_RD ? 2:1);
	int stat_thresh = 0;

	/* initial skip decision */
	if (current->coding_type != S_VOP)	{ /* no fast SKIP for S(GMC)-VOPs */
		if (pMB->dquant == 0 && sad00 < pMB->quant * skip_thresh)
			if (Data->chroma || xvid_me_SkipDecisionP(pCurrent, pRef, x, y, iEdgedWidth/2, pMB->quant)) {
				ZeroMacroblockP(pMB, sad00);
				pMB->mode = MODE_NOT_CODED;
				return 1;
			}
	}

	if(MotionFlags & XVID_ME_DETECT_STATIC_MOTION) {
		VECTOR *cmpMV;
		VECTOR staticMV = { 0, 0 };
		const MACROBLOCK * pMBs = current->mbs;

		if (current->coding_type == S_VOP) 
			cmpMV = &pMB->amv;
		else
			cmpMV = &staticMV;

		if(x > 0 && y > 0 && x < pParam->mb_width) {
			if(MVequal((&pMBs[(x-1) + y * pParam->mb_width])->mvs[0], *cmpMV) &&
			   MVequal((&pMBs[x + (y-1) * pParam->mb_width])->mvs[0], *cmpMV) &&
			   MVequal((&pMBs[(x+1) + (y-1) * pParam->mb_width])->mvs[0], *cmpMV) &&
			   MVequal(prevMB->mvs[0], *cmpMV)) {
				stat_thresh = MAX((&pMBs[(x-1) + y * pParam->mb_width])->sad16,
							  MAX((&pMBs[x + (y-1) * pParam->mb_width])->sad16,
							  MAX((&pMBs[(x+1) + (y-1) * pParam->mb_width])->sad16,
							  prevMB->sad16)));
			} else {
				stat_thresh = MIN((&pMBs[(x-1) + y * pParam->mb_width])->sad16,
							  MIN((&pMBs[x + (y-1) * pParam->mb_width])->sad16,
							  MIN((&pMBs[(x+1) + (y-1) * pParam->mb_width])->sad16,
							  prevMB->sad16)));
			}
		}
	}

	 /* favorize (0,0) or global vector for cartoons */
	if (current->vop_flags & XVID_VOP_CARTOON) {
		if (current->coding_type == S_VOP) {
			int32_t iSAD = sad16(pCurrent->y + (x + y * iEdgedWidth) * 16,
			pGMC->y + 16*y*iEdgedWidth + 16*x, iEdgedWidth, 65536);

			if (Data->chroma) {
				iSAD += sad8(pCurrent->u + x*8 + y*(iEdgedWidth/2)*8, pGMC->u + 8*y*(iEdgedWidth/2) + 8*x, iEdgedWidth/2);
				iSAD += sad8(pCurrent->v + (x + y*(iEdgedWidth/2))*8, pGMC->v + 8*y*(iEdgedWidth/2) + 8*x, iEdgedWidth/2);
			}

			if (iSAD <= stat_thresh) {		/* mode decision GMC */
				pMB->mode = MODE_INTER;
				pMB->sad16 = pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] = iSAD;
				pMB->mcsel = 1;
				if (Data->qpel) {
					pMB->qmvs[0] = pMB->qmvs[1] = pMB->qmvs[2] = pMB->qmvs[3] = pMB->amv;
					pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = pMB->amv.x/2;
					pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = pMB->amv.y/2;
				} else
					pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] = pMB->mvs[3] = pMB->amv;
				
				return 1;
			}
		}
		else if (sad00 < stat_thresh) {
			VECTOR predMV;
			if (Data->qpel)
				predMV = get_qpmv2(current->mbs, pParam->mb_width, 0, x, y, 0);
			else
				predMV = get_pmv2(current->mbs, pParam->mb_width, 0, x, y, 0);

			ZeroMacroblockP(pMB, sad00);
			pMB->cbp = 0x3f;
			pMB->pmvs[0].x = - predMV.x;
			pMB->pmvs[0].y = - predMV.y;
			return 1;
		}
	}

	return 0;
}

static __inline uint32_t
MakeGoodMotionFlags(const uint32_t MotionFlags, const uint32_t VopFlags, const uint32_t VolFlags)
{
	uint32_t Flags = MotionFlags;

	if (!(VopFlags & XVID_VOP_MODEDECISION_RD))
		Flags &= ~(XVID_ME_QUARTERPELREFINE16_RD+XVID_ME_QUARTERPELREFINE8_RD+XVID_ME_HALFPELREFINE16_RD+XVID_ME_HALFPELREFINE8_RD+XVID_ME_EXTSEARCH_RD);

	if (Flags & XVID_ME_EXTSEARCH_RD)
		Flags |= XVID_ME_HALFPELREFINE16_RD;

	if (Flags & XVID_ME_EXTSEARCH_RD && MotionFlags & XVID_ME_EXTSEARCH8)
		Flags |= XVID_ME_HALFPELREFINE8_RD;

	if (Flags & XVID_ME_HALFPELREFINE16_RD)
		Flags |= XVID_ME_QUARTERPELREFINE16_RD;

	if (Flags & XVID_ME_HALFPELREFINE8_RD) {
		Flags |= XVID_ME_QUARTERPELREFINE8_RD;
		Flags &= ~XVID_ME_HALFPELREFINE8;
	}

	if (Flags & XVID_ME_QUARTERPELREFINE8_RD)
		Flags &= ~XVID_ME_QUARTERPELREFINE8;

	if (Flags & XVID_ME_QUARTERPELREFINE16_RD)
		Flags &= ~XVID_ME_QUARTERPELREFINE16;

	if (!(VolFlags & XVID_VOL_QUARTERPEL))
		Flags &= ~(XVID_ME_QUARTERPELREFINE16+XVID_ME_QUARTERPELREFINE8+XVID_ME_QUARTERPELREFINE16_RD+XVID_ME_QUARTERPELREFINE8_RD);

	if (!(VopFlags & XVID_VOP_HALFPEL))
		Flags &= ~(XVID_ME_EXTSEARCH16+XVID_ME_HALFPELREFINE16+XVID_ME_HALFPELREFINE8+XVID_ME_HALFPELREFINE16_RD+XVID_ME_HALFPELREFINE8_RD);

	if (VopFlags & XVID_VOP_GREYSCALE)
		Flags &= ~(XVID_ME_CHROMA_PVOP + XVID_ME_CHROMA_BVOP);

	if (Flags & XVID_ME_FASTREFINE8)
		Flags &= ~XVID_ME_HALFPELREFINE8_RD;

	if (Flags & XVID_ME_FASTREFINE16)
		Flags &= ~XVID_ME_HALFPELREFINE16_RD;

	return Flags;
}

static __inline void
motionStatsPVOP(int * const MVmax, int * const mvCount, int * const mvSum,
				const MACROBLOCK * const pMB, const int qpel)
{
	const VECTOR * const mv = qpel ? pMB->qmvs : pMB->mvs;
	int i;
	int max = *MVmax;

	switch (pMB->mode) {
	case MODE_INTER4V:
		*mvCount += 3;
		for(i = 3; i; i--) {
			if (mv[i].x > max) max = mv[i].x;
			else if (-mv[i].x - 1 > max) max = -mv[i].x - 1;
			*mvSum += mv[i].x * mv[i].x;
			if (mv[i].y > max) max = mv[i].y;
			else if (-mv[i].y - 1 > max) max = -mv[i].y - 1;
			*mvSum += mv[i].y * mv[i].y;
		}
	case MODE_INTER:
		(*mvCount)++;
		*mvSum += mv[0].x * mv[0].x;
		*mvSum += mv[0].y * mv[0].y;
		if (mv[0].x > max) max = mv[0].x;
		else if (-mv[0].x - 1 > max) max = -mv[0].x - 1;
		if (mv[0].y > max) max = mv[0].y;
		else if (-mv[0].y - 1 > max) max = -mv[0].y - 1;
		*MVmax = max;
	default:
		break;
	}
}

void
MotionEstimation(MBParam * const pParam,
				 FRAMEINFO * const current,
				 FRAMEINFO * const reference,
				 const IMAGE * const pRefH,
				 const IMAGE * const pRefV,
				 const IMAGE * const pRefHV,
				 const IMAGE * const pGMC,
				 const uint32_t iLimit)
{
	MACROBLOCK *const pMBs = current->mbs;
	const IMAGE *const pCurrent = &current->image;
	const IMAGE *const pRef = &reference->image;

	const uint32_t mb_width = pParam->mb_width;
	const uint32_t mb_height = pParam->mb_height;
	const uint32_t iEdgedWidth = pParam->edged_width;
	const uint32_t MotionFlags = MakeGoodMotionFlags(current->motion_flags, current->vop_flags, current->vol_flags);
	int stat_thresh = 0;
	int MVmax = 0, mvSum = 0, mvCount = 0;

	uint32_t x, y;
	int sad00;
	int skip_thresh = INITIAL_SKIP_THRESH * \
		(current->vop_flags & XVID_VOP_MODEDECISION_RD ? 2:1);
	int block = 0;

	/* some pre-initialized thingies for SearchP */
	DECLARE_ALIGNED_MATRIX(dct_space, 3, 64, int16_t, CACHE_LINE);
	SearchData Data;
	memset(&Data, 0, sizeof(SearchData));
	Data.iEdgedWidth = iEdgedWidth;
	Data.iFcode = current->fcode;
	Data.rounding = pParam->m_rounding_type;
	Data.qpel = (current->vol_flags & XVID_VOL_QUARTERPEL ? 1:0);
	Data.chroma = MotionFlags & XVID_ME_CHROMA_PVOP;
	Data.dctSpace = dct_space;
	Data.quant_type = !(pParam->vol_flags & XVID_VOL_MPEGQUANT);
	Data.mpeg_quant_matrices = pParam->mpeg_quant_matrices;

	Data.RefQ = pRefV->u; /* a good place, also used in MC (for similar purpose) */
	if (sadInit) (*sadInit) ();

	for (y = 0; y < mb_height; y++)	{
		for (x = 0; x < mb_width; x++)	{
			MACROBLOCK *pMB = &pMBs[block];
			MACROBLOCK *prevMB = &reference->mbs[block];
			int skip;
			block++;

			pMB->sad16 =
				sad16v(pCurrent->y + (x + y * iEdgedWidth) * 16,
							pRef->y + (x + y * iEdgedWidth) * 16,
							pParam->edged_width, pMB->sad8);

			sad00 = 4*MAX(MAX(pMB->sad8[0], pMB->sad8[1]), MAX(pMB->sad8[2], pMB->sad8[3]));

			if (Data.chroma) {
				Data.chromaSAD = sad8(pCurrent->u + x*8 + y*(iEdgedWidth/2)*8,
									pRef->u + x*8 + y*(iEdgedWidth/2)*8, iEdgedWidth/2)
								+ sad8(pCurrent->v + (x + y*(iEdgedWidth/2))*8,
									pRef->v + (x + y*(iEdgedWidth/2))*8, iEdgedWidth/2);
				pMB->sad16 += Data.chromaSAD;
				sad00 += Data.chromaSAD;
			}

			skip = InitialSkipDecisionP(sad00, pParam, current, pMB, prevMB, x, y, &Data, pGMC, 
										pCurrent, pRef, MotionFlags);
			if (skip) continue;

			SearchP(pRef, pRefH->y, pRefV->y, pRefHV->y, pCurrent, x,
					y, MotionFlags, current->vop_flags,
					&Data, pParam, pMBs, reference->mbs, pMB);

			if (current->vop_flags & XVID_VOP_MODEDECISION_RD)
				xvid_me_ModeDecision_RD(&Data, pMB, pMBs, x, y, pParam,
								MotionFlags, current->vop_flags, current->vol_flags,
								pCurrent, pRef, pGMC, current->coding_type);

			else if (current->vop_flags & XVID_VOP_FAST_MODEDECISION_RD)
				xvid_me_ModeDecision_Fast(&Data, pMB, pMBs, x, y, pParam,
								MotionFlags, current->vop_flags, current->vol_flags,
								pCurrent, pRef, pGMC, current->coding_type);
			else
				ModeDecision_SAD(&Data, pMB, pMBs, x, y, pParam,
								MotionFlags, current->vop_flags, current->vol_flags,
								pCurrent, pRef, pGMC, current->coding_type, sad00);


			motionStatsPVOP(&MVmax, &mvCount, &mvSum, pMB, Data.qpel);
		}
	}

	current->fcode = getMinFcode(MVmax);
	current->sStat.iMvSum = mvSum;
	current->sStat.iMvCount = mvCount;
}

void
MotionEstimateSMP(SMPmotionData * h)
{
	const MBParam * const pParam = h->pParam;
	const FRAMEINFO * const current = h->current;
	const FRAMEINFO * const reference = h->reference;
	const IMAGE * const pRefH = h->pRefH;
	const IMAGE * const pRefV = h->pRefV;
	const IMAGE * const pRefHV = h->pRefHV;
	const IMAGE * const pGMC = h->pGMC;
	uint32_t MotionFlags = MakeGoodMotionFlags(current->motion_flags,
												current->vop_flags,
												current->vol_flags);

	MACROBLOCK *const pMBs = current->mbs;
	const IMAGE *const pCurrent = &current->image;
	const IMAGE *const pRef = &reference->image;

	const uint32_t mb_width = pParam->mb_width;
	const uint32_t mb_height = pParam->mb_height;
	const uint32_t iEdgedWidth = pParam->edged_width;
	int stat_thresh = 0;
	int MVmax = 0, mvSum = 0, mvCount = 0;
	int y_step = h->y_step;
	int start_y = h->start_y;

	uint32_t x, y;
	int sad00;
	int skip_thresh = INITIAL_SKIP_THRESH * \
		(current->vop_flags & XVID_VOP_MODEDECISION_RD ? 2:1);
	int block = start_y*mb_width;
	int * complete_count_self = h->complete_count_self;
	const volatile int * complete_count_above = h->complete_count_above;
	int max_mbs;
	int current_mb = 0;

	/* some pre-initialized thingies for SearchP */
	DECLARE_ALIGNED_MATRIX(dct_space, 3, 64, int16_t, CACHE_LINE);
	SearchData Data;
	memset(&Data, 0, sizeof(SearchData));
	Data.iEdgedWidth = iEdgedWidth;
	Data.iFcode = current->fcode;
	Data.rounding = pParam->m_rounding_type;
	Data.qpel = (current->vol_flags & XVID_VOL_QUARTERPEL ? 1:0);
	Data.chroma = MotionFlags & XVID_ME_CHROMA_PVOP;
	Data.dctSpace = dct_space;
	Data.quant_type = !(pParam->vol_flags & XVID_VOL_MPEGQUANT);
	Data.mpeg_quant_matrices = pParam->mpeg_quant_matrices;

	/* todo: sort out temp memory space */
	Data.RefQ = h->RefQ;
	if (sadInit) (*sadInit) ();

	max_mbs = 0;

	for (y = start_y; y < mb_height; y += y_step)	{
		if (y == 0) max_mbs = mb_width; /* we can process all blocks of the first row */

		for (x = 0; x < mb_width; x++)	{
			
			MACROBLOCK *pMB, *prevMB;
			int skip;

			if (current_mb >= max_mbs) {
				/* we ME-ed all macroblocks we safely could. grab next portion */
				int above_count = *complete_count_above; /* sync point */
				if (above_count == mb_width) {
					/* full line above is ready */
					above_count = mb_width+1;
					if (y < mb_height-y_step) {
						/* this is not last line, grab a portion of MBs from the next line too */
						above_count += MAX(0, complete_count_above[1] - 1);
					}
				}

				max_mbs = current_mb + above_count - x - 1;
				
				if (current_mb >= max_mbs) {
					/* current workload is zero */
					x--;
					sched_yield();
					continue;
				}
			}


			pMB = &pMBs[block];
			prevMB = &reference->mbs[block];

			pMB->sad16 =
				sad16v(pCurrent->y + (x + y * iEdgedWidth) * 16,
							pRef->y + (x + y * iEdgedWidth) * 16,
							pParam->edged_width, pMB->sad8);

			sad00 = 4*MAX(MAX(pMB->sad8[0], pMB->sad8[1]), MAX(pMB->sad8[2], pMB->sad8[3]));

			if (Data.chroma) {
				Data.chromaSAD = sad8(pCurrent->u + x*8 + y*(iEdgedWidth/2)*8,
									pRef->u + x*8 + y*(iEdgedWidth/2)*8, iEdgedWidth/2)
								+ sad8(pCurrent->v + (x + y*(iEdgedWidth/2))*8,
									pRef->v + (x + y*(iEdgedWidth/2))*8, iEdgedWidth/2);
				pMB->sad16 += Data.chromaSAD;
				sad00 += Data.chromaSAD;
			}

			skip = InitialSkipDecisionP(sad00, pParam, current, pMB, prevMB, x, y, &Data, pGMC, 
										pCurrent, pRef, MotionFlags);

			if (skip) {
				current_mb++;
				block++;
				*complete_count_self = x+1;
				continue;
			}

			SearchP(pRef, pRefH->y, pRefV->y, pRefHV->y, pCurrent, x,
					y, MotionFlags, current->vop_flags,
					&Data, pParam, pMBs, reference->mbs, pMB);

			if (current->vop_flags & XVID_VOP_MODEDECISION_RD)
				xvid_me_ModeDecision_RD(&Data, pMB, pMBs, x, y, pParam,
								MotionFlags, current->vop_flags, current->vol_flags,
								pCurrent, pRef, pGMC, current->coding_type);

			else if (current->vop_flags & XVID_VOP_FAST_MODEDECISION_RD)
				xvid_me_ModeDecision_Fast(&Data, pMB, pMBs, x, y, pParam,
								MotionFlags, current->vop_flags, current->vol_flags,
								pCurrent, pRef, pGMC, current->coding_type);
			else
				ModeDecision_SAD(&Data, pMB, pMBs, x, y, pParam,
								MotionFlags, current->vop_flags, current->vol_flags,
								pCurrent, pRef, pGMC, current->coding_type, sad00);

			*complete_count_self = x+1;

			current_mb++;
			block++;

			motionStatsPVOP(&MVmax, &mvCount, &mvSum, pMB, Data.qpel);
			
		}
		block += (y_step-1)*pParam->mb_width;
		complete_count_self++;
		complete_count_above++;
	}

	h->minfcode = getMinFcode(MVmax);

	h->MVmax = MVmax;
	h->mvSum = mvSum;
	h->mvCount = mvCount;
}


