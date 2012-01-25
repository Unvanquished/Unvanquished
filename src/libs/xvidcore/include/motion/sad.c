/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Sum Of Absolute Difference related code -
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
 * $Id: sad.c,v 1.16 2004/04/12 15:49:56 edgomez Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "../global.h"
#include "sad.h"

#include <stdlib.h>

sad16FuncPtr sad16;
sad8FuncPtr sad8;
sad16biFuncPtr sad16bi;
sad8biFuncPtr sad8bi;		/* not really sad16, but no difference in prototype */
dev16FuncPtr dev16;
sad16vFuncPtr sad16v;
sse8Func_16bitPtr sse8_16bit;
sse8Func_8bitPtr sse8_8bit;

sadInitFuncPtr sadInit;


uint32_t
sad16_c(const uint8_t * const cur,
		const uint8_t * const ref,
		const uint32_t stride,
		const uint32_t best_sad)
{

	uint32_t sad = 0;
	uint32_t j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {
			sad += abs(ptr_cur[0] - ptr_ref[0]);
			sad += abs(ptr_cur[1] - ptr_ref[1]);
			sad += abs(ptr_cur[2] - ptr_ref[2]);
			sad += abs(ptr_cur[3] - ptr_ref[3]);
			sad += abs(ptr_cur[4] - ptr_ref[4]);
			sad += abs(ptr_cur[5] - ptr_ref[5]);
			sad += abs(ptr_cur[6] - ptr_ref[6]);
			sad += abs(ptr_cur[7] - ptr_ref[7]);
			sad += abs(ptr_cur[8] - ptr_ref[8]);
			sad += abs(ptr_cur[9] - ptr_ref[9]);
			sad += abs(ptr_cur[10] - ptr_ref[10]);
			sad += abs(ptr_cur[11] - ptr_ref[11]);
			sad += abs(ptr_cur[12] - ptr_ref[12]);
			sad += abs(ptr_cur[13] - ptr_ref[13]);
			sad += abs(ptr_cur[14] - ptr_ref[14]);
			sad += abs(ptr_cur[15] - ptr_ref[15]);

			if (sad >= best_sad)
				return sad;

			ptr_cur += stride;
			ptr_ref += stride;

	}

	return sad;

}

uint32_t
sad16bi_c(const uint8_t * const cur,
		  const uint8_t * const ref1,
		  const uint8_t * const ref2,
		  const uint32_t stride)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref1 = ref1;
	uint8_t const *ptr_ref2 = ref2;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++) {
			int pixel = (ptr_ref1[i] + ptr_ref2[i] + 1) / 2;
			sad += abs(ptr_cur[i] - pixel);
		}

		ptr_cur += stride;
		ptr_ref1 += stride;
		ptr_ref2 += stride;

	}

	return sad;

}

uint32_t
sad8bi_c(const uint8_t * const cur,
		  const uint8_t * const ref1,
		  const uint8_t * const ref2,
		  const uint32_t stride)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref1 = ref1;
	uint8_t const *ptr_ref2 = ref2;

	for (j = 0; j < 8; j++) {

		for (i = 0; i < 8; i++) {
			int pixel = (ptr_ref1[i] + ptr_ref2[i] + 1) / 2;
			sad += abs(ptr_cur[i] - pixel);
		}

		ptr_cur += stride;
		ptr_ref1 += stride;
		ptr_ref2 += stride;

	}

	return sad;

}



uint32_t
sad8_c(const uint8_t * const cur,
	   const uint8_t * const ref,
	   const uint32_t stride)
{
	uint32_t sad = 0;
	uint32_t j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 8; j++) {

		sad += abs(ptr_cur[0] - ptr_ref[0]);
		sad += abs(ptr_cur[1] - ptr_ref[1]);
		sad += abs(ptr_cur[2] - ptr_ref[2]);
		sad += abs(ptr_cur[3] - ptr_ref[3]);
		sad += abs(ptr_cur[4] - ptr_ref[4]);
		sad += abs(ptr_cur[5] - ptr_ref[5]);
		sad += abs(ptr_cur[6] - ptr_ref[6]);
		sad += abs(ptr_cur[7] - ptr_ref[7]);

		ptr_cur += stride;
		ptr_ref += stride;

	}

	return sad;
}


/* average deviation from mean */

uint32_t
dev16_c(const uint8_t * const cur,
		const uint32_t stride)
{

	uint32_t mean = 0;
	uint32_t dev = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++)
			mean += *(ptr_cur + i);

		ptr_cur += stride;

	}

	mean /= (16 * 16);
	ptr_cur = cur;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++)
			dev += abs(*(ptr_cur + i) - (int32_t) mean);

		ptr_cur += stride;

	}

	return dev;
}

uint32_t sad16v_c(const uint8_t * const cur,
			   const uint8_t * const ref,
			   const uint32_t stride,
			   int32_t *sad)
{
	sad[0] = sad8(cur, ref, stride);
	sad[1] = sad8(cur + 8, ref + 8, stride);
	sad[2] = sad8(cur + 8*stride, ref + 8*stride, stride);
	sad[3] = sad8(cur + 8*stride + 8, ref + 8*stride + 8, stride);

	return sad[0]+sad[1]+sad[2]+sad[3];
}

uint32_t sad32v_c(const uint8_t * const cur,
			   const uint8_t * const ref,
			   const uint32_t stride,
			   int32_t *sad)
{
	sad[0] = sad16(cur, ref, stride, 256*4096);
	sad[1] = sad16(cur + 16, ref + 16, stride, 256*4096);
	sad[2] = sad16(cur + 16*stride, ref + 16*stride, stride, 256*4096);
	sad[3] = sad16(cur + 16*stride + 16, ref + 16*stride + 16, stride, 256*4096);

	return sad[0]+sad[1]+sad[2]+sad[3];
}



#define MRSAD16_CORRFACTOR 8
uint32_t
mrsad16_c(const uint8_t * const cur,
		  const uint8_t * const ref,
		  const uint32_t stride,
		  const uint32_t best_sad)
{

	uint32_t sad = 0;
	int32_t mean = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			mean += ((int) *(ptr_cur + i) - (int) *(ptr_ref + i));
		}
		ptr_cur += stride;
		ptr_ref += stride;

	}
	mean /= 256;

	for (j = 0; j < 16; j++) {

		ptr_cur -= stride;
		ptr_ref -= stride;

		for (i = 0; i < 16; i++) {

			sad += abs(*(ptr_cur + i) - *(ptr_ref + i) - mean);
			if (sad >= best_sad) {
				return MRSAD16_CORRFACTOR * sad;
			}
		}
	}

	return MRSAD16_CORRFACTOR * sad;
}

uint32_t
sse8_16bit_c(const int16_t * b1,
			 const int16_t * b2,
			 const uint32_t stride)
{
	int i;
	int sse = 0;

	for (i=0; i<8; i++) {
		sse += (b1[0] - b2[0])*(b1[0] - b2[0]);
		sse += (b1[1] - b2[1])*(b1[1] - b2[1]);
		sse += (b1[2] - b2[2])*(b1[2] - b2[2]);
		sse += (b1[3] - b2[3])*(b1[3] - b2[3]);
		sse += (b1[4] - b2[4])*(b1[4] - b2[4]);
		sse += (b1[5] - b2[5])*(b1[5] - b2[5]);
		sse += (b1[6] - b2[6])*(b1[6] - b2[6]);
		sse += (b1[7] - b2[7])*(b1[7] - b2[7]);

		b1 = (const int16_t*)((int8_t*)b1+stride);
		b2 = (const int16_t*)((int8_t*)b2+stride);
	}

	return(sse);
}

uint32_t
sse8_8bit_c(const uint8_t * b1,
			const uint8_t * b2,
			const uint32_t stride)
{
	int i;
	int sse = 0;

	for (i=0; i<8; i++) {
		sse += (b1[0] - b2[0])*(b1[0] - b2[0]);
		sse += (b1[1] - b2[1])*(b1[1] - b2[1]);
		sse += (b1[2] - b2[2])*(b1[2] - b2[2]);
		sse += (b1[3] - b2[3])*(b1[3] - b2[3]);
		sse += (b1[4] - b2[4])*(b1[4] - b2[4]);
		sse += (b1[5] - b2[5])*(b1[5] - b2[5]);
		sse += (b1[6] - b2[6])*(b1[6] - b2[6]);
		sse += (b1[7] - b2[7])*(b1[7] - b2[7]);

		b1 = b1+stride;
		b2 = b2+stride;
	}

	return(sse);
}
