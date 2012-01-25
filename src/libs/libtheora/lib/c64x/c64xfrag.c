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
#include <string.h>
#include "c64xint.h"



/*14 cycles.*/
void oc_frag_copy_c64x(unsigned char *restrict _dst,
 const unsigned char *restrict _src,int _ystride){
  unsigned char *restrict       d2;
  const unsigned char *restrict s2;
  d2=_dst+_ystride;
  s2=_src+_ystride;
#define OC_ITER() \
  do{ \
    _amem8(_dst)=_mem8(_src); \
    _dst+=2*_ystride; \
    _src+=2*_ystride; \
    _amem8(d2)=_mem8(s2); \
    d2+=2*_ystride; \
    s2+=2*_ystride; \
  } \
  while(0)
  OC_ITER();
  OC_ITER();
  OC_ITER();
  OC_ITER();
#undef OC_ITER
}

void oc_frag_copy_list_c64x(unsigned char *_dst_frame,
 const unsigned char *_src_frame,int _ystride,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,const ptrdiff_t *_frag_buf_offs){
  ptrdiff_t fragii;
  /*9 cycles per iteration.*/
  for(fragii=0;fragii<_nfragis;fragii++){
    const unsigned char *restrict src;
    const unsigned char *restrict s2;
    unsigned char       *restrict dst;
    unsigned char       *restrict d2;
    ptrdiff_t                     frag_buf_off;
    frag_buf_off=_frag_buf_offs[_fragis[fragii]];
    dst=_dst_frame+frag_buf_off;
    src=_src_frame+frag_buf_off;
    d2=dst+_ystride;
    s2=src+_ystride;
#define OC_ITER() \
  do{ \
    _amem8(dst)=_amem8_const(src); \
    dst+=2*_ystride; \
    src+=2*_ystride; \
    _amem8(d2)=_amem8_const(s2); \
    d2+=2*_ystride; \
    s2+=2*_ystride; \
  } \
  while(0)
    OC_ITER();
    OC_ITER();
    OC_ITER();
    OC_ITER();
#undef OC_ITER
  }
}

/*34 cycles.*/
void oc_frag_recon_intra_c64x(unsigned char *_dst,int _ystride,
 const ogg_int16_t _residue[64]){
  int i;
  for(i=0;i<8;i++){
    long long ll;
    int       x1;
    int       y1;
    int       x2;
    int       y2;
    ll=_amem8_const(_residue+i*8+0);
    x1=_sadd2(_loll(ll),0x00800080);
    y1=_sadd2(_hill(ll),0x00800080);
    ll=_amem8_const(_residue+i*8+4);
    x2=_sadd2(_loll(ll),0x00800080);
    y2=_sadd2(_hill(ll),0x00800080);
    _amem8(_dst)=_itoll(_spacku4(y2,x2),_spacku4(y1,x1));
    _dst+=_ystride;
  }
}

/*41 cycles.*/
void oc_frag_recon_inter_c64x(unsigned char *_dst,const unsigned char *_src,
 int _ystride,const ogg_int16_t _residue[64]){
  int i;
  for(i=0;i<8;i++){
    long long ll;
    int       x1;
    int       y1;
    int       z1;
    int       x2;
    int       y2;
    int       z2;
    ll=_mem8_const(_src);
    z1=_loll(ll);
    z2=_hill(ll);
    ll=_amem8_const(_residue+i*8+0);
    x1=_sadd2(_unpklu4(z1),_loll(ll));
    y1=_sadd2(_unpkhu4(z1),_hill(ll));
    ll=_amem8_const(_residue+i*8+4);
    x2=_sadd2(_unpklu4(z2),_loll(ll));
    y2=_sadd2(_unpkhu4(z2),_hill(ll));
    _amem8(_dst)=_itoll(_spacku4(y2,x2),_spacku4(y1,x1));
    _dst+=_ystride;
    _src+=_ystride;
  }
}

/*56 cycles.*/
void oc_frag_recon_inter2_c64x(unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride,
 const ogg_int16_t _residue[64]){
  int i;
  for(i=0;i<8;i++){
    long long ll;
    int       a;
    int       b;
    int       c;
    int       d;
    int       x1;
    int       y1;
    int       z1;
    int       x2;
    int       y2;
    int       z2;
    ll=_mem8_const(_src1);
    a=_loll(ll);
    b=_hill(ll);
    ll=_mem8_const(_src2);
    c=_loll(ll);
    d=_hill(ll);
    ll=_amem8_const(_residue+i*8+0);
    z1=~_avgu4(~a,~c);
    x1=_sadd2(_unpklu4(z1),_loll(ll));
    y1=_sadd2(_unpkhu4(z1),_hill(ll));
    ll=_amem8_const(_residue+i*8+4);
    z2=~_avgu4(~b,~d);
    x2=_sadd2(_unpklu4(z2),_loll(ll));
    y2=_sadd2(_unpkhu4(z2),_hill(ll));
    _amem8(_dst)=_itoll(_spacku4(y2,x2),_spacku4(y1,x1));
    _dst+=_ystride;
    _src1+=_ystride;
    _src2+=_ystride;
  }
}

void oc_state_frag_recon_c64x(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,ogg_uint16_t _dc_quant){
  unsigned char *dst;
  ptrdiff_t      frag_buf_off;
  int            ystride;
  int            refi;
  /*Apply the inverse transform.*/
  /*Special case only having a DC component.*/
  if(_last_zzi<2){
    int         p;
    long long   ll;
    int         ci;
    /*We round this dequant product (and not any of the others) because there's
       no iDCT rounding.*/
    p=_dct_coeffs[0]*(ogg_int32_t)_dc_quant+15>>5;
    ll=_itoll(_pack2(p,p),_pack2(p,p));
    for(ci=0;ci<64;ci+=4)_amem8(_dct_coeffs+64+ci)=ll;
  }
  else{
    /*First, dequantize the DC coefficient.*/
    _dct_coeffs[0]=(ogg_int16_t)(_dct_coeffs[0]*(int)_dc_quant);
    oc_idct8x8_c64x(_dct_coeffs+64,_dct_coeffs,_last_zzi);
  }
  /*Fill in the target buffer.*/
  frag_buf_off=_state->frag_buf_offs[_fragi];
  refi=_state->frags[_fragi].refi;
  ystride=_state->ref_ystride[_pli];
  dst=_state->ref_frame_data[OC_FRAME_SELF]+frag_buf_off;
  if(refi==OC_FRAME_SELF)oc_frag_recon_intra_c64x(dst,ystride,_dct_coeffs+64);
  else{
    const unsigned char *ref;
    int                  mvoffsets[2];
    ref=_state->ref_frame_data[refi]+frag_buf_off;
    if(oc_state_get_mv_offsets(_state,mvoffsets,_pli,
     _state->frag_mvs[_fragi])>1){
      oc_frag_recon_inter2_c64x(dst,ref+mvoffsets[0],ref+mvoffsets[1],
       ystride,_dct_coeffs+64);
    }
    else oc_frag_recon_inter_c64x(dst,ref+mvoffsets[0],ystride,_dct_coeffs+64);
  }
}

/*46 cycles.*/
static void loop_filter_h(unsigned char *restrict _pix,int _ystride,int _ll){
  int p0;
  int p1;
  int p2;
  int p3;
  int p4;
  int p5;
  int p6;
  int p7;
  int y;
  _pix-=2;
  /*Do all the loads now to avoid the compiler's inability to prove they're not
     dependent on the stores later.*/
  p0=_mem4(_pix+_ystride*0);
  p1=_mem4(_pix+_ystride*1);
  p2=_mem4(_pix+_ystride*2);
  p3=_mem4(_pix+_ystride*3);
  p4=_mem4(_pix+_ystride*4);
  p5=_mem4(_pix+_ystride*5);
  p6=_mem4(_pix+_ystride*6);
  p7=_mem4(_pix+_ystride*7);
  for(y=0;y<8;y+=4){
    int f;
    int a;
    int b;
    int u;
    int v;
    /*We could pack things right after the dot product, but delaying it
       actually saves three cycles due to better instruction scheduling.*/
    a=_dotpsu4(0x01FD03FF,p0)+3>>3;
    b=_dotpsu4(0x01FD03FF,p1)+3>>3;
    u=_dotpsu4(0x01FD03FF,p2)+3>>3;
    v=_dotpsu4(0x01FD03FF,p3)+3>>3;
    f=_packl4(_pack2(v,u),_pack2(b,a));
    /*We split the results by sign and work with abs(f) here, since the C64x
       signed-unsigned addition with unsigned saturation is only available for
       16-bit operands.
      For 8-bit operands, we have to emulate it with a saturated addition and a
       saturated subtraction using separate unsigned values.
      There's no direct support for 8-bit saturated subtraction, either, so we
       have to emulate that as well, using either x-_minu4(x,y) or
       ~_saddu4(~x,y), depending on which one schedules better.*/
    f=_add4(0x80808080,f);
    b=_minu4(0x80808080,f);
    a=0x80808080-b;
    b=f-b;
    /*Compute f=clamp(0,2*L-abs(f),abs(f)).*/
    u=_saddu4(a,_ll);
    v=_saddu4(b,_ll);
    a=_saddu4(a,u);
    b=_saddu4(b,v);
    a=a-_minu4(a,u);
    b=b-_minu4(b,v);
    /*Apply the changes to the original pixels.*/
    u=_pack2(p1>>8,p0>>8);
    v=_pack2(p3>>8,p2>>8);
    p1=_packl4(v,u);
    p2=_packh4(v,u);
    p1=_saddu4(~_saddu4(~p1,b),a);
    p2=_saddu4(p2-_minu4(p2,a),b);
    /*For unaligned short stores, we have to store byte by byte.
      It's faster to do it explicitly than to use _mem2().*/
    _pix[_ystride*0+1]=(unsigned char)p1;
    _pix[_ystride*0+2]=(unsigned char)p2;
    _pix[_ystride*1+1]=(unsigned char)(p1>>8);
    _pix[_ystride*1+2]=(unsigned char)(p2>>8);
    _pix[_ystride*2+1]=(unsigned char)(p1>>16);
    _pix[_ystride*2+2]=(unsigned char)(p2>>16);
    _pix[_ystride*3+1]=(unsigned char)(p1>>24);
    _pix[_ystride*3+2]=(unsigned char)(p2>>24);
    p0=p4;
    p1=p5;
    p2=p6;
    p3=p7;
    _pix+=4*_ystride;
  }
}

/*38 cycles.*/
static void loop_filter_v(unsigned char * restrict _pix,int _ystride,int _ll){
  long long ll;
  int       p0;
  int       p1;
  int       p2;
  int       p3;
  int       p4;
  int       p5;
  int       p6;
  int       p7;
  int       a1;
  int       b1;
  int       f1;
  int       m1;
  int       u1;
  int       v1;
  int       a2;
  int       b2;
  int       f2;
  int       m2;
  int       u2;
  int       v2;
  /*Do all the loads now to avoid the compiler's inability to prove they're not
     dependent on the stores later.*/
  ll=_amem8(_pix-_ystride*2);
  p0=_loll(ll);
  p4=_hill(ll);
  ll=_amem8(_pix-_ystride*1);
  p1=_loll(ll);
  p5=_hill(ll);
  ll=_amem8(_pix+_ystride*0);
  p2=_loll(ll);
  p6=_hill(ll);
  ll=_amem8(_pix+_ystride*1);
  p3=_loll(ll);
  p7=_hill(ll);
  /*I can't find a way to put the rest in a loop that the compiler thinks is
     unrollable, so instead it's unrolled manually.*/
  /*This first part is based on the transformation
    f = -(3*(p2-p1)+p0-p3+4>>3)
      = -(3*(p2+255-p1)+(p0+255-p3)+4-1020>>3)
      = -(3*(p2+~p1)+(p0+~p3)-1016>>3)
      = 127-(3*(p2+~p1)+(p0+~p3)>>3)
      = 128+~(3*(p2+~p1)+(p0+~p3)>>3) (mod 256).
    Although _avgu4(a,b) = (a+b+1>>1) (biased up), we rely heavily on the
     fact that ~_avgu4(~a,~b) = (a+b>>1) (biased down).*/
  /*We need this first average both biased up and biased down.*/
  u1=~_avgu4(~p1,p2);
  v1=_avgu4(p1,~p2);
  /*The difference controls whether (p3+255-p0>>1) is biased up or down.*/
  m1=_sub4(u1,v1);
  a1=m1^_avgu4(m1^~p0,m1^p3);
  f1=_avgu4(_avgu4(a1,u1),v1);
  /*Instead of removing the bias by 128, we use it to split f by sign, since
     the C64x signed-unsigned addition with unsigned saturation is only
     available for 16-bit operands.
    For 8-bit operands, we have to emulate it with a saturated addition and a
     saturated subtraction using separate unsigned values.
    There's no direct support for 8-bit saturated subtraction, either, so we
     have to emulate that as well, using either x-_minu4(x,y) or
     ~_saddu4(~x,y), depending on which one schedules better.*/
  b1=_minu4(0x80808080,f1);
  a1=0x80808080-b1;
  b1=f1-b1;
  /*Compute f=clamp(0,2*L-abs(f),abs(f)).*/
  u1=_saddu4(a1,_ll);
  v1=_saddu4(b1,_ll);
  a1=_saddu4(a1,u1);
  b1=_saddu4(b1,v1);
  a1=a1-_minu4(a1,u1);
  b1=b1-_minu4(b1,v1);
  /*Apply the changes to the original pixels.*/
  p1=_saddu4(p1-_minu4(p1,b1),a1);
  p2=_saddu4(p2-_minu4(p2,a1),b1);
  /*We need this first average both biased up and biased down.*/
  u2=~_avgu4(~p5,p6);
  v2=_avgu4(p5,~p6);
  /*The difference controls whether (p3+255-p0>>1) is biased up or down.*/
  m2=_sub4(u2,v2);
  a2=m2^_avgu4(m2^~p4,m2^p7);
  f2=_avgu4(_avgu4(a2,u2),v2);
  /*Instead of removing the bias by 128, we use it to split f by sign.*/
  b2=_minu4(0x80808080,f2);
  a2=0x80808080-b2;
  b2=f2-b2;
  /*Compute f=clamp(0,2*L-abs(f),abs(f)).*/
  u2=_saddu4(a2,_ll);
  v2=_saddu4(b2,_ll);
  a2=_saddu4(a2,u2);
  b2=_saddu4(b2,v2);
  a2=a2-_minu4(a2,u2);
  b2=b2-_minu4(b2,v2);
  /*Apply the changes to the original pixels.*/
  p5=_saddu4(p5-_minu4(p5,b2),a2);
  p6=_saddu4(p6-_minu4(p6,a2),b2);
  /*Write out the results.*/
  _amem8(_pix-_ystride)=_itoll(p5,p1);
  _amem8(_pix)=_itoll(p6,p2);
}

void oc_loop_filter_init_c64x(signed char _bv[256],int _flimit){
  int ll;
  ll=_flimit<<1;
  ll=_pack2(ll,ll);
  ll=~_spacku4(ll,ll);
  *((int *)_bv)=ll;
}

void oc_state_loop_filter_frag_rows_c64x(const oc_theora_state *_state,
 signed char _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end){
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  const ptrdiff_t         *frag_buf_offs;
  unsigned char           *ref_frame_data;
  ptrdiff_t                fragi_top;
  ptrdiff_t                fragi_bot;
  ptrdiff_t                fragi0;
  ptrdiff_t                fragi0_end;
  int                      ystride;
  int                      nhfrags;
  int                      ll;
  fplane=_state->fplanes+_pli;
  nhfrags=fplane->nhfrags;
  fragi_top=fplane->froffset;
  fragi_bot=fragi_top+fplane->nfrags;
  fragi0=fragi_top+_fragy0*(ptrdiff_t)nhfrags;
  fragi0_end=fragi_top+_fragy_end*(ptrdiff_t)nhfrags;
  ystride=_state->ref_ystride[_pli];
  frags=_state->frags;
  frag_buf_offs=_state->frag_buf_offs;
  ref_frame_data=_state->ref_frame_data[_refi];
  ll=*((int *)_bv);
  /*The following loops are constructed somewhat non-intuitively on purpose.
    The main idea is: if a block boundary has at least one coded fragment on
     it, the filter is applied to it.
    However, the order that the filters are applied in matters, and VP3 chose
     the somewhat strange ordering used below.*/
  while(fragi0<fragi0_end){
    ptrdiff_t fragi;
    ptrdiff_t fragi_end;
    fragi=fragi0;
    fragi_end=fragi+nhfrags;
    while(fragi<fragi_end){
      if(frags[fragi].coded){
        unsigned char *ref;
        ref=ref_frame_data+frag_buf_offs[fragi];
        if(fragi>fragi0)loop_filter_h(ref,ystride,ll);
        if(fragi0>fragi_top)loop_filter_v(ref,ystride,ll);
        if(fragi+1<fragi_end&&!frags[fragi+1].coded){
          loop_filter_h(ref+8,ystride,ll);
        }
        if(fragi+nhfrags<fragi_bot&&!frags[fragi+nhfrags].coded){
          loop_filter_v(ref+(ystride<<3),ystride,ll);
        }
      }
      fragi++;
    }
    fragi0+=nhfrags;
  }
}
