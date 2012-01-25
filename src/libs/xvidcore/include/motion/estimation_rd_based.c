/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Rate-Distortion Based Motion Estimation for P- and S- VOPs  -
 *
 *  Copyright(C) 2003 Radoslaw Czyz <xvid@syskin.cjb.net>
 *               2003 Michael Militzer <michael@xvid.org>
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
 * $Id: estimation_rd_based.c,v 1.14 2005/12/09 04:45:35 syskin Exp $
 *
 ****************************************************************************/

/* RD mode decision and search */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* memcpy */

#include "../encoder.h"
#include "../bitstream/mbcoding.h"
#include "../prediction/mbprediction.h"
#include "../global.h"
#include "../image/interpolate8x8.h"
#include "estimation.h"
#include "motion.h"
#include "sad.h"
#include "../bitstream/zigzag.h"
#include "../quant/quant.h"
#include "../bitstream/vlc_codes.h"
#include "../dct/fdct.h"
#include "motion_inlines.h"

/* rd = BITS_MULT*bits + LAMBDA*distortion */
#define LAMBDA		( (int)(BITS_MULT*1.0) )

static __inline unsigned int
Block_CalcBits(	int16_t * const coeff,
				int16_t * const data,
				int16_t * const dqcoeff,
				const uint32_t quant, const int quant_type,
				uint32_t * cbp,
				const int block,
				const uint16_t * scan_table,
				const unsigned int lambda,
				const uint16_t * mpeg_quant_matrices,
				const unsigned int quant_sq)
{
	int sum;
	int bits;
	int distortion = 0;

	fdct((short * const)data);

	if (quant_type) sum = quant_h263_inter(coeff, data, quant, mpeg_quant_matrices);
	else sum = quant_mpeg_inter(coeff, data, quant, mpeg_quant_matrices);

	if (sum > 0) {
		*cbp |= 1 << (5 - block);
		bits = BITS_MULT * CodeCoeffInter_CalcBits(coeff, scan_table);

		if (quant_type) dequant_h263_inter(dqcoeff, coeff, quant, mpeg_quant_matrices);
		else dequant_mpeg_inter(dqcoeff, coeff, quant, mpeg_quant_matrices);

		distortion = sse8_16bit(data, dqcoeff, 8*sizeof(int16_t));
	} else {
		const static int16_t zero_block[64] =
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			};
		bits = 0;
		distortion = sse8_16bit(data, zero_block, 8*sizeof(int16_t));
	}


	return bits + (lambda*distortion)/quant_sq;
}

static __inline unsigned int
Block_CalcBitsIntra(MACROBLOCK * pMB,
					const unsigned int x,
					const unsigned int y,
					const unsigned int mb_width,
					const uint32_t block,
					int16_t coeff[64],
					int16_t qcoeff[64],
					int16_t dqcoeff[64],
					int16_t predictors[8],
					const uint32_t quant,
					const int quant_type,
					unsigned int bits[2],
					unsigned int cbp[2],
					unsigned int lambda,
					const uint16_t * mpeg_quant_matrices,
					const unsigned int quant_sq)
{
	int direction;
	int16_t *pCurrent;
	unsigned int i, coded;
	unsigned int distortion = 0;
	const uint32_t iDcScaler = get_dc_scaler(quant, block < 4);

	fdct((short * const)coeff);

	if (quant_type) {
		quant_h263_intra(qcoeff, coeff, quant, iDcScaler, mpeg_quant_matrices);
		dequant_h263_intra(dqcoeff, qcoeff, quant, iDcScaler, mpeg_quant_matrices);
	} else {
		quant_mpeg_intra(qcoeff, coeff, quant, iDcScaler, mpeg_quant_matrices);
		dequant_mpeg_intra(dqcoeff, qcoeff, quant, iDcScaler, mpeg_quant_matrices);
	}

	predict_acdc(pMB-(x+mb_width*y), x, y, mb_width, block, qcoeff,
				quant, iDcScaler, predictors, 0);
	
	direction = pMB->acpred_directions[block];
	pCurrent = (int16_t*)pMB->pred_values[block];

	/* store current coeffs to pred_values[] for future prediction */
	pCurrent[0] = qcoeff[0] * iDcScaler;
	pCurrent[0] = CLIP(pCurrent[0], -2048, 2047);
	for (i = 1; i < 8; i++) {
		pCurrent[i] = qcoeff[i];
		pCurrent[i + 7] = qcoeff[i * 8];
	}

	/* dc prediction */
	qcoeff[0] = qcoeff[0] - predictors[0];

	if (block < 4) bits[1] = bits[0] = dcy_tab[qcoeff[0] + 255].len - 3; /* 3 bits added before (4 times) */
	else bits[1] = bits[0] = dcc_tab[qcoeff[0] + 255].len - 2; /* 2 bits added before (2 times)*/

	/* calc cost before ac prediction */
	bits[0] += coded = CodeCoeffIntra_CalcBits(qcoeff, scan_tables[0]);
	if (coded > 0) cbp[0] |= 1 << (5 - block);

	/* apply ac prediction & calc cost*/
	if (direction == 1) {
		for (i = 1; i < 8; i++) {
			qcoeff[i] -= predictors[i];
			predictors[i] = qcoeff[i];
		}
	} else {						/* acpred_direction == 2 */
		for (i = 1; i < 8; i++) {
			qcoeff[i*8] -= predictors[i];
			predictors[i] = qcoeff[i*8];
		}
	}

	bits[1] += coded = CodeCoeffIntra_CalcBits(qcoeff, scan_tables[direction]);
	if (coded > 0) cbp[1] |= 1 << (5 - block);

	distortion = sse8_16bit(coeff, dqcoeff, 8*sizeof(int16_t));

	return (lambda*distortion)/quant_sq;
}



static void
CheckCandidateRD16(const int x, const int y, SearchData * const data, const unsigned int Direction)
{

	int16_t *in = data->dctSpace, *coeff = data->dctSpace + 64;
	/* minimum nuber of bits INTER can take is 1 (mcbpc) + 2 (cby) + 2 (vector) */
	int32_t rd = BITS_MULT * (1+2+2);
	VECTOR * current;
	const uint8_t * ptr;
	int i, t, xc, yc;
	unsigned cbp = 0;

	if ( (x > data->max_dx) || (x < data->min_dx)
		|| (y > data->max_dy) || (y < data->min_dy) ) return;

	if (!data->qpel_precision) {
		ptr = GetReference(x, y, data);
		current = data->currentMV;
		xc = x; yc = y;
	} else { /* x and y are in 1/4 precision */
		ptr = xvid_me_interpolate16x16qpel(x, y, 0, data);
		current = data->currentQMV;
		xc = x/2; yc = y/2;
	}

	for(i = 0; i < 4; i++) {
		int s = 8*((i&1) + (i>>1)*data->iEdgedWidth);
		transfer_8to16subro(in, data->Cur + s, ptr + s, data->iEdgedWidth);
		rd += data->temp[i] = Block_CalcBits(coeff, in, data->dctSpace + 128, data->iQuant, 
								data->quant_type, &cbp, i, data->scan_table, data->lambda[i], 
								data->mpeg_quant_matrices, data->quant_sq);
	}

	rd += t = BITS_MULT * (d_mv_bits(x, y, data->predMV, data->iFcode, data->qpel^data->qpel_precision) - 2);

	if (data->temp[0] + t < data->iMinSAD[1]) {
		data->iMinSAD[1] = data->temp[0] + t; current[1].x = x; current[1].y = y; data->cbp[1] = (data->cbp[1]&~32) | (cbp&32); }
	if (data->temp[1] < data->iMinSAD[2]) {
		data->iMinSAD[2] = data->temp[1]; current[2].x = x; current[2].y = y; data->cbp[1] = (data->cbp[1]&~16) | (cbp&16); }
	if (data->temp[2] < data->iMinSAD[3]) {
		data->iMinSAD[3] = data->temp[2]; current[3].x = x; current[3].y = y; data->cbp[1] = (data->cbp[1]&~8) | (cbp&8); }
	if (data->temp[3] < data->iMinSAD[4]) {
		data->iMinSAD[4] = data->temp[3]; current[4].x = x; current[4].y = y; data->cbp[1] = (data->cbp[1]&~4) | (cbp&4); }

	rd += BITS_MULT * (xvid_cbpy_tab[15-(cbp>>2)].len - 2);

	if (rd >= data->iMinSAD[0]) return;

	/* chroma */
	xc = (xc >> 1) + roundtab_79[xc & 0x3];
	yc = (yc >> 1) + roundtab_79[yc & 0x3];

	/* chroma U */
	ptr = interpolate8x8_switch2(data->RefQ, data->RefP[4], 0, 0, xc, yc, data->iEdgedWidth/2, data->rounding);
	transfer_8to16subro(in, data->CurU, ptr, data->iEdgedWidth/2);
	rd += Block_CalcBits(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type,
							&cbp, 4, data->scan_table, data->lambda[4], 
							data->mpeg_quant_matrices, data->quant_sq);
	if (rd >= data->iMinSAD[0]) return;

	/* chroma V */
	ptr = interpolate8x8_switch2(data->RefQ, data->RefP[5], 0, 0, xc, yc, data->iEdgedWidth/2, data->rounding);
	transfer_8to16subro(in, data->CurV, ptr, data->iEdgedWidth/2);
	rd += Block_CalcBits(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type,
							&cbp, 5, data->scan_table, data->lambda[5], 
							data->mpeg_quant_matrices, data->quant_sq);

	rd += BITS_MULT * (mcbpc_inter_tab[(MODE_INTER & 7) | ((cbp & 3) << 3)].len - 1); /* one was added before */

	if (rd < data->iMinSAD[0]) {
		data->iMinSAD[0] = rd;
		current[0].x = x; current[0].y = y;
		data->dir = Direction;
		*data->cbp = cbp;
	}
}

static void
CheckCandidateRD8(const int x, const int y, SearchData * const data, const unsigned int Direction)
{

	int16_t *in = data->dctSpace, *coeff = data->dctSpace + 64;
	int32_t rd;
	VECTOR * current;
	const uint8_t * ptr;
	unsigned int cbp = 0;

	if ( (x > data->max_dx) || (x < data->min_dx)
		|| (y > data->max_dy) || (y < data->min_dy) ) return;

	if (!data->qpel_precision) {
		ptr = GetReference(x, y, data);
		current = data->currentMV;
	} else { /* x and y are in 1/4 precision */
		ptr = xvid_me_interpolate8x8qpel(x, y, 0, 0, data);
		current = data->currentQMV;
	}

	transfer_8to16subro(in, data->Cur, ptr, data->iEdgedWidth);
	rd = Block_CalcBits(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type, 
						&cbp, 5, data->scan_table, data->lambda[0], 
						data->mpeg_quant_matrices, data->quant_sq);
	/* we took 2 bits into account before */
	rd += BITS_MULT * (d_mv_bits(x, y, data->predMV, data->iFcode, data->qpel^data->qpel_precision) - 2); 


	if (rd < data->iMinSAD[0]) {
		*data->cbp = cbp;
		data->iMinSAD[0] = rd;
		current[0].x = x; current[0].y = y;
		data->dir = Direction;
	}
}


static int
findRD_inter(SearchData * const Data,
			const int x, const int y,
			const MBParam * const pParam,
			const uint32_t MotionFlags)
{
	int i;
	int32_t bsad[5];

	if (Data->qpel) {
		for(i = 0; i < 5; i++) {
			Data->currentMV[i].x = Data->currentQMV[i].x/2;
			Data->currentMV[i].y = Data->currentQMV[i].y/2;
		}
		Data->qpel_precision = 1;
		CheckCandidateRD16(Data->currentQMV[0].x, Data->currentQMV[0].y, Data, 255);

		if (MotionFlags & (XVID_ME_HALFPELREFINE16_RD | XVID_ME_EXTSEARCH_RD)) { /* we have to prepare for halfpixel-precision search */
			for(i = 0; i < 5; i++) bsad[i] = Data->iMinSAD[i];
			get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
						pParam->width, pParam->height, Data->iFcode - Data->qpel, 1);
			Data->qpel_precision = 0;
			if (Data->currentQMV->x & 1 || Data->currentQMV->y & 1)
				CheckCandidateRD16(Data->currentMV[0].x, Data->currentMV[0].y, Data, 255);
		}

	} else { /* not qpel */

		CheckCandidateRD16(Data->currentMV[0].x, Data->currentMV[0].y, Data, 255);
	}

	if (MotionFlags&XVID_ME_EXTSEARCH_RD)
		xvid_me_SquareSearch(Data->currentMV->x, Data->currentMV->y, Data, 255, CheckCandidateRD16);

	if (MotionFlags&XVID_ME_HALFPELREFINE16_RD)
		xvid_me_SubpelRefine(Data->currentMV[0], Data, CheckCandidateRD16, 0);

	if (Data->qpel) {
		if (MotionFlags&(XVID_ME_EXTSEARCH_RD | XVID_ME_HALFPELREFINE16_RD)) { /* there was halfpel-precision search */
			for(i = 0; i < 5; i++) if (bsad[i] > Data->iMinSAD[i]) {
				Data->currentQMV[i].x = 2 * Data->currentMV[i].x; /* we have found a better match */
				Data->currentQMV[i].y = 2 * Data->currentMV[i].y;
			}

			/* preparing for qpel-precision search */
			Data->qpel_precision = 1;
			get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy, x, y, 4,
					pParam->width, pParam->height, Data->iFcode, 2);
		}
		if (MotionFlags & XVID_ME_QUARTERPELREFINE16_RD) {
			if (MotionFlags & XVID_ME_FASTREFINE16)
				FullRefine_Fast(Data, CheckCandidateRD16, 0);
			else 
				xvid_me_SubpelRefine(Data->currentQMV[0], Data, CheckCandidateRD16, 0);
		}
	}

	if (MotionFlags&XVID_ME_CHECKPREDICTION_RD) { /* let's check vector equal to prediction */
		VECTOR * v = Data->qpel ? Data->currentQMV : Data->currentMV;
		if (!MVequal(Data->predMV, *v))
			CheckCandidateRD16(Data->predMV.x, Data->predMV.y, Data, 255);
	}
	return Data->iMinSAD[0];
}

static int
findRD_inter4v(SearchData * const Data,
				MACROBLOCK * const pMB, const MACROBLOCK * const pMBs,
				const int x, const int y,
				const MBParam * const pParam, const uint32_t MotionFlags,
				const VECTOR * const backup)
{

	unsigned int cbp = 0, t = 0, i;
	
	/* minimum number of bits INTER4V can take is 2 (cbpy) + 3 (mcbpc) + 4*2 (vectors)*/
	int bits = (2+3+4*2)*BITS_MULT;
	SearchData Data2, *Data8 = &Data2;
	int sumx = 0, sumy = 0;
	int16_t *in = Data->dctSpace, *coeff = Data->dctSpace + 64;
	uint8_t * ptr;

	memcpy(Data8, Data, sizeof(SearchData));

	for (i = 0; i < 4; i++) { /* for all luma blocks */

		*Data8->iMinSAD = *(Data->iMinSAD + i + 1);
		*Data8->currentMV = *(Data->currentMV + i + 1);
		*Data8->currentQMV = *(Data->currentQMV + i + 1);
		Data8->Cur = Data->Cur + 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		Data8->RefP[0] = Data->RefP[0] + 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		Data8->RefP[2] = Data->RefP[2] + 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		Data8->RefP[1] = Data->RefP[1] + 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		Data8->RefP[3] = Data->RefP[3] + 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		*Data8->cbp = (Data->cbp[1] & (1<<(5-i))) ? 1:0; /* copy corresponding cbp bit */
		Data8->lambda[0] = Data->lambda[i];

		if(Data->qpel) {
			Data8->predMV = get_qpmv2(pMBs, pParam->mb_width, 0, x, y, i);
			if (i != 0)	t = d_mv_bits(	Data8->currentQMV->x, Data8->currentQMV->y,
										Data8->predMV, Data8->iFcode, 0) - 2;
		} else {
			Data8->predMV = get_pmv2(pMBs, pParam->mb_width, 0, x, y, i);
			if (i != 0)	t = d_mv_bits(	Data8->currentMV->x, Data8->currentMV->y,
										Data8->predMV, Data8->iFcode, 0) - 2;
		}

		get_range(&Data8->min_dx, &Data8->max_dx, &Data8->min_dy, &Data8->max_dy, 2*x + (i&1), 2*y + (i>>1), 3,
					pParam->width, pParam->height, Data8->iFcode, Data8->qpel+1);

		*Data8->iMinSAD += BITS_MULT * t;

		Data8->qpel_precision = Data8->qpel;
		/* checking the vector which has been found by SAD-based 8x8 search (if it's different than the one found so far) */
		{
			VECTOR *v = Data8->qpel ? Data8->currentQMV : Data8->currentMV;
			if (!MVequal (*v, backup[i+1]) )
				CheckCandidateRD8(backup[i+1].x, backup[i+1].y, Data8, 255);
		}

		if (Data8->qpel) {
			int bsad = Data8->iMinSAD[0];
			int bx = Data8->currentQMV->x;
			int by = Data8->currentQMV->y;

			Data8->currentMV->x = Data8->currentQMV->x/2;
			Data8->currentMV->y = Data8->currentQMV->y/2;

			if (MotionFlags&XVID_ME_HALFPELREFINE8_RD || (MotionFlags&XVID_ME_EXTSEARCH8 && MotionFlags&XVID_ME_EXTSEARCH_RD)) { /* halfpixel motion search follows */
				Data8->qpel_precision = 0;
				get_range(&Data8->min_dx, &Data8->max_dx, &Data8->min_dy, &Data8->max_dy, 2*x + (i&1), 2*y + (i>>1), 3,
							pParam->width, pParam->height, Data8->iFcode - 1, 1);

				if (Data8->currentQMV->x & 1 || Data8->currentQMV->y & 1)
					CheckCandidateRD8(Data8->currentMV->x, Data8->currentMV->y, Data8, 255);

				if (MotionFlags & XVID_ME_EXTSEARCH8 && MotionFlags & XVID_ME_EXTSEARCH_RD)
					xvid_me_SquareSearch(Data8->currentMV->x, Data8->currentMV->x, Data8, 255, CheckCandidateRD8);

				if (MotionFlags & XVID_ME_HALFPELREFINE8_RD)
					xvid_me_SubpelRefine(Data->currentMV[0], Data8, CheckCandidateRD8, 0);

				if(bsad > *Data8->iMinSAD) { /* we have found a better match */
					bx = Data8->currentQMV->x = 2*Data8->currentMV->x;
					by = Data8->currentQMV->y = 2*Data8->currentMV->y;
					bsad = Data8->iMinSAD[0];
				}

				Data8->qpel_precision = 1;
				get_range(&Data8->min_dx, &Data8->max_dx, &Data8->min_dy, &Data8->max_dy, 2*x + (i&1), 2*y + (i>>1), 3,
							pParam->width, pParam->height, Data8->iFcode, 2);

			}

			if (MotionFlags & XVID_ME_QUARTERPELREFINE8_RD) {
				if (MotionFlags & XVID_ME_FASTREFINE8)
					FullRefine_Fast(Data8, CheckCandidateRD8, 0);
				else xvid_me_SubpelRefine(Data->currentQMV[0], Data8, CheckCandidateRD8, 0);
			}

			if (bsad <= Data->iMinSAD[0]) { 
				/* we have not found a better match */
				Data8->iMinSAD[0] = bsad;
				Data8->currentQMV->x = bx;
				Data8->currentQMV->y = by;
			}

		} else { /* not qpel */

			if (MotionFlags & XVID_ME_EXTSEARCH8 && MotionFlags & XVID_ME_EXTSEARCH_RD) /* extsearch */
				xvid_me_SquareSearch(Data8->currentMV->x, Data8->currentMV->x, Data8, 255, CheckCandidateRD8);

			if (MotionFlags & XVID_ME_HALFPELREFINE8_RD)
				xvid_me_SubpelRefine(Data->currentMV[0], Data8, CheckCandidateRD8, 0); /* halfpel refinement */
		}

		/* checking vector equal to predicion */
		if (i != 0 && MotionFlags & XVID_ME_CHECKPREDICTION_RD) {
			const VECTOR * v = Data->qpel ? Data8->currentQMV : Data8->currentMV;
			if (!MVequal(*v, Data8->predMV))
				CheckCandidateRD8(Data8->predMV.x, Data8->predMV.y, Data8, 255);
		}

		bits += *Data8->iMinSAD;
		if (bits >= Data->iMinSAD[0]) return bits; /* no chances for INTER4V */

		/* MB structures for INTER4V mode; we have to set them here, we don't have predictor anywhere else */
		if(Data->qpel) {
			pMB->pmvs[i].x = Data8->currentQMV->x - Data8->predMV.x;
			pMB->pmvs[i].y = Data8->currentQMV->y - Data8->predMV.y;
			pMB->qmvs[i] = *Data8->currentQMV;
			sumx += Data8->currentQMV->x/2;
			sumy += Data8->currentQMV->y/2;
		} else {
			pMB->pmvs[i].x = Data8->currentMV->x - Data8->predMV.x;
			pMB->pmvs[i].y = Data8->currentMV->y - Data8->predMV.y;
			sumx += Data8->currentMV->x;
			sumy += Data8->currentMV->y;
		}
		pMB->mvs[i] = *Data8->currentMV;
		pMB->sad8[i] = 4 * *Data8->iMinSAD;
		if (Data8->cbp[0]) cbp |= 1 << (5 - i);

	} /* end - for all luma blocks */

	bits += BITS_MULT * (xvid_cbpy_tab[15-(cbp>>2)].len - 2); /* 2 were added before */

	/* let's check chroma */
	sumx = (sumx >> 3) + roundtab_76[sumx & 0xf];
	sumy = (sumy >> 3) + roundtab_76[sumy & 0xf];

	/* chroma U */
	ptr = interpolate8x8_switch2(Data->RefQ + 64, Data->RefP[4], 0, 0, sumx, sumy, Data->iEdgedWidth/2, Data->rounding);
	transfer_8to16subro(in, Data->CurU, ptr, Data->iEdgedWidth/2);
	bits += Block_CalcBits(coeff, in, Data->dctSpace + 128, Data->iQuant, Data->quant_type, &cbp, 4,
							Data->scan_table, Data->lambda[4], Data->mpeg_quant_matrices, Data->quant_sq);

	if (bits >= *Data->iMinSAD) return bits;

	/* chroma V */
	ptr = interpolate8x8_switch2(Data->RefQ + 64, Data->RefP[5], 0, 0, sumx, sumy, Data->iEdgedWidth/2, Data->rounding);
	transfer_8to16subro(in, Data->CurV, ptr, Data->iEdgedWidth/2);
	bits += Block_CalcBits(coeff, in, Data->dctSpace + 128, Data->iQuant, Data->quant_type, &cbp, 5,
							Data->scan_table, Data->lambda[5], Data->mpeg_quant_matrices, Data->quant_sq);

	bits += BITS_MULT*(mcbpc_inter_tab[(MODE_INTER4V & 7) | ((cbp & 3) << 3)].len - 3); /* 3 were added before */

	*Data->cbp = cbp;
	return bits;
}

static int
findRD_intra(SearchData * const Data, MACROBLOCK * pMB,
			 const int x, const int y, const int mb_width)
{
	unsigned int cbp[2] = {0, 0}, bits[2], i;
	/* minimum number of bits that WILL be coded in intra - mcbpc 5, cby 2 acdc flag - 1 and DC coeffs - 4*3+2*2 */
	int bits1 = BITS_MULT*(5+2+1+4*3+2*2), bits2 = BITS_MULT*(5+2+1+4*3+2*2);
	unsigned int distortion = 0;

	int16_t *in = Data->dctSpace, * coeff = Data->dctSpace + 64, * dqcoeff = Data->dctSpace + 128;
	const uint32_t iQuant = Data->iQuant;
	int16_t predictors[6][8];

	for(i = 0; i < 4; i++) {
		int s = 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		transfer_8to16copy(in, Data->Cur + s, Data->iEdgedWidth);
		

		distortion = Block_CalcBitsIntra(pMB, x, y, mb_width, i, in, coeff, dqcoeff,
								predictors[i], iQuant, Data->quant_type, bits, cbp, 
								Data->lambda[i], Data->mpeg_quant_matrices, Data->quant_sq);
		bits1 += distortion + BITS_MULT * bits[0];
		bits2 += distortion + BITS_MULT * bits[1];

		if (bits1 >= Data->iMinSAD[0] && bits2 >= Data->iMinSAD[0]) 
			return bits1;
	}

	bits1 += BITS_MULT * (xvid_cbpy_tab[cbp[0]>>2].len - 2); /* two bits were added before */
	bits2 += BITS_MULT * (xvid_cbpy_tab[cbp[1]>>2].len - 2);

	/*chroma U */
	transfer_8to16copy(in, Data->CurU, Data->iEdgedWidth/2);
	distortion = Block_CalcBitsIntra(pMB, x, y, mb_width, 4, in, coeff, dqcoeff,
									predictors[4], iQuant, Data->quant_type, bits, cbp, 
									Data->lambda[4], Data->mpeg_quant_matrices, Data->quant_sq);
	bits1 += distortion + BITS_MULT * bits[0];
	bits2 += distortion + BITS_MULT * bits[1];

	if (bits1 >= Data->iMinSAD[0] && bits2 >= Data->iMinSAD[0]) 
		return bits1;

	/* chroma V */
	transfer_8to16copy(in, Data->CurV, Data->iEdgedWidth/2);
	distortion = Block_CalcBitsIntra(pMB, x, y, mb_width, 5, in, coeff, dqcoeff,
									predictors[5], iQuant, Data->quant_type, bits, cbp, 
									Data->lambda[5], Data->mpeg_quant_matrices, Data->quant_sq);

	bits1 += distortion + BITS_MULT * bits[0];
	bits2 += distortion + BITS_MULT * bits[1];

	bits1 += BITS_MULT * (mcbpc_inter_tab[(MODE_INTRA & 7) | ((cbp[0] & 3) << 3)].len - 5); /* 5 bits were added before */
	bits2 += BITS_MULT * (mcbpc_inter_tab[(MODE_INTRA & 7) | ((cbp[1] & 3) << 3)].len - 5);

	*Data->cbp = bits1 <= bits2 ? cbp[0] : cbp[1];

	return MIN(bits1, bits2);
}


static int
findRD_gmc(SearchData * const Data, const IMAGE * const vGMC, const int x, const int y)
{
	/* minimum nubler of bits - 1 (mcbpc) + 2 (cby) + 1 (mcsel) */
	int bits = BITS_MULT * (1+2+1);
	unsigned int cbp = 0, i;
	int16_t *in = Data->dctSpace, * coeff = Data->dctSpace + 64;

	for(i = 0; i < 4; i++) {
		int s = 8*((i&1) + (i>>1)*Data->iEdgedWidth);
		transfer_8to16subro(in, Data->Cur + s, vGMC->y + s + 16*(x+y*Data->iEdgedWidth), Data->iEdgedWidth);
		bits += Block_CalcBits(coeff, in, Data->dctSpace + 128, Data->iQuant, Data->quant_type, &cbp, i,
								Data->scan_table, Data->lambda[i], Data->mpeg_quant_matrices, Data->quant_sq);
		if (bits >= Data->iMinSAD[0]) return bits;
	}

	bits += BITS_MULT * (xvid_cbpy_tab[15-(cbp>>2)].len - 2);

	/*chroma U */
	transfer_8to16subro(in, Data->CurU, vGMC->u + 8*(x+y*(Data->iEdgedWidth/2)), Data->iEdgedWidth/2);
	bits += Block_CalcBits(coeff, in, Data->dctSpace + 128, Data->iQuant, Data->quant_type, &cbp, 4,
							Data->scan_table, Data->lambda[4], Data->mpeg_quant_matrices, Data->quant_sq);

	if (bits >= Data->iMinSAD[0]) return bits;

	/* chroma V */
	transfer_8to16subro(in, Data->CurV , vGMC->v + 8*(x+y*(Data->iEdgedWidth/2)), Data->iEdgedWidth/2);
	bits += Block_CalcBits(coeff, in, Data->dctSpace + 128, Data->iQuant, Data->quant_type, &cbp, 5,
							Data->scan_table, Data->lambda[5], Data->mpeg_quant_matrices, Data->quant_sq);

	bits += BITS_MULT * (mcbpc_inter_tab[(MODE_INTER & 7) | ((cbp & 3) << 3)].len - 1);

	*Data->cbp = cbp;

	return bits;
}

void
xvid_me_ModeDecision_RD(SearchData * const Data,
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
				const int coding_type)
{
	int mode = MODE_INTER;
	int mcsel = 0;
	int inter4v = (VopFlags & XVID_VOP_INTER4V) && (pMB->dquant == 0);
	const uint32_t iQuant = pMB->quant;
	int min_rd, intra_rd, i, cbp;
	VECTOR backup[5], *v;
	Data->iQuant = iQuant;
	Data->quant_sq = iQuant*iQuant;
	Data->scan_table = VopFlags & XVID_VOP_ALTERNATESCAN ?
						scan_tables[2] : scan_tables[0];

	pMB->mcsel = 0;

	v = Data->qpel ? Data->currentQMV : Data->currentMV;
	for (i = 0; i < 5; i++) {
		Data->iMinSAD[i] = 256*4096;
		backup[i] = v[i];
	}

	for (i = 0; i < 6; i++) {
		Data->lambda[i] = (LAMBDA*pMB->lambda[i])>>LAMBDA_EXP;
	}

	min_rd = findRD_inter(Data, x, y, pParam, MotionFlags);
	cbp = *Data->cbp;

	if (coding_type == S_VOP) {
		int gmc_rd;
		*Data->iMinSAD = min_rd += BITS_MULT*1; /* mcsel */
		gmc_rd = findRD_gmc(Data, vGMC, x, y);
		if (gmc_rd < min_rd) {
			mcsel = 1;
			*Data->iMinSAD = min_rd = gmc_rd;
			mode = MODE_INTER;
			cbp = *Data->cbp;
		}
	}

	if (inter4v) {
		int v4_rd;
		v4_rd = findRD_inter4v(Data, pMB, pMBs, x, y, pParam, MotionFlags, backup);
		if (v4_rd < min_rd) {
			Data->iMinSAD[0] = min_rd = v4_rd;
			mode = MODE_INTER4V;
			cbp = *Data->cbp;
		}
	}
	
	/* there is no way for INTRA to take less than 24 bits - go to findRD_intra() for calculations */
	if (min_rd > 24*BITS_MULT) { 
		intra_rd = findRD_intra(Data, pMB, x, y, pParam->mb_width);
		if (intra_rd < min_rd) {
			*Data->iMinSAD = min_rd = intra_rd;
			mode = MODE_INTRA;
			cbp = *Data->cbp;
		}
	}

	pMB->sad16 = pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] = 0;
	pMB->cbp = cbp;

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

void
xvid_me_ModeDecision_Fast(SearchData * const Data,
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
			const int coding_type)
{
	int mode = MODE_INTER;
	int mcsel = 0;
	int inter4v = (VopFlags & XVID_VOP_INTER4V) && (pMB->dquant == 0);
	const uint32_t iQuant = pMB->quant;
	const int skip_possible = (coding_type == P_VOP) && (pMB->dquant == 0);
    int sad;
	int min_rd = -1, intra_rd, i, cbp = 63;
	VECTOR backup[5], *v;
	int sad_backup[5];
	int InterBias = MV16_INTER_BIAS;
	int thresh = 0;
	int top = 0, top_right = 0, left = 0;
	Data->scan_table = VopFlags & XVID_VOP_ALTERNATESCAN ?
						scan_tables[2] : scan_tables[0];

	pMB->mcsel = 0;
	Data->iQuant = iQuant;
	Data->quant_sq = iQuant*iQuant;

	for (i = 0; i < 6; i++) {
		Data->lambda[i] = (LAMBDA*pMB->lambda[i])>>LAMBDA_EXP;
	}

	/* INTER <-> INTER4V decision */
	if ((Data->iMinSAD[0] + 75 < Data->iMinSAD[1] +
		Data->iMinSAD[2] + Data->iMinSAD[3] + Data->iMinSAD[4])) { /* normal, fast, SAD-based mode decision */
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
		if (skip_possible && (pMB->sad16 < (int)iQuant * MAX_SAD00_FOR_SKIP))
			if ( (100*sad)/(pMB->sad16+1) > FINAL_SKIP_THRESH)
				if (Data->chroma || xvid_me_SkipDecisionP(pCurrent, pRef, x, y, Data->iEdgedWidth/2, iQuant)) {
					mode = MODE_NOT_CODED;
					sad = 0;  /* Compiler warning */
					goto early_out;
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
	} else { /* Rate-Distortion INTER<->INTER4V */
		Data->iQuant = iQuant;
		v = Data->qpel ? Data->currentQMV : Data->currentMV;

		/* final skip decision, a.k.a. "the vector you found, really that good?" */
		if (skip_possible && (pMB->sad16 < (int)iQuant * MAX_SAD00_FOR_SKIP))
			if ( (100*Data->iMinSAD[0])/(pMB->sad16+1) > FINAL_SKIP_THRESH)
				if (Data->chroma || xvid_me_SkipDecisionP(pCurrent, pRef, x, y, Data->iEdgedWidth/2, iQuant)) {
					mode = MODE_NOT_CODED;
					sad = 0; /* Compiler warning */
					goto early_out;
				}

		for (i = 0; i < 5; i++) {
			sad_backup[i] = Data->iMinSAD[i];
			Data->iMinSAD[i] = 256*4096;
			backup[i] = v[i];
		}

		min_rd = findRD_inter(Data, x, y, pParam, MotionFlags);
		cbp = *Data->cbp;
		sad = sad_backup[0];

		if (coding_type == S_VOP) {
			int gmc_rd;

			*Data->iMinSAD = min_rd += BITS_MULT*1; /* mcsel */
			gmc_rd = findRD_gmc(Data, vGMC, x, y);
			if (gmc_rd < min_rd) {
				mcsel = 1;
				*Data->iMinSAD = min_rd = gmc_rd;
				mode = MODE_INTER;
				cbp = *Data->cbp;
				sad = sad16(Data->Cur,
					vGMC->y + 16*y*Data->iEdgedWidth + 16*x, Data->iEdgedWidth, 65536);
				if (Data->chroma) {
					sad += sad8(Data->CurU, vGMC->u + 8*y*(Data->iEdgedWidth/2) + 8*x, Data->iEdgedWidth/2);
					sad += sad8(Data->CurV, vGMC->v + 8*y*(Data->iEdgedWidth/2) + 8*x, Data->iEdgedWidth/2);
				}
			}
		}

		if (inter4v) {
			int v4_rd;
			v4_rd = findRD_inter4v(Data, pMB, pMBs, x, y, pParam, MotionFlags, backup);
			if (v4_rd < min_rd) {
				Data->iMinSAD[0] = min_rd = v4_rd;
				mode = MODE_INTER4V;
				cbp = *Data->cbp;
				sad = sad_backup[1] + sad_backup[2] +
					  sad_backup[3] + sad_backup[4] + IMV16X16 * (int32_t)iQuant;
			}
		}
	}

	left = top = top_right = -1;
	thresh = 0;

	if((x > 0) && (y > 0) && (x < (int32_t) pParam->mb_width)) {
		left = (&pMBs[(x-1) + y * pParam->mb_width])->sad16; /* left */
		top = (&pMBs[x + (y-1) * pParam->mb_width])->sad16; /* top */
		top_right = (&pMBs[(x+1) + (y-1) * pParam->mb_width])->sad16; /* top right */

		if(((&pMBs[(x-1) + y * pParam->mb_width])->mode != MODE_INTRA) &&
		   ((&pMBs[x + (y-1) * pParam->mb_width])->mode != MODE_INTRA) &&
		   ((&pMBs[(x+1) + (y-1) * pParam->mb_width])->mode != MODE_INTRA)) {
			thresh = MAX(MAX(top, left), top_right);
		}
		else
			thresh = MIN(MIN(top, left), top_right);
	}

	/* INTRA <-> INTER decision */
	if (sad < thresh) { /* normal, fast, SAD-based mode decision */
		/* intra decision */

		if (iQuant > 8) InterBias += 100 * (iQuant - 8); /* to make high quants work */
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
	} else { /* Rate-Distortion INTRA<->INTER */
		if(min_rd < 0) {
			Data->iQuant = iQuant;
			v = Data->qpel ? Data->currentQMV : Data->currentMV;

			for (i = 0; i < 5; i++) {
				Data->iMinSAD[i] = 256*4096;
				backup[i] = v[i];
			}

			if(mode == MODE_INTER) {
				min_rd = findRD_inter(Data, x, y, pParam, MotionFlags);
				cbp = *Data->cbp;

				if (coding_type == S_VOP) {
					int gmc_rd;

					*Data->iMinSAD = min_rd += BITS_MULT*1; /* mcsel */
					gmc_rd = findRD_gmc(Data, vGMC, x, y);
					if (gmc_rd < min_rd) {
						mcsel = 1;
						*Data->iMinSAD = min_rd = gmc_rd;
						mode = MODE_INTER;
						cbp = *Data->cbp;
					}
				}
			}

			if(mode == MODE_INTER4V) {
				int v4_rd;
				v4_rd = findRD_inter4v(Data, pMB, pMBs, x, y, pParam, MotionFlags, backup);
				if (v4_rd < min_rd) {
					Data->iMinSAD[0] = min_rd = v4_rd;
					mode = MODE_INTER4V;
					cbp = *Data->cbp;
				}
			}
		}

		intra_rd = findRD_intra(Data, pMB, x, y, pParam->mb_width);
		if (intra_rd < min_rd) {
			*Data->iMinSAD = min_rd = intra_rd;
			mode = MODE_INTRA;
		}

		pMB->cbp = cbp;
	}

early_out:
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
