/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Image management functions -
 *
 *  Copyright(C) 2001-2004 Peter Ross <pross@xvid.org>
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
 * $Id: image.c,v 1.43.2.1 2009/05/25 08:31:16 Isibaar Exp $
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>				/* memcpy, memset */
#include <math.h>
#include "../portab.h"
#include "../global.h"			/* XVID_CSP_XXX's */
#include "../xvid.h"			/* XVID_CSP_XXX's */
#include "image.h"
#include "colorspace.h"
#include "interpolate8x8.h"
#include "../utils/mem_align.h"
#include "../motion/sad.h"
#include "../utils/emms.h"

#include "font.h"		/* XXX: remove later */

#define SAFETY	64
#define EDGE_SIZE2  (EDGE_SIZE/2)


int32_t
image_create(IMAGE * image,
			 uint32_t edged_width,
			 uint32_t edged_height)
{
	const uint32_t edged_width2 = edged_width / 2;
	const uint32_t edged_height2 = edged_height / 2;

	image->y =
		xvid_malloc(edged_width * (edged_height + 1) + SAFETY, CACHE_LINE);
	if (image->y == NULL) {
		return -1;
	}
	memset(image->y, 0, edged_width * (edged_height + 1) + SAFETY);

	image->u = xvid_malloc(edged_width2 * edged_height2 + SAFETY, CACHE_LINE);
	if (image->u == NULL) {
		xvid_free(image->y);
		image->y = NULL;
		return -1;
	}
	memset(image->u, 0, edged_width2 * edged_height2 + SAFETY);

	image->v = xvid_malloc(edged_width2 * edged_height2 + SAFETY, CACHE_LINE);
	if (image->v == NULL) {
		xvid_free(image->u);
		image->u = NULL;
		xvid_free(image->y);
		image->y = NULL;
		return -1;
	}
	memset(image->v, 0, edged_width2 * edged_height2 + SAFETY);

	image->y += EDGE_SIZE * edged_width + EDGE_SIZE;
	image->u += EDGE_SIZE2 * edged_width2 + EDGE_SIZE2;
	image->v += EDGE_SIZE2 * edged_width2 + EDGE_SIZE2;

	return 0;
}



void
image_destroy(IMAGE * image,
			  uint32_t edged_width,
			  uint32_t edged_height)
{
	const uint32_t edged_width2 = edged_width / 2;

	if (image->y) {
		xvid_free(image->y - (EDGE_SIZE * edged_width + EDGE_SIZE));
		image->y = NULL;
	}
	if (image->u) {
		xvid_free(image->u - (EDGE_SIZE2 * edged_width2 + EDGE_SIZE2));
		image->u = NULL;
	}
	if (image->v) {
		xvid_free(image->v - (EDGE_SIZE2 * edged_width2 + EDGE_SIZE2));
		image->v = NULL;
	}
}


void
image_swap(IMAGE * image1,
		   IMAGE * image2)
{
    SWAP(uint8_t*, image1->y, image2->y);
    SWAP(uint8_t*, image1->u, image2->u);
    SWAP(uint8_t*, image1->v, image2->v);
}


void
image_copy(IMAGE * image1,
		   IMAGE * image2,
		   uint32_t edged_width,
		   uint32_t height)
{
	memcpy(image1->y, image2->y, edged_width * height);
	memcpy(image1->u, image2->u, edged_width * height / 4);
	memcpy(image1->v, image2->v, edged_width * height / 4);
}

/* setedges bug was fixed in this BS version */
#define SETEDGES_BUG_BEFORE		18

void
image_setedges(IMAGE * image,
			   uint32_t edged_width,
			   uint32_t edged_height,
			   uint32_t width,
			   uint32_t height,
			   int bs_version)
{
	const uint32_t edged_width2 = edged_width / 2;
	uint32_t width2;
	uint32_t i;
	uint8_t *dst;
	uint8_t *src;

	dst = image->y - (EDGE_SIZE + EDGE_SIZE * edged_width);
	src = image->y;

	/* According to the Standard Clause 7.6.4, padding is done starting at 16
	 * pixel width and height multiples. This was not respected in old xvids */
	if (bs_version == 0 || bs_version >= SETEDGES_BUG_BEFORE) {
		width  = (width+15)&~15;
		height = (height+15)&~15;
	}

	width2 = width/2;

	for (i = 0; i < EDGE_SIZE; i++) {
		memset(dst, *src, EDGE_SIZE);
		memcpy(dst + EDGE_SIZE, src, width);
		memset(dst + edged_width - EDGE_SIZE, *(src + width - 1),
			   EDGE_SIZE);
		dst += edged_width;
	}

	for (i = 0; i < height; i++) {
		memset(dst, *src, EDGE_SIZE);
		memset(dst + edged_width - EDGE_SIZE, src[width - 1], EDGE_SIZE);
		dst += edged_width;
		src += edged_width;
	}

	src -= edged_width;
	for (i = 0; i < EDGE_SIZE; i++) {
		memset(dst, *src, EDGE_SIZE);
		memcpy(dst + EDGE_SIZE, src, width);
		memset(dst + edged_width - EDGE_SIZE, *(src + width - 1),
				   EDGE_SIZE);
		dst += edged_width;
	}


	/* U */
	dst = image->u - (EDGE_SIZE2 + EDGE_SIZE2 * edged_width2);
	src = image->u;

	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}

	for (i = 0; i < height / 2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
		src += edged_width2;
	}
	src -= edged_width2;
	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}


	/* V */
	dst = image->v - (EDGE_SIZE2 + EDGE_SIZE2 * edged_width2);
	src = image->v;

	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}

	for (i = 0; i < height / 2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
		src += edged_width2;
	}
	src -= edged_width2;
	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}
}

void
image_interpolate(const uint8_t * refn,
				  uint8_t * refh,
				  uint8_t * refv,
				  uint8_t * refhv,
				  uint32_t edged_width,
				  uint32_t edged_height,
				  uint32_t quarterpel,
				  uint32_t rounding)
{
	const uint32_t offset = EDGE_SIZE2 * (edged_width + 1); /* we only interpolate half of the edge area */
	const uint32_t stride_add = 7 * edged_width;

	uint8_t *n_ptr;
	uint8_t *h_ptr, *v_ptr, *hv_ptr;
	uint32_t x, y;

	n_ptr = (uint8_t*)refn;
	h_ptr = refh;
	v_ptr = refv;

	n_ptr -= offset;
	h_ptr -= offset;
	v_ptr -= offset;

	/* Note we initialize the hv pointer later, as we can optimize code a bit
	 * doing it down to up in quarterpel and up to down in halfpel */
	if(quarterpel) {

		for (y = 0; y < (edged_height - EDGE_SIZE); y += 8) {
			for (x = 0; x < (edged_width - EDGE_SIZE); x += 8) {
				interpolate8x8_6tap_lowpass_h(h_ptr, n_ptr, edged_width, rounding);
				interpolate8x8_6tap_lowpass_v(v_ptr, n_ptr, edged_width, rounding);

				n_ptr += 8;
				h_ptr += 8;
				v_ptr += 8;
			}

			n_ptr += EDGE_SIZE;
			h_ptr += EDGE_SIZE;
			v_ptr += EDGE_SIZE;

			h_ptr += stride_add;
			v_ptr += stride_add;
			n_ptr += stride_add;
		}

		h_ptr = refh + (edged_height - EDGE_SIZE - EDGE_SIZE2)*edged_width - EDGE_SIZE2;
		hv_ptr = refhv + (edged_height - EDGE_SIZE - EDGE_SIZE2)*edged_width - EDGE_SIZE2;

		for (y = 0; y < (edged_height - EDGE_SIZE); y = y + 8) {
			hv_ptr -= stride_add;
			h_ptr -= stride_add;
			hv_ptr -= EDGE_SIZE;
			h_ptr -= EDGE_SIZE;

			for (x = 0; x < (edged_width - EDGE_SIZE); x = x + 8) {
				hv_ptr -= 8;
				h_ptr -= 8;
				interpolate8x8_6tap_lowpass_v(hv_ptr, h_ptr, edged_width, rounding);
			}
		}
	} else {

		hv_ptr = refhv;
		hv_ptr -= offset;

		for (y = 0; y < (edged_height - EDGE_SIZE); y += 8) {
			for (x = 0; x < (edged_width - EDGE_SIZE); x += 8) {
				interpolate8x8_halfpel_h(h_ptr, n_ptr, edged_width, rounding);
				interpolate8x8_halfpel_v(v_ptr, n_ptr, edged_width, rounding);
				interpolate8x8_halfpel_hv(hv_ptr, n_ptr, edged_width, rounding);

				n_ptr += 8;
				h_ptr += 8;
				v_ptr += 8;
				hv_ptr += 8;
			}

			h_ptr += EDGE_SIZE;
			v_ptr += EDGE_SIZE;
			hv_ptr += EDGE_SIZE;
			n_ptr += EDGE_SIZE;

			h_ptr += stride_add;
			v_ptr += stride_add;
			hv_ptr += stride_add;
			n_ptr += stride_add;
		}
	}
}


/*
chroma optimize filter, invented by mf
a chroma pixel is average from the surrounding pixels, when the
correpsonding luma pixels are pure black or white.
*/

void
image_chroma_optimize(IMAGE * img, int width, int height, int edged_width)
{
	int x,y;
	int pixels = 0;

	for (y = 1; y < height/2 - 1; y++)
	for (x = 1; x < width/2 - 1; x++)
	{
#define IS_PURE(a)  ((a)<=16||(a)>=235)
#define IMG_Y(Y,X)	img->y[(Y)*edged_width + (X)]
#define IMG_U(Y,X)	img->u[(Y)*edged_width/2 + (X)]
#define IMG_V(Y,X)	img->v[(Y)*edged_width/2 + (X)]

		if (IS_PURE(IMG_Y(y*2  ,x*2  )) &&
			IS_PURE(IMG_Y(y*2  ,x*2+1)) &&
			IS_PURE(IMG_Y(y*2+1,x*2  )) &&
			IS_PURE(IMG_Y(y*2+1,x*2+1)))
		{
			IMG_U(y,x) = (IMG_U(y,x-1) + IMG_U(y-1, x) + IMG_U(y, x+1) + IMG_U(y+1, x)) / 4;
			IMG_V(y,x) = (IMG_V(y,x-1) + IMG_V(y-1, x) + IMG_V(y, x+1) + IMG_V(y+1, x)) / 4;
			pixels++;
		}

#undef IS_PURE
#undef IMG_Y
#undef IMG_U
#undef IMG_V
	}

	DPRINTF(XVID_DEBUG_DEBUG,"chroma_optimized_pixels = %i/%i\n", pixels, width*height/4);
}





/*
  perform safe packed colorspace conversion, by splitting
  the image up into an optimized area (pixel width divisible by 16),
  and two unoptimized/plain-c areas (pixel width divisible by 2)
*/

static void
safe_packed_conv(uint8_t * x_ptr, int x_stride,
				 uint8_t * y_ptr, uint8_t * u_ptr, uint8_t * v_ptr,
				 int y_stride, int uv_stride,
				 int width, int height, int vflip,
				 packedFunc * func_opt, packedFunc func_c, 
                 int size, int interlacing)
{
	int width_opt, width_c, height_opt;

    if (width<0 || width==1 || height==1) return; /* forget about it */

	if (func_opt != func_c && x_stride < size*((width+15)/16)*16)
	{
		width_opt = width & (~15);
		width_c = (width - width_opt) & (~1);
	}
	else if (func_opt != func_c && !(width&1) && (size==3))
	{
        /* MMX reads 4 bytes per pixel for RGB/BGR */
        width_opt = width - 2;
        width_c = 2;
    }
    else {
        /* Enforce the width to be divisable by two. */
		width_opt = width & (~1);
		width_c = 0;
	}

    /* packed conversions require height to be divisable by 2
       (or even by 4 for interlaced conversion) */
    if (interlacing)
        height_opt = height & (~3);
    else
        height_opt = height & (~1);

	func_opt(x_ptr, x_stride,
			y_ptr, u_ptr, v_ptr, y_stride, uv_stride,
			width_opt, height_opt, vflip);

	if (width_c)
	{
		func_c(x_ptr + size*width_opt, x_stride,
			y_ptr + width_opt, u_ptr + width_opt/2, v_ptr + width_opt/2,
			y_stride, uv_stride, width_c, height_opt, vflip);
	}
}



int
image_input(IMAGE * image,
			uint32_t width,
			int height,
			uint32_t edged_width,
			uint8_t * src[4],
			int src_stride[4],
			int csp,
			int interlacing)
{
	const int edged_width2 = edged_width/2;
	const int width2 = width/2;
	const int height2 = height/2;
#if 0
	const int height_signed = (csp & XVID_CSP_VFLIP) ? -height : height;
#endif

	switch (csp & ~XVID_CSP_VFLIP) {
	case XVID_CSP_RGB555:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgb555i_to_yv12  :rgb555_to_yv12,
			interlacing?rgb555i_to_yv12_c:rgb555_to_yv12_c, 2, interlacing);
		break;

	case XVID_CSP_RGB565:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgb565i_to_yv12  :rgb565_to_yv12,
			interlacing?rgb565i_to_yv12_c:rgb565_to_yv12_c, 2, interlacing);
		break;


	case XVID_CSP_BGR:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?bgri_to_yv12  :bgr_to_yv12,
			interlacing?bgri_to_yv12_c:bgr_to_yv12_c, 3, interlacing);
		break;

	case XVID_CSP_BGRA:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?bgrai_to_yv12  :bgra_to_yv12,
			interlacing?bgrai_to_yv12_c:bgra_to_yv12_c, 4, interlacing);
		break;

	case XVID_CSP_ABGR :
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?abgri_to_yv12  :abgr_to_yv12,
			interlacing?abgri_to_yv12_c:abgr_to_yv12_c, 4, interlacing);
		break;

	case XVID_CSP_RGB:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgbi_to_yv12  :rgb_to_yv12,
			interlacing?rgbi_to_yv12_c:rgb_to_yv12_c, 3, interlacing);
		break;
            
	case XVID_CSP_RGBA :
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgbai_to_yv12  :rgba_to_yv12,
			interlacing?rgbai_to_yv12_c:rgba_to_yv12_c, 4, interlacing);
		break;
            
	case XVID_CSP_ARGB:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?argbi_to_yv12  : argb_to_yv12,
			interlacing?argbi_to_yv12_c: argb_to_yv12_c, 4, interlacing);
		break;

	case XVID_CSP_YUY2:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yuyvi_to_yv12  :yuyv_to_yv12,
			interlacing?yuyvi_to_yv12_c:yuyv_to_yv12_c, 2, interlacing);
		break;

	case XVID_CSP_YVYU:		/* u/v swapped */
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->v, image->u,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yuyvi_to_yv12  :yuyv_to_yv12,
			interlacing?yuyvi_to_yv12_c:yuyv_to_yv12_c, 2, interlacing);
		break;

	case XVID_CSP_UYVY:
		safe_packed_conv(
			src[0], src_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?uyvyi_to_yv12  :uyvy_to_yv12,
			interlacing?uyvyi_to_yv12_c:uyvy_to_yv12_c, 2, interlacing);
		break;

	case XVID_CSP_I420:	/* YCbCr == YUV == internal colorspace for MPEG */
		yv12_to_yv12(image->y, image->u, image->v, edged_width, edged_width2,
			src[0], src[0] + src_stride[0]*height, src[0] + src_stride[0]*height + (src_stride[0]/2)*height2,
			src_stride[0], src_stride[0]/2, width, height, (csp & XVID_CSP_VFLIP));
		break;

	case XVID_CSP_YV12: /* YCrCb == YVA == U and V plane swapped */
		yv12_to_yv12(image->y, image->v, image->u, edged_width, edged_width2,
			src[0], src[0] + src_stride[0]*height, src[0] + src_stride[0]*height + (src_stride[0]/2)*height2,
			src_stride[0], src_stride[0]/2, width, height, (csp & XVID_CSP_VFLIP));
		break;

	case XVID_CSP_PLANAR:  /* YCbCr with arbitrary pointers and different strides for Y and UV */
		yv12_to_yv12(image->y, image->u, image->v, edged_width, edged_width2,
			src[0], src[1], src[2], src_stride[0], src_stride[1],  /* v: dst_stride[2] not yet supported */
			width, height, (csp & XVID_CSP_VFLIP));
		break;

	case XVID_CSP_NULL:
		break;

	default :
		return -1;
	}


	/* pad out image when the width and/or height is not a multiple of 16 */

	if (width & 15)
	{
		int i;
		int pad_width = 16 - (width&15);
		for (i = 0; i < height; i++)
		{
			memset(image->y + i*edged_width + width,
				 *(image->y + i*edged_width + width - 1), pad_width);
		}
		for (i = 0; i < height/2; i++)
		{
			memset(image->u + i*edged_width2 + width2,
				 *(image->u + i*edged_width2 + width2 - 1),pad_width/2);
			memset(image->v + i*edged_width2 + width2,
				 *(image->v + i*edged_width2 + width2 - 1),pad_width/2);
		}
	}

	if (height & 15)
	{
		int pad_height = 16 - (height&15);
		int length = ((width+15)/16)*16;
		int i;
		for (i = 0; i < pad_height; i++)
		{
			memcpy(image->y + (height+i)*edged_width,
				   image->y + (height-1)*edged_width,length);
		}

		for (i = 0; i < pad_height/2; i++)
		{
			memcpy(image->u + (height2+i)*edged_width2,
				   image->u + (height2-1)*edged_width2,length/2);
			memcpy(image->v + (height2+i)*edged_width2,
				   image->v + (height2-1)*edged_width2,length/2);
		}
	}

/*
	if (interlacing)
		image_printf(image, edged_width, height, 5,5, "[i]");
	image_dump_yuvpgm(image, edged_width, ((width+15)/16)*16, ((height+15)/16)*16, "\\encode.pgm");
*/
	return 0;
}



int
image_output(IMAGE * image,
			 uint32_t width,
			 int height,
			 uint32_t edged_width,
			 uint8_t * dst[4],
			 int dst_stride[4],
			 int csp,
			 int interlacing)
{
	const int edged_width2 = edged_width/2;
	int height2 = height/2;

/*
	if (interlacing)
		image_printf(image, edged_width, height, 5,100, "[i]=%i,%i",width,height);
	image_dump_yuvpgm(image, edged_width, width, height, "\\decode.pgm");
*/

	switch (csp & ~XVID_CSP_VFLIP) {
	case XVID_CSP_RGB555:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgb555i  :yv12_to_rgb555,
			interlacing?yv12_to_rgb555i_c:yv12_to_rgb555_c, 2, interlacing);
		return 0;

	case XVID_CSP_RGB565:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgb565i  :yv12_to_rgb565,
			interlacing?yv12_to_rgb565i_c:yv12_to_rgb565_c, 2, interlacing);
		return 0;

    case XVID_CSP_BGR:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_bgri  :yv12_to_bgr,
			interlacing?yv12_to_bgri_c:yv12_to_bgr_c, 3, interlacing);
		return 0;

	case XVID_CSP_BGRA:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_bgrai  :yv12_to_bgra,
			interlacing?yv12_to_bgrai_c:yv12_to_bgra_c, 4, interlacing);
		return 0;

	case XVID_CSP_ABGR:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_abgri  :yv12_to_abgr,
			interlacing?yv12_to_abgri_c:yv12_to_abgr_c, 4, interlacing);
		return 0;

	case XVID_CSP_RGB:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgbi  :yv12_to_rgb,
			interlacing?yv12_to_rgbi_c:yv12_to_rgb_c, 3, interlacing);
		return 0;

	case XVID_CSP_RGBA:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgbai  :yv12_to_rgba,
			interlacing?yv12_to_rgbai_c:yv12_to_rgba_c, 4, interlacing);
		return 0;

	case XVID_CSP_ARGB:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_argbi  :yv12_to_argb,
			interlacing?yv12_to_argbi_c:yv12_to_argb_c, 4, interlacing);
		return 0;

	case XVID_CSP_YUY2:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_yuyvi  :yv12_to_yuyv,
			interlacing?yv12_to_yuyvi_c:yv12_to_yuyv_c, 2, interlacing);
		return 0;

	case XVID_CSP_YVYU:		/* u,v swapped */
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->v, image->u,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_yuyvi  :yv12_to_yuyv,
			interlacing?yv12_to_yuyvi_c:yv12_to_yuyv_c, 2, interlacing);
		return 0;

	case XVID_CSP_UYVY:
		safe_packed_conv(
			dst[0], dst_stride[0], image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_uyvyi  :yv12_to_uyvy,
			interlacing?yv12_to_uyvyi_c:yv12_to_uyvy_c, 2, interlacing);
		return 0;

	case XVID_CSP_I420: /* YCbCr == YUV == internal colorspace for MPEG */
		yv12_to_yv12(dst[0], dst[0] + dst_stride[0]*height, dst[0] + dst_stride[0]*height + (dst_stride[0]/2)*height2,
			dst_stride[0], dst_stride[0]/2,
			image->y, image->u, image->v, edged_width, edged_width2,
			width, height, (csp & XVID_CSP_VFLIP));
		return 0;

	case XVID_CSP_YV12:	/* YCrCb == YVU == U and V plane swapped */
		yv12_to_yv12(dst[0], dst[0] + dst_stride[0]*height, dst[0] + dst_stride[0]*height + (dst_stride[0]/2)*height2,
			dst_stride[0], dst_stride[0]/2,
			image->y, image->v, image->u, edged_width, edged_width2,
			width, height, (csp & XVID_CSP_VFLIP));
		return 0;

	case XVID_CSP_PLANAR:  /* YCbCr with arbitrary pointers and different strides for Y and UV */
		yv12_to_yv12(dst[0], dst[1], dst[2],
			dst_stride[0], dst_stride[1],	/* v: dst_stride[2] not yet supported */
			image->y, image->u, image->v, edged_width, edged_width2,
			width, height, (csp & XVID_CSP_VFLIP));
		return 0;

	case XVID_CSP_INTERNAL :
		dst[0] = image->y;
		dst[1] = image->u;
		dst[2] = image->v;
		dst_stride[0] = edged_width;
		dst_stride[1] = edged_width/2;
		dst_stride[2] = edged_width/2;
		return 0;

	case XVID_CSP_NULL:
	case XVID_CSP_SLICE:
		return 0;

	}

	return -1;
}

float
image_psnr(IMAGE * orig_image,
		   IMAGE * recon_image,
		   uint16_t stride,
		   uint16_t width,
		   uint16_t height)
{
	int32_t diff, x, y, quad = 0;
	uint8_t *orig = orig_image->y;
	uint8_t *recon = recon_image->y;
	float psnr_y;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			diff = *(orig + x) - *(recon + x);
			quad += diff * diff;
		}
		orig += stride;
		recon += stride;
	}

	psnr_y = (float) quad / (float) (width * height);

	if (psnr_y) {
		psnr_y = (float) (255 * 255) / psnr_y;
		psnr_y = 10 * (float) log10(psnr_y);
	} else
		psnr_y = (float) 99.99;

	return psnr_y;
}


float sse_to_PSNR(long sse, int pixels)
{
        if (sse==0)
                return 99.99F;

        return 48.131F - 10*(float)log10((float)sse/(float)(pixels));   /* log10(255*255)=4.8131 */

}

long plane_sse(uint8_t *orig,
			   uint8_t *recon,
			   uint16_t stride,
			   uint16_t width,
			   uint16_t height)
{
	int y, bwidth, bheight;
	long sse = 0;

	bwidth  = width  & (~0x07);
	bheight = height & (~0x07);

	/* Compute the 8x8 integer part */
	for (y = 0; y<bheight; y += 8) {
		int x;

		/* Compute sse for the band */
		for (x = 0; x<bwidth; x += 8)
			sse += sse8_8bit(orig  + x, recon + x, stride);

		/* remaining pixels of the 8 pixels high band */
		for (x = bwidth; x < width; x++) {
			int diff;
			diff = *(orig + 0*stride + x) - *(recon + 0*stride + x);
			sse += diff * diff;
			diff = *(orig + 1*stride + x) - *(recon + 1*stride + x);
			sse += diff * diff;
			diff = *(orig + 2*stride + x) - *(recon + 2*stride + x);
			sse += diff * diff;
			diff = *(orig + 3*stride + x) - *(recon + 3*stride + x);
			sse += diff * diff;
			diff = *(orig + 4*stride + x) - *(recon + 4*stride + x);
			sse += diff * diff;
			diff = *(orig + 5*stride + x) - *(recon + 5*stride + x);
			sse += diff * diff;
			diff = *(orig + 6*stride + x) - *(recon + 6*stride + x);
			sse += diff * diff;
			diff = *(orig + 7*stride + x) - *(recon + 7*stride + x);
			sse += diff * diff;
		}

		orig  += 8*stride;
		recon += 8*stride;
	}

	/* Compute the down rectangle sse */
	for (y = bheight; y < height; y++) {
		int x;
		for (x = 0; x < width; x++) {
			int diff;
			diff = *(orig + x) - *(recon + x);
			sse += diff * diff;
		}
		orig += stride;
		recon += stride;
	}

	return (sse);
}

#if 0

#include <stdio.h>
#include <string.h>

int image_dump_pgm(uint8_t * bmp, uint32_t width, uint32_t height, char * filename)
{
	FILE * f;
	char hdr[1024];

	f = fopen(filename, "wb");
	if ( f == NULL)
	{
		return -1;
	}
	sprintf(hdr, "P5\n#xvid\n%i %i\n255\n", width, height);
	fwrite(hdr, strlen(hdr), 1, f);
	fwrite(bmp, width, height, f);
	fclose(f);

	return 0;
}


/* dump image+edges to yuv pgm files */

int image_dump(IMAGE * image, uint32_t edged_width, uint32_t edged_height, char * path, int number)
{
	char filename[1024];

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'y');
	image_dump_pgm(
		image->y - (EDGE_SIZE * edged_width + EDGE_SIZE),
		edged_width, edged_height, filename);

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'u');
	image_dump_pgm(
		image->u - (EDGE_SIZE2 * edged_width / 2 + EDGE_SIZE2),
		edged_width / 2, edged_height / 2, filename);

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'v');
	image_dump_pgm(
		image->v - (EDGE_SIZE2 * edged_width / 2 + EDGE_SIZE2),
		edged_width / 2, edged_height / 2, filename);

	return 0;
}
#endif



/* dump image to yuvpgm file */

#include <stdio.h>

int
image_dump_yuvpgm(const IMAGE * image,
				  const uint32_t edged_width,
				  const uint32_t width,
				  const uint32_t height,
				  char *filename)
{
	FILE *f;
	char hdr[1024];
	uint32_t i;
	uint8_t *bmp1;
	uint8_t *bmp2;


	f = fopen(filename, "wb");
	if (f == NULL) {
		return -1;
	}
	sprintf(hdr, "P5\n#xvid\n%i %i\n255\n", width, (3 * height) / 2);
	fwrite(hdr, strlen(hdr), 1, f);

	bmp1 = image->y;
	for (i = 0; i < height; i++) {
		fwrite(bmp1, width, 1, f);
		bmp1 += edged_width;
	}

	bmp1 = image->u;
	bmp2 = image->v;
	for (i = 0; i < height / 2; i++) {
		fwrite(bmp1, width / 2, 1, f);
		fwrite(bmp2, width / 2, 1, f);
		bmp1 += edged_width / 2;
		bmp2 += edged_width / 2;
	}

	fclose(f);
	return 0;
}


float
image_mad(const IMAGE * img1,
		  const IMAGE * img2,
		  uint32_t stride,
		  uint32_t width,
		  uint32_t height)
{
	const uint32_t stride2 = stride / 2;
	const uint32_t width2 = width / 2;
	const uint32_t height2 = height / 2;

	uint32_t x, y;
	uint32_t sum = 0;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			sum += abs(img1->y[x + y * stride] - img2->y[x + y * stride]);

	for (y = 0; y < height2; y++)
		for (x = 0; x < width2; x++)
			sum += abs(img1->u[x + y * stride2] - img2->u[x + y * stride2]);

	for (y = 0; y < height2; y++)
		for (x = 0; x < width2; x++)
			sum += abs(img1->v[x + y * stride2] - img2->v[x + y * stride2]);

	return (float) sum / (width * height * 3 / 2);
}

void
output_slice(IMAGE * cur, int stride, int width, xvid_image_t* out_frm, int mbx, int mby,int mbl) {
  uint8_t *dY,*dU,*dV,*sY,*sU,*sV;
  int stride2 = stride >> 1;
  int w = mbl << 4, w2,i;

  if(w > width)
    w = width;
  w2 = w >> 1;

  dY = (uint8_t*)out_frm->plane[0] + (mby << 4) * out_frm->stride[0] + (mbx << 4);
  dU = (uint8_t*)out_frm->plane[1] + (mby << 3) * out_frm->stride[1] + (mbx << 3);
  dV = (uint8_t*)out_frm->plane[2] + (mby << 3) * out_frm->stride[2] + (mbx << 3);
  sY = cur->y + (mby << 4) * stride + (mbx << 4);
  sU = cur->u + (mby << 3) * stride2 + (mbx << 3);
  sV = cur->v + (mby << 3) * stride2 + (mbx << 3);

  for(i = 0 ; i < 16 ; i++) {
    memcpy(dY,sY,w);
    dY += out_frm->stride[0];
    sY += stride;
  }
  for(i = 0 ; i < 8 ; i++) {
    memcpy(dU,sU,w2);
    dU += out_frm->stride[1];
    sU += stride2;
  }
  for(i = 0 ; i < 8 ; i++) {
    memcpy(dV,sV,w2);
    dV += out_frm->stride[2];
    sV += stride2;
  }
}


void
image_clear(IMAGE * img, int width, int height, int edged_width,
					int y, int u, int v)
{
	uint8_t * p;
	int i;

	p = img->y;
	for (i = 0; i < height; i++) {
		memset(p, y, width);
		p += edged_width;
	}

	p = img->u;
	for (i = 0; i < height/2; i++) {
		memset(p, u, width/2);
		p += edged_width/2;
	}

	p = img->v;
	for (i = 0; i < height/2; i++) {
		memset(p, v, width/2);
		p += edged_width/2;
	}
}

/****************************************************************************/

static void (*deintl_core)(uint8_t *, int width, int height, const int stride) = 0;
extern void xvid_deinterlace_sse(uint8_t *, int width, int height, const int stride);

#define CLIP_255(x)   ( ((x)&~255) ? ((-(x)) >> (8*sizeof((x))-1))&0xff : (x) )

static void deinterlace_c(uint8_t *pix, int width, int height, const int bps)
{
  pix += bps;
  while(width-->0)
  {
    int p1 = pix[-bps];
    int p2 = pix[0];
    int p0 = p2;
    int j = (height>>1) - 1;
    int V;
    unsigned char *P = pix++;
    while(j-->0)
    {
      const int  p3 = P[  bps];
      const int  p4 = P[2*bps];
      V =  ((p1+p3+1)>>1) + ((p2 - ((p0+p4+1)>>1)) >> 2);
      P[0] = CLIP_255( V );
      p0 = p2;
      p1 = p3;
      p2 = p4;
      P += 2*bps;
    }
    V =  ((p1+p1+1)>>1) + ((p2 - ((p0+p2+1)>>1)) >> 2);
    P[0] = CLIP_255( V );
  }
}
#undef CLIP_255

int xvid_image_deinterlace(xvid_image_t* img, int width, int height, int bottom_first)
{
	if (height&1)
		return 0;
	if (img->csp!=XVID_CSP_PLANAR && img->csp!=XVID_CSP_I420 && img->csp!=XVID_CSP_YV12)
		return 0;       /* not yet supported */	
	if (deintl_core==0) {
		deintl_core = deinterlace_c;
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
		{
			int cpu_flags = check_cpu_features();
			if (cpu_flags & XVID_CPU_MMX)
				deintl_core = xvid_deinterlace_sse;
		}
#endif			
	}
	if (!bottom_first) {
		deintl_core(img->plane[0], width,    height,    img->stride[0]);
		deintl_core(img->plane[1], width>>1, height>>1, img->stride[1]);
		deintl_core(img->plane[2], width>>1, height>>1, img->stride[2]);
	}
	else {
		deintl_core((uint8_t *)img->plane[0] + ( height    -1)*img->stride[0], width,    height,    -img->stride[0]);
		deintl_core((uint8_t *)img->plane[1] + ((height>>1)-1)*img->stride[1], width>>1, height>>1, -img->stride[1]);
		deintl_core((uint8_t *)img->plane[2] + ((height>>1)-1)*img->stride[2], width>>1, height>>1, -img->stride[2]);
	}
	emms();

	return 1;
}

