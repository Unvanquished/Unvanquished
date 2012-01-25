/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - MPEG4 Quantization related header  -
 *
 *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
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
 * $Id: quant_mpeg.c,v 1.5 2008/11/26 02:21:02 Isibaar Exp $
 *
 ****************************************************************************/

#include "../global.h"
#include "quant.h"
#include "quant_matrix.h"

/*****************************************************************************
 * Global function pointers
 ****************************************************************************/

/* Quant */
quant_intraFuncPtr quant_mpeg_intra;
quant_interFuncPtr quant_mpeg_inter;

/* DeQuant */
quant_intraFuncPtr dequant_mpeg_intra;
quant_interFuncPtr dequant_mpeg_inter;

/*****************************************************************************
 * Local data
 ****************************************************************************/

/* divide-by-multiply table
 * needs 17 bit shift (16 causes slight errors when q > 19) */

#define FIX(X)	  ((1UL << SCALEBITS) / (X) + 1)

static const uint32_t multipliers[32] =
{
	0,       FIX(2),  FIX(4),  FIX(6),
	FIX(8),	 FIX(10), FIX(12), FIX(14),
	FIX(16), FIX(18), FIX(20), FIX(22),
	FIX(24), FIX(26), FIX(28), FIX(30),
	FIX(32), FIX(34), FIX(36), FIX(38),
	FIX(40), FIX(42), FIX(44), FIX(46),
	FIX(48), FIX(50), FIX(52), FIX(54),
	FIX(56), FIX(58), FIX(60), FIX(62)
};

/*****************************************************************************
 * Function definitions
 ****************************************************************************/

/* quantize intra-block
 */

uint32_t
quant_mpeg_intra_c(int16_t * coeff,
				   const int16_t * data,
				   const uint32_t quant,
				   const uint32_t dcscalar,
				   const uint16_t * mpeg_quant_matrices)
{
	const uint16_t * intra_matrix_rec = mpeg_quant_matrices + 1*64;
	int i;
	int rounding = 1<<(SCALEBITS-1-3);

	coeff[0] = DIV_DIV(data[0], (int32_t) dcscalar);
	
	for (i = 1; i < 64; i++) {
		int32_t level = data[i];
		level *= intra_matrix_rec[i];
		level = (level + rounding)>>(SCALEBITS-3);
		coeff[i] = level;
	}

	return(0);
}

/* quantize inter-block
 *
 * level = DIV_DIV(16 * data[i], default_intra_matrix[i]);
 * coeff[i] = (level + quantd) / quant2;
 * sum += abs(level);
 */

uint32_t
quant_mpeg_inter_c(int16_t * coeff,
				   const int16_t * data,
				   const uint32_t quant,
				   const uint16_t * mpeg_quant_matrices)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t *inter_matrix = get_inter_matrix(mpeg_quant_matrices);
	uint32_t sum = 0;
	int i;

	for (i = 0; i < 64; i++) {
		if (data[i] < 0) {
			uint32_t level = -data[i];

			level = ((level << 4) + (inter_matrix[i] >> 1)) / inter_matrix[i];
			level = (level * mult) >> 17;
			sum += level;
			coeff[i] = -(int16_t) level;
		} else if (data[i] > 0) {
			uint32_t level = data[i];

			level = ((level << 4) + (inter_matrix[i] >> 1)) / inter_matrix[i];
			level = (level * mult) >> 17;
			sum += level;
			coeff[i] = level;
		} else {
			coeff[i] = 0;
		}
	}

	return(sum);
}

/* dequantize intra-block & clamp to [-2048,2047]
 *
 * data[i] = (coeff[i] * default_intra_matrix[i] * quant2) >> 4;
 */

uint32_t
dequant_mpeg_intra_c(int16_t * data,
					 const int16_t * coeff,
					 const uint32_t quant,
					 const uint32_t dcscalar,
					 const uint16_t * mpeg_quant_matrices)
{
	const uint16_t *intra_matrix = get_intra_matrix(mpeg_quant_matrices);
	int i;

	data[0] = coeff[0] * dcscalar;
	if (data[0] < -2048) {
		data[0] = -2048;
	} else if (data[0] > 2047) {
		data[0] = 2047;
	}

	for (i = 1; i < 64; i++) {
		if (coeff[i] == 0) {
			data[i] = 0;
		} else if (coeff[i] < 0) {
			uint32_t level = -coeff[i];

			level = (level * intra_matrix[i] * quant) >> 3;
			data[i] = (level <= 2048 ? -(int16_t) level : -2048);
		} else {
			uint32_t level = coeff[i];

			level = (level * intra_matrix[i] * quant) >> 3;
			data[i] = (level <= 2047 ? level : 2047);
		}
	}

	return(0);
}


/* dequantize inter-block & clamp to [-2048,2047]
 * data = ((2 * coeff + SIGN(coeff)) * inter_matrix[i] * quant) / 16
 */

uint32_t
dequant_mpeg_inter_c(int16_t * data,
					 const int16_t * coeff,
					 const uint32_t quant,
					 const uint16_t * mpeg_quant_matrices)
{
	uint32_t sum = 0;
	const uint16_t *inter_matrix = get_inter_matrix(mpeg_quant_matrices);
	int i;

	for (i = 0; i < 64; i++) {
		if (coeff[i] == 0) {
			data[i] = 0;
		} else if (coeff[i] < 0) {
			int32_t level = -coeff[i];

			level = ((2 * level + 1) * inter_matrix[i] * quant) >> 4;
			data[i] = (level <= 2048 ? -level : -2048);
		} else {
			uint32_t level = coeff[i];

			level = ((2 * level + 1) * inter_matrix[i] * quant) >> 4;
			data[i] = (level <= 2047 ? level : 2047);
		}

		sum ^= data[i];
	}

	/*	mismatch control */
	if ((sum & 1) == 0) {
		data[63] ^= 1;
	}

	return(0);
}
