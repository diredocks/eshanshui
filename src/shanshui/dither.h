// shanshui/dither.h — 1-bit 抖动（web/js/ui/dither.js 的 MCU 版）。
// 四种算法：THRESHOLD / BAYER4 / FLOYD / ATKINSON。
// 输入 gray[W*H]（0=黑..255=白），输出 bits[W*H/8]（bit=1=白，MSB 先行）。
// Floyd/Atkinson 用 3 行 int16 行缓冲（约 2.4KB BSS），不需整幅 float 图。
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

// 分带抖动（无 PSRAM 时用）：grayBand 为 W*h（全局行 y0..y0+h-1），
// 结果写入 bits 全局位流对应段。有序抖动无状态；扩散抖动的误差行
// (s_e1/s_e2) 由库内保持，带间调用 ditherBegin() 复位一次即可，
// 之后逐带调 ditherBand —— 与整幅 dither() 逐位一致。
void ditherBegin();
void ditherBand(const uint8_t* grayBand, uint8_t* bits, int y0, int h,
                DitherAlgo algo);

#if SHANSHUI_DEGAMMA
// 预计算 sRGB->linear LUT（256 项，空间换时间，boot 时调一次，约 256 次 powf）。
// dither() 内有 lazy 兜底（忘调也不会黑屏），但建议在 setup() 里显式调用。
void initGammaLut();
#endif

// 读取打包像素：1=白，0=黑。
inline uint8_t getBit(const uint8_t* bits, int x, int y) {
  size_t i = (size_t)y * SHANSHUI_W + (size_t)x;
  return (bits[i >> 3] >> (7 - (i & 7))) & 1;
}

} // namespace shanshui
