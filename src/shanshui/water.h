// shanshui/water.h — 水纹（web/js/scene/water.js 的 MCU 版）。
#pragma once
#include "brush.h"

namespace shanshui {

class Water {
 public:
  Water(Prng& rng, Noise& noise, Brush& brush)
      : rng_(rng), noise_(noise), brush_(brush) {}

  void water(float xoff, float yoff, float seed);

 private:
  Prng& rng_;
  Noise& noise_;
  Brush& brush_;
};

} // namespace shanshui
