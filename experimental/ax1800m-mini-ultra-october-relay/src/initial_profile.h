#ifndef INITIAL_PROFILE_H
#define INITIAL_PROFILE_H
#include <stdint.h>
#include <string.h>
/* Initial cluster-map fallback only. Dynamic map commands are not bridged. */
#define INITIAL_CLUSTER_MAP_URL "maps:/car/instrumentcluster/map"
static int stream_has_h264_idr(const unsigned char *b, unsigned len) {
  unsigned pos = 0;
  while (len - pos >= 5) {
    uint32_t n = ((uint32_t)b[pos] << 24) | ((uint32_t)b[pos+1] << 16)
               | ((uint32_t)b[pos+2] << 8) | b[pos+3];
    if (!n || n > len - pos - 4) return 0;
    if ((b[pos+4] & 0x1f) == 5) return 1;
    pos += 4 + n;
  }
  return 0;
}
#endif
