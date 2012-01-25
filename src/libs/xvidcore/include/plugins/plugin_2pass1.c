/******************************************************************************
 *
 *  XviD Bit Rate Controller Library
 *  - VBR 2 pass bitrate controler implementation -
 *
 *  Copyright (C) 2002-2003 Edouard Gomez <ed.gomez@free.fr>
 *
 *  The curve treatment algorithm is the one implemented by Foxer <email?> and
 *  Dirk Knop <dknop@gwdg.de> for the XviD vfw dynamic library.
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
 * $Id: plugin_2pass1.c,v 1.3 2004/07/18 12:46:44 syskin Exp $
 *
 *****************************************************************************/

#include <stdio.h>
#include <errno.h> /* errno var (or function with recent libc) */
#include <string.h> /* strerror() */

#include "../xvid.h"
#include "../image/image.h"


/* This preprocessor constant controls wheteher or not, first pass is done
 * using fast ME routines to speed up the 2pass process at the expense of
 * less precise first pass stats */
#define FAST1PASS
#define FAST1PASS_QPEL_TOO


/* context struct */
typedef struct
{
	FILE * stat_file;

    double fq_error;
} rc_2pass1_t;



static int rc_2pass1_create(xvid_plg_create_t * create, rc_2pass1_t ** handle)
{
    xvid_plugin_2pass1_t * param = (xvid_plugin_2pass1_t *)create->param;
	rc_2pass1_t * rc;

    /* check filename */
    if ((param->filename == NULL) ||
		(param->filename != NULL && param->filename[0] == '\0'))
        return XVID_ERR_FAIL;

    /* allocate context struct */
	if((rc = malloc(sizeof(rc_2pass1_t))) == NULL)
		return(XVID_ERR_MEMORY);

    /* Initialize safe defaults for 2pass 1 */
    rc->stat_file = NULL;

	/* Open the 1st pass file */
	if((rc->stat_file = fopen(param->filename, "w+b")) == NULL)
		return(XVID_ERR_FAIL);

	/* I swear xvidcore isn't buggy, but when using mencoder+xvid4 i observe
	 * this weird bug.
	 *
	 * Symptoms: The stats file grows until it's fclosed, but at this moment
	 *           a large part of the file is filled by 0x00 bytes w/o any
	 *           reasonable cause. The stats file is then completly unusable
	 *
	 * So far, i think i found "the why":
	 *  - take a MPEG stream containing 2 sequences (concatenate 2 MPEG files
	 *    together)
	 *  - Encode this MPEG file
	 *
	 * It should trigger the bug
	 *
	 * I think this is caused by some kind of race condition on mencoder module
	 * start/stop.
	 *  - mencoder encodes the first sequence
	 *    + xvid4 module opens xvid-twopass.stats and writes stats in it.
	 *  - mencoder detects the second sequence and initialize a second
	 *    module and stops the old encoder
	 *    + new xvid4 module opens a new xvid-twopass.stats, old xvid4
	 *      module closes it
	 *
	 * This is IT, got a racing condition.
	 * Unbuffered IO, may help ... */
	setbuf(rc->stat_file, NULL);

	/*
	 * The File Header
	 */
	fprintf(rc->stat_file, "# XviD 2pass stat file (core version %d.%d.%d)\n",
			XVID_VERSION_MAJOR(XVID_VERSION),
			XVID_VERSION_MINOR(XVID_VERSION),
			XVID_VERSION_PATCH(XVID_VERSION));
	fprintf(rc->stat_file, "# Please do not modify this file\n\n");

    rc->fq_error = 0;

    *handle = rc;
	return(0);
}


static int rc_2pass1_destroy(rc_2pass1_t * rc, xvid_plg_destroy_t * destroy)
{
	if (rc->stat_file) {
		if (fclose(rc->stat_file) == EOF) {
			DPRINTF(XVID_DEBUG_RC, "Error closing stats file (%s)", strerror(errno));
		}
	}
	rc->stat_file = NULL; /* Just a paranoid reset */
	free(rc); /* as the container structure is freed anyway */
	return(0);
}


static int rc_2pass1_before(rc_2pass1_t * rc, xvid_plg_data_t * data)
{
	 if (data->quant <= 0) {
		if (data->zone && data->zone->mode == XVID_ZONE_QUANT) {
			/* We disable no options in quant zones, as their implementation is
			 * based on the fact we do first pass exactly the same way as the
			 * second one to have exact zone size */
			rc->fq_error += (double)data->zone->increment / (double)data->zone->base;
			data->quant = (int)rc->fq_error;
			rc->fq_error -= data->quant;
		} else {
			data->quant = 2;

#ifdef FAST1PASS
			/* Given the fact our 2pass algorithm is based on very simple
			 * rules, we can disable some options that are too CPU intensive
			 * and do not provide the 2nd pass any benefit */

			/* First disable some motion flags */
			data->motion_flags &= ~XVID_ME_CHROMA_PVOP;
			data->motion_flags &= ~XVID_ME_CHROMA_BVOP;
			data->motion_flags &= ~XVID_ME_USESQUARES16;
			data->motion_flags &= ~XVID_ME_ADVANCEDDIAMOND16;
			data->motion_flags &= ~XVID_ME_EXTSEARCH16;

			/* And enable fast replacements */
			data->motion_flags |= XVID_ME_FAST_MODEINTERPOLATE;
			data->motion_flags |= XVID_ME_SKIP_DELTASEARCH;
			data->motion_flags |= XVID_ME_FASTREFINE16;
			data->motion_flags |= XVID_ME_BFRAME_EARLYSTOP;

			/* Now VOP flags (no fast replacements) */
			data->vop_flags &= ~XVID_VOP_MODEDECISION_RD;
			data->vop_flags &= ~XVID_VOP_RD_BVOP;
			data->vop_flags &= ~XVID_VOP_FAST_MODEDECISION_RD;
			data->vop_flags &= ~XVID_VOP_TRELLISQUANT;
			data->vop_flags &= ~XVID_VOP_INTER4V;
			data->vop_flags &= ~XVID_VOP_HQACPRED;

			/* Finnaly VOL flags
			 *
			 * NB: Qpel cannot be disable because this option really changes
			 *     too much the texture data compressibility, and thus the
			 *     second pass gets confused by too much impredictability
			 *     of frame sizes, and actually hurts quality */
#ifdef FAST1PASS_QPEL_TOO
			/* or maybe we can disable it after all? */
			data->vol_flags &= ~XVID_VOL_QUARTERPEL;
#endif
			data->vol_flags &= ~XVID_VOL_GMC;
#endif
		}
	}
	 return(0);
}


static int rc_2pass1_after(rc_2pass1_t * rc, xvid_plg_data_t * data)
{
	char type;
	xvid_enc_stats_t *stats = &data->stats;

	/* Frame type in ascii I/P/B */
	switch(stats->type) {
	case XVID_TYPE_IVOP:
		type = 'i';
		break;
	case XVID_TYPE_PVOP:
		type = 'p';
		break;
	case XVID_TYPE_BVOP:
		type = 'b';
		break;
	case XVID_TYPE_SVOP:
		type = 's';
		break;
	default: /* Should not go here */
		return(XVID_ERR_FAIL);
	}

	/* write the resulting statistics */

	fprintf(rc->stat_file, "%c %d %d %d %d %d %d\n",
			type,
			stats->quant,
			stats->kblks,
			stats->mblks,
			stats->ublks,
			stats->length,
			stats->hlength);

	return(0);
}



int xvid_plugin_2pass1(void * handle, int opt, void * param1, void * param2)
{
    switch(opt)
    {
    case XVID_PLG_INFO :
	case XVID_PLG_FRAME :
        return 0;

    case XVID_PLG_CREATE :
        return rc_2pass1_create((xvid_plg_create_t*)param1, param2);

    case XVID_PLG_DESTROY :
        return rc_2pass1_destroy((rc_2pass1_t*)handle, (xvid_plg_destroy_t*)param1);

    case XVID_PLG_BEFORE :
        return rc_2pass1_before((rc_2pass1_t*)handle, (xvid_plg_data_t*)param1);

    case XVID_PLG_AFTER :
        return rc_2pass1_after((rc_2pass1_t*)handle, (xvid_plg_data_t*)param1);
    }

    return XVID_ERR_FAIL;
}
