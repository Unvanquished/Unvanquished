/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Bitstream reader/writer inlined functions and constants-
 *
 *  Copyright (C) 2001-2003 Peter Ross <pross@xvid.org>
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
 * $Id: bitstream.h,v 1.24 2006/09/03 08:46:56 Skal Exp $
 *
 ****************************************************************************/

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include "../portab.h"
#include "../decoder.h"
#include "../encoder.h"


/*****************************************************************************
 * Constants
 ****************************************************************************/

/* comment any #defs we dont use */

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
/*#define VIDSESERR_ERROR_CODE  0x000001b4 */
#define VISOBJ_START_CODE		0x000001b5
#define VOP_START_CODE			0x000001b6
/*#define STUFFING_START_CODE	0x000001c3 */


#define VISOBJ_TYPE_VIDEO				1
/*#define VISOBJ_TYPE_STILLTEXTURE      2 */
/*#define VISOBJ_TYPE_MESH              3 */
/*#define VISOBJ_TYPE_FBA               4 */
/*#define VISOBJ_TYPE_3DMESH            5 */


#define VIDOBJLAY_TYPE_SIMPLE			1
/*#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE    2 */
/*#define VIDOBJLAY_TYPE_CORE				3 */
/*#define VIDOBJLAY_TYPE_MAIN				4 */
/*#define VIDOBJLAY_TYPE_NBIT				5 */
/*#define VIDOBJLAY_TYPE_ANIM_TEXT			6 */
/*#define VIDOBJLAY_TYPE_ANIM_MESH			7 */
/*#define VIDOBJLAY_TYPE_SIMPLE_FACE		8 */
/*#define VIDOBJLAY_TYPE_STILL_SCALABLE		9 */
#define VIDOBJLAY_TYPE_ART_SIMPLE		10
/*#define VIDOBJLAY_TYPE_CORE_SCALABLE		11 */
/*#define VIDOBJLAY_TYPE_ACE				12 */
/*#define VIDOBJLAY_TYPE_ADVANCED_SCALABLE_TEXTURE 13 */
/*#define VIDOBJLAY_TYPE_SIMPLE_FBA			14 */
/*#define VIDEOJLAY_TYPE_SIMPLE_STUDIO    15*/
/*#define VIDEOJLAY_TYPE_CORE_STUDIO      16*/
#define VIDOBJLAY_TYPE_ASP              17
/*#define VIDOBJLAY_TYPE_FGS              18*/


/*#define VIDOBJLAY_AR_SQUARE           1 */
/*#define VIDOBJLAY_AR_625TYPE_43       2 */
/*#define VIDOBJLAY_AR_525TYPE_43       3 */
/*#define VIDOBJLAY_AR_625TYPE_169      8 */
/*#define VIDOBJLAY_AR_525TYPE_169      9 */
#define VIDOBJLAY_AR_EXTPAR				15


#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3


#define SPRITE_NONE		0
#define SPRITE_STATIC	1
#define SPRITE_GMC		2



#define READ_MARKER()	BitstreamSkip(bs, 1)
#define WRITE_MARKER()	BitstreamPutBit(bs, 1)

/* vop coding types  */
/* intra, prediction, backward, sprite, not_coded */
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4

/* resync-specific */
#define NUMBITS_VP_RESYNC_MARKER  17
#define RESYNC_MARKER 1


/*****************************************************************************
 * Prototypes
 ****************************************************************************/

int read_video_packet_header(Bitstream *bs,
							 DECODER * dec,
							 const int addbits,
							 int *quant,
							 int *fcode_forward,
							 int *fcode_backward,
							 int *intra_dc_threshold);

/* header stuff */
int BitstreamReadHeaders(Bitstream * bs,
						 DECODER * dec,
						 uint32_t * rounding,
						 uint32_t * quant,
						 uint32_t * fcode_forward,
						 uint32_t * fcode_backward,
						 uint32_t * intra_dc_threshold,
						 WARPPOINTS * gmc_warp);


void BitstreamWriteVolHeader(Bitstream * const bs,
							 const MBParam * pParam,
							 const FRAMEINFO * const frame);

void BitstreamWriteVopHeader(Bitstream * const bs,
							 const MBParam * pParam,
							 const FRAMEINFO * const frame,
							 int vop_coded,
							 unsigned int quant);

void BitstreamWriteUserData(Bitstream * const bs,
							const char *data,
							const unsigned int length);

void
BitstreamWriteEndOfSequence(Bitstream * const bs);

void
BitstreamWriteGroupOfVopHeader(Bitstream * const bs,
                               const MBParam * pParam,
                               uint32_t is_closed_gov);

void write_video_packet_header(Bitstream * const bs,
                               const MBParam * pParam,
                               const FRAMEINFO * const frame,
                               int mbnum);

/*****************************************************************************
 * Bitstream
 ****************************************************************************/

/* Input buffer should be readable as full chunks of 8bytes, including
the end of the buffer. Padding might be appropriate. If only chunks
of 4bytes are applicable, define XVID_SAFE_BS_TAIL. Note that this will
slow decoding, so consider this as a last-resort solution */
/* #define XVID_SAFE_BS_TAIL */

/* initialise bitstream structure */

static void __inline
BitstreamInit(Bitstream * const bs,
			  void *const bitstream,
			  uint32_t length)
{
	uint32_t tmp;
	size_t bitpos;
	ptr_t adjbitstream = (ptr_t)bitstream;

	/*
	 * Start the stream on a uint32_t boundary, by rounding down to the
	 * previous uint32_t and skipping the intervening bytes.
	 */
	bitpos = ((sizeof(uint32_t)-1) & (size_t)bitstream);
	adjbitstream = adjbitstream - bitpos;
	bs->start = bs->tail = (uint32_t *) adjbitstream;

	tmp = *bs->start;
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufa = tmp;

	tmp = *(bs->start + 1);
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufb = tmp;

	bs->pos = bs->initpos = bitpos*8;
	/* preserve the intervening bytes */
	if (bs->initpos > 0)
		bs->buf = bs->bufa & (0xffffffff << (32 - bs->initpos));
	else
		bs->buf = 0;
	bs->length = length;
}


/* reset bitstream state */

static void __inline
BitstreamReset(Bitstream * const bs)
{
	uint32_t tmp;

	bs->tail = bs->start;

	tmp = *bs->start;
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufa = tmp;

	tmp = *(bs->start + 1);
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufb = tmp;

	/* preserve the intervening bytes */
	if (bs->initpos > 0)
		bs->buf = bs->bufa & (0xffffffff << (32 - bs->initpos));
	else
		bs->buf = 0;
	bs->pos = bs->initpos;
}


/* reads n bits from bitstream without changing the stream pos */

static uint32_t __inline
BitstreamShowBits(Bitstream * const bs,
				  const uint32_t bits)
{
	int nbit = (bits + bs->pos) - 32;

	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) | (bs->
																 bufb >> (32 -
																		  nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
	}
}


/* skip n bits forward in bitstream */

static __inline void
BitstreamSkip(Bitstream * const bs,
			  const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
		uint32_t tmp;

		bs->bufa = bs->bufb;
#if defined(XVID_SAFE_BS_TAIL)
		if (bs->tail<(bs->start+((bs->length+3)>>2)))
#endif
		{
			tmp = *((uint32_t *) bs->tail + 2);
#ifndef ARCH_IS_BIG_ENDIAN
			BSWAP(tmp);
#endif
			bs->bufb = tmp;
			bs->tail++;
		}
#if defined(XVID_SAFE_BS_TAIL)
		else {
			bs->bufb = 0;
		}
#endif
		bs->pos -= 32;
	}
}


/* number of bits to next byte alignment */
static __inline uint32_t
BitstreamNumBitsToByteAlign(Bitstream *bs)
{
	uint32_t n = (32 - bs->pos) % 8;
	return n == 0 ? 8 : n;
}


/* show nbits from next byte alignment */
static __inline uint32_t
BitstreamShowBitsFromByteAlign(Bitstream *bs, int bits)
{
	int bspos = bs->pos + BitstreamNumBitsToByteAlign(bs);
	int nbit = (bits + bspos) - 32;

	if (bspos >= 32) {
		return bs->bufb >> (32 - nbit);
	} else	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bspos)) << nbit) | (bs->
																 bufb >> (32 -
																		  nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bspos)) >> (32 - bspos - bits);
	}

}



/* move forward to the next byte boundary */

static __inline void
BitstreamByteAlign(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;

	if (remainder) {
		BitstreamSkip(bs, 8 - remainder);
	}
}


/* bitstream length (unit bits) */

static uint32_t __inline
BitstreamPos(const Bitstream * const bs)
{
	return((uint32_t)(8*((ptr_t)bs->tail - (ptr_t)bs->start) + bs->pos - bs->initpos));
}


/*
 * flush the bitstream & return length (unit bytes)
 * NOTE: assumes no futher bitstream functions will be called.
 */

static uint32_t __inline
BitstreamLength(Bitstream * const bs)
{
	uint32_t len = (uint32_t)((ptr_t)bs->tail - (ptr_t)bs->start);

	if (bs->pos) {
		uint32_t b = bs->buf;

#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail = b;

		len += (bs->pos + 7) / 8;
	}

	/* initpos is always on a byte boundary */
	if (bs->initpos)
		len -= bs->initpos/8;

	return len;
}


/* move bitstream position forward by n bits and write out buffer if needed */

static void __inline
BitstreamForward(Bitstream * const bs,
				 const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
		uint32_t b = bs->buf;

#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail++ = b;
		bs->buf = 0;
		bs->pos -= 32;
	}
}

/* read n bits from bitstream */

static uint32_t __inline
BitstreamGetBits(Bitstream * const bs,
				 const uint32_t n)
{
	uint32_t ret = BitstreamShowBits(bs, n);

	BitstreamSkip(bs, n);
	return ret;
}


/* read single bit from bitstream */

static uint32_t __inline
BitstreamGetBit(Bitstream * const bs)
{
	return BitstreamGetBits(bs, 1);
}


/* write single bit to bitstream */

static void __inline
BitstreamPutBit(Bitstream * const bs,
				const uint32_t bit)
{
	if (bit)
		bs->buf |= (0x80000000 >> bs->pos);

	BitstreamForward(bs, 1);
}


/* write n bits to bitstream */

static void __inline
BitstreamPutBits(Bitstream * const bs,
				 const uint32_t value,
				 const uint32_t size)
{
	uint32_t shift = 32 - bs->pos - size;

	if (shift <= 32) {
		bs->buf |= value << shift;
		BitstreamForward(bs, size);
	} else {
		uint32_t remainder;

		shift = size - (32 - bs->pos);
		bs->buf |= value >> shift;
		BitstreamForward(bs, size - shift);
		remainder = shift;

		shift = 32 - shift;

		bs->buf |= value << shift;
		BitstreamForward(bs, remainder);
	}
}

static const int stuffing_codes[8] =
{
	        /* nbits     stuffing code */
	0,		/* 1          0 */
	1,		/* 2          01 */
	3,		/* 3          011 */
	7,		/* 4          0111 */
	0xf,	/* 5          01111 */
	0x1f,	/* 6          011111 */
	0x3f,   /* 7          0111111 */
	0x7f,	/* 8          01111111 */
};

/* pad bitstream to the next byte boundary */

static void __inline
BitstreamPad(Bitstream * const bs)
{
	int bits = 8 - (bs->pos % 8);
	if (bits < 8)
		BitstreamPutBits(bs, stuffing_codes[bits - 1], bits);
}


/*
 * pad bitstream to the next byte boundary
 * alway pad: even if currently at the byte boundary
 */

static void __inline
BitstreamPadAlways(Bitstream * const bs)
{
	int bits = 8 - (bs->pos % 8);
	BitstreamPutBits(bs, stuffing_codes[bits - 1], bits);
}

#endif /* _BITSTREAM_H_ */
