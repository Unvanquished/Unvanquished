/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Prediction module -
 *
 *  Copyright (C) 2001-2003 Michael Militzer <isibaar@xvid.org>
 *                2001-2003 Peter Ross <pross@xvid.org>
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
 * $Id: mbprediction.c,v 1.18 2005/11/22 10:23:01 suxen_drol Exp $
 *
 ****************************************************************************/

#include <stdlib.h>

#include "../global.h"
#include "../encoder.h"
#include "mbprediction.h"
#include "../utils/mbfunctions.h"
#include "../bitstream/cbp.h"
#include "../bitstream/mbcoding.h"
#include "../bitstream/zigzag.h"


static int __inline
rescale(int predict_quant,
		int current_quant,
		int coeff)
{
	return (coeff != 0) ? DIV_DIV((coeff) * (predict_quant),
								  (current_quant)) : 0;
}


static const int16_t default_acdc_values[15] = {
	1024,
	0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0
};


/*	get dc/ac prediction direction for a single block and place
	predictor values into MB->pred_values[j][..]
*/


void
predict_acdc(MACROBLOCK * pMBs,
			 uint32_t x,
			 uint32_t y,
			 uint32_t mb_width,
			 uint32_t block,
			 int16_t qcoeff[64],
			 uint32_t current_quant,
			 int32_t iDcScaler,
			 int16_t predictors[8],
			 const int bound)

{
	const int mbpos = (y * mb_width) + x;
	int16_t *left, *top, *diag, *current;

	int32_t left_quant = current_quant;
	int32_t top_quant = current_quant;

	const int16_t *pLeft = default_acdc_values;
	const int16_t *pTop = default_acdc_values;
	const int16_t *pDiag = default_acdc_values;

	uint32_t index = x + y * mb_width;	/* current macroblock */
	int *acpred_direction = &pMBs[index].acpred_directions[block];
	uint32_t i;

	left = top = diag = current = NULL;

	/* grab left,top and diag macroblocks */

	/* left macroblock */

	if (x && mbpos >= bound + 1  &&
		(pMBs[index - 1].mode == MODE_INTRA ||
		 pMBs[index - 1].mode == MODE_INTRA_Q)) {

		left = (int16_t*)pMBs[index - 1].pred_values[0];
		left_quant = pMBs[index - 1].quant;
	}
	/* top macroblock */

	if (mbpos >= bound + (int)mb_width &&
		(pMBs[index - mb_width].mode == MODE_INTRA ||
		 pMBs[index - mb_width].mode == MODE_INTRA_Q)) {

		top = (int16_t*)pMBs[index - mb_width].pred_values[0];
		top_quant = pMBs[index - mb_width].quant;
	}
	/* diag macroblock */

	if (x && mbpos >= bound + (int)mb_width + 1 &&
		(pMBs[index - 1 - mb_width].mode == MODE_INTRA ||
		 pMBs[index - 1 - mb_width].mode == MODE_INTRA_Q)) {

		diag = (int16_t*)pMBs[index - 1 - mb_width].pred_values[0];
	}

	current = (int16_t*)pMBs[index].pred_values[0];

	/* now grab pLeft, pTop, pDiag _blocks_ */

	switch (block) {

	case 0:
		if (left)
			pLeft = left + MBPRED_SIZE;

		if (top)
			pTop = top + (MBPRED_SIZE << 1);

		if (diag)
			pDiag = diag + 3 * MBPRED_SIZE;

		break;

	case 1:
		pLeft = current;
		left_quant = current_quant;

		if (top) {
			pTop = top + 3 * MBPRED_SIZE;
			pDiag = top + (MBPRED_SIZE << 1);
		}
		break;

	case 2:
		if (left) {
			pLeft = left + 3 * MBPRED_SIZE;
			pDiag = left + MBPRED_SIZE;
		}

		pTop = current;
		top_quant = current_quant;

		break;

	case 3:
		pLeft = current + (MBPRED_SIZE << 1);
		left_quant = current_quant;

		pTop = current + MBPRED_SIZE;
		top_quant = current_quant;

		pDiag = current;

		break;

	case 4:
		if (left)
			pLeft = left + (MBPRED_SIZE << 2);
		if (top)
			pTop = top + (MBPRED_SIZE << 2);
		if (diag)
			pDiag = diag + (MBPRED_SIZE << 2);
		break;

	case 5:
		if (left)
			pLeft = left + 5 * MBPRED_SIZE;
		if (top)
			pTop = top + 5 * MBPRED_SIZE;
		if (diag)
			pDiag = diag + 5 * MBPRED_SIZE;
		break;
	}

	/* determine ac prediction direction & ac/dc predictor place rescaled ac/dc
	 * predictions into predictors[] for later use */
	if (abs(pLeft[0] - pDiag[0]) < abs(pDiag[0] - pTop[0])) {
		*acpred_direction = 1;	/* vertical */
		predictors[0] = DIV_DIV(pTop[0], iDcScaler);
		for (i = 1; i < 8; i++) {
			predictors[i] = rescale(top_quant, current_quant, pTop[i]);
		}
	} else {
		*acpred_direction = 2;	/* horizontal */
		predictors[0] = DIV_DIV(pLeft[0], iDcScaler);
		for (i = 1; i < 8; i++) {
			predictors[i] = rescale(left_quant, current_quant, pLeft[i + 7]);
		}
	}
}


/* decoder: add predictors to dct_codes[] and
   store current coeffs to pred_values[] for future prediction
*/

/* Up to this version, no DC clipping was performed, so we try to be backward
 * compatible to avoid artifacts */
#define BS_VERSION_BUGGY_DC_CLIPPING 34

void
add_acdc(MACROBLOCK * pMB,
		 uint32_t block,
		 int16_t dct_codes[64],
		 uint32_t iDcScaler,
		 int16_t predictors[8],
		 const int bsversion)
{
	uint8_t acpred_direction = pMB->acpred_directions[block];
	int16_t *pCurrent = (int16_t*)pMB->pred_values[block];
	uint32_t i;

	DPRINTF(XVID_DEBUG_COEFF,"predictor[0] %i\n", predictors[0]);

	dct_codes[0] += predictors[0];	/* dc prediction */
	pCurrent[0] = dct_codes[0]*iDcScaler;
	if (!bsversion || bsversion > BS_VERSION_BUGGY_DC_CLIPPING) {
		pCurrent[0] = CLIP(pCurrent[0], -2048, 2047);
	}

	if (acpred_direction == 1) {
		for (i = 1; i < 8; i++) {
			int level = dct_codes[i] + predictors[i];

			DPRINTF(XVID_DEBUG_COEFF,"predictor[%i] %i\n",i, predictors[i]);

			dct_codes[i] = level;
			pCurrent[i] = level;
			pCurrent[i + 7] = dct_codes[i * 8];
		}
	} else if (acpred_direction == 2) {
		for (i = 1; i < 8; i++) {
			int level = dct_codes[i * 8] + predictors[i];
			DPRINTF(XVID_DEBUG_COEFF,"predictor[%i] %i\n",i*8, predictors[i]);

			dct_codes[i * 8] = level;
			pCurrent[i + 7] = level;
			pCurrent[i] = dct_codes[i];
		}
	} else {
		for (i = 1; i < 8; i++) {
			pCurrent[i] = dct_codes[i];
			pCurrent[i + 7] = dct_codes[i * 8];
		}
	}
}



/*****************************************************************************
 ****************************************************************************/

/* encoder: subtract predictors from qcoeff[] and calculate S1/S2

returns sum of coeefficients *saved* if prediction is enabled

S1 = sum of all (qcoeff - prediction)
S2 = sum of all qcoeff
*/

static int
calc_acdc_coeff(MACROBLOCK * pMB,
		  uint32_t block,
		  int16_t qcoeff[64],
		  uint32_t iDcScaler,
		  int16_t predictors[8])
{
	int16_t *pCurrent = (int16_t*)pMB->pred_values[block];
	uint32_t i;
	int S1 = 0, S2 = 0;


	/* store current coeffs to pred_values[] for future prediction */

	pCurrent[0] = qcoeff[0] * iDcScaler;
	pCurrent[0] = CLIP(pCurrent[0], -2048, 2047);
	for (i = 1; i < 8; i++) {
		pCurrent[i] = qcoeff[i];
		pCurrent[i + 7] = qcoeff[i * 8];
	}

	/* subtract predictors and store back in predictors[] */

	qcoeff[0] = qcoeff[0] - predictors[0];

	if (pMB->acpred_directions[block] == 1) {
		for (i = 1; i < 8; i++) {
			int16_t level;

			level = qcoeff[i];
			S2 += abs(level);
			level -= predictors[i];
			S1 += abs(level);
			predictors[i] = level;
		}
	} else						/* acpred_direction == 2 */
	{
		for (i = 1; i < 8; i++) {
			int16_t level;

			level = qcoeff[i * 8];
			S2 += abs(level);
			level -= predictors[i];
			S1 += abs(level);
			predictors[i] = level;
		}

	}


	return S2 - S1;
}



/* returns the bits *saved* if prediction is enabled */

static int
calc_acdc_bits(MACROBLOCK * pMB,
		  uint32_t block,
		  int16_t qcoeff[64],
		  uint32_t iDcScaler,
		  int16_t predictors[8])
{
	const int direction = pMB->acpred_directions[block];
	int16_t *pCurrent = (int16_t*)pMB->pred_values[block];
	int16_t tmp[8];
	unsigned int i;
	int Z1, Z2;

	/* store current coeffs to pred_values[] for future prediction */
	pCurrent[0] = qcoeff[0] * iDcScaler;
	pCurrent[0] = CLIP(pCurrent[0], -2048, 2047);
	for (i = 1; i < 8; i++) {
		pCurrent[i] = qcoeff[i];
		pCurrent[i + 7] = qcoeff[i * 8];
	}


	/* dc prediction */
	qcoeff[0] = qcoeff[0] - predictors[0];

	/* calc cost before ac prediction */
	Z2 = CodeCoeffIntra_CalcBits(qcoeff, scan_tables[0]);

	/* apply ac prediction & calc cost*/
	if (direction == 1) {
		for (i = 1; i < 8; i++) {
			tmp[i] = qcoeff[i];
			qcoeff[i] -= predictors[i];
			predictors[i] = qcoeff[i];
		}
	}else{						/* acpred_direction == 2 */
		for (i = 1; i < 8; i++) {
			tmp[i] = qcoeff[i*8];
			qcoeff[i*8] -= predictors[i];
			predictors[i] = qcoeff[i*8];
		}
	}

	Z1 = CodeCoeffIntra_CalcBits(qcoeff, scan_tables[direction]);

	/* undo prediction */
	if (direction == 1) {
		for (i = 1; i < 8; i++)
			qcoeff[i] = tmp[i];
	}else{						/* acpred_direction == 2 */
		for (i = 1; i < 8; i++)
			qcoeff[i*8] = tmp[i];
	}

	return Z2-Z1;
}

/* apply predictors[] to qcoeff */

static void
apply_acdc(MACROBLOCK * pMB,
		   uint32_t block,
		   int16_t qcoeff[64],
		   int16_t predictors[8])
{
	unsigned int i;

	if (pMB->acpred_directions[block] == 1) {
		for (i = 1; i < 8; i++)
			qcoeff[i] = predictors[i];
	} else {
		for (i = 1; i < 8; i++)
			qcoeff[i * 8] = predictors[i];
	}
}


void
MBPrediction(FRAMEINFO * frame,
			 uint32_t x,
			 uint32_t y,
			 uint32_t mb_width,
			 int16_t qcoeff[6 * 64])
{

	int32_t j;
	int32_t iDcScaler, iQuant;
	int S = 0;
	int16_t predictors[6][8];

	MACROBLOCK *pMB = &frame->mbs[x + y * mb_width];
    iQuant = pMB->quant;

	if ((pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q)) {

		for (j = 0; j < 6; j++) {
			iDcScaler = get_dc_scaler(iQuant, j<4);

			predict_acdc(frame->mbs, x, y, mb_width, j, &qcoeff[j * 64],
						 iQuant, iDcScaler, predictors[j], 0);

			if ((frame->vop_flags & XVID_VOP_HQACPRED))
				S += calc_acdc_bits(pMB, j, &qcoeff[j * 64], iDcScaler, predictors[j]);
			else
				S += calc_acdc_coeff(pMB, j, &qcoeff[j * 64], iDcScaler, predictors[j]);

		}

		if (S<=0) {				/* dont predict */
			for (j = 0; j < 6; j++)
				pMB->acpred_directions[j] = 0;
		}else{
			for (j = 0; j < 6; j++)
				apply_acdc(pMB, j, &qcoeff[j * 64], predictors[j]);
		}

		pMB->cbp = calc_cbp(qcoeff);
	}
}

static const VECTOR zeroMV = { 0, 0 };

VECTOR
get_pmv2(const MACROBLOCK * const mbs,
		const int mb_width,
		const int bound,
		const int x,
		const int y,
		const int block)
{
	int lx, ly, lz;		/* left */
	int tx, ty, tz;		/* top */
	int rx, ry, rz;		/* top-right */
	int lpos, tpos, rpos;
	int num_cand = 0, last_cand = 1;

	VECTOR pmv[4];	/* left neighbour, top neighbour, top-right neighbour */

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

	lpos = lx + ly * mb_width;
	rpos = rx + ry * mb_width;
	tpos = tx + ty * mb_width;

	if (lpos >= bound && lx >= 0) {
		num_cand++;
		pmv[1] = mbs[lpos].mvs[lz];
	} else pmv[1] = zeroMV;

	if (tpos >= bound) {
		num_cand++;
		last_cand = 2;
		pmv[2] = mbs[tpos].mvs[tz];
	} else pmv[2] = zeroMV;

	if (rpos >= bound && rx < mb_width) {
		num_cand++;
		last_cand = 3;
		pmv[3] = mbs[rpos].mvs[rz];
	} else pmv[3] = zeroMV;

	/* If there're more than one candidate, we return the median vector */

	if (num_cand > 1) {
		/* set median */
		pmv[0].x =
			MIN(MAX(pmv[1].x, pmv[2].x),
				MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
		pmv[0].y =
			MIN(MAX(pmv[1].y, pmv[2].y),
				MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
		return pmv[0];
	}

	return pmv[last_cand];	/* no point calculating median mv */
}

VECTOR get_pmv2_interlaced(const MACROBLOCK * const mbs,
   const int mb_width,
   const int bound,
   const int x,
   const int y,
   const int block)
{
  int lx, ly, lz;   /* left */
  int tx, ty, tz;   /* top */
  int rx, ry, rz;   /* top-right */
  int lpos, tpos, rpos;
  int num_cand = 0, last_cand = 1;

  VECTOR pmv[4];  /* left neighbour, top neighbour, top-right neighbour */

  lx=x-1; ly=y;   lz=1;
  tx=x;   ty=y-1; tz=2;
  rx=x+1; ry=y-1; rz=2;

  lpos=lx+ly*mb_width;
  rpos=rx+ry*mb_width;
  tpos=tx+ty*mb_width;

  if(lx>=0 && lpos>=bound) 
  {
    num_cand++;
    if(mbs[lpos].field_pred)
     pmv[1] = mbs[lpos].mvs_avg;
    else 
     pmv[1] = mbs[lpos].mvs[lz];
  }
  else 
  {
    pmv[1] = zeroMV;
  }  

  if(tpos>=bound) 
  {
    num_cand++;
    last_cand=2;
    if(mbs[tpos].field_pred)
     pmv[2] = mbs[tpos].mvs_avg;
    else
     pmv[2] = mbs[tpos].mvs[tz];
  } 
  else
  { 
    pmv[2] = zeroMV;
  }
        
  if(rx<mb_width && rpos>=bound) 
  {
    num_cand++;
    last_cand = 3;
    if(mbs[rpos].field_pred)
     pmv[3] = mbs[rpos].mvs_avg;
    else
     pmv[3] = mbs[rpos].mvs[rz];
  } 
  else
  { 
    pmv[3] = zeroMV;
  }  

  /* If there're more than one candidate, we return the median vector */
  if(num_cand>1) 
  {
    /* set median */
    pmv[0].x = MIN(MAX(pmv[1].x, pmv[2].x),
               MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
    pmv[0].y = MIN(MAX(pmv[1].y, pmv[2].y),
               MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
          
    return pmv[0];
  }

  return pmv[last_cand];  /* no point calculating median mv */
}

VECTOR
get_qpmv2(const MACROBLOCK * const mbs,
		const int mb_width,
		const int bound,
		const int x,
		const int y,
		const int block)
{
	int lx, ly, lz;		/* left */
	int tx, ty, tz;		/* top */
	int rx, ry, rz;		/* top-right */
	int lpos, tpos, rpos;
	int num_cand = 0, last_cand = 1;

	VECTOR pmv[4];	/* left neighbour, top neighbour, top-right neighbour */

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

	lpos = lx + ly * mb_width;
	rpos = rx + ry * mb_width;
	tpos = tx + ty * mb_width;

	if (lpos >= bound && lx >= 0) {
		num_cand++;
		pmv[1] = mbs[lpos].qmvs[lz];
	} else pmv[1] = zeroMV;

	if (tpos >= bound) {
		num_cand++;
		last_cand = 2;
		pmv[2] = mbs[tpos].qmvs[tz];
	} else pmv[2] = zeroMV;

	if (rpos >= bound && rx < mb_width) {
		num_cand++;
		last_cand = 3;
		pmv[3] = mbs[rpos].qmvs[rz];
	} else pmv[3] = zeroMV;

	/* If there're more than one candidate, we return the median vector */

	if (num_cand > 1) {
		/* set median */
		pmv[0].x =
			MIN(MAX(pmv[1].x, pmv[2].x),
				MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
		pmv[0].y =
			MIN(MAX(pmv[1].y, pmv[2].y),
				MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
		return pmv[0];
	}

	return pmv[last_cand];	/* no point calculating median mv */
}
