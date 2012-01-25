/*****************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	- MPEG4 Quantization H263 implementation with altivec optimization -
 *
 *  Copyright(C) 2004 Christoph Naegeli <chn@kbw.ch>
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
 * $Id: quant_h263_altivec.c,v 1.2 2004/12/09 23:02:54 edgomez Exp $
 *
 ****************************************************************************/

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"
#include "../../global.h"

#undef DEBUG
#include <stdio.h>


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
 
#define QUANT_H263_INTRA_ALTIVEC()  \
acLevel = vec_perm(vec_ld(0, data), vec_ld(16, data), vec_lvsl(0, data));   \
zero_mask = vec_cmplt(acLevel, (vector signed short)zerovec);               \
acLevel = vec_abs(acLevel);                                                 \
\
m2_mask = vec_cmpgt(quant_m_2, (vector unsigned short)acLevel);     \
acLevel = vec_sel(acLevel, (vector signed short)zerovec, m2_mask);  \
\
even = vec_mule(mult, (vector unsigned short)acLevel);  \
odd = vec_mulo(mult, (vector unsigned short)acLevel);   \
\
even = vec_sr(even, vec_add(vec_splat_u32(8), vec_splat_u32(8)));   \
odd = vec_sr(odd, vec_add(vec_splat_u32(8), vec_splat_u32(8)));     \
\
acLevel = (vector signed short)vec_pack(vec_mergeh(even, odd), vec_mergel(even, odd));  \
acLevel = vec_xor(acLevel, zero_mask);                                                  \
acLevel = vec_add(acLevel, vec_and(zero_mask, vec_splat_s16(1)));                       \
vec_st(acLevel, 0, coeff);                                                              \
\
coeff += 8; \
data += 8

/* This function assumes:
 *  coeff is 16 byte aligned
 *  data is unaligned
 */

uint32_t
quant_h263_intra_altivec_c(int16_t *coeff,
                                    int16_t *data,
                                    const uint32_t quant,
                                    const uint32_t dcscalar,
                                    const uint16_t *mpeg_quant_matrices)
{
    vector unsigned char zerovec;
    vector unsigned short mult;
    vector unsigned short quant_m_2;
    vector signed short acLevel;
    
    register vector unsigned int even;
    register vector unsigned int odd;
    
    vector bool short zero_mask;
    vector bool short m2_mask;
    
    register int16_t *origin_coeff = coeff;
    register int16_t *origin_data = data;

#ifdef DEBUG
    if(((unsigned)coeff) & 15)
        fprintf(stderr, "quant_h263_intra_altivec_c:incorrect align, coeff: %lx\n", (long)coeff);
#endif
    
    zerovec = vec_splat_u8(0);
    
    *((unsigned short*)&mult) = (unsigned short)multipliers[quant];
    mult = vec_splat(mult, 0);
    
    *((unsigned short*)&quant_m_2) = (unsigned short)quant;
    quant_m_2 = vec_splat(quant_m_2, 0);
    quant_m_2 = vec_sl(quant_m_2, vec_splat_u16(1));
    
    QUANT_H263_INTRA_ALTIVEC();
    QUANT_H263_INTRA_ALTIVEC();
    QUANT_H263_INTRA_ALTIVEC();
    QUANT_H263_INTRA_ALTIVEC();
    
    QUANT_H263_INTRA_ALTIVEC();
    QUANT_H263_INTRA_ALTIVEC();
    QUANT_H263_INTRA_ALTIVEC();
    QUANT_H263_INTRA_ALTIVEC();
    
    // noch erstes setzen
    origin_coeff[0] = DIV_DIV(origin_data[0], (int32_t)dcscalar);
    
    return 0;
}


#define QUANT_H263_INTER_ALTIVEC() \
acLevel = vec_perm(vec_ld(0, data), vec_ld(16, data), vec_lvsl(0, data));           \
zero_mask = vec_cmplt(acLevel, (vector signed short)zerovec);                       \
acLevel = vec_abs(acLevel);                                                         \
acLevel = (vector signed short)vec_sub((vector unsigned short)acLevel, quant_d_2);  \
\
m2_mask = vec_cmpgt((vector signed short)quant_m_2, acLevel);       \
acLevel = vec_sel(acLevel, (vector signed short)zerovec, m2_mask);  \
\
even = vec_mule((vector unsigned short)acLevel, mult);  \
odd = vec_mulo((vector unsigned short)acLevel, mult);   \
\
even = vec_sr(even, vec_add(vec_splat_u32(8), vec_splat_u32(8)));   \
odd = vec_sr(odd, vec_add(vec_splat_u32(8), vec_splat_u32(8)));     \
\
acLevel = (vector signed short)vec_pack(vec_mergeh(even, odd), vec_mergel(even, odd));  \
sum_short = vec_add(sum_short, (vector unsigned short)acLevel);                         \
\
acLevel = vec_xor(acLevel, zero_mask);                              \
acLevel = vec_add(acLevel, vec_and(zero_mask, vec_splat_s16(1)));   \
\
vec_st(acLevel, 0, coeff);  \
\
coeff += 8; \
data += 8

/* This function assumes:
 *  coeff is 16 byte aligned
 *  data is unaligned
 */

uint32_t
quant_h263_inter_altivec_c(int16_t *coeff,
                            int16_t *data,
                            const uint32_t quant,
                            const uint16_t *mpeg_quant_matrices)
{
    vector unsigned char zerovec;
    vector unsigned short mult;
    vector unsigned short quant_m_2;
    vector unsigned short quant_d_2;
    vector unsigned short sum_short;
    vector signed short acLevel;
    
    vector unsigned int even;
    vector unsigned int odd;
    
    vector bool short m2_mask;
    vector bool short zero_mask;
    
    uint32_t result;

#ifdef DEBUG
    if(((unsigned)coeff) & 0x15)
        fprintf(stderr, "quant_h263_inter_altivec_c:incorrect align, coeff: %lx\n", (long)coeff);
#endif
    
    /* initialisation stuff */
    zerovec = vec_splat_u8(0);
    *((unsigned short*)&mult) = (unsigned short)multipliers[quant];
    mult = vec_splat(mult, 0);
    *((unsigned short*)&quant_m_2) = (unsigned short)quant;
    quant_m_2 = vec_splat(quant_m_2, 0);
    quant_m_2 = vec_sl(quant_m_2, vec_splat_u16(1));
    *((unsigned short*)&quant_d_2) = (unsigned short)quant;
    quant_d_2 = vec_splat(quant_d_2, 0);
    quant_d_2 = vec_sr(quant_d_2, vec_splat_u16(1));
    sum_short = (vector unsigned short)zerovec;
    
    /* Quantize */
    QUANT_H263_INTER_ALTIVEC();
    QUANT_H263_INTER_ALTIVEC();
    QUANT_H263_INTER_ALTIVEC();
    QUANT_H263_INTER_ALTIVEC();
    
    QUANT_H263_INTER_ALTIVEC();
    QUANT_H263_INTER_ALTIVEC();
    QUANT_H263_INTER_ALTIVEC();
    QUANT_H263_INTER_ALTIVEC();
        
    /* Calculate the return value */
    even = (vector unsigned int)vec_sum4s((vector signed short)sum_short, (vector signed int)zerovec);
    even = (vector unsigned int)vec_sums((vector signed int)even, (vector signed int)zerovec);
    even = vec_splat(even, 3);
    vec_ste(even, 0, &result);
    return result;
}



/*	dequantize intra-block & clamp to [-2048,2047]
 */


#define DEQUANT_H263_INTRA_ALTIVEC() \
acLevel = vec_perm(vec_ld(0,coeff_ptr), vec_ld(16,coeff_ptr), vec_lvsl(0,coeff_ptr)); \
equal_zero = vec_cmpeq(acLevel, (vector signed short)zerovec); \
less_zero = vec_cmplt(acLevel, (vector signed short)zerovec);   \
acLevel = vec_abs(acLevel); \
\
even = vec_mule((vector unsigned short)acLevel, quant_m_2); \
odd = vec_mulo((vector unsigned short)acLevel, quant_m_2);  \
\
high = vec_mergeh(even,odd);    \
low = vec_mergel(even,odd);     \
\
t = vec_sel(quant_add, (vector unsigned short)zerovec, equal_zero); \
high = vec_add(high, (vector unsigned int)vec_mergeh((vector unsigned short)zerovec, t)); \
low = vec_add(low, (vector unsigned int)vec_mergel((vector unsigned short)zerovec, t));   \
\
acLevel = vec_packs((vector signed int)high, (vector signed int)low); \
\
overflow = vec_cmpgt(acLevel, vec_2048);        \
acLevel = vec_sel(acLevel, vec_2048, overflow); \
overflow = (vector bool short)vec_and(overflow, vec_xor(less_zero, vec_splat_s16(-1))); \
overflow = (vector bool short)vec_and(overflow, vec_splat_s16(1));                      \
acLevel = vec_sub(acLevel, (vector signed short)overflow);                              \
\
acLevel = vec_xor(acLevel, less_zero);  \
acLevel = vec_add(acLevel, vec_and(less_zero, vec_splat_s16(1)));   \
\
vec_st(acLevel, 0, data_ptr); \
\
data_ptr += 8; \
coeff_ptr += 8

/* This function assumes:
 *  data is 16 byte aligned
 *  coeff is unaligned
 */

uint32_t
dequant_h263_intra_altivec_c(int16_t *data,
                        const int16_t *coeff,
                        const uint32_t quant,
                        const uint32_t dcscalar,
                        const uint16_t *mpeg_quant_matrices)
{
    vector signed short acLevel;
    vector signed short vec_2048;
    vector unsigned short quant_add;
    vector unsigned short quant_m_2;
    vector unsigned short t;
    
    vector bool short equal_zero;
    vector bool short less_zero;
    vector bool short overflow;
    
    register vector unsigned int even;
    register vector unsigned int odd;
    register vector unsigned int high;
    register vector unsigned int low;
    
    register vector unsigned char zerovec;
    
    register int16_t *data_ptr;
    register int16_t *coeff_ptr;

#ifdef DEBUG
    if(((unsigned)data) & 0x15)
        fprintf(stderr, "dequant_h263_intra_altivec_c:incorrect align, data: %lx\n", (long)data);
#endif

    /* initialize */
    *((unsigned short*)&quant_add) = (unsigned short)(quant & 1 ? quant : quant - 1);
    quant_add = vec_splat(quant_add,0);
    
    *((unsigned short*)&quant_m_2) = (unsigned short)(quant << 1);
    quant_m_2 = vec_splat(quant_m_2,0);
    
    vec_2048 = vec_sl(vec_splat_s16(1), vec_splat_u16(11));
    zerovec = vec_splat_u8(0);
    
    data_ptr = (int16_t*)data;
    coeff_ptr = (int16_t*)coeff;
    
    /* dequant */
    DEQUANT_H263_INTRA_ALTIVEC();
    DEQUANT_H263_INTRA_ALTIVEC();
    DEQUANT_H263_INTRA_ALTIVEC();
    DEQUANT_H263_INTRA_ALTIVEC();
    
    DEQUANT_H263_INTRA_ALTIVEC();
    DEQUANT_H263_INTRA_ALTIVEC();
    DEQUANT_H263_INTRA_ALTIVEC();
    DEQUANT_H263_INTRA_ALTIVEC();
        
    /* data[0] is special */
    data[0] = coeff[0] * dcscalar;
    if(data[0] < -2048)
        data[0] = -2048;
    else if(data[0] > 2047)
        data[0] = 2047;
    
    return 0;
}


/* dequantize inter-block & clamp to [-2048,2047]
 */

#define DEQUANT_H263_INTER_ALTIVEC() \
acLevel = vec_perm(vec_ld(0,coeff), vec_ld(16,coeff), vec_lvsl(0,coeff));   \
equal_zero = vec_cmpeq(acLevel, (vector signed short)zerovec);              \
less_zero = vec_cmplt(acLevel, (vector signed short)zerovec);               \
acLevel = vec_abs(acLevel);                                                 \
\
even = vec_mule((vector unsigned short)acLevel, quant_m_2); \
odd = vec_mulo((vector unsigned short)acLevel, quant_m_2);  \
high = vec_mergeh(even,odd);                                \
low = vec_mergel(even,odd);                                 \
\
t = vec_sel(quant_add, (vector unsigned short)zerovec, equal_zero);                         \
high = vec_add(high, (vector unsigned int)vec_mergeh((vector unsigned short)zerovec, t));   \
low = vec_add(low, (vector unsigned int)vec_mergel((vector unsigned short)zerovec, t));     \
acLevel = vec_packs((vector signed int)high, (vector signed int)low);                       \
\
overflow = vec_cmpgt(acLevel,vec_2048);         \
acLevel = vec_sel(acLevel, vec_2048, overflow); \
overflow = (vector bool short)vec_and(overflow, vec_xor(less_zero, vec_splat_s16(-1))); \
overflow = (vector bool short)vec_and(overflow, vec_splat_s16(1));                      \
acLevel = vec_sub(acLevel, (vector signed short)overflow);                              \
\
acLevel = vec_xor(acLevel, less_zero);                              \
acLevel = vec_add(acLevel, vec_and(less_zero, vec_splat_s16(1)));   \
\
vec_st(acLevel, 0, data);   \
data += 8;                  \
coeff += 8


/* This function assumes:
 *  data is 16 byte aligned
 *  coeff is unaligned
 */

uint32_t
dequant_h263_inter_altivec_c(int16_t *data,
                                int16_t *coeff,
                                const uint32_t quant,
                                const uint16_t *mpeg_quant_matrices)
{
    vector signed short acLevel;
    vector signed short vec_2048;
    
    vector unsigned short quant_m_2;
    vector unsigned short quant_add;
    vector unsigned short t;
    
    register vector unsigned int even;
    register vector unsigned int odd;
    register vector unsigned int high;
    register vector unsigned int low;
    
    register vector unsigned char zerovec;
    
    vector bool short equal_zero;
    vector bool short less_zero;
    vector bool short overflow;
    
#ifdef DEBUG
    /* print alignment errors if this is on */
    if(((unsigned)data) & 0x15)
        fprintf(stderr, "dequant_h263_inter_altivec_c:incorrect align, data: %lx\n", (long)data);
#endif
    
    /* initialize */
    *((unsigned short*)&quant_m_2) = (unsigned short)(quant << 1);
    quant_m_2 = vec_splat(quant_m_2,0);
    
    *((unsigned short*)&quant_add) = (unsigned short)(quant & 1 ? quant : quant - 1);
    quant_add = vec_splat(quant_add,0);
    
    vec_2048 = vec_sl(vec_splat_s16(1), vec_splat_u16(11));
    zerovec = vec_splat_u8(0);
    
    /* dequant */
    DEQUANT_H263_INTER_ALTIVEC();
    DEQUANT_H263_INTER_ALTIVEC();
    DEQUANT_H263_INTER_ALTIVEC();
    DEQUANT_H263_INTER_ALTIVEC();
    
    DEQUANT_H263_INTER_ALTIVEC();
    DEQUANT_H263_INTER_ALTIVEC();
    DEQUANT_H263_INTER_ALTIVEC();
    DEQUANT_H263_INTER_ALTIVEC();
    
    return 0;
}
