/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Altivec 8bit<->16bit transfer -
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
 * $Id: mem_transfer_altivec.c,v 1.2 2004/12/09 23:02:54 edgomez Exp $
 *
 ****************************************************************************/

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"


/* Turn this on if you like debugging the alignment */
#undef DEBUG

#include <stdio.h>

/* This function assumes:
 *	dst: 16 byte aligned
 */

#define COPY8TO16() \
s = vec_perm(vec_ld(0,src),vec_ld(16,src),vec_lvsl(0,src));\
vec_st((vector signed short)vec_mergeh(zerovec,s),0,dst);\
src += stride;\
dst += 8

void
transfer_8to16copy_altivec_c(int16_t *dst,
                            uint8_t * src,
                            uint32_t stride)
{
	register vector unsigned char s;
	register vector unsigned char zerovec;
		
#ifdef DEBUG
	/* Check the alignment */
	if((long)dst & 0xf)
		fprintf(stderr, "transfer_8to16copy_altivec_c:incorrect align, dst: %lx\n", (long)dst);
#endif
	
	/* initialization */
	zerovec = vec_splat_u8(0);
	
	COPY8TO16();
	COPY8TO16();
	COPY8TO16();
	COPY8TO16();
	
	COPY8TO16();
	COPY8TO16();
	COPY8TO16();
	COPY8TO16();
}


/*
 * This function assumes dst is 8 byte aligned and stride is a multiple of 8
 * src may be unaligned
 */

#define COPY16TO8() \
s = vec_perm(src[0], src[1], load_src_perm); \
packed = vec_packsu(s, vec_splat_s16(0)); \
mask = vec_perm(mask_stencil, mask_stencil, vec_lvsl(0, dst)); \
packed = vec_perm(packed, packed, vec_lvsl(0, dst)); \
packed = vec_sel(packed, vec_ld(0, dst), mask); \
vec_st(packed, 0, dst); \
src++; \
dst += stride

void transfer_16to8copy_altivec_c(uint8_t *dst,
                            vector signed short *src,
                            uint32_t stride)
{
    register vector signed short s;
    register vector unsigned char packed;
    register vector unsigned char mask_stencil;
    register vector unsigned char mask;
    register vector unsigned char load_src_perm;
    
#ifdef DEBUG
    /* if this is on, print alignment errors */
    if(((unsigned long) dst) & 0x7)
        fprintf(stderr, "transfer_16to8copy_altivec:incorrect align, dst %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "transfer_16to8copy_altivec:incorrect align, stride %u\n", stride);
#endif
    /* Initialisation stuff */
    load_src_perm = vec_lvsl(0, (unsigned char*)src);
    mask_stencil = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    COPY16TO8();
    COPY16TO8();
    COPY16TO8();
    COPY16TO8();
    
    COPY16TO8();
    COPY16TO8();
    COPY16TO8();
    COPY16TO8();
}



/*
 * This function assumes dst is 8 byte aligned and src is unaligned. Stride has
 * to be a multiple of 8
 */

#define COPY8TO8() \
tmp = vec_perm(vec_ld(0, src), vec_ld(16, src), vec_lvsl(0, src)); \
t0 = vec_perm(tmp, tmp, vec_lvsl(0, dst));\
t1 = vec_perm(mask, mask, vec_lvsl(0, dst));\
tmp = vec_sel(t0, vec_ld(0, dst), t1);\
vec_st(tmp, 0, dst);\
dst += stride;\
src += stride

void
transfer8x8_copy_altivec_c( uint8_t * dst,
                            uint8_t * src,
                            uint32_t stride)
{
    register vector unsigned char tmp;
    register vector unsigned char mask;
	register vector unsigned char t0, t1;
    
#ifdef DEBUG
    if(((unsigned long)dst) & 0x7)
        fprintf(stderr, "transfer8x8_copy_altivec:incorrect align, dst: %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "transfer8x8_copy_altivec:incorrect stride, stride: %u\n", stride);
#endif
    mask = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    COPY8TO8();
    COPY8TO8();
    COPY8TO8();
    COPY8TO8();
    
    COPY8TO8();
    COPY8TO8();
    COPY8TO8();
    COPY8TO8();
}


#define SUB8TO16() \
	c = vec_perm(vec_ld(0,cur),vec_ld(16,cur),vec_lvsl(0,cur));\
	r = vec_perm(vec_ld(0,ref),vec_ld(16,ref),vec_lvsl(0,ref));\
	cs = (vector signed short)vec_mergeh(ox00,c);\
	rs = (vector signed short)vec_mergeh(ox00,r);\
	\
	c = vec_lvsr(0,cur);\
	mask = vec_perm(mask_00ff, mask_00ff, c);\
	r = vec_perm(r, r, c);\
	r = vec_sel(r, vec_ld(0,cur), mask);\
	vec_st(r,0,cur);\
	vec_st( vec_sub(cs,rs), 0, dct );\
	\
	dct += 8;\
	cur += stride;\
	ref += stride


/* This function assumes:
 *	dct: 16 Byte aligned
 *	cur:  8 Byte aligned
 *	stride: multiple of 8
 */

void
transfer_8to16sub_altivec_c(int16_t * dct,
							uint8_t * cur,
							uint8_t * ref,
							const uint32_t stride)
{
	register vector unsigned char c,r;
	register vector unsigned char ox00;
	register vector unsigned char mask_00ff;
	register vector unsigned char mask;
	register vector signed short cs,rs;
	
#ifdef DEBUG
	if((long)dct & 0xf)
		fprintf(stderr, "transfer_8to16sub_altivec_c:incorrect align, dct: %lx\n", (long)dct);
	if((long)cur & 0x7)
		fprintf(stderr, "transfer_8to16sub_altivec_c:incorrect align, cur: %lx\n", (long)cur);
	if(stride & 0x7)
		fprintf(stderr, "transfer_8to16sub_altivec_c:incorrect stride, stride: %lu\n", (long)stride);
#endif
	/* initialize */
	ox00 = vec_splat_u8(0);
	mask_00ff = vec_pack((vector unsigned short)ox00,vec_splat_u16(-1));
	
	SUB8TO16();
	SUB8TO16();
	SUB8TO16();
	SUB8TO16();
	
	SUB8TO16();
	SUB8TO16();
	SUB8TO16();
	SUB8TO16();
}


#define SUBRO8TO16() \
	c = vec_perm(vec_ld(0,cur),vec_ld(16,cur),vec_lvsl(0,cur));\
	r = vec_perm(vec_ld(0,ref),vec_ld(16,ref),vec_lvsl(0,ref));\
	cs = (vector signed short)vec_mergeh(z,c);\
	rs = (vector signed short)vec_mergeh(z,r);\
	vec_st( vec_sub(cs,rs), 0, dct );\
	dct += 8;\
	cur += stride;\
	ref += stride


/* This function assumes:
 *	dct: 16 Byte aligned
 */

void
transfer_8to16subro_altivec_c(int16_t * dct,
					const uint8_t * cur,
					const uint8_t * ref,
					const uint32_t stride)
{
	register vector unsigned char c;
	register vector unsigned char r;
	register vector unsigned char z;
	register vector signed short cs;
	register vector signed short rs;
	
#ifdef DEBUG
	/* Check the alignment assumptions if this is on */
	if((long)dct & 0xf)
		fprintf(stderr, "transfer_8to16subro_altivec_c:incorrect align, dct: %lx\n", (long)dct);
#endif
	/* initialize */
	z = vec_splat_u8(0);
	
	SUBRO8TO16();
	SUBRO8TO16();
	SUBRO8TO16();
	SUBRO8TO16();
	
	SUBRO8TO16();
	SUBRO8TO16();
	SUBRO8TO16();
	SUBRO8TO16();
}

/*
 * This function assumes:
 *  dct: 16 bytes alignment
 *  cur: 8 bytes alignment
 *  ref1: unaligned
 *  ref2: unaligned
 *  stride: multiple of 8
 */

#define SUB28TO16() \
r1 = vec_perm(vec_ld(0, ref1), vec_ld(16, ref1), vec_lvsl(0, ref1)); \
r2 = vec_perm(vec_ld(0, ref2), vec_ld(16, ref2), vec_lvsl(0, ref2)); \
c = vec_perm(vec_ld(0, cur), vec_ld(16, cur), vec_lvsl(0, cur)); \
r = vec_avg(r1, r2); \
cs = (vector signed short)vec_mergeh(vec_splat_u8(0), c); \
rs = (vector signed short)vec_mergeh(vec_splat_u8(0), r); \
c = vec_perm(mask, mask, vec_lvsl(0, cur));\
r = vec_sel(r, vec_ld(0, cur), c);\
vec_st(r, 0, cur); \
*dct++ = vec_sub(cs, rs); \
cur += stride; \
ref1 += stride; \
ref2 += stride

void
transfer_8to16sub2_altivec_c(vector signed short *dct,
                             uint8_t *cur,
                             uint8_t *ref1,
                             uint8_t *ref2,
                             const uint32_t stride)
{
    vector unsigned char r1;
    vector unsigned char r2;
    vector unsigned char r;
    vector unsigned char c;
    vector unsigned char mask;
    vector signed short cs;
    vector signed short rs;
    
#ifdef DEBUG
    /* Dump alignment erros if DEBUG is set */
    if(((unsigned long)dct) & 0xf)
        fprintf(stderr, "transfer_8to16sub2_altivec_c:incorrect align, dct: %lx\n", (long)dct);
    if(((unsigned long)cur) & 0x7)
        fprintf(stderr, "transfer_8to16sub2_altivec_c:incorrect align, cur: %lx\n", (long)cur);
    if(stride & 0x7)
        fprintf(stderr, "transfer_8to16sub2_altivec_c:incorrect align, dct: %u\n", stride);
#endif
    
    /* Initialisation */
    mask = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));
    
    SUB28TO16();
    SUB28TO16();
    SUB28TO16();
    SUB28TO16();
    
    SUB28TO16();
    SUB28TO16();
    SUB28TO16();
    SUB28TO16();
}



/*
 * This function assumes:
 *  dst: 8 byte aligned
 *  src: unaligned
 *  stride: multiple of 8
 */

#define ADD16TO8() \
s = vec_perm(vec_ld(0, src), vec_ld(16, src), vec_lvsl(0, src)); \
d = vec_perm(vec_ld(0, dst), vec_ld(16, dst), vec_lvsl(0, dst)); \
ds = (vector signed short)vec_mergeh(vec_splat_u8(0), d); \
ds = vec_add(ds, s); \
packed = vec_packsu(ds, vec_splat_s16(0)); \
mask = vec_pack(vec_splat_u16(0), vec_splat_u16(-1)); \
mask = vec_perm(mask, mask, vec_lvsl(0, dst)); \
packed = vec_perm(packed, packed, vec_lvsl(0, dst)); \
packed = vec_sel(packed, vec_ld(0, dst), mask); \
vec_st(packed, 0, dst); \
src += 8; \
dst += stride

void
transfer_16to8add_altivec_c(uint8_t *dst,
                            int16_t *src,
                            uint32_t stride)
{
    vector signed short s;
    vector signed short ds;
    vector unsigned char d;
    vector unsigned char packed;
    vector unsigned char mask;

#ifdef DEBUG
    /* if this is set, dump alignment errors */
    if(((unsigned long)dst) & 0x7)
        fprintf(stderr, "transfer_16to8add_altivec_c:incorrect align, dst: %lx\n", (long)dst);
    if(stride & 0x7)
        fprintf(stderr, "transfer_16to8add_altivec_c:incorrect align, dst: %u\n", stride);
#endif

    ADD16TO8();
    ADD16TO8();
    ADD16TO8();
    ADD16TO8();
    
    ADD16TO8();
    ADD16TO8();
    ADD16TO8();
    ADD16TO8();
}
