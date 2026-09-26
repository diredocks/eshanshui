#include "noise.h"
#include "types.h"
#if SHANSHUI_NOISE_LUT
#include "noise_lut.h"
#endif
#include <math.h>

namespace shanshui {

// Y/Z wrap 常量沿用 p5（4/8），表掩码换成 SHANSHUI_PERLIN_SIZE-1。
#if SHANSHUI_NOISE_LUT
// 0.5*(1-cos(pi*i)) 查 COS_LUT（257 项 + 线性插值），热路径免调 cosf。
static inline float scaledCosine(float i) {
  float u = i * 256.0f;
  int k = (int)u;
  if (k >= 256) return COS_LUT[256];
  float f = u - (float)k;
  float a = COS_LUT[k], b = COS_LUT[k + 1];
  return a + (b - a) * f;
}
#else
static inline float scaledCosine(float i) {
  return 0.5f * (1.0f - cosf(i * kPi));
}
#endif

void Noise::ensure() {
  if (ready_) return;
  for (int i = 0; i < SHANSHUI_PERLIN_SIZE; i++)
    tab_[i] = (uint8_t)(rng_.next() * 255.0f);
  ready_ = true;
}

float Noise::noise(float x, float y, float z) {
  ensure();
  if (x < 0) x = -x;
  if (y < 0) y = -y;
  if (z < 0) z = -z;
  const int YWRAPB = 4, YWRAP = 1 << YWRAPB;
  const int ZWRAPB = 8, ZWRAP = 1 << ZWRAPB;

  int xi = (int)floorf(x), yi = (int)floorf(y), zi = (int)floorf(z);
  float xf = x - (float)xi, yf = y - (float)yi, zf = z - (float)zi;
  float r = 0.0f, ampl = 0.5f;

  // z==0 时 scaledCosine(zf)==0，整段 Z 层插值被 0 乘掉；y==0 同理。
  // 精简路径与原式逐位一致。
  if (z == 0.0f && y == 0.0f) {
    for (int o = 0; o < octaves_; o++) {
      int of = xi;
      float rxf = scaledCosine(xf);
      float n1 = at(of);
      n1 += rxf * (at(of + 1) - n1);
      r += n1 * ampl;
      ampl *= falloff_;
      xi <<= 1;
      xf *= 2.0f;
      if (xf >= 1.0f) { xi++; xf -= 1.0f; }
    }
    return r;
  }
  if (z == 0.0f) {
    for (int o = 0; o < octaves_; o++) {
      int of = xi + (yi << YWRAPB);
      float rxf = scaledCosine(xf), ryf = scaledCosine(yf);
      float n1 = at(of);
      n1 += rxf * (at(of + 1) - n1);
      float n2 = at(of + YWRAP);
      n2 += rxf * (at(of + YWRAP + 1) - n2);
      n1 += ryf * (n2 - n1);
      r += n1 * ampl;
      ampl *= falloff_;
      xi <<= 1;
      xf *= 2.0f;
      yi <<= 1;
      yf *= 2.0f;
      if (xf >= 1.0f) { xi++; xf -= 1.0f; }
      if (yf >= 1.0f) { yi++; yf -= 1.0f; }
    }
    return r;
  }
  for (int o = 0; o < octaves_; o++) {
    int of = xi + (yi << YWRAPB) + (zi << ZWRAPB);
    float rxf = scaledCosine(xf), ryf = scaledCosine(yf);
    float n1 = at(of);
    n1 += rxf * (at(of + 1) - n1);
    float n2 = at(of + YWRAP);
    n2 += rxf * (at(of + YWRAP + 1) - n2);
    n1 += ryf * (n2 - n1);
    of += ZWRAP;
    n2 = at(of);
    n2 += rxf * (at(of + 1) - n2);
    float n3 = at(of + YWRAP);
    n3 += rxf * (at(of + YWRAP + 1) - n3);
    n2 += ryf * (n3 - n2);
    n1 += scaledCosine(zf) * (n2 - n1);
    r += n1 * ampl;
    ampl *= falloff_;
    xi <<= 1;
    xf *= 2.0f;
    yi <<= 1;
    yf *= 2.0f;
    zi <<= 1;
    zf *= 2.0f;
    if (xf >= 1.0f) { xi++; xf -= 1.0f; }
    if (yf >= 1.0f) { yi++; yf -= 1.0f; }
    if (zf >= 1.0f) { zi++; zf -= 1.0f; }
  }
  return r;
}

} // namespace shanshui
