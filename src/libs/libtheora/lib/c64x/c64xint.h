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

#if !defined(_c64x_c64xint_H)
# define _c64x_c64xint_H (1)
# include "../internal.h"

# if defined(OC_C64X_ASM)
#  define oc_state_accel_init oc_state_accel_init_c64x
#  define oc_frag_copy(_state,_dst,_src,_ystride) \
  oc_frag_copy_c64x(_dst,_src,_ystride)
#  define oc_frag_copy_list(_state,_dst_frame,_src_frame,_ystride, \
 _fragis,_nfragis,_frag_buf_offs) \
  oc_frag_copy_list_c64x(_dst_frame,_src_frame,_ystride, \
   _fragis,_nfragis,_frag_buf_offs)
#  define oc_frag_recon_intra(_state,_dst,_dst_ystride,_residue) \
  oc_frag_recon_intra_c64x(_dst,_dst_ystride,_residue)
#  define oc_frag_recon_inter(_state,_dst,_src,_ystride,_residue) \
  oc_frag_recon_inter_c64x(_dst,_src,_ystride,_residue)
#  define oc_frag_recon_inter2(_state,_dst,_src1,_src2,_ystride,_residue) \
  oc_frag_recon_inter2_c64x(_dst,_src1,_src2,_ystride,_residue)
#  define oc_idct8x8(_state,_y,_x,_last_zzi) \
  oc_idct8x8_c64x(_y,_x,_last_zzi)
#  define oc_state_frag_recon oc_state_frag_recon_c64x
#  define oc_loop_filter_init(_state,_bv,_flimit) \
  oc_loop_filter_init_c64x(_bv,_flimit)
#  define oc_state_loop_filter_frag_rows oc_state_loop_filter_frag_rows_c64x
#  define oc_restore_fpu(_state) do{}while(0)
# endif

# include "../state.h"

void oc_state_accel_init_c64x(oc_theora_state *_state);

void oc_frag_copy_c64x(unsigned char *_dst,
 const unsigned char *_src,int _ystride);
void oc_frag_copy_list_c64x(unsigned char *_dst_frame,
 const unsigned char *_src_frame,int _ystride,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,const ptrdiff_t *_frag_buf_offs);
void oc_frag_recon_intra_c64x(unsigned char *_dst,int _ystride,
 const ogg_int16_t *_residue);
void oc_frag_recon_inter_c64x(unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter2_c64x(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t *_residue);
void oc_idct8x8_c64x(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi);
void oc_state_frag_recon_c64x(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,ogg_uint16_t _dc_quant);
void oc_loop_filter_init_c64x(signed char _bv[256],int _flimit);
void oc_state_loop_filter_frag_rows_c64x(const oc_theora_state *_state,
 signed char _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end);

#endif
