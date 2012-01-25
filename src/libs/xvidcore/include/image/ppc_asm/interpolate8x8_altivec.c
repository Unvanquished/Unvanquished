/*****************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	- 8x8 block-based halfpel interpolation with altivec optimization -
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
 * $Id: interpolate8x8_altivec.c,v 1.3 2004/12/09 23:02:54 edgomez Exp $
 *
 ****************************************************************************/


#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"

#undef DEBUG
#include <stdio.h>

static inline unsigned
build_prefetch(unsigned char block_size, unsigned char block_count, short stride)
{
    if(block_size > 31)
        block_size = 0;
    
    return ((block_size << 24) | (block_count << 16) | stride);
}

#define NO_ROUNDING

#define ROUNDING \
s1 = vec_and(vec_add(s1, s2), vec_splat_u8(1)); \
d = vec_sub(d, s1);

#define INTERPLATE8X8_HALFPEL_H(round) \
s1 = vec_perm(vec_ld(0, src), vec_ld(16, src), vec_lvsl(0, src)); \
s2 = vec_perm(s1, s1, s2_mask); \
d = vec_avg(s1, s2); \
round; \
mask = vec_perm(mask_stencil, mask_stencil, vec_lvsl(0, dst)); \
d = vec_perm(d, d, vec_lvsl(0, dst)); \
d = vec_sel(d, vec_ld(0, dst), mask); \
vec_st(d, 0, dst); \
dst += stride; \
src += stride


/* This function assumes:
 *  dst is 8 byte aligned
 *  src is unaligned
 *  stride is a multiple of 8
 */
void
interpolate8x8_halfpel_h_altivec_c( uint8_t *dst,
                                    uint8_t *src,
                                    const uint32_t stride,
                                    const uint32_t rounding)
{
    register vector unsigned char s1, s2;
    register vector unsigned char d;
    register vector unsigned char mask;
    register vector unsigned char s2_mask;
    register vector unsigned char mask_stencil;
    
#ifdef DEBUG
    /* Dump alignment errors if DEBUG is defined */
    if(((unsigned long)dst) & 0x7)
        fprintf(stderr, "interpolate8x8_halfpel_h_altivec_c:incorrect align, dst: %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "interpolate8x8_halfpel_h_altivec_c:incorrect stride, stride: %u\n", stride);
#endif

    s2_mask = vec_lvsl(1, (unsigned char*)0);
    mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    if(rounding) {
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
        INTERPLATE8X8_HALFPEL_H(ROUNDING);
    }
    else {
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_H(NO_ROUNDING);
    }
}

#define INTERPLATE8X8_HALFPEL_V(round) \
s1 = vec_perm(vec_ld(0, src), vec_ld(16, src), vec_lvsl(0, src)); \
s2 = vec_perm(vec_ld(0, src + stride), vec_ld(16, src + stride), vec_lvsl(0, src + stride)); \
d = vec_avg(s1, s2); \
round; \
mask = vec_perm(mask_stencil, mask_stencil, vec_lvsl(0, dst)); \
d = vec_perm(d, d, vec_lvsl(0, dst)); \
d = vec_sel(d, vec_ld(0, dst), mask); \
vec_st(d, 0, dst); \
dst += stride; \
src += stride

/*
 * This function assumes
 *  dst is 8 byte aligned
 *  src is unaligned
 *  stride is a multiple of 8
 */
void
interpolate8x8_halfpel_v_altivec_c( uint8_t *dst,
                                    uint8_t *src,
                                    const uint32_t stride,
                                    const uint32_t rounding)
{
    vector unsigned char s1, s2;
    vector unsigned char d;
    vector unsigned char mask;
    vector unsigned char mask_stencil;
    
#ifdef DEBUG
    /* if this is on, print alignment errors */
    if(((unsigned long)dst) & 0x7)
        fprintf(stderr, "interpolate8x8_halfpel_v_altivec_c:incorrect align, dst: %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "interpolate8x8_halfpel_v_altivec_c:incorrect stride, stride: %u\n", stride);
#endif

    mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    if(rounding) {
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
        INTERPLATE8X8_HALFPEL_V(ROUNDING);
    }
    else {
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
        INTERPLATE8X8_HALFPEL_V(NO_ROUNDING);
    }
}


#define INTERPOLATE8X8_HALFPEL_HV(adding) \
t = vec_perm(vec_ld(0, src), vec_ld(16, src), vec_lvsl(0, src)); \
s1 = (vector unsigned short)vec_mergeh(zerovec, t); \
t = vec_perm(vec_ld(1, src), vec_ld(17, src), vec_lvsl(1, src)); \
s2 = (vector unsigned short)vec_mergeh(zerovec, t); \
t = vec_perm(vec_ld(0, src + stride), vec_ld(16, src + stride), vec_lvsl(0, src + stride)); \
s3 = (vector unsigned short)vec_mergeh(zerovec, t); \
t = vec_perm(vec_ld(1, src + stride), vec_ld(17, src + stride), vec_lvsl(1, src + stride)); \
s4 = (vector unsigned short)vec_mergeh(zerovec, t); \
s1 = vec_add(s1,s2);\
s3 = vec_add(s3,s4);\
s1 = vec_add(s1,s3);\
s1 = vec_add(s1, adding); \
s1 = vec_sr(s1, two); \
t = vec_pack(s1, s1); \
mask = vec_perm(mask_stencil, mask_stencil, vec_lvsl(0, dst)); \
t = vec_sel(t, vec_ld(0, dst), mask); \
vec_st(t, 0, dst); \
dst += stride; \
src += stride

void
interpolate8x8_halfpel_hv_altivec_c(uint8_t *dst,
                                    uint8_t *src,
                                    const uint32_t stride,
                                    const uint32_t rounding)
{
    vector unsigned short s1, s2, s3, s4;
    vector unsigned char t;
    vector unsigned short one, two;
    vector unsigned char zerovec;
    vector unsigned char mask;
    vector unsigned char mask_stencil;
    
    /* Initialisation stuff */
    zerovec = vec_splat_u8(0);
    one = vec_splat_u16(1);
    two = vec_splat_u16(2);
    mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    if(rounding) {
        INTERPOLATE8X8_HALFPEL_HV(one);
        INTERPOLATE8X8_HALFPEL_HV(one);
        INTERPOLATE8X8_HALFPEL_HV(one);
        INTERPOLATE8X8_HALFPEL_HV(one);
        
        INTERPOLATE8X8_HALFPEL_HV(one);
        INTERPOLATE8X8_HALFPEL_HV(one);
        INTERPOLATE8X8_HALFPEL_HV(one);
        INTERPOLATE8X8_HALFPEL_HV(one);
    }
    else {
        INTERPOLATE8X8_HALFPEL_HV(two);
        INTERPOLATE8X8_HALFPEL_HV(two);
        INTERPOLATE8X8_HALFPEL_HV(two);
        INTERPOLATE8X8_HALFPEL_HV(two);
        
        INTERPOLATE8X8_HALFPEL_HV(two);
        INTERPOLATE8X8_HALFPEL_HV(two);
        INTERPOLATE8X8_HALFPEL_HV(two);
        INTERPOLATE8X8_HALFPEL_HV(two);
    }
}

/*
 * This function assumes:
 *  dst is 8 byte aligned
 *  src1 is unaligned
 *  src2 is unaligned
 *  stirde is a multiple of 8
 *  rounding is smaller than than max signed short + 2
 */

void
interpolate8x8_avg2_altivec_c(  uint8_t *dst,
                                const uint8_t *src1,
                                const uint8_t *src2,
                                const uint32_t stride,
                                const uint32_t rounding,
                                const uint32_t height)
{
    uint32_t i;
    vector unsigned char t;
    vector unsigned char mask;
    vector unsigned char mask_stencil;
    vector unsigned char zerovec;
    vector signed short s1, s2;
    vector signed short d;
    vector signed short round;
    
#ifdef DEBUG
    /* If this is on, print alignment errors */
    if(((unsigned long)dst) & 0x7)
        fprintf(stderr, "interpolate8x8_avg2_altivec_c:incorrect align, dst: %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "interpolate8x8_avg2_altivec_c:incorrect stride, stride: %u\n", stride);
    if(rounding > (32767 + 2))
        fprintf(stderr, "interpolate8x8_avg2_altivec_c:incorrect rounding, rounding: %d\n", rounding);
#endif

    /* initialisation */
    zerovec = vec_splat_u8(0);
    *((short*)&round) = 1 - rounding;
    round = vec_splat(round, 0);
    mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    for(i = 0; i < height; i++) {
        
        t = vec_perm(vec_ld(0, src1), vec_ld(16, src1), vec_lvsl(0, src1));
        d = vec_add((vector signed short)zerovec, round);
        s1 = (vector signed short)vec_mergeh(zerovec, t);
        
        t = vec_perm(vec_ld(0, src2), vec_ld(16, src2), vec_lvsl(0, src2));
        d = vec_add(d, s1);
        s2 = (vector signed short)vec_mergeh(zerovec, t);
        
        d = vec_add(d, s2);
        d = vec_sr(d, vec_splat_u16(1));
        
        t = vec_pack((vector unsigned short)d, (vector unsigned short)zerovec);
        mask = vec_perm(mask_stencil, mask_stencil, vec_lvsl(0, dst));
        t = vec_perm(t, t, vec_lvsl(0, dst));
        t = vec_sel(t, vec_ld(0, dst), mask);
        vec_st(t, 0, dst);
        
        dst += stride;
        src1 += stride;
        src2 += stride;
    }
}


#define INTERPOLATE8X8_AVG4() \
d = r;  \
\
t = vec_perm(vec_ld(0, src1), vec_ld(16, src1), vec_lvsl(0, src1)); \
s = (vector signed short)vec_mergeh(zerovec, t);                    \
d = vec_add(d, s);                                                  \
\
t = vec_perm(vec_ld(0, src2), vec_ld(16, src2), vec_lvsl(0, src2)); \
s = (vector signed short)vec_mergeh(zerovec, t);                    \
d = vec_add(d, s);                                                  \
\
t = vec_perm(vec_ld(0, src3), vec_ld(16, src3), vec_lvsl(0, src3)); \
s = (vector signed short)vec_mergeh(zerovec, t);                    \
d = vec_add(d, s);                                                  \
\
t = vec_perm(vec_ld(0, src4), vec_ld(16, src4), vec_lvsl(0, src4)); \
s = (vector signed short)vec_mergeh(zerovec, t);                    \
d = vec_add(d, s);                                                  \
\
d = vec_sr(d, shift);                                               \
\
t = vec_pack((vector unsigned short)d, (vector unsigned short)zerovec); \
mask = vec_perm(mask_stencil, mask_stencil, vec_lvsl(0, dst));          \
t = vec_perm(t, t, vec_lvsl(0, dst));                                   \
t = vec_sel(t, vec_ld(0, dst), mask);                                   \
vec_st(t, 0, dst);                                                      \
\
dst += stride;  \
src1 += stride; \
src2 += stride; \
src3 += stride; \
src4 += stride

/* This function assumes:
 *  dst is 8 byte aligned
 *  src1, src2, src3, src4 are unaligned
 *  stride is a multiple of 8
 */

void
interpolate8x8_avg4_altivec_c(uint8_t *dst,
                                const uint8_t *src1, const uint8_t *src2,
                                const uint8_t *src3, const uint8_t *src4,
                                const uint32_t stride, const uint32_t rounding)
{
    vector signed short r;
    register vector signed short s, d;
    register vector unsigned short shift;
    register vector unsigned char t;
    register vector unsigned char zerovec;
    register vector unsigned char mask;
    register vector unsigned char mask_stencil;

#ifdef DEBUG
    /* if debug is set, print alignment errors */
    if(((unsigned)dst) & 0x7)
        fprintf(stderr, "interpolate8x8_avg4_altivec_c:incorrect align, dst: %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "interpolate8x8_avg4_altivec_c:incorrect stride, stride: %u\n", stride);
#endif

    /* Initialization */
    zerovec = vec_splat_u8(0);
    *((short*)&r) = 2 - rounding;
    r = vec_splat(r, 0);
    shift = vec_splat_u16(2);
    mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    /* interpolate */
    INTERPOLATE8X8_AVG4();
    INTERPOLATE8X8_AVG4();
    INTERPOLATE8X8_AVG4();
    INTERPOLATE8X8_AVG4();
    
    INTERPOLATE8X8_AVG4();
    INTERPOLATE8X8_AVG4();
    INTERPOLATE8X8_AVG4();
    INTERPOLATE8X8_AVG4();
}

/*
 * This function assumes:
 *  dst is 8 byte aligned
 *  src is unaligned
 *  stirde is a multiple of 8
 *  rounding is ignored
 */
void
interpolate8x8_halfpel_add_altivec_c(uint8_t *dst, const uint8_t *src, const uint32_t stride, const uint32_t rouding)
{
	interpolate8x8_avg2_altivec_c(dst, dst, src, stride, 0, 8);
}

#define INTERPOLATE8X8_HALFPEL_H_ADD_ROUND() \
	mask_dst = vec_lvsl(0,dst);										\
	s1 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));	\
	d = vec_perm(vec_ld(0,dst),vec_ld(16,dst),mask_dst);		\
	\
	s2 = vec_perm(s1,s1,rot1);\
	tmp = vec_avg(s1,s2);\
	s1 = vec_xor(s1,s2);\
	s1 = vec_sub(tmp,vec_and(s1,one));\
	\
	d = vec_avg(s1,d);\
	\
	mask = vec_perm(mask_stencil, mask_stencil, mask_dst); \
	d = vec_perm(d,d,mask_dst);	\
	d = vec_sel(d,vec_ld(0,dst),mask);	\
	vec_st(d,0,dst);					\
	\
	dst += stride; \
	src += stride

#define INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND() \
	mask_dst = vec_lvsl(0,dst);										\
	s1 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));	\
	d = vec_perm(vec_ld(0,dst),vec_ld(16,dst),mask_dst);		\
	\
	s1 = vec_avg(s1, vec_perm(s1,s1,rot1));\
	d = vec_avg(s1,d);\
	\
	mask = vec_perm(mask_stencil,mask_stencil,mask_dst);\
	d = vec_perm(d,d,mask_dst);\
	d = vec_sel(d,vec_ld(0,dst),mask);\
	vec_st(d,0,dst);\
	\
	dst += stride;\
	src += stride

/*
 * This function assumes:
 *	dst is 8 byte aligned
 *	src is unaligned
 *	stride is a multiple of 8
 */
void
interpolate8x8_halfpel_h_add_altivec_c(uint8_t *dst, uint8_t *src, const uint32_t stride, const uint32_t rounding)
{
	register vector unsigned char s1,s2;
	register vector unsigned char d;
	register vector unsigned char tmp;
	
	register vector unsigned char mask_dst;
	register vector unsigned char one;
	register vector unsigned char rot1;
	
	register vector unsigned char mask_stencil;
	register vector unsigned char mask;
	
#ifdef DEBUG
	if(((unsigned)dst) & 0x7)
		fprintf(stderr, "interpolate8x8_halfpel_h_add_altivec_c:incorrect align, dst: %lx\n", (long)dst);
	if(stride & 0x7)
		fprintf(stderr, "interpolate8x8_halfpel_h_add_altivec_c:incorrect stride, stride: %u\n", stride);
#endif
	
	/* initialization */
	mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
	one = vec_splat_u8(1);
	rot1 = vec_lvsl(1,(unsigned char*)0);
	
	if(rounding) {
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_ROUND();
	}
	else {
		
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_H_ADD_NOROUND();
	}
}




#define INTERPOLATE8X8_HALFPEL_V_ADD_ROUND()\
	src += stride;\
	mask_dst = vec_lvsl(0,dst);\
	s2 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));\
	d = vec_perm(vec_ld(0,dst),vec_ld(16,dst),mask_dst);\
	\
	tmp = vec_avg(s1,s2);\
	s1 = vec_xor(s1,s2);\
	s1 = vec_sub(tmp,vec_and(s1,vec_splat_u8(1)));\
	d = vec_avg(s1,d);\
	\
	mask = vec_perm(mask_stencil,mask_stencil,mask_dst);\
	d = vec_perm(d,d,mask_dst);\
	d = vec_sel(d,vec_ld(0,dst),mask);\
	vec_st(d,0,dst);\
	\
	s1 = s2;\
	\
	dst += stride

#define INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND()\
	src += stride;\
	mask_dst = vec_lvsl(0,dst);\
	s2 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));\
	d = vec_perm(vec_ld(0,dst),vec_ld(16,dst),mask_dst);\
	\
	s1 = vec_avg(s1,s2);\
	d = vec_avg(s1,d);\
	\
	mask = vec_perm(mask_stencil,mask_stencil,mask_dst);\
	d = vec_perm(d,d,mask_dst);\
	d = vec_sel(d,vec_ld(0,dst),mask);\
	vec_st(d,0,dst);\
	\
	s1 = s2;\
	dst += stride

/*
 * This function assumes:
 *	dst: 8 byte aligned
 *	src: unaligned
 *	stride is a multiple of 8
 */
 
void
interpolate8x8_halfpel_v_add_altivec_c(uint8_t *dst, uint8_t *src, const uint32_t stride, const uint32_t rounding)
{
	register vector unsigned char s1,s2;
	register vector unsigned char tmp;
	register vector unsigned char d;
	
	register vector unsigned char mask;
	register vector unsigned char mask_dst;
	register vector unsigned char mask_stencil;
	
#ifdef DEBUG
	if(((unsigned)dst) & 0x7)
		fprintf(stderr, "interpolate8x8_halfpel_v_add_altivec_c:incorrect align, dst: %lx\n", (long)dst);
	if(stride & 0x7)
		fprintf(stderr, "interpolate8x8_halfpel_v_add_altivec_c:incorrect align, dst: %u\n", stride);
#endif

	/* initialization */
	mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
	
	if(rounding) {
		
		/* Interpolate vertical with rounding */
		s1 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));
		
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
			
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_ROUND();
	}
	else {
		/* Interpolate vertical without rounding */
		s1 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));
		
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_V_ADD_NOROUND();
	}
}



#define INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND()\
	src += stride;\
	mask_dst = vec_lvsl(0,dst);\
	c10 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));\
	d = vec_perm(vec_ld(0,dst),vec_ld(16,dst),mask_dst);\
	c11 = vec_perm(c10,c10,rot1);\
	\
	s00 = (vector unsigned short)vec_mergeh(zero,c00);\
	s01 = (vector unsigned short)vec_mergeh(zero,c01);\
	s10 = (vector unsigned short)vec_mergeh(zero,c10);\
	s11 = (vector unsigned short)vec_mergeh(zero,c11);\
	\
	s00 = vec_add(s00,s10);\
	s01 = vec_add(s01,s11);\
	s00 = vec_add(s00,s01);\
	s00 = vec_add(s00,one);\
	\
	s00 = vec_sr(s00,two);\
	s00 = vec_add(s00, (vector unsigned short)vec_mergeh(zero,d));\
	s00 = vec_sr(s00,one);\
	\
	d = vec_pack(s00,s00);\
	mask = vec_perm(mask_stencil,mask_stencil,mask_dst);\
	d = vec_sel(d,vec_ld(0,dst),mask);\
	vec_st(d,0,dst);\
	\
	c00 = c10;\
	c01 = c11;\
	dst += stride
	

#define INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND()\
	src += stride;\
	mask_dst = vec_lvsl(0,dst);\
	c10 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));\
	d = vec_perm(vec_ld(0,dst),vec_ld(16,dst),mask_dst);\
	c11 = vec_perm(c10,c10,rot1);\
	\
	s00 = (vector unsigned short)vec_mergeh(zero,c00);\
	s01 = (vector unsigned short)vec_mergeh(zero,c01);\
	s10 = (vector unsigned short)vec_mergeh(zero,c10);\
	s11 = (vector unsigned short)vec_mergeh(zero,c11);\
	\
	s00 = vec_add(s00,s10);\
	s01 = vec_add(s01,s11);\
	s00 = vec_add(s00,s01);\
	s00 = vec_add(s00,two);\
	s00 = vec_sr(s00,two);\
	\
	c00 = vec_pack(s00,s00);\
	d = vec_avg(d,c00);\
	\
	mask = vec_perm(mask_stencil,mask_stencil,mask_dst);\
	d = vec_perm(d,d,mask_dst);\
	d = vec_sel(d,vec_ld(0,dst),mask);\
	vec_st(d,0,dst);\
	\
	c00 = c10;\
	c01 = c11;\
	dst += stride


/*
 * This function assumes:
 *	dst: 8 byte aligned
 *	src: unaligned
 *	stride: multiple of 8
 */

void
interpolate8x8_halfpel_hv_add_altivec_c(uint8_t *dst, uint8_t *src, const uint32_t stride, const uint32_t rounding)
{
	register vector unsigned char  c00,c10,c01,c11;
	register vector unsigned short s00,s10,s01,s11;
	register vector unsigned char  d;
	
	register vector unsigned char mask;
	register vector unsigned char mask_stencil;
	
	register vector unsigned char rot1;
	register vector unsigned char mask_dst;
	register vector unsigned char zero;
	register vector unsigned short one,two;
	
#ifdef DEBUG
	if(((unsigned)dst) & 0x7)
		fprintf(stderr, "interpolate8x8_halfpel_hv_add_altivec_c:incorrect align, dst: %lx\n", (long)dst);
	if(stride & 0x7)
		fprintf(stderr, "interpolate8x8_halfpel_hv_add_altivec_c:incorrect stride, stride: %u\n", stride);
#endif
	
	/* initialization */
	mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
	rot1 = vec_lvsl(1,(unsigned char*)0);
	zero = vec_splat_u8(0);
	one = vec_splat_u16(1);
	two = vec_splat_u16(2);
	
	if(rounding) {
		
		/* Load the first row 'manually' */
		c00 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));
		c01 = vec_perm(c00,c00,rot1);
		
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_ROUND();
	}
	else {
		
		/* Load the first row 'manually' */
		c00 = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));
		c01 = vec_perm(c00,c00,rot1);
		
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
		INTERPOLATE8X8_HALFPEL_HV_ADD_NOROUND();
	}
}
