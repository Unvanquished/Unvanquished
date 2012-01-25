/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2007                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id$

 ********************************************************************/
#if !defined(_c64x_c64xdec_H)
# define _c64x_c64xdec_H (1)
# include "c64xint.h"

# if defined(OC_C64X_ASM)
#  define oc_dec_accel_init oc_dec_accel_init_c64x
#  define oc_dec_dc_unpredict_mcu_plane oc_dec_dc_unpredict_mcu_plane_c64x
# endif

# include "../decint.h"

void oc_dec_accel_init_c64x(oc_dec_ctx *_dec);

void oc_dec_dc_unpredict_mcu_plane_c64x(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe,int _pli);

#endif
