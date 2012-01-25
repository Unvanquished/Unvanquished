/*****************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	- 8x8 block-based halfpel interpolation -
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
 * $Id: interpolate8x8.c,v 1.15 2005/09/13 12:12:15 suxen_drol Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "../global.h"
#include "interpolate8x8.h"

/* function pointers */
INTERPOLATE8X8_PTR interpolate8x8_halfpel_h;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_v;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv;

INTERPOLATE8X8_PTR interpolate8x4_halfpel_h;
INTERPOLATE8X8_PTR interpolate8x4_halfpel_v;
INTERPOLATE8X8_PTR interpolate8x4_halfpel_hv;

INTERPOLATE8X8_PTR interpolate8x8_halfpel_add;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_h_add;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_v_add;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv_add;

INTERPOLATE8X8_AVG2_PTR interpolate8x8_avg2;
INTERPOLATE8X8_AVG4_PTR interpolate8x8_avg4;

INTERPOLATE_LOWPASS_PTR interpolate8x8_lowpass_h;
INTERPOLATE_LOWPASS_PTR interpolate8x8_lowpass_v;

INTERPOLATE_LOWPASS_PTR interpolate16x16_lowpass_h;
INTERPOLATE_LOWPASS_PTR interpolate16x16_lowpass_v;

INTERPOLATE_LOWPASS_HV_PTR interpolate8x8_lowpass_hv;
INTERPOLATE_LOWPASS_HV_PTR interpolate16x16_lowpass_hv;

INTERPOLATE8X8_6TAP_LOWPASS_PTR interpolate8x8_6tap_lowpass_h;
INTERPOLATE8X8_6TAP_LOWPASS_PTR interpolate8x8_6tap_lowpass_v;

void
interpolate8x8_avg2_c(uint8_t * dst, const uint8_t * src1, const uint8_t *src2, const uint32_t stride, const uint32_t rounding, const uint32_t height)
{
    uint32_t i;
	const int32_t round = 1 - rounding;

    for(i = 0; i < height; i++) {
        dst[0] = (src1[0] + src2[0] + round) >> 1;
        dst[1] = (src1[1] + src2[1] + round) >> 1;
        dst[2] = (src1[2] + src2[2] + round) >> 1;
        dst[3] = (src1[3] + src2[3] + round) >> 1;
        dst[4] = (src1[4] + src2[4] + round) >> 1;
        dst[5] = (src1[5] + src2[5] + round) >> 1;
        dst[6] = (src1[6] + src2[6] + round) >> 1;
        dst[7] = (src1[7] + src2[7] + round) >> 1;

        dst += stride;
        src1 += stride;
        src2 += stride;
    }
}

void
interpolate8x8_halfpel_add_c(uint8_t * const dst, const uint8_t * const src, const uint32_t stride, const uint32_t rounding)
{
	interpolate8x8_avg2_c(dst, dst, src, stride, 0, 8);
}

void interpolate8x8_avg4_c(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, const uint8_t *src3, const uint8_t *src4, const uint32_t stride, const uint32_t rounding)
{
    int32_t i;
	const int32_t round = 2 - rounding;

    for(i = 0; i < 8; i++) {
        dst[0] = (src1[0] + src2[0] + src3[0] + src4[0] + round) >> 2;
        dst[1] = (src1[1] + src2[1] + src3[1] + src4[1] + round) >> 2;
        dst[2] = (src1[2] + src2[2] + src3[2] + src4[2] + round) >> 2;
        dst[3] = (src1[3] + src2[3] + src3[3] + src4[3] + round) >> 2;
        dst[4] = (src1[4] + src2[4] + src3[4] + src4[4] + round) >> 2;
        dst[5] = (src1[5] + src2[5] + src3[5] + src4[5] + round) >> 2;
        dst[6] = (src1[6] + src2[6] + src3[6] + src4[6] + round) >> 2;
        dst[7] = (src1[7] + src2[7] + src3[7] + src4[7] + round) >> 2;

		dst += stride;
        src1 += stride;
        src2 += stride;
        src3 += stride;
        src4 += stride;
    }
}

/* dst = interpolate(src) */

void
interpolate8x8_halfpel_h_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uintptr_t j;

	if (rounding) {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + 1] )>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + 2] )>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + 3] )>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + 4] )>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + 5] )>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + 6] )>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + 7] )>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + 8] )>>1);
		}
	} else {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + 1] + 1)>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + 2] + 1)>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + 3] + 1)>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + 4] + 1)>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + 5] + 1)>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + 6] + 1)>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + 7] + 1)>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + 8] + 1)>>1);
		}
	}
}

/* dst = interpolate(src) */

void
interpolate8x4_halfpel_h_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uintptr_t j;

	if (rounding) {
		for (j = 0; j < 4*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + 1] )>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + 2] )>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + 3] )>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + 4] )>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + 5] )>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + 6] )>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + 7] )>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + 8] )>>1);
		}
	} else {
		for (j = 0; j < 4*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + 1] + 1)>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + 2] + 1)>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + 3] + 1)>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + 4] + 1)>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + 5] + 1)>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + 6] + 1)>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + 7] + 1)>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + 8] + 1)>>1);
		}
	}
}

/* dst = (dst + interpolate(src)/2 */

void
interpolate8x8_halfpel_h_add_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uintptr_t j;

	if (rounding) {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((((src[j + 0] + src[j + 1] )>>1) + dst[j+0] + 1)>>1);
				dst[j + 1] = (uint8_t)((((src[j + 1] + src[j + 2] )>>1) + dst[j+1] + 1)>>1);
				dst[j + 2] = (uint8_t)((((src[j + 2] + src[j + 3] )>>1) + dst[j+2] + 1)>>1);
				dst[j + 3] = (uint8_t)((((src[j + 3] + src[j + 4] )>>1) + dst[j+3] + 1)>>1);
				dst[j + 4] = (uint8_t)((((src[j + 4] + src[j + 5] )>>1) + dst[j+4] + 1)>>1);
				dst[j + 5] = (uint8_t)((((src[j + 5] + src[j + 6] )>>1) + dst[j+5] + 1)>>1);
				dst[j + 6] = (uint8_t)((((src[j + 6] + src[j + 7] )>>1) + dst[j+6] + 1)>>1);
				dst[j + 7] = (uint8_t)((((src[j + 7] + src[j + 8] )>>1) + dst[j+7] + 1)>>1);
		}
	} else {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((((src[j + 0] + src[j + 1] + 1)>>1) + dst[j+0] + 1)>>1);
				dst[j + 1] = (uint8_t)((((src[j + 1] + src[j + 2] + 1)>>1) + dst[j+1] + 1)>>1);
				dst[j + 2] = (uint8_t)((((src[j + 2] + src[j + 3] + 1)>>1) + dst[j+2] + 1)>>1);
				dst[j + 3] = (uint8_t)((((src[j + 3] + src[j + 4] + 1)>>1) + dst[j+3] + 1)>>1);
				dst[j + 4] = (uint8_t)((((src[j + 4] + src[j + 5] + 1)>>1) + dst[j+4] + 1)>>1);
				dst[j + 5] = (uint8_t)((((src[j + 5] + src[j + 6] + 1)>>1) + dst[j+5] + 1)>>1);
				dst[j + 6] = (uint8_t)((((src[j + 6] + src[j + 7] + 1)>>1) + dst[j+6] + 1)>>1);
				dst[j + 7] = (uint8_t)((((src[j + 7] + src[j + 8] + 1)>>1) + dst[j+7] + 1)>>1);
		}
	}
}

/* dst = interpolate(src) */

void
interpolate8x8_halfpel_v_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uintptr_t j;


	if (rounding) {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + stride + 0] )>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + stride + 1] )>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + stride + 2] )>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + stride + 3] )>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + stride + 4] )>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + stride + 5] )>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + stride + 6] )>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + stride + 7] )>>1);
		}
	} else {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + stride + 0] + 1)>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + stride + 1] + 1)>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + stride + 2] + 1)>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + stride + 3] + 1)>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + stride + 4] + 1)>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + stride + 5] + 1)>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + stride + 6] + 1)>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + stride + 7] + 1)>>1);
		}
	}
}

/* dst = interpolate(src) */

void
interpolate8x4_halfpel_v_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uintptr_t j;


	if (rounding) {
		for (j = 0; j < 4*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + stride + 0] )>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + stride + 1] )>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + stride + 2] )>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + stride + 3] )>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + stride + 4] )>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + stride + 5] )>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + stride + 6] )>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + stride + 7] )>>1);
		}
	} else {
		for (j = 0; j < 4*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j + 0] + src[j + stride + 0] + 1)>>1);
				dst[j + 1] = (uint8_t)((src[j + 1] + src[j + stride + 1] + 1)>>1);
				dst[j + 2] = (uint8_t)((src[j + 2] + src[j + stride + 2] + 1)>>1);
				dst[j + 3] = (uint8_t)((src[j + 3] + src[j + stride + 3] + 1)>>1);
				dst[j + 4] = (uint8_t)((src[j + 4] + src[j + stride + 4] + 1)>>1);
				dst[j + 5] = (uint8_t)((src[j + 5] + src[j + stride + 5] + 1)>>1);
				dst[j + 6] = (uint8_t)((src[j + 6] + src[j + stride + 6] + 1)>>1);
				dst[j + 7] = (uint8_t)((src[j + 7] + src[j + stride + 7] + 1)>>1);
		}
	}
}

/* dst = (dst + interpolate(src))/2 */

void
interpolate8x8_halfpel_v_add_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uintptr_t j;


	if (rounding) {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((((src[j + 0] + src[j + stride + 0] )>>1) + dst[j+0] + 1)>>1);
				dst[j + 1] = (uint8_t)((((src[j + 1] + src[j + stride + 1] )>>1) + dst[j+1] + 1)>>1);
				dst[j + 2] = (uint8_t)((((src[j + 2] + src[j + stride + 2] )>>1) + dst[j+2] + 1)>>1);
				dst[j + 3] = (uint8_t)((((src[j + 3] + src[j + stride + 3] )>>1) + dst[j+3] + 1)>>1);
				dst[j + 4] = (uint8_t)((((src[j + 4] + src[j + stride + 4] )>>1) + dst[j+4] + 1)>>1);
				dst[j + 5] = (uint8_t)((((src[j + 5] + src[j + stride + 5] )>>1) + dst[j+5] + 1)>>1);
				dst[j + 6] = (uint8_t)((((src[j + 6] + src[j + stride + 6] )>>1) + dst[j+6] + 1)>>1);
				dst[j + 7] = (uint8_t)((((src[j + 7] + src[j + stride + 7] )>>1) + dst[j+7] + 1)>>1);
		}
	} else {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((((src[j + 0] + src[j + stride + 0] + 1)>>1) + dst[j+0] + 1)>>1);
				dst[j + 1] = (uint8_t)((((src[j + 1] + src[j + stride + 1] + 1)>>1) + dst[j+1] + 1)>>1);
				dst[j + 2] = (uint8_t)((((src[j + 2] + src[j + stride + 2] + 1)>>1) + dst[j+2] + 1)>>1);
				dst[j + 3] = (uint8_t)((((src[j + 3] + src[j + stride + 3] + 1)>>1) + dst[j+3] + 1)>>1);
				dst[j + 4] = (uint8_t)((((src[j + 4] + src[j + stride + 4] + 1)>>1) + dst[j+4] + 1)>>1);
				dst[j + 5] = (uint8_t)((((src[j + 5] + src[j + stride + 5] + 1)>>1) + dst[j+5] + 1)>>1);
				dst[j + 6] = (uint8_t)((((src[j + 6] + src[j + stride + 6] + 1)>>1) + dst[j+6] + 1)>>1);
				dst[j + 7] = (uint8_t)((((src[j + 7] + src[j + stride + 7] + 1)>>1) + dst[j+7] + 1)>>1);
		}
	}
}

/* dst = interpolate(src) */

void
interpolate8x8_halfpel_hv_c(uint8_t * const dst,
							const uint8_t * const src,
							const uint32_t stride,
							const uint32_t rounding)
{
	uintptr_t j;

	if (rounding) {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j+0] + src[j+1] + src[j+stride+0] + src[j+stride+1] +1)>>2);
				dst[j + 1] = (uint8_t)((src[j+1] + src[j+2] + src[j+stride+1] + src[j+stride+2] +1)>>2);
				dst[j + 2] = (uint8_t)((src[j+2] + src[j+3] + src[j+stride+2] + src[j+stride+3] +1)>>2);
				dst[j + 3] = (uint8_t)((src[j+3] + src[j+4] + src[j+stride+3] + src[j+stride+4] +1)>>2);
				dst[j + 4] = (uint8_t)((src[j+4] + src[j+5] + src[j+stride+4] + src[j+stride+5] +1)>>2);
				dst[j + 5] = (uint8_t)((src[j+5] + src[j+6] + src[j+stride+5] + src[j+stride+6] +1)>>2);
				dst[j + 6] = (uint8_t)((src[j+6] + src[j+7] + src[j+stride+6] + src[j+stride+7] +1)>>2);
				dst[j + 7] = (uint8_t)((src[j+7] + src[j+8] + src[j+stride+7] + src[j+stride+8] +1)>>2);
		}
	} else {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j+0] + src[j+1] + src[j+stride+0] + src[j+stride+1] +2)>>2);
				dst[j + 1] = (uint8_t)((src[j+1] + src[j+2] + src[j+stride+1] + src[j+stride+2] +2)>>2);
				dst[j + 2] = (uint8_t)((src[j+2] + src[j+3] + src[j+stride+2] + src[j+stride+3] +2)>>2);
				dst[j + 3] = (uint8_t)((src[j+3] + src[j+4] + src[j+stride+3] + src[j+stride+4] +2)>>2);
				dst[j + 4] = (uint8_t)((src[j+4] + src[j+5] + src[j+stride+4] + src[j+stride+5] +2)>>2);
				dst[j + 5] = (uint8_t)((src[j+5] + src[j+6] + src[j+stride+5] + src[j+stride+6] +2)>>2);
				dst[j + 6] = (uint8_t)((src[j+6] + src[j+7] + src[j+stride+6] + src[j+stride+7] +2)>>2);
				dst[j + 7] = (uint8_t)((src[j+7] + src[j+8] + src[j+stride+7] + src[j+stride+8] +2)>>2);
		}
	}
}

/* dst = interpolate(src) */

void
interpolate8x4_halfpel_hv_c(uint8_t * const dst,
							const uint8_t * const src,
							const uint32_t stride,
							const uint32_t rounding)
{
	uintptr_t j;

	if (rounding) {
		for (j = 0; j < 4*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j+0] + src[j+1] + src[j+stride+0] + src[j+stride+1] +1)>>2);
				dst[j + 1] = (uint8_t)((src[j+1] + src[j+2] + src[j+stride+1] + src[j+stride+2] +1)>>2);
				dst[j + 2] = (uint8_t)((src[j+2] + src[j+3] + src[j+stride+2] + src[j+stride+3] +1)>>2);
				dst[j + 3] = (uint8_t)((src[j+3] + src[j+4] + src[j+stride+3] + src[j+stride+4] +1)>>2);
				dst[j + 4] = (uint8_t)((src[j+4] + src[j+5] + src[j+stride+4] + src[j+stride+5] +1)>>2);
				dst[j + 5] = (uint8_t)((src[j+5] + src[j+6] + src[j+stride+5] + src[j+stride+6] +1)>>2);
				dst[j + 6] = (uint8_t)((src[j+6] + src[j+7] + src[j+stride+6] + src[j+stride+7] +1)>>2);
				dst[j + 7] = (uint8_t)((src[j+7] + src[j+8] + src[j+stride+7] + src[j+stride+8] +1)>>2);
		}
	} else {
		for (j = 0; j < 4*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((src[j+0] + src[j+1] + src[j+stride+0] + src[j+stride+1] +2)>>2);
				dst[j + 1] = (uint8_t)((src[j+1] + src[j+2] + src[j+stride+1] + src[j+stride+2] +2)>>2);
				dst[j + 2] = (uint8_t)((src[j+2] + src[j+3] + src[j+stride+2] + src[j+stride+3] +2)>>2);
				dst[j + 3] = (uint8_t)((src[j+3] + src[j+4] + src[j+stride+3] + src[j+stride+4] +2)>>2);
				dst[j + 4] = (uint8_t)((src[j+4] + src[j+5] + src[j+stride+4] + src[j+stride+5] +2)>>2);
				dst[j + 5] = (uint8_t)((src[j+5] + src[j+6] + src[j+stride+5] + src[j+stride+6] +2)>>2);
				dst[j + 6] = (uint8_t)((src[j+6] + src[j+7] + src[j+stride+6] + src[j+stride+7] +2)>>2);
				dst[j + 7] = (uint8_t)((src[j+7] + src[j+8] + src[j+stride+7] + src[j+stride+8] +2)>>2);
		}
	}
}

/* dst = (interpolate(src) + dst)/2 */

void
interpolate8x8_halfpel_hv_add_c(uint8_t * const dst,
							const uint8_t * const src,
							const uint32_t stride,
							const uint32_t rounding)
{
	uintptr_t j;

	if (rounding) {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((((src[j+0] + src[j+1] + src[j+stride+0] + src[j+stride+1] +1)>>2) + dst[j+0])>>1);
				dst[j + 1] = (uint8_t)((((src[j+1] + src[j+2] + src[j+stride+1] + src[j+stride+2] +1)>>2) + dst[j+1])>>1);
				dst[j + 2] = (uint8_t)((((src[j+2] + src[j+3] + src[j+stride+2] + src[j+stride+3] +1)>>2) + dst[j+2])>>1);
				dst[j + 3] = (uint8_t)((((src[j+3] + src[j+4] + src[j+stride+3] + src[j+stride+4] +1)>>2) + dst[j+3])>>1);
				dst[j + 4] = (uint8_t)((((src[j+4] + src[j+5] + src[j+stride+4] + src[j+stride+5] +1)>>2) + dst[j+4])>>1);
				dst[j + 5] = (uint8_t)((((src[j+5] + src[j+6] + src[j+stride+5] + src[j+stride+6] +1)>>2) + dst[j+5])>>1);
				dst[j + 6] = (uint8_t)((((src[j+6] + src[j+7] + src[j+stride+6] + src[j+stride+7] +1)>>2) + dst[j+6])>>1);
				dst[j + 7] = (uint8_t)((((src[j+7] + src[j+8] + src[j+stride+7] + src[j+stride+8] +1)>>2) + dst[j+7])>>1);
		}
	} else {
		for (j = 0; j < 8*stride; j+=stride) {
				dst[j + 0] = (uint8_t)((((src[j+0] + src[j+1] + src[j+stride+0] + src[j+stride+1] +2)>>2) + dst[j+0] + 1)>>1);
				dst[j + 1] = (uint8_t)((((src[j+1] + src[j+2] + src[j+stride+1] + src[j+stride+2] +2)>>2) + dst[j+1] + 1)>>1);
				dst[j + 2] = (uint8_t)((((src[j+2] + src[j+3] + src[j+stride+2] + src[j+stride+3] +2)>>2) + dst[j+2] + 1)>>1);
				dst[j + 3] = (uint8_t)((((src[j+3] + src[j+4] + src[j+stride+3] + src[j+stride+4] +2)>>2) + dst[j+3] + 1)>>1);
				dst[j + 4] = (uint8_t)((((src[j+4] + src[j+5] + src[j+stride+4] + src[j+stride+5] +2)>>2) + dst[j+4] + 1)>>1);
				dst[j + 5] = (uint8_t)((((src[j+5] + src[j+6] + src[j+stride+5] + src[j+stride+6] +2)>>2) + dst[j+5] + 1)>>1);
				dst[j + 6] = (uint8_t)((((src[j+6] + src[j+7] + src[j+stride+6] + src[j+stride+7] +2)>>2) + dst[j+6] + 1)>>1);
				dst[j + 7] = (uint8_t)((((src[j+7] + src[j+8] + src[j+stride+7] + src[j+stride+8] +2)>>2) + dst[j+7] + 1)>>1);
		}
	}
}

/*************************************************************
 * QPEL STUFF STARTS HERE                                    *
 *************************************************************/

void interpolate8x8_6tap_lowpass_h_c(uint8_t *dst, uint8_t *src, int32_t stride, int32_t rounding)
{
    int32_t i;
	uint8_t round_add = 16 - rounding;

    for(i = 0; i < 8; i++)
    {

        dst[0] = CLIP((((src[-2] + src[3]) + 5 * (((src[0] + src[1])<<2) - (src[-1] + src[2])) + round_add) >> 5), 0, 255);
        dst[1] = CLIP((((src[-1] + src[4]) + 5 * (((src[1] + src[2])<<2) - (src[0] + src[3])) + round_add) >> 5), 0, 255);
        dst[2] = CLIP((((src[0] + src[5]) + 5 * (((src[2] + src[3])<<2) - (src[1] + src[4])) + round_add) >> 5), 0, 255);
        dst[3] = CLIP((((src[1] + src[6]) + 5 * (((src[3] + src[4])<<2) - (src[2] + src[5])) + round_add) >> 5), 0, 255);
        dst[4] = CLIP((((src[2] + src[7]) + 5 * (((src[4] + src[5])<<2) - (src[3] + src[6])) + round_add) >> 5), 0, 255);
        dst[5] = CLIP((((src[3] + src[8]) + 5 * (((src[5] + src[6])<<2) - (src[4] + src[7])) + round_add) >> 5), 0, 255);
        dst[6] = CLIP((((src[4] + src[9]) + 5 * (((src[6] + src[7])<<2) - (src[5] + src[8])) + round_add) >> 5), 0, 255);
        dst[7] = CLIP((((src[5] + src[10]) + 5 * (((src[7] + src[8])<<2) - (src[6] + src[9])) + round_add) >> 5), 0, 255);

        dst += stride;
        src += stride;
    }
}

void interpolate16x16_lowpass_h_c(uint8_t *dst, uint8_t *src, int32_t stride, int32_t rounding)
{
    int32_t i;
	uint8_t round_add = 16 - rounding;

    for(i = 0; i < 17; i++)
    {

        dst[0] = CLIP(((7 * ((src[0]<<1) - src[2]) +  23 * src[1] + 3 * src[3] - src[4] + round_add) >> 5), 0, 255);
        dst[1] = CLIP(((19 * src[1] + 20 * src[2] - src[5] + 3 * (src[4] - src[0] - (src[3]<<1)) + round_add) >> 5), 0, 255);
        dst[2] = CLIP(((20 * (src[2] + src[3]) + (src[0]<<1) + 3 * (src[5] - ((src[1] + src[4])<<1)) - src[6] + round_add) >> 5), 0, 255);

        dst[3] = CLIP(((20 * (src[3] + src[4]) + 3 * ((src[6] + src[1]) - ((src[2] + src[5])<<1)) - (src[0] + src[7]) + round_add) >> 5), 0, 255);
        dst[4] = CLIP(((20 * (src[4] + src[5]) - 3 * (((src[3] + src[6])<<1) - (src[2] + src[7])) - (src[1] + src[8]) + round_add) >> 5), 0, 255);
        dst[5] = CLIP(((20 * (src[5] + src[6]) - 3 * (((src[4] + src[7])<<1) - (src[3] + src[8])) - (src[2] + src[9]) + round_add) >> 5), 0, 255);
        dst[6] = CLIP(((20 * (src[6] + src[7]) - 3 * (((src[5] + src[8])<<1) - (src[4] + src[9])) - (src[3] + src[10]) + round_add) >> 5), 0, 255);
        dst[7] = CLIP(((20 * (src[7] + src[8]) - 3 * (((src[6] + src[9])<<1) - (src[5] + src[10])) - (src[4] + src[11]) + round_add) >> 5), 0, 255);
        dst[8] = CLIP(((20 * (src[8] + src[9]) - 3 * (((src[7] + src[10])<<1) - (src[6] + src[11])) - (src[5] + src[12]) + round_add) >> 5), 0, 255);
        dst[9] = CLIP(((20 * (src[9] + src[10]) - 3 * (((src[8] + src[11])<<1) - (src[7] + src[12])) - (src[6] + src[13]) + round_add) >> 5), 0, 255);
        dst[10] = CLIP(((20 * (src[10] + src[11]) - 3 * (((src[9] + src[12])<<1) - (src[8] + src[13])) - (src[7] + src[14]) + round_add) >> 5), 0, 255);
        dst[11] = CLIP(((20 * (src[11] + src[12]) - 3 * (((src[10] + src[13])<<1) - (src[9] + src[14])) - (src[8] + src[15]) + round_add) >> 5), 0, 255);
        dst[12] = CLIP(((20 * (src[12] + src[13]) - 3 * (((src[11] + src[14])<<1) - (src[10] + src[15])) - (src[9] + src[16]) + round_add) >> 5), 0, 255);

        dst[13] = CLIP(((20 * (src[13] + src[14]) + (src[16]<<1) + 3 * (src[11] - ((src[12] + src[15]) << 1)) - src[10] + round_add) >> 5), 0, 255);
        dst[14] = CLIP(((19 * src[15] + 20 * src[14] + 3 * (src[12] - src[16] - (src[13] << 1)) - src[11] + round_add) >> 5), 0, 255);
        dst[15] = CLIP(((23 * src[15] + 7 * ((src[16]<<1) - src[14]) + 3 * src[13] - src[12] + round_add) >> 5), 0, 255);

        dst += stride;
        src += stride;
    }
}

void interpolate8x8_lowpass_h_c(uint8_t *dst, uint8_t *src, int32_t stride, int32_t rounding)
{
    int32_t i;
	uint8_t round_add = 16 - rounding;

    for(i = 0; i < 9; i++)
    {

        dst[0] = CLIP(((7 * ((src[0]<<1) - src[2]) + 23 * src[1] + 3 * src[3] - src[4] + round_add) >> 5), 0, 255);
        dst[1] = CLIP(((19 * src[1] + 20 * src[2] - src[5] + 3 * (src[4] - src[0] - (src[3]<<1)) + round_add) >> 5), 0, 255);
        dst[2] = CLIP(((20 * (src[2] + src[3]) + (src[0]<<1) + 3 * (src[5] - ((src[1] + src[4])<<1)) - src[6] + round_add) >> 5), 0, 255);
        dst[3] = CLIP(((20 * (src[3] + src[4]) + 3 * ((src[6] + src[1]) - ((src[2] + src[5])<<1)) - (src[0] + src[7]) + round_add) >> 5), 0, 255);
        dst[4] = CLIP(((20 * (src[4] + src[5]) - 3 * (((src[3] + src[6])<<1) - (src[2] + src[7])) - (src[1] + src[8]) + round_add) >> 5), 0, 255);
        dst[5] = CLIP(((20 * (src[5] + src[6]) + (src[8]<<1) + 3 * (src[3] - ((src[4] + src[7]) << 1)) - src[2] + round_add) >> 5), 0, 255);
        dst[6] = CLIP(((19 * src[7] + 20 * src[6] + 3 * (src[4] - src[8] - (src[5] << 1)) - src[3] + round_add) >> 5), 0, 255);
        dst[7] = CLIP(((23 * src[7] + 7 * ((src[8]<<1) - src[6]) + 3 * src[5] - src[4] + round_add) >> 5), 0, 255);

        dst += stride;
        src += stride;
    }
}

void interpolate8x8_6tap_lowpass_v_c(uint8_t *dst, uint8_t *src, int32_t stride, int32_t rounding)
{
    int32_t i;
	uint8_t round_add = 16 - rounding;

    for(i = 0; i < 8; i++)
    {
        int32_t src_2 = src[-2*stride];
        int32_t src_1 = src[-stride];
        int32_t src0 = src[0];
        int32_t src1 = src[stride];
        int32_t src2 = src[2 * stride];
        int32_t src3 = src[3 * stride];
        int32_t src4 = src[4 * stride];
        int32_t src5 = src[5 * stride];
        int32_t src6 = src[6 * stride];
        int32_t src7 = src[7 * stride];
        int32_t src8 = src[8 * stride];
        int32_t src9 = src[9 * stride];
        int32_t src10 = src[10 * stride];

        dst[0]			= CLIP((((src_2 + src3) + 5 * (((src0 + src1)<<2) - (src_1 + src2)) + round_add) >> 5), 0, 255);
        dst[stride]		= CLIP((((src_1 + src4) + 5 * (((src1 + src2)<<2) - (src0 + src3)) + round_add) >> 5), 0, 255);
        dst[2 * stride] = CLIP((((src0 + src5) + 5 * (((src2 + src3)<<2) - (src1 + src4)) + round_add) >> 5), 0, 255);
        dst[3 * stride] = CLIP((((src1 + src6) + 5 * (((src3 + src4)<<2) - (src2 + src5)) + round_add) >> 5), 0, 255);
        dst[4 * stride] = CLIP((((src2 + src7) + 5 * (((src4 + src5)<<2) - (src3 + src6)) + round_add) >> 5), 0, 255);
        dst[5 * stride] = CLIP((((src3 + src8) + 5 * (((src5 + src6)<<2) - (src4 + src7)) + round_add) >> 5), 0, 255);
        dst[6 * stride] = CLIP((((src4 + src9) + 5 * (((src6 + src7)<<2) - (src5 + src8)) + round_add) >> 5), 0, 255);
        dst[7 * stride] = CLIP((((src5 + src10) + 5 * (((src7 + src8)<<2) - (src6 + src9)) + round_add) >> 5), 0, 255);

		dst++;
        src++;
    }
}

void interpolate16x16_lowpass_v_c(uint8_t *dst, uint8_t *src, int32_t stride, int32_t rounding)
{
    int32_t i;
	uint8_t round_add = 16 - rounding;

    for(i = 0; i < 17; i++)
    {
        int32_t src0 = src[0];
        int32_t src1 = src[stride];
        int32_t src2 = src[2 * stride];
        int32_t src3 = src[3 * stride];
        int32_t src4 = src[4 * stride];
        int32_t src5 = src[5 * stride];
        int32_t src6 = src[6 * stride];
        int32_t src7 = src[7 * stride];
        int32_t src8 = src[8 * stride];
        int32_t src9 = src[9 * stride];
        int32_t src10 = src[10 * stride];
        int32_t src11 = src[11 * stride];
        int32_t src12 = src[12 * stride];
        int32_t src13 = src[13 * stride];
        int32_t src14 = src[14 * stride];
        int32_t src15 = src[15 * stride];
        int32_t src16 = src[16 * stride];


        dst[0] = CLIP(((7 * ((src0<<1) - src2) +  23 * src1 + 3 * src3 - src4 + round_add) >> 5), 0, 255);
        dst[stride] = CLIP(((19 * src1 + 20 * src2 - src5 + 3 * (src4 - src0 - (src3<<1)) + round_add) >> 5), 0, 255);
        dst[2*stride] = CLIP(((20 * (src2 + src3) + (src0<<1) + 3 * (src5 - ((src1 + src4)<<1)) - src6 + round_add) >> 5), 0, 255);

        dst[3*stride] = CLIP(((20 * (src3 + src4) + 3 * ((src6 + src1) - ((src2 + src5)<<1)) - (src0 + src7) + round_add) >> 5), 0, 255);
        dst[4*stride] = CLIP(((20 * (src4 + src5) - 3 * (((src3 + src6)<<1) - (src2 + src7)) - (src1 + src8) + round_add) >> 5), 0, 255);
        dst[5*stride] = CLIP(((20 * (src5 + src6) - 3 * (((src4 + src7)<<1) - (src3 + src8)) - (src2 + src9) + round_add) >> 5), 0, 255);
        dst[6*stride] = CLIP(((20 * (src6 + src7) - 3 * (((src5 + src8)<<1) - (src4 + src9)) - (src3 + src10) + round_add) >> 5), 0, 255);
        dst[7*stride] = CLIP(((20 * (src7 + src8) - 3 * (((src6 + src9)<<1) - (src5 + src10)) - (src4 + src11) + round_add) >> 5), 0, 255);
        dst[8*stride] = CLIP(((20 * (src8 + src9) - 3 * (((src7 + src10)<<1) - (src6 + src11)) - (src5 + src12) + round_add) >> 5), 0, 255);
        dst[9*stride] = CLIP(((20 * (src9 + src10) - 3 * (((src8 + src11)<<1) - (src7 + src12)) - (src6 + src13) + round_add) >> 5), 0, 255);
        dst[10*stride] = CLIP(((20 * (src10 + src11) - 3 * (((src9 + src12)<<1) - (src8 + src13)) - (src7 + src14) + round_add) >> 5), 0, 255);
        dst[11*stride] = CLIP(((20 * (src11 + src12) - 3 * (((src10 + src13)<<1) - (src9 + src14)) - (src8 + src15) + round_add) >> 5), 0, 255);
        dst[12*stride] = CLIP(((20 * (src12 + src13) - 3 * (((src11 + src14)<<1) - (src10 + src15)) - (src9 + src16) + round_add) >> 5), 0, 255);

        dst[13*stride] = CLIP(((20 * (src13 + src14) + (src16<<1) + 3 * (src11 - ((src12 + src15) << 1)) - src10 + round_add) >> 5), 0, 255);
        dst[14*stride] = CLIP(((19 * src15 + 20 * src14 + 3 * (src12 - src16 - (src13 << 1)) - src11 + round_add) >> 5), 0, 255);
        dst[15*stride] = CLIP(((23 * src15 + 7 * ((src16<<1) - src14) + 3 * src13 - src12 + round_add) >> 5), 0, 255);

		dst++;
        src++;
    }
}

void interpolate8x8_lowpass_v_c(uint8_t *dst, uint8_t *src, int32_t stride, int32_t rounding)
{
    int32_t i;
	uint8_t round_add = 16 - rounding;

    for(i = 0; i < 9; i++)
    {
        int32_t src0 = src[0];
        int32_t src1 = src[stride];
        int32_t src2 = src[2 * stride];
        int32_t src3 = src[3 * stride];
        int32_t src4 = src[4 * stride];
        int32_t src5 = src[5 * stride];
        int32_t src6 = src[6 * stride];
        int32_t src7 = src[7 * stride];
        int32_t src8 = src[8 * stride];

        dst[0]			= CLIP(((7 * ((src0<<1) - src2) + 23 * src1 + 3 * src3 - src4 + round_add) >> 5), 0, 255);
        dst[stride]		= CLIP(((19 * src1 + 20 * src2 - src5 + 3 * (src4 - src0 - (src3 << 1)) + round_add) >> 5), 0, 255);
        dst[2 * stride] = CLIP(((20 * (src2 + src3) + (src0<<1) + 3 * (src5 - ((src1 + src4) <<1 )) - src6 + round_add) >> 5), 0, 255);
        dst[3 * stride] = CLIP(((20 * (src3 + src4) + 3 * ((src6 + src1) - ((src2 + src5)<<1)) - (src0 + src7) + round_add) >> 5), 0, 255);
        dst[4 * stride] = CLIP(((20 * (src4 + src5) + 3 * ((src2 + src7) - ((src3 + src6)<<1)) - (src1 + src8) + round_add) >> 5), 0, 255);
        dst[5 * stride] = CLIP(((20 * (src5 + src6) + (src8<<1) + 3 * (src3 - ((src4 + src7) << 1)) - src2 + round_add) >> 5), 0, 255);
        dst[6 * stride] = CLIP(((19 * src7 + 20 * src6 - src3 + 3 * (src4 - src8 - (src5 << 1)) + round_add) >> 5), 0, 255);
        dst[7 * stride] = CLIP(((7 * ((src8<<1) - src6) + 23 * src7 + 3 * src5 - src4 + round_add) >> 5), 0, 255);

		dst++;
        src++;
    }
}

void interpolate16x16_lowpass_hv_c(uint8_t *dst1, uint8_t *dst2, uint8_t *src, int32_t stride, int32_t rounding)
{
	int32_t i;
	uint8_t round_add = 16 - rounding;
	uint8_t *h_ptr = dst2;

    for(i = 0; i < 17; i++)
    {

        h_ptr[0] = CLIP(((7 * ((src[0]<<1) - src[2]) +  23 * src[1] + 3 * src[3] - src[4] + round_add) >> 5), 0, 255);
        h_ptr[1] = CLIP(((19 * src[1] + 20 * src[2] - src[5] + 3 * (src[4] - src[0] - (src[3]<<1)) + round_add) >> 5), 0, 255);
        h_ptr[2] = CLIP(((20 * (src[2] + src[3]) + (src[0]<<1) + 3 * (src[5] - ((src[1] + src[4])<<1)) - src[6] + round_add) >> 5), 0, 255);

        h_ptr[3] = CLIP(((20 * (src[3] + src[4]) + 3 * ((src[6] + src[1]) - ((src[2] + src[5])<<1)) - (src[0] + src[7]) + round_add) >> 5), 0, 255);
        h_ptr[4] = CLIP(((20 * (src[4] + src[5]) - 3 * (((src[3] + src[6])<<1) - (src[2] + src[7])) - (src[1] + src[8]) + round_add) >> 5), 0, 255);
        h_ptr[5] = CLIP(((20 * (src[5] + src[6]) - 3 * (((src[4] + src[7])<<1) - (src[3] + src[8])) - (src[2] + src[9]) + round_add) >> 5), 0, 255);
        h_ptr[6] = CLIP(((20 * (src[6] + src[7]) - 3 * (((src[5] + src[8])<<1) - (src[4] + src[9])) - (src[3] + src[10]) + round_add) >> 5), 0, 255);
        h_ptr[7] = CLIP(((20 * (src[7] + src[8]) - 3 * (((src[6] + src[9])<<1) - (src[5] + src[10])) - (src[4] + src[11]) + round_add) >> 5), 0, 255);
        h_ptr[8] = CLIP(((20 * (src[8] + src[9]) - 3 * (((src[7] + src[10])<<1) - (src[6] + src[11])) - (src[5] + src[12]) + round_add) >> 5), 0, 255);
        h_ptr[9] = CLIP(((20 * (src[9] + src[10]) - 3 * (((src[8] + src[11])<<1) - (src[7] + src[12])) - (src[6] + src[13]) + round_add) >> 5), 0, 255);
        h_ptr[10] = CLIP(((20 * (src[10] + src[11]) - 3 * (((src[9] + src[12])<<1) - (src[8] + src[13])) - (src[7] + src[14]) + round_add) >> 5), 0, 255);
        h_ptr[11] = CLIP(((20 * (src[11] + src[12]) - 3 * (((src[10] + src[13])<<1) - (src[9] + src[14])) - (src[8] + src[15]) + round_add) >> 5), 0, 255);
        h_ptr[12] = CLIP(((20 * (src[12] + src[13]) - 3 * (((src[11] + src[14])<<1) - (src[10] + src[15])) - (src[9] + src[16]) + round_add) >> 5), 0, 255);

        h_ptr[13] = CLIP(((20 * (src[13] + src[14]) + (src[16]<<1) + 3 * (src[11] - ((src[12] + src[15]) << 1)) - src[10] + round_add) >> 5), 0, 255);
        h_ptr[14] = CLIP(((19 * src[15] + 20 * src[14] + 3 * (src[12] - src[16] - (src[13] << 1)) - src[11] + round_add) >> 5), 0, 255);
        h_ptr[15] = CLIP(((23 * src[15] + 7 * ((src[16]<<1) - src[14]) + 3 * src[13] - src[12] + round_add) >> 5), 0, 255);

        h_ptr += stride;
        src += stride;
    }

	interpolate16x16_lowpass_v_c(dst1, dst2, stride, rounding);

}

void interpolate8x8_lowpass_hv_c(uint8_t *dst1, uint8_t *dst2, uint8_t *src, int32_t stride, int32_t rounding)
{
	int32_t i;
	uint8_t round_add = 16 - rounding;
	uint8_t *h_ptr = dst2;

    for(i = 0; i < 9; i++)
    {

        h_ptr[0] = CLIP(((7 * ((src[0]<<1) - src[2]) + 23 * src[1] + 3 * src[3] - src[4] + round_add) >> 5), 0, 255);
        h_ptr[1] = CLIP(((19 * src[1] + 20 * src[2] - src[5] + 3 * (src[4] - src[0] - (src[3]<<1)) + round_add) >> 5), 0, 255);
        h_ptr[2] = CLIP(((20 * (src[2] + src[3]) + (src[0]<<1) + 3 * (src[5] - ((src[1] + src[4])<<1)) - src[6] + round_add) >> 5), 0, 255);
        h_ptr[3] = CLIP(((20 * (src[3] + src[4]) + 3 * ((src[6] + src[1]) - ((src[2] + src[5])<<1)) - (src[0] + src[7]) + round_add) >> 5), 0, 255);
        h_ptr[4] = CLIP(((20 * (src[4] + src[5]) - 3 * (((src[3] + src[6])<<1) - (src[2] + src[7])) - (src[1] + src[8]) + round_add) >> 5), 0, 255);
        h_ptr[5] = CLIP(((20 * (src[5] + src[6]) + (src[8]<<1) + 3 * (src[3] - ((src[4] + src[7]) << 1)) - src[2] + round_add) >> 5), 0, 255);
        h_ptr[6] = CLIP(((19 * src[7] + 20 * src[6] + 3 * (src[4] - src[8] - (src[5] << 1)) - src[3] + round_add) >> 5), 0, 255);
        h_ptr[7] = CLIP(((23 * src[7] + 7 * ((src[8]<<1) - src[6]) + 3 * src[5] - src[4] + round_add) >> 5), 0, 255);

        h_ptr += stride;
        src += stride;
    }

	interpolate8x8_lowpass_v_c(dst1, dst2, stride, rounding);

}
