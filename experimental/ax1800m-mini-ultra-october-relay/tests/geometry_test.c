#include "../src/geometry.h"
#include <assert.h>
#include <stdio.h>
static void put(unsigned char *p,float v){memcpy(p,&v,4);}
int main(void){
  unsigned char src[128]={0},dst[128];
  const float canvases[][2]={{1010,376},{1440,540},{800,480},{1264,640}};
  for(unsigned i=0;i<4;i++){
    put(src+0x10,canvases[i][0]);put(src+0x14,canvases[i][1]);put(src+0x28,canvases[i][0]);put(src+0x2c,canvases[i][1]);
    assert(ax_make_header(dst,1,123,src)==0);assert(ax_float(dst+0x10)==canvases[i][0]);assert(ax_float(dst+0x14)==canvases[i][1]);
  }
  put(src+0x10,1010);put(src+0x14,376);put(src+0x20,151);put(src+0x24,56);put(src+0x28,708);put(src+0x2c,244);
  assert(ax_make_header(dst,1,123,src)==0);assert(ax_float(dst+0x20)==151&&ax_float(dst+0x28)==708);
  put(src+0x28,1010);assert(ax_make_header(dst,1,123,src)<0);
  put(src+0x10,NAN);assert(ax_make_header(dst,1,123,src)<0);
  assert(ax_make_header(dst,1,123,0)<0);assert(ax_make_header(dst,0,123,0)==0);
  puts("GEOMETRY=PASS four_canvases=1 source_rect_retained=1 invalid_config_rejected=1 vehicle=UNTESTED");
}
