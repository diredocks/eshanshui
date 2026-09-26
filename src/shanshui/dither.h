// shanshui/dither.h — 1-bit 抖动。
// 输入 gray[W*H]（0=黑..255=白），输出 bits[W*H/8]（bit=1=白，MSB 先行）。
#pragma once

#include "types.h"

namespace shanshui {

enum DitherAlgo : uint8_t {
  DITHER_THRESHOLD = 0,
  DITHER_BAYER4 = 1,
  DITHER_FLOYD = 2,
  DITHER_ATKINSON = 3,
};

void dither(const uint8_t* gray, uint8_t* bits, DitherAlgo algo);

// 分带抖动：grayBand 为 W*h（全局行 y0..y0+h-1），结果写入 bits 对应段。
// 扩散抖动的误差跨带延续，与整幅 dither() 逐位一致。
void ditherBegin();
void ditherBand(const uint8_t* grayBand, uint8_t* bits, int y0, int h,
                DitherAlgo algo);

#if SHANSHUI_DEGAMMA
// 预计算 sRGB->linear LUT（boot 时调一次；dither 内有 lazy 兜底）。
void initGammaLut();
#endif

} // namespace shanshui
