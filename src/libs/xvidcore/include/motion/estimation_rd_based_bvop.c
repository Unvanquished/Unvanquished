/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Rate-Distortion Based Motion Estimation for B- VOPs  -
 *
 *  Copyright(C) 2004 Radoslaw Czyz <xvid@syskin.cjb.net>
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
 * $Id: estimation_rd_based_bvop.c,v 1.10 2005/12/09 04:45:35 syskin Exp $
 *
 ****************************************************************************/

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
Block_CalcBits_BVOP(int16_t * const coeff,
				int16_t * const data,
				int16_t * const dqcoeff,
				const uint32_t quant, const int quant_type,
				uint32_t * cbp,
				const int block,
				const uint16_t * scan_table,
				const unsigned int lambda,
				const uint16_t * mpeg_quant_matrices,
				const unsigned int quant_sq,
				int * const cbpcost)
{
	int sum;
	int bits;
	int distortion = 0;

	fdct((short * const)data);

	if (quant_type) sum = quant_h263_inter(coeff, data, quant, mpeg_quant_matrices);
	else sum = quant_mpeg_inter(coeff, data, quant, mpeg_quant_matrices);

	if ((sum >= 3) || (coeff[1] != 0) || (coeff[8] != 0) || (coeff[0] != 0)) {
		*cbp |= 1 << (5 - block);
		bits = BITS_MULT * CodeCoeffInter_CalcBits(coeff, scan_table);
		bits += *cbpcost;
		*cbpcost = 0; /* don't add cbp cost second time */

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
Block_CalcBits_BVOP_direct(int16_t * const coeff,
				int16_t * const data,
				int16_t * const dqcoeff,
				const uint32_t quant, const int quant_type,
				uint32_t * cbp,
				const int block,
				const uint16_t * scan_table,
				const unsigned int lambda,
				const uint16_t * mpeg_quant_matrices,
				const unsigned int quant_sq,
				int * const cbpcost)
{
	int sum;
	int bits;
	int distortion = 0;

	fdct((short * const)data);

	if (quant_type) sum = quant_h263_inter(coeff, data, quant, mpeg_quant_matrices);
	else sum = quant_mpeg_inter(coeff, data, quant, mpeg_quant_matrices);

	if ((sum >= 3) || (coeff[1] != 0) || (coeff[8] != 0) || (coeff[0] > 0) || (coeff[0] < -1)) {
		*cbp |= 1 << (5 - block);
		bits = BITS_MULT * CodeCoeffInter_CalcBits(coeff, scan_table);
		bits += *cbpcost;
		*cbpcost = 0;

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

static void
CheckCandidateRDBF(const int x, const int y, SearchData * const data, const unsigned int Direction)
{

	int16_t *in = data->dctSpace, *coeff = data->dctSpace + 64;
	int32_t rd = (3+2)*BITS_MULT; /* 3 bits for mode + 2 for vector (minimum) */
	VECTOR * current;
	const uint8_t * ptr;
	int i, xc, yc;
	unsigned cbp = 0;
	int cbpcost = 7*BITS_MULT; /* how much to add if cbp turns out to be non-zero */

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

	rd += BITS_MULT*(d_mv_bits(x, y, data->predMV, data->iFcode, data->qpel^data->qpel_precision)-2);

	for(i = 0; i < 4; i++) {
		int s = 8*((i&1) + (i>>1)*data->iEdgedWidth);
		transfer_8to16subro(in, data->Cur + s, ptr + s, data->iEdgedWidth);
		rd += Block_CalcBits_BVOP(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type,
								&cbp, i, data->scan_table, data->lambda[i], data->mpeg_quant_matrices, 
								data->quant_sq, &cbpcost);
		if (rd >= data->iMinSAD[0]) return;
	}

	/* chroma */
	xc = (xc >> 1) + roundtab_79[xc & 0x3];
	yc = (yc >> 1) + roundtab_79[yc & 0x3];

	/* chroma U */
	ptr = interpolate8x8_switch2(data->RefQ, data->RefP[4], 0, 0, xc, yc, data->iEdgedWidth/2, data->rounding);
	transfer_8to16subro(in, data->CurU, ptr, data->iEdgedWidth/2);
	rd += Block_CalcBits_BVOP(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type,
								&cbp, 4, data->scan_table, data->lambda[4], data->mpeg_quant_matrices, 
								data->quant_sq, &cbpcost);
	if (rd >= data->iMinSAD[0]) return;

	/* chroma V */
	ptr = interpolate8x8_switch2(data->RefQ, data->RefP[5], 0, 0, xc, yc, data->iEdgedWidth/2, data->rounding);
	transfer_8to16subro(in, data->CurV, ptr, data->iEdgedWidth/2);
	rd += Block_CalcBits_BVOP(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type, 
								&cbp, 5, data->scan_table, data->lambda[5], data->mpeg_quant_matrices, 
								data->quant_sq, &cbpcost);

	if (rd < data->iMinSAD[0]) {
		data->iMinSAD[0] = rd;
		current[0].x = x; current[0].y = y;
		data->dir = Direction;
		*data->cbp = cbp;
	}
}

static void
CheckCandidateRDDirect(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t xcf = 0, ycf = 0, xcb = 0, ycb = 0;
	int32_t rd = 1*BITS_MULT;
	int16_t *in = data->dctSpace, *coeff = data->dctSpace + 64;
	unsigned int cbp = 0;
	unsigned int k;
	VECTOR mvs, b_mvs;
	int cbpcost = 6*BITS_MULT; /* how much to add if cbp turns out to be non-zero */

	const uint8_t *ReferenceF, *ReferenceB;

	if (( x > 31) || ( x < -32) || ( y > 31) || (y < -32)) return;

	for (k = 0; k < 4; k++) {
		int s = 8*((k&1) + (k>>1)*data->iEdgedWidth);

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
			ReferenceF = xvid_me_interpolate8x8qpel(mvs.x, mvs.y, k, 0, data);
			ReferenceB = xvid_me_interpolate8x8qpel(b_mvs.x, b_mvs.y, k, 1, data);
		} else {
			xcf += mvs.x; ycf += mvs.y;
			xcb += b_mvs.x; ycb += b_mvs.y;
			ReferenceF = GetReference(mvs.x, mvs.y, data) + s;
			ReferenceB = GetReferenceB(b_mvs.x, b_mvs.y, 1, data) + s;
		}

		transfer_8to16sub2ro(in, data->Cur + s, ReferenceF, ReferenceB, data->iEdgedWidth);
		rd += Block_CalcBits_BVOP_direct(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type, 
										&cbp, k, data->scan_table, data->lambda[k], data->mpeg_quant_matrices, 
										data->quant_sq, &cbpcost);
		if (rd > *(data->iMinSAD)) return;
	}
	
	/* chroma */
	xcf = (xcf >> 3) + roundtab_76[xcf & 0xf];
	ycf = (ycf >> 3) + roundtab_76[ycf & 0xf];
	xcb = (xcb >> 3) + roundtab_76[xcb & 0xf];
	ycb = (ycb >> 3) + roundtab_76[ycb & 0xf];

	/* chroma U */
	ReferenceF = interpolate8x8_switch2(data->RefQ, data->RefP[4], 0, 0, xcf, ycf, data->iEdgedWidth/2, data->rounding);
	ReferenceB = interpolate8x8_switch2(data->RefQ + 16, data->b_RefP[4], 0, 0, xcb, ycb, data->iEdgedWidth/2, data->rounding);
	transfer_8to16sub2ro(in, data->CurU, ReferenceF, ReferenceB, data->iEdgedWidth/2);
	rd += Block_CalcBits_BVOP_direct(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type,
									&cbp, 4, data->scan_table, data->lambda[4], data->mpeg_quant_matrices,
									data->quant_sq, &cbpcost);
	if (rd >= data->iMinSAD[0]) return;

	/* chroma V */
	ReferenceF = interpolate8x8_switch2(data->RefQ, data->RefP[5], 0, 0, xcf, ycf, data->iEdgedWidth/2, data->rounding);
	ReferenceB = interpolate8x8_switch2(data->RefQ + 16, data->b_RefP[5], 0, 0, xcb, ycb, data->iEdgedWidth/2, data->rounding);
	transfer_8to16sub2ro(in, data->CurV, ReferenceF, ReferenceB, data->iEdgedWidth/2);
	rd += Block_CalcBits_BVOP_direct(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type,
									&cbp, 5, data->scan_table, data->lambda[5], data->mpeg_quant_matrices,
									data->quant_sq, &cbpcost);

	if (cbp || x != 0 || y != 0)
		rd += BITS_MULT * d_mv_bits(x, y, zeroMV, 1, 0);

	if (rd < *(data->iMinSAD)) {
		*data->iMinSAD = rd;
		data->currentMV->x = x; data->currentMV->y = y;
		data->dir = Direction;
		*data->cbp = cbp;
	}
}

static void
CheckCandidateRDInt(const int x, const int y, SearchData * const data, const unsigned int Direction)
{
	int32_t xf, yf, xb, yb, xcf, ycf, xcb, ycb;
	int32_t rd = 2*BITS_MULT;
	int16_t *in = data->dctSpace, *coeff = data->dctSpace + 64;
	unsigned int cbp = 0;
	unsigned int i;
	int cbpcost = 7*BITS_MULT; /* how much to add if cbp turns out to be non-zero */

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

	rd += BITS_MULT * (d_mv_bits(xf, yf, data->predMV, data->iFcode, data->qpel^data->qpel_precision)
					+ d_mv_bits(xb, yb, data->bpredMV, data->iFcode, data->qpel^data->qpel_precision));

	for(i = 0; i < 4; i++) {
		int s = 8*((i&1) + (i>>1)*data->iEdgedWidth);
		if (rd >= *data->iMinSAD) return;
		transfer_8to16sub2ro(in, data->Cur + s, ReferenceF + s, ReferenceB + s, data->iEdgedWidth);
		rd += Block_CalcBits_BVOP(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type, &cbp,
								i, data->scan_table, data->lambda[i], data->mpeg_quant_matrices,
								data->quant_sq, &cbpcost);
	}

	/* chroma */
	xcf = (xcf >> 1) + roundtab_79[xcf & 0x3];
	ycf = (ycf >> 1) + roundtab_79[ycf & 0x3];
	xcb = (xcb >> 1) + roundtab_79[xcb & 0x3];
	ycb = (ycb >> 1) + roundtab_79[ycb & 0x3];

	/* chroma U */
	ReferenceF = interpolate8x8_switch2(data->RefQ, data->RefP[4], 0, 0, xcf, ycf, data->iEdgedWidth/2, data->rounding);
	ReferenceB = interpolate8x8_switch2(data->RefQ + 16, data->b_RefP[4], 0, 0, xcb, ycb, data->iEdgedWidth/2, data->rounding);
	transfer_8to16sub2ro(in, data->CurU, ReferenceF, ReferenceB, data->iEdgedWidth/2);
	rd += Block_CalcBits_BVOP(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type, &cbp,
								4, data->scan_table, data->lambda[4], data->mpeg_quant_matrices, 
								data->quant_sq, &cbpcost);
	if (rd >= data->iMinSAD[0]) return;


	/* chroma V */
	ReferenceF = interpolate8x8_switch2(data->RefQ, data->RefP[5], 0, 0, xcf, ycf, data->iEdgedWidth/2, data->rounding);
	ReferenceB = interpolate8x8_switch2(data->RefQ + 16, data->b_RefP[5], 0, 0, xcb, ycb, data->iEdgedWidth/2, data->rounding);
	transfer_8to16sub2ro(in, data->CurV, ReferenceF, ReferenceB, data->iEdgedWidth/2);
	rd += Block_CalcBits_BVOP(coeff, in, data->dctSpace + 128, data->iQuant, data->quant_type, &cbp,
								5, data->scan_table, data->lambda[5], data->mpeg_quant_matrices,
								data->quant_sq, &cbpcost);

	if (rd < *(data->iMinSAD)) {
		*data->iMinSAD = rd;
		current->x = x; current->y = y;
		data->dir = Direction;
		*data->cbp = cbp;
	}
}

static int
SearchInterpolate_RD(const int x, const int y,
					 const uint32_t MotionFlags,
					 const MBParam * const pParam,
					 int32_t * const best_sad,
					 SearchData * const Data)
{
	int i, j;

	Data->iMinSAD[0] = *best_sad;

	get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy,
		x, y, 4, pParam->width, pParam->height, Data->iFcode, 1 + Data->qpel);
	
	Data->qpel_precision = Data->qpel;

	if (Data->qpel) {
		i = Data->currentQMV[0].x; j = Data->currentQMV[0].y;
	} else {
		i = Data->currentMV[0].x; j = Data->currentMV[0].y;
	}

	CheckCandidateRDInt(i, j, Data, 1);

	return Data->iMinSAD[0];
}

static int
SearchDirect_RD(const int x, const int y,
					 const uint32_t MotionFlags,
					 const MBParam * const pParam,
					 int32_t * const best_sad,
					 SearchData * const Data)
{
	Data->iMinSAD[0] = *best_sad;
	
	Data->qpel_precision = Data->qpel;

	CheckCandidateRDDirect(Data->currentMV->x, Data->currentMV->y, Data, 255);

	return Data->iMinSAD[0];
}

static int
SearchBF_RD(const int x, const int y,
					 const uint32_t MotionFlags,
					 const MBParam * const pParam,
					 int32_t * const best_sad,
					 SearchData * const Data)
{
	int i, j;

	Data->iMinSAD[0] = *best_sad;

	get_range(&Data->min_dx, &Data->max_dx, &Data->min_dy, &Data->max_dy,
		x, y, 4, pParam->width, pParam->height, Data->iFcode, 1 + Data->qpel);
	
	Data->qpel_precision = Data->qpel;

	if (Data->qpel) {
		i = Data->currentQMV[0].x; j = Data->currentQMV[0].y;
	} else {
		i = Data->currentMV[0].x; j = Data->currentMV[0].y;
	}

	CheckCandidateRDBF(i, j, Data, 1);

	return Data->iMinSAD[0];
}

static int
get_sad_for_mode(int mode, 
				 SearchData * const Data_d,
				 SearchData * const Data_b,
				 SearchData * const Data_f,
				 SearchData * const Data_i)
{
	switch(mode) {
		case MODE_DIRECT: return Data_d->iMinSAD[0];
		case MODE_FORWARD: return Data_f->iMinSAD[0];
		case MODE_BACKWARD: return Data_b->iMinSAD[0];
		default:
		case MODE_INTERPOLATE: return Data_i->iMinSAD[0];
	}
}

void 
ModeDecision_BVOP_RD(SearchData * const Data_d,
					 SearchData * const Data_b,
					 SearchData * const Data_f,
					 SearchData * const Data_i,
					 MACROBLOCK * const pMB,
					 const MACROBLOCK * const b_mb,
					 VECTOR * f_predMV,
					 VECTOR * b_predMV,
					 const uint32_t MotionFlags,
					 const MBParam * const pParam,
					 int x, int y,
					 int best_sad)
{
	int mode = MODE_DIRECT, k;
	int f_rd, b_rd, i_rd, d_rd, best_rd;
	const int qpel = Data_d->qpel;
	const uint32_t iQuant = Data_d->iQuant;
	int i;
	int ref_quant = b_mb->quant;
	int no_of_checks = 0;

	int order[4] = {MODE_DIRECT, MODE_FORWARD, MODE_BACKWARD, MODE_INTERPOLATE};

	Data_d->scan_table = Data_b->scan_table = Data_f->scan_table = Data_i->scan_table 
		= /*VopFlags & XVID_VOP_ALTERNATESCAN ? scan_tables[2] : */scan_tables[0];
	*Data_f->cbp = *Data_b->cbp = *Data_i->cbp = *Data_d->cbp = 63;

	f_rd = b_rd = i_rd = d_rd = best_rd = 256*4096;

	for (i = 0; i < 6; i++) {
		/* re-calculate as if it was p-frame's quant +.5 */
		int lam = (pMB->lambda[i]*LAMBDA*iQuant*iQuant)/(ref_quant*(ref_quant+1));
		lam >>= LAMBDA_EXP;
		Data_d->lambda[i] = lam;
		Data_b->lambda[i] = lam;
		Data_f->lambda[i] = lam;
		Data_i->lambda[i] = lam;
	}

	/* find the best order of evaluation - smallest SAD comes first, because *if* it means smaller RD,
	   early-stops will activate sooner */

	for (i = 3; i >= 0; i--) {
		int j;
		for (j = 0; j < i; j++) {
			int sad1 = get_sad_for_mode(order[j], Data_d, Data_b, Data_f, Data_i);
			int sad2 = get_sad_for_mode(order[j+1], Data_d, Data_b, Data_f, Data_i);
			if (sad1 > sad2) {
				int t = order[j];
				order[j] = order[j+1];
				order[j+1] = t;
			}
		}
	}

	for(i = 0; i < 4; i++)
		if (get_sad_for_mode(order[i], Data_d, Data_b, Data_f, Data_i) < 2*best_sad)
			no_of_checks++;

	if (no_of_checks > 1) {
		/* evaluate cost of all modes */
		for (i = 0; i < no_of_checks; i++) {
			int rd;
			if (2*best_sad < get_sad_for_mode(order[i], Data_d, Data_b, Data_f, Data_i)) 
				break; /* further SADs are too big */
			
			switch (order[i]) {
			case MODE_DIRECT:
				rd = d_rd = SearchDirect_RD(x, y, MotionFlags, pParam, &best_rd, Data_d);
				break;
			case MODE_FORWARD:
				rd = f_rd = SearchBF_RD(x, y, MotionFlags, pParam, &best_rd, Data_f) + 1*BITS_MULT; /* extra one bit for FORWARD vs BACKWARD */
				break;
			case MODE_BACKWARD:
				rd = b_rd = SearchBF_RD(x, y, MotionFlags, pParam, &best_rd, Data_b);
				break;
			default:
			case MODE_INTERPOLATE:
				rd = i_rd = SearchInterpolate_RD(x, y, MotionFlags, pParam, &best_rd, Data_i);
				break;
			}
			if (rd < best_rd) {
				mode = order[i];
				best_rd = rd;
			}
		}
	} else {
		/* only 1 mode is below the threshold */
		mode = order[0];
		best_rd = 0;
	}

	pMB->sad16 = best_rd;
	pMB->mode = mode;

	switch (mode) {

	case MODE_DIRECT:
		if (!qpel && b_mb->mode != MODE_INTER4V) pMB->mode = MODE_DIRECT_NO4V; /* for faster compensation */

		pMB->pmvs[3] = Data_d->currentMV[0];

		pMB->cbp = *Data_d->cbp;

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
		pMB->cbp = *Data_f->cbp;
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
		pMB->cbp = *Data_b->cbp;
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
		pMB->cbp = *Data_i->cbp;
		break;
	}
}
