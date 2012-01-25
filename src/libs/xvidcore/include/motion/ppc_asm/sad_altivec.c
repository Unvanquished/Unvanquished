/*

    Copyright (C) 2002 Benjamin Herrenschmidt <benh@kernel.crashing.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    $Id: sad_altivec.c,v 1.11 2004/12/09 23:02:54 edgomez Exp $
*/

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif


#include "../../portab.h"

/* no debugging by default */
#undef DEBUG

#include <stdio.h>

#define SAD16() \
t1  = vec_perm(ref[0], ref[1], perm);  /* align current vector  */ \
t2  = vec_max(t1, *cur);      	 /* find largest of two           */ \
t1  = vec_min(t1, *cur); 	         /* find smaller of two           */ \
t1  = vec_sub(t2, t1);                   /* find absolute difference      */ \
sad = vec_sum4s(t1, vec_splat_u32(0));                /* sum of differences */ \
sumdiffs = (vector unsigned int)vec_sums((vector signed int)sad, (vector signed int)sumdiffs);    /* accumulate sumdiffs */ \
if(vec_any_ge(sumdiffs, best_vec)) \
    goto bail; \
cur += stride; ref += stride;

/*
 * This function assumes cur and stride are 16 bytes aligned and ref is unaligned
 */

uint32_t
sad16_altivec_c(vector unsigned char *cur,
			  vector unsigned char *ref,
			  uint32_t stride,
			  const uint32_t best_sad)
{
	vector unsigned char perm;
	vector unsigned char t1, t2;
	vector unsigned int sad;
	vector unsigned int sumdiffs;
	vector unsigned int best_vec;
	uint32_t result;

        
#ifdef DEBUG
        /* print alignment errors if DEBUG is on */
	if (((unsigned long) cur) & 0xf)
		fprintf(stderr, "sad16_altivec:incorrect align, cur: %lx\n", (long)cur);
	if (stride & 0xf)
		fprintf(stderr, "sad16_altivec:incorrect align, stride: %lu\n", stride);
#endif
	/* initialization */
	sad = vec_splat_u32(0);
	sumdiffs = sad;
	stride >>= 4;
	perm = vec_lvsl(0, (unsigned char *) ref);
	*((uint32_t*)&best_vec) = best_sad;
	best_vec = vec_splat(best_vec, 0);

	/* perform sum of differences between current and previous */
	SAD16();
	SAD16();
	SAD16();
	SAD16();

	SAD16();
	SAD16();
	SAD16();
	SAD16();

	SAD16();
	SAD16();
	SAD16();
	SAD16();
        
	SAD16();
	SAD16();
	SAD16();
	SAD16();

  bail:
	/* copy vector sum into unaligned result */
	sumdiffs = vec_splat(sumdiffs, 3);
	vec_ste(sumdiffs, 0, (uint32_t*) &result);
	return result;
}


#define SAD8() \
	c = vec_perm(vec_ld(0,cur),vec_ld(16,cur),vec_lvsl(0,cur));\
	r = vec_perm(vec_ld(0,ref),vec_ld(16,ref),vec_lvsl(0,ref));\
	c = vec_sub(vec_max(c,r),vec_min(c,r));\
	sad = vec_sum4s(c,sad);\
	cur += stride;\
	ref += stride

/*
 * This function assumes nothing
 */
 
uint32_t
sad8_altivec_c(const uint8_t * cur,
	   const uint8_t *ref,
	   const uint32_t stride)
{
	uint32_t result = 0;
	
	register vector unsigned int sad;
	register vector unsigned char c;
	register vector unsigned char r;
	
	/* initialize */
	sad = vec_splat_u32(0);
	
	/* Perform sad operations */
	SAD8();
	SAD8();
	SAD8();
	SAD8();
	
	SAD8();
	SAD8();
	SAD8();
	SAD8();
	
	/* finish addition, add the first 2 together */
	sad = vec_and(sad, (vector unsigned int)vec_pack(vec_splat_u16(-1),vec_splat_u16(0)));
	sad = (vector unsigned int)vec_sums((vector signed int)sad, vec_splat_s32(0));
	sad = vec_splat(sad,3);
	vec_ste(sad, 0, &result);
		
	return result;
}




#define MEAN16() \
mean = vec_sum4s(*ptr,mean);\
ptr += stride

#define DEV16() \
t2  = vec_max(*ptr, mn);                    /* find largest of two           */ \
t3  = vec_min(*ptr, mn);                    /* find smaller of two           */ \
t2  = vec_sub(t2, t3);                      /* find absolute difference      */ \
dev = vec_sum4s(t2, dev); \
ptr += stride

/*
 * This function assumes cur is 16 bytes aligned and stride is 16 bytes
 * aligned
*/

uint32_t
dev16_altivec_c(vector unsigned char *cur,
			  uint32_t stride)
{
	vector unsigned char t2, t3, mn;
	vector unsigned int mean, dev;
	vector unsigned int sumdiffs;
	vector unsigned char *ptr;
	uint32_t result;

#ifdef DEBUG
        /* print alignment errors if DEBUG is on */
        if(((unsigned long)cur) & 0x7)
            fprintf(stderr, "dev16_altivec:incorrect align, cur: %lx\n", (long)cur);
        if(stride & 0xf)
            fprintf(stderr, "dev16_altivec:incorrect align, stride: %lu\n", stride);
#endif

	dev = mean = vec_splat_u32(0);
	stride >>= 4;
        
	/* set pointer to iterate through cur */
	ptr = cur;
        
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
	MEAN16();
        
        /* Add all together in sumdiffs */
	sumdiffs = (vector unsigned int)vec_sums((vector signed int) mean, vec_splat_s32(0));
        /* teilen durch 16 * 16 */
        mn = vec_perm((vector unsigned char)sumdiffs, (vector unsigned char)sumdiffs, vec_splat_u8(14));

        /* set pointer to iterate through cur */
        ptr = cur;
        
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();
	DEV16();

	/* sum all parts of difference into one 32 bit quantity */
	sumdiffs = (vector unsigned int)vec_sums((vector signed int) dev, vec_splat_s32(0));

	/* copy vector sum into unaligned result */
	sumdiffs = vec_splat(sumdiffs, 3);
	vec_ste(sumdiffs, 0, (uint32_t*) &result);
	return result;
}

#define SAD16BI() \
    t1 = vec_perm(ref1[0], ref1[1], mask1); \
    t2 = vec_perm(ref2[0], ref2[1], mask2); \
    t1 = vec_avg(t1, t2); \
    t2 = vec_max(t1, *cur); \
    t1 = vec_min(t1, *cur); \
    sad = vec_sub(t2, t1); \
    sum = vec_sum4s(sad, sum); \
    cur += stride; \
    ref1 += stride; \
    ref2 += stride

/*
 * This function assumes cur is 16 bytes aligned, stride is 16 bytes
 * aligned and ref1 and ref2 is unaligned
*/

uint32_t
sad16bi_altivec_c(vector unsigned char *cur,
                        vector unsigned char *ref1,
                        vector unsigned char *ref2,
                        uint32_t stride)
{
    vector unsigned char t1, t2;
    vector unsigned char mask1, mask2;
    vector unsigned char sad;
    vector unsigned int sum;
    uint32_t result;
    
#ifdef DEBUG
    /* print alignment errors if this is on */
    if((long)cur & 0xf)
        fprintf(stderr, "sad16bi_altivec:incorrect align, cur: %lx\n", (long)cur);
    if(stride & 0xf)
        fprintf(stderr, "sad16bi_altivec:incorrect align, cur: %lu\n", stride);
#endif
    
    /* Initialisation stuff */
    stride >>= 4;
    mask1 = vec_lvsl(0, (unsigned char*)ref1);
    mask2 = vec_lvsl(0, (unsigned char*)ref2);
    sad = vec_splat_u8(0);
    sum = (vector unsigned int)sad;
    
    SAD16BI();
    SAD16BI();
    SAD16BI();
    SAD16BI();
    
    SAD16BI();
    SAD16BI();
    SAD16BI();
    SAD16BI();
    
    SAD16BI();
    SAD16BI();
    SAD16BI();
    SAD16BI();
    
    SAD16BI();
    SAD16BI();
    SAD16BI();
    SAD16BI();
    
    sum = (vector unsigned int)vec_sums((vector signed int)sum, vec_splat_s32(0));
    sum = vec_splat(sum, 3);
    vec_ste(sum, 0, (uint32_t*)&result);
    
    return result;
}


#define SSE8_16BIT() \
b1_vec = vec_perm(vec_ld(0,b1), vec_ld(16,b1), vec_lvsl(0,b1)); \
b2_vec = vec_perm(vec_ld(0,b2), vec_ld(16,b2), vec_lvsl(0,b2)); \
diff = vec_sub(b1_vec,b2_vec);  \
sum = vec_msum(diff,diff,sum);  \
b1 = (const int16_t*)((int8_t*)b1+stride);  \
b2 = (const int16_t*)((int8_t*)b2+stride)

uint32_t
sse8_16bit_altivec_c(const int16_t * b1,
			 const int16_t * b2,
			 const uint32_t stride)
{
    register vector signed short b1_vec;
    register vector signed short b2_vec;
    register vector signed short diff;
    register vector signed int sum;
    uint32_t result;
    
    /* initialize */
    sum = vec_splat_s32(0);
    
    SSE8_16BIT();
    SSE8_16BIT();
    SSE8_16BIT();
    SSE8_16BIT();
    
    SSE8_16BIT();
    SSE8_16BIT();
    SSE8_16BIT();
    SSE8_16BIT();
        
    /* sum the vector */
    sum = vec_sums(sum, vec_splat_s32(0));
    sum = vec_splat(sum,3);
    
    vec_ste(sum,0,(int*)&result);
    
    /* and return */
    return result;
}
