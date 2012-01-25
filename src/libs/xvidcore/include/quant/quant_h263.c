/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - MPEG4 Quantization H263 implementation -
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
 * $Id: quant_h263.c,v 1.8 2004/03/22 22:36:24 edgomez Exp $
 *
 ****************************************************************************/

#include "../global.h"
#include "quant.h"

/*****************************************************************************
 * Global function pointers
 ****************************************************************************/

/* Quant */
quant_intraFuncPtr quant_h263_intra;
quant_interFuncPtr quant_h263_inter;

/* DeQuant */
quant_intraFuncPtr dequant_h263_intra;
quant_interFuncPtr dequant_h263_inter;

/*****************************************************************************
 * Local data
 ****************************************************************************/

/* divide-by-multiply table
 * a 16 bit shiting is enough in this case */

#define SCALEBITS	16
#define FIX(X)		((1L << SCALEBITS) / (X) + 1)

static const uint32_t multipliers[32] =
{
	0,       FIX(2),  FIX(4),  FIX(6),
	FIX(8),  FIX(10), FIX(12), FIX(14),
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

/*	quantize intra-block
 */

uint32_t
quant_h263_intra_c(int16_t * coeff,
				   const int16_t * data,
				   const uint32_t quant,
				   const uint32_t dcscalar,
				   const uint16_t * mpeg_quant_matrices)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t quant_m_2 = quant << 1;
	int i;

	coeff[0] = DIV_DIV(data[0], (int32_t) dcscalar);

	for (i = 1; i < 64; i++) {
		int16_t acLevel = data[i];

		if (acLevel < 0) {
			acLevel = -acLevel;
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}
			acLevel = (acLevel * mult) >> SCALEBITS;
			coeff[i] = -acLevel;
		} else {
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}
			acLevel = (acLevel * mult) >> SCALEBITS;
			coeff[i] = acLevel;
		}
	}

	return(0);
}


/*	quantize inter-block
 */

uint32_t
quant_h263_inter_c(int16_t * coeff,
				   const int16_t * data,
				   const uint32_t quant,
				   const uint16_t * mpeg_quant_matrices)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t quant_m_2 = quant << 1;
	const uint16_t quant_d_2 = quant >> 1;
	uint32_t sum = 0;
	uint32_t i;

	for (i = 0; i < 64; i++) {
		int16_t acLevel = data[i];

		if (acLevel < 0) {
			acLevel = (-acLevel) - quant_d_2;
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}

			acLevel = (acLevel * mult) >> SCALEBITS;
			sum += acLevel;		/* sum += |acLevel| */
			coeff[i] = -acLevel;
		} else {
			acLevel -= quant_d_2;
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}
			acLevel = (acLevel * mult) >> SCALEBITS;
			sum += acLevel;
			coeff[i] = acLevel;
		}
	}

	return(sum);
}


/*	dequantize intra-block & clamp to [-2048,2047]
 */

uint32_t
dequant_h263_intra_c(int16_t * data,
					 const int16_t * coeff,
					 const uint32_t quant,
					 const uint32_t dcscalar,
					 const uint16_t * mpeg_quant_matrices)
{
	const int32_t quant_m_2 = quant << 1;
	const int32_t quant_add = (quant & 1 ? quant : quant - 1);
	int i;

	data[0] = coeff[0] * dcscalar;
	if (data[0] < -2048) {
		data[0] = -2048;
	} else if (data[0] > 2047) {
		data[0] = 2047;
	}

	for (i = 1; i < 64; i++) {
		int32_t acLevel = coeff[i];

		if (acLevel == 0) {
			data[i] = 0;
		} else if (acLevel < 0) {
			acLevel = quant_m_2 * -acLevel + quant_add;
			data[i] = (acLevel <= 2048 ? -acLevel : -2048);
		} else {
			acLevel = quant_m_2 * acLevel + quant_add;
			data[i] = (acLevel <= 2047 ? acLevel : 2047);
		}
	}

	return(0);
}



/* dequantize inter-block & clamp to [-2048,2047]
 */

uint32_t
dequant_h263_inter_c(int16_t * data,
					 const int16_t * coeff,
					 const uint32_t quant,
					 const uint16_t * mpeg_quant_matrices)
{
	const uint16_t quant_m_2 = quant << 1;
	const uint16_t quant_add = (quant & 1 ? quant : quant - 1);
	int i;

	for (i = 0; i < 64; i++) {
		int16_t acLevel = coeff[i];

		if (acLevel == 0) {
			data[i] = 0;
		} else if (acLevel < 0) {
			acLevel = acLevel * quant_m_2 - quant_add;
			data[i] = (acLevel >= -2048 ? acLevel : -2048);
		} else {
			acLevel = acLevel * quant_m_2 + quant_add;
			data[i] = (acLevel <= 2047 ? acLevel : 2047);
		}
	}

	return(0);
}
