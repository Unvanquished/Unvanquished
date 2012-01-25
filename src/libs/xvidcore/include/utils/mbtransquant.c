/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - MB Transfert/Quantization functions -
 *
 *  Copyright(C) 2001-2003  Peter Ross <pross@xvid.org>
 *               2001-2003  Michael Militzer <isibaar@xvid.org>
 *               2003       Edouard Gomez <ed.gomez@free.fr>
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
 * $Id: mbtransquant.c,v 1.32 2006/07/10 08:09:59 syskin Exp $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../portab.h"
#include "mbfunctions.h"

#include "../global.h"
#include "mem_transfer.h"
#include "timer.h"
#include "../bitstream/mbcoding.h"
#include "../bitstream/zigzag.h"
#include "../dct/fdct.h"
#include "../dct/idct.h"
#include "../quant/quant.h"
#include "../encoder.h"

#include  "../quant/quant_matrix.h"

MBFIELDTEST_PTR MBFieldTest;

/*
 * Skip blocks having a coefficient sum below this value. This value will be
 * corrected according to the MB quantizer to avoid artifacts for quant==1
 */
#define PVOP_TOOSMALL_LIMIT 1
#define BVOP_TOOSMALL_LIMIT 3

/*****************************************************************************
 * Local functions
 ****************************************************************************/

/* permute block and return field dct choice */
static __inline uint32_t
MBDecideFieldDCT(int16_t data[6 * 64])
{
	uint32_t field = MBFieldTest(data);

	if (field)
		MBFrameToField(data);

	return field;
}

/* Performs Forward DCT on all blocks */
static __inline void
MBfDCT(const MBParam * const pParam,
	   const FRAMEINFO * const frame,
	   MACROBLOCK * const pMB,
	   uint32_t x_pos,
	   uint32_t y_pos,
	   int16_t data[6 * 64])
{
	/* Handles interlacing */
	start_timer();
	pMB->field_dct = 0;
	if ((frame->vol_flags & XVID_VOL_INTERLACING) &&
		(x_pos>0) && (x_pos<pParam->mb_width-1) &&
		(y_pos>0) && (y_pos<pParam->mb_height-1)) {
		pMB->field_dct = MBDecideFieldDCT(data);
	}
	stop_interlacing_timer();

	/* Perform DCT */
	start_timer();
	fdct((short * const)&data[0 * 64]);
	fdct((short * const)&data[1 * 64]);
	fdct((short * const)&data[2 * 64]);
	fdct((short * const)&data[3 * 64]);
	fdct((short * const)&data[4 * 64]);
	fdct((short * const)&data[5 * 64]);
	stop_dct_timer();
}

/* Performs Inverse DCT on all blocks */
static __inline void
MBiDCT(int16_t data[6 * 64],
	   const uint8_t cbp)
{
	start_timer();
	if(cbp & (1 << (5 - 0))) idct((short * const)&data[0 * 64]);
	if(cbp & (1 << (5 - 1))) idct((short * const)&data[1 * 64]);
	if(cbp & (1 << (5 - 2))) idct((short * const)&data[2 * 64]);
	if(cbp & (1 << (5 - 3))) idct((short * const)&data[3 * 64]);
	if(cbp & (1 << (5 - 4))) idct((short * const)&data[4 * 64]);
	if(cbp & (1 << (5 - 5))) idct((short * const)&data[5 * 64]);
	stop_idct_timer();
}

/* Quantize all blocks -- Intra mode */
static __inline void
MBQuantIntra(const MBParam * pParam,
			 const FRAMEINFO * const frame,
			 const MACROBLOCK * pMB,
			 int16_t qcoeff[6 * 64],
			 int16_t data[6*64])
{
	int scaler_lum, scaler_chr;
	quant_intraFuncPtr quant;

	/* check if quant matrices need to be re-initialized with new quant */
	if (pParam->vol_flags & XVID_VOL_MPEGQUANT) {
		if (pParam->last_quant_initialized_intra != pMB->quant) {
			init_intra_matrix(pParam->mpeg_quant_matrices, pMB->quant);
		}
		quant = quant_mpeg_intra;
	} else {
		quant = quant_h263_intra;
	}

	scaler_lum = get_dc_scaler(pMB->quant, 1);
	scaler_chr = get_dc_scaler(pMB->quant, 0);

	/* Quantize the block */
	start_timer();
	quant(&data[0 * 64], &qcoeff[0 * 64], pMB->quant, scaler_lum, pParam->mpeg_quant_matrices);
	quant(&data[1 * 64], &qcoeff[1 * 64], pMB->quant, scaler_lum, pParam->mpeg_quant_matrices);
	quant(&data[2 * 64], &qcoeff[2 * 64], pMB->quant, scaler_lum, pParam->mpeg_quant_matrices);
	quant(&data[3 * 64], &qcoeff[3 * 64], pMB->quant, scaler_lum, pParam->mpeg_quant_matrices);
	quant(&data[4 * 64], &qcoeff[4 * 64], pMB->quant, scaler_chr, pParam->mpeg_quant_matrices);
	quant(&data[5 * 64], &qcoeff[5 * 64], pMB->quant, scaler_chr, pParam->mpeg_quant_matrices);
	stop_quant_timer();
}

/* DeQuantize all blocks -- Intra mode */
static __inline void
MBDeQuantIntra(const MBParam * pParam,
			   const int iQuant,
			   int16_t qcoeff[6 * 64],
			   int16_t data[6*64])
{
	int mpeg;
	int scaler_lum, scaler_chr;

	quant_intraFuncPtr const dequant[2] =
		{
			dequant_h263_intra,
			dequant_mpeg_intra
		};

	mpeg = !!(pParam->vol_flags & XVID_VOL_MPEGQUANT);
	scaler_lum = get_dc_scaler(iQuant, 1);
	scaler_chr = get_dc_scaler(iQuant, 0);

	start_timer();
	dequant[mpeg](&qcoeff[0 * 64], &data[0 * 64], iQuant, scaler_lum, pParam->mpeg_quant_matrices);
	dequant[mpeg](&qcoeff[1 * 64], &data[1 * 64], iQuant, scaler_lum, pParam->mpeg_quant_matrices);
	dequant[mpeg](&qcoeff[2 * 64], &data[2 * 64], iQuant, scaler_lum, pParam->mpeg_quant_matrices);
	dequant[mpeg](&qcoeff[3 * 64], &data[3 * 64], iQuant, scaler_lum, pParam->mpeg_quant_matrices);
	dequant[mpeg](&qcoeff[4 * 64], &data[4 * 64], iQuant, scaler_chr, pParam->mpeg_quant_matrices);
	dequant[mpeg](&qcoeff[5 * 64], &data[5 * 64], iQuant, scaler_chr, pParam->mpeg_quant_matrices);
	stop_iquant_timer();
}

static int
dct_quantize_trellis_c(int16_t *const Out,
					   const int16_t *const In,
					   int Q,
					   const uint16_t * const Zigzag,
					   const uint16_t * const QuantMatrix,
					   int Non_Zero,
					   int Sum,
					   int Lambda_Mod);

/* Quantize all blocks -- Inter mode */
static __inline uint8_t
MBQuantInter(const MBParam * pParam,
			 const FRAMEINFO * const frame,
			 const MACROBLOCK * pMB,
			 int16_t data[6 * 64],
			 int16_t qcoeff[6 * 64],
			 int bvop,
			 int limit)
{

	int i;
	uint8_t cbp = 0;
	int sum;
	int code_block, mpeg;

	quant_interFuncPtr const quant[2] =
		{
			quant_h263_inter,
			quant_mpeg_inter
		};

	mpeg = !!(pParam->vol_flags & XVID_VOL_MPEGQUANT);

	for (i = 0; i < 6; i++) {

		/* Quantize the block */
		start_timer();

		sum = quant[mpeg](&qcoeff[i*64], &data[i*64], pMB->quant, pParam->mpeg_quant_matrices);

		if(sum && (pMB->quant > 2) && (frame->vop_flags & XVID_VOP_TRELLISQUANT)) {
			const uint16_t *matrix;
			const static uint16_t h263matrix[] =
				{
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16,
					16, 16, 16, 16, 16, 16, 16, 16
				};

			matrix = (mpeg)?get_inter_matrix(pParam->mpeg_quant_matrices):h263matrix;
			sum = dct_quantize_trellis_c(&qcoeff[i*64], &data[i*64],
										 pMB->quant, &scan_tables[0][0],
										 matrix,
										 63,
										 sum,
										 pMB->lambda[i]);
		}
		stop_quant_timer();

		/*
		 * We code the block if the sum is higher than the limit and if the first
		 * two AC coefficients in zig zag order are not zero.
		 */
		code_block = 0;
		if ((sum >= limit) || (qcoeff[i*64+1] != 0) || (qcoeff[i*64+8] != 0)) {
			code_block = 1;
		} else {

			if (bvop && (pMB->mode == MODE_DIRECT || pMB->mode == MODE_DIRECT_NO4V)) {
				/* dark blocks prevention for direct mode */
				if ((qcoeff[i*64] < -1) || (qcoeff[i*64] > 0))
					code_block = 1;
			} else {
				/* not direct mode */
				if (qcoeff[i*64] != 0)
					code_block = 1;
			}
		}

		/* Set the corresponding cbp bit */
		cbp |= code_block << (5 - i);
	}

	return(cbp);
}

/* DeQuantize all blocks -- Inter mode */
static __inline void
MBDeQuantInter(const MBParam * pParam,
			   const int iQuant,
			   int16_t data[6 * 64],
			   int16_t qcoeff[6 * 64],
			   const uint8_t cbp)
{
	int mpeg;

	quant_interFuncPtr const dequant[2] =
		{
			dequant_h263_inter,
			dequant_mpeg_inter
		};

	mpeg = !!(pParam->vol_flags & XVID_VOL_MPEGQUANT);

	start_timer();
	if(cbp & (1 << (5 - 0))) dequant[mpeg](&data[0 * 64], &qcoeff[0 * 64], iQuant, pParam->mpeg_quant_matrices);
	if(cbp & (1 << (5 - 1))) dequant[mpeg](&data[1 * 64], &qcoeff[1 * 64], iQuant, pParam->mpeg_quant_matrices);
	if(cbp & (1 << (5 - 2))) dequant[mpeg](&data[2 * 64], &qcoeff[2 * 64], iQuant, pParam->mpeg_quant_matrices);
	if(cbp & (1 << (5 - 3))) dequant[mpeg](&data[3 * 64], &qcoeff[3 * 64], iQuant, pParam->mpeg_quant_matrices);
	if(cbp & (1 << (5 - 4))) dequant[mpeg](&data[4 * 64], &qcoeff[4 * 64], iQuant, pParam->mpeg_quant_matrices);
	if(cbp & (1 << (5 - 5))) dequant[mpeg](&data[5 * 64], &qcoeff[5 * 64], iQuant, pParam->mpeg_quant_matrices);
	stop_iquant_timer();
}

typedef void (transfer_operation_8to16_t) (int16_t *Dst, const uint8_t *Src, int BpS);
typedef void (transfer_operation_16to8_t) (uint8_t *Dst, const int16_t *Src, int BpS);


static __inline void
MBTrans8to16(const MBParam * const pParam,
			 const FRAMEINFO * const frame,
			 const MACROBLOCK * const pMB,
			 const uint32_t x_pos,
			 const uint32_t y_pos,
			 int16_t data[6 * 64])
{
	uint32_t stride = pParam->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	const IMAGE * const pCurrent = &frame->image;

	/* Image pointers */
	pY_Cur = pCurrent->y + (y_pos << 4) * stride  + (x_pos << 4);
	pU_Cur = pCurrent->u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = pCurrent->v + (y_pos << 3) * stride2 + (x_pos << 3);

	/* Do the transfer */
	start_timer();
	transfer_8to16copy(&data[0 * 64], pY_Cur, stride);
	transfer_8to16copy(&data[1 * 64], pY_Cur + 8, stride);
	transfer_8to16copy(&data[2 * 64], pY_Cur + next_block, stride);
	transfer_8to16copy(&data[3 * 64], pY_Cur + next_block + 8, stride);
	transfer_8to16copy(&data[4 * 64], pU_Cur, stride2);
	transfer_8to16copy(&data[5 * 64], pV_Cur, stride2);
	stop_transfer_timer();
}

static __inline void
MBTrans16to8(const MBParam * const pParam,
			 const FRAMEINFO * const frame,
			 const MACROBLOCK * const pMB,
			 const uint32_t x_pos,
			 const uint32_t y_pos,
			 int16_t data[6 * 64],
			 const uint32_t add, /* Must be 1 or 0 */
			 const uint8_t cbp)
{
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	uint32_t stride = pParam->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	const IMAGE * const pCurrent = &frame->image;

	/* Array of function pointers, indexed by [add] */
	transfer_operation_16to8_t  * const functions[2] =
		{
			(transfer_operation_16to8_t*)transfer_16to8copy,
			(transfer_operation_16to8_t*)transfer_16to8add,
		};

	transfer_operation_16to8_t *transfer_op = NULL;

	/* Image pointers */
	pY_Cur = pCurrent->y + (y_pos << 4) * stride  + (x_pos << 4);
	pU_Cur = pCurrent->u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = pCurrent->v + (y_pos << 3) * stride2 + (x_pos << 3);

	if (pMB->field_dct) {
		next_block = stride;
		stride *= 2;
	}

	/* Operation function */
	transfer_op = functions[add];

	/* Do the operation */
	start_timer();
	if (cbp&32) transfer_op(pY_Cur,						&data[0 * 64], stride);
	if (cbp&16) transfer_op(pY_Cur + 8,					&data[1 * 64], stride);
	if (cbp& 8) transfer_op(pY_Cur + next_block,		&data[2 * 64], stride);
	if (cbp& 4) transfer_op(pY_Cur + next_block + 8,	&data[3 * 64], stride);
	if (cbp& 2) transfer_op(pU_Cur,						&data[4 * 64], stride2);
	if (cbp& 1) transfer_op(pV_Cur,						&data[5 * 64], stride2);
	stop_transfer_timer();
}

/*****************************************************************************
 * Module functions
 ****************************************************************************/

void
MBTransQuantIntra(const MBParam * const pParam,
				  const FRAMEINFO * const frame,
				  MACROBLOCK * const pMB,
				  const uint32_t x_pos,
				  const uint32_t y_pos,
				  int16_t data[6 * 64],
				  int16_t qcoeff[6 * 64])
{

	/* Transfer data */
	MBTrans8to16(pParam, frame, pMB, x_pos, y_pos, data);

	/* Perform DCT (and field decision) */
	MBfDCT(pParam, frame, pMB, x_pos, y_pos, data);

	/* Quantize the block */
	MBQuantIntra(pParam, frame, pMB, data, qcoeff);

	/* DeQuantize the block */
	MBDeQuantIntra(pParam, pMB->quant, data, qcoeff);

	/* Perform inverse DCT*/
	MBiDCT(data, 0x3F);

 	/* Transfer back the data -- Don't add data */
	MBTrans16to8(pParam, frame, pMB, x_pos, y_pos, data, 0, 0x3F);
}


uint8_t
MBTransQuantInter(const MBParam * const pParam,
				  const FRAMEINFO * const frame,
				  MACROBLOCK * const pMB,
				  const uint32_t x_pos,
				  const uint32_t y_pos,
				  int16_t data[6 * 64],
				  int16_t qcoeff[6 * 64])
{
	uint8_t cbp;
	uint32_t limit;

	/* There is no MBTrans8to16 for Inter block, that's done in motion compensation
	 * already */

	/* Perform DCT (and field decision) */
	MBfDCT(pParam, frame, pMB, x_pos, y_pos, data);

	/* Set the limit threshold */
	limit = PVOP_TOOSMALL_LIMIT + ((pMB->quant == 1)? 1 : 0);

	if (frame->vop_flags & XVID_VOP_CARTOON)
		limit *= 3;

	/* Quantize the block */
	cbp = MBQuantInter(pParam, frame, pMB, data, qcoeff, 0, limit);

	/* DeQuantize the block */
	MBDeQuantInter(pParam, pMB->quant, data, qcoeff, cbp);

	/* Perform inverse DCT*/
	MBiDCT(data, cbp);

 	/* Transfer back the data -- Add the data */
	MBTrans16to8(pParam, frame, pMB, x_pos, y_pos, data, 1, cbp);

	return(cbp);
}

uint8_t
MBTransQuantInterBVOP(const MBParam * pParam,
					  FRAMEINFO * frame,
					  MACROBLOCK * pMB,
					  const uint32_t x_pos,
					  const uint32_t y_pos,
					  int16_t data[6 * 64],
					  int16_t qcoeff[6 * 64])
{
	uint8_t cbp;
	uint32_t limit;

	/* There is no MBTrans8to16 for Inter block, that's done in motion compensation
	 * already */

	/* Perform DCT (and field decision) */
	MBfDCT(pParam, frame, pMB, x_pos, y_pos, data);

	/* Set the limit threshold */
	limit = BVOP_TOOSMALL_LIMIT;

	if (frame->vop_flags & XVID_VOP_CARTOON)
		limit *= 2;

	/* Quantize the block */
	cbp = MBQuantInter(pParam, frame, pMB, data, qcoeff, 1, limit);

	/*
	 * History comment:
	 * We don't have to DeQuant, iDCT and Transfer back data for B-frames.
	 *
	 * BUT some plugins require the rebuilt original frame to be passed so we
	 * have to take care of that here
	 */
	if((pParam->plugin_flags & XVID_REQORIGINAL)) {

		/* DeQuantize the block */
		MBDeQuantInter(pParam, pMB->quant, data, qcoeff, cbp);

		/* Perform inverse DCT*/
		MBiDCT(data, cbp);

		/* Transfer back the data -- Add the data */
		MBTrans16to8(pParam, frame, pMB, x_pos, y_pos, data, 1, cbp);
	}

	return(cbp);
}

/* if sum(diff between field lines) < sum(diff between frame lines), use field dct */
uint32_t
MBFieldTest_c(int16_t data[6 * 64])
{
	const uint8_t blocks[] =
		{ 0 * 64, 0 * 64, 0 * 64, 0 * 64, 2 * 64, 2 * 64, 2 * 64, 2 * 64 };
	const uint8_t lines[] = { 0, 16, 32, 48, 0, 16, 32, 48 };

	int frame = 0, field = 0;
	int i, j;

	for (i = 0; i < 7; ++i) {
		for (j = 0; j < 8; ++j) {
			frame +=
				abs(data[0 * 64 + (i + 1) * 8 + j] - data[0 * 64 + i * 8 + j]);
			frame +=
				abs(data[1 * 64 + (i + 1) * 8 + j] - data[1 * 64 + i * 8 + j]);
			frame +=
				abs(data[2 * 64 + (i + 1) * 8 + j] - data[2 * 64 + i * 8 + j]);
			frame +=
				abs(data[3 * 64 + (i + 1) * 8 + j] - data[3 * 64 + i * 8 + j]);

			field +=
				abs(data[blocks[i + 1] + lines[i + 1] + j] -
					data[blocks[i] + lines[i] + j]);
			field +=
				abs(data[blocks[i + 1] + lines[i + 1] + 8 + j] -
					data[blocks[i] + lines[i] + 8 + j]);
			field +=
				abs(data[blocks[i + 1] + 64 + lines[i + 1] + j] -
					data[blocks[i] + 64 + lines[i] + j]);
			field +=
				abs(data[blocks[i + 1] + 64 + lines[i + 1] + 8 + j] -
					data[blocks[i] + 64 + lines[i] + 8 + j]);
		}
	}

	return (frame >= (field + 350));
}


/* deinterlace Y blocks vertically */

#define MOVLINE(X,Y) memcpy(X, Y, sizeof(tmp))
#define LINE(X,Y)	&data[X*64 + Y*8]

void
MBFrameToField(int16_t data[6 * 64])
{
	int16_t tmp[8];

	/* left blocks */

	/* 1=2, 2=4, 4=8, 8=1 */
	MOVLINE(tmp, LINE(0, 1));
	MOVLINE(LINE(0, 1), LINE(0, 2));
	MOVLINE(LINE(0, 2), LINE(0, 4));
	MOVLINE(LINE(0, 4), LINE(2, 0));
	MOVLINE(LINE(2, 0), tmp);

	/* 3=6, 6=12, 12=9, 9=3 */
	MOVLINE(tmp, LINE(0, 3));
	MOVLINE(LINE(0, 3), LINE(0, 6));
	MOVLINE(LINE(0, 6), LINE(2, 4));
	MOVLINE(LINE(2, 4), LINE(2, 1));
	MOVLINE(LINE(2, 1), tmp);

	/* 5=10, 10=5 */
	MOVLINE(tmp, LINE(0, 5));
	MOVLINE(LINE(0, 5), LINE(2, 2));
	MOVLINE(LINE(2, 2), tmp);

	/* 7=14, 14=13, 13=11, 11=7 */
	MOVLINE(tmp, LINE(0, 7));
	MOVLINE(LINE(0, 7), LINE(2, 6));
	MOVLINE(LINE(2, 6), LINE(2, 5));
	MOVLINE(LINE(2, 5), LINE(2, 3));
	MOVLINE(LINE(2, 3), tmp);

	/* right blocks */

	/* 1=2, 2=4, 4=8, 8=1 */
	MOVLINE(tmp, LINE(1, 1));
	MOVLINE(LINE(1, 1), LINE(1, 2));
	MOVLINE(LINE(1, 2), LINE(1, 4));
	MOVLINE(LINE(1, 4), LINE(3, 0));
	MOVLINE(LINE(3, 0), tmp);

	/* 3=6, 6=12, 12=9, 9=3 */
	MOVLINE(tmp, LINE(1, 3));
	MOVLINE(LINE(1, 3), LINE(1, 6));
	MOVLINE(LINE(1, 6), LINE(3, 4));
	MOVLINE(LINE(3, 4), LINE(3, 1));
	MOVLINE(LINE(3, 1), tmp);

	/* 5=10, 10=5 */
	MOVLINE(tmp, LINE(1, 5));
	MOVLINE(LINE(1, 5), LINE(3, 2));
	MOVLINE(LINE(3, 2), tmp);

	/* 7=14, 14=13, 13=11, 11=7 */
	MOVLINE(tmp, LINE(1, 7));
	MOVLINE(LINE(1, 7), LINE(3, 6));
	MOVLINE(LINE(3, 6), LINE(3, 5));
	MOVLINE(LINE(3, 5), LINE(3, 3));
	MOVLINE(LINE(3, 3), tmp);
}

/*****************************************************************************
 *               Trellis based R-D optimal quantization
 *
 *   Trellis Quant code (C) 2003 Pascal Massimino skal(at)planet-d.net
 *
 ****************************************************************************/

/*----------------------------------------------------------------------------
 *
 *        Trellis-Based quantization
 *
 * So far I understand this paper:
 *
 *  "Trellis-Based R-D Optimal Quantization in H.263+"
 *    J.Wen, M.Luttrell, J.Villasenor
 *    IEEE Transactions on Image Processing, Vol.9, No.8, Aug. 2000.
 *
 * we are at stake with a simplified Bellmand-Ford / Dijkstra Single
 * Source Shortest Path algo. But due to the underlying graph structure
 * ("Trellis"), it can be turned into a dynamic programming algo,
 * partially saving the explicit graph's nodes representation. And
 * without using a heap, since the open frontier of the DAG is always
 * known, and of fixed size.
 *--------------------------------------------------------------------------*/



/* Codes lengths for relevant levels. */

/* let's factorize: */
static const uint8_t Code_Len0[64] = {
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len1[64] = {
	20,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len2[64] = {
	19,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len3[64] = {
	18,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len4[64] = {
	17,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len5[64] = {
	16,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len6[64] = {
	15,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len7[64] = {
	13,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len8[64] = {
	11,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len9[64] = {
	12,21,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len10[64] = {
	12,20,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len11[64] = {
	12,19,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len12[64] = {
	11,17,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len13[64] = {
	11,15,21,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len14[64] = {
	10,12,19,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len15[64] = {
	10,13,17,19,21,21,21,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len16[64] = {
	9,12,13,18,18,19,19,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30};
static const uint8_t Code_Len17[64] = {
	8,11,13,14,14,14,15,19,19,19,21,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len18[64] = {
	7, 9,11,11,13,13,13,15,15,15,16,22,22,22,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len19[64] = {
	5, 7, 9,10,10,11,11,11,11,11,13,14,16,17,17,18,18,18,18,18,18,18,18,20,20,21,21,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 };
static const uint8_t Code_Len20[64] = {
	3, 4, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9,10,10,10,10,10,10,10,10,12,12,13,13,12,13,14,15,15,
	15,16,16,16,16,17,17,17,18,18,19,19,19,19,19,19,19,19,21,21,22,22,30,30,30,30,30,30,30,30,30,30 };

/* a few more table for LAST table: */
static const uint8_t Code_Len21[64] = {
	13,20,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30};
static const uint8_t Code_Len22[64] = {
	12,15,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
	30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30};
static const uint8_t Code_Len23[64] = {
	10,12,15,15,15,16,16,16,16,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,18,19,19,19,19,20,20,20,
	20,21,21,21,21,21,21,21,21,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30};
static const uint8_t Code_Len24[64] = {
	5, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,11,11,11,11,12,12,12,
	12,13,13,13,13,13,13,13,13,14,16,16,16,16,17,17,17,17,18,18,18,18,18,18,18,18,19,19,19,19,19,19};


static const uint8_t * const B16_17_Code_Len[24] = { /* levels [1..24] */
	Code_Len20,Code_Len19,Code_Len18,Code_Len17,
	Code_Len16,Code_Len15,Code_Len14,Code_Len13,
	Code_Len12,Code_Len11,Code_Len10,Code_Len9,
	Code_Len8, Code_Len7 ,Code_Len6 ,Code_Len5,
	Code_Len4, Code_Len3, Code_Len3 ,Code_Len2,
	Code_Len2, Code_Len1, Code_Len1, Code_Len1,
};

static const uint8_t * const B16_17_Code_Len_Last[6] = { /* levels [1..6] */
	Code_Len24,Code_Len23,Code_Len22,Code_Len21, Code_Len3, Code_Len1,
};

/* TL_SHIFT controls the precision of the RD optimizations in trellis
 * valid range is [10..16]. The bigger, the more trellis is vulnerable
 * to overflows in cost formulas.
 *  - 10 allows ac values up to 2^11 == 2048
 *  - 16 allows ac values up to 2^8 == 256
 */
#define TL_SHIFT 11
#define TL(q) ((0xfe00>>(16-TL_SHIFT))/(q*q))

static const int Trellis_Lambda_Tabs[31] = {
	TL( 1),TL( 2),TL( 3),TL( 4),TL( 5),TL( 6), TL( 7),
	TL( 8),TL( 9),TL(10),TL(11),TL(12),TL(13),TL(14), TL(15),
	TL(16),TL(17),TL(18),TL(19),TL(20),TL(21),TL(22), TL(23),
	TL(24),TL(25),TL(26),TL(27),TL(28),TL(29),TL(30), TL(31)
};
#undef TL

static int __inline
Find_Last(const int16_t *C, const uint16_t *Zigzag, int i)
{
	while(i>=0)
		if (C[Zigzag[i]])
			return i;
		else i--;
	return -1;
}

#define TRELLIS_MIN_EFFORT	3

/* this routine has been strippen of all debug code */
static int
dct_quantize_trellis_c(int16_t *const Out,
					   const int16_t *const In,
					   int Q,
					   const uint16_t * const Zigzag,
					   const uint16_t * const QuantMatrix,
					   int Non_Zero,
					   int Sum,
					   int Lambda_Mod)
{

	/* Note: We should search last non-zero coeffs on *real* DCT input coeffs
	 * (In[]), not quantized one (Out[]). However, it only improves the result
	 * *very* slightly (~0.01dB), whereas speed drops to crawling level :)
	 * Well, actually, taking 1 more coeff past Non_Zero into account sometimes
	 * helps. */
	typedef struct { int16_t Run, Level; } NODE;

	NODE Nodes[65], Last = { 0, 0};
	uint32_t Run_Costs0[64+1];
	uint32_t * const Run_Costs = Run_Costs0 + 1;

	/* it's 1/lambda, actually */
	const int Lambda = (Lambda_Mod*Trellis_Lambda_Tabs[Q-1])>>LAMBDA_EXP;

	int Run_Start = -1;
	uint32_t Min_Cost = 2<<TL_SHIFT;

	int Last_Node = -1;
	uint32_t Last_Cost = 0;

	int i, j;

	/* source (w/ CBP penalty) */
	Run_Costs[-1] = 2<<TL_SHIFT;

	Non_Zero = Find_Last(Out, Zigzag, Non_Zero);
	if (Non_Zero < TRELLIS_MIN_EFFORT) 
		Non_Zero = TRELLIS_MIN_EFFORT;

	for(i=0; i<=Non_Zero; i++) {
		const int q = ((Q*QuantMatrix[Zigzag[i]])>>4);
		const int Mult = 2*q;
		const int Bias = (q-1) | 1;
		const int Lev0 = Mult + Bias;

		const int AC = In[Zigzag[i]];
		const int Level1 = Out[Zigzag[i]];
		const unsigned int Dist0 = Lambda* AC*AC;
		uint32_t Best_Cost = 0xf0000000;
		Last_Cost += Dist0;

		/* very specialized loop for -1,0,+1 */
		if ((uint32_t)(Level1+1)<3) {
			int dQ;
			int Run;
			uint32_t Cost0;

			if (AC<0) {
				Nodes[i].Level = -1;
				dQ = Lev0 + AC;
			} else {
				Nodes[i].Level = 1;
				dQ = Lev0 - AC;
			}
			Cost0 = Lambda*dQ*dQ;

			Nodes[i].Run = 1;
			Best_Cost = (Code_Len20[0]<<TL_SHIFT) + Run_Costs[i-1]+Cost0;
			for(Run=i-Run_Start; Run>0; --Run) {
				const uint32_t Cost_Base = Cost0 + Run_Costs[i-Run];
				const uint32_t Cost = Cost_Base + (Code_Len20[Run-1]<<TL_SHIFT);
				const uint32_t lCost = Cost_Base + (Code_Len24[Run-1]<<TL_SHIFT);

				/* TODO: what about tie-breaks? Should we favor short runs or
				 * long runs? Although the error is the same, it would not be
				 * spread the same way along high and low frequencies... */

				/* Gruel: I'd say, favour short runs => hifreq errors (HVS) */

				if (Cost<Best_Cost) {
					Best_Cost	 = Cost;
					Nodes[i].Run = Run;
				}

				if (lCost<Last_Cost) {
					Last_Cost  = lCost;
					Last.Run   = Run;
					Last_Node  = i;
				}
			}
			if (Last_Node==i)
				Last.Level = Nodes[i].Level;
		} else if (51U>(uint32_t)(Level1+25)) {
			/* "big" levels (not less than ESC3, though) */
			const uint8_t *Tbl_L1, *Tbl_L2, *Tbl_L1_Last, *Tbl_L2_Last;
			int Level2;
			int dQ1, dQ2;
			int Run;
			uint32_t Dist1,Dist2;
			int dDist21;

			if (Level1>1) {
				dQ1 = Level1*Mult-AC + Bias;
				dQ2 = dQ1 - Mult;
				Level2 = Level1-1;
				Tbl_L1		= (Level1<=24) ? B16_17_Code_Len[Level1-1]	   : Code_Len0;
				Tbl_L2		= (Level2<=24) ? B16_17_Code_Len[Level2-1]	   : Code_Len0;
				Tbl_L1_Last = (Level1<=6) ? B16_17_Code_Len_Last[Level1-1] : Code_Len0;
				Tbl_L2_Last = (Level2<=6) ? B16_17_Code_Len_Last[Level2-1] : Code_Len0;
			} else { /* Level1<-1 */
				dQ1 = Level1*Mult-AC - Bias;
				dQ2 = dQ1 + Mult;
				Level2 = Level1 + 1;
				Tbl_L1		= (Level1>=-24) ? B16_17_Code_Len[Level1^-1]	  : Code_Len0;
				Tbl_L2		= (Level2>=-24) ? B16_17_Code_Len[Level2^-1]	  : Code_Len0;
				Tbl_L1_Last = (Level1>=- 6) ? B16_17_Code_Len_Last[Level1^-1] : Code_Len0;
				Tbl_L2_Last = (Level2>=- 6) ? B16_17_Code_Len_Last[Level2^-1] : Code_Len0;
			}

			Dist1 = Lambda*dQ1*dQ1;
			Dist2 = Lambda*dQ2*dQ2;
			dDist21 = Dist2-Dist1;

			for(Run=i-Run_Start; Run>0; --Run)
			{
				const uint32_t Cost_Base = Dist1 + Run_Costs[i-Run];
				uint32_t Cost1, Cost2;
				int bLevel;

				/* for sub-optimal (but slightly worth it, speed-wise) search,
				 * uncomment the following:
				 *		if (Cost_Base>=Best_Cost) continue;
				 * (? doesn't seem to have any effect -- gruel ) */

				Cost1 = Cost_Base + (Tbl_L1[Run-1]<<TL_SHIFT);
				Cost2 = Cost_Base + (Tbl_L2[Run-1]<<TL_SHIFT) + dDist21;

				if (Cost2<Cost1) {
					Cost1 = Cost2;
					bLevel = Level2;
				} else {
					bLevel = Level1;
				}

				if (Cost1<Best_Cost) {
					Best_Cost = Cost1;
					Nodes[i].Run   = Run;
					Nodes[i].Level = bLevel;
				}

				Cost1 = Cost_Base + (Tbl_L1_Last[Run-1]<<TL_SHIFT);
				Cost2 = Cost_Base + (Tbl_L2_Last[Run-1]<<TL_SHIFT) + dDist21;

				if (Cost2<Cost1) {
					Cost1 = Cost2;
					bLevel = Level2;
				} else {
					bLevel = Level1;
				}

				if (Cost1<Last_Cost) {
					Last_Cost  = Cost1;
					Last.Run   = Run;
					Last.Level = bLevel;
					Last_Node  = i;
				}
			} /* end of "for Run" */
		} else {
			/* Very very high levels, with no chance of being optimizable
			 * => Simply pick best Run. */
			int Run;
			for(Run=i-Run_Start; Run>0; --Run) {  
				/* 30 bits + no distortion */
				const uint32_t Cost = (30<<TL_SHIFT) + Run_Costs[i-Run];
				if (Cost<Best_Cost) {
					Best_Cost = Cost;
					Nodes[i].Run   = Run;
					Nodes[i].Level = Level1;
				}

				if (Cost<Last_Cost) {
					Last_Cost  = Cost;
					Last.Run   = Run;
					Last.Level = Level1;
					Last_Node  = i;	 
				}
			}
		}


		Run_Costs[i] = Best_Cost;

		if (Best_Cost < Min_Cost + Dist0) {
			Min_Cost = Best_Cost;
			Run_Start = i;
		} else {
			/* as noticed by Michael Niedermayer (michaelni at gmx.at),
			 * there's a code shorter by 1 bit for a larger run (!), same
			 * level. We give it a chance by not moving the left barrier too
			 * much. */
			while( Run_Costs[Run_Start]>Min_Cost+(1<<TL_SHIFT) )
				Run_Start++;

			/* spread on preceding coeffs the cost incurred by skipping this
			 * one */
			for(j=Run_Start; j<i; ++j) Run_Costs[j] += Dist0;
			Min_Cost += Dist0;
		}
	}

	/* It seems trellis doesn't give good results... just leave the block untouched
	 * and return the original sum value */
	if (Last_Node<0)
		return Sum;

	/* reconstruct optimal sequence backward with surviving paths */
	memset(Out, 0x00, 64*sizeof(*Out));
	Out[Zigzag[Last_Node]] = Last.Level;
	i = Last_Node - Last.Run;
	Sum = abs(Last.Level);
	while(i>=0) {
		Out[Zigzag[i]] = Nodes[i].Level;
		Sum += abs(Nodes[i].Level);
		i -= Nodes[i].Run;
	}

	return Sum;
}

/* original version including heavy debugging info */

#ifdef DBGTRELL

#define DBG 0

static __inline uint32_t Evaluate_Cost(const int16_t *C, int Mult, int Bias,
									   const uint16_t * Zigzag, int Max, int Lambda)
{
#if (DBG>0)
	const int16_t * const Ref = C + 6*64;
	int Last = Max;
	int Bits = 0;
	int Dist = 0;
	int i;
	uint32_t Cost;

	while(Last>=0 && C[Zigzag[Last]]==0)
		Last--;

	if (Last>=0) {
		int j=0, j0=0;
		int Run, Level;

		Bits = 2;   /* CBP */
		while(j<Last) {
			while(!C[Zigzag[j]])
				j++;
			if (j==Last)
				break;
			Level=C[Zigzag[j]];
			Run = j - j0;
			j0 = ++j;
			if (Level>=-24 && Level<=24)
				Bits += B16_17_Code_Len[(Level<0) ? -Level-1 : Level-1][Run];
			else
				Bits += 30;
		}
		Level = C[Zigzag[Last]];
		Run = j - j0;
		if (Level>=-6 && Level<=6)
			Bits += B16_17_Code_Len_Last[(Level<0) ? -Level-1 : Level-1][Run];
		else
			Bits += 30;
	}

	for(i=0; i<=Last; ++i) {
		int V = C[Zigzag[i]]*Mult;
		if (V>0)
			V += Bias;
		else
			if (V<0)
				V -= Bias;
		V -= Ref[Zigzag[i]];
		Dist += V*V;
	}
	Cost = Lambda*Dist + (Bits<<TL_SHIFT);
	if (DBG==1)
		printf( " Last:%2d/%2d Cost = [(Bits=%5.0d) + Lambda*(Dist=%6.0d) = %d ] >>12= %d ", Last,Max, Bits, Dist, Cost, Cost>>12 );
	return Cost;

#else
	return 0;
#endif
}


static int
dct_quantize_trellis_h263_c(int16_t *const Out, const int16_t *const In, int Q, const uint16_t * const Zigzag, int Non_Zero)
{

    /*
	 * Note: We should search last non-zero coeffs on *real* DCT input coeffs (In[]),
	 * not quantized one (Out[]). However, it only improves the result *very*
	 * slightly (~0.01dB), whereas speed drops to crawling level :)
	 * Well, actually, taking 1 more coeff past Non_Zero into account sometimes helps.
	 */
	typedef struct { int16_t Run, Level; } NODE;

	NODE Nodes[65], Last;
	uint32_t Run_Costs0[64+1];
	uint32_t * const Run_Costs = Run_Costs0 + 1;
	const int Mult = 2*Q;
	const int Bias = (Q-1) | 1;
	const int Lev0 = Mult + Bias;
	const int Lambda = Trellis_Lambda_Tabs[Q-1];    /* it's 1/lambda, actually */

	int Run_Start = -1;
	Run_Costs[-1] = 2<<TL_SHIFT;                          /* source (w/ CBP penalty) */
	uint32_t Min_Cost = 2<<TL_SHIFT;

	int Last_Node = -1;
	uint32_t Last_Cost = 0;

	int i, j;

#if (DBG>0)
	Last.Level = 0; Last.Run = -1; /* just initialize to smthg */
#endif

	Non_Zero = Find_Last(Out, Zigzag, Non_Zero);
	if (Non_Zero<0)
		return -1;

	for(i=0; i<=Non_Zero; i++)
	{
		const int AC = In[Zigzag[i]];
		const int Level1 = Out[Zigzag[i]];
		const int Dist0 = Lambda* AC*AC;
		uint32_t Best_Cost = 0xf0000000;
		Last_Cost += Dist0;

		if ((uint32_t)(Level1+1)<3)                 /* very specialized loop for -1,0,+1 */
		{
			int dQ;
			int Run;
			uint32_t Cost0;

			if (AC<0) {
				Nodes[i].Level = -1;
				dQ = Lev0 + AC;
			} else {
				Nodes[i].Level = 1;
				dQ = Lev0 - AC;
			}
			Cost0 = Lambda*dQ*dQ;

			Nodes[i].Run = 1;
			Best_Cost = (Code_Len20[0]<<TL_SHIFT) + Run_Costs[i-1]+Cost0;
			for(Run=i-Run_Start; Run>0; --Run)
			{
				const uint32_t Cost_Base = Cost0 + Run_Costs[i-Run];
				const uint32_t Cost = Cost_Base + (Code_Len20[Run-1]<<TL_SHIFT);
				const uint32_t lCost = Cost_Base + (Code_Len24[Run-1]<<TL_SHIFT);

				/*
				 * TODO: what about tie-breaks? Should we favor short runs or
				 * long runs? Although the error is the same, it would not be
				 * spread the same way along high and low frequencies...
				 */
				if (Cost<Best_Cost) {
					Best_Cost    = Cost;
					Nodes[i].Run = Run;
				}

				if (lCost<Last_Cost) {
					Last_Cost  = lCost;
					Last.Run   = Run;
					Last_Node  = i;
				}
			}
			if (Last_Node==i)
				Last.Level = Nodes[i].Level;

			if (DBG==1) {
				Run_Costs[i] = Best_Cost;
				printf( "Costs #%2d: ", i);
				for(j=-1;j<=Non_Zero;++j) {
					if (j==Run_Start)            printf( " %3.0d|", Run_Costs[j]>>12 );
					else if (j>Run_Start && j<i) printf( " %3.0d|", Run_Costs[j]>>12 );
					else if (j==i)               printf( "(%3.0d)", Run_Costs[j]>>12 );
					else                         printf( "  - |" );
				}
				printf( "<%3.0d %2d %d>", Min_Cost>>12, Nodes[i].Level, Nodes[i].Run );
				printf( "  Last:#%2d {%3.0d %2d %d}", Last_Node, Last_Cost>>12, Last.Level, Last.Run );
				printf( " AC:%3.0d Dist0:%3d Dist(%d)=%d", AC, Dist0>>12, Nodes[i].Level, Cost0>>12 );
				printf( "\n" );
			}
		}
		else                      /* "big" levels */
		{
			const uint8_t *Tbl_L1, *Tbl_L2, *Tbl_L1_Last, *Tbl_L2_Last;
			int Level2;
			int dQ1, dQ2;
			int Run;
			uint32_t Dist1,Dist2;
			int dDist21;

			if (Level1>1) {
				dQ1 = Level1*Mult-AC + Bias;
				dQ2 = dQ1 - Mult;
				Level2 = Level1-1;
				Tbl_L1      = (Level1<=24) ? B16_17_Code_Len[Level1-1]     : Code_Len0;
				Tbl_L2      = (Level2<=24) ? B16_17_Code_Len[Level2-1]     : Code_Len0;
				Tbl_L1_Last = (Level1<=6) ? B16_17_Code_Len_Last[Level1-1] : Code_Len0;
				Tbl_L2_Last = (Level2<=6) ? B16_17_Code_Len_Last[Level2-1] : Code_Len0;
			} else { /* Level1<-1 */
				dQ1 = Level1*Mult-AC - Bias;
				dQ2 = dQ1 + Mult;
				Level2 = Level1 + 1;
				Tbl_L1      = (Level1>=-24) ? B16_17_Code_Len[Level1^-1]      : Code_Len0;
				Tbl_L2      = (Level2>=-24) ? B16_17_Code_Len[Level2^-1]      : Code_Len0;
				Tbl_L1_Last = (Level1>=- 6) ? B16_17_Code_Len_Last[Level1^-1] : Code_Len0;
				Tbl_L2_Last = (Level2>=- 6) ? B16_17_Code_Len_Last[Level2^-1] : Code_Len0;
			}
			Dist1 = Lambda*dQ1*dQ1;
			Dist2 = Lambda*dQ2*dQ2;
			dDist21 = Dist2-Dist1;

			for(Run=i-Run_Start; Run>0; --Run)
			{
				const uint32_t Cost_Base = Dist1 + Run_Costs[i-Run];
				uint32_t Cost1, Cost2;
				int bLevel;

/*
 * for sub-optimal (but slightly worth it, speed-wise) search, uncomment the following:
 *        if (Cost_Base>=Best_Cost) continue;
 */
				Cost1 = Cost_Base + (Tbl_L1[Run-1]<<TL_SHIFT);
				Cost2 = Cost_Base + (Tbl_L2[Run-1]<<TL_SHIFT) + dDist21;

				if (Cost2<Cost1) {
					Cost1 = Cost2;
					bLevel = Level2;
				} else
					bLevel = Level1;

				if (Cost1<Best_Cost) {
					Best_Cost = Cost1;
					Nodes[i].Run   = Run;
					Nodes[i].Level = bLevel;
				}

				Cost1 = Cost_Base + (Tbl_L1_Last[Run-1]<<TL_SHIFT);
				Cost2 = Cost_Base + (Tbl_L2_Last[Run-1]<<TL_SHIFT) + dDist21;

				if (Cost2<Cost1) {
					Cost1 = Cost2;
					bLevel = Level2;
				} else
					bLevel = Level1;

				if (Cost1<Last_Cost) {
					Last_Cost  = Cost1;
					Last.Run   = Run;
					Last.Level = bLevel;
					Last_Node  = i;
				}
			} /* end of "for Run" */

			if (DBG==1) {
				Run_Costs[i] = Best_Cost;
				printf( "Costs #%2d: ", i);
				for(j=-1;j<=Non_Zero;++j) {
					if (j==Run_Start)            printf( " %3.0d|", Run_Costs[j]>>12 );
					else if (j>Run_Start && j<i) printf( " %3.0d|", Run_Costs[j]>>12 );
					else if (j==i)               printf( "(%3.0d)", Run_Costs[j]>>12 );
					else                         printf( "  - |" );
				}
				printf( "<%3.0d %2d %d>", Min_Cost>>12, Nodes[i].Level, Nodes[i].Run );
				printf( "  Last:#%2d {%3.0d %2d %d}", Last_Node, Last_Cost>>12, Last.Level, Last.Run );
				printf( " AC:%3.0d Dist0:%3d Dist(%2d):%3d Dist(%2d):%3d", AC, Dist0>>12, Level1, Dist1>>12, Level2, Dist2>>12 );
				printf( "\n" );
			}
		}

		Run_Costs[i] = Best_Cost;

		if (Best_Cost < Min_Cost + Dist0) {
			Min_Cost = Best_Cost;
			Run_Start = i;
		}
		else
		{
			/*
			 * as noticed by Michael Niedermayer (michaelni at gmx.at), there's
			 * a code shorter by 1 bit for a larger run (!), same level. We give
			 * it a chance by not moving the left barrier too much.
			 */

			while( Run_Costs[Run_Start]>Min_Cost+(1<<TL_SHIFT) )
				Run_Start++;

			/* spread on preceding coeffs the cost incurred by skipping this one */
			for(j=Run_Start; j<i; ++j) Run_Costs[j] += Dist0;
			Min_Cost += Dist0;
		}
	}

	if (DBG) {
		Last_Cost = Evaluate_Cost(Out,Mult,Bias, Zigzag,Non_Zero, Lambda);
		if (DBG==1) {
			printf( "=> " );
			for(i=0; i<=Non_Zero; ++i) printf( "[%3.0d] ", Out[Zigzag[i]] );
			printf( "\n" );
		}
	}

	if (Last_Node<0)
		return -1;

	/* reconstruct optimal sequence backward with surviving paths */
	memset(Out, 0x00, 64*sizeof(*Out));
	Out[Zigzag[Last_Node]] = Last.Level;
	i = Last_Node - Last.Run;
	while(i>=0) {
		Out[Zigzag[i]] = Nodes[i].Level;
		i -= Nodes[i].Run;
	}

	if (DBG) {
		uint32_t Cost = Evaluate_Cost(Out,Mult,Bias, Zigzag,Non_Zero, Lambda);
		if (DBG==1) {
			printf( "<= " );
			for(i=0; i<=Last_Node; ++i) printf( "[%3.0d] ", Out[Zigzag[i]] );
			printf( "\n--------------------------------\n" );
		}
		if (Cost>Last_Cost) printf( "!!! %u > %u\n", Cost, Last_Cost );
	}
	return Last_Node;
}

#undef DBG

#endif
