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

#include "c64xint.h"

#if defined(OC_C64X_ASM)

void oc_state_accel_init_c64x(oc_theora_state *_state){
  oc_state_accel_init_c(_state);
# if defined(OC_STATE_USE_VTABLE)
  _state->opt_vtable.frag_copy=oc_frag_copy_c64x;
  _state->opt_vtable.frag_recon_intra=oc_frag_recon_intra_c64x;
  _state->opt_vtable.frag_recon_inter=oc_frag_recon_inter_c64x;
  _state->opt_vtable.frag_recon_inter2=oc_frag_recon_inter2_c64x;
  _state->opt_vtable.idct8x8=oc_idct8x8_c64x;
  _state->opt_vtable.state_frag_recon=oc_state_frag_recon_c64x;
  _state->opt_vtable.frag_copy_list=oc_frag_copy_list_c64x;
  _state->opt_vtable.loop_filter_init=oc_loop_filter_init_c64x;
  _state->opt_vtable.state_loop_filter_frag_rows=
   oc_state_loop_filter_frag_rows_c64x;
  _state->opt_vtable.restore_fpu=oc_restore_fpu_c;
# endif
}

#endif
