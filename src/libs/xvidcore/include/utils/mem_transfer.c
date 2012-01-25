/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8bit<->16bit transfer  -
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
 * $Id: mem_transfer.c,v 1.16 2005/09/13 12:12:15 suxen_drol Exp $
 *
 ****************************************************************************/

#include "../global.h"
#include "mem_transfer.h"

/* Function pointers - Initialized in the xvid.c module */

TRANSFER_8TO16COPY_PTR transfer_8to16copy;
TRANSFER_16TO8COPY_PTR transfer_16to8copy;

TRANSFER_8TO16SUB_PTR  transfer_8to16sub;
TRANSFER_8TO16SUBRO_PTR  transfer_8to16subro;
TRANSFER_8TO16SUB2_PTR transfer_8to16sub2;
TRANSFER_8TO16SUB2RO_PTR transfer_8to16sub2ro;
TRANSFER_16TO8ADD_PTR  transfer_16to8add;

TRANSFER8X8_COPY_PTR transfer8x8_copy;
TRANSFER8X4_COPY_PTR transfer8x4_copy;

#define USE_REFERENCE_C

/*****************************************************************************
 *
 * All these functions are used to transfer data from a 8 bit data array
 * to a 16 bit data array.
 *
 * This is typically used during motion compensation, that's why some
 * functions also do the addition/substraction of another buffer during the
 * so called  transfer.
 *
 ****************************************************************************/

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (8bit)  = SRC
 *    DST (16bit) = SRC
 */
void
transfer_8to16copy_c(int16_t * const dst,
					 const uint8_t * const src,
					 uint32_t stride)
{
	int i, j;
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			dst[j * 8 + i] = (int16_t) src[j * stride + i];
		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(SRC, 255), 0)
 */
void
transfer_16to8copy_c(uint8_t * const dst,
					 const int16_t * const src,
					 uint32_t stride)
{
	int i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
#ifdef USE_REFERENCE_C
			int16_t pixel = src[j * 8 + i];

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}
			dst[j * stride + i] = (uint8_t) pixel;
#else
			const int16_t pixel = src[j * 8 + i];
			const uint8_t value = (uint8_t)( (pixel&~255) ? (-pixel)>>(8*sizeof(pixel)-1) : pixel );
			dst[j*stride + i] = value;
#endif
    }
	}
}




/*
 * C   - the current buffer
 * R   - the reference buffer
 * DCT - the dct coefficient buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    R   (8bit)  = R
 *    C   (8bit)  = R
 *    DCT (16bit) = C - R
 */
void
transfer_8to16sub_c(int16_t * const dct,
					uint8_t * const cur,
					const uint8_t * ref,
					const uint32_t stride)
{
	int i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			const uint8_t c = cur[j * stride + i];
			const uint8_t r = ref[j * stride + i];

			cur[j * stride + i] = r;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}


void
transfer_8to16subro_c(int16_t * const dct,
					const uint8_t * const cur,
					const uint8_t * ref,
					const uint32_t stride)
{
	int i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			const uint8_t c = cur[j * stride + i];
			const uint8_t r = ref[j * stride + i];
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}



/*
 * C   - the current buffer
 * R1  - the 1st reference buffer
 * R2  - the 2nd reference buffer
 * DCT - the dct coefficient buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    R1  (8bit) = R1
 *    R2  (8bit) = R2
 *    R   (temp) = min((R1 + R2)/2, 255)
 *    DCT (16bit)= C - R
 *    C   (8bit) = R
 */
void
transfer_8to16sub2_c(int16_t * const dct,
					 uint8_t * const cur,
					 const uint8_t * ref1,
					 const uint8_t * ref2,
					 const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			const uint8_t c = cur[j * stride + i];
			const uint8_t r = (ref1[j * stride + i] + ref2[j * stride + i] + 1) >> 1;
			cur[j * stride + i] = r;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}

void
transfer_8to16sub2ro_c(int16_t * const dct,
					 const uint8_t * const cur,
					 const uint8_t * ref1,
					 const uint8_t * ref2,
					 const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			const uint8_t c = cur[j * stride + i];
			const uint8_t r = (ref1[j * stride + i] + ref2[j * stride + i] + 1) >> 1;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}


/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 16->8 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(DST+SRC, 255), 0)
 */
void
transfer_16to8add_c(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride)
{
	int i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
#ifdef USE_REFERENCE_C
			int16_t pixel = (int16_t) dst[j * stride + i] + src[j * 8 + i];

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}
			dst[j * stride + i] = (uint8_t) pixel;
#else
      const int16_t pixel = (int16_t) dst[j * stride + i] + src[j * 8 + i];
			const uint8_t value = (uint8_t)( (pixel&~255) ? (-pixel)>>(8*sizeof(pixel)-1) : pixel );
			dst[j*stride + i] = value;
#endif

		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->8 bit transfer and this serie of operations :
 *
 *    SRC (8bit) = SRC
 *    DST (8bit) = SRC
 */
void
transfer8x8_copy_c(uint8_t * const dst,
				   const uint8_t * const src,
				   const uint32_t stride)
{
	int j, i;

	for (j = 0; j < 8; ++j) {
	    uint8_t *d = dst + j*stride;
		const uint8_t *s = src + j*stride;

		for (i = 0; i < 8; ++i)
		{
			*d++ = *s++;
		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->8 bit transfer and this serie of operations :
 *
 *    SRC (8bit) = SRC
 *    DST (8bit) = SRC
 */
void
transfer8x4_copy_c(uint8_t * const dst,
				   const uint8_t * const src,
				   const uint32_t stride)
{
	uint32_t j;

	for (j = 0; j < 4; j++) {
		uint32_t *d= (uint32_t*)(dst + j*stride);
		const uint32_t *s = (const uint32_t*)(src + j*stride);
		*(d+0) = *(s+0);
		*(d+1) = *(s+1);
	}
}
