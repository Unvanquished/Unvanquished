/*****************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	- MPEG4 Quantization MPEG implementation with altivec optimization -
 *
 *  Copyright(C) 2005 Christoph Naegeli <chn@kbw.ch>
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
 * $Id: quant_mpeg_altivec.c,v 1.1 2005/04/04 23:49:37 edgomez Exp $
 *
 ****************************************************************************/

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"
#include "../../global.h"

#include "../quant.h"
#include "../quant_matrix.h"

#undef DEBUG
#include <stdio.h>

/* Some useful typedefs */
typedef vector unsigned char vec_uint8_t;
typedef vector unsigned short vec_uint16_t;
typedef vector unsigned int vec_uint32_t;

typedef vector signed char vec_sint8_t;
typedef vector signed short vec_sint16_t;
typedef vector signed int vec_sint32_t;

/*****************************************************************************
 * Local data
 ****************************************************************************/

#define VM18P 3
#define VM18Q 4

/* divide-by-multiply table
 * needs 17 bit shift (16 causes slight errors when q > 19) */

#define SCALEBITS 17

#define DEQUANT_MPEG_INTRA() \
	level = vec_perm(vec_ld(0,coeff_ptr),vec_ld(16,coeff_ptr),vec_lvsl(0,coeff_ptr));\
	zero_less = vec_cmplt(level,ox00);\
	level = vec_abs(level);\
	vintra = vec_perm(vec_ld(0,intra_matrix),vec_ld(16,intra_matrix),vec_lvsl(0,intra_matrix));\
	even = vec_mule((vec_uint16_t)level,vintra);\
	odd = vec_mulo((vec_uint16_t)level,vintra);\
	t = vec_splat_u32(-16);\
	et = vec_msum( (vec_uint16_t)even, (vec_uint16_t)swap, (vec_uint32_t)ox00);\
	ot = vec_msum( (vec_uint16_t)odd, (vec_uint16_t)swap, (vec_uint32_t)ox00);\
	et = vec_sl(et, t);\
	ot = vec_sl(ot, t);\
	even = vec_mulo( (vec_uint16_t)even, (vec_uint16_t)vquant);\
	odd = vec_mulo( (vec_uint16_t)odd, (vec_uint16_t)vquant);\
	t = vec_splat_u32(3);\
	even = vec_add(even,et);\
	odd = vec_add(odd,ot);\
	even = vec_sr(even,t);\
	odd = vec_sr(odd,t);\
	/* Pack & Clamp to [-2048,2047] */\
	level = vec_packs( (vec_sint32_t)vec_mergeh(even,odd), (vec_sint32_t)vec_mergel(even,odd) );\
	t = (vec_uint32_t)vec_splat_s16(-1);\
	overflow = vec_cmpgt(level, vec_add(vec_2048, (vec_sint16_t)t));\
	level = vec_sel(level, vec_2048, overflow);\
	overflow = (vector bool short)vec_and(overflow, vec_xor(zero_less, (vec_sint16_t)t));\
	t = (vec_uint32_t)vec_splat_s16(1);\
	overflow = (vector bool short)vec_and(overflow, (vec_sint16_t)t);\
	level = vec_sub(level, (vec_sint16_t)overflow);\
	/* Invert the negative ones */\
	level = vec_xor(level, zero_less);\
	level = vec_add(level, (vec_sint16_t)vec_and(zero_less, (vec_uint16_t)t));\
	/* Save & Advance Pointers */\
	vec_st(level,0,data_ptr);\
	data_ptr += 8;\
	intra_matrix += 8;\
	coeff_ptr += 8


/* This function assuems:
 *	data: 16 Byte aligned
 */

uint32_t
dequant_mpeg_intra_altivec_c(int16_t * data,
					 const int16_t * coeff,
					 const uint32_t quant,
					 const uint32_t dcscalar,
					 const uint16_t * mpeg_quant_matrices)
{
	register const uint16_t *intra_matrix = get_intra_matrix(mpeg_quant_matrices);
	register const int16_t *coeff_ptr = coeff;
	register int16_t *data_ptr = data;
	
	register vec_sint16_t ox00;
	register vec_sint16_t level;
	register vec_sint16_t vec_2048;
	register vec_uint16_t vintra;
	register vec_uint32_t swap;
	register vec_uint32_t even,odd;
	register vec_uint32_t et,ot,t;
	
	vec_uint32_t vquant;
	vector bool short zero_less;
	vector bool short overflow;
	
#ifdef DEBUG
	if((long)data & 0xf)
		fprintf(stderr, "xvidcore: error in dequant_mpeg_intra_altivec_c, incorrect align: %x\n", data);
#endif

	/* Initialize */
	ox00 = vec_splat_s16(0);
	*((uint32_t*)&vquant) = quant;
	vquant = vec_splat(vquant,0);
	
	swap = vec_rl(vquant, vec_splat_u32(-16));
	vec_2048 = (vec_sint16_t)vec_rl(vec_splat_u16(8),vec_splat_u16(8));
	
	DEQUANT_MPEG_INTRA();
	DEQUANT_MPEG_INTRA();
	DEQUANT_MPEG_INTRA();
	DEQUANT_MPEG_INTRA();
	
	DEQUANT_MPEG_INTRA();
	DEQUANT_MPEG_INTRA();
	DEQUANT_MPEG_INTRA();
	DEQUANT_MPEG_INTRA();
	
	/* Process the first */
	data[0] = coeff[0] * dcscalar;
	if (data[0] < -2048) {
		data[0] = -2048;
	} else if (data[0] > 2047) {
		data[0] = 2047;
	}
		
	return 0;
}



#define DEQUANT_MPEG_INTER() \
	level = vec_perm(vec_ld(0,coeff),vec_ld(16,coeff),vec_lvsl(0,coeff));\
	zero_eq = vec_cmpeq(level,ox00);\
	zero_less = vec_cmplt(level,ox00);\
	level = vec_abs(level);\
	vinter = vec_perm(vec_ld(0,inter_matrix),vec_ld(16,inter_matrix),vec_lvsl(0,inter_matrix));\
	t = vec_splat_u32(1);\
	hi = (vec_uint32_t)vec_unpackh(level);\
	lo = (vec_uint32_t)vec_unpackl(level);\
	hi = vec_sl(hi, t);\
	lo = vec_sl(lo, t);\
	hi = vec_add(hi, t);\
	lo = vec_add(lo, t);\
	/* Multiplication with vinter */\
	sw_hi = vec_rl(hi, v16);\
	sw_lo = vec_rl(lo, v16);\
	hi = vec_mulo((vec_uint16_t)hi, vec_mergeh((vec_uint16_t)ox00,vinter));\
	lo = vec_mulo((vec_uint16_t)lo, vec_mergel((vec_uint16_t)ox00,vinter));\
	sw_hi = vec_mulo((vec_uint16_t)sw_hi, vec_mergeh((vec_uint16_t)ox00,vinter));\
	sw_lo = vec_mulo((vec_uint16_t)sw_lo, vec_mergeh((vec_uint16_t)ox00,vinter));\
	hi = vec_add(hi, vec_sl(sw_hi,v16));\
	lo = vec_add(lo, vec_sl(sw_lo,v16));\
	/* Multiplication with Quant */\
	t = vec_splat_u32(4);\
	sw_hi = vec_msum( (vec_uint16_t)hi, (vec_uint16_t)swap, (vec_uint32_t)ox00 );\
	sw_lo = vec_msum( (vec_uint16_t)lo, (vec_uint16_t)swap, (vec_uint32_t)ox00 );\
	hi = vec_mulo( (vec_uint16_t)hi, (vec_uint16_t)vquant );\
	lo = vec_mulo( (vec_uint16_t)lo, (vec_uint16_t)vquant );\
	hi = vec_add(hi, vec_sl(sw_hi,v16));\
	lo = vec_add(lo, vec_sl(sw_lo,v16));\
	hi = vec_sr(hi, t);\
	lo = vec_sr(lo, t);\
	/* Pack & Clamp to [-2048,2047] */\
	t = (vec_uint32_t)vec_splat_s16(-1);\
	level = vec_packs((vec_sint32_t)hi, (vec_sint32_t)lo);\
	overflow = vec_cmpgt(level, vec_add(v2048, (vec_sint16_t)t));\
	level = vec_sel(level, v2048, overflow);\
	overflow = (vector bool short)vec_and(overflow, vec_xor(zero_less, (vec_sint16_t)t));\
	t = (vec_uint32_t)vec_splat_s16(1);\
	overflow = (vector bool short)vec_and(overflow, (vec_sint16_t)t);\
	level = vec_sub(level, (vec_sint16_t)overflow);\
	level = vec_sel(level, ox00, zero_eq);\
	level = vec_xor(level, zero_less);\
	level = vec_add(level, (vec_sint16_t)vec_and(zero_less, (vec_uint16_t)t));\
	/* Get vsum */\
	vsum = vec_xor(vsum, (vec_uint32_t)vec_unpackh(level));\
	vsum = vec_xor(vsum, (vec_uint32_t)vec_unpackl(level));\
	/* Save & Advance Pointers */\
	vec_st(level,0,data);\
	data+=8;\
	inter_matrix+=8;\
	coeff+=8


/* This function assumes:
 *	data: 16 Byte aligned
 */

uint32_t
dequant_mpeg_inter_altivec_c(int16_t * data,
					 const int16_t * coeff,
					 const uint32_t quant,
					 const uint16_t * mpeg_quant_matrices)
{
	register uint32_t sum;
	register const uint16_t *inter_matrix = get_inter_matrix(mpeg_quant_matrices);
	
	register vec_sint16_t ox00;
	register vec_sint16_t v2048;
	register vec_sint16_t level;
	register vec_uint16_t vinter;
	register vec_uint32_t hi,lo;
	register vec_uint32_t sw_hi,sw_lo;
	register vec_uint32_t swap;
	register vec_uint32_t t,v16;
	
	vec_uint32_t vsum;
	vec_uint32_t vquant;
	vector bool short zero_eq;
	vector bool short zero_less;
	vector bool short overflow;
	
#ifdef DEBUG
	if((long)data & 0xf)
		fprintf(stderr, "xvidcore: error in dequant_mpeg_inter_altivec_c, incorrect align: %x\n", data);
#endif
	
	/* Initialization */
	ox00 = vec_splat_s16(0);
	v16 = vec_splat_u32(-16);
	v2048 = vec_rl(vec_splat_s16(8),vec_splat_u16(8));
	
	vsum = (vec_uint32_t)ox00;
	
	*((uint32_t*)&vquant) = quant;
	vquant = vec_splat(vquant,0);
	swap = vec_rl(vquant,v16);
	
	DEQUANT_MPEG_INTER();
	DEQUANT_MPEG_INTER();
	DEQUANT_MPEG_INTER();
	DEQUANT_MPEG_INTER();
	
	DEQUANT_MPEG_INTER();
	DEQUANT_MPEG_INTER();
	DEQUANT_MPEG_INTER();
	DEQUANT_MPEG_INTER();
	
	sum = ((uint32_t*)&vsum)[0];
	sum ^= ((uint32_t*)&vsum)[1];
	sum ^= ((uint32_t*)&vsum)[2];
	sum ^= ((uint32_t*)&vsum)[3];
	
	/* mismatch control */
	if((sum & 1) == 0) {
		data -= 1;
		*data ^= 1;
	}
	
	return 0;
}
