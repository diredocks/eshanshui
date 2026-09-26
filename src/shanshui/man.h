// shanshui/man.h — 人物（web/man.js 的 MCU 版）。
#pragma once

#include "brush.h"

namespace shanshui {

class Man {
 public:
  enum ClothMode : uint8_t { CM_SLEEVE = 0, CM_BODY, CM_HEAD };
  Man(Prng& rng, Noise& noise, Brush& brush, Raster& raster)
      : rng_(rng), noise_(noise), brush_(brush), ras_(raster) {}

  // hat: 0=hat01(斗笠), 1=hat02(箬帽); ite: 0=无, 1=stick01(船篙)。
  // lenMul: 9 个长度系数（null 用默认 [0,30,20,30,30,30,30,30,30]）。
  void man(float xoff, float yoff, float sca, bool fli, int hat = 0,
           int ite = 0, const float* lenMul = nullptr);

 private:
  Prng& rng_;
  Noise& noise_;
  Brush& brush_;
  Raster& ras_;
  int expand(const Pt* pts, int n, ClothMode mode, float sca, Pt* o0, Pt* o1,
             int maxOut);
  void tranpoly(const Pt& p0, const Pt& p1, const float* local, int n, bool fli,
                Pt* out);
  void hat01(const Pt& p0, const Pt& p1, bool fli);
  void hat02(const Pt& p0, const Pt& p1, bool fli);
  void stick01(const Pt& p0, const Pt& p1, bool fli);
  void cloth(const Pt* plist, int n, ClothMode mode, float sca, float xoff,
             float yoff, bool fli);
};

} // namespace shanshui
