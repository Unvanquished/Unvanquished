/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Quantization matrix management code  -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
 *               2002 Peter Ross <pross@xvid.org>
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
 * $Id: quant_matrix.c,v 1.16 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#include "quant_matrix.h"

#define FIX(X)   (((X)==1) ? 0xFFFF : ((1UL << 16) / (X) + 1))
#define FIXL(X)    ((1UL << 16) / (X) - 1)

/*****************************************************************************
 * Default matrices
 ****************************************************************************/

static const uint8_t default_intra_matrix[64] = {
	 8, 17, 18, 19, 21, 23, 25, 27,
	17, 18, 19, 21, 23, 25, 27, 28,
	20, 21, 22, 23, 24, 26, 28, 30,
	21, 22, 23, 24, 26, 28, 30, 32,
	22, 23, 24, 26, 28, 30, 32, 35,
	23, 24, 26, 28, 30, 32, 35, 38,
	25, 26, 28, 30, 32, 35, 38, 41,
	27, 28, 30, 32, 35, 38, 41, 45
};

static const uint8_t default_inter_matrix[64] = {
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33
};

const uint16_t *
get_intra_matrix(const uint16_t * mpeg_quant_matrices)
{
	return(mpeg_quant_matrices + 0*64);
}

const uint16_t *
get_inter_matrix(const uint16_t * mpeg_quant_matrices)
{
	return(mpeg_quant_matrices + 4*64);
}

const uint8_t *
get_default_intra_matrix(void)
{
	return default_intra_matrix;
}

const uint8_t *
get_default_inter_matrix(void)
{
	return default_inter_matrix;
}

int
is_custom_intra_matrix(const uint16_t * mpeg_quant_matrices)
{
	int i;
	const uint16_t *intra_matrix = get_intra_matrix(mpeg_quant_matrices);
	const uint8_t *def_intra_matrix = get_default_intra_matrix();

	for (i = 0; i < 64; i++) {
		if(intra_matrix[i] != def_intra_matrix[i])
			return 1;
	}
	return 0;
}

int
is_custom_inter_matrix(const uint16_t * mpeg_quant_matrices)
{
	int i;
	const uint16_t *inter_matrix = get_inter_matrix(mpeg_quant_matrices);
	const uint8_t *def_inter_matrix = get_default_inter_matrix();

	for (i = 0; i < 64; i++) {
		if(inter_matrix[i] != (uint16_t)def_inter_matrix[i])
			return 1;
	}
	return 0;
}

void
set_intra_matrix(uint16_t * mpeg_quant_matrices, const uint8_t * matrix)
{
	int i;
	uint16_t *intra_matrix = mpeg_quant_matrices + 0*64;

	for (i = 0; i < 64; i++) {
		intra_matrix[i] = (!i) ? (uint16_t)8: (uint16_t)matrix[i];
	}
}

void
init_intra_matrix(uint16_t * mpeg_quant_matrices, uint32_t quant)
{
	int i;
	uint16_t *intra_matrix = mpeg_quant_matrices + 0*64;
	uint16_t *intra_matrix_rec = mpeg_quant_matrices + 1*64;

	for (i = 0; i < 64; i++) {
		uint32_t div = intra_matrix[i]*quant;
		intra_matrix_rec[i] = ((uint32_t)((1<<SCALEBITS) + (div>>1)))/div;
	}
}

void
set_inter_matrix(uint16_t * mpeg_quant_matrices, const uint8_t * matrix)
{
	int i;
	uint16_t *inter_matrix = mpeg_quant_matrices + 4*64;
	uint16_t *inter_matrix1 = mpeg_quant_matrices + 5*64;
	uint16_t *inter_matrix_fix = mpeg_quant_matrices + 6*64;
	uint16_t *inter_matrix_fixl = mpeg_quant_matrices + 7*64;

	for (i = 0; i < 64; i++) {
		inter_matrix1[i] = ((inter_matrix[i] = (int16_t) matrix[i])>>1);
		inter_matrix1[i] += ((inter_matrix[i] == 1) ? 1: 0);
		inter_matrix_fix[i] = FIX(inter_matrix[i]);
		inter_matrix_fixl[i] = FIXL(inter_matrix[i]);
	}
}

void
init_mpeg_matrix(uint16_t * mpeg_quant_matrices) {

	set_intra_matrix(mpeg_quant_matrices, default_intra_matrix);
	set_inter_matrix(mpeg_quant_matrices, default_inter_matrix);
}
