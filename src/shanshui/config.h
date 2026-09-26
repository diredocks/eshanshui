// shanshui/config.h — 全局可调宏（可在编译时 -D 覆盖，无需改代码）.
// 目标：Arduino/ESP32 + 桌面跨平台。默认输出 400x300 二值图。
#pragma once
#include <stdint.h>

// ---- 只读数据放 flash（PROGMEM）----
// ESP32 上 PROGMEM 为空宏：static const 默认进 .rodata（flash 映射），直接读即可；
// AVR 上 PROGMEM 为真 flash 段。本库目标为 Arduino/ESP32 + 桌面跨平台，读表一律
// 直接访问（ESP32/桌面/ESP8266 正确；AVR 不在目标内，不用 pgm_read_* 改写调用方）。
// 写法统一为 `static const T X[] PROGMEM = {...}`，意图明确且跨核可移植。
// 注意：用作编译期常量（如数组界 MTX_N）的不加 PROGMEM。
#if defined(ARDUINO)
#include <pgmspace.h>
#elif !defined(PROGMEM)
#define PROGMEM
#endif

#ifndef SHANSHUI_W
#define SHANSHUI_W 400
#endif
#ifndef SHANSHUI_H
#define SHANSHUI_H 300
#endif

// 每字节存 8 像素，MSB 先行，bit=1=白(纸)，bit=0=黑(墨)。
#ifndef SHANSHUI_BITS_SIZE
#define SHANSHUI_BITS_SIZE ((SHANSHUI_W) * (SHANSHUI_H) / 8)
#endif
#ifndef SHANSHUI_GRAY_SIZE
#define SHANSHUI_GRAY_SIZE ((SHANSHUI_W) * (SHANSHUI_H))
#endif

// 分带渲染：无 PSRAM 时整幅 gray（120KB）堆不上，改按带重放场景。
// grayBand 缓冲只需 W*BAND_H（如 400x50=20KB），逐带 render+dither。
// BAND_H 不必整除 H，generateBanded 末带自动收窄。
#ifndef SHANSHUI_BAND_H
#define SHANSHUI_BAND_H 50
#endif
#ifndef SHANSHUI_BAND_SIZE
#define SHANSHUI_BAND_SIZE ((SHANSHUI_W) * (SHANSHUI_BAND_H))
#endif

// 世界->像素映射：px = (world - cursx) * ZOOM（与 web/js/scene/view.js 一致）。
#ifndef SHANSHUI_ZOOM
#define SHANSHUI_ZOOM 1.142f
#endif

// 场景分块宽度（与 web/js/scene/state.js cwid 一致）。
#ifndef SHANSHUI_CWID
#define SHANSHUI_CWID 512
#endif

// Perlin 噪声：表长 256（原 web 为 4096，MCU 上太占 RAM），octaves 默认 4。
#ifndef SHANSHUI_PERLIN_SIZE
#define SHANSHUI_PERLIN_SIZE 256
#endif
#ifndef SHANSHUI_NOISE_OCTAVES
#define SHANSHUI_NOISE_OCTAVES 4
#endif
#ifndef SHANSHUI_NOISE_FALLOFF
#define SHANSHUI_NOISE_FALLOFF 0.5f
#endif

// 笔触/多边形静态上限（内部临时数组放 BSS，不做堆分配、不吃栈）。
#ifndef SHANSHUI_POLY_MAX
#define SHANSHUI_POLY_MAX 128
#endif
#ifndef SHANSHUI_STROKE_MAX
#define SHANSHUI_STROKE_MAX 96
#endif
#ifndef SHANSHUI_BLOB_N
#define SHANSHUI_BLOB_N 21
#endif

// 山体分辨率：mountain [10,50]，flatMount [5,50]，与 web 一致。
#ifndef SHANSHUI_MOUNT_I
#define SHANSHUI_MOUNT_I 10
#endif
#ifndef SHANSHUI_MOUNT_J
#define SHANSHUI_MOUNT_J 50
#endif
#ifndef SHANSHUI_FLAT_I
#define SHANSHUI_FLAT_I 5
#endif
#ifndef SHANSHUI_FLAT_J
#define SHANSHUI_FLAT_J 50
#endif

// SHANSHUI_LITE=1 时纹理笔数减半（ESP32 低内存/求快时 -DSHANSHUI_LITE=1）。
#ifdef SHANSHUI_LITE
#define SHANSHUI_TEX_DIV 2
#else
#define SHANSHUI_TEX_DIV 1
#endif

// 抖动去 gamma：默认关闭（桌面如需可 -DSHANSHUI_DEGAMMA=1 打开）。
// 打开后以 256 项 LUT 做 sRGB->linear（启动时预计算，逐像素只查表）。
#ifndef SHANSHUI_DEGAMMA
#define SHANSHUI_DEGAMMA 0
#endif
// 噪声淡入曲线用 COS_LUT（257 项查表）替代 cosf：热路径省掉 cosf，误差 <1e-5。
// 需要与 web 逐位一致时 -DSHANSHUI_NOISE_LUT=0 关闭（回到 0.5*(1-cos)）。
#ifndef SHANSHUI_NOISE_LUT
#define SHANSHUI_NOISE_LUT 1
#endif
// Gamma 指数（sRGB 近似，精确分段公式可用 2.2 等价代替，误差 < 1 LSB）。
#ifndef SHANSHUI_GAMMA
#define SHANSHUI_GAMMA 2.2f
#endif

// 编译期约束：宽须为 8 倍数（bit 打包），噪声表须为 2 的幂（掩码索引）。
#if __cplusplus >= 201103L
static_assert(SHANSHUI_W % 8 == 0, "SHANSHUI_W must be a multiple of 8");
static_assert((SHANSHUI_PERLIN_SIZE & (SHANSHUI_PERLIN_SIZE - 1)) == 0,
              "SHANSHUI_PERLIN_SIZE must be a power of two");
#endif
