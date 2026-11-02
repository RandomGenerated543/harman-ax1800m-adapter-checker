#ifndef AX_GEOMETRY_H
#define AX_GEOMETRY_H
#include <stdint.h>
#include <string.h>
#include <math.h>
static float ax_float(const unsigned char *p) {float v;memcpy(&v,p,4);return v;}
/* Transfer actual source config geometry; never invent a SEAT/Skoda canvas. */
static int ax_make_header(unsigned char out[128],int type,uint32_t bytes,const unsigned char *cfg) {
  memset(out,0,128);memcpy(out,&bytes,4);out[4]=(unsigned char)type;
  if(type==0)return 0;
  if(type!=1||!cfg)return -1;
  const unsigned offsets[]={0x10,0x14,0x20,0x24,0x28,0x2c};
  for(unsigned i=0;i<6;i++)if(!isfinite(ax_float(cfg+offsets[i])))return -1;
  float w=ax_float(cfg+0x10),h=ax_float(cfg+0x14);
  float x=ax_float(cfg+0x20),y=ax_float(cfg+0x24),vw=ax_float(cfg+0x28),vh=ax_float(cfg+0x2c);
  if(w<16||h<16||w>8192||h>8192||x<0||y<0||vw<=0||vh<=0||x+vw>w||y+vh>h)return -1;
  for(unsigned i=0;i<6;i++)memcpy(out+offsets[i],cfg+offsets[i],4);
  return 0;
}
#endif
