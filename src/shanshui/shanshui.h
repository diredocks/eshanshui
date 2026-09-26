// shanshui/shanshui.h — 顶层 API（无 UI，直接出二值数组）。
//
// 内存（调用方提供，库内零 malloc/new）：
//   整幅：gray[SHANSHUI_GRAY_SIZE]  uint8 灰度（400×300 = 120KB）——有 PSRAM
//         时放 PSRAM（如 heap_caps_malloc(..., MALLOC_CAP_SPIRAM)），场景只渲染
//         1 次，比分带快约 ×带数。
//   分带：grayBand[W*bandH]         如 400×50 = 20KB（无 PSRAM 板用这个）
//   bits[SHANSHUI_BITS_SIZE]  打包二值（15KB，bit=1=白/MSB先行）
// BSS（库内部静态）：网格 ~10KB＋笔触/光栅/抖动行缓冲 ~7KB。
// 栈：最深调用链（船→人→衣→笔触，cloth/man/flatMount 大数组在栈上）峰值
// 约 10KB，Arduino loopTask 默认 8KB 不够 —— 建议独立任务 ≥24KB 栈跑
// generate（跑完即删），见 eshanshui.ino 的 genTaskFn。
//
// 用法（有 PSRAM，推荐，最快）:
//   uint8_t* gray = (uint8_t*)heap_caps_malloc(SHANSHUI_GRAY_SIZE, MALLOC_CAP_SPIRAM);
//   static uint8_t bits[SHANSHUI_BITS_SIZE];
//   shanshui::generate(20260925u, gray, bits, shanshui::DITHER_FLOYD);
//   heap_caps_free(gray);
// 用法（无 PSRAM，分带，慢约 ×带数，输出逐位一致）:
//   static uint8_t band[SHANSHUI_BAND_SIZE], bits[SHANSHUI_BITS_SIZE];
//   shanshui::generateBanded(20260925u, band, SHANSHUI_BAND_H, bits,
//                            shanshui::DITHER_ATKINSON);
#pragma once
#include "dither.h"

namespace shanshui {

// 灰度渲染（seed 确定画面；cursx 为水平视窗，默认 0 与 web 首屏一致）。
void renderGray(uint32_t seed, uint8_t* gray, float cursx = 0);
void renderGrayStr(const char* seed, uint8_t* gray, float cursx = 0);
// 分带灰度渲染：只画全局行 y0..y0+h-1 到 grayBand（W*h，调用方保证容量）。
void renderGrayBand(uint32_t seed, uint8_t* grayBand, int y0, int h,
                    float cursx = 0);
void renderGrayStrBand(const char* seed, uint8_t* grayBand, int y0, int h,
                       float cursx = 0);
// 一步到位：渲染＋抖动。
void generate(uint32_t seed, uint8_t* gray, uint8_t* bits, DitherAlgo algo,
              float cursx = 0);
void generateStr(const char* seed, uint8_t* gray, uint8_t* bits,
                 DitherAlgo algo, float cursx = 0);
// 分带一步到位：grayBand 只需 W*bandH；dither 误差跨带延续，
// 输出 bits 与整幅 generate() 逐位一致。
void generateBanded(uint32_t seed, uint8_t* grayBand, int bandH,
                    uint8_t* bits, DitherAlgo algo, float cursx = 0);
void generateStrBanded(const char* seed, uint8_t* grayBand, int bandH,
                       uint8_t* bits, DitherAlgo algo, float cursx = 0);

} // namespace shanshui
