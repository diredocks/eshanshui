// shanshui/shanshui.h — 顶层 API（无 UI，直接出二值数组）。
//
// 内存由调用方提供，库内零 malloc/new：
//   整幅：gray[SHANSHUI_GRAY_SIZE]（400×300=120KB，建议放 PSRAM）；
//   分带：grayBand[W*bandH]（如 400×50=20KB）。
// 最深调用链栈峰值约 10KB，建议独立任务 ≥24KB 栈跑 generate。
#pragma once

#include "dither.h"

namespace shanshui {

// cursx 为水平视窗，默认 0 与 web 首屏一致。
void renderGray(uint32_t seed, uint8_t* gray, float cursx = 0);
void renderGrayStr(const char* seed, uint8_t* gray, float cursx = 0);
// 只画全局行 y0..y0+h-1 到 grayBand。
void renderGrayBand(uint32_t seed, uint8_t* grayBand, int y0, int h,
                    float cursx = 0);
void renderGrayStrBand(const char* seed, uint8_t* grayBand, int y0, int h,
                       float cursx = 0);
void generate(uint32_t seed, uint8_t* gray, uint8_t* bits, DitherAlgo algo,
              float cursx = 0);
void generateStr(const char* seed, uint8_t* gray, uint8_t* bits,
                 DitherAlgo algo, float cursx = 0);
// 分带一步到位：dither 误差跨带延续，输出与 generate() 逐位一致。
void generateBanded(uint32_t seed, uint8_t* grayBand, int bandH,
                    uint8_t* bits, DitherAlgo algo, float cursx = 0);
void generateStrBanded(const char* seed, uint8_t* grayBand, int bandH,
                       uint8_t* bits, DitherAlgo algo, float cursx = 0);

} // namespace shanshui
