// shanshui/config.h — 全局可调宏（可用 -D 覆盖）。
#pragma once
#include <stdint.h>

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

// 每字节 8 像素，MSB 先行，bit=1=白，bit=0=黑。
#ifndef SHANSHUI_BITS_SIZE
#define SHANSHUI_BITS_SIZE ((SHANSHUI_W) * (SHANSHUI_H) / 8)
#endif
#ifndef SHANSHUI_GRAY_SIZE
#define SHANSHUI_GRAY_SIZE ((SHANSHUI_W) * (SHANSHUI_H))
#endif

// 分带渲染：按带重放场景。
#ifndef SHANSHUI_BAND_H
#define SHANSHUI_BAND_H 50
#endif
#ifndef SHANSHUI_BAND_SIZE
#define SHANSHUI_BAND_SIZE ((SHANSHUI_W) * (SHANSHUI_BAND_H))
#endif

// 世界->像素：px = (world - cursx) * ZOOM。
#ifndef SHANSHUI_ZOOM
#define SHANSHUI_ZOOM 1.142f
#endif

// 场景分块宽度。
#ifndef SHANSHUI_CWID
#define SHANSHUI_CWID 512
#endif

// Perlin 噪声表长。
#ifndef SHANSHUI_PERLIN_SIZE
#define SHANSHUI_PERLIN_SIZE 256
#endif
#ifndef SHANSHUI_NOISE_OCTAVES
#define SHANSHUI_NOISE_OCTAVES 4
#endif
#ifndef SHANSHUI_NOISE_FALLOFF
#define SHANSHUI_NOISE_FALLOFF 0.5f
#endif

// 笔触/多边形静态上限。
#ifndef SHANSHUI_POLY_MAX
#define SHANSHUI_POLY_MAX 128
#endif
#ifndef SHANSHUI_STROKE_MAX
#define SHANSHUI_STROKE_MAX 96
#endif
#ifndef SHANSHUI_BLOB_N
#define SHANSHUI_BLOB_N 21
#endif

// 山体网格分辨率。
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

// SHANSHUI_LITE=1 时纹理笔数减半。
#ifdef SHANSHUI_LITE
#define SHANSHUI_TEX_DIV 2
#else
#define SHANSHUI_TEX_DIV 1
#endif

// SHANSHUI_DEGAMMA=1 时用 256 项 LUT 做 sRGB->linear。
#ifndef SHANSHUI_DEGAMMA
#define SHANSHUI_DEGAMMA 0
#endif
// SHANSHUI_NOISE_LUT=0 时回到 cosf。
#ifndef SHANSHUI_NOISE_LUT
#define SHANSHUI_NOISE_LUT 1
#endif
#ifndef SHANSHUI_GAMMA
#define SHANSHUI_GAMMA 2.2f
#endif

// 宽须为 8 倍数（bit 打包），噪声表须为 2 的幂（掩码索引）。
#if __cplusplus >= 201103L
static_assert(SHANSHUI_W % 8 == 0, "SHANSHUI_W must be a multiple of 8");
static_assert((SHANSHUI_PERLIN_SIZE & (SHANSHUI_PERLIN_SIZE - 1)) == 0,
              "SHANSHUI_PERLIN_SIZE must be a power of two");
#endif
