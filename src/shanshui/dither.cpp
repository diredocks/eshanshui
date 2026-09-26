#include "dither.h"
#if SHANSHUI_DEGAMMA
#include <math.h> // 仅 LUT 预计算用一次 powf，热循环无浮点。
#endif

namespace shanshui {

// Bayer4（web Dither.bayer4），阈值 = (b+0.5)*255/16，四舍五入到整数。
static const uint8_t BAYER_TH[16] = {
  8, 135, 40, 167, 199, 72, 231, 104, 56, 183, 24, 151, 247, 120, 215, 88,
};
// 按 (y%4)*4+(x%4) 查表，上表已按该顺序排好：
// bayer4=[0,8,2,10, 12,4,14,6, 3,11,1,9, 15,7,13,5] -> th 如上。

#if SHANSHUI_DEGAMMA
// sRGB->linear LUT（uint8 域：lin255 = round(255*(g/255)^gamma)）。
// 存 float 会把浮点乘/转换带进逐像素热循环；本管线全整数，uint8 零代价。
static uint8_t s_gamma_lut[256];
static bool s_gamma_ready = false;

void initGammaLut() {
  if (s_gamma_ready) return;
  for (int i = 0; i < 256; i++) {
    float lin = powf(i * (1.0f / 255.0f), (float)SHANSHUI_GAMMA);
    int v = (int)(lin * 255.0f + 0.5f);
    s_gamma_lut[i] = (uint8_t)(v > 255 ? 255 : v);
  }
  s_gamma_ready = true;
}

static inline uint8_t degamma(uint8_t g) {
  if (!s_gamma_ready) initGammaLut(); // 兜底：未显式 init 也不出错。
  return s_gamma_lut[g];
}
#else
static inline uint8_t degamma(uint8_t g) { return g; }
#endif

static inline void emitBit(uint8_t* bits, size_t i, uint8_t white) {
  if (white)
    bits[i >> 3] |= (uint8_t)(0x80 >> (i & 7));
  else
    bits[i >> 3] &= (uint8_t)~(0x80 >> (i & 7));
}

static void ditherOrderedBand(const uint8_t* grayBand, uint8_t* bits,
                              int y0, int h, DitherAlgo algo) {
  if (!grayBand || !bits) return;
  if (y0 < 0) { h += y0; y0 = 0; }
  if (y0 + h > SHANSHUI_H) h = SHANSHUI_H - y0;
  if (h <= 0) return;
  // W 为 8 倍数，带边界按字节对齐，可整字节清零。
  size_t bpr = SHANSHUI_W / 8;
  for (size_t i = (size_t)y0 * bpr; i < (size_t)(y0 + h) * bpr; i++)
    bits[i] = 0;
  for (int yl = 0; yl < h; yl++) {
    int y = y0 + yl;
    for (int x = 0; x < SHANSHUI_W; x++) {
      size_t i = (size_t)y * SHANSHUI_W + x;
      uint8_t g = degamma(grayBand[(size_t)yl * SHANSHUI_W + x]);
      uint8_t w;
      if (algo == DITHER_BAYER4)
        w = g > BAYER_TH[(size_t)(y & 3) * 4 + (x & 3)] ? 1 : 0;
      else
        w = g >= 128 ? 1 : 0;
      emitBit(bits, i, w);
    }
  }
}

static void ditherOrdered(const uint8_t* gray, uint8_t* bits, DitherAlgo algo) {
  ditherOrderedBand(gray, bits, 0, SHANSHUI_H, algo);
}

// 行缓冲放 BSS（3*400 int16 = 2.4KB），不占栈。
// s_e1/s_e2 为扩散误差状态：整幅 dither() 每次复位；分带时 ditherBegin()
// 复位一次，带间保持 —— 带缝误差完整延续，与整幅逐位一致。
static int16_t s_row[SHANSHUI_W];
static int16_t s_e1[SHANSHUI_W];
static int16_t s_e2[SHANSHUI_W];

void ditherBegin() {
  for (int x = 0; x < SHANSHUI_W; x++) s_e1[x] = s_e2[x] = 0;
}

static void ditherDiffuseBand(const uint8_t* grayBand, uint8_t* bits, int y0,
                              int h, bool atkinson) {
  if (!grayBand || !bits) return;
  if (y0 < 0) { h += y0; y0 = 0; }
  if (y0 + h > SHANSHUI_H) h = SHANSHUI_H - y0;
  if (h <= 0) return;
  size_t bpr = SHANSHUI_W / 8;
  for (size_t i = (size_t)y0 * bpr; i < (size_t)(y0 + h) * bpr; i++)
    bits[i] = 0;
  for (int yl = 0; yl < h; yl++) {
    int y = y0 + yl;
    const uint8_t* grow = grayBand + (size_t)yl * SHANSHUI_W;
    for (int x = 0; x < SHANSHUI_W; x++) {
      s_row[x] = (int16_t)((int)degamma(grow[x]) + s_e1[x]);
      s_e1[x] = s_e2[x];
      s_e2[x] = 0;
    }
    for (int x = 0; x < SHANSHUI_W; x++) {
      int old = s_row[x];
      int nv = old >= 128 ? 255 : 0;
      emitBit(bits, (size_t)y * SHANSHUI_W + x, nv == 255 ? 1 : 0);
      int err = old - nv;
      if (err == 0) continue;
      if (!atkinson) {
        // Floyd-Steinberg: 右7/16，下行左3/16、中5/16、右1/16。
        if (x + 1 < SHANSHUI_W) s_row[x + 1] += (int16_t)((err * 7 + (err >= 0 ? 8 : -8)) / 16);
        if (y + 1 < SHANSHUI_H) {
          if (x > 0) s_e1[x - 1] += (int16_t)((err * 3 + (err >= 0 ? 8 : -8)) / 16);
          s_e1[x] += (int16_t)((err * 5 + (err >= 0 ? 8 : -8)) / 16);
          if (x + 1 < SHANSHUI_W) s_e1[x + 1] += (int16_t)(err / 16); // 1/16
        }
      } else {
        // Atkinson: 6 邻域各 1/8。
        int q = (err + (err >= 0 ? 4 : -4)) / 8;
        if (x + 1 < SHANSHUI_W) s_row[x + 1] += (int16_t)q;
        if (x + 2 < SHANSHUI_W) s_row[x + 2] += (int16_t)q;
        if (y + 1 < SHANSHUI_H) {
          if (x > 0) s_e1[x - 1] += (int16_t)q;
          s_e1[x] += (int16_t)q;
          if (x + 1 < SHANSHUI_W) s_e1[x + 1] += (int16_t)q;
        }
        if (y + 2 < SHANSHUI_H) s_e2[x] += (int16_t)q;
      }
    }
  }
}

static void ditherDiffuse(const uint8_t* gray, uint8_t* bits, bool atkinson) {
  ditherBegin();
  ditherDiffuseBand(gray, bits, 0, SHANSHUI_H, atkinson);
}

void dither(const uint8_t* gray, uint8_t* bits, DitherAlgo algo) {
  switch (algo) {
    case DITHER_BAYER4:
    case DITHER_THRESHOLD:
      ditherOrdered(gray, bits, algo);
      break;
    case DITHER_ATKINSON:
      ditherDiffuse(gray, bits, true);
      break;
    case DITHER_FLOYD:
    default:
      ditherDiffuse(gray, bits, false);
      break;
  }
}

void ditherBand(const uint8_t* grayBand, uint8_t* bits, int y0, int h,
                DitherAlgo algo) {
  switch (algo) {
    case DITHER_BAYER4:
    case DITHER_THRESHOLD:
      ditherOrderedBand(grayBand, bits, y0, h, algo);
      break;
    case DITHER_ATKINSON:
      ditherDiffuseBand(grayBand, bits, y0, h, true);
      break;
    case DITHER_FLOYD:
    default:
      ditherDiffuseBand(grayBand, bits, y0, h, false);
      break;
  }
}

} // namespace shanshui
