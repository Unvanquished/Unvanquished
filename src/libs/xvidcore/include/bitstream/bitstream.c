/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Bitstream reader/writer -
 *
 *  Copyright (C) 2001-2003 Peter Ross <pross@xvid.org>
 *                     2003 Cristoph Lampert <gruel@web.de>
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
 * $Id: bitstream.c,v 1.58 2007/07/24 09:43:10 Isibaar Exp $
 *
 ****************************************************************************/

#include <string.h>
#include <stdio.h>

#include "bitstream.h"
#include "zigzag.h"
#include "../quant/quant_matrix.h"
#include "mbcoding.h"


static const uint8_t log2_tab_16[16] =  { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };
 
static uint32_t __inline log2bin(uint32_t value)
{
  int n = 0;
  if (value & 0xffff0000) {
    value >>= 16;
    n += 16;
  }
  if (value & 0xff00) {
    value >>= 8;
    n += 8;
  }
  if (value & 0xf0) {
    value >>= 4;
    n += 4;
  }
 return n + log2_tab_16[value];
}

static const uint32_t intra_dc_threshold_table[] = {
	32,							/* never use */
	13,
	15,
	17,
	19,
	21,
	23,
	1,
};


static void
bs_get_matrix(Bitstream * bs,
			  uint8_t * matrix)
{
	int i = 0;
	int last, value = 0;

	do {
		last = value;
		value = BitstreamGetBits(bs, 8);
		matrix[scan_tables[0][i++]] = value;
	}
	while (value != 0 && i < 64);

	if (value != 0) return;

	i--;
	while (i < 64) {
		matrix[scan_tables[0][i++]] = last;
	}
}



/*
 * for PVOP addbits == fcode - 1
 * for BVOP addbits == max(fcode,bcode) - 1
 * returns mbpos
 */
int
read_video_packet_header(Bitstream *bs,
						DECODER * dec,
						const int addbits,
						int * quant,
						int * fcode_forward,
						int  * fcode_backward,
						int * intra_dc_threshold)
{
	int startcode_bits = NUMBITS_VP_RESYNC_MARKER + addbits;
	int mbnum_bits = log2bin(dec->mb_width *  dec->mb_height - 1);
	int mbnum;
	int hec = 0;

	BitstreamSkip(bs, BitstreamNumBitsToByteAlign(bs));
	BitstreamSkip(bs, startcode_bits);

	DPRINTF(XVID_DEBUG_STARTCODE, "<video_packet_header>\n");

	if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
	{
		hec = BitstreamGetBit(bs);		/* header_extension_code */
		if (hec && !(dec->sprite_enable == SPRITE_STATIC /* && current_coding_type = I_VOP */))
		{
			BitstreamSkip(bs, 13);			/* vop_width */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_height */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_horizontal_mc_spatial_ref */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_vertical_mc_spatial_ref */
			READ_MARKER();
		}
	}

	mbnum = BitstreamGetBits(bs, mbnum_bits);		/* macroblock_number */
	DPRINTF(XVID_DEBUG_HEADER, "mbnum %i\n", mbnum);

	if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
	{
		*quant = BitstreamGetBits(bs, dec->quant_bits);	/* quant_scale */
		DPRINTF(XVID_DEBUG_HEADER, "quant %i\n", *quant);
	}

	if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR)
		hec = BitstreamGetBit(bs);		/* header_extension_code */


	DPRINTF(XVID_DEBUG_HEADER, "header_extension_code %i\n", hec);
	if (hec)
	{
		int time_base;
		int time_increment;
		int coding_type;

		for (time_base=0; BitstreamGetBit(bs)!=0; time_base++);		/* modulo_time_base */
		READ_MARKER();
		if (dec->time_inc_bits)
			time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	/* vop_time_increment */
		READ_MARKER();
		DPRINTF(XVID_DEBUG_HEADER,"time %i:%i\n", time_base, time_increment);

		coding_type = BitstreamGetBits(bs, 2);
		DPRINTF(XVID_DEBUG_HEADER,"coding_type %i\n", coding_type);

		if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
		{
			BitstreamSkip(bs, 1);	/* change_conv_ratio_disable */
			if (coding_type != I_VOP)
				BitstreamSkip(bs, 1);	/* vop_shape_coding_type */
		}

		if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
		{
			*intra_dc_threshold = intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

			if (dec->sprite_enable == SPRITE_GMC && coding_type == S_VOP &&
				dec->sprite_warping_points > 0)
			{
				/* TODO: sprite trajectory */
			}
			if (dec->reduced_resolution_enable &&
				dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR &&
				(coding_type == P_VOP || coding_type == I_VOP))
			{
				BitstreamSkip(bs, 1); /* XXX: vop_reduced_resolution */
			}

			if (coding_type != I_VOP && fcode_forward)
			{
				*fcode_forward = BitstreamGetBits(bs, 3);
				DPRINTF(XVID_DEBUG_HEADER,"fcode_forward %i\n", *fcode_forward);
			}

			if (coding_type == B_VOP && fcode_backward)
			{
				*fcode_backward = BitstreamGetBits(bs, 3);
				DPRINTF(XVID_DEBUG_HEADER,"fcode_backward %i\n", *fcode_backward);
			}
		}
	}

	if (dec->newpred_enable)
	{
		int vop_id;
		int vop_id_for_prediction;

		vop_id = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
		DPRINTF(XVID_DEBUG_HEADER, "vop_id %i\n", vop_id);
		if (BitstreamGetBit(bs))	/* vop_id_for_prediction_indication */
		{
			vop_id_for_prediction = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
			DPRINTF(XVID_DEBUG_HEADER, "vop_id_for_prediction %i\n", vop_id_for_prediction);
		}
		READ_MARKER();
	}

	return mbnum;
}




/* vol estimation header */
static void
read_vol_complexity_estimation_header(Bitstream * bs, DECODER * dec)
{
	ESTIMATION * e = &dec->estimation;

	e->method = BitstreamGetBits(bs, 2);	/* estimation_method */
	DPRINTF(XVID_DEBUG_HEADER,"+ complexity_estimation_header; method=%i\n", e->method);

	if (e->method == 0 || e->method == 1)
	{
		if (!BitstreamGetBit(bs))		/* shape_complexity_estimation_disable */
		{
			e->opaque = BitstreamGetBit(bs);		/* opaque */
			e->transparent = BitstreamGetBit(bs);		/* transparent */
			e->intra_cae = BitstreamGetBit(bs);		/* intra_cae */
			e->inter_cae = BitstreamGetBit(bs);		/* inter_cae */
			e->no_update = BitstreamGetBit(bs);		/* no_update */
			e->upsampling = BitstreamGetBit(bs);		/* upsampling */
		}

		if (!BitstreamGetBit(bs))	/* texture_complexity_estimation_set_1_disable */
		{
			e->intra_blocks = BitstreamGetBit(bs);		/* intra_blocks */
			e->inter_blocks = BitstreamGetBit(bs);		/* inter_blocks */
			e->inter4v_blocks = BitstreamGetBit(bs);		/* inter4v_blocks */
			e->not_coded_blocks = BitstreamGetBit(bs);		/* not_coded_blocks */
		}
	}

	READ_MARKER();

	if (!BitstreamGetBit(bs))		/* texture_complexity_estimation_set_2_disable */
	{
		e->dct_coefs = BitstreamGetBit(bs);		/* dct_coefs */
		e->dct_lines = BitstreamGetBit(bs);		/* dct_lines */
		e->vlc_symbols = BitstreamGetBit(bs);		/* vlc_symbols */
		e->vlc_bits = BitstreamGetBit(bs);		/* vlc_bits */
	}

	if (!BitstreamGetBit(bs))		/* motion_compensation_complexity_disable */
	{
		e->apm = BitstreamGetBit(bs);		/* apm */
		e->npm = BitstreamGetBit(bs);		/* npm */
		e->interpolate_mc_q = BitstreamGetBit(bs);		/* interpolate_mc_q */
		e->forw_back_mc_q = BitstreamGetBit(bs);		/* forw_back_mc_q */
		e->halfpel2 = BitstreamGetBit(bs);		/* halfpel2 */
		e->halfpel4 = BitstreamGetBit(bs);		/* halfpel4 */
	}

	READ_MARKER();

	if (e->method == 1)
	{
		if (!BitstreamGetBit(bs))	/* version2_complexity_estimation_disable */
		{
			e->sadct = BitstreamGetBit(bs);		/* sadct */
			e->quarterpel = BitstreamGetBit(bs);		/* quarterpel */
		}
	}
}


/* vop estimation header */
static void
read_vop_complexity_estimation_header(Bitstream * bs, DECODER * dec, int coding_type)
{
	ESTIMATION * e = &dec->estimation;

	if (e->method == 0 || e->method == 1)
	{
		if (coding_type == I_VOP) {
			if (e->opaque)		BitstreamSkip(bs, 8);	/* dcecs_opaque */
			if (e->transparent) BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols) BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
		}

		if (coding_type == P_VOP) {
			if (e->opaque) BitstreamSkip(bs, 8);		/* */
			if (e->transparent) BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling) BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols) BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
			if (e->quarterpel)	BitstreamSkip(bs, 8);	/* */
		}
		if (coding_type == B_VOP) {
			if (e->opaque)		BitstreamSkip(bs, 8);	/* */
			if (e->transparent)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->interpolate_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
			if (e->quarterpel)	BitstreamSkip(bs, 8);	/* */
		}

		if (coding_type == S_VOP && dec->sprite_enable == SPRITE_STATIC) {
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks)	BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->interpolate_mc_q) BitstreamSkip(bs, 8);	/* */
		}
	}
}





/*
decode headers
returns coding_type, or -1 if error
*/

#define VIDOBJ_START_CODE_MASK		0x0000001f
#define VIDOBJLAY_START_CODE_MASK	0x0000000f

int
BitstreamReadHeaders(Bitstream * bs,
					 DECODER * dec,
					 uint32_t * rounding,
					 uint32_t * quant,
					 uint32_t * fcode_forward,
					 uint32_t * fcode_backward,
					 uint32_t * intra_dc_threshold,
					 WARPPOINTS *gmc_warp)
{
	uint32_t vol_ver_id;
	uint32_t coding_type;
	uint32_t start_code;
	uint32_t time_incr = 0;
	int32_t time_increment = 0;
	int resize = 0;

	while ((BitstreamPos(bs) >> 3) + 4 <= bs->length) {

		BitstreamByteAlign(bs);
		start_code = BitstreamShowBits(bs, 32);

		if (start_code == VISOBJSEQ_START_CODE) {

			int profile;

			DPRINTF(XVID_DEBUG_STARTCODE, "<visual_object_sequence>\n");

			BitstreamSkip(bs, 32);	/* visual_object_sequence_start_code */
			profile = BitstreamGetBits(bs, 8);	/* profile_and_level_indication */

			DPRINTF(XVID_DEBUG_HEADER, "profile_and_level_indication %i\n", profile);

		} else if (start_code == VISOBJSEQ_STOP_CODE) {

			BitstreamSkip(bs, 32);	/* visual_object_sequence_stop_code */

			DPRINTF(XVID_DEBUG_STARTCODE, "</visual_object_sequence>\n");

		} else if (start_code == VISOBJ_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<visual_object>\n");

			BitstreamSkip(bs, 32);	/* visual_object_start_code */
			if (BitstreamGetBit(bs))	/* is_visual_object_identified */
			{
				dec->ver_id = BitstreamGetBits(bs, 4);	/* visual_object_ver_id */
				DPRINTF(XVID_DEBUG_HEADER,"visobj_ver_id %i\n", dec->ver_id);
				BitstreamSkip(bs, 3);	/* visual_object_priority */
			} else {
				dec->ver_id = 1;
			}

			if (BitstreamShowBits(bs, 4) != VISOBJ_TYPE_VIDEO)	/* visual_object_type */
			{
				DPRINTF(XVID_DEBUG_ERROR, "visual_object_type != video\n");
				return -1;
			}
			BitstreamSkip(bs, 4);

			/* video_signal_type */

			if (BitstreamGetBit(bs))	/* video_signal_type */
			{
				DPRINTF(XVID_DEBUG_HEADER,"+ video_signal_type\n");
				BitstreamSkip(bs, 3);	/* video_format */
				BitstreamSkip(bs, 1);	/* video_range */
				if (BitstreamGetBit(bs))	/* color_description */
				{
					DPRINTF(XVID_DEBUG_HEADER,"+ color_description");
					BitstreamSkip(bs, 8);	/* color_primaries */
					BitstreamSkip(bs, 8);	/* transfer_characteristics */
					BitstreamSkip(bs, 8);	/* matrix_coefficients */
				}
			}
		} else if ((start_code & ~VIDOBJ_START_CODE_MASK) == VIDOBJ_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<video_object>\n");
			DPRINTF(XVID_DEBUG_HEADER, "vo id %i\n", start_code & VIDOBJ_START_CODE_MASK);

			BitstreamSkip(bs, 32);	/* video_object_start_code */

		} else if ((start_code & ~VIDOBJLAY_START_CODE_MASK) == VIDOBJLAY_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<video_object_layer>\n");
			DPRINTF(XVID_DEBUG_HEADER, "vol id %i\n", start_code & VIDOBJLAY_START_CODE_MASK);

			BitstreamSkip(bs, 32);	/* video_object_layer_start_code */
			BitstreamSkip(bs, 1);	/* random_accessible_vol */

            BitstreamSkip(bs, 8);   /* video_object_type_indication */

			if (BitstreamGetBit(bs))	/* is_object_layer_identifier */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ is_object_layer_identifier\n");
				vol_ver_id = BitstreamGetBits(bs, 4);	/* video_object_layer_verid */
				DPRINTF(XVID_DEBUG_HEADER,"ver_id %i\n", vol_ver_id);
				BitstreamSkip(bs, 3);	/* video_object_layer_priority */
			} else {
				vol_ver_id = dec->ver_id;
			}

			dec->aspect_ratio = BitstreamGetBits(bs, 4);

			if (dec->aspect_ratio == VIDOBJLAY_AR_EXTPAR)	/* aspect_ratio_info */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ aspect_ratio_info\n");
				dec->par_width = BitstreamGetBits(bs, 8);	/* par_width */
				dec->par_height = BitstreamGetBits(bs, 8);	/* par_height */
			}

			if (BitstreamGetBit(bs))	/* vol_control_parameters */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ vol_control_parameters\n");
				BitstreamSkip(bs, 2);	/* chroma_format */
				dec->low_delay = BitstreamGetBit(bs);	/* low_delay */
				DPRINTF(XVID_DEBUG_HEADER, "low_delay %i\n", dec->low_delay);
				if (BitstreamGetBit(bs))	/* vbv_parameters */
				{
					unsigned int bitrate;
					unsigned int buffer_size;
					unsigned int occupancy;

					DPRINTF(XVID_DEBUG_HEADER,"+ vbv_parameters\n");

					bitrate = BitstreamGetBits(bs,15) << 15;	/* first_half_bit_rate */
					READ_MARKER();
					bitrate |= BitstreamGetBits(bs,15);		/* latter_half_bit_rate */
					READ_MARKER();

					buffer_size = BitstreamGetBits(bs, 15) << 3;	/* first_half_vbv_buffer_size */
					READ_MARKER();
					buffer_size |= BitstreamGetBits(bs, 3);		/* latter_half_vbv_buffer_size */

					occupancy = BitstreamGetBits(bs, 11) << 15;	/* first_half_vbv_occupancy */
					READ_MARKER();
					occupancy |= BitstreamGetBits(bs, 15);	/* latter_half_vbv_occupancy */
					READ_MARKER();

					DPRINTF(XVID_DEBUG_HEADER,"bitrate %d (unit=400 bps)\n", bitrate);
					DPRINTF(XVID_DEBUG_HEADER,"buffer_size %d (unit=16384 bits)\n", buffer_size);
					DPRINTF(XVID_DEBUG_HEADER,"occupancy %d (unit=64 bits)\n", occupancy);
				}
			}else{
				dec->low_delay = dec->low_delay_default;
			}

			dec->shape = BitstreamGetBits(bs, 2);	/* video_object_layer_shape */

			DPRINTF(XVID_DEBUG_HEADER, "shape %i\n", dec->shape);
			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
			{
				DPRINTF(XVID_DEBUG_ERROR,"non-rectangular shapes are not supported\n");
			}

			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1) {
				BitstreamSkip(bs, 4);	/* video_object_layer_shape_extension */
			}

			READ_MARKER();

			/********************** for decode B-frame time ***********************/
			dec->time_inc_resolution = BitstreamGetBits(bs, 16);	/* vop_time_increment_resolution */
			DPRINTF(XVID_DEBUG_HEADER,"vop_time_increment_resolution %i\n", dec->time_inc_resolution);

			if (dec->time_inc_resolution > 0) {
				dec->time_inc_bits = MAX(log2bin(dec->time_inc_resolution-1), 1);
			} else {
				/* for "old" xvid compatibility, set time_inc_bits = 1 */
				dec->time_inc_bits = 1;
			}

			READ_MARKER();

			if (BitstreamGetBit(bs))	/* fixed_vop_rate */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ fixed_vop_rate\n");
				BitstreamSkip(bs, dec->time_inc_bits);	/* fixed_vop_time_increment */
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR) {
					uint32_t width, height;

					READ_MARKER();
					width = BitstreamGetBits(bs, 13);	/* video_object_layer_width */
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);	/* video_object_layer_height */
					READ_MARKER();

					DPRINTF(XVID_DEBUG_HEADER, "width %i\n", width);
					DPRINTF(XVID_DEBUG_HEADER, "height %i\n", height);

					if (dec->width != width || dec->height != height)
					{
						if (dec->fixed_dimensions)
						{
							DPRINTF(XVID_DEBUG_ERROR, "decoder width/height does not match bitstream\n");
							return -1;
						}
						resize = 1;
						dec->width = width;
						dec->height = height;
					}
				}

				dec->interlacing = BitstreamGetBit(bs);
				DPRINTF(XVID_DEBUG_HEADER, "interlacing %i\n", dec->interlacing);

				if (!BitstreamGetBit(bs))	/* obmc_disable */
				{
					DPRINTF(XVID_DEBUG_ERROR, "obmc_disabled==false not supported\n");
					/* TODO */
					/* fucking divx4.02 has this enabled */
				}

				dec->sprite_enable = BitstreamGetBits(bs, (vol_ver_id == 1 ? 1 : 2));	/* sprite_enable */

				if (dec->sprite_enable == SPRITE_STATIC || dec->sprite_enable == SPRITE_GMC)
				{
					int low_latency_sprite_enable;

					if (dec->sprite_enable != SPRITE_GMC)
					{
						int sprite_width;
						int sprite_height;
						int sprite_left_coord;
						int sprite_top_coord;
						sprite_width = BitstreamGetBits(bs, 13);		/* sprite_width */
						READ_MARKER();
						sprite_height = BitstreamGetBits(bs, 13);	/* sprite_height */
						READ_MARKER();
						sprite_left_coord = BitstreamGetBits(bs, 13);	/* sprite_left_coordinate */
						READ_MARKER();
						sprite_top_coord = BitstreamGetBits(bs, 13);	/* sprite_top_coordinate */
						READ_MARKER();
					}
					dec->sprite_warping_points = BitstreamGetBits(bs, 6);		/* no_of_sprite_warping_points */
					dec->sprite_warping_accuracy = BitstreamGetBits(bs, 2);		/* sprite_warping_accuracy */
					dec->sprite_brightness_change = BitstreamGetBits(bs, 1);		/* brightness_change */
					if (dec->sprite_enable != SPRITE_GMC)
					{
						low_latency_sprite_enable = BitstreamGetBits(bs, 1);		/* low_latency_sprite_enable */
					}
				}

				if (vol_ver_id != 1 &&
					dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
					BitstreamSkip(bs, 1);	/* sadct_disable */
				}

				if (BitstreamGetBit(bs))	/* not_8_bit */
				{
					DPRINTF(XVID_DEBUG_HEADER, "not_8_bit==true (ignored)\n");
					dec->quant_bits = BitstreamGetBits(bs, 4);	/* quant_precision */
					BitstreamSkip(bs, 4);	/* bits_per_pixel */
				} else {
					dec->quant_bits = 5;
				}

				if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
					BitstreamSkip(bs, 1);	/* no_gray_quant_update */
					BitstreamSkip(bs, 1);	/* composition_method */
					BitstreamSkip(bs, 1);	/* linear_composition */
				}

				dec->quant_type = BitstreamGetBit(bs);	/* quant_type */
				DPRINTF(XVID_DEBUG_HEADER, "quant_type %i\n", dec->quant_type);

				if (dec->quant_type) {
					if (BitstreamGetBit(bs))	/* load_intra_quant_mat */
					{
						uint8_t matrix[64];

						DPRINTF(XVID_DEBUG_HEADER, "load_intra_quant_mat\n");

						bs_get_matrix(bs, matrix);
						set_intra_matrix(dec->mpeg_quant_matrices, matrix);
					} else
						set_intra_matrix(dec->mpeg_quant_matrices, get_default_intra_matrix());

					if (BitstreamGetBit(bs))	/* load_inter_quant_mat */
					{
						uint8_t matrix[64];

						DPRINTF(XVID_DEBUG_HEADER, "load_inter_quant_mat\n");

						bs_get_matrix(bs, matrix);
						set_inter_matrix(dec->mpeg_quant_matrices, matrix);
					} else
						set_inter_matrix(dec->mpeg_quant_matrices, get_default_inter_matrix());

					if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
						DPRINTF(XVID_DEBUG_ERROR, "greyscale matrix not supported\n");
						return -1;
					}

				}


				if (vol_ver_id != 1) {
					dec->quarterpel = BitstreamGetBit(bs);	/* quarter_sample */
					DPRINTF(XVID_DEBUG_HEADER,"quarterpel %i\n", dec->quarterpel);
				}
				else
					dec->quarterpel = 0;


				dec->complexity_estimation_disable = BitstreamGetBit(bs);	/* complexity estimation disable */
				if (!dec->complexity_estimation_disable)
				{
					read_vol_complexity_estimation_header(bs, dec);
				}

				BitstreamSkip(bs, 1);	/* resync_marker_disable */

				if (BitstreamGetBit(bs))	/* data_partitioned */
				{
					DPRINTF(XVID_DEBUG_ERROR, "data_partitioned not supported\n");
					BitstreamSkip(bs, 1);	/* reversible_vlc */
				}

				if (vol_ver_id != 1) {
					dec->newpred_enable = BitstreamGetBit(bs);
					if (dec->newpred_enable)	/* newpred_enable */
					{
						DPRINTF(XVID_DEBUG_HEADER, "+ newpred_enable\n");
						BitstreamSkip(bs, 2);	/* requested_upstream_message_type */
						BitstreamSkip(bs, 1);	/* newpred_segment_type */
					}
					dec->reduced_resolution_enable = BitstreamGetBit(bs);	/* reduced_resolution_vop_enable */
					DPRINTF(XVID_DEBUG_HEADER, "reduced_resolution_enable %i\n", dec->reduced_resolution_enable);
				}
				else
				{
					dec->newpred_enable = 0;
					dec->reduced_resolution_enable = 0;
				}

				dec->scalability = BitstreamGetBit(bs);	/* scalability */
				if (dec->scalability)
				{
					DPRINTF(XVID_DEBUG_ERROR, "scalability not supported\n");
					BitstreamSkip(bs, 1);	/* hierarchy_type */
					BitstreamSkip(bs, 4);	/* ref_layer_id */
					BitstreamSkip(bs, 1);	/* ref_layer_sampling_direc */
					BitstreamSkip(bs, 5);	/* hor_sampling_factor_n */
					BitstreamSkip(bs, 5);	/* hor_sampling_factor_m */
					BitstreamSkip(bs, 5);	/* vert_sampling_factor_n */
					BitstreamSkip(bs, 5);	/* vert_sampling_factor_m */
					BitstreamSkip(bs, 1);	/* enhancement_type */
					if(dec->shape == VIDOBJLAY_SHAPE_BINARY /* && hierarchy_type==0 */) {
						BitstreamSkip(bs, 1);	/* use_ref_shape */
						BitstreamSkip(bs, 1);	/* use_ref_texture */
						BitstreamSkip(bs, 5);	/* shape_hor_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* shape_hor_sampling_factor_m */
						BitstreamSkip(bs, 5);	/* shape_vert_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* shape_vert_sampling_factor_m */
					}
					return -1;
				}
			} else				/* dec->shape == BINARY_ONLY */
			{
				if (vol_ver_id != 1) {
					dec->scalability = BitstreamGetBit(bs); /* scalability */
					if (dec->scalability)
					{
						DPRINTF(XVID_DEBUG_ERROR, "scalability not supported\n");
						BitstreamSkip(bs, 4);	/* ref_layer_id */
						BitstreamSkip(bs, 5);	/* hor_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* hor_sampling_factor_m */
						BitstreamSkip(bs, 5);	/* vert_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* vert_sampling_factor_m */
						return -1;
					}
				}
				BitstreamSkip(bs, 1);	/* resync_marker_disable */

			}

			return (resize ? -3 : -2 );	/* VOL */

		} else if (start_code == GRPOFVOP_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<group_of_vop>\n");

			BitstreamSkip(bs, 32);
			{
				int hours, minutes, seconds;

				hours = BitstreamGetBits(bs, 5);
				minutes = BitstreamGetBits(bs, 6);
				READ_MARKER();
				seconds = BitstreamGetBits(bs, 6);

				DPRINTF(XVID_DEBUG_HEADER, "time %ih%im%is\n", hours,minutes,seconds);
			}
			BitstreamSkip(bs, 1);	/* closed_gov */
			BitstreamSkip(bs, 1);	/* broken_link */

		} else if (start_code == VOP_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<vop>\n");

			BitstreamSkip(bs, 32);	/* vop_start_code */

			coding_type = BitstreamGetBits(bs, 2);	/* vop_coding_type */
			DPRINTF(XVID_DEBUG_HEADER, "coding_type %i\n", coding_type);

			/*********************** for decode B-frame time ***********************/
			while (BitstreamGetBit(bs) != 0)	/* time_base */
				time_incr++;

			READ_MARKER();

			if (dec->time_inc_bits) {
				time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	/* vop_time_increment */
			}

			DPRINTF(XVID_DEBUG_HEADER, "time_base %i\n", time_incr);
			DPRINTF(XVID_DEBUG_HEADER, "time_increment %i\n", time_increment);

			DPRINTF(XVID_DEBUG_TIMECODE, "%c %i:%i\n",
				coding_type == I_VOP ? 'I' : coding_type == P_VOP ? 'P' : coding_type == B_VOP ? 'B' : 'S',
				time_incr, time_increment);

			if (coding_type != B_VOP) {
				dec->last_time_base = dec->time_base;
				dec->time_base += time_incr;
				dec->time = dec->time_base*dec->time_inc_resolution + time_increment;
				dec->time_pp = (int32_t)(dec->time - dec->last_non_b_time);
                dec->last_non_b_time = dec->time;
			} else {
                dec->time = (dec->last_time_base + time_incr)*dec->time_inc_resolution + time_increment;
				dec->time_bp = dec->time_pp - (int32_t)(dec->last_non_b_time - dec->time);
			}
            if (dec->time_pp <= 0) dec->time_pp = 1;
			DPRINTF(XVID_DEBUG_HEADER,"time_pp=%i\n", dec->time_pp);
			DPRINTF(XVID_DEBUG_HEADER,"time_bp=%i\n", dec->time_bp);

			READ_MARKER();

			if (!BitstreamGetBit(bs))	/* vop_coded */
			{
				DPRINTF(XVID_DEBUG_HEADER, "vop_coded==false\n");
				return N_VOP;
			}

			if (dec->newpred_enable)
			{
				int vop_id;
				int vop_id_for_prediction;

				vop_id = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
				DPRINTF(XVID_DEBUG_HEADER, "vop_id %i\n", vop_id);
				if (BitstreamGetBit(bs))	/* vop_id_for_prediction_indication */
				{
					vop_id_for_prediction = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
					DPRINTF(XVID_DEBUG_HEADER, "vop_id_for_prediction %i\n", vop_id_for_prediction);
				}
				READ_MARKER();
			}



			/* fix a little bug by MinChen <chenm002@163.com> */
			if ((dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) &&
				( (coding_type == P_VOP) || (coding_type == S_VOP && dec->sprite_enable == SPRITE_GMC) ) ) {
				*rounding = BitstreamGetBit(bs);	/* rounding_type */
				DPRINTF(XVID_DEBUG_HEADER, "rounding %i\n", *rounding);
			}

			if (dec->reduced_resolution_enable &&
				dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR &&
				(coding_type == P_VOP || coding_type == I_VOP)) {

				if (BitstreamGetBit(bs));
					DPRINTF(XVID_DEBUG_ERROR, "RRV not supported (anymore)\n");
			}

			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
				if(!(dec->sprite_enable == SPRITE_STATIC && coding_type == I_VOP)) {

					uint32_t width, height;
					uint32_t horiz_mc_ref, vert_mc_ref;

					width = BitstreamGetBits(bs, 13);
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);
					READ_MARKER();
					horiz_mc_ref = BitstreamGetBits(bs, 13);
					READ_MARKER();
					vert_mc_ref = BitstreamGetBits(bs, 13);
					READ_MARKER();

					DPRINTF(XVID_DEBUG_HEADER, "width %i\n", width);
					DPRINTF(XVID_DEBUG_HEADER, "height %i\n", height);
					DPRINTF(XVID_DEBUG_HEADER, "horiz_mc_ref %i\n", horiz_mc_ref);
					DPRINTF(XVID_DEBUG_HEADER, "vert_mc_ref %i\n", vert_mc_ref);
				}

				BitstreamSkip(bs, 1);	/* change_conv_ratio_disable */
				if (BitstreamGetBit(bs))	/* vop_constant_alpha */
				{
					BitstreamSkip(bs, 8);	/* vop_constant_alpha_value */
				}
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (!dec->complexity_estimation_disable)
				{
					read_vop_complexity_estimation_header(bs, dec, coding_type);
				}

				/* intra_dc_vlc_threshold */
				*intra_dc_threshold =
					intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

				dec->top_field_first = 0;
				dec->alternate_vertical_scan = 0;

				if (dec->interlacing) {
					dec->top_field_first = BitstreamGetBit(bs);
					DPRINTF(XVID_DEBUG_HEADER, "interlace top_field_first %i\n", dec->top_field_first);
					dec->alternate_vertical_scan = BitstreamGetBit(bs);
					DPRINTF(XVID_DEBUG_HEADER, "interlace alternate_vertical_scan %i\n", dec->alternate_vertical_scan);

				}
			}

			if ((dec->sprite_enable == SPRITE_STATIC || dec->sprite_enable== SPRITE_GMC) && coding_type == S_VOP) {

				int i;

				for (i = 0 ; i < dec->sprite_warping_points; i++)
				{
					int length;
					int x = 0, y = 0;

					/* sprite code borowed from ffmpeg; thx Michael Niedermayer <michaelni@gmx.at> */
					length = bs_get_spritetrajectory(bs);
					if(length){
						x= BitstreamGetBits(bs, length);
						if ((x >> (length - 1)) == 0) /* if MSB not set it is negative*/
							x = - (x ^ ((1 << length) - 1));
					}
					READ_MARKER();

					length = bs_get_spritetrajectory(bs);
					if(length){
						y = BitstreamGetBits(bs, length);
						if ((y >> (length - 1)) == 0) /* if MSB not set it is negative*/
							y = - (y ^ ((1 << length) - 1));
					}
					READ_MARKER();

					gmc_warp->duv[i].x = x;
					gmc_warp->duv[i].y = y;

					DPRINTF(XVID_DEBUG_HEADER,"sprite_warping_point[%i] xy=(%i,%i)\n", i, x, y);
				}

				if (dec->sprite_brightness_change)
				{
					/* XXX: brightness_change_factor() */
				}
				if (dec->sprite_enable == SPRITE_STATIC)
				{
					/* XXX: todo */
				}

			}

			if ((*quant = BitstreamGetBits(bs, dec->quant_bits)) < 1)	/* vop_quant */
				*quant = 1;
			DPRINTF(XVID_DEBUG_HEADER, "quant %i\n", *quant);

			if (coding_type != I_VOP) {
				*fcode_forward = BitstreamGetBits(bs, 3);	/* fcode_forward */
				DPRINTF(XVID_DEBUG_HEADER, "fcode_forward %i\n", *fcode_forward);
			}

			if (coding_type == B_VOP) {
				*fcode_backward = BitstreamGetBits(bs, 3);	/* fcode_backward */
				DPRINTF(XVID_DEBUG_HEADER, "fcode_backward %i\n", *fcode_backward);
			}
			if (!dec->scalability) {
				if ((dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) &&
					(coding_type != I_VOP)) {
					BitstreamSkip(bs, 1);	/* vop_shape_coding_type */
				}
			}
			return coding_type;

		} else if (start_code == USERDATA_START_CODE) {
			char tmp[256];
		    int i, version, build;
			char packed;

			BitstreamSkip(bs, 32);	/* user_data_start_code */

			memset(tmp, 0, 256);
			tmp[0] = BitstreamShowBits(bs, 8);

			for(i = 1; i < 256; i++){
				tmp[i] = (BitstreamShowBits(bs, 16) & 0xFF);

				if(tmp[i] == 0)
					break;

				BitstreamSkip(bs, 8);
			}

			DPRINTF(XVID_DEBUG_STARTCODE, "<user_data>: %s\n", tmp);

			/* read xvid bitstream version */
			if(strncmp(tmp, "XviD", 4) == 0) {
				if (tmp[strlen(tmp)-1] == 'C') {				
					sscanf(tmp, "XviD%dC", &dec->bs_version);
					dec->cartoon_mode = 1;
				}
				else
					sscanf(tmp, "XviD%d", &dec->bs_version);

				DPRINTF(XVID_DEBUG_HEADER, "xvid bitstream version=%i\n", dec->bs_version);
			}

		    /* divx detection */
			i = sscanf(tmp, "DivX%dBuild%d%c", &version, &build, &packed);
			if (i < 2)
				i = sscanf(tmp, "DivX%db%d%c", &version, &build, &packed);

			if (i >= 2)
			{
				dec->packed_mode = (i == 3 && packed == 'p');
				DPRINTF(XVID_DEBUG_HEADER, "divx version=%i, build=%i packed=%i\n",
						version, build, dec->packed_mode);
			}

		} else					/* start_code == ? */
		{
			if (BitstreamShowBits(bs, 24) == 0x000001) {
				DPRINTF(XVID_DEBUG_STARTCODE, "<unknown: %x>\n", BitstreamShowBits(bs, 32));
			}
			BitstreamSkip(bs, 8);
		}
	}

#if 0
	DPRINTF("*** WARNING: no vop_start_code found");
#endif
	return -1;					/* ignore it */
}


/* write custom quant matrix */

static void
bs_put_matrix(Bitstream * bs,
			  const uint16_t * matrix)
{
	int i, j;
	const int last = matrix[scan_tables[0][63]];

	for (j = 63; j > 0 && matrix[scan_tables[0][j - 1]] == last; j--);

	for (i = 0; i <= j; i++) {
		BitstreamPutBits(bs, matrix[scan_tables[0][i]], 8);
	}

	if (j < 63) {
		BitstreamPutBits(bs, 0, 8);
	}
}


/*
	write vol header
*/
void
BitstreamWriteVolHeader(Bitstream * const bs,
						const MBParam * pParam,
						const FRAMEINFO * const frame)
{
	static const unsigned int vo_id = 0;
	static const unsigned int vol_id = 0;
	int vol_ver_id = 1;
	int vol_type_ind = VIDOBJLAY_TYPE_SIMPLE;
	int vol_profile = pParam->profile;

	if ( (pParam->vol_flags & XVID_VOL_QUARTERPEL) ||
         (pParam->vol_flags & XVID_VOL_GMC))
		vol_ver_id = 2;

    if ((pParam->vol_flags & (XVID_VOL_MPEGQUANT|XVID_VOL_QUARTERPEL|XVID_VOL_GMC|XVID_VOL_INTERLACING)) ||
         pParam->max_bframes>0) {
        vol_type_ind = VIDOBJLAY_TYPE_ASP;
    }

	/* visual_object_sequence_start_code */
#if 0
	BitstreamPad(bs);
#endif

	/*
	 * no padding here, anymore. You have to make sure that you are
	 * byte aligned, and that always 1-8 padding bits have been written
	 */

    if (!vol_profile) {
		/* Profile was not set by client app, use the more permissive profile
		 * compatible with the vol_type_id */
		switch(vol_type_ind) {
		case VIDOBJLAY_TYPE_ASP:
			vol_profile = 0xf5; /* ASP level 5 */
			break;
		case VIDOBJLAY_TYPE_ART_SIMPLE:
			vol_profile = 0x94; /* ARTS level 4 */
			break;
		default:
			vol_profile = 0x03; /* Simple level 3 */
			break;
		}
	}

	/* Write the VOS header */
	BitstreamPutBits(bs, VISOBJSEQ_START_CODE, 32);
	BitstreamPutBits(bs, vol_profile, 8); 	/* profile_and_level_indication */


	/* visual_object_start_code */
	BitstreamPad(bs);
	BitstreamPutBits(bs, VISOBJ_START_CODE, 32);
	BitstreamPutBits(bs, 0, 1);		/* is_visual_object_identifier */

	/* Video type */
	BitstreamPutBits(bs, VISOBJ_TYPE_VIDEO, 4);		/* visual_object_type */
	BitstreamPutBit(bs, 0); /* video_signal_type */

	/* video object_start_code & vo_id */
	BitstreamPadAlways(bs); /* next_start_code() */
	BitstreamPutBits(bs, VIDOBJ_START_CODE|(vo_id&0x5), 32);

	/* video_object_layer_start_code & vol_id */
	BitstreamPad(bs);
	BitstreamPutBits(bs, VIDOBJLAY_START_CODE|(vol_id&0x4), 32);

	BitstreamPutBit(bs, 0);		/* random_accessible_vol */
	BitstreamPutBits(bs, vol_type_ind, 8);	/* video_object_type_indication */

	if (vol_ver_id == 1) {
		BitstreamPutBit(bs, 0);				/* is_object_layer_identified (0=not given) */
	} else {
		BitstreamPutBit(bs, 1);		/* is_object_layer_identified */
		BitstreamPutBits(bs, vol_ver_id, 4);	/* vol_ver_id == 2 */
		BitstreamPutBits(bs, 4, 3); /* vol_ver_priority (1==highest, 7==lowest) */
	}

	/* Aspect ratio */
	BitstreamPutBits(bs, pParam->par, 4); /* aspect_ratio_info (1=1:1) */
	if(pParam->par == XVID_PAR_EXT) {
		BitstreamPutBits(bs, pParam->par_width, 8);
		BitstreamPutBits(bs, pParam->par_height, 8);
	}

	BitstreamPutBit(bs, 1);	/* vol_control_parameters */
	BitstreamPutBits(bs, 1, 2);	/* chroma_format 1="4:2:0" */

	if (pParam->max_bframes > 0) {
		BitstreamPutBit(bs, 0);	/* low_delay */
	} else
	{
		BitstreamPutBit(bs, 1);	/* low_delay */
	}
	BitstreamPutBit(bs, 0);	/* vbv_parameters (0=not given) */

	BitstreamPutBits(bs, 0, 2);	/* video_object_layer_shape (0=rectangular) */

	WRITE_MARKER();

	/*
	 * time_inc_resolution; ignored by current decore versions
	 * eg. 2fps     res=2       inc=1
	 *     25fps    res=25      inc=1
	 *     29.97fps res=30000   inc=1001
	 */
	BitstreamPutBits(bs, pParam->fbase, 16);

	WRITE_MARKER();

    if (pParam->fincr>0) {
		BitstreamPutBit(bs, 1);		/* fixed_vop_rate = 1 */
		BitstreamPutBits(bs, pParam->fincr, MAX(log2bin(pParam->fbase-1),1));	/* fixed_vop_time_increment */
    }else{
        BitstreamPutBit(bs, 0);		/* fixed_vop_rate = 0 */
    }

	WRITE_MARKER();
	BitstreamPutBits(bs, pParam->width, 13);	/* width */
	WRITE_MARKER();
	BitstreamPutBits(bs, pParam->height, 13);	/* height */
	WRITE_MARKER();

	BitstreamPutBit(bs, pParam->vol_flags & XVID_VOL_INTERLACING);	/* interlace */
	BitstreamPutBit(bs, 1);		/* obmc_disable (overlapped block motion compensation) */

	if (vol_ver_id != 1)
	{	if ((pParam->vol_flags & XVID_VOL_GMC))
		{	BitstreamPutBits(bs, 2, 2);		/* sprite_enable=='GMC' */
			BitstreamPutBits(bs, 3, 6);		/* no_of_sprite_warping_points */
			BitstreamPutBits(bs, 3, 2);		/* sprite_warping_accuracy 0==1/2, 1=1/4, 2=1/8, 3=1/16 */
			BitstreamPutBit(bs, 0);			/* sprite_brightness_change (not supported) */

			/*
			 * currently we use no_of_sprite_warping_points==2, sprite_warping_accuracy==3
			 * for DivX5 compatability
			 */

		} else
			BitstreamPutBits(bs, 0, 2);		/* sprite_enable==off */
	}
	else
		BitstreamPutBit(bs, 0);		/* sprite_enable==off */

	BitstreamPutBit(bs, 0);		/* not_8_bit */

	/* quant_type   0=h.263  1=mpeg4(quantizer tables) */
	BitstreamPutBit(bs, pParam->vol_flags & XVID_VOL_MPEGQUANT);

	if ((pParam->vol_flags & XVID_VOL_MPEGQUANT)) {
		BitstreamPutBit(bs, is_custom_intra_matrix(pParam->mpeg_quant_matrices));	/* load_intra_quant_mat */
		if(is_custom_intra_matrix(pParam->mpeg_quant_matrices))
			bs_put_matrix(bs, get_intra_matrix(pParam->mpeg_quant_matrices));

		BitstreamPutBit(bs, is_custom_inter_matrix(pParam->mpeg_quant_matrices));	/* load_inter_quant_mat */
		if(is_custom_inter_matrix(pParam->mpeg_quant_matrices))
			bs_put_matrix(bs, get_inter_matrix(pParam->mpeg_quant_matrices));
	}

	if (vol_ver_id != 1) {
		if ((pParam->vol_flags & XVID_VOL_QUARTERPEL))
			BitstreamPutBit(bs, 1);	 	/* quarterpel  */
		else
			BitstreamPutBit(bs, 0);		/* no quarterpel */
	}

	BitstreamPutBit(bs, 1);		/* complexity_estimation_disable */
	BitstreamPutBit(bs, 1);		/* resync_marker_disable */
	BitstreamPutBit(bs, 0);		/* data_partitioned */

	if (vol_ver_id != 1) {
		BitstreamPutBit(bs, 0);		/* newpred_enable */
		BitstreamPutBit(bs, 0);		/* reduced_resolution_vop_enabled */
	}

	BitstreamPutBit(bs, 0);		/* scalability */

	BitstreamPadAlways(bs); /* next_start_code(); */

	/* divx5 userdata string */
#define DIVX5_ID ((char *)"DivX503b1393")
  if ((pParam->global_flags & XVID_GLOBAL_DIVX5_USERDATA)) {
    BitstreamWriteUserData(bs, DIVX5_ID, strlen(DIVX5_ID));
  	if (pParam->max_bframes > 0 && (pParam->global_flags & XVID_GLOBAL_PACKED))
      BitstreamPutBits(bs, 'p', 8);
	}

	/* xvid id */
	{
		const char xvid_user_format[] = "XviD%04d%c";
		char xvid_user_data[100];
		sprintf(xvid_user_data,
				xvid_user_format,
				XVID_BS_VERSION,
				(frame->vop_flags & XVID_VOP_CARTOON)?'C':'\0');
		BitstreamWriteUserData(bs, xvid_user_data, strlen(xvid_user_data));
	}
}


/*
  write vop header
*/
void
BitstreamWriteVopHeader(
						Bitstream * const bs,
						const MBParam * pParam,
						const FRAMEINFO * const frame,
						int vop_coded,
						unsigned int quant)
{
	uint32_t i;

#if 0
	BitstreamPad(bs);
#endif

	/*
	 * no padding here, anymore. You have to make sure that you are
	 * byte aligned, and that always 1-8 padding bits have been written
	 */

	BitstreamPutBits(bs, VOP_START_CODE, 32);

	BitstreamPutBits(bs, frame->coding_type, 2);
#if 0
	DPRINTF(XVID_DEBUG_HEADER, "coding_type = %i\n", frame->coding_type);
#endif

	for (i = 0; i < frame->seconds; i++) {
		BitstreamPutBit(bs, 1);
	}
	BitstreamPutBit(bs, 0);

	WRITE_MARKER();

	/* time_increment: value=nth_of_sec, nbits = log2(resolution) */
	BitstreamPutBits(bs, frame->ticks, MAX(log2bin(pParam->fbase-1), 1));
#if 0
	DPRINTF("[%i:%i] %c",
			frame->seconds, frame->ticks,
			frame->coding_type == I_VOP ? 'I' :
			frame->coding_type == P_VOP ? 'P' :
			frame->coding_type == S_VOP ? 'S' :	'B');
#endif

	WRITE_MARKER();

	if (!vop_coded) {
		BitstreamPutBits(bs, 0, 1);
#if 0
		BitstreamPadAlways(bs); /*  next_start_code() */
#endif
		/* NB: It's up to the function caller to write the next_start_code().
		 * At the moment encoder.c respects that requisite because a VOP
		 * always ends with a next_start_code either if it's coded or not
		 * and encoder.c terminates a frame with a next_start_code in whatever
		 * case */
		return;
	}

	BitstreamPutBits(bs, 1, 1);	/* vop_coded */

	if ( (frame->coding_type == P_VOP) || (frame->coding_type == S_VOP) )
		BitstreamPutBits(bs, frame->rounding_type, 1);

	BitstreamPutBits(bs, 0, 3);	/* intra_dc_vlc_threshold */

	if ((frame->vol_flags & XVID_VOL_INTERLACING)) {
		BitstreamPutBit(bs, (frame->vop_flags & XVID_VOP_TOPFIELDFIRST));
		BitstreamPutBit(bs, (frame->vop_flags & XVID_VOP_ALTERNATESCAN));
	}

	if (frame->coding_type == S_VOP) {
		if (1)	{		/* no_of_sprite_warping_points>=1 (we use 2!) */
			int k;
			for (k=0;k<3;k++)
			{
				bs_put_spritetrajectory(bs, frame->warp.duv[k].x ); /* du[k]  */
				WRITE_MARKER();

				bs_put_spritetrajectory(bs, frame->warp.duv[k].y ); /* dv[k]  */
				WRITE_MARKER();

			if ((frame->vol_flags & XVID_VOL_QUARTERPEL))
			{
				DPRINTF(XVID_DEBUG_HEADER,"sprite_warping_point[%i] xy=(%i,%i) *QPEL*\n", k, frame->warp.duv[k].x/2, frame->warp.duv[k].y/2);
			}
			else
			{
				DPRINTF(XVID_DEBUG_HEADER,"sprite_warping_point[%i] xy=(%i,%i)\n", k, frame->warp.duv[k].x, frame->warp.duv[k].y);
			}
			}
		}
	}


#if 0
	DPRINTF(XVID_DEBUG_HEADER, "quant = %i\n", quant);
#endif

	BitstreamPutBits(bs, quant, 5);	/* quantizer */

	if (frame->coding_type != I_VOP)
		BitstreamPutBits(bs, frame->fcode, 3);	/* forward_fixed_code */

	if (frame->coding_type == B_VOP)
		BitstreamPutBits(bs, frame->bcode, 3);	/* backward_fixed_code */

}

void
BitstreamWriteUserData(Bitstream * const bs,
						const char *data,
						const unsigned int length)
{
	int i;

	BitstreamPad(bs);
	BitstreamPutBits(bs, USERDATA_START_CODE, 32);

	for (i = 0; i < length; i++) {
		BitstreamPutBits(bs, data[i], 8);
	}

}

/*
 * Group of VOP
 */
void
BitstreamWriteGroupOfVopHeader(Bitstream * const bs,
                               const MBParam * pParam,
                               uint32_t is_closed_gov)
{
  int64_t time = (pParam->m_stamp + (pParam->fbase/2)) / pParam->fbase;
  int hours, minutes, seconds;

  /* compute time_code */
  seconds = time % 60; time /= 60;
  minutes = time % 60; time /= 60;
  hours = time % 24; /* don't overflow */
      
  BitstreamPutBits(bs, GRPOFVOP_START_CODE, 32);
  BitstreamPutBits(bs, hours, 5);
  BitstreamPutBits(bs, minutes, 6);
  BitstreamPutBit(bs, 1);
  BitstreamPutBits(bs, seconds, 6);
  BitstreamPutBits(bs, is_closed_gov, 1);
  BitstreamPutBits(bs, 0, 1); /* broken_link */
}

/*
 * End of Sequence
 */
void
BitstreamWriteEndOfSequence(Bitstream * const bs)
{
    BitstreamPadAlways(bs);
    BitstreamPutBits(bs, VISOBJSEQ_STOP_CODE, 32);
}

/*
 * Video Packet (resync marker)
 */

void write_video_packet_header(Bitstream * const bs,
                               const MBParam * pParam,
                               const FRAMEINFO * const frame,
                               int mbnum)
{
    const int mbnum_bits = log2bin(pParam->mb_width *  pParam->mb_height - 1);
    uint32_t nbitsresyncmarker;

    if (frame->coding_type == I_VOP)
      nbitsresyncmarker = NUMBITS_VP_RESYNC_MARKER;  /* 16 zeros followed by a 1. */
    else if (frame->coding_type == P_VOP)
      nbitsresyncmarker = NUMBITS_VP_RESYNC_MARKER-1 + frame->fcode;
    else /* B_VOP */
      nbitsresyncmarker = MAX(NUMBITS_VP_RESYNC_MARKER+1, NUMBITS_VP_RESYNC_MARKER-1 + MAX(frame->fcode, frame->bcode));

    BitstreamPadAlways(bs);
    BitstreamPutBits(bs, RESYNC_MARKER, nbitsresyncmarker);
    BitstreamPutBits(bs, mbnum, mbnum_bits);
    BitstreamPutBits(bs, frame->quant, 5);
    BitstreamPutBit(bs, 0); /* hec */
}
