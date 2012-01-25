/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Postprocessing header  -
 *
 *  Copyright(C) 2003 Michael Militzer <isibaar@xvid.org>
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
 * $Id: postprocessing.h,v 1.6 2004/07/15 10:08:22 suxen_drol Exp $
 *
 ****************************************************************************/

#ifndef _POSTPROCESSING_H_
#define _POSTPROCESSING_H_

#include <stdlib.h>
#include "../portab.h"

/* Filtering thresholds */

#define THR1 2
#define THR2 6

#define MAX_NOISE 4096
#define MAX_SHIFT 1024
#define MAX_RES (MAX_NOISE - MAX_SHIFT)

#define DERING_STRENGTH		2

typedef struct {
	int8_t  xvid_thresh_tbl[511];
	uint8_t xvid_abs_tbl[511];
	int8_t  xvid_noise1[MAX_NOISE * sizeof(int8_t)];
	int8_t  xvid_noise2[MAX_NOISE * sizeof(int8_t)];
	int8_t  *xvid_prev_shift[MAX_RES][6];
	int prev_quant;
} XVID_POSTPROC;

void
image_postproc(XVID_POSTPROC *tbls, IMAGE * img, int edged_width,
				const MACROBLOCK * mbs, int mb_width, int mb_height, int mb_stride,
				int flags, int brightness, int frame_num, int bvop);

void deblock8x8_h(XVID_POSTPROC *tbls, uint8_t *img, int stride, int quant, int dering);
void deblock8x8_v(XVID_POSTPROC *tbls, uint8_t *img, int stride, int quant, int dering);

void init_postproc(XVID_POSTPROC *tbls);
void init_noise(XVID_POSTPROC *tbls);
void init_deblock(XVID_POSTPROC *tbls);

void add_noise(XVID_POSTPROC * tbls, uint8_t *dst, uint8_t *src, int stride, int width, int height, int shiftptr, int quant);


typedef void (IMAGEBRIGHTNESS) (uint8_t * dst,
						int stride,
						int width, 
						int height, 
						int offset);
typedef IMAGEBRIGHTNESS *IMAGEBRIGHTNESS_PTR;

extern IMAGEBRIGHTNESS_PTR image_brightness;

IMAGEBRIGHTNESS image_brightness_c;
IMAGEBRIGHTNESS image_brightness_mmx;
IMAGEBRIGHTNESS image_brightness_sse2;

#endif
