// shanshui/noise.h — Perlin 值噪声（web/noise.js 的 MCU 版）。
#pragma once

#include "config.h"
#include "prng.h"

namespace shanshui {

class Noise {
 public:
  explicit Noise(Prng& rng) : rng_(rng), ready_(false) {}
  float noise(float x, float y = 0.0f, float z = 0.0f);
  void setDetail(int octaves, float falloff) {
    if (octaves > 0) octaves_ = octaves;
    if (falloff > 0.0f) falloff_ = falloff;
  }

 private:
  Prng& rng_;
  bool ready_;
  int octaves_ = SHANSHUI_NOISE_OCTAVES;
  float falloff_ = SHANSHUI_NOISE_FALLOFF;
  uint8_t tab_[SHANSHUI_PERLIN_SIZE];
  void ensure();
  inline float at(int idx) const {
    return (float)tab_[idx & (SHANSHUI_PERLIN_SIZE - 1)] * (1.0f / 255.0f);
  }
};

} // namespace shanshui
