#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

namespace obd {
inline bool decode(uint8_t pid, const uint8_t *d, size_t n, float &v) {
  const size_t need = (pid == 0x0C || pid == 0x10 || pid == 0x42) ? 2 : 1;
  if (n < need) return false;
  switch (pid) {
    case 0x04: case 0x11: case 0x2F: v = d[0] * 100.0f / 255; return true;
    case 0x05: case 0x0F: v = int(d[0]) - 40; return true;
    case 0x0C: v = (256 * d[0] + d[1]) / 4.0f; return true;
    case 0x0D: v = d[0]; return true;
    case 0x10: v = (256 * d[0] + d[1]) / 100.0f; return true;
    case 0x42: v = (256 * d[0] + d[1]) / 1000.0f; return true;
    default: return false;
  }
}
inline bool supported(uint32_t bits, uint8_t base, uint8_t pid) {
  return pid > base && pid <= base + 32 && (bits & (1UL << (32 - (pid - base))));
}
inline void dtc(uint8_t a, uint8_t b, char out[6]) {
  snprintf(out, 6, "%c%u%X%02X", "PCBU"[a >> 6], (a >> 4) & 3, a & 15, b);
}
// One bounded ISO-TP receive transaction. Caller enforces source ID and timeout.
struct IsoTp {
  uint8_t data[128]{};
  size_t size = 0, used = 0;
  uint8_t next = 1;
  bool active = false;
  void reset() { size = used = 0; next = 1; active = false; }
  // -1 malformed, 0 ignored, 1 complete, 2 first frame (send flow control), 3 progressing
  int feed(const uint8_t *d, size_t n) {
    if (!n || n > 8) return -1;
    uint8_t kind = d[0] >> 4;
    if (kind == 0) {
      reset(); size = d[0] & 15;
      if (!size || size > 7 || size + 1 > n) { reset(); return -1; }
      for (size_t i=0;i<size;i++) data[i]=d[i+1];
      used = size; return 1;
    }
    if (kind == 1) {
      reset(); if (n != 8) return -1;
      size = ((d[0] & 15) << 8) | d[1];
      if (size <= 7 || size > sizeof(data)) { reset(); return -1; }
      for (size_t i=2;i<n;i++) data[used++]=d[i];
      active = true; return 2;
    }
    if (kind == 2 && active) {
      if ((d[0] & 15) != next || n < 2 || (size-used > n-1 && n != 8)) { reset(); return -1; }
      next = (next + 1) & 15;
      for(size_t i=1;i<n && used<size;i++) data[used++]=d[i];
      if(used==size) { active=false; return 1; }
      return 3;
    }
    return 0;
  }
};
}
