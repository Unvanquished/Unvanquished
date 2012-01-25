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
#include "c64xdec.h"

#if defined(OC_C64X_ASM)

void oc_dec_accel_init_c64x(oc_dec_ctx *_dec){
# if defined(OC_DEC_USE_VTABLE)
  _dec->opt_vtable.dc_unpredict_mcu_plane=oc_dec_dc_unpredict_mcu_plane_c64x;
# endif
}


/*Undo the DC prediction in a single plane of an MCU (one or two super block
   rows).
  As a side effect, the number of coded and uncoded fragments in this plane of
   the MCU is also computed.*/
void oc_dec_dc_unpredict_mcu_plane_c64x(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe,int _pli){
  const oc_fragment_plane *fplane;
  oc_fragment             *frags;
  int                     *pred_last;
  ptrdiff_t                ncoded_fragis;
  ptrdiff_t                fragi;
  int                      fragx;
  int                      fragy;
  int                      fragy0;
  int                      fragy_end;
  int                      nhfrags;
  /*Compute the first and last fragment row of the current MCU for this
     plane.*/
  fplane=_dec->state.fplanes+_pli;
  fragy0=_pipe->fragy0[_pli];
  fragy_end=_pipe->fragy_end[_pli];
  nhfrags=fplane->nhfrags;
  pred_last=_pipe->pred_last[_pli];
  frags=_dec->state.frags;
  ncoded_fragis=0;
  fragi=fplane->froffset+fragy0*(ptrdiff_t)nhfrags;
  for(fragy=fragy0;fragy<fragy_end;fragy++){
    if(fragy==0){
      /*For the first row, all of the cases reduce to just using the previous
         predictor for the same reference frame.*/
      for(fragx=0;fragx<nhfrags;fragx++,fragi++){
        int coded;
        int refi;
        /*The TI compiler refuses to pipeline this if we put it in an if(coded)
           block.
          We can do the loads unconditionally, which helps move them earlier.
          We do the store unconditionally too, because if we use a conditional
           store, the compiler propagates the condition back to the operations
           the store depended on, presumably to reduce cache pressure by
           eliminating dead loads.
          However, these loads are "free" in the cache sense, since reading the
           coded flag brings in all four bytes anyway, and starting the loads
           before we know the coded flag saves 6 cycles.*/
        refi=frags[fragi].refi;
        coded=frags[fragi].coded;
        frags[fragi].dc=pred_last[refi]+=frags[fragi].dc&-coded;
        ncoded_fragis+=coded;
      }
    }
    else{
      oc_fragment *u_frags;
      int          l_ref;
      int          ul_ref;
      int          u_ref;
      u_frags=frags-nhfrags;
      l_ref=-1;
      ul_ref=-1;
      u_ref=u_frags[fragi].refi;
      for(fragx=0;fragx<nhfrags;fragx++,fragi++){
        int ur_ref;
        int refi;
        if(fragx+1>=nhfrags)ur_ref=-1;
        else ur_ref=u_frags[fragi+1].refi;
        refi=frags[fragi].refi;
        if(frags[fragi].coded){
          static const int OC_PRED_SCALE[16][2]={
            {0x00000000,0x00000000},
            {0x00000000,0x00000080},
            {0x00800000,0x00000000},
            {0x00000000,0x00000080},
            {0x00000080,0x00000000},
            {0x00000040,0x00000040},
            {0x00000080,0x00000000},
            {0xFF980074,0x00000074},
            {0x00000000,0x00800000},
            {0x00000000,0x0035004B},
            {0x00400000,0x00400000},
            {0x00000000,0x0035004B},
            {0x00000080,0x00000000},
            {0x00000000,0x0035004B},
            {0x00180050,0x00180000},
            {0xFF980074,0x00000074},
          };
          ogg_int16_t p0;
          ogg_int16_t p1;
          ogg_int16_t p2;
          ogg_int16_t p3;
          int         pred;
          int         pflags;
          /*29 cycles.*/
          /*HACK: This p0 reference could potentially be out of bounds, but
             because we know what allocator Leonora is using, we know it can't
             segfault.*/
          p0=u_frags[fragi-1].dc;
          p1=u_frags[fragi].dc;
          p2=u_frags[fragi+1].dc;
          p3=frags[fragi-1].dc;
          pflags=_cmpeq4(_packl4(_pack2(ur_ref,u_ref),_pack2(ul_ref,l_ref)),
           _packl4(_pack2(refi,refi),_pack2(refi,refi)));
          if(pflags==0)pred=pred_last[refi];
          else{
            pred=(_dotp2(_pack2(p0,p1),OC_PRED_SCALE[pflags][0])
             +_dotp2(_pack2(p2,p3),OC_PRED_SCALE[pflags][1]))/128;
            if((pflags&7)==7){
              if(abs(pred-p1)>128)pred=p1;
              else if(abs(pred-p3)>128)pred=p3;
              else if(abs(pred-p0)>128)pred=p0;
            }
          }
          pred_last[refi]=frags[fragi].dc+=pred;
          ncoded_fragis++;
          l_ref=refi;
        }
        else l_ref=-1;
        ul_ref=u_ref;
        u_ref=ur_ref;
      }
    }
  }
  _pipe->ncoded_fragis[_pli]=ncoded_fragis;
  /*Also save the number of uncoded fragments so we know how many to copy.*/
  _pipe->nuncoded_fragis[_pli]=
   (fragy_end-fragy0)*(ptrdiff_t)nhfrags-ncoded_fragis;
}

#endif
