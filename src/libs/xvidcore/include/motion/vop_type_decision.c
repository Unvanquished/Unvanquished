/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - ME-based Frame Type Decision -
 *
 *  Copyright(C) 2002-2003 Radoslaw Czyz <xvid@syskin.cjb.net>
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
 * $Id: vop_type_decision.c,v 1.5 2004/12/05 04:53:01 syskin Exp $
 *
 ****************************************************************************/

#include "../encoder.h"
#include "../prediction/mbprediction.h"
#include "estimation.h"
#include "motion.h"
#include "sad.h"
#include "gmc.h"
#include "../utils/emms.h"
#include "motion_inlines.h"


#define INTRA_THRESH	2000
#define INTER_THRESH	40
#define INTRA_THRESH2	90

/* when we are in 1/I_SENS_TH before forced keyframe, we start to decrese i-frame threshold */
#define I_SENS_TH		3 

/* how much we subtract from each p-frame threshold for 2nd, 3rd etc. b-frame in a row */
#define P_SENS_BIAS		18

/* .. but never below INTER_THRESH_MIN */
#define INTER_THRESH_MIN 5

static void
CheckCandidate32I(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	/* maximum speed */
	int32_t sad;

	if ( (x > data->max_dx) || (x < data->min_dx)
		|| (y > data->max_dy) || (y < data->min_dy) ) return;

	sad = sad32v_c(data->Cur, data->RefP[0] + x + y*((int)data->iEdgedWidth),
					data->iEdgedWidth, data->temp);

	if (sad < *(data->iMinSAD)) {
		*(data->iMinSAD) = sad;
		data->currentMV[0].x = x; data->currentMV[0].y = y;
		data->dir = Direction;
	}
	if (data->temp[0] < data->iMinSAD[1]) {
		data->iMinSAD[1] = data->temp[0]; data->currentMV[1].x = x; data->currentMV[1].y = y; }
	if (data->temp[1] < data->iMinSAD[2]) {
		data->iMinSAD[2] = data->temp[1]; data->currentMV[2].x = x; data->currentMV[2].y = y; }
	if (data->temp[2] < data->iMinSAD[3]) {
		data->iMinSAD[3] = data->temp[2]; data->currentMV[3].x = x; data->currentMV[3].y = y; }
	if (data->temp[3] < data->iMinSAD[4]) {
		data->iMinSAD[4] = data->temp[3]; data->currentMV[4].x = x; data->currentMV[4].y = y; }
}

static __inline void
MEanalyzeMB (	const uint8_t * const pRef,
				const uint8_t * const pCur,
				const int x,
				const int y,
				const MBParam * const pParam,
				MACROBLOCK * const pMBs,
				SearchData * const Data)
{

	int i;
	VECTOR pmv[3];
	MACROBLOCK * const pMB = &pMBs[x + y * pParam->mb_width];

	unsigned int simplicity = 0;

	for (i = 0; i < 5; i++) Data->iMinSAD[i] = MV_MAX_ERROR;

	get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
			pParam->width, pParam->height, Data->iFcode - Data->qpel - 1, 0);

	Data->Cur = pCur + (x + y * pParam->edged_width) * 16;
	Data->RefP[0] = pRef + (x + y * pParam->edged_width) * 16;

	pmv[0].x = pMB->mvs[0].x;
	pmv[0].y = pMB->mvs[0].y;

	CheckCandidate32I(pmv[0].x, pmv[0].y, Data, 0);

	if (*Data->iMinSAD > 200) {

		pmv[1].x = pmv[1].y = 0;

		/* median is only used as prediction. it doesn't have to be real */
		if (x == 1 && y == 1) Data->predMV.x = Data->predMV.y = 0;
		else
			if (x == 1) /* left macroblock does not have any vector now */
				Data->predMV = (pMB - pParam->mb_width)->mvs[0]; /* top instead of median */
			else if (y == 1) /* top macroblock doesn't have it's vector */
				Data->predMV = (pMB - 1)->mvs[0]; /* left instead of median */
			else Data->predMV = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 0); /* else median */

		pmv[2].x = Data->predMV.x;
		pmv[2].y = Data->predMV.y;

		if (!vector_repeats(pmv, 1))
			CheckCandidate32I(pmv[1].x, pmv[1].y, Data, 1);
		if (!vector_repeats(pmv, 2))
			CheckCandidate32I(pmv[2].x, pmv[2].y, Data, 2);

		if (*Data->iMinSAD > 500) { /* diamond only if needed */
			unsigned int mask = make_mask(pmv, 3, Data->dir);
			xvid_me_DiamondSearch(Data->currentMV->x, Data->currentMV->y, Data, mask, CheckCandidate32I);
		} else simplicity++;

		if (*Data->iMinSAD > 500) /* refinement from 2-pixel to 1-pixel */
			xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidate32I, 0);
		else simplicity++;
	} else simplicity++;

	for (i = 0; i < 4; i++) {
		MACROBLOCK * MB = &pMBs[x + (i&1) + (y+(i>>1)) * pParam->mb_width];
		MB->mvs[0] = MB->mvs[1] = MB->mvs[2] = MB->mvs[3] = Data->currentMV[i];
		MB->mode = MODE_INTER;
		/* if we skipped some search steps, we have to assume that SAD would be lower with them */
		MB->sad16 = Data->iMinSAD[i+1] - (simplicity<<7);
		if (MB->sad16 < 0) MB->sad16 = 0;
	}
}

int
MEanalysis(	const IMAGE * const pRef,
			const FRAMEINFO * const Current,
			const MBParam * const pParam,
			const int maxIntra, /* maximum number if non-I frames */
			const int intraCount, /* number of non-I frames after last I frame; 0 if we force P/B frame */
			const int bCount, /* number of B frames in a row */
			const int b_thresh,
			const MACROBLOCK * const prev_mbs)
{
	uint32_t x, y, intra = 0;
	int sSAD = 0;
	MACROBLOCK * const pMBs = Current->mbs;
	const IMAGE * const pCurrent = &Current->image;
	int IntraThresh = INTRA_THRESH, 
		InterThresh = INTER_THRESH + b_thresh,
		IntraThresh2 = INTRA_THRESH2;
		
	int blocks = 10;
	int complexity = 0;

	SearchData Data;
	Data.iEdgedWidth = pParam->edged_width;
	Data.iFcode = Current->fcode;
	Data.qpel = (pParam->vol_flags & XVID_VOL_QUARTERPEL)? 1: 0;
	Data.qpel_precision = 0;

	if (intraCount != 0) {
		if (intraCount < 30) {
			/* we're right after an I frame 
			   we increase thresholds to prevent consecutive i-frames */
			if (intraCount < 10) IntraThresh += 15*(10 - intraCount)*(10 - intraCount);
			IntraThresh2 += 4*(30 - intraCount);
		} else if (I_SENS_TH*(maxIntra - intraCount) < maxIntra) {
			/* we're close to maximum. we decrease thresholds to catch any good keyframe */
			IntraThresh -= IntraThresh*((maxIntra - I_SENS_TH*(maxIntra - intraCount))/maxIntra);
			IntraThresh2 -= IntraThresh2*((maxIntra - I_SENS_TH*(maxIntra - intraCount))/maxIntra);
		}
	}

	InterThresh -= P_SENS_BIAS * bCount;
	if (InterThresh < INTER_THRESH_MIN) InterThresh = INTER_THRESH_MIN;

	if (sadInit) (*sadInit) ();

	for (y = 1; y < pParam->mb_height-1; y += 2) {
		for (x = 1; x < pParam->mb_width-1; x += 2) {
			int i;
			blocks += 10;

			if (bCount == 0) pMBs[x + y * pParam->mb_width].mvs[0] = zeroMV;
			else { /* extrapolation of the vector found for last frame */
				pMBs[x + y * pParam->mb_width].mvs[0].x =
					(prev_mbs[x + y * pParam->mb_width].mvs[0].x * (bCount+1) ) / bCount;
				pMBs[x + y * pParam->mb_width].mvs[0].y =
					(prev_mbs[x + y * pParam->mb_width].mvs[0].y * (bCount+1) ) / bCount;
			}

			MEanalyzeMB(pRef->y, pCurrent->y, x, y, pParam, pMBs, &Data);

			for (i = 0; i < 4; i++) {
				int dev;
				MACROBLOCK *pMB = &pMBs[x+(i&1) + (y+(i>>1)) * pParam->mb_width];
				dev = dev16(pCurrent->y + (x + (i&1) + (y + (i>>1)) * pParam->edged_width) * 16,
								pParam->edged_width);

				complexity += MAX(dev, 300);
				if (dev + IntraThresh < pMB->sad16) {
					pMB->mode = MODE_INTRA;
					if (++intra > ((pParam->mb_height-2)*(pParam->mb_width-2))/2) return I_VOP;
				}

				if (pMB->mvs[0].x == 0 && pMB->mvs[0].y == 0)
					if (dev > 1000 && pMB->sad16 < 1000)
						sSAD += 512;

				sSAD += (dev < 3000) ? pMB->sad16 : pMB->sad16/2; /* blocks with big contrast differences usually have large SAD - while they look very good in b-frames */
			}
		}
	}
	complexity >>= 7;

	sSAD /= complexity + 4*blocks;

	if (sSAD > IntraThresh2) return I_VOP;
	if (sSAD > InterThresh) return P_VOP;
	emms();
	return B_VOP;
}
