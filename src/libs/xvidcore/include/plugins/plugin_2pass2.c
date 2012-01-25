/******************************************************************************
 *
 *  XviD Bit Rate Controller Library
 *  - VBR 2 pass bitrate controller implementation -
 *
 *  Copyright (C)      2002 Foxer <email?>
 *                     2002 Dirk Knop <dknop@gwdg.de>
 *                2002-2003 Edouard Gomez <ed.gomez@free.fr>
 *                     2003 Pete Ross <pross@xvid.org>
 *
 *  This curve treatment algorithm is the one originally implemented by Foxer
 *  and tuned by Dirk Knop for the XviD vfw frontend.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: plugin_2pass2.c,v 1.7 2005/03/27 03:59:42 suxen_drol Exp $
 *
 *****************************************************************************/

#define BQUANT_PRESCALE
#undef COMPENSATE_FORMULA

/* forces second pass not to be bigger than first */
#undef PASS_SMALLER

/* automatically alters overflow controls (strength and improvement/degradation)
	to fight most common problems without user's knowladge */
#define SMART_OVERFLOW_SETTING

#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "../xvid.h"
#include "../image/image.h"

/*****************************************************************************
 * Some default settings
 ****************************************************************************/

#define DEFAULT_KEYFRAME_BOOST 0
#define DEFAULT_OVERFLOW_CONTROL_STRENGTH 10
#define DEFAULT_CURVE_COMPRESSION_HIGH 0
#define DEFAULT_CURVE_COMPRESSION_LOW 0
#define DEFAULT_MAX_OVERFLOW_IMPROVEMENT 10
#define DEFAULT_MAX_OVERFLOW_DEGRADATION 10

/* Keyframe settings */
#define DEFAULT_KFREDUCTION 20
#define DEFAULT_KFTHRESHOLD 1

/*****************************************************************************
 * Some default constants (can be tuned)
 ****************************************************************************/

/* Specify the invariant part of the headers bits (header+MV)
 * as  hlength/cst */
#define INVARIANT_HEADER_PART_IVOP 1 /* factor 1.0f   */
#define INVARIANT_HEADER_PART_PVOP 2 /* factor 0.5f   */
#define INVARIANT_HEADER_PART_BVOP 8 /* factor 0.125f */

/*****************************************************************************
 * Structures
 ****************************************************************************/

/* Statistics */
typedef struct {
	int type;               /* first pass type */
	int quant;              /* first pass quant */
	int blks[3];            /* k,m,y blks */
	int length;             /* first pass length */
	int invariant;          /* what we assume as being invariant between the two passes, it's a sub part of header + MV bits */
	int scaled_length;      /* scaled length */
	int desired_length;     /* desired length; calculated during encoding */
	int error;

	int zone_mode;   /* XVID_ZONE_xxx */
	double weight;
} twopass_stat_t;

/* Context struct */
typedef struct
{
	xvid_plugin_2pass2_t param;

	/*----------------------------------
	 * constant statistical data
	 *--------------------------------*/

	/* Number of frames of the sequence */
	int num_frames;

	/* Number of Intra frames of the sequence */
	int num_keyframes;

	/* Target filesize to reach */
	uint64_t target;

	/* Count of each frame types */
	int count[3];

	/* Total length of each frame types (1st pass) */
	uint64_t tot_length[3];
	uint64_t tot_invariant[3];

	/* Average length of each frame types (used first for 1st pass data and
	 * then for scaled averages */
	double avg_length[3];

	/* Minimum frame length allowed for each frame type */
	int min_length[3];

	/* Total bytes per frame type once the curve has been scaled
	 * NB: advanced parameters do not change this value. This field
	 *     represents the total scaled w/o any advanced settings */
	uint64_t tot_scaled_length[3];

	/* Maximum observed frame size observed during the first pass, the RC
	 * will try tp force all frame sizes in the second pass to be under that
	 * limit */
	int max_length;

	/*----------------------------------
	 * Zones statistical data
	 *--------------------------------*/

	/* Total length used by XVID_ZONE_QUANT zones */
	uint64_t tot_quant;
	uint64_t tot_quant_invariant;

	/* Holds the total amount of frame bytes, zone weighted (only scalable
	 * part of frame bytes) */
	uint64_t tot_weighted;

	/*----------------------------------
	 * Advanced settings helper ratios
	 *--------------------------------*/

	/* This the ratio that has to be applied to all p/b frames in order
	 * to reserve/retrieve bits for/from keyframe boosting and consecutive
	 * keyframe penalty */
	double pb_iboost_tax_ratio;

	/* This the ratio to apply to all b/p frames in order to respect the
	 * assymetric curve compression while respecting a target filesize
	 * NB: The assymetric delta gain has to be computed before this ratio
	 *     is applied, and then the delta is added to the scaled size */
	double assymetric_tax_ratio;

	/*----------------------------------
	 * Data from the stats file kept
	 * into RAM for easy access
	 *--------------------------------*/

	/* Array of keyframe locations
	 * eg: rc->keyframe_locations[100] returns the frame number of the 100th
	 *     keyframe */
	int *keyframe_locations;

	/* Index of the last keyframe used in the keyframe_location */
	int KF_idx;

	/* Array of all 1st pass data file -- see the twopass_stat_t structure
	 * definition for more details */
	twopass_stat_t * stats;

	/*----------------------------------
	 * Hysteresis helpers
	 *--------------------------------*/

	/* This field holds the int2float conversion errors of each quant per
	 * frame type, this allow the RC to keep track of rouding error and thus
	 * increase or decrease the chosen quant according to this residue */
	double quant_error[3][32];

	/* This fields stores the count of each quant usage per frame type
	 * No real role but for debugging */
	int quant_count[3][32];

	/* Last valid quantizer used per frame type, it allows quantizer
	 * increament/decreament limitation in order to avoid big image quality
	 * "jumps" */
	int last_quant[3];

	/*----------------------------------
	 * Overflow control
	 *--------------------------------*/

	/* Current overflow that has to be distributed to p/b frames */
	double overflow;

	/* Total overflow for keyframes -- not distributed directly */
	double KFoverflow;

	/* Amount of keyframe overflow to introduce to the global p/b frame
	 * overflow counter at each encoded frame */
	double KFoverflow_partial;

	/* Unknown ???
	 * ToDo: description */
	double fq_error;

	int min_quant; /* internal minimal quant, prevents wrong quants from being used */

	/*----------------------------------
	 * Debug
	 *--------------------------------*/
	double desired_total;
	double real_total;
} rc_2pass2_t;


/*****************************************************************************
 * Sub plugin functions prototypes
 ****************************************************************************/

static int rc_2pass2_create(xvid_plg_create_t * create, rc_2pass2_t ** handle);
static int rc_2pass2_before(rc_2pass2_t * rc, xvid_plg_data_t * data);
static int rc_2pass2_after(rc_2pass2_t * rc, xvid_plg_data_t * data);
static int rc_2pass2_destroy(rc_2pass2_t * rc, xvid_plg_destroy_t * destroy);

/*****************************************************************************
 * Plugin definition
 ****************************************************************************/

int
xvid_plugin_2pass2(void * handle, int opt, void * param1, void * param2)
{
	switch(opt) {
	case XVID_PLG_INFO :
	case XVID_PLG_FRAME :
		return 0;

	case XVID_PLG_CREATE :
		return rc_2pass2_create((xvid_plg_create_t*)param1, param2);

	case XVID_PLG_DESTROY :
		return rc_2pass2_destroy((rc_2pass2_t*)handle, (xvid_plg_destroy_t*)param1);

	case XVID_PLG_BEFORE :
		return rc_2pass2_before((rc_2pass2_t*)handle, (xvid_plg_data_t*)param1);

	case XVID_PLG_AFTER :
		return rc_2pass2_after((rc_2pass2_t*)handle, (xvid_plg_data_t*)param1);
	}

	return XVID_ERR_FAIL;
}

/*****************************************************************************
 * Sub plugin functions definitions
 ****************************************************************************/

/* First a few local helping function prototypes */
static  int statsfile_count_frames(rc_2pass2_t * rc, char * filename);
static  int statsfile_load(rc_2pass2_t *rc, char * filename);
static void zone_process(rc_2pass2_t *rc, const xvid_plg_create_t * create);
static void first_pass_stats_prepare_data(rc_2pass2_t * rc);
static void first_pass_scale_curve_internal(rc_2pass2_t *rc);
static void scaled_curve_apply_advanced_parameters(rc_2pass2_t * rc);
static  int check_curve_for_vbv_compliancy(rc_2pass2_t * rc, const float fps);
static  int scale_curve_for_vbv_compliancy(rc_2pass2_t * rc, const float fps);
#if 0
static void stats_print(rc_2pass2_t * rc);
#endif

/*----------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

static int
rc_2pass2_create(xvid_plg_create_t * create, rc_2pass2_t **handle)
{
	xvid_plugin_2pass2_t * param = (xvid_plugin_2pass2_t *)create->param;
	rc_2pass2_t * rc;
	int i;

	rc = malloc(sizeof(rc_2pass2_t));
	if (rc == NULL)
		return XVID_ERR_MEMORY;

	/* v1.0.x */
	rc->param.version = param->version;
	rc->param.bitrate = param->bitrate;
	rc->param.filename = param->filename;
	rc->param.keyframe_boost = param->keyframe_boost;
	rc->param.curve_compression_high = param->curve_compression_high;
	rc->param.curve_compression_low = param->curve_compression_low;
	rc->param.overflow_control_strength = param->overflow_control_strength;
	rc->param.max_overflow_improvement = param->max_overflow_improvement;
	rc->param.max_overflow_degradation = param->max_overflow_degradation;
	rc->param.kfreduction = param->kfreduction;
	rc->param.kfthreshold = param->kfthreshold;
	rc->param.container_frame_overhead = param->container_frame_overhead;

	if (XVID_VERSION_MINOR(param->version) >= 1) {
		rc->param.vbv_size = param->vbv_size;
		rc->param.vbv_initial = param->vbv_initial;
		rc->param.vbv_maxrate = param->vbv_maxrate;
		rc->param.vbv_peakrate = param->vbv_peakrate;
	}else{
		rc->param.vbv_size =
		rc->param.vbv_initial =
		rc->param.vbv_maxrate =
		rc->param.vbv_peakrate = 0;
	}

	/* Initialize all defaults */
#define _INIT(a, b) if((a) <= 0) (a) = (b)
	/* Let's set our defaults if needed */
	_INIT(rc->param.keyframe_boost, DEFAULT_KEYFRAME_BOOST);
	_INIT(rc->param.overflow_control_strength, DEFAULT_OVERFLOW_CONTROL_STRENGTH);
	_INIT(rc->param.curve_compression_high, DEFAULT_CURVE_COMPRESSION_HIGH);
	_INIT(rc->param.curve_compression_low, DEFAULT_CURVE_COMPRESSION_LOW);
	_INIT(rc->param.max_overflow_improvement, DEFAULT_MAX_OVERFLOW_IMPROVEMENT);
	_INIT(rc->param.max_overflow_degradation,  DEFAULT_MAX_OVERFLOW_DEGRADATION);

	/* Keyframe settings */
	_INIT(rc->param.kfreduction, DEFAULT_KFREDUCTION);
	_INIT(rc->param.kfthreshold, DEFAULT_KFTHRESHOLD);
#undef _INIT

	/* Initialize some stuff to zero */
	for(i=0; i<3; i++) {
		int j;
		for (j=0; j<32; j++) {
			rc->quant_error[i][j] = 0;
			rc->quant_count[i][j] = 0;
		}
	}

	for (i=0; i<3; i++) rc->last_quant[i] = 0;

	rc->fq_error = 0;
	rc->min_quant = 1;

	/* Count frames (and intra frames) in the stats file, store the result into
	 * the rc structure */
	if (statsfile_count_frames(rc, param->filename) == -1) {
		DPRINTF(XVID_DEBUG_RC,"[xvid rc] -- ERROR: fopen %s failed\n", param->filename);
		free(rc);
		return(XVID_ERR_FAIL);
	}

	/* Allocate the stats' memory */
	if ((rc->stats = malloc(rc->num_frames * sizeof(twopass_stat_t))) == NULL) {
		free(rc);
		return(XVID_ERR_MEMORY);
	}

	/* Allocate keyframes location's memory
	 * PS: see comment in pre_process0 for the +1 location requirement */
	rc->keyframe_locations = malloc((rc->num_keyframes + 1) * sizeof(int));
	if (rc->keyframe_locations == NULL) {
		free(rc->stats);
		free(rc);
		return(XVID_ERR_MEMORY);
	}

	/* Load the first pass stats */
	if (statsfile_load(rc, param->filename) == -1) {
		DPRINTF(XVID_DEBUG_RC,"[xvid rc] -- ERROR: fopen %s failed\n", param->filename);
		free(rc->keyframe_locations);
		free(rc->stats);
		free(rc);
		return XVID_ERR_FAIL;
	}

	/* Compute the target filesize */
	if (rc->param.bitrate<0) {
		/* if negative, bitrate equals the target (in kbytes) */
		rc->target = ((uint64_t)(-rc->param.bitrate)) * 1024;
	} else if (rc->num_frames  < create->fbase/create->fincr) {
		/* Source sequence is less than 1s long, we do as if it was 1s long */
		rc->target = rc->param.bitrate / 8;
	} else {
		/* Target filesize = bitrate/8 * numframes / framerate */
		rc->target =
			((uint64_t)rc->param.bitrate * (uint64_t)rc->num_frames * \
			 (uint64_t)create->fincr) / \
			((uint64_t)create->fbase * 8);
	}

	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Frame rate: %d/%d (%ffps)\n",
			create->fbase, create->fincr,
			(double)create->fbase/(double)create->fincr);
	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Number of frames: %d\n", rc->num_frames);
	if(rc->param.bitrate>=0)
		DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Target bitrate: %ld\n", rc->param.bitrate);
	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Target filesize: %lld\n", rc->target);

	/* Compensate the average frame overhead caused by the container */
	rc->target -= rc->num_frames*rc->param.container_frame_overhead;
	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Container Frame overhead: %d\n", rc->param.container_frame_overhead);
	if(rc->param.container_frame_overhead)
		DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- New target filesize after container compensation: %lld\n", rc->target);

	/* When bitrate is not given it means it has been scaled by an external
	 * application */
	if (rc->param.bitrate) {
		/* Apply zone settings
		 * - set rc->tot_quant which represents the total num of bytes spent in
		 *   fixed quant zones
		 * - set rc->tot_weighted which represents the total amount of bytes
		 *   spent in normal or weighted zones in first pass (normal zones can
		 *   be considered weight=1)
		 * - set rc->tot_quant_invariant which represents the total num of bytes
		 *   spent in fixed quant zones for headers */
		zone_process(rc, create);
	} else {
		/* External scaling -- zones are ignored */
		for (i=0;i<rc->num_frames;i++) {
			rc->stats[i].zone_mode = XVID_ZONE_WEIGHT;
			rc->stats[i].weight = 1.0;
		}
		rc->tot_quant = 0;
	}

	/* Gathers some information about first pass stats:
	 *  - finds the minimum frame length for each frame type during 1st pass.
	 *     rc->min_size[]
	 *  - determines the maximum frame length observed (no frame type distinction).
	 *     rc->max_size
	 *  - count how many times each frame type has been used.
	 *     rc->count[]
	 *  - total bytes used per frame type
	 *     rc->tot_length[]
	 *  - total bytes considered invariant between the 2 passes
	 *  - store keyframe location
	 *     rc->keyframe_locations[]
	 */
	first_pass_stats_prepare_data(rc);

	/* If we have a user bitrate, it means it's an internal curve scaling */
	if (rc->param.bitrate) {
		/* Perform internal curve scaling */
		first_pass_scale_curve_internal(rc);
	}

	/* Apply advanced curve options, and compute some parameters in order to
	 * shape the curve in the BEFORE/AFTER pair of functions */
	scaled_curve_apply_advanced_parameters(rc);

	/* Check curve for VBV compliancy and rescale if necessary */
#ifdef VBV_FORCE
	if (rc->param.vbv_size==0) {
		rc->param.vbv_size      =  3145728;
		rc->param.vbv_initial   =  2359296;
		rc->param.vbv_maxrate   =  4854000;
		rc->param.vbv_peakrate  =  8000000;
	}
#endif

	/* vbv_size==0 switches VBV check off */
	if (rc->param.vbv_size > 0)  {
		const double fps = (double)create->fbase/(double)create->fincr;
		int status = check_curve_for_vbv_compliancy(rc, fps);

		if (status) {
			DPRINTF(XVID_DEBUG_RC, "[xvid rc] Underflow detected - Scaling Curve for compliancy.\n");
		}

		status = scale_curve_for_vbv_compliancy(rc, fps);

		if (status == 0) {
			DPRINTF(XVID_DEBUG_RC, "[xvid rc] VBV compliant curve scaling done.\n");
		} else {
			DPRINTF(XVID_DEBUG_RC, "[xvid rc] VBV compliant curve scaling impossible.\n");
		}
	}
	*handle = rc;
	return(0);
}

/*----------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

static int
rc_2pass2_destroy(rc_2pass2_t * rc, xvid_plg_destroy_t * destroy)
{
	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- target_total:%lld desired_total:%.2f (%.2f%%) actual_total:%.2f (%.2f%%)\n",
			rc->target,
			rc->desired_total,
			100*rc->desired_total/(double)rc->target,
			rc->real_total,
			100*rc->real_total/(double)rc->target);

	free(rc->keyframe_locations);
	free(rc->stats);
	free(rc);
	return(0);
}

/*----------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

static int
rc_2pass2_before(rc_2pass2_t * rc, xvid_plg_data_t * data)
{
	twopass_stat_t * s = &rc->stats[data->frame_num];
	double dbytes;
	double scaled_quant;
	double overflow;
	int capped_to_max_framesize = 0;

	/* This function is quite long but easy to understand. In order to simplify
	 * the code path (a bit), we treat 3 cases that can return immediatly. */

	/* First case: Another plugin has already set a quantizer */
	if (data->quant > 0)
		return(0);

	/* Second case: insufficent stats data
	 * We can't guess much what we should do, let core decide all alone */
	if (data->frame_num >= rc->num_frames) {
		DPRINTF(XVID_DEBUG_RC,"[xvid rc] -- stats file too short (now processing frame %d)",
			data->frame_num);
		return(0);
	}

	/* Third case: We are in a Quant zone
	 * Quant zones must just ensure we use the same settings as first pass
	 * So set the quantizer and the type */
	if (s->zone_mode == XVID_ZONE_QUANT) {
		/* Quant stuff */
		rc->fq_error += s->weight;
		data->quant = (int)rc->fq_error;
		rc->fq_error -= data->quant;

		/* The type stuff */
		data->type = s->type;

		/* The only required data for AFTER step is this one for the overflow
		 * control */
		s->desired_length = s->length;

		return(0);
	}


	/*************************************************************************/
	/*************************************************************************/
	/*************************************************************************/

	/*-------------------------------------------------------------------------
	 * Frame bit allocation first part
	 *
	 * First steps apply user settings, just like it is done in the theoritical
	 * scaled_curve_apply_advanced_parameters
	 *-----------------------------------------------------------------------*/

	/* Set desired to what we are wanting to obtain for this frame */
	dbytes = (double)s->scaled_length;

	/* IFrame user settings*/
	if (s->type == XVID_TYPE_IVOP) {
		/* Keyframe boosting -- All keyframes benefit from it */
		dbytes += dbytes*rc->param.keyframe_boost / 100;

#if 0 /* ToDo: decide how to apply kfthresholding */
#endif
	} else {

		/* P/S/B frames must reserve some bits for iframe boosting */
		dbytes *= rc->pb_iboost_tax_ratio;

		/* Apply assymetric curve compression */
		if (rc->param.curve_compression_high || rc->param.curve_compression_low) {
			double assymetric_delta;

			/* Compute the assymetric delta, this is computed before applying
			 * the tax, as done in the pre_process function */
			if (dbytes > rc->avg_length[s->type-1])
				assymetric_delta = (rc->avg_length[s->type-1] - dbytes) * rc->param.curve_compression_high / 100.0;
			else
				assymetric_delta = (rc->avg_length[s->type-1] - dbytes) * rc->param.curve_compression_low  / 100.0;

			/* Now we must apply the assymetric tax, else our curve compression
			 * would not give a theoritical target size equal to what it is
			 * expected */
			dbytes *= rc->assymetric_tax_ratio;

			/* Now we can add the assymetric delta */
			dbytes += assymetric_delta;
		}
	}

	/* That is what we would like to have -- Don't put that chunk after
	 * overflow control, otherwise, overflow is counted twice and you obtain
	 * half sized bitrate sequences */
	s->desired_length  = (int)dbytes;
	rc->desired_total += dbytes;

	/*------------------------------------------------------------------------
	 * Frame bit allocation: overflow control part.
	 *
	 * Unlike the theoritical scaled_curve_apply_advanced_parameters, here
	 * it's real encoding and we need to make sure we don't go so far from
	 * what is our ideal scaled curve.
	 *-----------------------------------------------------------------------*/

	/* Compute the overflow we should compensate */
	if (s->type != XVID_TYPE_IVOP || rc->overflow > 0) {
		double frametype_factor;
		double framesize_factor;

		/* Take only the desired part of overflow */
		overflow = rc->overflow;

		/* Factor that will take care to decrease the overflow applied
		 * according to the importance of this frame type in term of
		 * overall size */
		frametype_factor  = rc->count[XVID_TYPE_IVOP-1]*rc->avg_length[XVID_TYPE_IVOP-1];
		frametype_factor += rc->count[XVID_TYPE_PVOP-1]*rc->avg_length[XVID_TYPE_PVOP-1];
		frametype_factor += rc->count[XVID_TYPE_BVOP-1]*rc->avg_length[XVID_TYPE_BVOP-1];
		frametype_factor /= rc->count[s->type-1]*rc->avg_length[s->type-1];
		frametype_factor  = 1/frametype_factor;

		/* Factor that will take care not to compensate too much for this frame
		 * size */
		framesize_factor  = dbytes;
		framesize_factor /= rc->avg_length[s->type-1];

		/* Treat only the overflow part concerned by this frame type and size */
		overflow *= frametype_factor;
#if 0
		/* Leave this one alone, as it impacts badly on quality */
		overflow *= framesize_factor;
#endif

		/* Apply the overflow strength imposed by the user */
		overflow *= (rc->param.overflow_control_strength/100.0f);
	} else {
		/* no negative overflow applied in IFrames because:
		 *  - their role is important as they're references for P/BFrames.
		 *  - there aren't much in typical sequences, so if an IFrame overflows too
		 *    much, this overflow may impact the next IFrame too much and generate
		 *    a sequence of poor quality frames */
		overflow = 0;
	}

	/* Make sure we are not trying to compensate more overflow than we even have */
	if (fabs(overflow) > fabs(rc->overflow))
		overflow = rc->overflow;

	/* Make sure the overflow doesn't make the frame size to get out of the range
	 * [-max_degradation..+max_improvment] */
	if (overflow > dbytes*rc->param.max_overflow_improvement / 100) {
		if(overflow <= dbytes)
			dbytes += dbytes * rc->param.max_overflow_improvement / 100;
		else
			dbytes += overflow * rc->param.max_overflow_improvement / 100;
	} else if (overflow < - dbytes * rc->param.max_overflow_degradation / 100) {
		dbytes -= dbytes * rc->param.max_overflow_degradation / 100;
	} else {
		dbytes += overflow;
	}

	/*-------------------------------------------------------------------------
	 * Frame bit allocation last part:
	 *
	 * Cap frame length so we don't reach neither bigger frame sizes than first
	 * pass nor smaller than the allowed minimum.
	 *-----------------------------------------------------------------------*/

#ifdef PASS_SMALLER
	if (dbytes > s->length) {
		dbytes = s->length;
	}
#endif

	/* Prevent stupid desired sizes under logical values */
	if (dbytes < rc->min_length[s->type-1]) {
		dbytes = rc->min_length[s->type-1];
	}

	/*------------------------------------------------------------------------
	 * Desired frame length <-> quantizer mapping
	 *-----------------------------------------------------------------------*/

#ifdef BQUANT_PRESCALE
	/* For bframes we prescale the quantizer to avoid too high quant scaling */
	if(s->type == XVID_TYPE_BVOP) {

		twopass_stat_t *b_ref = s;

		/* Find the reference frame */
		while(b_ref != &rc->stats[0] && b_ref->type == XVID_TYPE_BVOP)
			b_ref--;

		/* Compute the original quant */
		s->quant  = 2*(100*s->quant - data->bquant_offset);
		s->quant += data->bquant_ratio - 1; /* to avoid rounding issues */
		s->quant  = s->quant/data->bquant_ratio - b_ref->quant;
	}
#endif

	/* Don't laugh at this very 'simple' quant<->size relationship, it
	 * proves to be acurate enough for our algorithm */
	scaled_quant = (double)s->quant*(double)s->length/(double)dbytes;

#ifdef COMPENSATE_FORMULA
	/* We know xvidcore will apply the bframe formula again, so we compensate
	 * it right now to make sure we would not apply it twice */
	if(s->type == XVID_TYPE_BVOP) {

		twopass_stat_t *b_ref = s;

		/* Find the reference frame */
		while(b_ref != &rc->stats[0] && b_ref->type == XVID_TYPE_BVOP)
			b_ref--;

		/* Compute the quant it would be if the core did not apply the bframe
		 * formula */
		scaled_quant  = 100*scaled_quant - data->bquant_offset;
		scaled_quant += data->bquant_ratio - 1; /* to avoid rouding issues */
		scaled_quant /= data->bquant_ratio;
	}
#endif

	/* Quantizer has been scaled using floating point operations/results, we
	 * must cast it to integer */
	data->quant = (int)scaled_quant;

	/* Let's clip the computed quantizer, if needed */
	if (data->quant < 1) {
		data->quant = 1;
	} else if (data->quant > 31) {
		data->quant = 31;
	} else {

		/* The frame quantizer has not been clipped, this appears to be a good
		 * computed quantizer, do not loose quantizer decimal part that we
		 * accumulate for later reuse when its sum represents a complete
		 * unit. */
		rc->quant_error[s->type-1][data->quant] += scaled_quant - (double)data->quant;

		if (rc->quant_error[s->type-1][data->quant] >= 1.0) {
			rc->quant_error[s->type-1][data->quant] -= 1.0;
			data->quant++;
		} else if (rc->quant_error[s->type-1][data->quant] <= -1.0) {
			rc->quant_error[s->type-1][data->quant] += 1.0;
			data->quant--;
		}
	}

	/* Now we have a computed quant that is in the right quante range, with a
	 * possible +1 correction due to cumulated error. We can now safely clip
	 * the quantizer again with user's quant ranges. "Safely" means the Rate
	 * Control could learn more about this quantizer, this knowledge is useful
	 * for future frames even if it this quantizer won't be really used atm,
	 * that's why we don't perform this clipping earlier. */
	if (data->quant < data->min_quant[s->type-1]) {
		data->quant = data->min_quant[s->type-1];
	} else if (data->quant > data->max_quant[s->type-1]) {
		data->quant = data->max_quant[s->type-1];
	}

	if (data->quant < rc->min_quant) data->quant = rc->min_quant;

	/* To avoid big quality jumps from frame to frame, we apply a "security"
	 * rule that makes |last_quant - new_quant| <= 2. This rule only applies
	 * to predicted frames (P and B) */
	if (s->type != XVID_TYPE_IVOP && rc->last_quant[s->type-1] && capped_to_max_framesize == 0) {

		if (data->quant > rc->last_quant[s->type-1] + 2) {
			data->quant = rc->last_quant[s->type-1] + 2;
			DPRINTF(XVID_DEBUG_RC,
					"[xvid rc] -- frame %d p/b-frame quantizer prevented from rising too steeply\n",
					data->frame_num);
		}
		if (data->quant < rc->last_quant[s->type-1] - 2) {
			data->quant = rc->last_quant[s->type-1] - 2;
			DPRINTF(XVID_DEBUG_RC,
					"[xvid rc] -- frame:%d p/b-frame quantizer prevented from falling too steeply\n",
					data->frame_num);
		}
	}

	/* We don't want to pollute the RC histerisis when our computed quant has
	 * been computed from a capped frame size */
	if (capped_to_max_framesize == 0)
		rc->last_quant[s->type-1] = data->quant;

	/* Don't forget to force 1st pass frame type ;-) */
	data->type = s->type;

	return 0;
}

/*----------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

static int
rc_2pass2_after(rc_2pass2_t * rc, xvid_plg_data_t * data)
{
	const char frame_type[4] = { 'i', 'p', 'b', 's'};
	twopass_stat_t * s = &rc->stats[data->frame_num];

	/* Insufficent stats data */
	if (data->frame_num >= rc->num_frames)
		return 0;

	/* Update the quantizer counter */
	rc->quant_count[s->type-1][data->quant]++;

	/* Update the frame type overflow */
	if (data->type == XVID_TYPE_IVOP) {
		int kfdiff = 0;

		if(rc->KF_idx != rc->num_frames -1) {
			kfdiff  = rc->keyframe_locations[rc->KF_idx+1];
			kfdiff -= rc->keyframe_locations[rc->KF_idx];
		}

		/* Flush Keyframe overflow accumulator */
		rc->overflow += rc->KFoverflow;

		/* Store the frame overflow to the keyframe accumulator */
		rc->KFoverflow = s->desired_length - data->length;

		if (kfdiff > 1) {
			/* Non-consecutive keyframes case:
			 * We can then divide this total keyframe overflow into equal parts
			 * that we will distribute into regular overflow at each frame
			 * between the sequence bounded by two IFrames */
			rc->KFoverflow_partial = rc->KFoverflow / (kfdiff - 1);
		} else {
			/* Consecutive keyframes case:
			 * Flush immediatly the keyframe overflow and reset keyframe
			 * overflow */
			rc->overflow += rc->KFoverflow;
			rc->KFoverflow = 0;
			rc->KFoverflow_partial = 0;
		}
		rc->KF_idx++;
	} else {
		/* Accumulate the frame overflow */
		rc->overflow += s->desired_length - data->length;

		/* Distribute part of the keyframe overflow */
		rc->overflow += rc->KFoverflow_partial;

		/* Don't forget to substract that same amount from the total keyframe
		 * overflow */
		rc->KFoverflow -= rc->KFoverflow_partial;
	}

	s->error = s->desired_length - data->length;
	rc->real_total += data->length;

	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- frame:%d type:%c quant:%d stats:%d scaled:%d desired:%d actual:%d error:%d overflow:%.2f\n",
			data->frame_num,
			frame_type[data->type-1],
			data->quant,
			s->length,
			s->scaled_length,
			s->desired_length,
			s->desired_length - s->error,
			-s->error,
			rc->overflow);

	return(0);
}

/*****************************************************************************
 * Helper functions definition
 ****************************************************************************/

/* Default buffer size for reading lines */
#define BUF_SZ   1024

/* Helper functions for reading/parsing the stats file */
static char *skipspaces(char *string);
static int iscomment(char *string);
static char *readline(FILE *f);

/* This function counts the number of frame entries in the stats file
 * It also counts the number of I Frames */
static int
statsfile_count_frames(rc_2pass2_t * rc, char * filename)
{
	FILE * f;
	char *line;
	int lines;

	rc->num_frames = 0;
	rc->num_keyframes = 0;

	if ((f = fopen(filename, "rb")) == NULL)
		return(-1);

	lines = 0;
	while ((line = readline(f)) != NULL) {

		char *ptr;
		char type;
		int fields;

		lines++;

		/* We skip spaces */
		ptr = skipspaces(line);

		/* Skip coment lines or empty lines */
		if(iscomment(ptr) || *ptr == '\0') {
			free(line);
			continue;
		}

		/* Read the stat line from buffer */
		fields = sscanf(ptr, "%c", &type);

		/* Valid stats files have at least 7 fields */
		if (fields == 1) {
			switch(type) {
			case 'i':
			case 'I':
				rc->num_keyframes++;
			case 'p':
			case 'P':
			case 'b':
			case 'B':
			case 's':
			case 'S':
				rc->num_frames++;
				break;
			default:
				DPRINTF(XVID_DEBUG_RC,
						"[xvid rc] -- WARNING: L%d unknown frame type used (%c).\n",
						lines, type);
			}
		} else {
				DPRINTF(XVID_DEBUG_RC,
						"[xvid rc] -- WARNING: L%d misses some stat fields (%d).\n",
						lines, 7-fields);
		}

		/* Free the line buffer */
		free(line);
	}

	/* We are done with the file */
	fclose(f);

	return(0);
}

/* open stats file(s) and read into rc->stats array */
static int
statsfile_load(rc_2pass2_t *rc, char * filename)
{
	FILE * f;
	int processed_entries;

	/* Opens the file */
	if ((f = fopen(filename, "rb"))==NULL)
		return(-1);

	processed_entries = 0;
	while(processed_entries < rc->num_frames) {
		char type;
		int fields;
		twopass_stat_t * s = &rc->stats[processed_entries];
		char *line, *ptr;

		/* Read the line from the file */
		if((line = readline(f)) == NULL)
			break;

		/* We skip spaces */
		ptr = skipspaces(line);

		/* Skip comment lines or empty lines */
		if(iscomment(ptr) || *ptr == '\0') {
			free(line);
			continue;
		}

		/* Reset this field that is optional */
		s->scaled_length = 0;

		/* Convert the fields */
		fields = sscanf(ptr,
						"%c %d %d %d %d %d %d %d\n",
						&type,
						&s->quant,
						&s->blks[0], &s->blks[1], &s->blks[2],
						&s->length, &s->invariant /* not really yet */,
						&s->scaled_length);

		/* Free line buffer, we don't need it anymore */
		free(line);

		/* Fail silently, this has probably been warned in
		 * statsfile_count_frames */
		if(fields != 7 && fields != 8)
			continue;

		/* Convert frame type and compute the invariant length part */
		switch(type) {
		case 'i':
		case 'I':
			s->type = XVID_TYPE_IVOP;
			s->invariant /= INVARIANT_HEADER_PART_IVOP;
			break;
		case 'p':
		case 'P':
		case 's':
		case 'S':
			s->type = XVID_TYPE_PVOP;
			s->invariant /= INVARIANT_HEADER_PART_PVOP;
			break;
		case 'b':
		case 'B':
			s->type = XVID_TYPE_BVOP;
			s->invariant /= INVARIANT_HEADER_PART_BVOP;
			break;
		default:
			/* Same as before, fail silently */
			continue;
		}

		/* Ok it seems it's been processed correctly */
		processed_entries++;
	}

	/* Close the file */
	fclose(f);

	return(0);
}

/* pre-process the statistics data
 * - for each type, count, tot_length, min_length, max_length
 * - set keyframes_locations, tot_prescaled */
static void
first_pass_stats_prepare_data(rc_2pass2_t * rc)
{
	int i,j;

	/* *rc fields initialization
	 * NB: INT_MAX and INT_MIN are used in order to be immediately replaced
	 *     with real values of the 1pass */
	for (i=0; i<3; i++) {
		rc->count[i]=0;
		rc->tot_length[i] = 0;
		rc->tot_invariant[i] = 0;
		rc->min_length[i] = INT_MAX;
	}

	rc->max_length = INT_MIN;
	rc->tot_weighted = 0;

	/* Loop through all frames and find/compute all the stuff this function
	 * is supposed to do */
	for (i=j=0; i<rc->num_frames; i++) {
		twopass_stat_t * s = &rc->stats[i];

		rc->count[s->type-1]++;
		rc->tot_length[s->type-1] += s->length;
		rc->tot_invariant[s->type-1] += s->invariant;
		if (s->zone_mode != XVID_ZONE_QUANT)
			rc->tot_weighted += (int)(s->weight*(s->length - s->invariant));

		if (s->length < rc->min_length[s->type-1]) {
			rc->min_length[s->type-1] = s->length;
		}

		if (s->length > rc->max_length) {
			rc->max_length = s->length;
		}

		if (s->type == XVID_TYPE_IVOP) {
			rc->keyframe_locations[j] = i;
			j++;
		}
	}

	/* NB:
	 * The "per sequence" overflow system considers a natural sequence to be
	 * formed by all frames between two iframes, so if we want to make sure
	 * the system does not go nuts during last sequence, we force the last
	 * frame to appear in the keyframe locations array. */
	rc->keyframe_locations[j] = i;

	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Min 1st pass IFrame length: %d\n", rc->min_length[0]);
	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Min 1st pass PFrame length: %d\n", rc->min_length[1]);
	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Min 1st pass BFrame length: %d\n", rc->min_length[2]);
}

/* calculate zone weight "center" */
static void
zone_process(rc_2pass2_t *rc, const xvid_plg_create_t * create)
{
	int i,j;
	int n = 0;

	rc->tot_quant = 0;
	rc->tot_quant_invariant = 0;

	if (create->num_zones == 0) {
		for (j = 0; j < rc->num_frames; j++) {
			rc->stats[j].zone_mode = XVID_ZONE_WEIGHT;
			rc->stats[j].weight = 1.0;
		}
		n += rc->num_frames;
	}


	for(i=0; i < create->num_zones; i++) {

		int next = (i+1<create->num_zones) ? create->zones[i+1].frame : rc->num_frames;

		/* Zero weight make no sense */
		if (create->zones[i].increment == 0) create->zones[i].increment = 1;
		/* And obviously an undetermined infinite makes even less sense */
		if (create->zones[i].base == 0) create->zones[i].base = 1;

		if (i==0 && create->zones[i].frame > 0) {
			for (j = 0; j < create->zones[i].frame && j < rc->num_frames; j++) {
				rc->stats[j].zone_mode = XVID_ZONE_WEIGHT;
				rc->stats[j].weight = 1.0;
			}
			n += create->zones[i].frame;
		}

		if (create->zones[i].mode == XVID_ZONE_WEIGHT) {
			for (j = create->zones[i].frame; j < next && j < rc->num_frames; j++ ) {
				rc->stats[j].zone_mode = XVID_ZONE_WEIGHT;
				rc->stats[j].weight = (double)create->zones[i].increment / (double)create->zones[i].base;
			}
			next -= create->zones[i].frame;
			n += next;
		} else{  /* XVID_ZONE_QUANT */
			for (j = create->zones[i].frame; j < next && j < rc->num_frames; j++ ) {
				rc->stats[j].zone_mode = XVID_ZONE_QUANT;
				rc->stats[j].weight = (double)create->zones[i].increment / (double)create->zones[i].base;
				rc->tot_quant += rc->stats[j].length;
				rc->tot_quant_invariant += rc->stats[j].invariant;
			}
		}
	}
}


/* scale the curve */
static void
first_pass_scale_curve_internal(rc_2pass2_t *rc)
{
	int64_t target;
	int64_t total_invariant;
	double scaler;
	int i, num_MBs;

	/* We only scale texture data ! */
	total_invariant	 = rc->tot_invariant[XVID_TYPE_IVOP-1];
	total_invariant += rc->tot_invariant[XVID_TYPE_PVOP-1];
	total_invariant += rc->tot_invariant[XVID_TYPE_BVOP-1];
	/* don't forget to substract header bytes used in quant zones, otherwise we
	 * counting them twice */
	total_invariant -= rc->tot_quant_invariant;

	/* We remove the bytes used by the fixed quantizer zones during first pass
	 * with the same quants, so we know very precisely how much that
	 * represents */
	target	= rc->target;
	target -= rc->tot_quant;

	/* Let's compute a linear scaler in order to perform curve scaling */
	scaler = (double)(target - total_invariant) / (double)(rc->tot_weighted);

#ifdef SMART_OVERFLOW_SETTING
	if (scaler > 0.9) {
		rc->param.max_overflow_degradation *= 5;
		rc->param.max_overflow_improvement *= 5;
		rc->param.overflow_control_strength *= 3;
	} else if (scaler > 0.6) {
		rc->param.max_overflow_degradation *= 2;
		rc->param.max_overflow_improvement *= 2;
		rc->param.overflow_control_strength *= 2;
	} else {
		rc->min_quant = 2;
	}
#endif

	/* Compute min frame lengths (for each frame type) according to the number
	 * of MBs. We sum all block type counters of frame 0, this gives us the
	 * number of MBs.
	 *
	 * We compare these hardcoded values with observed values in first pass
	 * (determined in pre_process0).Then we keep the real minimum. */

	/* Number of MBs */
	num_MBs  = rc->stats[0].blks[0];
	num_MBs += rc->stats[0].blks[1];
	num_MBs += rc->stats[0].blks[2];

	/* Minimum for I frames */
	if(rc->min_length[XVID_TYPE_IVOP-1] > ((num_MBs*22) + 240) / 8)
		rc->min_length[XVID_TYPE_IVOP-1] = ((num_MBs*22) + 240) / 8;

	/* Minimum for P/S frames */
	if(rc->min_length[XVID_TYPE_PVOP-1] > ((num_MBs) + 88)  / 8)
		rc->min_length[XVID_TYPE_PVOP-1] = ((num_MBs) + 88)  / 8;

	/* Minimum for B frames */
	if(rc->min_length[XVID_TYPE_BVOP-1] > 8)
		rc->min_length[XVID_TYPE_BVOP-1] = 8;

	/* Perform an initial scale pass.
	 *
	 * If a frame size is scaled underneath our hardcoded minimums, then we
	 * force the frame size to the minimum, and deduct the original & scaled
	 * frame length from the original and target total lengths */
	for (i=0; i<rc->num_frames; i++) {
		twopass_stat_t * s = &rc->stats[i];
		int len;

		/* No need to scale frame length for which a specific quantizer is
		 * specified thanks to zones */
		if (s->zone_mode == XVID_ZONE_QUANT) {
			s->scaled_length = s->length;
			continue;
		}

		/* Compute the scaled length -- only non invariant data length is scaled */
		len = s->invariant + (int)((double)(s->length-s->invariant) * scaler * s->weight);

		/* Compare with the computed minimum */
		if (len < rc->min_length[s->type-1]) {
			/* This is a 'forced size' frame, set its frame size to the
			 * computed minimum */
			s->scaled_length = rc->min_length[s->type-1];

			/* Remove both scaled and original size from their respective
			 * total counters, as we prepare a second pass for 'regular'
			 * frames */
			target -= s->scaled_length;
		} else {
			/* Do nothing for now, we'll scale this later */
			s->scaled_length = 0;
		}
	}

	/* The first pass on data substracted all 'forced size' frames from the
	 * total counters. Now, it's possible to scale the 'regular' frames. */

	/* Scaling factor for 'regular' frames */
	scaler = (double)(target - total_invariant) / (double)(rc->tot_weighted);

	/* Do another pass with the new scaler */
	for (i=0; i<rc->num_frames; i++) {
		twopass_stat_t * s = &rc->stats[i];

		/* Ignore frame with forced frame sizes */
		if (s->scaled_length == 0)
			s->scaled_length = s->invariant + (int)((double)(s->length-s->invariant) * scaler * s->weight);
	}

	/* Job done */
	return;
}

/* Apply all user settings to the scaled curve
 * This implies:
 *   keyframe boosting
 *   high/low compression */
static void
scaled_curve_apply_advanced_parameters(rc_2pass2_t * rc)
{
	int i;
	int64_t ivop_boost_total;

	/* Reset the rate controller (per frame type) total byte counters */
	for (i=0; i<3; i++) rc->tot_scaled_length[i] = 0;

	/* Compute total bytes for each frame type */
	for (i=0; i<rc->num_frames;i++) {
		twopass_stat_t *s = &rc->stats[i];
		rc->tot_scaled_length[s->type-1] += s->scaled_length;
	}

	/* First we compute the total amount of bits needed, as being described by
	 * the scaled distribution. During this pass over the complete stats data,
	 * we see how much bits two user settings will get/give from/to p&b frames:
	 *  - keyframe boosting
	 *  - keyframe distance penalty */
	rc->KF_idx = 0;
	ivop_boost_total = 0;
	for (i=0; i<rc->num_frames; i++) {
		twopass_stat_t * s = &rc->stats[i];

		/* Some more work is needed for I frames */
		if (s->type == XVID_TYPE_IVOP) {
			int ivop_boost;

			/* Accumulate bytes needed for keyframe boosting */
			ivop_boost = s->scaled_length*rc->param.keyframe_boost/100;

#if 0 /* ToDo: decide how to apply kfthresholding */
#endif
			/* If the frame size drops under the minimum length, then cap ivop_boost */
			if (ivop_boost + s->scaled_length < rc->min_length[XVID_TYPE_IVOP-1])
				ivop_boost = rc->min_length[XVID_TYPE_IVOP-1] - s->scaled_length;

			/* Accumulate the ivop boost */
			ivop_boost_total += ivop_boost;

			/* Don't forget to update the keyframe index */
			rc->KF_idx++;
		}
	}

	/* Initialize the IBoost tax ratio for P/S/B frames
	 *
	 * This ratio has to be applied to p/b/s frames in order to reserve
	 * additional bits for keyframes (keyframe boosting) or if too much
	 * keyframe distance is applied, bits retrieved from the keyframes.
	 *
	 * ie pb_length *= rc->pb_iboost_tax_ratio;
	 *
	 *    gives the ideal length of a p/b frame */

	/* Compute the total length of p/b/s frames (temporary storage into
	 * movie_curve) */
	rc->pb_iboost_tax_ratio  = (double)rc->tot_scaled_length[XVID_TYPE_PVOP-1];
	rc->pb_iboost_tax_ratio += (double)rc->tot_scaled_length[XVID_TYPE_BVOP-1];

	/* Compute the ratio described above
	 *     taxed_total = sum(0, n, tax*scaled_length)
	 * <=> taxed_total = tax.sum(0, n, scaled_length)
	 * <=> tax = taxed_total / original_total */
	rc->pb_iboost_tax_ratio =
		(rc->pb_iboost_tax_ratio - ivop_boost_total) /
		rc->pb_iboost_tax_ratio;

	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- IFrame boost tax ratio:%.2f\n",
			rc->pb_iboost_tax_ratio);

	/* Compute the average size of frames per frame type */
	for(i=0; i<3; i++) {
		/* Special case for missing type or weird case */
		if (rc->count[i] == 0 || rc->pb_iboost_tax_ratio == 0) {
			rc->avg_length[i] = 1;
		} else {
			rc->avg_length[i] = rc->tot_scaled_length[i];

			if (i == (XVID_TYPE_IVOP-1)) {
				/* I Frames total has to be added the boost total */
				rc->avg_length[i] += ivop_boost_total;
			} else {
				/* P/B frames has to taxed */
				rc->avg_length[i] *= rc->pb_iboost_tax_ratio;
			}

			/* Finally compute the average frame size */
			rc->avg_length[i] /= (double)rc->count[i];
		}
	}

	/* Assymetric curve compression */
	if (rc->param.curve_compression_high || rc->param.curve_compression_low) {
		double symetric_total;
		double assymetric_delta_total;

		/* Like I frame boosting, assymetric curve compression modifies the total
		 * amount of needed bits, we must compute the ratio so we can prescale
		 lengths */
		symetric_total = 0;
		assymetric_delta_total = 0;
		for (i=0; i<rc->num_frames; i++) {
			double assymetric_delta;
			double dbytes;
			twopass_stat_t * s = &rc->stats[i];

			/* I Frames are not concerned by assymetric scaling */
			if (s->type == XVID_TYPE_IVOP)
				continue;

			/* During the real run, we would have to apply the iboost tax */
			dbytes = s->scaled_length * rc->pb_iboost_tax_ratio;

			/* Update the symmetric curve compression total */
			symetric_total += dbytes;

			/* Apply assymetric curve compression */
			if (dbytes > rc->avg_length[s->type-1])
				assymetric_delta = (rc->avg_length[s->type-1] - dbytes) * (double)rc->param.curve_compression_high / 100.0f;
			else
				assymetric_delta = (rc->avg_length[s->type-1] - dbytes) * (double)rc->param.curve_compression_low  / 100.0f;

			/* Cap to the minimum frame size if needed */
			if (dbytes + assymetric_delta < rc->min_length[s->type-1])
				assymetric_delta = rc->min_length[s->type-1] - dbytes;

			/* Accumulate after assymetric curve compression */
			assymetric_delta_total += assymetric_delta;
		}

		/* Compute the tax that all p/b frames have to pay in order to respect the
		 * bit distribution changes that the assymetric compression curve imposes
		 * We want assymetric_total = sum(0, n-1, tax.scaled_length)
		 *      ie assymetric_total = ratio.sum(0, n-1, scaled_length)
		 *         ratio = assymetric_total / symmetric_total */
		rc->assymetric_tax_ratio = ((double)symetric_total - (double)assymetric_delta_total) / (double)symetric_total;
	} else {
		rc->assymetric_tax_ratio = 1.0f;
	}

	DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- Assymetric tax ratio:%.2f\n", rc->assymetric_tax_ratio);

	/* Last bits that need to be reset */
	rc->overflow = 0;
	rc->KFoverflow = 0;
	rc->KFoverflow_partial = 0;
	rc->KF_idx = 0;
	rc->desired_total = 0;
	rc->real_total = 0;

	/* Job done */
	return;
}

/*****************************************************************************
 * VBV compliancy check and scale
 * MPEG-4 standard specifies certain restrictions for bitrate/framesize in VBR
 * to enable playback on devices with limited readspeed and memory (and which
 * aren't...)
 *
 * DivX profiles have 2 criteria: VBV as in MPEG standard
 *                                a limit on peak bitrate for any 3 seconds
 *
 * But if VBV is fulfilled, peakrate is automatically fulfilled in any profile
 * define so far, so we check for it (for completeness) but correct only VBV
 *
 *****************************************************************************/

#define VBV_COMPLIANT 0
#define VBV_UNDERFLOW 1 /* video buffer runs empty */
#define VBV_OVERFLOW 2  /* doesn't exist for VBR encoding */
#define VBV_PEAKRATE 4  /* peak bitrate (within 3s) violated */

static int
check_curve_for_vbv_compliancy(rc_2pass2_t * rc, const float fps)
{
	/* We do all calculations in float, for higher accuracy,
	 * and in bytes for convenience.
	 *
	 * typical values from DivX Home Theater profile:
	 *  vbv_size= 384*1024 (384kB)
	 *  vbv_initial= 288*1024 (75% fill)
	 *  maxrate= 4854000 (4.854MBps)
	 *  peakrate= 8000000 (8MBps)
	 *
	 *  PAL: offset3s = 75 (3 seconds of 25fps)
	 *  NTSC: offset3s = 90 (3 seconds of 29.97fps) or 72 (3 seconds of 23.976fps)
	 */

	const float vbv_size = (float)rc->param.vbv_size/8.f;
	float vbvfill = (float)rc->param.vbv_initial/8.f;
	float vbvmin;

	const float maxrate = (float)rc->param.vbv_maxrate;
	const float peakrate = (float)rc->param.vbv_peakrate;
	const float r0 = (int)(maxrate/fps+0.5)/8.f;

	int bytes3s = 0;
	int offset3s = (int)(3.f*fps+0.5);
	int i;

	/* 1Gbit should be enough to inuitialize the vbvmin
	 *	an arbitrary high value */
	vbvmin = 1000*1000*1000;

	for (i=0; i<rc->num_frames; i++) {
		/* DivX 3s peak bitrate check  */
		bytes3s += rc->stats[i].scaled_length;
		if (i>=offset3s)
			bytes3s -= rc->stats[i-offset3s].scaled_length;

    /* ignore peakrate constraint if peakrate is <= 0.f */
		if (peakrate>0.f && 8.f*bytes3s > 3*peakrate)
			return(VBV_PEAKRATE);

		/* update vbv fill level */
		vbvfill += r0 - rc->stats[i].scaled_length;

		/* this check is _NOT_ an "overflow"! only reading from disk stops then */
		if (vbvfill > vbv_size)
			vbvfill = vbv_size;

		/* but THIS would be an underflow. report it! */
		if (vbvfill < 0)
			return(VBV_UNDERFLOW);

		/* Store the minimum buffer filling */
		if (vbvfill < vbvmin)
			vbvmin = vbvfill;
	}

	DPRINTF(XVID_DEBUG_RC, "[xvid rc] Minimum buffer fill: %f bytes\n", vbvmin);

	return(VBV_COMPLIANT);
}


static int
scale_curve_for_vbv_compliancy(rc_2pass2_t * rc, const float fps)
{
	/* correct any VBV violations. Peak bitrate violations disappears
	 * by this automatically
	 *
	 * This implementation follows
	 *
	 * Westerink, Rajagopalan, Gonzales "Two-pass MPEG-2 variable-bitrate encoding"
	 * IBM J. RES. DEVELOP. VOL 43, No. 4, July 1999, p.471--488
	 *
	 * Thanks, guys! This paper rocks!!! */

	/* For each scene of len N, we have to check up to N^2 possible buffer fills.
	 * This works well with MPEG-2 where N==12 or so, but for MPEG-4 it's a
	 * little slow...
	 *
	 * TODO: Better control on VBVfill between scenes */

	const float vbv_size = (float)rc->param.vbv_size/8.f;
	const float vbv_initial = (float)rc->param.vbv_initial/8.f;

	const float maxrate = 0.9*rc->param.vbv_maxrate;
	const float vbv_low = 0.10f*vbv_size;
	const float r0 = (int)(maxrate/fps+0.5)/8.f;

	int i,k,l,n,violation = 0;
	float *scenefactor;
	int *scenestart;
	int *scenelength;

	/* first step: determine how many "scenes" there are and store their
	 * boundaries we could get all this from existing keyframe_positions,
	 * somehow, but there we don't have a min_scenelength, and it's no big
	 * deal to get it again. */

	const int min_scenelength = (int)(fps+0.5);
	int num_scenes = 0;
	int last_scene = -999;
	for (i=0; i<rc->num_frames; i++) {
		if ((rc->stats[i].type == XVID_TYPE_IVOP) && (i-last_scene>min_scenelength)) {
			last_scene = i;
			num_scenes++;
		}
	}

	scenefactor = (float*)malloc(num_scenes*sizeof(float));
	scenestart = (int*)malloc(num_scenes*sizeof(int));
	scenelength = (int*)malloc(num_scenes*sizeof(int));

	if ((!scenefactor) || (!scenestart) || (!scenelength) ) {
		free(scenefactor);
		free(scenestart);
		free(scenelength);
		/* remember: free(0) is valid and does exactly nothing. */
		return(-1);
	}

	/* count again and safe the length/position */

	num_scenes = 0;
	last_scene = -999;
	for (i=0; i<rc->num_frames; i++) {
		if ((rc->stats[i].type == XVID_TYPE_IVOP) && (i-last_scene>min_scenelength)) {
			if (num_scenes>0) {
				scenelength[num_scenes-1]=i-last_scene;
			}
			scenestart[num_scenes]=i;
			num_scenes++;
			last_scene = i;
		}
	}
	scenelength[num_scenes-1]=i-last_scene;

	/* second step: check for each scene, how much we can scale its frames up or
	 * down such that the VBV restriction is just fulfilled */
#define R(k,n) (((n)+1-(k))*r0)     /* how much enters the buffer between frame k and n */
	for (l=0; l<num_scenes;l++) {
		const int start = scenestart[l];
		const int length = scenelength[l];
		twopass_stat_t * frames = &rc->stats[start];

		float S0n,Skn;
		float f,minf = 99999.f;

		S0n=0.;
		for (n=0;n<=length-1;n++) {
			S0n += frames[n].scaled_length;

			k = 0;
			Skn = S0n;
			f = (R(k,n-1) + (vbv_initial - vbv_low)) / Skn;
			if (f < minf)
				minf = f;

			for (k=1;k<=n;k++) {
				Skn -= frames[k].scaled_length;

				f = (R(k,n-1) + (vbv_size - vbv_low)) / Skn;
				if (f < minf)
					minf = f;
			}
		}

		/* special case: at the end, fill buffer up to vbv_initial again
		 *
		 * TODO: Allow other values for buffer fill between scenes
		 * e.g. if n=N is smallest f-value, then check for better value */

		n=length;
		k=0;
		Skn = S0n;
		f = R(k,n-1)/Skn;
		if (f < minf)
			minf = f;

		for (k=1;k<=n-1;k++) {
			Skn -= frames[k].scaled_length;

			f = (R(k,n-1) + (vbv_initial - vbv_low)) / Skn;
			if (f < minf)
				minf = f;
		}

		DPRINTF(XVID_DEBUG_RC, "[xvid rc] Scene %d (Frames %d-%d): VBVfactor %f\n",
			   	l, start, start+length-1 , minf);

		scenefactor[l] = minf;
	}
#undef R

	/* last step: now we know of any scene how much it can be scaled up or down
	 * without violating VBV. Next, distribute bits from the evil scenes to the
	 * good ones */
	do {
		float S_red = 0.f;    /* how much to redistribute */
		float S_elig = 0.f;   /* sum of bit for those scenes you can still swallow something*/
		float f_red;
		int l;

		/* check how much is wrong */
		for (l=0;l<num_scenes;l++) {
			const int start = scenestart[l];
			const int length = scenelength[l];
			twopass_stat_t * frames = &rc->stats[start];

			/* exactly 1 means "don't touch this anymore!" */
			if (scenefactor[l] == 1.)
				continue;

			/* within limits */
			if (scenefactor[l] > 1.) {
				for (n= 0; n < length; n++)
					S_elig += frames[n].scaled_length;
			} else {
				/* underflowing segment */
				for (n= 0; n < length; n++) {
					float newbytes = (float)frames[n].scaled_length * scenefactor[l];
					S_red += (float)frames[n].scaled_length - (float)newbytes;
					frames[n].scaled_length =(int)newbytes;
				}
				scenefactor[l] = 1.f;
			}
		}

		/* no more underflows */
		if (S_red < 1.f)
			break;

		if (S_elig < 1.f) {
			DPRINTF(XVID_DEBUG_RC, "[xvid rc] Everything underflowing.\n");
			free(scenefactor);
			free(scenestart);
			free(scenelength);
			return(-2);
		}

		f_red = (1.f + S_red/S_elig);

		DPRINTF(XVID_DEBUG_RC, "[xvid rc] Moving %.0f kB to avoid buffer underflow, correction factor: %.5f\n",
			   	S_red/1024.f, f_red);

		violation=0;
		/* scale remaining scenes up to meet total size */
		for (l=0; l<num_scenes; l++) {
			const int start = scenestart[l];
			const int length = scenelength[l];
			twopass_stat_t * frames = &rc->stats[start];

			if (scenefactor[l] == 1.)
				continue;

			/* there shouldn't be any segments with factor<1 left, so all the rest is >1 */
			for (n= 0; n < length; n++) {
				frames[n].scaled_length = (int)(frames[n].scaled_length * f_red + 0.5);
			}

			scenefactor[l] /= f_red;
			if (scenefactor[l] < 1.f)
				violation=1;
		}

	} while (violation);

	free(scenefactor);
	free(scenestart);
	free(scenelength);
	return(0);
}


/*****************************************************************************
 * Still more low level stuff (nothing to do with stats treatment)
 ****************************************************************************/

/* This function returns an allocated string containing a complete line read
 * from the file starting at the current position */
static char *
readline(FILE *f)
{
	char *buffer = NULL;
	int buffer_size = 0;
	int pos = 0;

	do {
		int c;

		/* Read a character from the stream */
		c = fgetc(f);

		/* Is that EOF or new line ? */
		if(c == EOF || c == '\n')
			break;

		/* Do we have to update buffer ? */
		if(pos >= buffer_size - 1) {
			buffer_size += BUF_SZ;
			buffer = (char*)realloc(buffer, buffer_size);
			if (buffer == NULL)
				return(NULL);
		}

		buffer[pos] = c;
		pos++;
	} while(1);

	/* Read \n or EOF */
	if (buffer == NULL) {
		/* EOF, so we reached the end of the file, return NULL */
		if(feof(f))
			return(NULL);

		/* Just an empty line with just a newline, allocate a 1 byte buffer to
		 * store a zero length string */
		buffer = (char*)malloc(1);
		if(buffer == NULL)
			return(NULL);
	}

	/* Zero terminated string */
	buffer[pos] = '\0';

	return(buffer);
}

/* This function returns a pointer to the first non space char in the given
 * string */
static char *
skipspaces(char *string)
{
	const char spaces[] =
		{
			' ','\t','\0'
		};
	const char *spacechar = spaces;

	if (string == NULL) return(NULL);

	while (*string != '\0') {
		/* Test against space chars */
		while (*spacechar != '\0') {
			if (*string == *spacechar) {
				string++;
				spacechar = spaces;
				break;
			}
			spacechar++;
		}

		/* No space char */
		if (*spacechar == '\0') return(string);
	}

	return(string);
}

/* This function returns a boolean that tells if the string is only a
 * comment */
static int
iscomment(char *string)
{
	const char comments[] =
		{
			'#',';', '%', '\0'
		};
	const char *cmtchar = comments;
	int iscomment = 0;

	if (string == NULL) return(1);

	string = skipspaces(string);

	while(*cmtchar != '\0') {
		if(*string == *cmtchar) {
			iscomment = 1;
			break;
		}
		cmtchar++;
	}

	return(iscomment);
}

#if 0
static void
stats_print(rc_2pass2_t * rc)
{
	int i;
	const char frame_type[4] = { 'i', 'p', 'b', 's'};

	for (i=0; i<rc->num_frames; i++) {
		twopass_stat_t *s = &rc->stats[i];
		DPRINTF(XVID_DEBUG_RC, "[xvid rc] -- frame:%d type:%c quant:%d stats:%d scaled:%d desired:%d actual:%d overflow(%c):%.2f\n",
				i, frame_type[s->type-1], -1, s->length, s->scaled_length,
				s->desired_length, -1, frame_type[s->type-1], -1.0f);
	}
}
#endif
